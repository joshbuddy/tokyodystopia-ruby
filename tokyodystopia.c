/*************************************************************************************************
 * Ruby binding of Tokyo Dystopia
 *************************************************************************************************/


#include "ruby.h"
#include <dystopia.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define IDBVNDATA      "@idb"
#define NUMBUFSIZ      32

/* private function prototypes */
static VALUE StringValueEx(VALUE vobj);
static TCLIST *varytolist(VALUE vary);
static VALUE listtovary(TCLIST *list);
static TCMAP *vhashtomap(VALUE vhash);
static VALUE maptovhash(TCMAP *map);

static void idb_init(void);
static VALUE idb_errmsg(int argc, VALUE *argv, VALUE vself);
static VALUE idb_ecode(VALUE vself);
static VALUE idb_initialize(VALUE vself);
static VALUE idb_tune(int argc, VALUE *argv, VALUE vself);
static VALUE idb_setcache(int argc, VALUE *argv, VALUE vself);
static VALUE idb_setfwmmax(int argc, VALUE *argv, VALUE vself);
static VALUE idb_open(int argc, VALUE *argv, VALUE vself);
static VALUE idb_close(VALUE vself);
static VALUE idb_put(VALUE vself, VALUE vkey, VALUE vval);
static VALUE idb_out(VALUE vself, VALUE vkey);
static VALUE idb_get(VALUE vself, VALUE vkey);
static VALUE idb_search(VALUE vself, VALUE vexp, VALUE vmode);
static VALUE idb_compound_search(VALUE vself, VALUE vexp);
static VALUE idb_iterinit(VALUE vself);
static VALUE idb_iternext(VALUE vself);
static VALUE idb_sync(VALUE vself);
static VALUE idb_optimize(VALUE vself);
static VALUE idb_vanish(VALUE vself);
static VALUE idb_copy(VALUE vself, VALUE vpath);
static VALUE idb_path(VALUE vself);
static VALUE idb_rnum(VALUE vself);
static VALUE idb_fsiz(VALUE vself);

/*************************************************************************************************
 * public objects
 *************************************************************************************************/

VALUE mod_tokyodystopia;
VALUE cls_idb;
VALUE cls_idb_data;

int Init_tokyodystopia(void){
  mod_tokyodystopia = rb_define_module("TokyoDystopia");
  rb_define_const(mod_tokyodystopia, "VERSION", rb_str_new2(tcversion));
  idb_init();
  return 0;
}



/*************************************************************************************************
 * private objects
 *************************************************************************************************/


static VALUE StringValueEx(VALUE vobj){
  char kbuf[NUMBUFSIZ];
  int ksiz;
  switch(TYPE(vobj)){
  case T_FIXNUM:
    ksiz = sprintf(kbuf, "%d", (int)FIX2INT(vobj));
    return rb_str_new(kbuf, ksiz);
  case T_BIGNUM:
    ksiz = sprintf(kbuf, "%lld", (long long)NUM2LL(vobj));
    return rb_str_new(kbuf, ksiz);
  case T_TRUE:
    ksiz = sprintf(kbuf, "true");
    return rb_str_new(kbuf, ksiz);
  case T_FALSE:
    ksiz = sprintf(kbuf, "false");
    return rb_str_new(kbuf, ksiz);
  case T_NIL:
    ksiz = sprintf(kbuf, "nil");
    return rb_str_new(kbuf, ksiz);
  }
  return StringValue(vobj);
}


static TCLIST *varytolist(VALUE vary){
  VALUE vval;
  TCLIST *list;
  int i, num;
  num = RARRAY_LEN(vary);
  list = tclistnew2(num);
  for(i = 0; i < num; i++){
    vval = rb_ary_entry(vary, i);
    vval = StringValueEx(vval);
    tclistpush(list, RSTRING_PTR(vval), RSTRING_LEN(vval));
  }
  return list;
}



static VALUE longstovary(uint64_t *list, int num){
  VALUE vary;
  int i;
  vary = rb_ary_new2(num);
  for(i = 0; i < num; i++){
    rb_ary_push(vary, LL2NUM(list[i]));
  }
  return vary;
}

