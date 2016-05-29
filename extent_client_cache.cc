// RPC stubs for clxents to talk to extent_server

#include "extent_client_cache.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <slock.h>
#include <jsl_log.h>

// The calls assume that the caller holds a lock on the extent

extent_client_cache::extent_client_cache(std::string dst) {
  ec = new extent_client(dst);
}

extent_protocol::status extent_client_cache::get(extent_protocol::extentid_t eid, 
                                                 std::string &buf) {
  extent_protocol::status res = extent_protocol::OK;
  bool found = true;
  {
    ScopedLock ml(&mutex); 
    jsl_log(JSL_DBG_ME, "extent_client_cache: get %lud %llu\n", pthread_self(), eid);
    if (kv_store.find(eid) == kv_store.end()) {
      found = false;
    }
  }
  Value copy;
  if (!found) { // not found locally
    // TODO -> ajouter une opération sur le serveur qui renvoie tout ?
    res = ec->get(eid, copy.buf); 
    if (res != extent_protocol::OK) {
      return res;
    }
    res = ec->getattr(eid, copy.attr);
    if (res != extent_protocol::OK) {
      return res;
    }
  }
  // qu'est ce qui peut arriver quand on n'a pas le verrou ici ?

  {
    ScopedLock ml(&mutex); 
    Value &v = kv_store[eid];
    if (!found) {
      kv_store[eid] = copy;
    }
    buf = v.buf;
  }
  return res;
}

extent_protocol::status extent_client_cache::getattr(extent_protocol::extentid_t eid, 
                                                     extent_protocol::attr &attr) {
  extent_protocol::status res = extent_protocol::OK;
  bool found = true;
  {
    ScopedLock ml(&mutex); 
    jsl_log(JSL_DBG_ME, "extent_client_cache: getattr %lud %llu\n", pthread_self(), eid);
    if (kv_store.find(eid) == kv_store.end()) {
      found = false;
    }
  }
  Value copy;
  if (!found) { // not found locally
    // TODO -> ajouter une opération sur le serveur qui renvoie tout ?
    res = ec->get(eid, copy.buf); 
    if (res != extent_protocol::OK) {
      return res;
    }
    res = ec->getattr(eid, copy.attr);
    if (res != extent_protocol::OK) {
      return res;
    }
  }
  // qu'est ce qui peut arriver quand on n'a pas le verrou ici ?

  {
    ScopedLock ml(&mutex); 
    Value &v = kv_store[eid];
    if (!found) {
      kv_store[eid] = copy;
    }
    attr = v.attr;
  }
  return res;
}

extent_protocol::status extent_client_cache::put(extent_protocol::extentid_t eid,
                                                 std::string buf) {
  ScopedLock ml(&mutex);
  jsl_log(JSL_DBG_ME, "extent_client_cache: put %lud %llu\n", pthread_self(), eid);
  Value &v = kv_store[eid];
  v.buf = buf;
  v.attr.size = buf.size();
  v.dirty = true;
  time_t cur_time = time(NULL);

  // TODO why change ctime at every access?
  v.attr.ctime = cur_time;
  v.attr.mtime = cur_time;
  return extent_protocol::OK;
}

extent_protocol::status extent_client_cache::remove(
    extent_protocol::extentid_t eid) {
  ScopedLock ml(&mutex);
  jsl_log(JSL_DBG_ME, "extent_client_cache remove %lud %llu\n", pthread_self(), eid);
  if (kv_store.find(eid) == kv_store.end()) {
    return extent_protocol::NOENT;
  }
  kv_store.erase(eid);
  return extent_protocol::OK;
}

void extent_client_cache::flush(extent_protocol::extentid_t eid) {
  ScopedLock ml(&mutex);
  jsl_log(JSL_DBG_ME, "extent_client_cache: flush %lud %llu %p\n", pthread_self(), eid, this);
  extent_protocol::status res = extent_protocol::OK;

  if (kv_store.find(eid) == kv_store.end()) {
    jsl_log(JSL_DBG_ME, "extent_client_cache: flush - remove %llu\n", eid);
    res = ec->remove(eid); 
    // TODO pas toujours bon... test-lab-3-b 
    VERIFY(res == extent_protocol::OK);
    return;
  }
  
  Value &v = kv_store[eid];
  if (v.dirty) {
    jsl_log(JSL_DBG_ME, "extent_client_cache: flush - write back %llu buf = %s\n ", eid, v.buf.c_str());
    // writeback
    res = ec->put(eid, v.buf);
    VERIFY(res == extent_protocol::OK);
  }
  jsl_log(JSL_DBG_ME, "extent_client_cache: flush - erase %llu\n", eid);
  kv_store.erase(eid);
  return;
}


