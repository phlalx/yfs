// extent client interface.

#ifndef extent_client_cache_h
#define extent_client_cache_h

#include <string>
#include "extent_protocol.h"
#include "extent_client.h"
#include "rpc.h"

class extent_client_cache {
	
private:
	extent_client *ec;

public:
	extent_client_cache(std::string dst);

	extent_protocol::status get(extent_protocol::extentid_t eid, 
		std::string &buf);
	extent_protocol::status getattr(extent_protocol::extentid_t eid, 
		extent_protocol::attr &a);
	extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);
	extent_protocol::status remove(extent_protocol::extentid_t eid);
};

#endif 

