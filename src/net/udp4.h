#ifndef net_udp4_h
#define net_udp4_h

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#include "mesh.h"

// overall server
typedef struct net_udp4_struct
{
  int server;
  int port;
  mesh_t mesh;
  xht_t pipes;
  lob_t path; // to us
} *net_udp4_t;

// create a new listening udp server
net_udp4_t net_udp4_new(mesh_t mesh, int port);
void net_udp4_free(net_udp4_t net);

// receive a packet into this mesh
net_udp4_t net_udp4_receive(net_udp4_t net);

#endif