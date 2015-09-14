#include "tmesh.h"
#include "util_sys.h"
#include "util_unix.h"
#include "unit_test.h"

uint32_t device_check(tmesh_t tm, uint8_t medium[6])
{
  return 1;
}

medium_t device_get(tmesh_t tm, uint8_t medium[6])
{
  medium_t m;
  if(!(m = malloc(sizeof(struct medium_struct)))) return LOG("OOM");
  memset(m,0,sizeof (struct medium_struct));
  memcpy(m->bin,medium,6);
  m->chans = 100;
  m->min = 10;
  m->max = 1000;
  return m;
}

medium_t device_free(tmesh_t tm, medium_t m)
{
  return NULL;
}

static struct radio_struct test_device = {
  device_check,
  device_get,
  device_free,
  NULL,
  {0}
};

int main(int argc, char **argv)
{
  fail_unless(!e3x_init(NULL)); // random seed
  fail_unless(radio_device(&test_device));
  
  mesh_t meshA = mesh_new(3);
  fail_unless(meshA);
  lob_t secretsA = mesh_generate(meshA);
  fail_unless(secretsA);

  lob_t idB = e3x_generate();
  hashname_t hnB = hashname_keys(lob_linked(idB));
  fail_unless(hnB);
  link_t link = link_get(meshA,hnB->hashname);
  fail_unless(link);
  
  tmesh_t netA = tmesh_new(meshA, NULL);
  fail_unless(netA);
  cmnty_t c = tmesh_private(netA,"fzjb5f4tn4","foo");
  fail_unless(c);
  fail_unless(c->medium->bin[0] == 46);
  fail_unless(c->pipe->path);
  LOG("netA %.*s",c->pipe->path->head_len,c->pipe->path->head);


  fail_unless(!tmesh_public(netA, "kzdhpa5n6r", NULL));
  fail_unless((c = tmesh_public(netA, "kzdhpa5n6r", "")));
  mote_t m = tmesh_link(netA, c, link);
  fail_unless(m);
  fail_unless(m->link == link);
  fail_unless(m->epochs);
  fail_unless(m == tmesh_link(netA, c, link));
  


  return 0;
}

