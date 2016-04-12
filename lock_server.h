// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include <string>
#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"
#include "pthread.h"
#include <map>
#include <vector>

class lock_server {

 protected:
  int nacquire;

  struct lock_info {
  	bool is_locked;
  	pthread_cond_t waiting;

  	lock_info() {
  		is_locked = false;
  		waiting = PTHREAD_COND_INITIALIZER;
  	};
  };

  // list of threads waiting for the lock
  std::map<lock_protocol::lockid_t, lock_info> locks; 

  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


 public:
  lock_server();
  ~lock_server() {};
  lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status acquire(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status release(int clt, lock_protocol::lockid_t lid, int &);
};

#endif 