static VALUE listtovary(TCLIST *list){
  VALUE vary;
  const char *vbuf;
  int i, num, vsiz;
  num = tclistnum(list);
  vary = rb_ary_new2(num);
  for(i = 0; i < num; i++){
    vbuf = tclistval(list, i, &vsiz);
    rb_ary_push(vary, rb_str_new(vbuf, vsiz));
  }
  return vary;
}


static TCMAP *vhashtomap(VALUE vhash){
  VALUE vkeys, vkey, vval;
  TCMAP *map;
  int i, num;
  map = tcmapnew2(31);
  vkeys = rb_funcall(vhash, rb_intern("keys"), 0);
  num = RARRAY_LEN(vkeys);
  for(i = 0; i < num; i++){
    vkey = rb_ary_entry(vkeys, i);
    vval = rb_hash_aref(vhash, vkey);
    vkey = StringValueEx(vkey);
    vval = StringValueEx(vval);
    tcmapput(map, RSTRING_PTR(vkey), RSTRING_LEN(vkey), RSTRING_PTR(vval), RSTRING_LEN(vval));
  }
  return map;
}


static VALUE maptovhash(TCMAP *map){
  const char *kbuf, *vbuf;
  int ksiz, vsiz;
  VALUE vhash;
  vhash = rb_hash_new();
  tcmapiterinit(map);
  while((kbuf = tcmapiternext(map, &ksiz)) != NULL){
    vbuf = tcmapiterval(kbuf, &vsiz);
    rb_hash_aset(vhash, rb_str_new(kbuf, ksiz), rb_str_new(vbuf, vsiz));
  }
  return vhash;
}


