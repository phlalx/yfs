#ifndef lock_server_cache_h
#define lock_server_cache_h

#include <string>

#include <map>
#include <set>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_server.h"


class lock_server_cache {
 private:
  int nacquire;
  
  struct lock_info {
  	bool is_locked;
  	std::string id;
  	// TODO should use some FIFO structure instead to ensure fairness
  	std::set<std::string> waiting;

  	lock_info() {
  		is_locked = false; // redundant? TODO
  		id = ""; // TODO
  	};
  };

  // list of threads waiting for the lock
  std::map<lock_protocol::lockid_t, lock_info> locks; 

  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

  void revoke(const std::string &client_to_be_revoked, const lock_protocol::lockid_t &lid);

  void retry(const std::string &client_to_be_retried, const lock_protocol::lockid_t &lid);

 public:
  lock_server_cache();
  lock_protocol::status stat(lock_protocol::lockid_t, int &);
  int acquire(lock_protocol::lockid_t, std::string id, int &);
  int release(lock_protocol::lockid_t, std::string id, int &);
};

#endif
