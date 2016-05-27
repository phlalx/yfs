// RPC stubs for clients to talk to extent_server

#include "extent_client_cache.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

// The calls assume that the caller holds a lock on the extent

extent_client_cache::extent_client_cache(std::string dst)
{
  ec = new extent_client(dst);
}

extent_protocol::status
extent_client_cache::get(extent_protocol::extentid_t eid, std::string &buf)
{
  return ec->get(eid, buf);
}

extent_protocol::status
extent_client_cache::getattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr &attr)
{
  return ec->getattr(eid, attr);
}

extent_protocol::status
extent_client_cache::put(extent_protocol::extentid_t eid, std::string buf)
{
  return ec->put(eid, buf);
}

extent_protocol::status
extent_client_cache::remove(extent_protocol::extentid_t eid)
{
  return ec->remove(eid);
}


