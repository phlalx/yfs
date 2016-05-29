#ifndef extent_client_cache_h
#define extent_client_cache_h

#include <string>
#include "extent_protocol.h"
#include "extent_client.h"
#include "rpc.h"

/**
  * Cached client to the extent server
  * Operations are mostly performed locally 
  * All operations are protected by a mutex   
  * Sequential consistency is guaranteed as long as 
  * the caller owns the lock on the extent
  */
class extent_client_cache : extent_client {
	
private:

	struct Extent {
		extent_protocol::attr attr;
		std::string buf;
		bool dirty;
	};

	std::map<extent_protocol::extentid_t, Extent> store_; 

	pthread_mutex_t mutex_ = PTHREAD_MUTEX_INITIALIZER;
	
public:
	extent_client_cache(const std::string &dst);

	/** local operation, unless eid is not present */
	extent_protocol::status get(extent_protocol::extentid_t eid, 
		std::string &buf);

	/** local operation, unless eid is not present */
	extent_protocol::status getattr(extent_protocol::extentid_t eid, 
		extent_protocol::attr &a);

	/* always local */
	extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);

	/* always local */
	extent_protocol::status remove(extent_protocol::extentid_t eid);

  /**
   *  Flush writes back eid if it's dirty or has been removed 
   *  Called by the lock client before release the lock
   */
	void flush(extent_protocol::extentid_t eid);
};

#endif 

