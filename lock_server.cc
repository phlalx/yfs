// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

lock_server::lock_server():
  nacquire (0)
{
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{
  pthread_mutex_lock(&mutex);
  lock_protocol::status ret = lock_protocol::OK;
  printf("acquire request from clt %d\n", clt);

  if (locks.find(lid) == locks.end()) {
    locks[lid] = lock_info();
  } 

  lock_info &li = locks[lid];

  while (li.is_locked) {
    pthread_cond_wait(&li.waiting, &mutex);
  }

  li.is_locked = true;
  pthread_mutex_unlock(&mutex);
  return ret;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
  pthread_mutex_lock(&mutex);
  lock_protocol::status ret = lock_protocol::OK;
  printf("release request from clt %d\n", clt);

  if (locks.find(lid) == locks.end()) {
  	assert(false);
  } else {
    lock_info &li = locks[lid];
    assert(li.is_locked);
    li.is_locked = false; 
    printf("unlock %llu\n", lid);
    pthread_cond_signal(&li.waiting);
  }
  pthread_mutex_unlock(&mutex);

  return ret;
}