static void idb_init(void){
  cls_idb = rb_define_class_under(mod_tokyodystopia, "IDB", rb_cObject);
  rb_define_const(cls_idb, "ESUCCESS", INT2NUM(TCESUCCESS));
  rb_define_const(cls_idb, "ETHREAD", INT2NUM(TCETHREAD));
  rb_define_const(cls_idb, "EINVALID", INT2NUM(TCEINVALID));
  rb_define_const(cls_idb, "ENOFILE", INT2NUM(TCENOFILE));
  rb_define_const(cls_idb, "ENOPERM", INT2NUM(TCENOPERM));
  rb_define_const(cls_idb, "EMETA", INT2NUM(TCEMETA));
  rb_define_const(cls_idb, "ERHEAD", INT2NUM(TCERHEAD));
  rb_define_const(cls_idb, "EOPEN", INT2NUM(TCEOPEN));
  rb_define_const(cls_idb, "ECLOSE", INT2NUM(TCECLOSE));
  rb_define_const(cls_idb, "ETRUNC", INT2NUM(TCETRUNC));
  rb_define_const(cls_idb, "ESYNC", INT2NUM(TCESYNC));
  rb_define_const(cls_idb, "ESTAT", INT2NUM(TCESTAT));
  rb_define_const(cls_idb, "ESEEK", INT2NUM(TCESEEK));
  rb_define_const(cls_idb, "EREAD", INT2NUM(TCEREAD));
  rb_define_const(cls_idb, "EWRITE", INT2NUM(TCEWRITE));
  rb_define_const(cls_idb, "EMMAP", INT2NUM(TCEMMAP));
  rb_define_const(cls_idb, "ELOCK", INT2NUM(TCELOCK));
  rb_define_const(cls_idb, "EUNLINK", INT2NUM(TCEUNLINK));
  rb_define_const(cls_idb, "ERENAME", INT2NUM(TCERENAME));
  rb_define_const(cls_idb, "EMKDIR", INT2NUM(TCEMKDIR));
  rb_define_const(cls_idb, "ERMDIR", INT2NUM(TCERMDIR));
  rb_define_const(cls_idb, "EKEEP", INT2NUM(TCEKEEP));
  rb_define_const(cls_idb, "ENOREC", INT2NUM(TCENOREC));
  rb_define_const(cls_idb, "EMISC", INT2NUM(TCEMISC));
  rb_define_const(cls_idb, "TLARGE", INT2NUM(HDBTLARGE));
  rb_define_const(cls_idb, "TDEFLATE", INT2NUM(HDBTDEFLATE));
  rb_define_const(cls_idb, "TBZIP", INT2NUM(HDBTBZIP));
  rb_define_const(cls_idb, "TTCBS", INT2NUM(HDBTTCBS));
  rb_define_const(cls_idb, "OREADER", INT2NUM(HDBOREADER));
  rb_define_const(cls_idb, "OWRITER", INT2NUM(HDBOWRITER));
  rb_define_const(cls_idb, "OCREAT", INT2NUM(HDBOCREAT));
  rb_define_const(cls_idb, "OTRUNC", INT2NUM(HDBOTRUNC));
  rb_define_const(cls_idb, "ONOLCK", INT2NUM(HDBONOLCK));
  rb_define_const(cls_idb, "OLCKNB", INT2NUM(HDBOLCKNB));
  rb_define_const(cls_idb, "OTSYNC", INT2NUM(HDBOTSYNC));
  rb_define_private_method(cls_idb, "initialize", idb_initialize, 0);
  rb_define_method(cls_idb, "errmsg", idb_errmsg, -1);
  rb_define_method(cls_idb, "ecode", idb_ecode, 0);
  rb_define_method(cls_idb, "tune", idb_tune, -1);
  rb_define_method(cls_idb, "setcache", idb_setcache, 2);
  rb_define_method(cls_idb, "setfwmmax", idb_setfwmmax, -1);
  rb_define_method(cls_idb, "open", idb_open, -1);
  rb_define_method(cls_idb, "close", idb_close, 0);
  rb_define_method(cls_idb, "put", idb_put, 2);
  rb_define_method(cls_idb, "out", idb_out, 1);
  rb_define_method(cls_idb, "get", idb_get, 1);
  rb_define_method(cls_idb, "search", idb_search, 2);
  rb_define_method(cls_idb, "search_compound", idb_compound_search, 1);
  rb_define_method(cls_idb, "iterinit", idb_iterinit, 0);
  rb_define_method(cls_idb, "iternext", idb_iternext, 0);
  rb_define_method(cls_idb, "sync", idb_sync, 0);
  rb_define_method(cls_idb, "optimize", idb_optimize, 0);
  rb_define_method(cls_idb, "vanish", idb_vanish, 0);
  rb_define_method(cls_idb, "copy", idb_copy, 1);
  rb_define_method(cls_idb, "path", idb_path, 0);
  rb_define_method(cls_idb, "rnum", idb_rnum, 0);
  rb_define_method(cls_idb, "fsiz", idb_fsiz, 0);
}


static VALUE idb_initialize(VALUE vself){
  VALUE vidb;
  TCIDB *idb;
  idb = tcidbnew();
  vidb = Data_Wrap_Struct(cls_idb_data, 0, tcidbdel, idb);
  rb_iv_set(vself, IDBVNDATA, vidb);
  return Qnil;
}


static VALUE idb_errmsg(int argc, VALUE *argv, VALUE vself){
  VALUE vidb, vecode;
  TCIDB *idb;
  const char *msg;
  int ecode;
  rb_scan_args(argc, argv, "01", &vecode);
  vidb = rb_iv_get(vself, IDBVNDATA);
  Data_Get_Struct(vidb, TCIDB, idb);
  ecode = (vecode == Qnil) ? tcidbecode(idb) : NUM2INT(vecode);
  msg = tcidberrmsg(ecode);
  return rb_str_new2(msg);
}


static VALUE idb_ecode(VALUE vself){
  VALUE vidb;
  TCIDB *idb;
  vidb = rb_iv_get(vself, IDBVNDATA);
  Data_Get_Struct(vidb, TCIDB, idb);
  return INT2NUM(tcidbecode(idb));
}


