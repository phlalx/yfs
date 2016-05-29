#ifndef extent_client_h
#define extent_client_h

#include <string>
#include "extent_protocol.h"
#include "rpc.h"

/**
  * RPC stub to talk to extent_server
  * The calls assume that the caller holds a lock on the extent
  * thread-safe (thanks to rpcc)
  */
class extent_client {
	
private:
	rpcc *cl;

public:
	extent_client(std::string dst);

	extent_protocol::status get(extent_protocol::extentid_t eid, 
							    std::string &buf);
	extent_protocol::status getattr(extent_protocol::extentid_t eid, 
		                            extent_protocol::attr &a);
	extent_protocol::status put(extent_protocol::extentid_t eid, 
		                         std::string buf);
	extent_protocol::status remove(extent_protocol::extentid_t eid);
};

#endif 

