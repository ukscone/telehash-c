/*
LOB; // is singleton
var lob = LOB.create(); // typeof "lob"
lob.key = value; // lob setter
lob.foo; // lob getter, creates copy of value
LOB.body(lob); // returns body
*/

#include "throwback.h"

// using upper case LOB prefix for all singleton methods, lower case lob for all object type ones

static dew_t LOB_create(dew_t stack, dew_t args, dew_t result)
{
  dew_set_object(result, TYPEOF_LOB, lob_new());
  return stack;
}

static dew_t LOB_body(dew_t stack, dew_t args, dew_t result)
{
  if(!dew_typeof(args,TYPEOF_LOB)) return dew_error(dew_set_char(result,"use LOB.body(<lob>)",0), stack);
  lob_t lob = (lob_t)(args->value);

  dew_t to_set = dew_arg(args, 2, IS_QUOTED);
  if(to_set) lob_body(lob, (uint8_t*)dew_get_char(to_set), dew_get_len(to_set));

  // NOTE using raw body pointer, care must be taken as it becomes invalid
  char *body = (char*)lob_body_get(lob);
  if(!body) body = ""; // no body is empty buffer
  dew_set_char(result, body, lob_body_len(lob));
  return stack;
}

static dew_t LOB_json(dew_t stack, dew_t args, dew_t result)
{
  if(!dew_typeof(args,TYPEOF_LOB)) return dew_error(dew_set_char(result,"use LOB.json(<lob>)",0), stack);
  lob_t lob = (lob_t)(args->value);
  char *json = lob_json(lob);
  dew_set_copy(result,json,0);
  return stack;
}

static dew_t LOB_getter(dew_t stack, dew_t this, char *key, uint8_t len, dew_t result, void *arg)
{
  if (strncmp(key,"create",len) == 0){
    dew_set_fun(result, &LOB_create);
  } else if (strncmp(key,"body",len) == 0) {
    dew_set_fun(result, &LOB_body);
  } else if (strncmp(key,"json",len) == 0) {
    dew_set_fun(result, &LOB_json);
  }else{
    dew_err(dew_set_char(result,"undefined",0));
  }
  return stack;
}

static dew_t tb_lob_get(dew_t stack, dew_t this, char *key, uint8_t len, dew_t result, void *arg)
{
  char *safe = malloc(len+1);
  memcpy(safe,key,len);
  safe[len] = 0;
  char *raw = lob_get_raw(this->value,safe);
  uint16_t rlen = lob_get_len(this->value,safe);
  free(safe);
  if(!raw || !rlen) return dew_error(dew_set_char(result,"undefined",0), stack);

  // since raw is a json value, just eval it
  dew_t val = dew_set_char(dew_new(),raw,rlen);
  stack = dew_eval(stack, val, result);
  dew_free(val);

  return stack;
}

static dew_t tb_lob_set(dew_t stack, dew_t this, char *key, uint8_t len, dew_t val, void *arg)
{
  char* json = dew_json(val);
  lob_set_raw(this->value, key, len, json, strlen(json));
  return stack;
}

static dew_t tb_lob_free(dew_t this, void *arg)
{
  this->value = lob_free(this->value);
  return this;
}

struct dew_type_struct tb_lob_type = {&tb_lob_get, &tb_lob_set, &tb_lob_free, NULL, TYPEOF_LOB};

dew_t dew_lib_lob(dew_t stack)
{
  dew_set_type(stack,&tb_lob_type);
  stack = dew_act_singleton(stack, "LOB", LOB_getter, NULL, NULL);
  return stack;
}

dew_t dew_set_lob(dew_t d, lob_t lob)
{
  return dew_set_object(d,TYPEOF_LOB,lob);
}

lob_t dew_get_lob(dew_t d, bool own)
{
  if(!dew_typeof(d,TYPEOF_LOB)) return LOG_DEBUG("invalid type");
  lob_t ret = d->value;
  if(own) d->value = NULL;
  return ret;
}
