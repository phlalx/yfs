#ifndef yfs_client_h
#define yfs_client_h

#include <string>
#include "extent_client_cache.h"
#include <vector>
#include "lock_protocol.h"
#include "lock_client_cache.h"
#include "jsl_log.h"

/** 
 *  This class provides the interface from the fuse module to yfs.
 *  It encapsulates the lock client cache client and the extent cache client 
 *  We assume that RPC always succeed, all extent server must be locked through
 *  the lock server.
 *  When lock is revoked, cached extent are written back before releasing the 
 *  lock
 */
class yfs_client {

public:

  typedef unsigned long long inum;
  enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST };
  typedef int status;

  struct fileinfo {
    unsigned long long size;
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };

  struct dirinfo {
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };

  struct dirent {
    std::string name;
    yfs_client::inum inum;
    dirent(std::string name, yfs_client::inum inum) : name(name), inum(inum) {}
  };

  yfs_client(std::string, std::string);

  void acquireLock(inum i);

  void releaseLock(inum i);

  bool isfile(inum);

  bool isdir(inum);

  status getfile(inum, fileinfo &);

  status getdir(inum, dirinfo &);

  // TODO supprimer les VERIFY qui feront planter le programme et utiliser 
  // les codes de retour dédiés (cf. get attr dans fuse.cc)

  // precondition: parent is a dir
  status create(inum parent, const char *name, inum &file_inum);

  // precondition: parent is a dir
  // we don't know if name is a dir or a file, so we use a file info
  // to retrieve the attributes 
  status lookup(inum parent, const char *name, inum &file_inum);

  // precondition: parent is a dir, and it exists
  status read_dir(inum parent, std::vector<dirent> &v);

  status mkdir(inum parent, const char *name, inum &dir_inum);

  status read(inum num, size_t size, off_t off, std::string &buf);

  status write(inum num, size_t size, off_t off, const char *buf);

  status unlink(inum parent, const char *name);

  status resize(inum num, size_t size);
  
private:
  
  extent_client_cache *ec;
  lock_client *lc = NULL;

  class my_lock_release_user : public lock_release_user {
  private:
    extent_client_cache *ec;
  public:
    my_lock_release_user(extent_client_cache *ec) : ec(ec) {};
    virtual void dorelease(lock_protocol::lockid_t id) {
      jsl_log(JSL_DBG_ME, "yfs_client do release %llu\n", id);
      ec->flush(id);
      jsl_log(JSL_DBG_ME, "yfs_client do release done\n");
    } 
  };

  static std::string filename(inum);
  static inum n2i(std::string);
  static std::string serialize_dir(std::vector<dirent>);
  static void deserialize_dir(std::string, std::vector<dirent> &);
  static inum fresh_inum(bool is_dir);
};

#endif 
