#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <jsl_log.h>

extent_server::extent_server() {
  extent_protocol::extentid_t n = extent_protocol::root_inum; 
  Value &v = kv_store[n]; 
  v.buf = "";
  v.attr.size = 0;
  time_t cur_time = time(NULL);
  v.attr.ctime = cur_time;
  v.attr.mtime = cur_time;
}

extent_protocol::status extent_server::put(extent_protocol::extentid_t id, 
                                           std::string buf, int &) { 
  ScopedLock mut(&mutex);
  jsl_log(JSL_DBG_ME, "extent_server: put %llu buf = %s\n", id, buf.c_str());
  Value &v = kv_store[id];
  v.buf = buf;
  v.attr.size = buf.size();
  time_t cur_time = time(NULL);

  // We change ctime at every access (?)
  v.attr.ctime = cur_time;
  v.attr.mtime = cur_time;
  return extent_protocol::OK;
}

extent_protocol::status extent_server::get(extent_protocol::extentid_t id,
                                           std::string &buf) {
  ScopedLock mut(&mutex);
  jsl_log(JSL_DBG_ME, "extent_server: get %llu\n", id);
  if (kv_store.find(id) == kv_store.end()) {
    jsl_log(JSL_DBG_ME, "extent_server: get %llu -> NOENT", id);
    return extent_protocol::NOENT;
  }
  Value &v = kv_store[id];
  buf = v.buf;
  v.attr.atime = time(NULL);
  jsl_log(JSL_DBG_ME, "extent_server: get %llu -> buf = %s\n", id, v.buf.c_str());
  return extent_protocol::OK;
}

extent_protocol::status extent_server::getattr(extent_protocol::extentid_t id, 
                                               extent_protocol::attr &a) {
  ScopedLock mut(&mutex);
  jsl_log(JSL_DBG_ME, "extent_server: get attr %llu\n", id);
  if (kv_store.find(id) == kv_store.end()) {
    return extent_protocol::NOENT;
  }
  a = kv_store[id].attr;
  return extent_protocol::OK;
}

extent_protocol::status extent_server::remove(extent_protocol::extentid_t id, 
                                              int &) {
  ScopedLock mut(&mutex);
  jsl_log(JSL_DBG_ME, "extent_server: remove %llu\n", id);
  if (kv_store.find(id) == kv_store.end()) {
    return extent_protocol::NOENT;
  }
  kv_store.erase(id);
  return extent_protocol::OK;
}