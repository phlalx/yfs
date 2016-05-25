// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include "lock_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>     /* srand, rand */
#include <time.h> 
#include <jsl_log.h> 
#include <sstream> 
#include <algorithm> 

void yfs_client::acquireLock(inum i) {
  jsl_log(JSL_DBG_ME, "acquiring lock\n");
  lock_protocol::status st = lc->acquire(i);
  jsl_log(JSL_DBG_ME, "acquiring lock - ok %d\n", st);
}

void yfs_client::releaseLock(inum i) {
  jsl_log(JSL_DBG_ME, "releasing lock\n");
  lock_protocol::status st = lc->release(i);
  jsl_log(JSL_DBG_ME, "releasing lock - ok %d\n", st);
}

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst) {
  ec = new extent_client(extent_dst);
  lc = new lock_client(lock_dst);
  srand (time(NULL));  // TODO déjà fait dans fuse ??

  // create root directory
  const int root_inum = 1;

  // TODO est-ce que c'est judicieux de mettre ça dans le constructeur ?
  VERIFY(ec->put(root_inum, "") == extent_protocol::OK);
}

yfs_client::status yfs_client::create(inum parent, const char *name, inum &file_inum) {
  jsl_log(JSL_DBG_ME, "yfs_client_create %s\n", name);

  extent_protocol::status st1, st2;

  std::vector<dirent> content;
  st1 = read_dir(parent, content);
  if (st1 != extent_protocol::OK) {
    return IOERR; 
  }

  for (dirent const &de : content) {
    if (de.name == name) { 
      jsl_log(JSL_DBG_ME, "yfs_client_create file exists\n");
      return EXIST; 
    }
  }
  file_inum = fresh_inum(false);
  content.push_back(dirent(name, file_inum));
  std::string new_dir = serialize_dir(content);

  st1 = ec->put(file_inum, "");
  st2 = ec->put(parent, new_dir);
  if (st1 != extent_protocol::OK || st2 != extent_protocol::OK) {
    return IOERR; 
  }

  jsl_log(JSL_DBG_ME, "yfs_client_create file %016llx and added to parent dir\n", file_inum);
  return OK;
}

yfs_client::status yfs_client::mkdir(inum parent, const char *name, inum &dir_inum) {
  jsl_log(JSL_DBG_ME, "yfs_client_mkdir %s\n", name);
  std::vector<dirent> content;
  extent_protocol::status st1, st2;
  st1 = read_dir(parent, content);
  if (st1 != extent_protocol::OK) {
    return IOERR; 
  }
  for (dirent const &de : content) {
    if (de.name == name) { 
     jsl_log(JSL_DBG_ME, "yfs_client_mkdir dir exists\n");
     return EXIST; 
    }
  }
  dir_inum = fresh_inum(true);
  content.push_back(dirent(name, dir_inum));
  std::string new_dir = serialize_dir(content);
  st1 = ec->put(dir_inum, "");
  st2 = ec->put(parent, new_dir);
  if (st1 != extent_protocol::OK || st2 != extent_protocol::OK) {
    return IOERR; 
  }
  jsl_log(JSL_DBG_ME, "yfs_client_mkdir dir %016llx created and added to parent dir\n", dir_inum);
  return OK;
}

// TODO utiliser lookup plutot que de refaire la recherche dans le dir à chaque fois
yfs_client::status yfs_client::unlink(inum parent, const char *name) {
 jsl_log(JSL_DBG_ME, "yfs_client_unlink %s\n", name);
 std::vector<dirent> content;
 extent_protocol::status st1;
 st1 = read_dir(parent, content);
 if (st1 != extent_protocol::OK) {
    return IOERR; 
 }

 auto it = std::find_if(content.begin(), content.end(), [name] (dirent &s) { return s.name == name; } );
 if (it == content.end()) {
   jsl_log(JSL_DBG_ME, "yfs_client_unlink file doesn't exists\n");
   return NOENT;
 }
 jsl_log(JSL_DBG_ME, "yfs_client_unlink removing file\n");
 st1 = ec->remove(it->inum);
 if (st1 != extent_protocol::OK) { 
  return IOERR;
 }
 content.erase(it);
 std::string new_dir = serialize_dir(content);
 st1 = ec->put(parent, new_dir);
 if (st1 != extent_protocol::OK) { 
  return IOERR;
 }
 return OK;
}

yfs_client::status yfs_client::lookup(inum parent, const char *name, inum &file_inum) {
  jsl_log(JSL_DBG_ME, "yfs_client_lookup %s\n", name);
  std::vector<dirent> content;
  extent_protocol::status st1;
  st1 = read_dir(parent, content);
  if (st1 != extent_protocol::OK) {
    return IOERR; 
  }
  for (dirent const &de : content) {
    if (de.name == name) {
      file_inum = de.inum;
      jsl_log(JSL_DBG_ME, "yfs_client_lookup found %s %llx\n", name, file_inum);
      return OK; 
    }
  }
  jsl_log(JSL_DBG_ME, "yfs_client_lookup didn't found %s\n", name);
  return NOENT;
}

