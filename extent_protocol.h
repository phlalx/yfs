// extent wire protocol

#ifndef extent_protocol_h
#define extent_protocol_h

#include "rpc.h"

/** 
  * Common types for RPC between extent_client and extent_server 
  */ 
  
class extent_protocol {
 public:
  typedef int status;

  typedef unsigned long long extentid_t;

  static const extentid_t root_inum = 1L;

  enum xxstatus { OK, RPCERR, NOENT, IOERR };

  enum rpc_numbers {
    put = 0x6001,
    get,
    getattr,
    remove
  };

  struct attr {
    unsigned int atime;
    unsigned int mtime;
    unsigned int ctime;
    unsigned int size;
    attr() : atime(0), mtime(0), ctime(0), size(0) { }
  };

};

inline unmarshall &
operator>>(unmarshall &u, extent_protocol::attr &a)
{
  u >> a.atime;
  u >> a.mtime;
  u >> a.ctime;
  u >> a.size;
  return u;
}

inline marshall &
operator<<(marshall &m, extent_protocol::attr a)
{
  m << a.atime;
  m << a.mtime;
  m << a.ctime;
  m << a.size;
  return m;
}

#endif 
