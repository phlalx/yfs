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
  jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: acquire\n", id.c_str(), pthread_self(), lid);
  pthread_mutex_lock(&mutex);

  lock_protocol::status ret = lock_protocol::OK;

  lock_info &li = locks[lid];

  while (li.st == LOCKED || li.st == ACQUIRING || li.st == RELEASING || li.st == ACQUIRING_RELEASING) {
    VERIFY(0);
    jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: client thread waiting\n", id.c_str(), pthread_self(), lid);
    li.number_waiting++; 
    pthread_cond_wait(&li.waiting_local, &mutex);
    li.number_waiting--; 
    jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: wait completed\n", id.c_str(), pthread_self(), lid);
  }
  // either st is NONE, or FREE

  // FREE is the easiest case, nothing to do except return 
  // and changing the lock state
  if (li.st == FREE) {
    li.set(this, lid, LOCKED);
    pthread_mutex_unlock(&mutex);
    return ret;
  }

  VERIFY(li.st == NONE);
  // gotta acquire the lock from the server

  int r;
  li.set(this, lid, ACQUIRING);

  // releasing the mutex as we don't want to RPC while holding the mutex
  pthread_mutex_unlock(&mutex);

  jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: server.acquire\n", id.c_str(), pthread_self(), lid);
  lock_protocol::status ret2 = cl->call(lock_protocol::acquire, lid, id, r);

  // get the mutex back
  pthread_mutex_lock(&mutex);
  if (ret2 == lock_protocol::OK)
  {
    jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: server.acquire -> OK\n", id.c_str(), pthread_self(), lid);
    VERIFY(li.st == ACQUIRING || li.st == ACQUIRING_RELEASING);
    if (li.st == ACQUIRING_RELEASING) {
      li.set(this, lid, RELEASING);
    } else {
      li.set(this, lid, LOCKED);
    }
    pthread_mutex_unlock(&mutex);
    return ret;
  }

  jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: server.acquire -> RETRY\n", id.c_str(), pthread_self(), lid);
  VERIFY(ret2 == lock_protocol::RETRY);

  if (!li.is_revoked) {
    pthread_cond_wait(&li.waiting_local, &mutex);
  }
  li.is_revoked = false;

  jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: woken up by retry rpc from server\n", id.c_str(), pthread_self(), lid);
  // finalement, on obtient le verrou
  if (li.st == ACQUIRING_RELEASING) {
    li.set(this, lid, RELEASING);
  } else {
    li.set(this, lid, LOCKED);
  }
  pthread_mutex_unlock(&mutex);
  return ret;
}

lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid)
{
  jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: release\n", id.c_str(), pthread_self(), lid);
  pthread_mutex_lock(&mutex);

  lock_info &li = locks[lid];
  if (li.st == LOCKED) {
    li.set(this, lid, FREE); 
    jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: signal client thread\n", id.c_str(), pthread_self(), lid);
    // wake up a sleeping thread, if any
    pthread_cond_signal(&li.waiting_local);
    pthread_mutex_unlock(&mutex);
  } else if (li.st == RELEASING) {
    bool acquire = false;
    if (li.number_waiting == 0) {
      li.set(this, lid, NONE);
      pthread_mutex_unlock(&mutex);
    } else {
      li.set(this, lid, ACQUIRING);
      acquire = true;
      pthread_mutex_unlock(&mutex);
    }
    jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: server.release\n", id.c_str(), pthread_self(), lid);
    int r;
    lock_protocol::status ret2 = cl->call(lock_protocol::release, lid, id, r);
    if (acquire) {
      VERIFY(0);
    }
  } else {
    VERIFY(0);
  }

  return lock_protocol::OK;
}

rlock_protocol::status
lock_client_cache::revoke_handler(lock_protocol::lockid_t lid, int &)
{
  jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: revoke\n", id.c_str(), pthread_self(), lid);
  ScopedLock ml(&mutex);
  lock_info &li = locks[lid];
  li.is_revoked = true;
  if (li.st == FREE) {
   jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: server.release\n", id.c_str(), pthread_self(), lid);
   int r;
   lock_protocol::status ret2 = cl->call(lock_protocol::release, lid, id, r); 
   return rlock_protocol::OK;
  } else if (li.st == LOCKED) {
    li.set(this, lid, RELEASING);
    // englobe this with a while condition - spurious WUs
    //jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: wait for relase from client thread\n", id.c_str(), pthread_self(), lid);
    //pthread_cond_wait(&li.waiting_from_local, &mutex);
    //jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: wait completed - released\n", id.c_str(), pthread_self(), lid);
    return rlock_protocol::OK;
  } else if (li.st == ACQUIRING) {
    // received revoke before retry
    li.set(this, lid, ACQUIRING_RELEASING);
    return rlock_protocol::OK;
  } else if (li.st == NONE) {
    VERIFY(0);
  } else if (li.st == RELEASING) {
    VERIFY(0);
  } else {
    VERIFY(0);
  }
  
  return rlock_protocol::OK;
}

rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid, int &)
{
  jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: retry\n", id.c_str(), pthread_self(), lid);
  // waking next client

  ScopedLock ml(&mutex);
  lock_info &li = locks[lid];
  VERIFY(li.st == ACQUIRING);
//  li.set(this, lid, LOCKED);
  pthread_cond_signal(&li.waiting_local);
  int ret = rlock_protocol::OK;
  return ret;
}



