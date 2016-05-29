#ifndef lock_protocol_h
#define lock_protocol_h

#include "rpc.h"

/**
  * Common types for RPCs between lock_client (or lock_client_cache) 
  * and lock_server 
  */
class lock_protocol {
 public:
  enum xxstatus { OK, RETRY, RPCERR, NOENT, IOERR };
  typedef int status;
  typedef unsigned long long lockid_t;
  typedef unsigned long long xid_t;
  enum rpc_numbers {
    acquire = 0x7001,
    release,
    stat
  };
};

/**
  * Common types for RPCs between lock_server and lock_client_cache
  */
class rlock_protocol {
 public:
  enum xxstatus { OK, RPCERR };
  typedef int status;
  enum rpc_numbers {
    revoke = 0x8001,
    retry = 0x8002
  };
};
#endif 
