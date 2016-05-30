#include "extent_client_cache.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <slock.h>
#include <jsl_log.h>

extent_client_cache::extent_client_cache(const std::string &dst) : extent_client(dst) {
}

extent_protocol::status extent_client_cache::get(extent_protocol::extentid_t eid, 
                                                 std::string &buf) {
  extent_protocol::status res = extent_protocol::OK;
  bool found = true;
  {
    ScopedLock ml(&mutex_); 
    jsl_log(JSL_DBG_ME, "extent_client_cache: get %lud %llu\n", pthread_self(), eid);
    if (store_.find(eid) == store_.end()) {
      found = false;
    }
  }
  Value copy;
  if (!found) { 
    // TODO(phil) ajouter une opération sur le serveur qui renvoie tout ?
    res = extent_client::get(eid, copy.buf); 
    if (res != extent_protocol::OK) {
      return res;
    }
    res = extent_client::getattr(eid, copy.attr);
    if (res != extent_protocol::OK) {
      return res;
    }
  }
  // qu'est ce qui peut arriver quand on n'a pas le verrou ici ?

  {
    ScopedLock ml(&mutex_); 
    Value &v = store_[eid];
    if (!found) {
      store_[eid] = copy;
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
    ScopedLock ml(&mutex_); 
    jsl_log(JSL_DBG_ME, "extent_client_cache: getattr %lud %llu\n", pthread_self(), eid);
    if (store_.find(eid) == store_.end()) {
      found = false;
    }
  }
  Value copy;
  if (!found) { // not found locally
    // TODO -> ajouter une opération sur le serveur qui renvoie tout ?
    res = extent_client::get(eid, copy.buf); 
    if (res != extent_protocol::OK) {
      return res;
    }
    res = extent_client::getattr(eid, copy.attr);
    if (res != extent_protocol::OK) {
      return res;
    }
  }
  // qu'est ce qui peut arriver quand on n'a pas le verrou ici ?

  {
    ScopedLock ml(&mutex_); 
    Value &v = store_[eid];
    if (!found) {
      store_[eid] = copy;
    }
    attr = v.attr;
  }
  return res;
}

extent_protocol::status extent_client_cache::put(extent_protocol::extentid_t eid,
                                                 std::string buf) {
  ScopedLock ml(&mutex_);
  jsl_log(JSL_DBG_ME, "extent_client_cache: put %lud %llu\n", pthread_self(), eid);
  Value &v = store_[eid];
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
  ScopedLock ml(&mutex_);
  jsl_log(JSL_DBG_ME, "extent_client_cache remove %lud %llu\n", pthread_self(), eid);
  if (store_.find(eid) == store_.end()) {
    return extent_protocol::NOENT;
  }
  store_.erase(eid);
  return extent_protocol::OK;
}

void extent_client_cache::flush(extent_protocol::extentid_t eid) {
  ScopedLock ml(&mutex_);
  jsl_log(JSL_DBG_ME, "extent_client_cache: flush %lud %llu %p\n", pthread_self(), eid, this);
  extent_protocol::status res = extent_protocol::OK;

  if (store_.find(eid) == store_.end()) {
    jsl_log(JSL_DBG_ME, "extent_client_cache: flush - remove %llu\n", eid);
    res = extent_client::remove(eid); 
    // TODO pas toujours bon... test-lab-3-b 
    VERIFY(res == extent_protocol::OK);
    return;
  }
  
  Value &v = store_[eid];
  if (v.dirty) {
    jsl_log(JSL_DBG_ME, "extent_client_cache: flush - write back %llu buf = %s\n ", eid, v.buf.c_str());
    // writeback
    res = extent_client::put(eid, v.buf);
    VERIFY(res == extent_protocol::OK);
  }
  jsl_log(JSL_DBG_ME, "extent_client_cache: flush - erase %llu\n", eid);
  store_.erase(eid);
  return;
}


