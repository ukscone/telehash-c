#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "net_tmesh.h"


// individual pipe local info
typedef struct pipe_tmesh_struct
{
  struct sockaddr_in sa;
  int client;
  net_tmesh_t net;
  util_chunks_t chunks;
} *pipe_tmesh_t;

// just make sure it's connected
pipe_tmesh_t tmesh_to(pipe_t pipe)
{
  struct sockaddr_in addr;
  int ret;
  pipe_tmesh_t to;
  if(!pipe || !pipe->arg) return NULL;
  to = (pipe_tmesh_t)pipe->arg;
  if(to->client > 0) return to;
  
  // no socket yet, connect one
  if((to->client = socket(AF_INET, SOCK_STREAM, 0)) <= 0) return LOG("client socket faied %s, will retry next send",strerror(errno));

  // try to bind to our server port for some reflection
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(to->net->port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  bind(to->client, (struct sockaddr *)&addr, sizeof(struct sockaddr)); // optional, ignore fail

  fcntl(to->client, F_SETFL, O_NONBLOCK);
  ret = connect(to->client, (struct sockaddr *)&(to->sa), sizeof(struct sockaddr));
  if(ret < 0 && errno != EWOULDBLOCK && errno != EINPROGRESS)
  {
    LOG("client socket connect faied to %s: %s, will retry next send",pipe->id,strerror(errno));
    close(to->client);
    to->client = 0;
    return NULL;
  }

  LOG("connected to %s",pipe->id);
  return to;
}

// do all chunk/socket stuff
pipe_t tmesh_flush(pipe_t pipe)
{
  ssize_t len;
  lob_t packet;
  uint8_t buf[256];
  pipe_tmesh_t to = tmesh_to(pipe);
  if(!to) return NULL;

  if(util_chunks_len(to->chunks))
  {
    while((len = write(to->client, util_chunks_write(to->chunks), util_chunks_len(to->chunks))) > 0)
    {
      LOG("wrote %d bytes to %s",len,pipe->id);
      util_chunks_written(to->chunks, (size_t)len);
    }
  }

  while((len = read(to->client, buf, 256)) > 0)
  {
    LOG("reading %d bytes from %s",len,pipe->id);
    util_chunks_read(to->chunks, buf, (size_t)len);
  }

  // any incoming full packets can be received
  while((packet = util_chunks_receive(to->chunks))) mesh_receive(to->net->mesh, packet, pipe);

  if(len < 0 && errno != EWOULDBLOCK && errno != EINPROGRESS)
  {
    LOG("socket error to %s: %s",pipe->id,strerror(errno));
    close(to->client);
    to->client = 0;
  }

  return pipe;
}

// chunkize a packet
void tmesh_send(pipe_t pipe, lob_t packet, link_t link)
{
  pipe_tmesh_t to = tmesh_to(pipe);
  if(!to || !packet || !link) return;

  util_chunks_send(to->chunks, packet);
  tmesh_flush(pipe);
}

pipe_t tmesh_free(pipe_t pipe)
{
  pipe_tmesh_t to;
  if(!pipe) return NULL;
  to = (pipe_tmesh_t)pipe->arg;
  if(!to) return LOG("internal error, invalid pipe, leaking it");

  xht_set(to->net->pipes,pipe->id,NULL);
  pipe_free(pipe);
  if(to->client > 0) close(to->client);
  util_chunks_free(to->chunks);
  free(to);
  return NULL;
}

// internal, get or create a pipe
pipe_t tmesh_pipe(net_tmesh_t net, char *ip, int port)
{
  pipe_t pipe;
  pipe_tmesh_t to;
  char id[23];

  snprintf(id,23,"%s:%d",ip,port);
  pipe = xht_get(net->pipes,id);
  if(pipe) return pipe;

  // create new tmesh pipe
  if(!(pipe = pipe_new("tmesh"))) return NULL;
  if(!(pipe->arg = to = malloc(sizeof (struct pipe_tmesh_struct)))) return pipe_free(pipe);
  memset(to,0,sizeof (struct pipe_tmesh_struct));
  to->net = net;
  to->sa.sin_family = AF_INET;
  inet_aton(ip, &(to->sa.sin_addr));
  to->sa.sin_port = htons(port);
  if(!(to->chunks = util_chunks_new(0))) return tmesh_free(pipe);
  util_chunks_cloak(to->chunks); // enable cloaking by default

  // set up pipe
  pipe->id = strdup(id);
  xht_set(net->pipes,pipe->id,pipe);
  pipe->send = tmesh_send;

  return pipe;
}


pipe_t tmesh_path(link_t link, lob_t path)
{
  net_tmesh_t net;
  char *ip;
  int port;

  // just sanity check the path first
  if(!link || !path) return NULL;
  if(!(net = xht_get(link->mesh->index, "net_tmesh"))) return NULL;
  if(util_cmp("tmesh",lob_get(path,"type"))) return NULL;
  if(!(ip = lob_get(path,"ip"))) return LOG("missing ip");
  if((port = lob_get_int(path,"port")) <= 0) return LOG("missing port");
  return tmesh_pipe(net, ip, port);
}

net_tmesh_t net_tmesh_new(mesh_t mesh, lob_t options)
{
  int port, sock, opt = 1;
  unsigned int pipes;
  net_tmesh_t net;
  struct sockaddr_in sa;
  socklen_t size = sizeof(struct sockaddr_in);
  
  port = lob_get_int(options,"port");
  pipes = lob_get_uint(options,"pipes");
  if(!pipes) pipes = 11; // hashtable for active pipes

  // create a udp socket
  if((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP) ) < 0 ) return LOG("failed to create socket %s",strerror(errno));

  memset(&sa,0,sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  if(bind(sock, (struct sockaddr*)&sa, size) < 0) return LOG("bind failed %s",strerror(errno));
  getsockname(sock, (struct sockaddr*)&sa, &size);
  if(listen(sock, 10) < 0) return LOG("listen failed %s",strerror(errno));
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt , sizeof(int));
  fcntl(sock, F_SETFL, O_NONBLOCK);

  if(!(net = malloc(sizeof (struct net_tmesh_struct)))) return LOG("OOM");
  memset(net,0,sizeof (struct net_tmesh_struct));
  net->server = sock;
  net->port = ntohs(sa.sin_port);
  net->pipes = xht_new(pipes);

  // connect us to this mesh
  net->mesh = mesh;
  xht_set(mesh->index, "net_tmesh", net);
  mesh_on_path(mesh, "net_tmesh", tmesh_path);
  
  // convenience
  net->path = lob_new();
  lob_set(net->path,"type","tmesh");
  lob_set(net->path,"ip","127.0.0.1");
  lob_set_int(net->path,"port",net->port);
  mesh->paths = lob_push(mesh->paths, net->path);

  return net;
}

void net_tmesh_free(net_tmesh_t net)
{
  if(!net) return;
  close(net->server);
  xht_free(net->pipes);
//  lob_free(net->path); // managed by mesh->paths
  free(net);
  return;
}

// process any new incoming connections
void net_tmesh_accept(net_tmesh_t net)
{
  struct sockaddr_in addr;
  int client;
  pipe_t pipe;
  pipe_tmesh_t to;
  socklen_t size = sizeof(struct sockaddr_in);

  while((client = accept(net->server, (struct sockaddr *)&addr,&size)) > 0)
  {
    fcntl(client, F_SETFL, O_NONBLOCK);
    if(!(pipe = tmesh_pipe(net, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port)))) continue;
    LOG("incoming connection from %s",pipe->id);
    to = (pipe_tmesh_t)pipe->arg;
    if(to->client > 0) close(to->client);
    to->client = client;
  }

}

// check a single pipe's socket for any read/write activity
void _walkflush(xht_t h, const char *key, void *val, void *arg)
{
  tmesh_flush((pipe_t)val);
}

net_tmesh_t net_tmesh_loop(net_tmesh_t net)
{
  net_tmesh_accept(net);
  xht_walk(net->pipes, _walkflush, NULL);
  return net;
}

#endif
