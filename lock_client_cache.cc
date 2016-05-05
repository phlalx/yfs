// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "tprintf.h"
#include "jsl_log.h"

lock_client_cache::lock_client_cache(std::string xdst, 
				     class lock_release_user *_lu)
  : lock_client(xdst), lu(_lu)
{
  rpcs *rlsrpc = new rpcs(0);
  rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::revoke_handler);
  rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::retry_handler);

  const char *hname;
  hname = "127.0.0.1";
  std::ostringstream host;
  host << hname << ":" << rlsrpc->port();
  id = host.str();
}

lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid)
{
  const lock_protocol::status ret = lock_protocol::OK;
  { 
    ScopedLock ml(&mutex);
    jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: acquire\n", id.c_str(), pthread_self(), lid);
    lock_info &li = locks[lid];

    if (li.st == FREE) {
      li.set(this, lid, LOCKED);
      return ret;
    }

    if (li.st != NONE) {
    // For now, suppose there's only one client thread
    // meaning li.st == NONE or FREE
      VERIFY(0);
    }
    li.set(this, lid, ACQUIRING);
  }

  jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: server.acquire\n", id.c_str(), pthread_self(), lid);
  int r;
  lock_protocol::status acquire_ret = cl->call(lock_protocol::acquire, lid, id, r);

  { 
    ScopedLock ml(&mutex);
    lock_info &li = locks[lid];

    if (acquire_ret == lock_protocol::OK) {
      jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: server.acquire -> OK\n", id.c_str(), pthread_self(), lid);
      VERIFY(li.st == ACQUIRING || !li.is_retried);
      li.set(this, lid, li.is_revoked?RELEASING:LOCKED);
      return ret;
    }

    jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: server.acquire -> RETRY\n", id.c_str(), pthread_self(), lid);
    VERIFY(acquire_ret == lock_protocol::RETRY);

    while (!li.is_retried) {
      pthread_cond_wait(&li.waiting_retry, &mutex);
    }

    jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: woken up by retry rpc\n", id.c_str(), pthread_self(), lid);

    li.set(this, lid, li.is_revoked?RELEASING:LOCKED);
    return ret;
  }
}

lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid)
{
  const lock_protocol::status ret = lock_protocol::OK;
  { 
    ScopedLock ml(&mutex); 
    jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: release\n", id.c_str(), pthread_self(), lid);
    lock_info &li = locks[lid];
    if (li.st == LOCKED) { 
      li.set(this, lid, FREE); 
      return ret;
    }
    VERIFY(li.st == RELEASING);
    li.set(this, lid, NONE);
  }

  jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: server.release\n", id.c_str(), pthread_self(), lid);
  int r;
  VERIFY(cl->call(lock_protocol::release, lid, id, r) == lock_protocol::OK);
  return ret; 
}

rlock_protocol::status
lock_client_cache::revoke_handler(lock_protocol::lockid_t lid, int &)
{
  bool to_release = false;
  {
    ScopedLock ml(&mutex);
    jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: revoke\n", id.c_str(), pthread_self(), lid);
    lock_info &li = locks[lid];
    li.is_revoked = true;
    if (li.st == FREE) {
      li.set(this, lid, NONE);
      to_release = true;
    } else if (li.st == LOCKED) {
      li.set(this, lid, RELEASING);
    } else if (li.st == ACQUIRING) {
      //
    } else { // NONE or RELEASING 
      VERIFY(0);
    }
  }
  if (to_release) {
   jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: server.release\n", id.c_str(), pthread_self(), lid);
   int r;
   VERIFY(cl->call(lock_protocol::release, lid, id, r) == lock_protocol::OK);
  }
  return rlock_protocol::OK;
}

rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid, int &)
{
  ScopedLock ml(&mutex);
  jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: retry\n", id.c_str(), pthread_self(), lid);
  lock_info &li = locks[lid];
  li.is_retried = true;
  VERIFY(li.st == ACQUIRING);
  pthread_cond_signal(&li.waiting_retry); // not always necessary
  int ret = rlock_protocol::OK;
  return ret;
}

