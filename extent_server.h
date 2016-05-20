// this is the extent server

#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>
#include "extent_protocol.h"

class extent_server {

  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

  struct Value {
  	extent_protocol::attr attr;
  	std::string buf;
  };

  std::map<extent_protocol::extentid_t, Value> kv_store; 

 public:
  extent_server();

  int put(extent_protocol::extentid_t id, std::string, int &);
  int get(extent_protocol::extentid_t id, std::string &);
  int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
  int remove(extent_protocol::extentid_t id, int &);
};

#endif 







