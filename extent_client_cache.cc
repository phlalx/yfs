// RPC stubs for clients to talk to extent_server

#include "extent_client_cache.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <jsl_log.h>

// The calls assume that the caller holds a lock on the extent

extent_client_cache::extent_client_cache(std::string dst)
{
  ec = new extent_client(dst);
}

extent_protocol::status
extent_client_cache::get(extent_protocol::extentid_t eid, std::string &buf)
{
  jsl_log(JSL_DBG_ME, "cache_extent_server: get %llu\n", eid);
  extent_protocol::status res = extent_protocol::OK;
  Value &v = kv_store[eid];
  if (kv_store.find(eid) == kv_store.end()) {
    // TODO -> ajouter une opération sur le serveur qui renvoie tout ?
    res = ec->get(eid, v.buf); 
    if (res != extent_protocol::OK) {
      return res;
    }
    res = ec->getattr(eid, v.attr);
    if (res != extent_protocol::OK) {
      return res;
    }
  }
  buf = v.buf;
  return res;
}

extent_protocol::status
extent_client_cache::getattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr &attr)
{
  jsl_log(JSL_DBG_ME, "cache_extent_server: getattr %llu\n", eid);
  // TODO factoriser les deux get
  extent_protocol::status res = extent_protocol::OK;
  Value &v = kv_store[eid];
  if (kv_store.find(eid) == kv_store.end()) {
    // TODO -> ajouter une opération sur le serveur qui renvoie tout ?
    res = ec->get(eid, v.buf); 
    if (res != extent_protocol::OK) {
      return res;
    }
    res = ec->getattr(eid, v.attr);
    if (res != extent_protocol::OK) {
      return res;
    }
  }
  attr = v.attr;
  return res;
}

extent_protocol::status
extent_client_cache::put(extent_protocol::extentid_t eid, std::string buf)
{
  jsl_log(JSL_DBG_ME, "cache_extent_server: put %llu\n", eid);
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

extent_protocol::status
extent_client_cache::remove(extent_protocol::extentid_t eid)
{
  jsl_log(JSL_DBG_ME, "cache_extent_server: remove %llu\n", eid);
  if (kv_store.find(eid) == kv_store.end()) {
    return extent_protocol::NOENT;
  }
  kv_store.erase(eid);
  return extent_protocol::OK;
}
