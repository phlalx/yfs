#ifndef extent_client_h
#define extent_client_h

#include <string>
#include "extent_protocol.h"
#include "rpc.h"
#include <memory>

/**
  * RPC stub to talk to the extent_server
  * Assume that the caller always holds a lock on the extent
  * Thread-safe (thanks to rpcc)
  */
class extent_client {
	
private:
	std::unique_ptr<rpcc> cl;

public:
	extent_client(const std::string &dst);

	extent_protocol::status get(extent_protocol::extentid_t eid, 
							    std::string &buf);

	extent_protocol::status getattr(extent_protocol::extentid_t eid, 
		                            extent_protocol::attr &a);

	extent_protocol::status put(extent_protocol::extentid_t eid, 
		                         std::string buf);

	extent_protocol::status remove(extent_protocol::extentid_t eid);
};

#endif 

