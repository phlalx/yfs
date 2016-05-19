// lock client interface.
#ifndef lock_client_cache_h

#define lock_client_cache_h

#include <string>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_client.h"
#include "lang/verify.h"
#include "jsl_log.h"

// Classes that inherit lock_release_user can override dorelease so that 
// that they will be called when lock_client releases a lock.
// You will not need to do anything with this class until Lab 5.
class lock_release_user {
 public:
  virtual void dorelease(lock_protocol::lockid_t) = 0;
  virtual ~lock_release_user() {};
};

class lock_client_cache : public lock_client {
private:

  enum State { ACQUIRING, RELEASING, FREE, NONE, LOCKED };
  struct lock_info {

    State st;
    pthread_cond_t waiting_from_server;  // acquire thread
    pthread_cond_t waiting_local;      // client thread

    bool is_revoked;
    bool is_retried;

    const char *to_string() {
      switch(st){
        case ACQUIRING: return "ACQUIRING";
        case RELEASING: return "RELEASING";
        case FREE: return "FREE";
        case NONE: return "NONE";
        case LOCKED: return "LOCKED";
      }
      return "--";
    }

    void set(lock_client_cache *lcc, lock_protocol::lockid_t lid, State new_st) {
        jsl_log(JSL_DBG_ME, "lock_client_cache %s %lud %llu: ", lcc->id.c_str(), pthread_self(), lid);
        jsl_log(JSL_DBG_ME, "%s -> ", to_string());
        st = new_st;
        jsl_log(JSL_DBG_ME, "%s (# = %d)\n", to_string(), number_waiting);
        if (st == FREE || st == NONE) {
          is_revoked = false; 
          is_retried = false; 
        }
    }

    int number_waiting;

    lock_info() {
      number_waiting = 0;
      st = NONE;
      waiting_from_server = PTHREAD_COND_INITIALIZER; 
      waiting_local = PTHREAD_COND_INITIALIZER; 
      is_revoked = false;
      is_retried = false;
    };
  };

  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  // list of threads waiting for the lock
  std::map<lock_protocol::lockid_t, lock_info> locks; 

  class lock_release_user *lu;
  int rlock_port;
  std::string hostname;
  std::string id;
public:
  lock_client_cache(std::string xdst, class lock_release_user *l = 0);
  virtual ~lock_client_cache() {};
  lock_protocol::status acquire(lock_protocol::lockid_t);
  lock_protocol::status release(lock_protocol::lockid_t);
  rlock_protocol::status revoke_handler(lock_protocol::lockid_t, int &);
  rlock_protocol::status retry_handler(lock_protocol::lockid_t, int &);
};


#endif
