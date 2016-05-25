#ifndef yfs_client_h
#define yfs_client_h

#include <string>
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>
#include "lock_protocol.h"
#include "lock_client.h"

class yfs_client {

  extent_client *ec;
  lock_client *lc;

  const lock_protocol::lockid_t global_lock = 1L; 

  struct ScopedLock {
  private:
    lock_client *lc;
    lock_protocol::lockid_t l;
  public:
    ScopedLock(lock_client *lc, lock_protocol::lockid_t l) : lc(lc), l(l) {
      lc->acquire(l);
    }
    ~ScopedLock() {
      lc->release(l);
    }
  };

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

private:
  
  static std::string filename(inum);
  static inum n2i(std::string);
  static std::string serialize_dir(std::vector<dirent>);
  static void deserialize_dir(std::string, std::vector<dirent> &);
  static inum fresh_inum(bool is_dir);

public:

  yfs_client(std::string, std::string);

  bool isfile(inum);
  bool isdir(inum);

  int getfile(inum, fileinfo &);
  int getdir(inum, dirinfo &);


  // TODO supprimer les VERIFY qui feront planter le programme et utiliser 
  // les codes de retour dédiés (cf. get attr dans fuse.cc)
  // TODO mettre ces méthodes dans FUSE.cc  ?

  // precondition: parent is a dir
  // -1 if name already exists
  int create(inum parent, const char *name, inum &file_inum);

  // precondition: parent is a dir
  // we don't know if name is a dir or a file, so we use a file info
  // to retrieve the attributes 
  bool lookup(inum parent, const char *name, inum &file_inum);

  // precondition: parent is a dir, and it exists
  void read_dir(inum parent, std::vector<dirent> &v);

  int mkdir(inum parent, const char *name, inum &dir_inum);

  status read(inum num, size_t size, off_t off, std::string &buf);

  status write(inum num, size_t size, off_t off, const char *buf);

  int unlink(inum parent, const char *name);

  status resize(inum num, size_t size);
};

#endif 