static VALUE idb_tune(int argc, VALUE *argv, VALUE vself){
  VALUE vidb, vbnum, vapow, vfpow, vopts;
  TCIDB *idb;
  int apow, fpow, opts;
  int64_t bnum;
  rb_scan_args(argc, argv, "04", &vbnum, &vapow, &vfpow, &vopts);
  bnum = (vbnum == Qnil) ? -1 : NUM2LL(vbnum);
  apow = (vapow == Qnil) ? -1 : NUM2INT(vapow);
  fpow = (vfpow == Qnil) ? -1 : NUM2INT(vfpow);
  opts = (vopts == Qnil) ? 0 : NUM2INT(vopts);
  vidb = rb_iv_get(vself, IDBVNDATA);
  Data_Get_Struct(vidb, TCIDB, idb);
  return tcidbtune(idb, bnum, apow, fpow, opts) ? Qtrue : Qfalse;
}


static VALUE idb_setcache(int argc, VALUE *argv, VALUE vself){
  VALUE vidb, vicsiz, vlcnum;
  TCIDB *idb;
  int64_t icsiz;
  int32_t lcnum;
  rb_scan_args(argc, argv, "02", &vicsiz, &vlcnum);
  icsiz = (vicsiz == Qnil) ? -1 : NUM2LL(vicsiz);
  lcnum = (vlcnum == Qnil) ? -1 : NUM2INT(vlcnum);
  vidb = rb_iv_get(vself, IDBVNDATA);
  Data_Get_Struct(vidb, TCIDB, idb);
  return tcidbsetcache(idb, icsiz, lcnum) ? Qtrue : Qfalse;
}


static VALUE idb_setfwmmax(int argc, VALUE *argv, VALUE vself){
  VALUE vidb, vfwmax;
  TCIDB *idb;
  uint32_t xfwmax;
  rb_scan_args(argc, argv, "01", &vfwmax);
  xfwmax = (vfwmax == Qnil) ? -1 : NUM2LL(vfwmax);
  vidb = rb_iv_get(vself, IDBVNDATA);
  Data_Get_Struct(vidb, TCIDB, idb);
  return tcidbsetfwmmax(idb, xfwmax) ? Qtrue : Qfalse;
}


static VALUE idb_open(int argc, VALUE *argv, VALUE vself){
  VALUE vidb, vpath, vomode;
  TCIDB *idb;
  int omode;
  rb_scan_args(argc, argv, "11", &vpath, &vomode);
  Check_Type(vpath, T_STRING);
  omode = (vomode == Qnil) ? HDBOREADER : NUM2INT(vomode);
  vidb = rb_iv_get(vself, IDBVNDATA);
  Data_Get_Struct(vidb, TCIDB, idb);
  return tcidbopen(idb, RSTRING_PTR(vpath), omode) ? Qtrue : Qfalse;
}


static VALUE idb_close(VALUE vself){
  VALUE vidb;
  TCIDB *idb;
  vidb = rb_iv_get(vself, IDBVNDATA);
  Data_Get_Struct(vidb, TCIDB, idb);
  return tcidbclose(idb) ? Qtrue : Qfalse;
}


static VALUE idb_put(VALUE vself, VALUE vkey, VALUE vval){
  VALUE vidb;
  TCIDB *idb;
  int64_t xkey = NUM2LL(vkey);
  vval = StringValueEx(vval);
  vidb = rb_iv_get(vself, IDBVNDATA);
  Data_Get_Struct(vidb, TCIDB, idb);
  return tcidbput(idb, xkey, RSTRING_PTR(vval)) ? Qtrue : Qfalse;
}

static VALUE idb_out(VALUE vself, VALUE vkey){
  VALUE vidb;
  TCIDB *idb;
  int64_t xkey = NUM2LL(vkey);
  vidb = rb_iv_get(vself, IDBVNDATA);
  Data_Get_Struct(vidb, TCIDB, idb);
  return tcidbout(idb, xkey) ? Qtrue : Qfalse;
}

static VALUE idb_get(VALUE vself, VALUE vkey){
  VALUE vidb, vval;
  TCIDB *idb;
  const char *vget;
  int64_t xkey = NUM2LL(vkey);
  vidb = rb_iv_get(vself, IDBVNDATA);
  Data_Get_Struct(vidb, TCIDB, idb);
  if(!(vget = tcidbget(idb, xkey))) return Qnil;
  vval = rb_str_new2(vget);
  return vval;
}

