// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <jsl_log.h>

extent_server::extent_server() {
  // lock dans le constructeur ?
  // create root directory TODO mettre cette constante 1 qq part
  const int root_inum = 1;
  Value &v = kv_store[root_inum];
  v.buf = "";
  v.attr.size = 0;
  time_t cur_time = time(NULL);

  // TODO why change ctime at every access?
  v.attr.ctime = cur_time;
  v.attr.mtime = cur_time;
}

int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{ 
  ScopedLock mut(&mutex);
  // LAB 2
  jsl_log(JSL_DBG_ME, "extent_server: put %llu\n", id);
  Value &v = kv_store[id];
  v.buf = buf;
  v.attr.size = buf.size();
  time_t cur_time = time(NULL);

  // TODO why change ctime at every access?
  v.attr.ctime = cur_time;
  v.attr.mtime = cur_time;
  return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
  ScopedLock mut(&mutex);
  // LAB 2
  jsl_log(JSL_DBG_ME, "extent_server: get %llu\n", id);
  if (kv_store.find(id) == kv_store.end()) {
    return extent_protocol::NOENT;
  }
  Value &v = kv_store[id];
  buf = v.buf;
  v.attr.atime = time(NULL);
  return extent_protocol::OK;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
  ScopedLock mut(&mutex);
  // LAB 2
  jsl_log(JSL_DBG_ME, "extent_server: get attr %llu\n", id);
  if (kv_store.find(id) == kv_store.end()) {
    return extent_protocol::NOENT;
  }
  a = kv_store[id].attr;
  return extent_protocol::OK;
}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
  ScopedLock mut(&mutex);
  // LAB 2
  jsl_log(JSL_DBG_ME, "extent_server: remove %llu\n", id);
  if (kv_store.find(id) == kv_store.end()) {
    return extent_protocol::NOENT;
  }
  kv_store.erase(id);
  return extent_protocol::OK;
}
