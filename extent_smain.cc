#include "rpc.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "extent_server.h"
#include "jsl_log.h"

/**
  * main loop of extent server
  */
int main(int argc, char *argv[]) {
  jsl_set_debug(JSL_DBG_ME);
  int count = 0;

  if(argc != 2){
    fprintf(stderr, "Usage: %s port\n", argv[0]);
    exit(1);
  }

  setvbuf(stdout, NULL, _IONBF, 0);

  char *count_env = getenv("RPC_COUNT");
  if(count_env != NULL){
    count = atoi(count_env);
  }

  rpcs server(atoi(argv[1]), count);
  extent_server es;

  server.reg(extent_protocol::get, &es, &extent_server::get);
  server.reg(extent_protocol::getattr, &es, &extent_server::getattr);
  server.reg(extent_protocol::put, &es, &extent_server::put);
  server.reg(extent_protocol::remove, &es, &extent_server::remove);

  // TODO(phil) change this
  while(1) sleep(1000);
}