static VALUE idb_iterinit(VALUE vself){
  VALUE vidb;
  TCIDB *idb;
  vidb = rb_iv_get(vself, IDBVNDATA);
  Data_Get_Struct(vidb, TCIDB, idb);
  return tcidbiterinit(idb) ? Qtrue : Qfalse;
}

static VALUE idb_iternext(VALUE vself){
  VALUE vidb;
  TCIDB *idb;
  vidb = rb_iv_get(vself, IDBVNDATA);
  Data_Get_Struct(vidb, TCIDB, idb);
  return LL2NUM(tcidbiternext(idb));
}

static VALUE idb_search(VALUE vself, VALUE vexp, VALUE vmode){
  VALUE vidb, vary;
  TCIDB *idb;
  uint64_t *keys;
  int np;
  int xmode;
  const char *xexp;
  vidb = rb_iv_get(vself, IDBVNDATA);
  Data_Get_Struct(vidb, TCIDB, idb);
  xmode = NUM2INT(vmode);
  xexp = StringValuePtr(vexp);
  keys = tcidbsearch(idb, xexp, xmode, &np);
  vary = longstovary(keys, (int)np);
  tcfree(keys);
  return vary;
}

static VALUE idb_compound_search(VALUE vself, VALUE vexp){
  VALUE vidb, vary;
  TCIDB *idb;
  uint64_t *keys;
  int np;
  const char *xexp;
  vidb = rb_iv_get(vself, IDBVNDATA);
  Data_Get_Struct(vidb, TCIDB, idb);
  xexp = StringValuePtr(vexp);
  keys = tcidbsearch2(idb, xexp, &np);
  vary = longstovary(keys, (int)np);
  tcfree(keys);
  return vary;
}

static VALUE idb_sync(VALUE vself){
  VALUE vidb;
  TCIDB *idb;
  vidb = rb_iv_get(vself, IDBVNDATA);
  Data_Get_Struct(vidb, TCIDB, idb);
  return tcidbsync(idb) ? Qtrue : Qfalse;
}

static VALUE idb_optimize(VALUE vself){
  VALUE vidb;
  TCIDB *idb;
  vidb = rb_iv_get(vself, IDBVNDATA);
  Data_Get_Struct(vidb, TCIDB, idb);
  return tcidboptimize(idb) ? Qtrue : Qfalse;
}

static VALUE idb_vanish(VALUE vself){
  VALUE vidb;
  TCIDB *idb;
  vidb = rb_iv_get(vself, IDBVNDATA);
  Data_Get_Struct(vidb, TCIDB, idb);
  return tcidbvanish(idb) ? Qtrue : Qfalse;
}

static VALUE idb_copy(VALUE vself, VALUE vpath){
  VALUE vidb;
  TCIDB *idb;
  Check_Type(vpath, T_STRING);
  vidb = rb_iv_get(vself, IDBVNDATA);
  Data_Get_Struct(vidb, TCIDB, idb);
  return tcidbcopy(idb, RSTRING_PTR(vpath)) ? Qtrue : Qfalse;
}

static VALUE idb_path(VALUE vself){
  VALUE vidb;
  TCIDB *idb;
  const char *path;
  vidb = rb_iv_get(vself, IDBVNDATA);
  Data_Get_Struct(vidb, TCIDB, idb);
  if(!(path = tcidbpath(idb))) return Qnil;
  return rb_str_new2(path);
}

static VALUE idb_rnum(VALUE vself){
  VALUE vidb;
  TCIDB *idb;
  vidb = rb_iv_get(vself, IDBVNDATA);
  Data_Get_Struct(vidb, TCIDB, idb);
  return LL2NUM(tcidbrnum(idb));
}


static VALUE idb_fsiz(VALUE vself){
  VALUE vidb;
  TCIDB *idb;
  vidb = rb_iv_get(vself, IDBVNDATA);
  Data_Get_Struct(vidb, TCIDB, idb);
  return LL2NUM(tcidbfsiz(idb));
}

/* END OF FILE */
