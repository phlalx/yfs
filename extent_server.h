#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>
#include "extent_protocol.h"

/** 
  * A simple map from extentid_t ids to extents. 
  * An extent is a pair (buffer, attribute)
  * To be used within an RPC server
  */
class extent_server {

private:
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

  struct Value {
  	extent_protocol::attr attr;
  	std::string buf;
  };

  std::map<extent_protocol::extentid_t, Value> kv_store; 

public:
  extent_server();

  extent_protocol::status put(extent_protocol::extentid_t id, std::string, int &);
  extent_protocol::status get(extent_protocol::extentid_t id, std::string &);
  extent_protocol::status getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
  extent_protocol::status remove(extent_protocol::extentid_t id, int &);
};

#endif 