yfs_client::status yfs_client::read_dir(inum parent, std::vector<dirent> &v) {
 jsl_log(JSL_DBG_ME, "yfs_client_read_dir %016llx\n", parent);
 VERIFY(isdir(parent));  
 std::string buf;
 extent_protocol::status st = ec->get(parent, buf);
 if (st != extent_protocol::OK) {
  return IOERR;
 }
 deserialize_dir(buf, v);
 return OK;
}

yfs_client::status yfs_client::read(inum num, size_t size, off_t off, std::string &buf) {
  jsl_log(JSL_DBG_ME, "yfs_client_read %016llx size %lu off %lu\n", num, size, off);
  std::string extent;
  extent_protocol::status st = ec->get(num, extent);
  if (st != extent_protocol::OK) {
    return IOERR;
  }

  size_t extent_size = extent.size();
  if ((size_t)off >= extent_size) { // TODO est-ce utile ? voir comportement par defaut de substr
    buf = "";
    return OK;
  }
  buf = extent.substr(off, size); 
  return OK;
}

yfs_client::status yfs_client::write(inum num, size_t size, off_t off, const char *buf) {
  jsl_log(JSL_DBG_ME, "yfs_client_write %016llx\n", num);
  std::string extent;
  if (size == 0) {
    return OK;
  }
  extent_protocol::status st = ec->get(num, extent);
  if (st != extent_protocol::OK) {
    return IOERR;
  }

  size_t old_size = extent.size();
  size_t new_size = std::max(old_size, off + size); 
  jsl_log(JSL_DBG_ME, "yfs_client_write %016llx - size %lu - old size %lu - new size - %lu - offset %lu\n", num, size, old_size, new_size, off);
  extent.resize(new_size, 0);
  VERIFY(extent.size() == new_size);
  if ((size_t)off > old_size) {
    jsl_log(JSL_DBG_ME, "yfs_client_write %016llx - there's a hole %lu\n", num, off - old_size);
  }

  // TODO replace this with a string method
  for (size_t i = 0; i < size; i++){
    extent[(size_t)(off + i)] = buf[i];
  }

  // update extent
  st = ec->put(num, extent);
  if (st != extent_protocol::OK) {
    return IOERR;
  }
  return OK;
}

yfs_client::status yfs_client::resize(inum num, size_t size) {
  jsl_log(JSL_DBG_ME, "yfs_client_resize %016llx\n", num);
  std::string extent;
  extent_protocol::status st1, st2;
  st1 = ec->get(num, extent);
  extent.resize(size, 0);
  st2 = ec->put(num, extent);
  if (st1 != extent_protocol::OK && st2 != extent_protocol::OK) {
    return IOERR;
  }
  return OK;
}

yfs_client::status
yfs_client::getfile(inum inum, fileinfo &fin)
{
  int r = OK;
  // You modify this function for Lab 3
  // - hold and release the file lock

  jsl_log(JSL_DBG_ME, "yfs_client getfile %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }

  fin.atime = a.atime;
  fin.mtime = a.mtime;
  fin.ctime = a.ctime;
  fin.size = a.size;
  printf("getfile %016llx -> sz %llx\n", inum, fin.size);

  release:

  return r;
}

yfs_client::status
yfs_client::getdir(inum inum, dirinfo &din)
{
  int r = OK;
  // You modify this function for Lab 3
  // - hold and release the directory lock

  jsl_log(JSL_DBG_ME, "yfs_client getdir %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  din.atime = a.atime;
  din.mtime = a.mtime;
  din.ctime = a.ctime;

  release:
  return r;
}

///// Local methods, don't access the server, stateless


bool
yfs_client::isfile(inum inum)
{
  if(inum & 0x80000000)
    return true;
  return false;
}

bool
yfs_client::isdir(inum inum)
{
  return ! isfile(inum);
}

// TODO at some point use const & wherever possible
std::string yfs_client::serialize_dir(std::vector<dirent> dir) {
  std::string res;
  // TODO antipattern? make that more efficient
  // TODO check that name can't contain "/"
  for (auto const &de : dir) {
    res += de.name; 
    res += "/"; 
    res += filename(de.inum); 
    res += "/"; 
  }
  jsl_log(JSL_DBG_ME, "yfs_client_serialize %s\n", res.c_str());
  return res;
} 

void yfs_client::deserialize_dir(std::string s, std::vector<dirent> &r){ 
 std::stringstream ss(s);
 std::string item1, item2;
 while (std::getline(ss, item1, '/')) {
   VERIFY(std::getline(ss, item2, '/'));
   r.push_back(dirent(item1, n2i(item2)));
 }
} 

yfs_client::inum
yfs_client::fresh_inum(bool is_dir)
{
  unsigned long int r = rand(); 
  if (!is_dir) { r |= 0x80000000; }
  return (inum) r;
}

yfs_client::inum
yfs_client::n2i(std::string n)
{
  std::istringstream ist(n);
  unsigned long long finum;
  ist >> finum;
  return finum;
}

std::string
yfs_client::filename(inum inum)
{
  std::ostringstream ost;
  ost << inum;
  return ost.str();
}