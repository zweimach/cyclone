#include <setjmp.h>
/* This is a C header file to be used by the output of the Cyclone to
   C translator.  The corresponding definitions are in file
   lib/runtime_cyc.c
*/
#ifndef _CYC_INCLUDE_H_
#define _CYC_INCLUDE_H_

#ifdef NO_CYC_PREFIX
#define ADD_PREFIX(x) x
#else
#define ADD_PREFIX(x) Cyc_##x
#endif

#ifndef offsetof
/* should be size_t, but int is fine. */
#define offsetof(t,n) ((int)(&(((t *)0)->n)))
#endif

/* Tagged arrays */
struct _dyneither_ptr {
  unsigned char *curr; 
  unsigned char *base; 
  unsigned char *last_plus_one; 
};  

/* Discriminated Unions */
struct _xtunion_struct { char *tag; };

/* Need one of these per thread (we don't have threads)
   The runtime maintains a stack that contains either _handler_cons
   structs or _RegionHandle structs.  The tag is 0 for a handler_cons
   and 1 for a region handle.  */
struct _RuntimeStack {
  int tag; /* 0 for an exception handler, 1 for a region handle */
  struct _RuntimeStack *next;
};

/* Regions */
struct _RegionPage {
#ifdef CYC_REGION_PROFILE
  unsigned total_bytes;
  unsigned free_bytes;
#endif
  struct _RegionPage *next;
  char data[1];  /*FJS: used to be size 0, but that's forbidden in ansi c*/
};

struct _RegionHandle {
  struct _RuntimeStack s;
  struct _RegionPage *curr;
  char               *offset;
  char               *last_plus_one;
  struct _DynRegionHandle *sub_regions;
#ifdef CYC_REGION_PROFILE
  const char         *name;
#else
  unsigned used_bytes;
  unsigned wasted_bytes;
#endif
};

struct _DynRegionFrame {
  struct _RuntimeStack s;
  struct _DynRegionHandle *x;
};

// A dynamic region is just a region handle.  We have the
// wrapper struct for type abstraction reasons.
struct Cyc_Core_DynamicRegion {
  struct _RegionHandle h;
};

extern struct _RegionHandle _new_region(const char *);
extern void * _region_malloc(struct _RegionHandle *, unsigned);
extern void * _region_calloc(struct _RegionHandle *, unsigned t, unsigned n);
extern void   _free_region(struct _RegionHandle *);
extern void   _reset_region(struct _RegionHandle *);
extern struct _RegionHandle *_open_dynregion(struct _DynRegionFrame *f,
                                             struct _DynRegionHandle *h);
extern void   _pop_dynregion();

/* Exceptions */
struct _handler_cons {
  struct _RuntimeStack s;
  jmp_buf handler;
};
extern void _push_handler(struct _handler_cons *);
extern void _push_region(struct _RegionHandle *);
extern void _npop_handler(int);
extern void _pop_handler();
extern void _pop_region();

#ifndef _throw
extern int _throw_null_fn(const char *filename, unsigned lineno);
extern int _throw_arraybounds_fn(const char *filename, unsigned lineno);
extern int _throw_badalloc_fn(const char *filename, unsigned lineno);
extern int _throw_match_fn(const char *filename, unsigned lineno);
extern int _throw_fn(void* e, const char *filename, unsigned lineno);
extern int _rethrow(void* e);
#define _throw_null() (_throw_null_fn(__FILE__,__LINE__))
#define _throw_arraybounds() (_throw_arraybounds_fn(__FILE__,__LINE__))
#define _throw_badalloc() (_throw_badalloc_fn(__FILE__,__LINE__))
#define _throw_match() (_throw_match_fn(__FILE__,__LINE__))
#define _throw(e) (_throw_fn((e),__FILE__,__LINE__))
#endif

extern struct _xtunion_struct *_exn_thrown;

/* Built-in Exceptions */
struct Cyc_Null_Exception_exn_struct { char *tag; };
struct Cyc_Array_bounds_exn_struct { char *tag; };
struct Cyc_Match_Exception_exn_struct { char *tag; };
struct Cyc_Bad_alloc_exn_struct { char *tag; };
extern char Cyc_Null_Exception[];
extern char Cyc_Array_bounds[];
extern char Cyc_Match_Exception[];
extern char Cyc_Bad_alloc[];

/* Built-in Run-time Checks and company */
#ifdef __APPLE__
#define _INLINE_FUNCTIONS
#endif

#ifdef CYC_ANSI_OUTPUT
#define _INLINE  
#define _INLINE_FUNCTIONS
#else
#define _INLINE inline
#endif

#ifdef VC_C
#define _CYC_U_LONG_LONG_T __int64
#else
#ifdef GCC_C
#define _CYC_U_LONG_LONG_T unsigned long long
#else
#define _CYC_U_LONG_LONG_T unsigned long long
#endif
#endif

#ifdef NO_CYC_NULL_CHECKS
#define _check_null(ptr) (ptr)
#else
#ifdef _INLINE_FUNCTIONS
static _INLINE void *
_check_null_fn(const void *ptr, const char *filename, unsigned lineno) {
  void*_check_null_temp = (void*)(ptr);
  if (!_check_null_temp) _throw_null_fn(filename,lineno);
  return _check_null_temp;
}
#define _check_null(p) (_check_null_fn((p),__FILE__,__LINE__))
#else
#define _check_null(ptr) \
  ({ void*_check_null_temp = (void*)(ptr); \
     if (!_check_null_temp) _throw_null(); \
     _check_null_temp; })
#endif
#endif

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_known_subscript_null(ptr,bound,elt_sz,index) ({ \
  ((char *)ptr) + (elt_sz)*(index); })
#else
#ifdef _INLINE_FUNCTIONS
static _INLINE char *
_check_known_subscript_null_fn(void *ptr, unsigned bound, unsigned elt_sz, unsigned index, const char *filename, unsigned lineno) {
  void*_cks_ptr = (void*)(ptr);
  unsigned _cks_bound = (bound);
  unsigned _cks_elt_sz = (elt_sz);
  unsigned _cks_index = (index);
  if (!_cks_ptr) _throw_null_fn(filename,lineno);
  if (_cks_index >= _cks_bound) _throw_arraybounds_fn(filename,lineno);
  return ((char *)_cks_ptr) + _cks_elt_sz*_cks_index;
}
#define _check_known_subscript_null(p,b,e) (_check_known_subscript_null_fn(p,b,e,__FILE__,__LINE__))
#else
#define _check_known_subscript_null(ptr,bound,elt_sz,index) ({ \
  void*_cks_ptr = (void*)(ptr); \
  unsigned _cks_bound = (bound); \
  unsigned _cks_elt_sz = (elt_sz); \
  unsigned _cks_index = (index); \
  if (!_cks_ptr) _throw_null(); \
  if (_cks_index >= _cks_bound) _throw_arraybounds(); \
  ((char *)_cks_ptr) + _cks_elt_sz*_cks_index; })
#endif
#endif

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_known_subscript_notnull(bound,index) (index)
#else
#ifdef _INLINE_FUNCTIONS
static _INLINE unsigned
_check_known_subscript_notnull_fn(unsigned bound,unsigned index,const char *filename,unsigned lineno) { 
  unsigned _cksnn_bound = (bound); 
  unsigned _cksnn_index = (index); 
  if (_cksnn_index >= _cksnn_bound) _throw_arraybounds_fn(filename,lineno); 
  return _cksnn_index;
}
#define _check_known_subscript_notnull(b,i) (_check_known_subscript_notnull_fn(b,i,__FILE__,__LINE__))
#else
#define _check_known_subscript_notnull(bound,index) ({ \
  unsigned _cksnn_bound = (bound); \
  unsigned _cksnn_index = (index); \
  if (_cksnn_index >= _cksnn_bound) _throw_arraybounds(); \
  _cksnn_index; })
#endif
#endif

/* Add i to zero-terminated pointer x.  Checks for x being null and
   ensures that x[0..i-1] are not 0. */
#ifdef NO_CYC_BOUNDS_CHECK
#define _zero_arr_plus_char_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_short_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_int_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_float_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_double_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_longdouble_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_voidstar_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#else
static _INLINE char *
_zero_arr_plus_char_fn(char *orig_x, unsigned int orig_sz, int orig_i,const char *filename, unsigned lineno) {
  unsigned int _czs_temp;
  if ((orig_x) == 0) _throw_null_fn(filename,lineno);
  if (orig_i < 0 || orig_sz == 0) _throw_arraybounds_fn(filename,lineno);
  for (_czs_temp=orig_sz-1; _czs_temp < orig_i; _czs_temp++)
    if (orig_x[_czs_temp] == 0) _throw_arraybounds_fn(filename,lineno);
  return orig_x + orig_i;
}
static _INLINE short *
_zero_arr_plus_short_fn(short *orig_x, unsigned int orig_sz, int orig_i,const char *filename, unsigned lineno) {
  unsigned int _czs_temp;
  if ((orig_x) == 0) _throw_null_fn(filename,lineno);
  if (orig_i < 0 || orig_sz == 0) _throw_arraybounds_fn(filename,lineno);
  for (_czs_temp=orig_sz-1; _czs_temp < orig_i; _czs_temp++)
    if (orig_x[_czs_temp] == 0) _throw_arraybounds_fn(filename,lineno);
  return orig_x + orig_i;
}
static _INLINE int *
_zero_arr_plus_int_fn(int *orig_x, unsigned int orig_sz, int orig_i, const char *filename, unsigned lineno) {
  unsigned int _czs_temp;
  if ((orig_x) == 0) _throw_null_fn(filename,lineno);
  if (orig_i < 0 || orig_sz == 0) _throw_arraybounds_fn(filename,lineno);
  for (_czs_temp=orig_sz-1; _czs_temp < orig_i; _czs_temp++)
    if (orig_x[_czs_temp] == 0) _throw_arraybounds_fn(filename,lineno);
  return orig_x + orig_i;
}
static _INLINE float *
_zero_arr_plus_float_fn(float *orig_x, unsigned int orig_sz, int orig_i,const char *filename, unsigned lineno) {
  unsigned int _czs_temp;
  if ((orig_x) == 0) _throw_null_fn(filename,lineno);
  if (orig_i < 0 || orig_sz == 0) _throw_arraybounds_fn(filename,lineno);
  for (_czs_temp=orig_sz-1; _czs_temp < orig_i; _czs_temp++)
    if (orig_x[_czs_temp] == 0) _throw_arraybounds_fn(filename,lineno);
  return orig_x + orig_i;
}
static _INLINE double *
_zero_arr_plus_double_fn(double *orig_x, unsigned int orig_sz, int orig_i,const char *filename, unsigned lineno) {
  unsigned int _czs_temp;
  if ((orig_x) == 0) _throw_null_fn(filename,lineno);
  if (orig_i < 0 || orig_sz == 0) _throw_arraybounds_fn(filename,lineno);
  for (_czs_temp=orig_sz-1; _czs_temp < orig_i; _czs_temp++)
    if (orig_x[_czs_temp] == 0) _throw_arraybounds_fn(filename,lineno);
  return orig_x + orig_i;
}
static _INLINE long double *
_zero_arr_plus_longdouble_fn(long double *orig_x, unsigned int orig_sz, int orig_i, const char *filename, unsigned lineno) {
  unsigned int _czs_temp;
  if ((orig_x) == 0) _throw_null_fn(filename,lineno);
  if (orig_i < 0 || orig_sz == 0) _throw_arraybounds_fn(filename,lineno);
  for (_czs_temp=orig_sz-1; _czs_temp < orig_i; _czs_temp++)
    if (orig_x[_czs_temp] == 0) _throw_arraybounds_fn(filename,lineno);
  return orig_x + orig_i;
}
static _INLINE void *
_zero_arr_plus_voidstar_fn(void **orig_x, unsigned int orig_sz, int orig_i,const char *filename,unsigned lineno) {
  unsigned int _czs_temp;
  if ((orig_x) == 0) _throw_null_fn(filename,lineno);
  if (orig_i < 0 || orig_sz == 0) _throw_arraybounds_fn(filename,lineno);
  for (_czs_temp=orig_sz-1; _czs_temp < orig_i; _czs_temp++)
    if (orig_x[_czs_temp] == 0) _throw_arraybounds_fn(filename,lineno);
  return orig_x + orig_i;
}
#endif

#define _zero_arr_plus_char(x,s,i) \
  (_zero_arr_plus_char_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_short(x,s,i) \
  (_zero_arr_plus_short_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_int(x,s,i) \
  (_zero_arr_plus_int_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_float(x,s,i) \
  (_zero_arr_plus_float_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_double(x,s,i) \
  (_zero_arr_plus_double_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_longdouble(x,s,i) \
  (_zero_arr_plus_longdouble_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_voidstar(x,s,i) \
  (_zero_arr_plus_voidstar_fn(x,s,i,__FILE__,__LINE__))


/* Calculates the number of elements in a zero-terminated, thin array.
   If non-null, the array is guaranteed to have orig_offset elements. */
static _INLINE int
_get_zero_arr_size_char(const char *orig_x, unsigned int orig_offset) {
  const char *_gres_x = orig_x;
  unsigned int _gres = 0;
  if (_gres_x != 0) {
     _gres = orig_offset;
     _gres_x += orig_offset - 1;
     while (*_gres_x != 0) { _gres_x++; _gres++; }
  }
  return _gres; 
}
static _INLINE int
_get_zero_arr_size_short(const short *orig_x, unsigned int orig_offset) {
  const short *_gres_x = orig_x;
  unsigned int _gres = 0;
  if (_gres_x != 0) {
     _gres = orig_offset;
     _gres_x += orig_offset - 1;
     while (*_gres_x != 0) { _gres_x++; _gres++; }
  }
  return _gres; 
}
static _INLINE int
_get_zero_arr_size_int(const int *orig_x, unsigned int orig_offset) {
  const int *_gres_x = orig_x;
  unsigned int _gres = 0;
  if (_gres_x != 0) {
     _gres = orig_offset;
     _gres_x += orig_offset - 1;
     while (*_gres_x != 0) { _gres_x++; _gres++; }
  }
  return _gres; 
}
static _INLINE int
_get_zero_arr_size_float(const float *orig_x, unsigned int orig_offset) {
  const float *_gres_x = orig_x;
  unsigned int _gres = 0;
  if (_gres_x != 0) {
     _gres = orig_offset;
     _gres_x += orig_offset - 1;
     while (*_gres_x != 0) { _gres_x++; _gres++; }
  }
  return _gres; 
}
static _INLINE int
_get_zero_arr_size_double(const double *orig_x, unsigned int orig_offset) {
  const double *_gres_x = orig_x;
  unsigned int _gres = 0;
  if (_gres_x != 0) {
     _gres = orig_offset;
     _gres_x += orig_offset - 1;
     while (*_gres_x != 0) { _gres_x++; _gres++; }
  }
  return _gres; 
}
static _INLINE int
_get_zero_arr_size_longdouble(const long double *orig_x, unsigned int orig_offset) {
  const long double *_gres_x = orig_x;
  unsigned int _gres = 0;
  if (_gres_x != 0) {
     _gres = orig_offset;
     _gres_x += orig_offset - 1;
     while (*_gres_x != 0) { _gres_x++; _gres++; }
  }
  return _gres; 
}
static _INLINE int
_get_zero_arr_size_voidstar(const void **orig_x, unsigned int orig_offset) {
  const void **_gres_x = orig_x;
  unsigned int _gres = 0;
  if (_gres_x != 0) {
     _gres = orig_offset;
     _gres_x += orig_offset - 1;
     while (*_gres_x != 0) { _gres_x++; _gres++; }
  }
  return _gres; 
}


/* Does in-place addition of a zero-terminated pointer (x += e and ++x).  
   Note that this expands to call _zero_arr_plus_<type>_fn. */
static _INLINE char *
_zero_arr_inplace_plus_char_fn(char **x, int orig_i,const char *filename,unsigned lineno) {
  *x = _zero_arr_plus_char_fn(*x,1,orig_i,filename,lineno);
  return *x;
}
#define _zero_arr_inplace_plus_char(x,i) \
  _zero_arr_inplace_plus_char_fn((char **)(x),i,__FILE__,__LINE__)
static _INLINE short *
_zero_arr_inplace_plus_short_fn(short **x, int orig_i,const char *filename,unsigned lineno) {
  *x = _zero_arr_plus_short_fn(*x,1,orig_i,filename,lineno);
  return *x;
}
#define _zero_arr_inplace_plus_short(x,i) \
  _zero_arr_inplace_plus_short_fn((short **)(x),i,__FILE__,__LINE__)
static _INLINE int *
_zero_arr_inplace_plus_int(int **x, int orig_i,const char *filename,unsigned lineno) {
  *x = _zero_arr_plus_int_fn(*x,1,orig_i,filename,lineno);
  return *x;
}
#define _zero_arr_inplace_plus_int(x,i) \
  _zero_arr_inplace_plus_int_fn((int **)(x),i,__FILE__,__LINE__)
static _INLINE float *
_zero_arr_inplace_plus_float_fn(float **x, int orig_i,const char *filename,unsigned lineno) {
  *x = _zero_arr_plus_float_fn(*x,1,orig_i,filename,lineno);
  return *x;
}
#define _zero_arr_inplace_plus_float(x,i) \
  _zero_arr_inplace_plus_float_fn((float **)(x),i,__FILE__,__LINE__)
static _INLINE double *
_zero_arr_inplace_plus_double_fn(double **x, int orig_i,const char *filename,unsigned lineno) {
  *x = _zero_arr_plus_double_fn(*x,1,orig_i,filename,lineno);
  return *x;
}
#define _zero_arr_inplace_plus_double(x,i) \
  _zero_arr_inplace_plus_double_fn((double **)(x),i,__FILE__,__LINE__)
static _INLINE long double *
_zero_arr_inplace_plus_longdouble_fn(long double **x, int orig_i,const char *filename,unsigned lineno) {
  *x = _zero_arr_plus_longdouble_fn(*x,1,orig_i,filename,lineno);
  return *x;
}
#define _zero_arr_inplace_plus_longdouble(x,i) \
  _zero_arr_inplace_plus_longdouble_fn((long double **)(x),i,__FILE__,__LINE__)
static _INLINE void *
_zero_arr_inplace_plus_voidstar_fn(void ***x, int orig_i,const char *filename,unsigned lineno) {
  *x = _zero_arr_plus_voidstar_fn(*x,1,orig_i,filename,lineno);
  return *x;
}
#define _zero_arr_inplace_plus_voidstar(x,i) \
  _zero_arr_inplace_plus_voidstar_fn((void ***)(x),i,__FILE__,__LINE__)

/* Does in-place increment of a zero-terminated pointer (e.g., x++). */
static _INLINE char *
_zero_arr_inplace_plus_post_char_fn(char **x, int orig_i,const char *filename,unsigned lineno){
  char * _zap_res = *x;
  *x = _zero_arr_plus_char_fn(_zap_res,1,orig_i,filename,lineno);
  return _zap_res;
}
#define _zero_arr_inplace_plus_post_char(x,i) \
  _zero_arr_inplace_plus_post_char_fn((char **)(x),(i),__FILE__,__LINE__)
static _INLINE short *
_zero_arr_inplace_plus_post_short_fn(short **x, int orig_i,const char *filename,unsigned lineno){
  short * _zap_res = *x;
  *x = _zero_arr_plus_short_fn(_zap_res,1,orig_i,filename,lineno);
  return _zap_res;
}
#define _zero_arr_inplace_plus_post_short(x,i) \
  _zero_arr_inplace_plus_post_short_fn((short **)(x),(i),__FILE__,__LINE__)
static _INLINE int *
_zero_arr_inplace_plus_post_int_fn(int **x, int orig_i,const char *filename, unsigned lineno){
  int * _zap_res = *x;
  *x = _zero_arr_plus_int_fn(_zap_res,1,orig_i,filename,lineno);
  return _zap_res;
}
#define _zero_arr_inplace_plus_post_int(x,i) \
  _zero_arr_inplace_plus_post_int_fn((int **)(x),(i),__FILE__,__LINE__)
static _INLINE float *
_zero_arr_inplace_plus_post_float_fn(float **x, int orig_i,const char *filename, unsigned lineno){
  float * _zap_res = *x;
  *x = _zero_arr_plus_float_fn(_zap_res,1,orig_i,filename,lineno);
  return _zap_res;
}
#define _zero_arr_inplace_plus_post_float(x,i) \
  _zero_arr_inplace_plus_post_float_fn((float **)(x),(i),__FILE__,__LINE__)
static _INLINE double *
_zero_arr_inplace_plus_post_double_fn(double **x, int orig_i,const char *filename,unsigned lineno){
  double * _zap_res = *x;
  *x = _zero_arr_plus_double_fn(_zap_res,1,orig_i,filename,lineno);
  return _zap_res;
}
#define _zero_arr_inplace_plus_post_double(x,i) \
  _zero_arr_inplace_plus_post_double_fn((double **)(x),(i),__FILE__,__LINE__)
static _INLINE long double *
_zero_arr_inplace_plus_post_longdouble_fn(long double **x, int orig_i,const char *filename,unsigned lineno){
  long double * _zap_res = *x;
  *x = _zero_arr_plus_longdouble_fn(_zap_res,1,orig_i,filename,lineno);
  return _zap_res;
}
#define _zero_arr_inplace_plus_post_longdouble(x,i) \
  _zero_arr_inplace_plus_post_longdouble_fn((long double **)(x),(i),__FILE__,__LINE__)
static _INLINE void **
_zero_arr_inplace_plus_post_voidstar_fn(void ***x, int orig_i,const char *filename,unsigned lineno){
  void ** _zap_res = *x;
  *x = _zero_arr_plus_voidstar_fn(_zap_res,1,orig_i,filename,lineno);
  return _zap_res;
}
#define _zero_arr_inplace_plus_post_voidstar(x,i) \
  _zero_arr_inplace_plus_post_voidstar_fn((void***)(x),(i),__FILE__,__LINE__)

/* functions for dealing with dynamically sized pointers */
#ifdef NO_CYC_BOUNDS_CHECKS
#ifdef _INLINE_FUNCTIONS
static _INLINE unsigned char *
_check_dyneither_subscript(struct _dyneither_ptr arr,unsigned elt_sz,unsigned index) {
  struct _dyneither_ptr _cus_arr = (arr);
  unsigned _cus_elt_sz = (elt_sz);
  unsigned _cus_index = (index);
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index;
  return _cus_ans;
}
#else
#define _check_dyneither_subscript(arr,elt_sz,index) ({ \
  struct _dyneither_ptr _cus_arr = (arr); \
  unsigned _cus_elt_sz = (elt_sz); \
  unsigned _cus_index = (index); \
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index; \
  _cus_ans; })
#endif
#else
#ifdef _INLINE_FUNCTIONS
static _INLINE unsigned char *
_check_dyneither_subscript_fn(struct _dyneither_ptr arr,unsigned elt_sz,unsigned index,const char *filename, unsigned lineno) {
  struct _dyneither_ptr _cus_arr = (arr);
  unsigned _cus_elt_sz = (elt_sz);
  unsigned _cus_index = (index);
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index;
  /* JGM: not needed! if (!_cus_arr.base) _throw_null(); */ 
  if (_cus_ans < _cus_arr.base || _cus_ans >= _cus_arr.last_plus_one)
    _throw_arraybounds_fn(filename,lineno);
  return _cus_ans;
}
#define _check_dyneither_subscript(a,s,i) \
  _check_dyneither_subscript_fn(a,s,i,__FILE__,__LINE__)
#else
#define _check_dyneither_subscript(arr,elt_sz,index) ({ \
  struct _dyneither_ptr _cus_arr = (arr); \
  unsigned _cus_elt_sz = (elt_sz); \
  unsigned _cus_index = (index); \
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index; \
  /* JGM: not needed! if (!_cus_arr.base) _throw_null();*/ \
  if (_cus_ans < _cus_arr.base || _cus_ans >= _cus_arr.last_plus_one) \
    _throw_arraybounds(); \
  _cus_ans; })
#endif
#endif

#ifdef _INLINE_FUNCTIONS
static _INLINE struct _dyneither_ptr
_tag_dyneither(const void *tcurr,unsigned elt_sz,unsigned num_elts) {
  struct _dyneither_ptr _tag_arr_ans;
  _tag_arr_ans.base = _tag_arr_ans.curr = (void*)(tcurr);
  _tag_arr_ans.last_plus_one = _tag_arr_ans.base + (elt_sz) * (num_elts);
  return _tag_arr_ans;
}
#else
#define _tag_dyneither(tcurr,elt_sz,num_elts) ({ \
  struct _dyneither_ptr _tag_arr_ans; \
  _tag_arr_ans.base = _tag_arr_ans.curr = (void*)(tcurr); \
  _tag_arr_ans.last_plus_one = _tag_arr_ans.base + (elt_sz) * (num_elts); \
  _tag_arr_ans; })
#endif

#ifdef _INLINE_FUNCTIONS
static _INLINE struct _dyneither_ptr *
_init_dyneither_ptr(struct _dyneither_ptr *arr_ptr,
                    void *arr, unsigned elt_sz, unsigned num_elts) {
  struct _dyneither_ptr *_itarr_ptr = (arr_ptr);
  void* _itarr = (arr);
  _itarr_ptr->base = _itarr_ptr->curr = _itarr;
  _itarr_ptr->last_plus_one = ((unsigned char *)_itarr) + (elt_sz) * (num_elts);
  return _itarr_ptr;
}
#else
#define _init_dyneither_ptr(arr_ptr,arr,elt_sz,num_elts) ({ \
  struct _dyneither_ptr *_itarr_ptr = (arr_ptr); \
  void* _itarr = (arr); \
  _itarr_ptr->base = _itarr_ptr->curr = _itarr; \
  _itarr_ptr->last_plus_one = ((char *)_itarr) + (elt_sz) * (num_elts); \
  _itarr_ptr; })
#endif

#ifdef NO_CYC_BOUNDS_CHECKS
#define _untag_dyneither_ptr(arr,elt_sz,num_elts) ((arr).curr)
#else
#ifdef _INLINE_FUNCTIONS
static _INLINE unsigned char *
_untag_dyneither_ptr_fn(struct _dyneither_ptr arr, 
                        unsigned elt_sz,unsigned num_elts,
                        const char *filename, unsigned lineno) {
  struct _dyneither_ptr _arr = (arr);
  unsigned char *_curr = _arr.curr;
  if (_curr < _arr.base || _curr + (elt_sz) * (num_elts) > _arr.last_plus_one)
    _throw_arraybounds_fn(filename,lineno);
  return _curr;
}
#define _untag_dyneither_ptr(a,s,e) \
  _untag_dyneither_ptr_fn(a,s,e,__FILE__,__LINE__)
#else
#define _untag_dyneither_ptr(arr,elt_sz,num_elts) ({ \
  struct _dyneither_ptr _arr = (arr); \
  unsigned char *_curr = _arr.curr; \
  if (_curr < _arr.base || _curr + (elt_sz) * (num_elts) > _arr.last_plus_one)\
    _throw_arraybounds(); \
  _curr; })
#endif
#endif

#ifdef _INLINE_FUNCTIONS
static _INLINE unsigned
_get_dyneither_size(struct _dyneither_ptr arr,unsigned elt_sz) {
  struct _dyneither_ptr _get_arr_size_temp = (arr);
  unsigned char *_get_arr_size_curr=_get_arr_size_temp.curr;
  unsigned char *_get_arr_size_last=_get_arr_size_temp.last_plus_one;
  return (_get_arr_size_curr < _get_arr_size_temp.base ||
          _get_arr_size_curr >= _get_arr_size_last) ? 0 :
    ((_get_arr_size_last - _get_arr_size_curr) / (elt_sz));
}
#else
#define _get_dyneither_size(arr,elt_sz) \
  ({struct _dyneither_ptr _get_arr_size_temp = (arr); \
    unsigned char *_get_arr_size_curr=_get_arr_size_temp.curr; \
    unsigned char *_get_arr_size_last=_get_arr_size_temp.last_plus_one; \
    (_get_arr_size_curr < _get_arr_size_temp.base || \
     _get_arr_size_curr >= _get_arr_size_last) ? 0 : \
    ((_get_arr_size_last - _get_arr_size_curr) / (elt_sz));})
#endif

#ifdef _INLINE_FUNCTIONS
static _INLINE struct _dyneither_ptr
_dyneither_ptr_plus(struct _dyneither_ptr arr,unsigned elt_sz,int change) {
  struct _dyneither_ptr _ans = (arr);
  _ans.curr += ((int)(elt_sz))*(change);
  return _ans;
}
#else
#define _dyneither_ptr_plus(arr,elt_sz,change) ({ \
  struct _dyneither_ptr _ans = (arr); \
  _ans.curr += ((int)(elt_sz))*(change); \
  _ans; })
#endif

#ifdef _INLINE_FUNCTIONS
static _INLINE struct _dyneither_ptr
_dyneither_ptr_inplace_plus(struct _dyneither_ptr *arr_ptr,unsigned elt_sz,
                            int change) {
  struct _dyneither_ptr * _arr_ptr = (arr_ptr);
  _arr_ptr->curr += ((int)(elt_sz))*(change);
  return *_arr_ptr;
}
#else
#define _dyneither_ptr_inplace_plus(arr_ptr,elt_sz,change) ({ \
  struct _dyneither_ptr * _arr_ptr = (arr_ptr); \
  _arr_ptr->curr += ((int)(elt_sz))*(change); \
  *_arr_ptr; })
#endif

#ifdef _INLINE_FUNCTIONS
static _INLINE struct _dyneither_ptr
_dyneither_ptr_inplace_plus_post(struct _dyneither_ptr *arr_ptr,unsigned elt_sz,int change) {
  struct _dyneither_ptr * _arr_ptr = (arr_ptr);
  struct _dyneither_ptr _ans = *_arr_ptr;
  _arr_ptr->curr += ((int)(elt_sz))*(change);
  return _ans;
}
#else
#define _dyneither_ptr_inplace_plus_post(arr_ptr,elt_sz,change) ({ \
  struct _dyneither_ptr * _arr_ptr = (arr_ptr); \
  struct _dyneither_ptr _ans = *_arr_ptr; \
  _arr_ptr->curr += ((int)(elt_sz))*(change); \
  _ans; })
#endif

/* Decrease the upper bound on a fat pointer by numelts where sz is
   the size of the pointer's type.  Note that this can't be a macro
   if we're to get initializers right. */
static struct 
_dyneither_ptr _dyneither_ptr_decrease_size(struct _dyneither_ptr x,
                                            unsigned int sz,
                                            unsigned int numelts) {
  x.last_plus_one -= sz * numelts; 
  return x; 
}

/* Allocation */

extern void* GC_malloc(int);
extern void* GC_malloc_atomic(int);
extern void* GC_calloc(unsigned,unsigned);
extern void* GC_calloc_atomic(unsigned,unsigned);

#define _CYC_MAX_REGION_CONST 2
#define _CYC_MIN_ALIGNMENT (sizeof(double))

#ifdef CYC_REGION_PROFILE
extern int rgn_total_bytes;
#endif

static _INLINE void *_fast_region_malloc(struct _RegionHandle *r, unsigned orig_s) {  
  if (r > (struct _RegionHandle *)_CYC_MAX_REGION_CONST && r->curr != 0) { 
#ifdef CYC_NOALIGN
    unsigned s =  orig_s;
#else
    unsigned s =  (orig_s + _CYC_MIN_ALIGNMENT - 1) & (~(_CYC_MIN_ALIGNMENT -1)); 
#endif
    char *result; 
    result = r->offset; 
    if (s <= (r->last_plus_one - result)) {
      r->offset = result + s; 
#ifdef CYC_REGION_PROFILE
    r->curr->free_bytes = r->curr->free_bytes - s;
    rgn_total_bytes += s;
#endif
      return result;
    }
  } 
  return _region_malloc(r,orig_s); 
}

/* FIX?  Not sure if we want to pass filename and lineno in here... */
static _INLINE void* _cycalloc(int n) {
  void * ans = (void *)GC_malloc(n);
  if(!ans)
    _throw_badalloc();
  return ans;
}
static _INLINE void* _cycalloc_atomic(int n) {
  void * ans = (void *)GC_malloc_atomic(n);
  if(!ans)
    _throw_badalloc();
  return ans;
}
static _INLINE void* _cyccalloc(unsigned n, unsigned s) {
  void* ans = (void*)GC_calloc(n,s);
  if (!ans)
    _throw_badalloc();
  return ans;
}
static _INLINE void* _cyccalloc_atomic(unsigned n, unsigned s) {
  void* ans = (void*)GC_calloc_atomic(n,s);
  if (!ans)
    _throw_badalloc();
  return ans;
}
#define MAX_MALLOC_SIZE (1 << 28)
static _INLINE unsigned int _check_times(unsigned x, unsigned y) {
  _CYC_U_LONG_LONG_T whole_ans = 
    ((_CYC_U_LONG_LONG_T)x)*((_CYC_U_LONG_LONG_T)y);
  unsigned word_ans = (unsigned)whole_ans;
  if(word_ans < whole_ans || word_ans > MAX_MALLOC_SIZE)
    _throw_badalloc();
  return word_ans;
}

#if defined(CYC_REGION_PROFILE) 
extern void* _profile_GC_malloc(int,const char *file,const char *func,
                                int lineno);
extern void* _profile_GC_malloc_atomic(int,const char *file,
                                       const char *func,int lineno);
extern void* _profile_region_malloc(struct _RegionHandle *, unsigned,
                                    const char *file,
                                    const char *func,
                                    int lineno);
extern void* _profile_region_calloc(struct _RegionHandle *, unsigned,
                                    unsigned,
                                    const char *file,
                                    const char *func,
                                    int lineno);
extern struct _RegionHandle _profile_new_region(const char *rgn_name,
						const char *file,
						const char *func,
                                                int lineno);
extern void _profile_free_region(struct _RegionHandle *,
				 const char *file,
                                 const char *func,
                                 int lineno);
#  if !defined(RUNTIME_CYC)
#define _new_region(n) _profile_new_region(n,__FILE__,__FUNCTION__,__LINE__)
#define _free_region(r) _profile_free_region(r,__FILE__,__FUNCTION__,__LINE__)
#define _region_malloc(rh,n) _profile_region_malloc(rh,n,__FILE__,__FUNCTION__,__LINE__)
#define _region_calloc(rh,n,t) _profile_region_calloc(rh,n,t,__FILE__,__FUNCTION__,__LINE__)
#  endif
#define _cycalloc(n) _profile_GC_malloc(n,__FILE__,__FUNCTION__,__LINE__)
#define _cycalloc_atomic(n) _profile_GC_malloc_atomic(n,__FILE__,__FUNCTION__,__LINE__)
#endif
#endif

/* the next two routines swap [x] and [y]; not thread safe! */
static _INLINE void _swap_word(void *x, void *y) {
  unsigned long *lx = (unsigned long *)x, *ly = (unsigned long *)y, tmp;
  tmp = *lx;
  *lx = *ly;
  *ly = tmp;
}
static _INLINE void _swap_dyneither(struct _dyneither_ptr *x, 
				   struct _dyneither_ptr *y) {
  struct _dyneither_ptr tmp = *x;
  *x = *y;
  *y = tmp;
}

# 35 "core.h"
 typedef char*Cyc_Cstring;
typedef char*Cyc_CstringNN;
typedef struct _dyneither_ptr Cyc_string_t;
# 40
typedef struct _dyneither_ptr Cyc_mstring_t;
# 43
typedef struct _dyneither_ptr*Cyc_stringptr_t;
# 47
typedef struct _dyneither_ptr*Cyc_mstringptr_t;
# 50
typedef char*Cyc_Cbuffer_t;
# 52
typedef char*Cyc_CbufferNN_t;
# 54
typedef struct _dyneither_ptr Cyc_buffer_t;
# 56
typedef struct _dyneither_ptr Cyc_mbuffer_t;
# 59
typedef int Cyc_bool;
# 26 "cycboot.h"
typedef unsigned long Cyc_size_t;
# 33
typedef unsigned short Cyc_mode_t;struct Cyc___cycFILE;
# 49
typedef struct Cyc___cycFILE Cyc_FILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 68
typedef void*Cyc_parg_t;
# 73
struct _dyneither_ptr Cyc_aprintf(struct _dyneither_ptr,struct _dyneither_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned int*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _dyneither_ptr f1;};
# 127
typedef void*Cyc_sarg_t;
# 232 "cycboot.h"
struct _dyneither_ptr Cyc_vrprintf(struct _RegionHandle*,struct _dyneither_ptr,struct _dyneither_ptr);extern char Cyc_FileCloseError[15];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14];struct Cyc_FileOpenError_exn_struct{char*tag;struct _dyneither_ptr f1;};
# 79 "core.h"
typedef unsigned int Cyc_Core_sizeof_t;struct Cyc_Core_Opt{void*v;};
# 83
typedef struct Cyc_Core_Opt*Cyc_Core_opt_t;struct _tuple0{void*f1;void*f2;};
# 113 "core.h"
void*Cyc_Core_snd(struct _tuple0*);extern char Cyc_Core_Invalid_argument[17];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _dyneither_ptr f1;};extern char Cyc_Core_Failure[8];struct Cyc_Core_Failure_exn_struct{char*tag;struct _dyneither_ptr f1;};extern char Cyc_Core_Impossible[11];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _dyneither_ptr f1;};extern char Cyc_Core_Not_found[10];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _dyneither_ptr f1;};
# 167
extern struct _RegionHandle*Cyc_Core_heap_region;
# 170
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;
# 205
typedef struct Cyc_Core_DynamicRegion*Cyc_Core_region_key_t;
# 211
typedef struct Cyc_Core_DynamicRegion*Cyc_Core_uregion_key_t;
# 216
typedef struct Cyc_Core_DynamicRegion*Cyc_Core_rcregion_key_t;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};
# 299 "core.h"
typedef void*Cyc_Core___cyclone_internal_array_t;
typedef unsigned int Cyc_Core___cyclone_internal_singleton;
# 303
inline static void* arrcast(struct _dyneither_ptr dyn,unsigned int bd,unsigned int sz){
# 308
if(bd >> 20  || sz >> 12)
return 0;{
unsigned char*ptrbd=dyn.curr + bd * sz;
if(((ptrbd < dyn.curr  || dyn.curr == 0) || dyn.curr < dyn.base) || ptrbd > dyn.last_plus_one)
# 315
return 0;
return dyn.curr;};}struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 39 "list.h"
typedef struct Cyc_List_List*Cyc_List_list_t;
# 49 "list.h"
typedef struct Cyc_List_List*Cyc_List_List_t;
# 61
int Cyc_List_length(struct Cyc_List_List*x);
# 76
struct Cyc_List_List*Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x);
# 79
struct Cyc_List_List*Cyc_List_rmap(struct _RegionHandle*,void*(*f)(void*),struct Cyc_List_List*x);
# 86
struct Cyc_List_List*Cyc_List_rmap_c(struct _RegionHandle*,void*(*f)(void*,void*),void*env,struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 184
struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);
# 190
struct Cyc_List_List*Cyc_List_rappend(struct _RegionHandle*,struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4];struct Cyc_List_Nth_exn_struct{char*tag;};
# 242
void*Cyc_List_nth(struct Cyc_List_List*x,int n);
# 246
struct Cyc_List_List*Cyc_List_nth_tail(struct Cyc_List_List*x,int i);
# 265
void*Cyc_List_find_c(void*(*pred)(void*,void*),void*env,struct Cyc_List_List*x);
# 276
struct Cyc_List_List*Cyc_List_rzip(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y);struct Cyc_Lineno_Pos{struct _dyneither_ptr logical_file;struct _dyneither_ptr line;int line_no;int col;};
# 32 "lineno.h"
typedef struct Cyc_Lineno_Pos*Cyc_Lineno_pos_t;extern char Cyc_Position_Exit[5];struct Cyc_Position_Exit_exn_struct{char*tag;};
# 37 "position.h"
typedef unsigned int Cyc_Position_seg_t;struct Cyc_Position_Lex_Position_Error_kind_struct{int tag;};struct Cyc_Position_Parse_Position_Error_kind_struct{int tag;};struct Cyc_Position_Elab_Position_Error_kind_struct{int tag;};
# 46
typedef void*Cyc_Position_error_kind_t;struct Cyc_Position_Error{struct _dyneither_ptr source;unsigned int seg;void*kind;struct _dyneither_ptr desc;};
# 53
typedef struct Cyc_Position_Error*Cyc_Position_error_t;extern char Cyc_Position_Nocontext[10];struct Cyc_Position_Nocontext_exn_struct{char*tag;};struct Cyc_Relations_Reln;
# 73 "absyn.h"
typedef struct Cyc_Relations_Reln*Cyc_Relations_reln_t;
typedef struct Cyc_List_List*Cyc_Relations_relns_t;
# 79
typedef void*Cyc_Tcpat_decision_opt_t;
# 87
typedef struct _dyneither_ptr*Cyc_Absyn_field_name_t;
typedef struct _dyneither_ptr*Cyc_Absyn_var_t;
typedef struct _dyneither_ptr*Cyc_Absyn_tvarname_t;
typedef struct _dyneither_ptr*Cyc_Absyn_var_opt_t;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};
# 99
typedef union Cyc_Absyn_Nmspace Cyc_Absyn_nmspace_t;
union Cyc_Absyn_Nmspace Cyc_Absyn_Loc_n;
union Cyc_Absyn_Nmspace Cyc_Absyn_Rel_n(struct Cyc_List_List*);
# 103
union Cyc_Absyn_Nmspace Cyc_Absyn_Abs_n(struct Cyc_List_List*ns,int C_scope);struct _tuple1{union Cyc_Absyn_Nmspace f1;struct _dyneither_ptr*f2;};
# 106
typedef struct _tuple1*Cyc_Absyn_qvar_t;typedef struct _tuple1*Cyc_Absyn_qvar_opt_t;
typedef struct _tuple1*Cyc_Absyn_typedef_name_t;
typedef struct _tuple1*Cyc_Absyn_typedef_name_opt_t;
# 111
typedef enum Cyc_Absyn_Scope Cyc_Absyn_scope_t;
typedef struct Cyc_Absyn_Tqual Cyc_Absyn_tqual_t;
typedef enum Cyc_Absyn_Size_of Cyc_Absyn_size_of_t;
typedef struct Cyc_Absyn_Kind*Cyc_Absyn_kind_t;
typedef void*Cyc_Absyn_kindbound_t;
typedef struct Cyc_Absyn_Tvar*Cyc_Absyn_tvar_t;
typedef enum Cyc_Absyn_Sign Cyc_Absyn_sign_t;
typedef enum Cyc_Absyn_AggrKind Cyc_Absyn_aggr_kind_t;
typedef void*Cyc_Absyn_bounds_t;
typedef struct Cyc_Absyn_PtrAtts Cyc_Absyn_ptr_atts_t;
typedef struct Cyc_Absyn_PtrInfo Cyc_Absyn_ptr_info_t;
typedef struct Cyc_Absyn_VarargInfo Cyc_Absyn_vararg_info_t;
typedef struct Cyc_Absyn_FnInfo Cyc_Absyn_fn_info_t;
typedef struct Cyc_Absyn_DatatypeInfo Cyc_Absyn_datatype_info_t;
typedef struct Cyc_Absyn_DatatypeFieldInfo Cyc_Absyn_datatype_field_info_t;
typedef struct Cyc_Absyn_AggrInfo Cyc_Absyn_aggr_info_t;
typedef struct Cyc_Absyn_ArrayInfo Cyc_Absyn_array_info_t;
typedef void*Cyc_Absyn_type_t;typedef void*Cyc_Absyn_rgntype_t;typedef void*Cyc_Absyn_type_opt_t;
typedef union Cyc_Absyn_Cnst Cyc_Absyn_cnst_t;
typedef enum Cyc_Absyn_Primop Cyc_Absyn_primop_t;
typedef enum Cyc_Absyn_Incrementor Cyc_Absyn_incrementor_t;
typedef struct Cyc_Absyn_VarargCallInfo Cyc_Absyn_vararg_call_info_t;
typedef void*Cyc_Absyn_raw_exp_t;
typedef struct Cyc_Absyn_Exp*Cyc_Absyn_exp_t;typedef struct Cyc_Absyn_Exp*Cyc_Absyn_exp_opt_t;
typedef void*Cyc_Absyn_raw_stmt_t;
typedef struct Cyc_Absyn_Stmt*Cyc_Absyn_stmt_t;typedef struct Cyc_Absyn_Stmt*Cyc_Absyn_stmt_opt_t;
typedef void*Cyc_Absyn_raw_pat_t;
typedef struct Cyc_Absyn_Pat*Cyc_Absyn_pat_t;
typedef void*Cyc_Absyn_binding_t;
typedef struct Cyc_Absyn_Switch_clause*Cyc_Absyn_switch_clause_t;
typedef struct Cyc_Absyn_Fndecl*Cyc_Absyn_fndecl_t;
typedef struct Cyc_Absyn_Aggrdecl*Cyc_Absyn_aggrdecl_t;
typedef struct Cyc_Absyn_Datatypefield*Cyc_Absyn_datatypefield_t;
typedef struct Cyc_Absyn_Datatypedecl*Cyc_Absyn_datatypedecl_t;
typedef struct Cyc_Absyn_Typedefdecl*Cyc_Absyn_typedefdecl_t;
typedef struct Cyc_Absyn_Enumfield*Cyc_Absyn_enumfield_t;
typedef struct Cyc_Absyn_Enumdecl*Cyc_Absyn_enumdecl_t;
typedef struct Cyc_Absyn_Vardecl*Cyc_Absyn_vardecl_t;typedef struct Cyc_Absyn_Vardecl*Cyc_Absyn_vardecl_opt_t;
typedef void*Cyc_Absyn_raw_decl_t;
typedef struct Cyc_Absyn_Decl*Cyc_Absyn_decl_t;
typedef void*Cyc_Absyn_designator_t;
typedef void*Cyc_Absyn_absyn_annot_t;
typedef void*Cyc_Absyn_attribute_t;
typedef struct Cyc_List_List*Cyc_Absyn_attributes_t;
typedef struct Cyc_Absyn_Aggrfield*Cyc_Absyn_aggrfield_t;
typedef void*Cyc_Absyn_offsetof_field_t;
typedef struct Cyc_Absyn_MallocInfo Cyc_Absyn_malloc_info_t;
typedef enum Cyc_Absyn_Coercion Cyc_Absyn_coercion_t;
typedef struct Cyc_Absyn_PtrLoc*Cyc_Absyn_ptrloc_t;
# 162
enum Cyc_Absyn_Scope{Cyc_Absyn_Static  = 0,Cyc_Absyn_Abstract  = 1,Cyc_Absyn_Public  = 2,Cyc_Absyn_Extern  = 3,Cyc_Absyn_ExternC  = 4,Cyc_Absyn_Register  = 5};struct Cyc_Absyn_Tqual{int print_const;int q_volatile;int q_restrict;int real_const;unsigned int loc;};
# 183
enum Cyc_Absyn_Size_of{Cyc_Absyn_Char_sz  = 0,Cyc_Absyn_Short_sz  = 1,Cyc_Absyn_Int_sz  = 2,Cyc_Absyn_Long_sz  = 3,Cyc_Absyn_LongLong_sz  = 4};
# 188
enum Cyc_Absyn_AliasQual{Cyc_Absyn_Aliasable  = 0,Cyc_Absyn_Unique  = 1,Cyc_Absyn_Top  = 2};
# 195
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind  = 0,Cyc_Absyn_MemKind  = 1,Cyc_Absyn_BoxKind  = 2,Cyc_Absyn_RgnKind  = 3,Cyc_Absyn_EffKind  = 4,Cyc_Absyn_IntKind  = 5};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};
# 215
enum Cyc_Absyn_Sign{Cyc_Absyn_Signed  = 0,Cyc_Absyn_Unsigned  = 1,Cyc_Absyn_None  = 2};
# 217
enum Cyc_Absyn_AggrKind{Cyc_Absyn_StructA  = 0,Cyc_Absyn_UnionA  = 1};struct _union_Constraint_Eq_constr{int tag;void*val;};struct _union_Constraint_Forward_constr{int tag;union Cyc_Absyn_Constraint*val;};struct _union_Constraint_No_constr{int tag;int val;};union Cyc_Absyn_Constraint{struct _union_Constraint_Eq_constr Eq_constr;struct _union_Constraint_Forward_constr Forward_constr;struct _union_Constraint_No_constr No_constr;};
# 226
typedef union Cyc_Absyn_Constraint*Cyc_Absyn_conref_t;struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _dyneither_ptr*name;int identity;void*kind;};struct Cyc_Absyn_DynEither_b_Absyn_Bounds_struct{int tag;};struct Cyc_Absyn_Upper_b_Absyn_Bounds_struct{int tag;struct Cyc_Absyn_Exp*f1;};
# 252
extern struct Cyc_Absyn_DynEither_b_Absyn_Bounds_struct Cyc_Absyn_DynEither_b_val;struct Cyc_Absyn_PtrLoc{unsigned int ptr_loc;unsigned int rgn_loc;unsigned int zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;union Cyc_Absyn_Constraint*nullable;union Cyc_Absyn_Constraint*bounds;union Cyc_Absyn_Constraint*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_typ;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _dyneither_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_typ;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple1*name;int is_extensible;};struct _union_DatatypeInfoU_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfoU_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfoU{struct _union_DatatypeInfoU_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfoU_KnownDatatype KnownDatatype;};struct Cyc_Absyn_DatatypeInfo{union Cyc_Absyn_DatatypeInfoU datatype_info;struct Cyc_List_List*targs;};struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple1*datatype_name;struct _tuple1*field_name;int is_extensible;};struct _union_DatatypeFieldInfoU_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple2{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfoU_KnownDatatypefield{int tag;struct _tuple2 val;};union Cyc_Absyn_DatatypeFieldInfoU{struct _union_DatatypeFieldInfoU_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfoU_KnownDatatypefield KnownDatatypefield;};
# 339
union Cyc_Absyn_DatatypeFieldInfoU Cyc_Absyn_KnownDatatypefield(struct Cyc_Absyn_Datatypedecl*,struct Cyc_Absyn_Datatypefield*);struct Cyc_Absyn_DatatypeFieldInfo{union Cyc_Absyn_DatatypeFieldInfoU field_info;struct Cyc_List_List*targs;};struct _tuple3{enum Cyc_Absyn_AggrKind f1;struct _tuple1*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfoU_UnknownAggr{int tag;struct _tuple3 val;};struct _union_AggrInfoU_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfoU{struct _union_AggrInfoU_UnknownAggr UnknownAggr;struct _union_AggrInfoU_KnownAggr KnownAggr;};
# 352
union Cyc_Absyn_AggrInfoU Cyc_Absyn_KnownAggr(struct Cyc_Absyn_Aggrdecl**);struct Cyc_Absyn_AggrInfo{union Cyc_Absyn_AggrInfoU aggr_info;struct Cyc_List_List*targs;};struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;union Cyc_Absyn_Constraint*zero_term;unsigned int zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};
# 372
typedef void*Cyc_Absyn_raw_type_decl_t;struct Cyc_Absyn_TypeDecl{void*r;unsigned int loc;};
# 377
typedef struct Cyc_Absyn_TypeDecl*Cyc_Absyn_type_decl_t;struct Cyc_Absyn_VoidType_Absyn_Type_struct{int tag;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_DatatypeType_Absyn_Type_struct{int tag;struct Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldType_Absyn_Type_struct{int tag;struct Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_IntType_Absyn_Type_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatType_Absyn_Type_struct{int tag;int f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AggrType_Absyn_Type_struct{int tag;struct Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_EnumType_Absyn_Type_struct{int tag;struct _tuple1*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_RgnHandleType_Absyn_Type_struct{int tag;void*f1;};struct Cyc_Absyn_DynRgnType_Absyn_Type_struct{int tag;void*f1;void*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TagType_Absyn_Type_struct{int tag;void*f1;};struct Cyc_Absyn_HeapRgn_Absyn_Type_struct{int tag;};struct Cyc_Absyn_UniqueRgn_Absyn_Type_struct{int tag;};struct Cyc_Absyn_RefCntRgn_Absyn_Type_struct{int tag;};struct Cyc_Absyn_AccessEff_Absyn_Type_struct{int tag;void*f1;};struct Cyc_Absyn_JoinEff_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_RgnsEff_Absyn_Type_struct{int tag;void*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_BuiltinType_Absyn_Type_struct{int tag;struct _dyneither_ptr f1;struct Cyc_Absyn_Kind*f2;};
# 432 "absyn.h"
extern struct Cyc_Absyn_HeapRgn_Absyn_Type_struct Cyc_Absyn_HeapRgn_val;
extern struct Cyc_Absyn_UniqueRgn_Absyn_Type_struct Cyc_Absyn_UniqueRgn_val;
# 435
extern struct Cyc_Absyn_VoidType_Absyn_Type_struct Cyc_Absyn_VoidType_val;struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned int f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_Absyn_Exp*f6;struct Cyc_Absyn_Exp*f7;};
# 448
typedef void*Cyc_Absyn_funcparams_t;
# 451
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft  = 0,Cyc_Absyn_Scanf_ft  = 1};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noconsume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;union Cyc_Absyn_Constraint*f1;unsigned int f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;union Cyc_Absyn_Constraint*f2;unsigned int f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned int f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned int f1;struct Cyc_List_List*f2;};
# 515
typedef void*Cyc_Absyn_type_modifier_t;struct _union_Cnst_Null_c{int tag;int val;};struct _tuple4{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple4 val;};struct _union_Cnst_Wchar_c{int tag;struct _dyneither_ptr val;};struct _tuple5{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple6 val;};struct _tuple7{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple7 val;};struct _tuple8{struct _dyneither_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple8 val;};struct _union_Cnst_String_c{int tag;struct _dyneither_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _dyneither_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 531
union Cyc_Absyn_Cnst Cyc_Absyn_Char_c(enum Cyc_Absyn_Sign,char);
# 533
union Cyc_Absyn_Cnst Cyc_Absyn_Short_c(enum Cyc_Absyn_Sign,short);
union Cyc_Absyn_Cnst Cyc_Absyn_Int_c(enum Cyc_Absyn_Sign,int);
# 541
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus  = 0,Cyc_Absyn_Times  = 1,Cyc_Absyn_Minus  = 2,Cyc_Absyn_Div  = 3,Cyc_Absyn_Mod  = 4,Cyc_Absyn_Eq  = 5,Cyc_Absyn_Neq  = 6,Cyc_Absyn_Gt  = 7,Cyc_Absyn_Lt  = 8,Cyc_Absyn_Gte  = 9,Cyc_Absyn_Lte  = 10,Cyc_Absyn_Not  = 11,Cyc_Absyn_Bitnot  = 12,Cyc_Absyn_Bitand  = 13,Cyc_Absyn_Bitor  = 14,Cyc_Absyn_Bitxor  = 15,Cyc_Absyn_Bitlshift  = 16,Cyc_Absyn_Bitlrshift  = 17,Cyc_Absyn_Bitarshift  = 18,Cyc_Absyn_Numelts  = 19};
# 548
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc  = 0,Cyc_Absyn_PostInc  = 1,Cyc_Absyn_PreDec  = 2,Cyc_Absyn_PostDec  = 3};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _dyneither_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned int f1;};
# 566
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion  = 0,Cyc_Absyn_No_coercion  = 1,Cyc_Absyn_NonNull_to_Null  = 2,Cyc_Absyn_Other_coercion  = 3};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftyp_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _dyneither_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _dyneither_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple9{struct _dyneither_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple9*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct _tuple1*f1;struct Cyc_Absyn_Enumdecl*f2;struct Cyc_Absyn_Enumfield*f3;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;struct _tuple1*f1;void*f2;struct Cyc_Absyn_Enumfield*f3;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _dyneither_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _dyneither_ptr f2;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned int loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple10{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple10 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _dyneither_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple10 f2;struct _tuple10 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _dyneither_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple10 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_ResetRegion_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Stmt{void*r;unsigned int loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _dyneither_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple1*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned int loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned int loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple1*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple1*name;struct Cyc_List_List*tvs;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_Absyn_Stmt*body;void*cached_typ;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;};struct Cyc_Absyn_Aggrfield{struct _dyneither_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple1*name;struct Cyc_List_List*typs;unsigned int loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple1*name;struct Cyc_Absyn_Exp*tag;unsigned int loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple1*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;int f3;struct Cyc_Absyn_Exp*f4;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _dyneither_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned int loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _dyneither_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 930 "absyn.h"
struct Cyc_Absyn_Tqual Cyc_Absyn_const_tqual(unsigned int);
# 932
struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(unsigned int);
# 936
union Cyc_Absyn_Constraint*Cyc_Absyn_empty_conref();
# 939
void*Cyc_Absyn_conref_val(union Cyc_Absyn_Constraint*x);
# 942
void*Cyc_Absyn_conref_def(void*y,union Cyc_Absyn_Constraint*x);
# 944
void*Cyc_Absyn_conref_constr(void*y,union Cyc_Absyn_Constraint*x);
extern union Cyc_Absyn_Constraint*Cyc_Absyn_true_conref;
extern union Cyc_Absyn_Constraint*Cyc_Absyn_false_conref;
extern union Cyc_Absyn_Constraint*Cyc_Absyn_bounds_one_conref;
extern union Cyc_Absyn_Constraint*Cyc_Absyn_bounds_dyneither_conref;
# 955
void*Cyc_Absyn_new_evar(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv);
# 957
void*Cyc_Absyn_wildtyp(struct Cyc_Core_Opt*);
# 960
extern void*Cyc_Absyn_char_typ;extern void*Cyc_Absyn_uchar_typ;extern void*Cyc_Absyn_ushort_typ;extern void*Cyc_Absyn_uint_typ;extern void*Cyc_Absyn_ulonglong_typ;
# 962
extern void*Cyc_Absyn_schar_typ;extern void*Cyc_Absyn_sshort_typ;extern void*Cyc_Absyn_sint_typ;extern void*Cyc_Absyn_slonglong_typ;
# 964
void*Cyc_Absyn_float_typ(int);void*Cyc_Absyn_wchar_typ();
# 974
void*Cyc_Absyn_exn_typ();
# 985
extern void*Cyc_Absyn_bounds_one;
# 990
void*Cyc_Absyn_atb_typ(void*t,void*rgn,struct Cyc_Absyn_Tqual tq,void*b,union Cyc_Absyn_Constraint*zero_term);
# 993
void*Cyc_Absyn_star_typ(void*t,void*rgn,struct Cyc_Absyn_Tqual tq,union Cyc_Absyn_Constraint*zero_term);
# 996
void*Cyc_Absyn_at_typ(void*t,void*rgn,struct Cyc_Absyn_Tqual tq,union Cyc_Absyn_Constraint*zero_term);
# 1001
void*Cyc_Absyn_dyneither_typ(void*t,void*rgn,struct Cyc_Absyn_Tqual tq,union Cyc_Absyn_Constraint*zero_term);
# 1012
void*Cyc_Absyn_array_typ(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,union Cyc_Absyn_Constraint*zero_term,unsigned int ztloc);
# 1016
struct Cyc_Absyn_Exp*Cyc_Absyn_new_exp(void*,unsigned int);
# 1018
struct Cyc_Absyn_Exp*Cyc_Absyn_copy_exp(struct Cyc_Absyn_Exp*);
struct Cyc_Absyn_Exp*Cyc_Absyn_const_exp(union Cyc_Absyn_Cnst,unsigned int);
# 1026
struct Cyc_Absyn_Exp*Cyc_Absyn_uint_exp(unsigned int,unsigned int);
# 1039
struct Cyc_Absyn_Exp*Cyc_Absyn_add_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned int);
# 1065
struct Cyc_Absyn_Exp*Cyc_Absyn_cast_exp(void*,struct Cyc_Absyn_Exp*,int user_cast,enum Cyc_Absyn_Coercion,unsigned int);
# 1067
struct Cyc_Absyn_Exp*Cyc_Absyn_sizeoftyp_exp(void*t,unsigned int);
# 1075
struct Cyc_Absyn_Exp*Cyc_Absyn_stmt_exp(struct Cyc_Absyn_Stmt*,unsigned int);
# 1079
struct Cyc_Absyn_Exp*Cyc_Absyn_valueof_exp(void*,unsigned int);
# 1085
extern struct Cyc_Absyn_Exp*Cyc_Absyn_uniquergn_exp;
# 1089
struct Cyc_Absyn_Stmt*Cyc_Absyn_exp_stmt(struct Cyc_Absyn_Exp*e,unsigned int loc);
# 1100
struct Cyc_Absyn_Stmt*Cyc_Absyn_decl_stmt(struct Cyc_Absyn_Decl*d,struct Cyc_Absyn_Stmt*s,unsigned int loc);
# 1161
void*Cyc_Absyn_pointer_expand(void*,int fresh_evar);
# 1163
int Cyc_Absyn_is_lvalue(struct Cyc_Absyn_Exp*);
# 1166
struct Cyc_Absyn_Aggrfield*Cyc_Absyn_lookup_field(struct Cyc_List_List*,struct _dyneither_ptr*);
# 1168
struct Cyc_Absyn_Aggrfield*Cyc_Absyn_lookup_decl_field(struct Cyc_Absyn_Aggrdecl*,struct _dyneither_ptr*);
# 1190
struct _tuple1*Cyc_Absyn_binding2qvar(void*);
# 38 "string.h"
unsigned long Cyc_strlen(struct _dyneither_ptr s);
# 49 "string.h"
int Cyc_strcmp(struct _dyneither_ptr s1,struct _dyneither_ptr s2);struct Cyc_PP_Ppstate;
# 41 "pp.h"
typedef struct Cyc_PP_Ppstate*Cyc_PP_ppstate_t;struct Cyc_PP_Out;
# 43
typedef struct Cyc_PP_Out*Cyc_PP_out_t;struct Cyc_PP_Doc;
# 45
typedef struct Cyc_PP_Doc*Cyc_PP_doc_t;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 65 "absynpp.h"
struct _dyneither_ptr Cyc_Absynpp_typ2string(void*);
# 70
struct _dyneither_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*);
# 72
struct _dyneither_ptr Cyc_Absynpp_qvar2string(struct _tuple1*);struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};
# 34 "iter.h"
typedef struct Cyc_Iter_Iter Cyc_Iter_iter_t;
# 37
int Cyc_Iter_next(struct Cyc_Iter_Iter,void*);struct Cyc_Set_Set;
# 40 "set.h"
typedef struct Cyc_Set_Set*Cyc_Set_set_t;extern char Cyc_Set_Absent[7];struct Cyc_Set_Absent_exn_struct{char*tag;};struct Cyc_Dict_T;
# 46 "dict.h"
typedef const struct Cyc_Dict_T*Cyc_Dict_tree;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};
# 52
typedef struct Cyc_Dict_Dict Cyc_Dict_dict_t;extern char Cyc_Dict_Present[8];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_RgnOrder_RgnPO;
# 33 "rgnorder.h"
typedef struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_rgn_po_t;
# 35
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_initial_fn_po(struct _RegionHandle*,struct Cyc_List_List*tvs,struct Cyc_List_List*po,void*effect,struct Cyc_Absyn_Tvar*fst_rgn,unsigned int);
# 42
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_add_outlives_constraint(struct _RegionHandle*,struct Cyc_RgnOrder_RgnPO*po,void*eff,void*rgn,unsigned int loc);
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_add_youngest(struct _RegionHandle*,struct Cyc_RgnOrder_RgnPO*po,struct Cyc_Absyn_Tvar*rgn,int resetable,int opened);
int Cyc_RgnOrder_is_region_resetable(struct Cyc_RgnOrder_RgnPO*po,struct Cyc_Absyn_Tvar*r);
int Cyc_RgnOrder_effect_outlives(struct Cyc_RgnOrder_RgnPO*po,void*eff,void*rgn);
int Cyc_RgnOrder_satisfies_constraints(struct Cyc_RgnOrder_RgnPO*po,struct Cyc_List_List*constraints,void*default_bound,int do_pin);
# 48
int Cyc_RgnOrder_eff_outlives_eff(struct Cyc_RgnOrder_RgnPO*po,void*eff1,void*eff2);
# 51
void Cyc_RgnOrder_print_region_po(struct Cyc_RgnOrder_RgnPO*po);extern char Cyc_Tcenv_Env_error[10];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_CList{void*hd;const struct Cyc_Tcenv_CList*tl;};
# 43 "tcenv.h"
typedef const struct Cyc_Tcenv_CList*Cyc_Tcenv_mclist_t;
typedef const struct Cyc_Tcenv_CList*const Cyc_Tcenv_clist_t;struct Cyc_Tcenv_VarRes_Tcenv_Resolved_struct{int tag;void*f1;};struct Cyc_Tcenv_AggrRes_Tcenv_Resolved_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Tcenv_DatatypeRes_Tcenv_Resolved_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct Cyc_Tcenv_EnumRes_Tcenv_Resolved_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Tcenv_AnonEnumRes_Tcenv_Resolved_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};
# 54
typedef void*Cyc_Tcenv_resolved_t;struct Cyc_Tcenv_Genv{struct _RegionHandle*grgn;struct Cyc_Set_Set*namespaces;struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;struct Cyc_List_List*availables;};
# 73
typedef struct Cyc_Tcenv_Genv*Cyc_Tcenv_genv_t;struct Cyc_Tcenv_Fenv;
# 77
typedef struct Cyc_Tcenv_Fenv*Cyc_Tcenv_fenv_t;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Dict_Dict ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof;int in_extern_c_include;};
# 88
typedef struct Cyc_Tcenv_Tenv*Cyc_Tcenv_tenv_t;
# 92
struct _RegionHandle*Cyc_Tcenv_get_fnrgn(struct Cyc_Tcenv_Tenv*);
# 114 "tcenv.h"
void*Cyc_Tcenv_lookup_ordinary(struct _RegionHandle*,struct Cyc_Tcenv_Tenv*,unsigned int,struct _tuple1*,int is_use);
struct Cyc_Absyn_Aggrdecl**Cyc_Tcenv_lookup_aggrdecl(struct Cyc_Tcenv_Tenv*,unsigned int,struct _tuple1*);
struct Cyc_Absyn_Datatypedecl**Cyc_Tcenv_lookup_datatypedecl(struct Cyc_Tcenv_Tenv*,unsigned int,struct _tuple1*);
# 121
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_allow_valueof(struct _RegionHandle*,struct Cyc_Tcenv_Tenv*);
# 124
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew  = 0,Cyc_Tcenv_InNew  = 1,Cyc_Tcenv_InNewAggr  = 2};
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_new_status(struct _RegionHandle*,enum Cyc_Tcenv_NewStatus status,struct Cyc_Tcenv_Tenv*);
# 127
enum Cyc_Tcenv_NewStatus Cyc_Tcenv_new_status(struct Cyc_Tcenv_Tenv*te);
# 129
int Cyc_Tcenv_abstract_val_ok(struct Cyc_Tcenv_Tenv*te);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_abstract_val_ok(struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te);
# 132
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_abstract_val_ok(struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te);
# 137
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_copy_tenv(struct _RegionHandle*,struct Cyc_Tcenv_Tenv*);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_local_var(struct _RegionHandle*,unsigned int,struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Vardecl*);
# 141
struct Cyc_List_List*Cyc_Tcenv_lookup_type_vars(struct Cyc_Tcenv_Tenv*);
struct Cyc_Core_Opt*Cyc_Tcenv_lookup_opt_type_vars(struct Cyc_Tcenv_Tenv*te);
# 158
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_notreadctxt(struct _RegionHandle*,struct Cyc_Tcenv_Tenv*te);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_notreadctxt(struct _RegionHandle*,struct Cyc_Tcenv_Tenv*te);
int Cyc_Tcenv_in_notreadctxt(struct Cyc_Tcenv_Tenv*te);
# 163
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_lhs(struct _RegionHandle*,struct Cyc_Tcenv_Tenv*te);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_lhs(struct _RegionHandle*,struct Cyc_Tcenv_Tenv*te);
# 171
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_block(struct _RegionHandle*,unsigned int,struct Cyc_Tcenv_Tenv*);
# 179
void*Cyc_Tcenv_curr_rgn(struct Cyc_Tcenv_Tenv*);
# 183
void Cyc_Tcenv_check_rgn_accessible(struct Cyc_Tcenv_Tenv*,unsigned int,void*rgn);
# 187
void Cyc_Tcenv_check_effect_accessible(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void*eff);
# 194
void Cyc_Tcenv_check_rgn_partial_order(struct Cyc_Tcenv_Tenv*te,unsigned int loc,struct Cyc_List_List*po);
# 38 "tcutil.h"
void*Cyc_Tcutil_impos(struct _dyneither_ptr fmt,struct _dyneither_ptr ap);
# 40
void Cyc_Tcutil_terr(unsigned int,struct _dyneither_ptr fmt,struct _dyneither_ptr ap);
# 42
void Cyc_Tcutil_warn(unsigned int,struct _dyneither_ptr fmt,struct _dyneither_ptr ap);extern char Cyc_Tcutil_AbortTypeCheckingFunction[26];struct Cyc_Tcutil_AbortTypeCheckingFunction_exn_struct{char*tag;};
# 58
void*Cyc_Tcutil_copy_type(void*t);
# 61
struct Cyc_Absyn_Exp*Cyc_Tcutil_deep_copy_exp(int preserve_types,struct Cyc_Absyn_Exp*);
# 64
int Cyc_Tcutil_kind_leq(struct Cyc_Absyn_Kind*k1,struct Cyc_Absyn_Kind*k2);
# 68
struct Cyc_Absyn_Kind*Cyc_Tcutil_tvar_kind(struct Cyc_Absyn_Tvar*t,struct Cyc_Absyn_Kind*def);
struct Cyc_Absyn_Kind*Cyc_Tcutil_typ_kind(void*t);
# 71
void*Cyc_Tcutil_compress(void*t);
void Cyc_Tcutil_unchecked_cast(struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Exp*,void*,enum Cyc_Absyn_Coercion);
int Cyc_Tcutil_coerce_arg(struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Exp*,void*,int*alias_coercion);
int Cyc_Tcutil_coerce_assign(struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Exp*,void*);
int Cyc_Tcutil_coerce_to_bool(struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Exp*);
int Cyc_Tcutil_coerce_list(struct Cyc_Tcenv_Tenv*,void*,struct Cyc_List_List*);
int Cyc_Tcutil_coerce_uint_typ(struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Exp*);
int Cyc_Tcutil_coerce_sint_typ(struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Exp*);
# 80
int Cyc_Tcutil_coerceable(void*);
# 83
int Cyc_Tcutil_silent_castable(struct Cyc_Tcenv_Tenv*,unsigned int,void*,void*);
# 85
enum Cyc_Absyn_Coercion Cyc_Tcutil_castable(struct Cyc_Tcenv_Tenv*,unsigned int,void*,void*);struct _tuple11{struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Exp*f2;};
# 91
struct _tuple11 Cyc_Tcutil_insert_alias(struct Cyc_Absyn_Exp*e,void*e_typ);
# 99
int Cyc_Tcutil_is_integral(struct Cyc_Absyn_Exp*);
int Cyc_Tcutil_is_numeric(struct Cyc_Absyn_Exp*);
# 102
int Cyc_Tcutil_is_pointer_type(void*t);
int Cyc_Tcutil_is_zero(struct Cyc_Absyn_Exp*e);
int Cyc_Tcutil_is_boxed(void*t);
# 106
void*Cyc_Tcutil_pointer_elt_type(void*t);
void*Cyc_Tcutil_pointer_region(void*t);
# 111
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ak;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_bk;
# 118
extern struct Cyc_Absyn_Kind Cyc_Tcutil_tak;
# 120
extern struct Cyc_Absyn_Kind Cyc_Tcutil_tmk;
# 127
extern struct Cyc_Core_Opt Cyc_Tcutil_rko;
extern struct Cyc_Core_Opt Cyc_Tcutil_ako;
extern struct Cyc_Core_Opt Cyc_Tcutil_bko;
# 134
extern struct Cyc_Core_Opt Cyc_Tcutil_trko;
extern struct Cyc_Core_Opt Cyc_Tcutil_tako;
# 137
extern struct Cyc_Core_Opt Cyc_Tcutil_tmko;
# 153
int Cyc_Tcutil_zero_to_null(struct Cyc_Tcenv_Tenv*,void*t,struct Cyc_Absyn_Exp*e);
# 155
void*Cyc_Tcutil_max_arithmetic_type(void*,void*);
# 159
void Cyc_Tcutil_explain_failure();
# 161
int Cyc_Tcutil_unify(void*,void*);
# 166
void*Cyc_Tcutil_rsubstitute(struct _RegionHandle*,struct Cyc_List_List*,void*);
struct Cyc_List_List*Cyc_Tcutil_rsubst_rgnpo(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_List_List*);
# 188
void*Cyc_Tcutil_fndecl2typ(struct Cyc_Absyn_Fndecl*);struct _tuple12{struct Cyc_List_List*f1;struct _RegionHandle*f2;};struct _tuple13{struct Cyc_Absyn_Tvar*f1;void*f2;};
# 193
struct _tuple13*Cyc_Tcutil_r_make_inst_var(struct _tuple12*,struct Cyc_Absyn_Tvar*);
# 235 "tcutil.h"
void Cyc_Tcutil_check_type(unsigned int,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*k,int allow_evars,int allow_abs_aggr,void*);
# 245
void Cyc_Tcutil_check_nonzero_bound(unsigned int,union Cyc_Absyn_Constraint*);
# 247
void Cyc_Tcutil_check_bound(unsigned int,unsigned int i,union Cyc_Absyn_Constraint*);
# 249
int Cyc_Tcutil_is_bound_one(union Cyc_Absyn_Constraint*b);
# 253
struct Cyc_List_List*Cyc_Tcutil_resolve_aggregate_designators(struct _RegionHandle*rgn,unsigned int loc,struct Cyc_List_List*des,enum Cyc_Absyn_AggrKind,struct Cyc_List_List*fields);
# 259
int Cyc_Tcutil_is_tagged_pointer_typ(void*);
# 261
int Cyc_Tcutil_is_tagged_pointer_typ_elt(void*t,void**elt_typ_dest);
# 263
int Cyc_Tcutil_is_zero_pointer_typ_elt(void*t,void**elt_typ_dest);
# 273
int Cyc_Tcutil_is_zero_ptr_deref(struct Cyc_Absyn_Exp*e1,void**ptr_type,int*is_dyneither,void**elt_type);
# 278
int Cyc_Tcutil_is_noalias_region(void*r,int must_be_unique);
# 281
int Cyc_Tcutil_is_noalias_pointer(void*t,int must_be_unique);
# 286
int Cyc_Tcutil_is_noalias_path(struct _RegionHandle*,struct Cyc_Absyn_Exp*e);
# 291
int Cyc_Tcutil_is_noalias_pointer_or_aggr(struct _RegionHandle*,void*t);struct _tuple14{int f1;void*f2;};
# 295
struct _tuple14 Cyc_Tcutil_addressof_props(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e);
# 322
int Cyc_Tcutil_bits_only(void*t);
# 325
int Cyc_Tcutil_is_const_exp(struct Cyc_Absyn_Exp*e);
# 354
int Cyc_Tcutil_rgn_of_pointer(void*t,void**rgn);
# 357
void Cyc_Tcutil_check_no_qual(unsigned int loc,void*t);
# 370
int Cyc_Tcutil_is_array(void*t);
# 374
void*Cyc_Tcutil_promote_array(void*t,void*rgn,int convert_tag);
# 377
int Cyc_Tcutil_zeroable_type(void*t);struct _tuple15{unsigned int f1;int f2;};
# 28 "evexp.h"
struct _tuple15 Cyc_Evexp_eval_const_uint_exp(struct Cyc_Absyn_Exp*e);
# 32
int Cyc_Evexp_c_can_eval(struct Cyc_Absyn_Exp*e);
# 41 "evexp.h"
int Cyc_Evexp_same_const_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2);
# 48
int Cyc_Evexp_okay_szofarg(void*t);
# 27 "tcstmt.h"
void Cyc_Tcstmt_tcStmt(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Stmt*s,int new_block);
# 34 "formatstr.h"
struct Cyc_List_List*Cyc_Formatstr_get_format_typs(struct _RegionHandle*,struct Cyc_Tcenv_Tenv*,struct _dyneither_ptr,unsigned int);
# 37
struct Cyc_List_List*Cyc_Formatstr_get_scanf_typs(struct _RegionHandle*,struct Cyc_Tcenv_Tenv*,struct _dyneither_ptr,unsigned int);
# 32 "tcexp.h"
void*Cyc_Tcexp_tcExp(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*);
void*Cyc_Tcexp_tcExpInitializer(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*);struct Cyc_Tcexp_TestEnv{struct _tuple0*eq;int isTrue;};
# 39
typedef struct Cyc_Tcexp_TestEnv Cyc_Tcexp_testenv_t;
struct Cyc_Tcexp_TestEnv Cyc_Tcexp_tcTest(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e,struct _dyneither_ptr msg_part);
# 42
extern int Cyc_Tcexp_in_stmt_exp;struct Cyc_Hashtable_Table;
# 35 "hashtable.h"
typedef struct Cyc_Hashtable_Table*Cyc_Hashtable_table_t;
# 30 "jump_analysis.h"
typedef struct Cyc_Hashtable_Table*Cyc_JumpAnalysis_table_t;struct Cyc_JumpAnalysis_Jump_Anal_Result{struct Cyc_Hashtable_Table*pop_tables;struct Cyc_Hashtable_Table*succ_tables;struct Cyc_Hashtable_Table*pat_pop_tables;};
# 45 "jump_analysis.h"
typedef struct Cyc_JumpAnalysis_Jump_Anal_Result*Cyc_JumpAnalysis_jump_anal_res_t;
# 47
struct Cyc_JumpAnalysis_Jump_Anal_Result*Cyc_JumpAnalysis_jump_analysis(struct Cyc_List_List*tds);struct _union_RelnOp_RConst{int tag;unsigned int val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned int val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned int val;};struct _union_RelnOp_RReturn{int tag;unsigned int val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 40 "relations.h"
typedef union Cyc_Relations_RelnOp Cyc_Relations_reln_op_t;
# 51
enum Cyc_Relations_Relation{Cyc_Relations_Req  = 0,Cyc_Relations_Rneq  = 1,Cyc_Relations_Rlte  = 2,Cyc_Relations_Rlt  = 3};
typedef enum Cyc_Relations_Relation Cyc_Relations_relation_t;struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};
# 43 "cf_flowinfo.h"
int Cyc_CfFlowInfo_anal_error;
void Cyc_CfFlowInfo_aerr(unsigned int loc,struct _dyneither_ptr fmt,struct _dyneither_ptr ap);struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;};struct Cyc_CfFlowInfo_InitParam_CfFlowInfo_Root_struct{int tag;int f1;void*f2;};
# 54
typedef void*Cyc_CfFlowInfo_root_t;struct Cyc_CfFlowInfo_Place{void*root;struct Cyc_List_List*fields;};
# 65
typedef struct Cyc_CfFlowInfo_Place*Cyc_CfFlowInfo_place_t;
# 67
enum Cyc_CfFlowInfo_InitLevel{Cyc_CfFlowInfo_NoneIL  = 0,Cyc_CfFlowInfo_ThisIL  = 1,Cyc_CfFlowInfo_AllIL  = 2};
# 72
typedef enum Cyc_CfFlowInfo_InitLevel Cyc_CfFlowInfo_initlevel_t;extern char Cyc_CfFlowInfo_IsZero[7];struct Cyc_CfFlowInfo_IsZero_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_CfFlowInfo_NotZero[8];struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_List_List*f1;};extern char Cyc_CfFlowInfo_UnknownZ[9];struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_List_List*f1;};struct _union_AbsLVal_PlaceL{int tag;struct Cyc_CfFlowInfo_Place*val;};struct _union_AbsLVal_UnknownL{int tag;int val;};union Cyc_CfFlowInfo_AbsLVal{struct _union_AbsLVal_PlaceL PlaceL;struct _union_AbsLVal_UnknownL UnknownL;};
# 87
typedef union Cyc_CfFlowInfo_AbsLVal Cyc_CfFlowInfo_absLval_t;
# 92
typedef void*Cyc_CfFlowInfo_absRval_t;
typedef void*Cyc_CfFlowInfo_absRval_opt_t;
typedef struct Cyc_Dict_Dict Cyc_CfFlowInfo_flowdict_t;
typedef struct _dyneither_ptr Cyc_CfFlowInfo_aggrdict_t;struct Cyc_CfFlowInfo_UnionRInfo{int is_union;int fieldnum;};
# 100
typedef struct Cyc_CfFlowInfo_UnionRInfo Cyc_CfFlowInfo_union_rinfo_t;struct Cyc_CfFlowInfo_Zero_CfFlowInfo_AbsRVal_struct{int tag;};struct Cyc_CfFlowInfo_NotZeroAll_CfFlowInfo_AbsRVal_struct{int tag;};struct Cyc_CfFlowInfo_NotZeroThis_CfFlowInfo_AbsRVal_struct{int tag;};struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct{int tag;enum Cyc_CfFlowInfo_InitLevel f1;};struct Cyc_CfFlowInfo_Esc_CfFlowInfo_AbsRVal_struct{int tag;enum Cyc_CfFlowInfo_InitLevel f1;};struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_CfFlowInfo_Place*f1;};struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_CfFlowInfo_UnionRInfo f1;struct _dyneither_ptr f2;};struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;void*f3;};struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_Absyn_Vardecl*f1;void*f2;};
# 121
typedef struct Cyc_Dict_Dict Cyc_CfFlowInfo_dict_set_t;
# 128
typedef struct Cyc_Dict_Dict Cyc_CfFlowInfo_place_set_t;struct _union_FlowInfo_BottomFL{int tag;int val;};struct _tuple16{struct Cyc_Dict_Dict f1;struct Cyc_List_List*f2;};struct _union_FlowInfo_ReachableFL{int tag;struct _tuple16 val;};union Cyc_CfFlowInfo_FlowInfo{struct _union_FlowInfo_BottomFL BottomFL;struct _union_FlowInfo_ReachableFL ReachableFL;};
# 144 "cf_flowinfo.h"
typedef union Cyc_CfFlowInfo_FlowInfo Cyc_CfFlowInfo_flow_t;struct Cyc_CfFlowInfo_FlowEnv{struct _RegionHandle*r;void*zero;void*notzeroall;void*notzerothis;void*unknown_none;void*unknown_this;void*unknown_all;void*esc_none;void*esc_this;void*esc_all;struct Cyc_Dict_Dict mt_flowdict;struct Cyc_Dict_Dict mt_place_set;struct Cyc_CfFlowInfo_Place*dummy_place;};
# 163
typedef struct Cyc_CfFlowInfo_FlowEnv*Cyc_CfFlowInfo_flow_env_t;struct Cyc_CfFlowInfo_UniqueRgn_k_CfFlowInfo_KillRgn_struct{int tag;};struct Cyc_CfFlowInfo_RefCntRgn_k_CfFlowInfo_KillRgn_struct{int tag;};struct Cyc_CfFlowInfo_Region_k_CfFlowInfo_KillRgn_struct{int tag;struct Cyc_Absyn_Tvar*f1;};
# 235 "cf_flowinfo.h"
typedef void*Cyc_CfFlowInfo_killrgn_t;
# 52 "tcexp.cyc"
static void*Cyc_Tcexp_expr_err(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct _dyneither_ptr msg,struct _dyneither_ptr ap){
# 56
({void*_tmp0=0;Cyc_Tcutil_terr(loc,(struct _dyneither_ptr)Cyc_vrprintf(Cyc_Core_heap_region,msg,ap),_tag_dyneither(_tmp0,sizeof(void*),0));});
if(topt == 0)
return Cyc_Absyn_wildtyp(Cyc_Tcenv_lookup_opt_type_vars(te));else{
# 60
return*topt;}}struct _tuple17{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
# 71 "tcexp.cyc"
static struct _tuple17*Cyc_Tcexp_make_struct_arg(struct Cyc_Absyn_Exp*e){
return({struct _tuple17*_tmp1=_cycalloc(sizeof(*_tmp1));_tmp1->f1=0;_tmp1->f2=e;_tmp1;});}
# 76
static int Cyc_Tcexp_resolve_unknown_fn(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e,struct Cyc_Absyn_Exp*e1,struct Cyc_List_List*es){
void*_tmp2=e1->r;void*_tmp3=_tmp2;void**_tmp28;if(((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp3)->tag == 1){_LL1: _tmp28=(void**)&((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp3)->f1;_LL2: {
# 79
struct _tuple1*_tmp4=Cyc_Absyn_binding2qvar(*_tmp28);
struct _handler_cons _tmp5;_push_handler(& _tmp5);{int _tmp7=0;if(setjmp(_tmp5.handler))_tmp7=1;if(!_tmp7){
{struct _RegionHandle _tmp8=_new_region("r");struct _RegionHandle*r=& _tmp8;_push_region(r);
{void*_tmp9=Cyc_Tcenv_lookup_ordinary(r,te,e1->loc,_tmp4,1);void*_tmpA=_tmp9;struct Cyc_Absyn_Aggrdecl*_tmp22;struct Cyc_Absyn_Datatypedecl*_tmp21;struct Cyc_Absyn_Datatypefield*_tmp20;void*_tmp1F;switch(*((int*)_tmpA)){case 0: if(((struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct*)((struct Cyc_Tcenv_VarRes_Tcenv_Resolved_struct*)_tmpA)->f1)->tag == 0){_LL6: _LL7:
# 84
({struct Cyc_String_pa_PrintArg_struct _tmpD;_tmpD.tag=0;_tmpD.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmp4));({void*_tmpB[1]={& _tmpD};Cyc_Tcutil_terr(e1->loc,({const char*_tmpC="undeclared identifier %s";_tag_dyneither(_tmpC,sizeof(char),25);}),_tag_dyneither(_tmpB,sizeof(void*),1));});});{
int _tmpE=0;_npop_handler(1);return _tmpE;};}else{_LL8: _tmp1F=(void*)((struct Cyc_Tcenv_VarRes_Tcenv_Resolved_struct*)_tmpA)->f1;_LL9:
# 87
*_tmp28=_tmp1F;{
int _tmpF=1;_npop_handler(1);return _tmpF;};}case 2: _LLA: _tmp21=((struct Cyc_Tcenv_DatatypeRes_Tcenv_Resolved_struct*)_tmpA)->f1;_tmp20=((struct Cyc_Tcenv_DatatypeRes_Tcenv_Resolved_struct*)_tmpA)->f2;_LLB:
# 90
 if(_tmp20->typs == 0){
# 92
({struct Cyc_String_pa_PrintArg_struct _tmp12;_tmp12.tag=0;_tmp12.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_qvar2string(_tmp20->name));({void*_tmp10[1]={& _tmp12};Cyc_Tcutil_terr(e->loc,({const char*_tmp11="%s is a constant, not a function";_tag_dyneither(_tmp11,sizeof(char),33);}),_tag_dyneither(_tmp10,sizeof(void*),1));});});{
int _tmp13=0;_npop_handler(1);return _tmp13;};}
# 96
e->r=(void*)({struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*_tmp14=_cycalloc(sizeof(*_tmp14));_tmp14[0]=({struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct _tmp15;_tmp15.tag=30;_tmp15.f1=es;_tmp15.f2=_tmp21;_tmp15.f3=_tmp20;_tmp15;});_tmp14;});{
int _tmp16=1;_npop_handler(1);return _tmp16;};case 1: _LLC: _tmp22=((struct Cyc_Tcenv_AggrRes_Tcenv_Resolved_struct*)_tmpA)->f1;_LLD: {
# 99
struct Cyc_List_List*_tmp17=((struct Cyc_List_List*(*)(struct _tuple17*(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Tcexp_make_struct_arg,es);
e->r=(void*)({struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_tmp18=_cycalloc(sizeof(*_tmp18));_tmp18[0]=({struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct _tmp19;_tmp19.tag=28;_tmp19.f1=_tmp22->name;_tmp19.f2=0;_tmp19.f3=_tmp17;_tmp19.f4=_tmp22;_tmp19;});_tmp18;});{
int _tmp1A=1;_npop_handler(1);return _tmp1A;};}case 4: _LLE: _LLF:
 goto _LL11;default: _LL10: _LL11:
# 104
({struct Cyc_String_pa_PrintArg_struct _tmp1D;_tmp1D.tag=0;_tmp1D.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmp4));({void*_tmp1B[1]={& _tmp1D};Cyc_Tcutil_terr(e->loc,({const char*_tmp1C="%s is an enum constructor, not a function";_tag_dyneither(_tmp1C,sizeof(char),42);}),_tag_dyneither(_tmp1B,sizeof(void*),1));});});{
int _tmp1E=0;_npop_handler(1);return _tmp1E;};}_LL5:;}
# 82
;_pop_region(r);}
# 81
;_pop_handler();}else{void*_tmp6=(void*)_exn_thrown;void*_tmp23=_tmp6;void*_tmp27;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp23)->tag == Cyc_Dict_Absent){_LL13: _LL14:
# 109
({struct Cyc_String_pa_PrintArg_struct _tmp26;_tmp26.tag=0;_tmp26.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmp4));({void*_tmp24[1]={& _tmp26};Cyc_Tcutil_terr(e1->loc,({const char*_tmp25="undeclared identifier %s";_tag_dyneither(_tmp25,sizeof(char),25);}),_tag_dyneither(_tmp24,sizeof(void*),1));});});
return 0;}else{_LL15: _tmp27=_tmp23;_LL16:(int)_rethrow(_tmp27);}_LL12:;}};}}else{_LL3: _LL4:
# 113
 return 1;}_LL0:;}
# 117
static void Cyc_Tcexp_resolve_unresolved_mem(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct Cyc_Absyn_Exp*e,struct Cyc_List_List*des){
# 121
if(topt == 0){
# 123
e->r=(void*)({struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_tmp29=_cycalloc(sizeof(*_tmp29));_tmp29[0]=({struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct _tmp2A;_tmp2A.tag=25;_tmp2A.f1=des;_tmp2A;});_tmp29;});
return;}{
# 126
void*t=*topt;
void*_tmp2B=Cyc_Tcutil_compress(t);void*_tmp2C=_tmp2B;void*_tmp3B;struct Cyc_Absyn_Tqual _tmp3A;union Cyc_Absyn_AggrInfoU _tmp39;switch(*((int*)_tmp2C)){case 11: _LL18: _tmp39=(((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp2C)->f1).aggr_info;_LL19:
# 129
{union Cyc_Absyn_AggrInfoU _tmp2D=_tmp39;struct Cyc_Absyn_Aggrdecl*_tmp32;if((_tmp2D.UnknownAggr).tag == 1){_LL21: _LL22:
({void*_tmp2E=0;((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(({const char*_tmp2F="struct type not properly set";_tag_dyneither(_tmp2F,sizeof(char),29);}),_tag_dyneither(_tmp2E,sizeof(void*),0));});}else{_LL23: _tmp32=*(_tmp2D.KnownAggr).val;_LL24:
 e->r=(void*)({struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_tmp30=_cycalloc(sizeof(*_tmp30));_tmp30[0]=({struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct _tmp31;_tmp31.tag=28;_tmp31.f1=_tmp32->name;_tmp31.f2=0;_tmp31.f3=des;_tmp31.f4=_tmp32;_tmp31;});_tmp30;});}_LL20:;}
# 133
goto _LL17;case 8: _LL1A: _tmp3B=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp2C)->f1).elt_type;_tmp3A=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp2C)->f1).tq;_LL1B:
 e->r=(void*)({struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_tmp33=_cycalloc(sizeof(*_tmp33));_tmp33[0]=({struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct _tmp34;_tmp34.tag=25;_tmp34.f1=des;_tmp34;});_tmp33;});goto _LL17;case 12: _LL1C: _LL1D:
 e->r=(void*)({struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*_tmp35=_cycalloc(sizeof(*_tmp35));_tmp35[0]=({struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct _tmp36;_tmp36.tag=29;_tmp36.f1=t;_tmp36.f2=des;_tmp36;});_tmp35;});goto _LL17;default: _LL1E: _LL1F:
 e->r=(void*)({struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_tmp37=_cycalloc(sizeof(*_tmp37));_tmp37[0]=({struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct _tmp38;_tmp38.tag=25;_tmp38.f1=des;_tmp38;});_tmp37;});goto _LL17;}_LL17:;};}
# 143
static void Cyc_Tcexp_tcExpNoInst(struct Cyc_Tcenv_Tenv*te,void**topt,struct Cyc_Absyn_Exp*e);
static void*Cyc_Tcexp_tcExpNoPromote(struct Cyc_Tcenv_Tenv*te,void**topt,struct Cyc_Absyn_Exp*e);
# 147
static void Cyc_Tcexp_tcExpList(struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*es){
for(0;es != 0;es=es->tl){
Cyc_Tcexp_tcExp(te,0,(struct Cyc_Absyn_Exp*)es->hd);}}
# 153
static void Cyc_Tcexp_check_contains_assign(struct Cyc_Absyn_Exp*e){
void*_tmp3C=e->r;void*_tmp3D=_tmp3C;if(((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp3D)->tag == 3){if(((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp3D)->f2 == 0){_LL26: _LL27:
({void*_tmp3E=0;Cyc_Tcutil_warn(e->loc,({const char*_tmp3F="assignment in test";_tag_dyneither(_tmp3F,sizeof(char),19);}),_tag_dyneither(_tmp3E,sizeof(void*),0));});goto _LL25;}else{goto _LL28;}}else{_LL28: _LL29:
 goto _LL25;}_LL25:;}
# 161
struct Cyc_Tcexp_TestEnv Cyc_Tcexp_tcTest(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e,struct _dyneither_ptr msg_part){
Cyc_Tcexp_check_contains_assign(e);
Cyc_Tcexp_tcExp(te,& Cyc_Absyn_sint_typ,e);
if(!Cyc_Tcutil_coerce_to_bool(te,e))
({struct Cyc_String_pa_PrintArg_struct _tmp43;_tmp43.tag=0;_tmp43.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string((void*)_check_null(e->topt)));({struct Cyc_String_pa_PrintArg_struct _tmp42;_tmp42.tag=0;_tmp42.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)msg_part);({void*_tmp40[2]={& _tmp42,& _tmp43};Cyc_Tcutil_terr(e->loc,({const char*_tmp41="test of %s has type %s instead of integral or pointer type";_tag_dyneither(_tmp41,sizeof(char),59);}),_tag_dyneither(_tmp40,sizeof(void*),2));});});});
# 170
{void*_tmp44=e->r;void*_tmp45=_tmp44;enum Cyc_Absyn_Primop _tmp4E;struct Cyc_List_List*_tmp4D;if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp45)->tag == 2){_LL2B: _tmp4E=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp45)->f1;_tmp4D=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp45)->f2;_LL2C:
# 172
 if(_tmp4E == Cyc_Absyn_Eq  || _tmp4E == Cyc_Absyn_Neq){
struct _tuple0 _tmp46=({struct _tuple0 _tmp4C;_tmp4C.f1=(void*)_check_null(((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(_tmp4D))->hd)->topt);_tmp4C.f2=(void*)_check_null(((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(((struct Cyc_List_List*)_check_null(_tmp4D))->tl))->hd)->topt);_tmp4C;});struct _tuple0 _tmp47=_tmp46;void*_tmp4B;void*_tmp4A;if(((struct Cyc_Absyn_RgnHandleType_Absyn_Type_struct*)_tmp47.f1)->tag == 15){if(((struct Cyc_Absyn_RgnHandleType_Absyn_Type_struct*)_tmp47.f2)->tag == 15){_LL30: _tmp4B=(void*)((struct Cyc_Absyn_RgnHandleType_Absyn_Type_struct*)_tmp47.f1)->f1;_tmp4A=(void*)((struct Cyc_Absyn_RgnHandleType_Absyn_Type_struct*)_tmp47.f2)->f1;_LL31:
# 175
 return({struct Cyc_Tcexp_TestEnv _tmp48;_tmp48.eq=({struct _tuple0*_tmp49=_region_malloc(Cyc_Tcenv_get_fnrgn(te),sizeof(*_tmp49));_tmp49->f1=_tmp4B;_tmp49->f2=_tmp4A;_tmp49;});_tmp48.isTrue=_tmp4E == Cyc_Absyn_Eq;_tmp48;});}else{goto _LL32;}}else{_LL32: _LL33:
# 177
 goto _LL2F;}_LL2F:;}
# 180
goto _LL2A;}else{_LL2D: _LL2E:
 goto _LL2A;}_LL2A:;}
# 183
return({struct Cyc_Tcexp_TestEnv _tmp4F;_tmp4F.eq=0;_tmp4F.isTrue=0;_tmp4F;});}
# 204 "tcexp.cyc"
static int Cyc_Tcexp_wchar_numelts(struct _dyneither_ptr s){
return 1;}
# 209
static void*Cyc_Tcexp_tcConst(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,union Cyc_Absyn_Cnst*c,struct Cyc_Absyn_Exp*e){
void*t;
void*string_elt_typ=Cyc_Absyn_char_typ;
int string_numelts=0;
{union Cyc_Absyn_Cnst _tmp50=*((union Cyc_Absyn_Cnst*)_check_null(c));union Cyc_Absyn_Cnst _tmp51=_tmp50;struct _dyneither_ptr _tmp87;struct _dyneither_ptr _tmp86;enum Cyc_Absyn_Sign _tmp85;int _tmp84;int _tmp83;enum Cyc_Absyn_Sign _tmp82;enum Cyc_Absyn_Sign _tmp81;switch((_tmp51.Wstring_c).tag){case 2: switch(((_tmp51.Char_c).val).f1){case Cyc_Absyn_Signed: _LL35: _LL36:
 t=Cyc_Absyn_schar_typ;goto _LL34;case Cyc_Absyn_Unsigned: _LL37: _LL38:
 t=Cyc_Absyn_uchar_typ;goto _LL34;default: _LL39: _LL3A:
 t=Cyc_Absyn_char_typ;goto _LL34;}case 3: _LL3B: _LL3C:
 t=Cyc_Absyn_wchar_typ();goto _LL34;case 4: _LL3D: _tmp81=((_tmp51.Short_c).val).f1;_LL3E:
# 219
 t=_tmp81 == Cyc_Absyn_Unsigned?Cyc_Absyn_ushort_typ: Cyc_Absyn_sshort_typ;goto _LL34;case 6: _LL3F: _tmp82=((_tmp51.LongLong_c).val).f1;_LL40:
# 221
 t=_tmp82 == Cyc_Absyn_Unsigned?Cyc_Absyn_ulonglong_typ: Cyc_Absyn_slonglong_typ;goto _LL34;case 7: _LL41: _tmp83=((_tmp51.Float_c).val).f2;_LL42:
 t=Cyc_Absyn_float_typ(_tmp83);goto _LL34;case 5: _LL43: _tmp85=((_tmp51.Int_c).val).f1;_tmp84=((_tmp51.Int_c).val).f2;_LL44:
# 224
 if(topt == 0)
t=_tmp85 == Cyc_Absyn_Unsigned?Cyc_Absyn_uint_typ: Cyc_Absyn_sint_typ;else{
# 231
void*_tmp52=Cyc_Tcutil_compress(*topt);void*_tmp53=_tmp52;void*_tmp69;void*_tmp68;struct Cyc_Absyn_Tqual _tmp67;void*_tmp66;union Cyc_Absyn_Constraint*_tmp65;union Cyc_Absyn_Constraint*_tmp64;union Cyc_Absyn_Constraint*_tmp63;enum Cyc_Absyn_Sign _tmp62;enum Cyc_Absyn_Sign _tmp61;enum Cyc_Absyn_Sign _tmp60;enum Cyc_Absyn_Sign _tmp5F;switch(*((int*)_tmp53)){case 6: switch(((struct Cyc_Absyn_IntType_Absyn_Type_struct*)_tmp53)->f2){case Cyc_Absyn_Char_sz: _LL4C: _tmp5F=((struct Cyc_Absyn_IntType_Absyn_Type_struct*)_tmp53)->f1;_LL4D:
# 233
{enum Cyc_Absyn_Sign _tmp54=_tmp5F;switch(_tmp54){case Cyc_Absyn_Unsigned: _LL5B: _LL5C:
 t=Cyc_Absyn_uchar_typ;goto _LL5A;case Cyc_Absyn_Signed: _LL5D: _LL5E:
 t=Cyc_Absyn_schar_typ;goto _LL5A;default: _LL5F: _LL60:
 t=Cyc_Absyn_char_typ;goto _LL5A;}_LL5A:;}
# 238
*c=Cyc_Absyn_Char_c(_tmp5F,(char)_tmp84);
goto _LL4B;case Cyc_Absyn_Short_sz: _LL4E: _tmp60=((struct Cyc_Absyn_IntType_Absyn_Type_struct*)_tmp53)->f1;_LL4F:
# 241
 t=_tmp60 == Cyc_Absyn_Unsigned?Cyc_Absyn_ushort_typ: Cyc_Absyn_sshort_typ;
*c=Cyc_Absyn_Short_c(_tmp60,(short)_tmp84);
goto _LL4B;case Cyc_Absyn_Int_sz: _LL50: _tmp61=((struct Cyc_Absyn_IntType_Absyn_Type_struct*)_tmp53)->f1;_LL51:
# 245
 t=_tmp61 == Cyc_Absyn_Unsigned?Cyc_Absyn_uint_typ: Cyc_Absyn_sint_typ;
goto _LL4B;case Cyc_Absyn_Long_sz: _LL52: _tmp62=((struct Cyc_Absyn_IntType_Absyn_Type_struct*)_tmp53)->f1;_LL53:
# 248
 t=_tmp62 == Cyc_Absyn_Unsigned?Cyc_Absyn_uint_typ: Cyc_Absyn_sint_typ;
goto _LL4B;default: goto _LL58;}case 5: _LL54: _tmp68=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp53)->f1).elt_typ;_tmp67=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp53)->f1).elt_tq;_tmp66=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp53)->f1).ptr_atts).rgn;_tmp65=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp53)->f1).ptr_atts).nullable;_tmp64=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp53)->f1).ptr_atts).bounds;_tmp63=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp53)->f1).ptr_atts).zero_term;if(_tmp84 == 0){_LL55: {
# 252
static struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct nullc={0,{.Null_c={1,0}}};
e->r=(void*)& nullc;
if(((int(*)(int y,union Cyc_Absyn_Constraint*x))Cyc_Absyn_conref_constr)(1,_tmp65))return*topt;{
struct Cyc_List_List*_tmp55=Cyc_Tcenv_lookup_type_vars(te);
t=(void*)({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmp56=_cycalloc(sizeof(*_tmp56));_tmp56[0]=({struct Cyc_Absyn_PointerType_Absyn_Type_struct _tmp57;_tmp57.tag=5;_tmp57.f1=({struct Cyc_Absyn_PtrInfo _tmp58;_tmp58.elt_typ=_tmp68;_tmp58.elt_tq=_tmp67;_tmp58.ptr_atts=({(_tmp58.ptr_atts).rgn=_tmp66;(_tmp58.ptr_atts).nullable=Cyc_Absyn_true_conref;(_tmp58.ptr_atts).bounds=_tmp64;(_tmp58.ptr_atts).zero_term=_tmp63;(_tmp58.ptr_atts).ptrloc=0;_tmp58.ptr_atts;});_tmp58;});_tmp57;});_tmp56;});
# 258
goto _LL4B;};}}else{goto _LL58;}case 19: _LL56: _tmp69=(void*)((struct Cyc_Absyn_TagType_Absyn_Type_struct*)_tmp53)->f1;_LL57: {
# 260
struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_tmp5A=({struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_tmp5D=_cycalloc(sizeof(*_tmp5D));_tmp5D[0]=({struct Cyc_Absyn_ValueofType_Absyn_Type_struct _tmp5E;_tmp5E.tag=18;_tmp5E.f1=Cyc_Absyn_uint_exp((unsigned int)_tmp84,0);_tmp5E;});_tmp5D;});
# 267
t=(void*)({struct Cyc_Absyn_TagType_Absyn_Type_struct*_tmp5B=_cycalloc(sizeof(*_tmp5B));_tmp5B[0]=({struct Cyc_Absyn_TagType_Absyn_Type_struct _tmp5C;_tmp5C.tag=19;_tmp5C.f1=(void*)_tmp5A;_tmp5C;});_tmp5B;});
goto _LL4B;}default: _LL58: _LL59:
# 270
 t=_tmp85 == Cyc_Absyn_Unsigned?Cyc_Absyn_uint_typ: Cyc_Absyn_sint_typ;
goto _LL4B;}_LL4B:;}
# 273
goto _LL34;case 8: _LL45: _tmp86=(_tmp51.String_c).val;_LL46:
# 275
 string_numelts=(int)_get_dyneither_size(_tmp86,sizeof(char));
_tmp87=_tmp86;goto _LL48;case 9: _LL47: _tmp87=(_tmp51.Wstring_c).val;_LL48:
# 278
 if(string_numelts == 0){
string_numelts=Cyc_Tcexp_wchar_numelts(_tmp87);
string_elt_typ=Cyc_Absyn_wchar_typ();}{
# 282
struct Cyc_Absyn_Exp*elen=Cyc_Absyn_const_exp(Cyc_Absyn_Int_c(Cyc_Absyn_Unsigned,string_numelts),loc);
elen->topt=Cyc_Absyn_uint_typ;{
# 287
struct Cyc_Absyn_Upper_b_Absyn_Bounds_struct*_tmp6A=({struct Cyc_Absyn_Upper_b_Absyn_Bounds_struct*_tmp6E=_cycalloc(sizeof(*_tmp6E));_tmp6E[0]=({struct Cyc_Absyn_Upper_b_Absyn_Bounds_struct _tmp6F;_tmp6F.tag=1;_tmp6F.f1=elen;_tmp6F;});_tmp6E;});
t=Cyc_Absyn_atb_typ(string_elt_typ,(void*)& Cyc_Absyn_HeapRgn_val,Cyc_Absyn_const_tqual(0),(void*)_tmp6A,Cyc_Absyn_true_conref);
# 290
if(topt != 0){
void*_tmp6B=Cyc_Tcutil_compress(*topt);void*_tmp6C=_tmp6B;struct Cyc_Absyn_Tqual _tmp6D;switch(*((int*)_tmp6C)){case 8: _LL62: _tmp6D=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp6C)->f1).tq;_LL63:
# 295
 return Cyc_Absyn_array_typ(string_elt_typ,_tmp6D,elen,((union Cyc_Absyn_Constraint*(*)())Cyc_Absyn_empty_conref)(),0);case 5: _LL64: _LL65:
# 297
 if(!Cyc_Tcutil_unify(*topt,t) && Cyc_Tcutil_silent_castable(te,loc,t,*topt)){
e->topt=t;
Cyc_Tcutil_unchecked_cast(te,e,*topt,Cyc_Absyn_Other_coercion);
t=*topt;}else{
# 303
t=Cyc_Absyn_atb_typ(string_elt_typ,Cyc_Absyn_new_evar(& Cyc_Tcutil_rko,Cyc_Tcenv_lookup_opt_type_vars(te)),
Cyc_Absyn_const_tqual(0),(void*)_tmp6A,Cyc_Absyn_true_conref);
if(!Cyc_Tcutil_unify(*topt,t) && Cyc_Tcutil_silent_castable(te,loc,t,*topt)){
e->topt=t;
Cyc_Tcutil_unchecked_cast(te,e,*topt,Cyc_Absyn_Other_coercion);
t=*topt;}}
# 311
goto _LL61;default: _LL66: _LL67:
 goto _LL61;}_LL61:;}
# 315
return t;};};default: _LL49: _LL4A:
# 317
 if(topt != 0){
void*_tmp70=Cyc_Tcutil_compress(*topt);void*_tmp71=_tmp70;void*_tmp7A;struct Cyc_Absyn_Tqual _tmp79;void*_tmp78;union Cyc_Absyn_Constraint*_tmp77;union Cyc_Absyn_Constraint*_tmp76;union Cyc_Absyn_Constraint*_tmp75;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp71)->tag == 5){_LL69: _tmp7A=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp71)->f1).elt_typ;_tmp79=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp71)->f1).elt_tq;_tmp78=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp71)->f1).ptr_atts).rgn;_tmp77=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp71)->f1).ptr_atts).nullable;_tmp76=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp71)->f1).ptr_atts).bounds;_tmp75=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp71)->f1).ptr_atts).zero_term;_LL6A:
# 321
 if(((int(*)(int y,union Cyc_Absyn_Constraint*x))Cyc_Absyn_conref_constr)(1,_tmp77))return*topt;
return(void*)({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmp72=_cycalloc(sizeof(*_tmp72));_tmp72[0]=({struct Cyc_Absyn_PointerType_Absyn_Type_struct _tmp73;_tmp73.tag=5;_tmp73.f1=({struct Cyc_Absyn_PtrInfo _tmp74;_tmp74.elt_typ=_tmp7A;_tmp74.elt_tq=_tmp79;_tmp74.ptr_atts=({(_tmp74.ptr_atts).rgn=_tmp78;(_tmp74.ptr_atts).nullable=Cyc_Absyn_true_conref;(_tmp74.ptr_atts).bounds=_tmp76;(_tmp74.ptr_atts).zero_term=_tmp75;(_tmp74.ptr_atts).ptrloc=0;_tmp74.ptr_atts;});_tmp74;});_tmp73;});_tmp72;});}else{_LL6B: _LL6C:
# 324
 goto _LL68;}_LL68:;}{
# 327
struct Cyc_List_List*_tmp7B=Cyc_Tcenv_lookup_type_vars(te);
t=(void*)({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmp7C=_cycalloc(sizeof(*_tmp7C));_tmp7C[0]=({struct Cyc_Absyn_PointerType_Absyn_Type_struct _tmp7D;_tmp7D.tag=5;_tmp7D.f1=({struct Cyc_Absyn_PtrInfo _tmp7E;_tmp7E.elt_typ=Cyc_Absyn_new_evar(& Cyc_Tcutil_tako,({struct Cyc_Core_Opt*_tmp80=_cycalloc(sizeof(*_tmp80));_tmp80->v=_tmp7B;_tmp80;}));_tmp7E.elt_tq=
Cyc_Absyn_empty_tqual(0);_tmp7E.ptr_atts=({(_tmp7E.ptr_atts).rgn=
Cyc_Absyn_new_evar(& Cyc_Tcutil_trko,({struct Cyc_Core_Opt*_tmp7F=_cycalloc(sizeof(*_tmp7F));_tmp7F->v=_tmp7B;_tmp7F;}));(_tmp7E.ptr_atts).nullable=Cyc_Absyn_true_conref;(_tmp7E.ptr_atts).bounds=
# 332
((union Cyc_Absyn_Constraint*(*)())Cyc_Absyn_empty_conref)();(_tmp7E.ptr_atts).zero_term=((union Cyc_Absyn_Constraint*(*)())Cyc_Absyn_empty_conref)();(_tmp7E.ptr_atts).ptrloc=0;_tmp7E.ptr_atts;});_tmp7E;});_tmp7D;});_tmp7C;});
# 334
goto _LL34;};}_LL34:;}
# 336
return t;}
# 340
static void*Cyc_Tcexp_tcDatatype(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct Cyc_Absyn_Exp*e,struct Cyc_List_List*es,struct Cyc_Absyn_Datatypedecl*tud,struct Cyc_Absyn_Datatypefield*tuf);
# 345
static void*Cyc_Tcexp_tcVar(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct Cyc_Absyn_Exp*e,void**b){
# 348
void*_tmp88=*((void**)_check_null(b));void*_tmp89=_tmp88;struct Cyc_Absyn_Vardecl*_tmpBA;struct Cyc_Absyn_Vardecl*_tmpB9;struct Cyc_Absyn_Vardecl*_tmpB8;struct Cyc_Absyn_Fndecl*_tmpB7;struct Cyc_Absyn_Vardecl*_tmpB6;struct _tuple1*_tmpB5;switch(*((int*)_tmp89)){case 0: _LL6E: _tmpB5=((struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct*)_tmp89)->f1;_LL6F: {
# 350
struct _handler_cons _tmp8A;_push_handler(& _tmp8A);{int _tmp8C=0;if(setjmp(_tmp8A.handler))_tmp8C=1;if(!_tmp8C){
{struct _RegionHandle _tmp8D=_new_region("r");struct _RegionHandle*r=& _tmp8D;_push_region(r);
{void*_tmp8E=Cyc_Tcenv_lookup_ordinary(r,te,e->loc,_tmpB5,1);void*_tmp8F=_tmp8E;struct Cyc_Absyn_Datatypedecl*_tmpAA;struct Cyc_Absyn_Datatypefield*_tmpA9;void*_tmpA8;struct Cyc_Absyn_Enumfield*_tmpA7;struct Cyc_Absyn_Enumdecl*_tmpA6;struct Cyc_Absyn_Enumfield*_tmpA5;void*_tmpA4;switch(*((int*)_tmp8F)){case 0: if(((struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct*)((struct Cyc_Tcenv_VarRes_Tcenv_Resolved_struct*)_tmp8F)->f1)->tag == 0){_LL7B: _LL7C: {
# 354
void*_tmp93=({struct Cyc_String_pa_PrintArg_struct _tmp92;_tmp92.tag=0;_tmp92.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmpB5));({void*_tmp90[1]={& _tmp92};Cyc_Tcexp_expr_err(te,loc,0,({const char*_tmp91="undeclared identifier %s";_tag_dyneither(_tmp91,sizeof(char),25);}),_tag_dyneither(_tmp90,sizeof(void*),1));});});_npop_handler(1);return _tmp93;}}else{_LL7D: _tmpA4=(void*)((struct Cyc_Tcenv_VarRes_Tcenv_Resolved_struct*)_tmp8F)->f1;_LL7E:
# 356
*b=_tmpA4;{
void*_tmp94=Cyc_Tcexp_tcVar(te,loc,topt,e,b);_npop_handler(1);return _tmp94;};}case 3: _LL7F: _tmpA6=((struct Cyc_Tcenv_EnumRes_Tcenv_Resolved_struct*)_tmp8F)->f1;_tmpA5=((struct Cyc_Tcenv_EnumRes_Tcenv_Resolved_struct*)_tmp8F)->f2;_LL80:
# 359
 e->r=(void*)({struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*_tmp95=_cycalloc(sizeof(*_tmp95));_tmp95[0]=({struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct _tmp96;_tmp96.tag=31;_tmp96.f1=_tmpA5->name;_tmp96.f2=_tmpA6;_tmp96.f3=_tmpA5;_tmp96;});_tmp95;});{
void*_tmp99=(void*)({struct Cyc_Absyn_EnumType_Absyn_Type_struct*_tmp97=_cycalloc(sizeof(*_tmp97));_tmp97[0]=({struct Cyc_Absyn_EnumType_Absyn_Type_struct _tmp98;_tmp98.tag=13;_tmp98.f1=_tmpA6->name;_tmp98.f2=_tmpA6;_tmp98;});_tmp97;});_npop_handler(1);return _tmp99;};case 4: _LL81: _tmpA8=(void*)((struct Cyc_Tcenv_AnonEnumRes_Tcenv_Resolved_struct*)_tmp8F)->f1;_tmpA7=((struct Cyc_Tcenv_AnonEnumRes_Tcenv_Resolved_struct*)_tmp8F)->f2;_LL82:
# 362
 e->r=(void*)({struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*_tmp9A=_cycalloc(sizeof(*_tmp9A));_tmp9A[0]=({struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct _tmp9B;_tmp9B.tag=32;_tmp9B.f1=_tmpA7->name;_tmp9B.f2=_tmpA8;_tmp9B.f3=_tmpA7;_tmp9B;});_tmp9A;});{
void*_tmp9C=_tmpA8;_npop_handler(1);return _tmp9C;};case 2: _LL83: _tmpAA=((struct Cyc_Tcenv_DatatypeRes_Tcenv_Resolved_struct*)_tmp8F)->f1;_tmpA9=((struct Cyc_Tcenv_DatatypeRes_Tcenv_Resolved_struct*)_tmp8F)->f2;_LL84:
# 365
 e->r=(void*)({struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*_tmp9D=_cycalloc(sizeof(*_tmp9D));_tmp9D[0]=({struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct _tmp9E;_tmp9E.tag=30;_tmp9E.f1=0;_tmp9E.f2=_tmpAA;_tmp9E.f3=_tmpA9;_tmp9E;});_tmp9D;});{
void*_tmp9F=Cyc_Tcexp_tcDatatype(te,loc,topt,e,0,_tmpAA,_tmpA9);_npop_handler(1);return _tmp9F;};default: _LL85: _LL86: {
# 368
void*_tmpA3=({struct Cyc_String_pa_PrintArg_struct _tmpA2;_tmpA2.tag=0;_tmpA2.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmpB5));({void*_tmpA0[1]={& _tmpA2};Cyc_Tcexp_expr_err(te,loc,0,({const char*_tmpA1="bad occurrence of type name %s";_tag_dyneither(_tmpA1,sizeof(char),31);}),_tag_dyneither(_tmpA0,sizeof(void*),1));});});_npop_handler(1);return _tmpA3;}}_LL7A:;}
# 352
;_pop_region(r);}
# 351
;_pop_handler();}else{void*_tmp8B=(void*)_exn_thrown;void*_tmpAB=_tmp8B;void*_tmpAF;if(((struct Cyc_Dict_Absent_exn_struct*)_tmpAB)->tag == Cyc_Dict_Absent){_LL88: _LL89:
# 373
 return({struct Cyc_String_pa_PrintArg_struct _tmpAE;_tmpAE.tag=0;_tmpAE.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmpB5));({void*_tmpAC[1]={& _tmpAE};Cyc_Tcexp_expr_err(te,loc,0,({const char*_tmpAD="undeclared identifier %s";_tag_dyneither(_tmpAD,sizeof(char),25);}),_tag_dyneither(_tmpAC,sizeof(void*),1));});});}else{_LL8A: _tmpAF=_tmpAB;_LL8B:(int)_rethrow(_tmpAF);}_LL87:;}};}case 1: _LL70: _tmpB6=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)_tmp89)->f1;_LL71:
# 375
 return _tmpB6->type;case 2: _LL72: _tmpB7=((struct Cyc_Absyn_Funname_b_Absyn_Binding_struct*)_tmp89)->f1;_LL73:
 return Cyc_Tcutil_fndecl2typ(_tmpB7);case 5: _LL74: _tmpB8=((struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*)_tmp89)->f1;_LL75:
 _tmpB9=_tmpB8;goto _LL77;case 4: _LL76: _tmpB9=((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)_tmp89)->f1;_LL77:
 _tmpBA=_tmpB9;goto _LL79;default: _LL78: _tmpBA=((struct Cyc_Absyn_Param_b_Absyn_Binding_struct*)_tmp89)->f1;_LL79:
# 380
 if(te->allow_valueof){
void*_tmpB0=Cyc_Tcutil_compress(_tmpBA->type);void*_tmpB1=_tmpB0;void*_tmpB4;if(((struct Cyc_Absyn_TagType_Absyn_Type_struct*)_tmpB1)->tag == 19){_LL8D: _tmpB4=(void*)((struct Cyc_Absyn_TagType_Absyn_Type_struct*)_tmpB1)->f1;_LL8E:
# 383
 e->r=(void*)({struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*_tmpB2=_cycalloc(sizeof(*_tmpB2));_tmpB2[0]=({struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct _tmpB3;_tmpB3.tag=38;_tmpB3.f1=_tmpB4;_tmpB3;});_tmpB2;});
goto _LL8C;}else{_LL8F: _LL90:
 goto _LL8C;}_LL8C:;}
# 388
return _tmpBA->type;}_LL6D:;}
# 392
static void Cyc_Tcexp_check_format_args(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*fmt,struct Cyc_Core_Opt*opt_args,int arg_cnt,struct Cyc_List_List**alias_arg_exps,struct _RegionHandle*temp,struct Cyc_List_List*(*type_getter)(struct _RegionHandle*,struct Cyc_Tcenv_Tenv*,struct _dyneither_ptr,unsigned int)){
# 401
struct Cyc_List_List*desc_types;
{void*_tmpBB=fmt->r;void*_tmpBC=_tmpBB;struct _dyneither_ptr _tmpC2;struct _dyneither_ptr _tmpC1;switch(*((int*)_tmpBC)){case 0: if(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmpBC)->f1).String_c).tag == 8){_LL92: _tmpC1=((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmpBC)->f1).String_c).val;_LL93:
 _tmpC2=_tmpC1;goto _LL95;}else{goto _LL96;}case 13: if(((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)((struct Cyc_Absyn_Exp*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpBC)->f2)->r)->tag == 0){if(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)((struct Cyc_Absyn_Exp*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpBC)->f2)->r)->f1).String_c).tag == 8){_LL94: _tmpC2=((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)(((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpBC)->f2)->r)->f1).String_c).val;_LL95:
# 405
 desc_types=type_getter(temp,te,(struct _dyneither_ptr)_tmpC2,fmt->loc);goto _LL91;}else{goto _LL96;}}else{goto _LL96;}default: _LL96: _LL97:
# 409
 if(opt_args != 0){
struct Cyc_List_List*_tmpBD=(struct Cyc_List_List*)opt_args->v;
for(0;_tmpBD != 0;_tmpBD=_tmpBD->tl){
Cyc_Tcexp_tcExp(te,0,(struct Cyc_Absyn_Exp*)_tmpBD->hd);{
struct _RegionHandle _tmpBE=_new_region("temp");struct _RegionHandle*temp=& _tmpBE;_push_region(temp);
if(Cyc_Tcutil_is_noalias_pointer_or_aggr(temp,(void*)_check_null(((struct Cyc_Absyn_Exp*)_tmpBD->hd)->topt)) && !
Cyc_Tcutil_is_noalias_path(temp,(struct Cyc_Absyn_Exp*)_tmpBD->hd))
({void*_tmpBF=0;Cyc_Tcutil_terr(((struct Cyc_Absyn_Exp*)_tmpBD->hd)->loc,({const char*_tmpC0="Cannot consume non-unique paths; do swap instead";_tag_dyneither(_tmpC0,sizeof(char),49);}),_tag_dyneither(_tmpBF,sizeof(void*),0));});
# 414
;_pop_region(temp);};}}
# 420
return;}_LL91:;}
# 422
if(opt_args != 0){
struct Cyc_List_List*_tmpC3=(struct Cyc_List_List*)opt_args->v;
# 425
for(0;desc_types != 0  && _tmpC3 != 0;
(((desc_types=desc_types->tl,_tmpC3=_tmpC3->tl)),arg_cnt ++)){
int alias_coercion=0;
void*t=(void*)desc_types->hd;
struct Cyc_Absyn_Exp*e=(struct Cyc_Absyn_Exp*)_tmpC3->hd;
Cyc_Tcexp_tcExp(te,& t,e);
if(!Cyc_Tcutil_coerce_arg(te,e,t,& alias_coercion)){
({struct Cyc_String_pa_PrintArg_struct _tmpC7;_tmpC7.tag=0;_tmpC7.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string((void*)_check_null(e->topt)));({struct Cyc_String_pa_PrintArg_struct _tmpC6;_tmpC6.tag=0;_tmpC6.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t));({void*_tmpC4[2]={& _tmpC6,& _tmpC7};Cyc_Tcutil_terr(e->loc,({const char*_tmpC5="descriptor has type %s but argument has type %s";_tag_dyneither(_tmpC5,sizeof(char),48);}),_tag_dyneither(_tmpC4,sizeof(void*),2));});});});
Cyc_Tcutil_explain_failure();}
# 436
if(alias_coercion)
*alias_arg_exps=({struct Cyc_List_List*_tmpC8=_cycalloc(sizeof(*_tmpC8));_tmpC8->hd=(void*)arg_cnt;_tmpC8->tl=*alias_arg_exps;_tmpC8;});{
struct _RegionHandle _tmpC9=_new_region("temp");struct _RegionHandle*temp=& _tmpC9;_push_region(temp);
if(Cyc_Tcutil_is_noalias_pointer_or_aggr(temp,t) && !Cyc_Tcutil_is_noalias_path(temp,e))
({void*_tmpCA=0;Cyc_Tcutil_terr(((struct Cyc_Absyn_Exp*)_tmpC3->hd)->loc,({const char*_tmpCB="Cannot consume non-unique paths; do swap instead";_tag_dyneither(_tmpCB,sizeof(char),49);}),_tag_dyneither(_tmpCA,sizeof(void*),0));});
# 439
;_pop_region(temp);};}
# 444
if(desc_types != 0)
({void*_tmpCC=0;Cyc_Tcutil_terr(fmt->loc,({const char*_tmpCD="too few arguments";_tag_dyneither(_tmpCD,sizeof(char),18);}),_tag_dyneither(_tmpCC,sizeof(void*),0));});
if(_tmpC3 != 0){
({void*_tmpCE=0;Cyc_Tcutil_terr(((struct Cyc_Absyn_Exp*)_tmpC3->hd)->loc,({const char*_tmpCF="too many arguments";_tag_dyneither(_tmpCF,sizeof(char),19);}),_tag_dyneither(_tmpCE,sizeof(void*),0));});
(int)_throw((void*)({struct Cyc_Tcutil_AbortTypeCheckingFunction_exn_struct*_tmpD0=_cycalloc_atomic(sizeof(*_tmpD0));_tmpD0[0]=({struct Cyc_Tcutil_AbortTypeCheckingFunction_exn_struct _tmpD1;_tmpD1.tag=Cyc_Tcutil_AbortTypeCheckingFunction;_tmpD1;});_tmpD0;}));}}}
# 452
static void*Cyc_Tcexp_tcUnPrimop(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,enum Cyc_Absyn_Primop p,struct Cyc_Absyn_Exp*e){
# 454
void*t=Cyc_Tcutil_compress((void*)_check_null(e->topt));
enum Cyc_Absyn_Primop _tmpD2=p;switch(_tmpD2){case Cyc_Absyn_Plus: _LL99: _LL9A:
 goto _LL9C;case Cyc_Absyn_Minus: _LL9B: _LL9C:
# 458
 if(!Cyc_Tcutil_is_numeric(e))
({struct Cyc_String_pa_PrintArg_struct _tmpD5;_tmpD5.tag=0;_tmpD5.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t));({void*_tmpD3[1]={& _tmpD5};Cyc_Tcutil_terr(loc,({const char*_tmpD4="expecting numeric type but found %s";_tag_dyneither(_tmpD4,sizeof(char),36);}),_tag_dyneither(_tmpD3,sizeof(void*),1));});});
return(void*)_check_null(e->topt);case Cyc_Absyn_Not: _LL9D: _LL9E:
# 462
 Cyc_Tcexp_check_contains_assign(e);
if(!Cyc_Tcutil_coerce_to_bool(te,e))
({struct Cyc_String_pa_PrintArg_struct _tmpD8;_tmpD8.tag=0;_tmpD8.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t));({void*_tmpD6[1]={& _tmpD8};Cyc_Tcutil_terr(loc,({const char*_tmpD7="expecting integral or * type but found %s";_tag_dyneither(_tmpD7,sizeof(char),42);}),_tag_dyneither(_tmpD6,sizeof(void*),1));});});
return Cyc_Absyn_sint_typ;case Cyc_Absyn_Bitnot: _LL9F: _LLA0:
# 467
 if(!Cyc_Tcutil_is_integral(e))
({struct Cyc_String_pa_PrintArg_struct _tmpDB;_tmpDB.tag=0;_tmpDB.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t));({void*_tmpD9[1]={& _tmpDB};Cyc_Tcutil_terr(loc,({const char*_tmpDA="expecting integral type but found %s";_tag_dyneither(_tmpDA,sizeof(char),37);}),_tag_dyneither(_tmpD9,sizeof(void*),1));});});
return(void*)_check_null(e->topt);case Cyc_Absyn_Numelts: _LLA1: _LLA2:
# 471
{void*_tmpDC=t;union Cyc_Absyn_Constraint*_tmpE5;switch(*((int*)_tmpDC)){case 8: _LLA6: _LLA7:
 goto _LLA5;case 5: _LLA8: _tmpE5=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpDC)->f1).ptr_atts).bounds;_LLA9:
# 474
{void*_tmpDD=((void*(*)(void*y,union Cyc_Absyn_Constraint*x))Cyc_Absyn_conref_constr)((void*)& Cyc_Absyn_DynEither_b_val,_tmpE5);void*_tmpDE=_tmpDD;struct Cyc_Absyn_Exp*_tmpE1;if(((struct Cyc_Absyn_DynEither_b_Absyn_Bounds_struct*)_tmpDE)->tag == 0){_LLAD: _LLAE:
 goto _LLAC;}else{_LLAF: _tmpE1=((struct Cyc_Absyn_Upper_b_Absyn_Bounds_struct*)_tmpDE)->f1;_LLB0:
# 477
 if(!Cyc_Evexp_c_can_eval(_tmpE1) && !((unsigned int)Cyc_Tcenv_allow_valueof))
({void*_tmpDF=0;Cyc_Tcutil_terr(loc,({const char*_tmpE0="cannot apply numelts to a pointer with abstract bounds";_tag_dyneither(_tmpE0,sizeof(char),55);}),_tag_dyneither(_tmpDF,sizeof(void*),0));});
goto _LLAC;}_LLAC:;}
# 481
goto _LLA5;default: _LLAA: _LLAB:
# 483
({struct Cyc_String_pa_PrintArg_struct _tmpE4;_tmpE4.tag=0;_tmpE4.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t));({void*_tmpE2[1]={& _tmpE4};Cyc_Tcutil_terr(loc,({const char*_tmpE3="numelts requires pointer or array type, not %s";_tag_dyneither(_tmpE3,sizeof(char),47);}),_tag_dyneither(_tmpE2,sizeof(void*),1));});});}_LLA5:;}
# 485
return Cyc_Absyn_uint_typ;default: _LLA3: _LLA4:
({void*_tmpE6=0;((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(({const char*_tmpE7="Non-unary primop";_tag_dyneither(_tmpE7,sizeof(char),17);}),_tag_dyneither(_tmpE6,sizeof(void*),0));});}_LL98:;}
# 491
static void*Cyc_Tcexp_tcArithBinop(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,int(*checker)(struct Cyc_Absyn_Exp*)){
# 494
if(!checker(e1)){
({struct Cyc_String_pa_PrintArg_struct _tmpEA;_tmpEA.tag=0;_tmpEA.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string((void*)_check_null(e1->topt)));({void*_tmpE8[1]={& _tmpEA};Cyc_Tcutil_terr(e1->loc,({const char*_tmpE9="type %s cannot be used here";_tag_dyneither(_tmpE9,sizeof(char),28);}),_tag_dyneither(_tmpE8,sizeof(void*),1));});});
return Cyc_Absyn_wildtyp(Cyc_Tcenv_lookup_opt_type_vars(te));}
# 498
if(!checker(e2)){
({struct Cyc_String_pa_PrintArg_struct _tmpED;_tmpED.tag=0;_tmpED.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string((void*)_check_null(e2->topt)));({void*_tmpEB[1]={& _tmpED};Cyc_Tcutil_terr(e2->loc,({const char*_tmpEC="type %s cannot be used here";_tag_dyneither(_tmpEC,sizeof(char),28);}),_tag_dyneither(_tmpEB,sizeof(void*),1));});});
return Cyc_Absyn_wildtyp(Cyc_Tcenv_lookup_opt_type_vars(te));}{
# 502
void*t1=Cyc_Tcutil_compress((void*)_check_null(e1->topt));
void*t2=Cyc_Tcutil_compress((void*)_check_null(e2->topt));
return Cyc_Tcutil_max_arithmetic_type(t1,t2);};}
# 507
static void*Cyc_Tcexp_tcPlus(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
void*t1=Cyc_Tcutil_compress((void*)_check_null(e1->topt));
void*t2=Cyc_Tcutil_compress((void*)_check_null(e2->topt));
void*_tmpEE=t1;void*_tmp104;struct Cyc_Absyn_Tqual _tmp103;void*_tmp102;union Cyc_Absyn_Constraint*_tmp101;union Cyc_Absyn_Constraint*_tmp100;union Cyc_Absyn_Constraint*_tmpFF;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpEE)->tag == 5){_LLB2: _tmp104=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpEE)->f1).elt_typ;_tmp103=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpEE)->f1).elt_tq;_tmp102=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpEE)->f1).ptr_atts).rgn;_tmp101=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpEE)->f1).ptr_atts).nullable;_tmp100=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpEE)->f1).ptr_atts).bounds;_tmpFF=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpEE)->f1).ptr_atts).zero_term;_LLB3:
# 512
 if(!Cyc_Tcutil_kind_leq(Cyc_Tcutil_typ_kind(_tmp104),& Cyc_Tcutil_tmk))
({void*_tmpEF=0;Cyc_Tcutil_terr(e1->loc,({const char*_tmpF0="can't perform arithmetic on abstract pointer type";_tag_dyneither(_tmpF0,sizeof(char),50);}),_tag_dyneither(_tmpEF,sizeof(void*),0));});
if(Cyc_Tcutil_is_noalias_pointer(t1,0))
({void*_tmpF1=0;Cyc_Tcutil_terr(e1->loc,({const char*_tmpF2="can't perform arithmetic on non-aliasing pointer type";_tag_dyneither(_tmpF2,sizeof(char),54);}),_tag_dyneither(_tmpF1,sizeof(void*),0));});
if(!Cyc_Tcutil_coerce_sint_typ(te,e2))
({struct Cyc_String_pa_PrintArg_struct _tmpF5;_tmpF5.tag=0;_tmpF5.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t2));({void*_tmpF3[1]={& _tmpF5};Cyc_Tcutil_terr(e2->loc,({const char*_tmpF4="expecting int but found %s";_tag_dyneither(_tmpF4,sizeof(char),27);}),_tag_dyneither(_tmpF3,sizeof(void*),1));});});{
void*_tmpF6=((void*(*)(void*y,union Cyc_Absyn_Constraint*x))Cyc_Absyn_conref_constr)((void*)& Cyc_Absyn_DynEither_b_val,_tmp100);void*_tmpF7=_tmpF6;struct Cyc_Absyn_Exp*_tmpFE;if(((struct Cyc_Absyn_DynEither_b_Absyn_Bounds_struct*)_tmpF7)->tag == 0){_LLB7: _LLB8:
 return t1;}else{_LLB9: _tmpFE=((struct Cyc_Absyn_Upper_b_Absyn_Bounds_struct*)_tmpF7)->f1;_LLBA:
# 523
 if(((int(*)(int y,union Cyc_Absyn_Constraint*x))Cyc_Absyn_conref_def)(0,_tmpFF))
({void*_tmpF8=0;Cyc_Tcutil_warn(e1->loc,({const char*_tmpF9="pointer arithmetic on thin, zero-terminated pointer may be expensive.";_tag_dyneither(_tmpF9,sizeof(char),70);}),_tag_dyneither(_tmpF8,sizeof(void*),0));});{
# 531
struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmpFA=({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmpFB=_cycalloc(sizeof(*_tmpFB));_tmpFB[0]=({struct Cyc_Absyn_PointerType_Absyn_Type_struct _tmpFC;_tmpFC.tag=5;_tmpFC.f1=({struct Cyc_Absyn_PtrInfo _tmpFD;_tmpFD.elt_typ=_tmp104;_tmpFD.elt_tq=_tmp103;_tmpFD.ptr_atts=({(_tmpFD.ptr_atts).rgn=_tmp102;(_tmpFD.ptr_atts).nullable=Cyc_Absyn_true_conref;(_tmpFD.ptr_atts).bounds=Cyc_Absyn_bounds_dyneither_conref;(_tmpFD.ptr_atts).zero_term=_tmpFF;(_tmpFD.ptr_atts).ptrloc=0;_tmpFD.ptr_atts;});_tmpFD;});_tmpFC;});_tmpFB;});
# 535
Cyc_Tcutil_unchecked_cast(te,e1,(void*)_tmpFA,Cyc_Absyn_Other_coercion);
return(void*)_tmpFA;};}_LLB6:;};}else{_LLB4: _LLB5:
# 538
 return Cyc_Tcexp_tcArithBinop(te,e1,e2,Cyc_Tcutil_is_numeric);}_LLB1:;}
# 543
static void*Cyc_Tcexp_tcMinus(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
void*t1=(void*)_check_null(e1->topt);
void*t2=(void*)_check_null(e2->topt);
void*t1_elt=(void*)& Cyc_Absyn_VoidType_val;
void*t2_elt=(void*)& Cyc_Absyn_VoidType_val;
if(Cyc_Tcutil_is_tagged_pointer_typ_elt(t1,& t1_elt)){
if(Cyc_Tcutil_is_tagged_pointer_typ_elt(t2,& t2_elt)){
if(!Cyc_Tcutil_unify(t1_elt,t2_elt)){
({struct Cyc_String_pa_PrintArg_struct _tmp108;_tmp108.tag=0;_tmp108.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
# 553
Cyc_Absynpp_typ2string(t2));({struct Cyc_String_pa_PrintArg_struct _tmp107;_tmp107.tag=0;_tmp107.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t1));({void*_tmp105[2]={& _tmp107,& _tmp108};Cyc_Tcutil_terr(e1->loc,({const char*_tmp106="pointer arithmetic on values of different types (%s != %s)";_tag_dyneither(_tmp106,sizeof(char),59);}),_tag_dyneither(_tmp105,sizeof(void*),2));});});});
Cyc_Tcutil_explain_failure();}
# 556
return Cyc_Absyn_sint_typ;}else{
if(Cyc_Tcutil_is_pointer_type(t2)){
if(!Cyc_Tcutil_unify(t1_elt,Cyc_Tcutil_pointer_elt_type(t2))){
({struct Cyc_String_pa_PrintArg_struct _tmp10C;_tmp10C.tag=0;_tmp10C.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
# 561
Cyc_Absynpp_typ2string(t2));({struct Cyc_String_pa_PrintArg_struct _tmp10B;_tmp10B.tag=0;_tmp10B.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t1));({void*_tmp109[2]={& _tmp10B,& _tmp10C};Cyc_Tcutil_terr(e1->loc,({const char*_tmp10A="pointer arithmetic on values of different types (%s != %s)";_tag_dyneither(_tmp10A,sizeof(char),59);}),_tag_dyneither(_tmp109,sizeof(void*),2));});});});
Cyc_Tcutil_explain_failure();}
# 565
({void*_tmp10D=0;Cyc_Tcutil_warn(e1->loc,({const char*_tmp10E="coercing fat pointer to thin pointer to support subtraction";_tag_dyneither(_tmp10E,sizeof(char),60);}),_tag_dyneither(_tmp10D,sizeof(void*),0));});
Cyc_Tcutil_unchecked_cast(te,e1,Cyc_Absyn_star_typ(t1_elt,Cyc_Tcutil_pointer_region(t1),
Cyc_Absyn_empty_tqual(0),Cyc_Absyn_false_conref),Cyc_Absyn_Other_coercion);
# 569
return Cyc_Absyn_sint_typ;}else{
# 571
if(!Cyc_Tcutil_kind_leq(Cyc_Tcutil_typ_kind(t1_elt),& Cyc_Tcutil_tmk))
({void*_tmp10F=0;Cyc_Tcutil_terr(e1->loc,({const char*_tmp110="can't perform arithmetic on abstract pointer type";_tag_dyneither(_tmp110,sizeof(char),50);}),_tag_dyneither(_tmp10F,sizeof(void*),0));});
if(Cyc_Tcutil_is_noalias_pointer(t1,0))
({void*_tmp111=0;Cyc_Tcutil_terr(e1->loc,({const char*_tmp112="can't perform arithmetic on non-aliasing pointer type";_tag_dyneither(_tmp112,sizeof(char),54);}),_tag_dyneither(_tmp111,sizeof(void*),0));});
if(!Cyc_Tcutil_coerce_sint_typ(te,e2)){
({struct Cyc_String_pa_PrintArg_struct _tmp116;_tmp116.tag=0;_tmp116.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string(t2));({struct Cyc_String_pa_PrintArg_struct _tmp115;_tmp115.tag=0;_tmp115.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t1));({void*_tmp113[2]={& _tmp115,& _tmp116};Cyc_Tcutil_terr(e2->loc,({const char*_tmp114="expecting either %s or int but found %s";_tag_dyneither(_tmp114,sizeof(char),40);}),_tag_dyneither(_tmp113,sizeof(void*),2));});});});
Cyc_Tcutil_explain_failure();}
# 580
return t1;}}}
# 584
if(Cyc_Tcutil_is_pointer_type(t1)){
if(Cyc_Tcutil_is_pointer_type(t2) && Cyc_Tcutil_unify(Cyc_Tcutil_pointer_elt_type(t1),
Cyc_Tcutil_pointer_elt_type(t2))){
if(Cyc_Tcutil_is_tagged_pointer_typ(t2)){
({void*_tmp117=0;Cyc_Tcutil_warn(e1->loc,({const char*_tmp118="coercing fat pointer to thin pointer to support subtraction";_tag_dyneither(_tmp118,sizeof(char),60);}),_tag_dyneither(_tmp117,sizeof(void*),0));});
Cyc_Tcutil_unchecked_cast(te,e2,Cyc_Absyn_star_typ(Cyc_Tcutil_pointer_elt_type(t2),
Cyc_Tcutil_pointer_region(t2),
Cyc_Absyn_empty_tqual(0),Cyc_Absyn_false_conref),Cyc_Absyn_Other_coercion);}
# 594
({void*_tmp119=0;Cyc_Tcutil_warn(e1->loc,({const char*_tmp11A="thin pointer subtraction!";_tag_dyneither(_tmp11A,sizeof(char),26);}),_tag_dyneither(_tmp119,sizeof(void*),0));});
return Cyc_Absyn_sint_typ;}
# 597
({void*_tmp11B=0;Cyc_Tcutil_warn(e1->loc,({const char*_tmp11C="coercing thin pointer to integer to support subtraction";_tag_dyneither(_tmp11C,sizeof(char),56);}),_tag_dyneither(_tmp11B,sizeof(void*),0));});
Cyc_Tcutil_unchecked_cast(te,e1,Cyc_Absyn_sint_typ,Cyc_Absyn_Other_coercion);}
# 600
if(Cyc_Tcutil_is_pointer_type(t2)){
({void*_tmp11D=0;Cyc_Tcutil_warn(e1->loc,({const char*_tmp11E="coercing pointer to integer to support subtraction";_tag_dyneither(_tmp11E,sizeof(char),51);}),_tag_dyneither(_tmp11D,sizeof(void*),0));});
Cyc_Tcutil_unchecked_cast(te,e2,Cyc_Absyn_sint_typ,Cyc_Absyn_Other_coercion);}
# 605
return Cyc_Tcexp_tcArithBinop(te,e1,e2,Cyc_Tcutil_is_numeric);}
# 608
static void*Cyc_Tcexp_tcAnyBinop(struct Cyc_Tcenv_Tenv*te,unsigned int loc,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
int e1_is_num=Cyc_Tcutil_is_numeric(e1);
int e2_is_num=Cyc_Tcutil_is_numeric(e2);
void*t1=Cyc_Tcutil_compress((void*)_check_null(e1->topt));
void*t2=Cyc_Tcutil_compress((void*)_check_null(e2->topt));
if(e1_is_num  && e2_is_num)
return Cyc_Absyn_sint_typ;else{
# 616
if((Cyc_Tcutil_typ_kind(t1))->kind == Cyc_Absyn_BoxKind  || 
Cyc_Tcutil_unify(t1,Cyc_Absyn_new_evar(& Cyc_Tcutil_bko,Cyc_Tcenv_lookup_opt_type_vars(te)))){
if(Cyc_Tcutil_unify(t1,t2))
return Cyc_Absyn_sint_typ;else{
# 621
if(Cyc_Tcutil_silent_castable(te,loc,t2,t1)){
Cyc_Tcutil_unchecked_cast(te,e2,t1,Cyc_Absyn_Other_coercion);
return Cyc_Absyn_sint_typ;}else{
if(Cyc_Tcutil_silent_castable(te,loc,t1,t2)){
Cyc_Tcutil_unchecked_cast(te,e1,t2,Cyc_Absyn_Other_coercion);
return Cyc_Absyn_sint_typ;}else{
if(Cyc_Tcutil_zero_to_null(te,t2,e1) || Cyc_Tcutil_zero_to_null(te,t1,e2))
return Cyc_Absyn_sint_typ;else{
goto pointer_cmp;}}}}}else{
# 634
pointer_cmp: {
struct _tuple0 _tmp11F=({struct _tuple0 _tmp123;_tmp123.f1=Cyc_Tcutil_compress(t1);_tmp123.f2=Cyc_Tcutil_compress(t2);_tmp123;});struct _tuple0 _tmp120=_tmp11F;void*_tmp122;void*_tmp121;switch(*((int*)_tmp120.f1)){case 5: if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp120.f2)->tag == 5){_LLBC: _tmp122=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp120.f1)->f1).elt_typ;_tmp121=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp120.f2)->f1).elt_typ;_LLBD:
# 638
 if(Cyc_Tcutil_unify(_tmp122,_tmp121))return Cyc_Absyn_sint_typ;goto _LLBB;}else{goto _LLC0;}case 15: if(((struct Cyc_Absyn_RgnHandleType_Absyn_Type_struct*)_tmp120.f2)->tag == 15){_LLBE: _LLBF:
# 640
 return Cyc_Absyn_sint_typ;}else{goto _LLC0;}default: _LLC0: _LLC1:
 goto _LLBB;}_LLBB:;}
# 643
({struct Cyc_String_pa_PrintArg_struct _tmp127;_tmp127.tag=0;_tmp127.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string(t2));({struct Cyc_String_pa_PrintArg_struct _tmp126;_tmp126.tag=0;_tmp126.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t1));({void*_tmp124[2]={& _tmp126,& _tmp127};Cyc_Tcutil_terr(loc,({const char*_tmp125="comparison not allowed between %s and %s";_tag_dyneither(_tmp125,sizeof(char),41);}),_tag_dyneither(_tmp124,sizeof(void*),2));});});});
Cyc_Tcutil_explain_failure();
return Cyc_Absyn_wildtyp(Cyc_Tcenv_lookup_opt_type_vars(te));}}}
# 650
static void*Cyc_Tcexp_tcEqPrimop(struct Cyc_Tcenv_Tenv*te,unsigned int loc,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
# 652
if(Cyc_Tcexp_tcAnyBinop(te,loc,e1,e2)== Cyc_Absyn_sint_typ)
return Cyc_Absyn_sint_typ;{
# 658
void*t1=Cyc_Tcutil_compress((void*)_check_null(e1->topt));
void*t2=Cyc_Tcutil_compress((void*)_check_null(e2->topt));
{struct _tuple0 _tmp128=({struct _tuple0 _tmp12C;_tmp12C.f1=t1;_tmp12C.f2=t2;_tmp12C;});struct _tuple0 _tmp129=_tmp128;void*_tmp12B;void*_tmp12A;if(((struct Cyc_Absyn_RgnHandleType_Absyn_Type_struct*)_tmp129.f1)->tag == 15){if(((struct Cyc_Absyn_RgnHandleType_Absyn_Type_struct*)_tmp129.f2)->tag == 15){_LLC3: _tmp12B=(void*)((struct Cyc_Absyn_RgnHandleType_Absyn_Type_struct*)_tmp129.f1)->f1;_tmp12A=(void*)((struct Cyc_Absyn_RgnHandleType_Absyn_Type_struct*)_tmp129.f2)->f1;_LLC4:
# 663
 return Cyc_Absyn_sint_typ;}else{goto _LLC5;}}else{_LLC5: _LLC6:
 goto _LLC2;}_LLC2:;}
# 666
({struct Cyc_String_pa_PrintArg_struct _tmp130;_tmp130.tag=0;_tmp130.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string(t2));({struct Cyc_String_pa_PrintArg_struct _tmp12F;_tmp12F.tag=0;_tmp12F.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t1));({void*_tmp12D[2]={& _tmp12F,& _tmp130};Cyc_Tcutil_terr(loc,({const char*_tmp12E="comparison not allowed between %s and %s";_tag_dyneither(_tmp12E,sizeof(char),41);}),_tag_dyneither(_tmp12D,sizeof(void*),2));});});});
Cyc_Tcutil_explain_failure();
return Cyc_Absyn_wildtyp(Cyc_Tcenv_lookup_opt_type_vars(te));};}
# 674
static void*Cyc_Tcexp_tcBinPrimop(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,enum Cyc_Absyn_Primop p,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
# 676
enum Cyc_Absyn_Primop _tmp131=p;switch(_tmp131){case Cyc_Absyn_Plus: _LLC8: _LLC9:
 return Cyc_Tcexp_tcPlus(te,e1,e2);case Cyc_Absyn_Minus: _LLCA: _LLCB:
 return Cyc_Tcexp_tcMinus(te,e1,e2);case Cyc_Absyn_Times: _LLCC: _LLCD:
# 680
 goto _LLCF;case Cyc_Absyn_Div: _LLCE: _LLCF:
 return Cyc_Tcexp_tcArithBinop(te,e1,e2,Cyc_Tcutil_is_numeric);case Cyc_Absyn_Mod: _LLD0: _LLD1:
# 683
 goto _LLD3;case Cyc_Absyn_Bitand: _LLD2: _LLD3:
 goto _LLD5;case Cyc_Absyn_Bitor: _LLD4: _LLD5:
 goto _LLD7;case Cyc_Absyn_Bitxor: _LLD6: _LLD7:
 goto _LLD9;case Cyc_Absyn_Bitlshift: _LLD8: _LLD9:
 goto _LLDB;case Cyc_Absyn_Bitlrshift: _LLDA: _LLDB:
 goto _LLDD;case Cyc_Absyn_Bitarshift: _LLDC: _LLDD:
 return Cyc_Tcexp_tcArithBinop(te,e1,e2,Cyc_Tcutil_is_integral);case Cyc_Absyn_Eq: _LLDE: _LLDF:
# 693
 goto _LLE1;case Cyc_Absyn_Neq: _LLE0: _LLE1:
 return Cyc_Tcexp_tcEqPrimop(te,loc,e1,e2);case Cyc_Absyn_Gt: _LLE2: _LLE3:
# 696
 goto _LLE5;case Cyc_Absyn_Lt: _LLE4: _LLE5:
 goto _LLE7;case Cyc_Absyn_Gte: _LLE6: _LLE7:
 goto _LLE9;case Cyc_Absyn_Lte: _LLE8: _LLE9:
 return Cyc_Tcexp_tcAnyBinop(te,loc,e1,e2);default: _LLEA: _LLEB:
# 701
({void*_tmp132=0;((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(({const char*_tmp133="bad binary primop";_tag_dyneither(_tmp133,sizeof(char),18);}),_tag_dyneither(_tmp132,sizeof(void*),0));});}_LLC7:;}
# 705
static void*Cyc_Tcexp_tcPrimop(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,enum Cyc_Absyn_Primop p,struct Cyc_List_List*es){
# 713
if(p == Cyc_Absyn_Minus  && ((int(*)(struct Cyc_List_List*x))Cyc_List_length)(es)== 1){
struct Cyc_Absyn_Exp*_tmp134=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd;
void*_tmp135=Cyc_Tcexp_tcExp(te,topt,_tmp134);
if(!Cyc_Tcutil_is_numeric(_tmp134))
({struct Cyc_String_pa_PrintArg_struct _tmp138;_tmp138.tag=0;_tmp138.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(_tmp135));({void*_tmp136[1]={& _tmp138};Cyc_Tcutil_terr(_tmp134->loc,({const char*_tmp137="expecting numeric type but found %s";_tag_dyneither(_tmp137,sizeof(char),36);}),_tag_dyneither(_tmp136,sizeof(void*),1));});});
return _tmp135;}
# 720
Cyc_Tcexp_tcExpList(te,es);{
void*t;
{int _tmp139=((int(*)(struct Cyc_List_List*x))Cyc_List_length)(es);int _tmp13A=_tmp139;switch(_tmp13A){case 0: _LLED: _LLEE:
 return({void*_tmp13B=0;Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp13C="primitive operator has 0 arguments";_tag_dyneither(_tmp13C,sizeof(char),35);}),_tag_dyneither(_tmp13B,sizeof(void*),0));});case 1: _LLEF: _LLF0:
 t=Cyc_Tcexp_tcUnPrimop(te,loc,topt,p,(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd);goto _LLEC;case 2: _LLF1: _LLF2:
 t=Cyc_Tcexp_tcBinPrimop(te,loc,topt,p,(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd,(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(((struct Cyc_List_List*)_check_null(es))->tl))->hd);goto _LLEC;default: _LLF3: _LLF4:
 return({void*_tmp13D=0;Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp13E="primitive operator has > 2 arguments";_tag_dyneither(_tmp13E,sizeof(char),37);}),_tag_dyneither(_tmp13D,sizeof(void*),0));});}_LLEC:;}
# 728
return t;};}struct _tuple18{struct Cyc_Absyn_Tqual f1;void*f2;};
# 731
static int Cyc_Tcexp_check_writable_aggr(unsigned int loc,void*t){
void*_tmp13F=Cyc_Tcutil_compress(t);
void*_tmp140=_tmp13F;struct Cyc_List_List*_tmp15C;void*_tmp15B;struct Cyc_Absyn_Tqual _tmp15A;struct Cyc_Absyn_Datatypefield*_tmp159;struct Cyc_List_List*_tmp158;struct Cyc_Absyn_Aggrdecl*_tmp157;switch(*((int*)_tmp140)){case 11: if((((((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp140)->f1).aggr_info).KnownAggr).tag == 2){_LLF6: _tmp157=*(((((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp140)->f1).aggr_info).KnownAggr).val;_LLF7:
# 735
 if(_tmp157->impl == 0){
({void*_tmp141=0;Cyc_Tcutil_terr(loc,({const char*_tmp142="attempt to write an abstract aggregate";_tag_dyneither(_tmp142,sizeof(char),39);}),_tag_dyneither(_tmp141,sizeof(void*),0));});
return 0;}else{
# 739
_tmp158=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp157->impl))->fields;goto _LLF9;}}else{goto _LL100;}case 12: _LLF8: _tmp158=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp140)->f2;_LLF9:
# 741
 for(0;_tmp158 != 0;_tmp158=_tmp158->tl){
struct Cyc_Absyn_Aggrfield*_tmp143=(struct Cyc_Absyn_Aggrfield*)_tmp158->hd;
if((_tmp143->tq).real_const){
({struct Cyc_String_pa_PrintArg_struct _tmp146;_tmp146.tag=0;_tmp146.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*_tmp143->name);({void*_tmp144[1]={& _tmp146};Cyc_Tcutil_terr(loc,({const char*_tmp145="attempt to over-write an aggregate with const member %s";_tag_dyneither(_tmp145,sizeof(char),56);}),_tag_dyneither(_tmp144,sizeof(void*),1));});});
return 0;}
# 747
if(!Cyc_Tcexp_check_writable_aggr(loc,_tmp143->type))return 0;}
# 749
return 1;case 4: if((((((struct Cyc_Absyn_DatatypeFieldType_Absyn_Type_struct*)_tmp140)->f1).field_info).KnownDatatypefield).tag == 2){_LLFA: _tmp159=((((((struct Cyc_Absyn_DatatypeFieldType_Absyn_Type_struct*)_tmp140)->f1).field_info).KnownDatatypefield).val).f2;_LLFB:
# 751
{struct Cyc_List_List*_tmp147=_tmp159->typs;for(0;_tmp147 != 0;_tmp147=_tmp147->tl){
struct _tuple18*_tmp148=(struct _tuple18*)_tmp147->hd;struct _tuple18*_tmp149=_tmp148;struct Cyc_Absyn_Tqual _tmp14E;void*_tmp14D;_LL103: _tmp14E=_tmp149->f1;_tmp14D=_tmp149->f2;_LL104:;
if(_tmp14E.real_const){
({struct Cyc_String_pa_PrintArg_struct _tmp14C;_tmp14C.tag=0;_tmp14C.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(_tmp159->name));({void*_tmp14A[1]={& _tmp14C};Cyc_Tcutil_terr(loc,({const char*_tmp14B="attempt to over-write a datatype field (%s) with a const member";_tag_dyneither(_tmp14B,sizeof(char),64);}),_tag_dyneither(_tmp14A,sizeof(void*),1));});});
return 0;}
# 757
if(!Cyc_Tcexp_check_writable_aggr(loc,_tmp14D))return 0;}}
# 759
return 1;}else{goto _LL100;}case 8: _LLFC: _tmp15B=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp140)->f1).elt_type;_tmp15A=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp140)->f1).tq;_LLFD:
# 761
 if(_tmp15A.real_const){
({void*_tmp14F=0;Cyc_Tcutil_terr(loc,({const char*_tmp150="attempt to over-write a const array";_tag_dyneither(_tmp150,sizeof(char),36);}),_tag_dyneither(_tmp14F,sizeof(void*),0));});
return 0;}
# 765
return Cyc_Tcexp_check_writable_aggr(loc,_tmp15B);case 10: _LLFE: _tmp15C=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp140)->f1;_LLFF:
# 767
 for(0;_tmp15C != 0;_tmp15C=_tmp15C->tl){
struct _tuple18*_tmp151=(struct _tuple18*)_tmp15C->hd;struct _tuple18*_tmp152=_tmp151;struct Cyc_Absyn_Tqual _tmp156;void*_tmp155;_LL106: _tmp156=_tmp152->f1;_tmp155=_tmp152->f2;_LL107:;
if(_tmp156.real_const){
({void*_tmp153=0;Cyc_Tcutil_terr(loc,({const char*_tmp154="attempt to over-write a tuple field with a const member";_tag_dyneither(_tmp154,sizeof(char),56);}),_tag_dyneither(_tmp153,sizeof(void*),0));});
return 0;}
# 773
if(!Cyc_Tcexp_check_writable_aggr(loc,_tmp155))return 0;}
# 775
return 1;default: _LL100: _LL101:
 return 1;}_LLF5:;}
# 783
static void Cyc_Tcexp_check_writable(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e){
# 786
if(!Cyc_Tcexp_check_writable_aggr(e->loc,(void*)_check_null(e->topt)))return;
{void*_tmp15D=e->r;void*_tmp15E=_tmp15D;struct Cyc_Absyn_Exp*_tmp18D;struct Cyc_Absyn_Exp*_tmp18C;struct Cyc_Absyn_Exp*_tmp18B;struct Cyc_Absyn_Exp*_tmp18A;struct _dyneither_ptr*_tmp189;struct Cyc_Absyn_Exp*_tmp188;struct _dyneither_ptr*_tmp187;struct Cyc_Absyn_Exp*_tmp186;struct Cyc_Absyn_Exp*_tmp185;struct Cyc_Absyn_Vardecl*_tmp184;struct Cyc_Absyn_Vardecl*_tmp183;struct Cyc_Absyn_Vardecl*_tmp182;struct Cyc_Absyn_Vardecl*_tmp181;switch(*((int*)_tmp15E)){case 1: switch(*((int*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp15E)->f1)){case 3: _LL109: _tmp181=((struct Cyc_Absyn_Param_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp15E)->f1)->f1;_LL10A:
 _tmp182=_tmp181;goto _LL10C;case 4: _LL10B: _tmp182=((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp15E)->f1)->f1;_LL10C:
 _tmp183=_tmp182;goto _LL10E;case 5: _LL10D: _tmp183=((struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp15E)->f1)->f1;_LL10E:
 _tmp184=_tmp183;goto _LL110;case 1: _LL10F: _tmp184=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp15E)->f1)->f1;_LL110:
 if(!(_tmp184->tq).real_const)return;goto _LL108;default: goto _LL11D;}case 22: _LL111: _tmp186=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp15E)->f1;_tmp185=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp15E)->f2;_LL112:
# 793
{void*_tmp15F=Cyc_Tcutil_compress((void*)_check_null(_tmp186->topt));void*_tmp160=_tmp15F;struct Cyc_List_List*_tmp171;struct Cyc_Absyn_Tqual _tmp170;struct Cyc_Absyn_Tqual _tmp16F;switch(*((int*)_tmp160)){case 5: _LL120: _tmp16F=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp160)->f1).elt_tq;_LL121:
 _tmp170=_tmp16F;goto _LL123;case 8: _LL122: _tmp170=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp160)->f1).tq;_LL123:
 if(!_tmp170.real_const)return;goto _LL11F;case 10: _LL124: _tmp171=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp160)->f1;_LL125: {
# 797
struct _tuple15 _tmp161=Cyc_Evexp_eval_const_uint_exp(_tmp185);struct _tuple15 _tmp162=_tmp161;unsigned int _tmp16E;int _tmp16D;_LL129: _tmp16E=_tmp162.f1;_tmp16D=_tmp162.f2;_LL12A:;
if(!_tmp16D){
({void*_tmp163=0;Cyc_Tcutil_terr(e->loc,({const char*_tmp164="tuple projection cannot use sizeof or offsetof";_tag_dyneither(_tmp164,sizeof(char),47);}),_tag_dyneither(_tmp163,sizeof(void*),0));});
return;}
# 802
{struct _handler_cons _tmp165;_push_handler(& _tmp165);{int _tmp167=0;if(setjmp(_tmp165.handler))_tmp167=1;if(!_tmp167){
{struct _tuple18*_tmp168=((struct _tuple18*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth)(_tmp171,(int)_tmp16E);struct _tuple18*_tmp169=_tmp168;struct Cyc_Absyn_Tqual _tmp16A;_LL12C: _tmp16A=_tmp169->f1;_LL12D:;
if(!_tmp16A.real_const){_npop_handler(0);return;}}
# 803
;_pop_handler();}else{void*_tmp166=(void*)_exn_thrown;void*_tmp16B=_tmp166;void*_tmp16C;if(((struct Cyc_List_Nth_exn_struct*)_tmp16B)->tag == Cyc_List_Nth){_LL12F: _LL130:
# 805
 return;}else{_LL131: _tmp16C=_tmp16B;_LL132:(int)_rethrow(_tmp16C);}_LL12E:;}};}
goto _LL11F;}default: _LL126: _LL127:
 goto _LL11F;}_LL11F:;}
# 809
goto _LL108;case 20: _LL113: _tmp188=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp15E)->f1;_tmp187=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp15E)->f2;_LL114:
# 811
{void*_tmp172=Cyc_Tcutil_compress((void*)_check_null(_tmp188->topt));void*_tmp173=_tmp172;struct Cyc_List_List*_tmp175;struct Cyc_Absyn_Aggrdecl**_tmp174;switch(*((int*)_tmp173)){case 11: if((((((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp173)->f1).aggr_info).KnownAggr).tag == 2){_LL134: _tmp174=(((((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp173)->f1).aggr_info).KnownAggr).val;_LL135: {
# 813
struct Cyc_Absyn_Aggrfield*sf=
_tmp174 == 0?0: Cyc_Absyn_lookup_decl_field(*_tmp174,_tmp187);
if(sf == 0  || !(sf->tq).real_const)return;
goto _LL133;}}else{goto _LL138;}case 12: _LL136: _tmp175=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp173)->f2;_LL137: {
# 818
struct Cyc_Absyn_Aggrfield*sf=Cyc_Absyn_lookup_field(_tmp175,_tmp187);
if(sf == 0  || !(sf->tq).real_const)return;
goto _LL133;}default: _LL138: _LL139:
 goto _LL133;}_LL133:;}
# 823
goto _LL108;case 21: _LL115: _tmp18A=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp15E)->f1;_tmp189=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp15E)->f2;_LL116:
# 825
{void*_tmp176=Cyc_Tcutil_compress((void*)_check_null(_tmp18A->topt));void*_tmp177=_tmp176;void*_tmp17C;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp177)->tag == 5){_LL13B: _tmp17C=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp177)->f1).elt_typ;_LL13C:
# 827
{void*_tmp178=Cyc_Tcutil_compress(_tmp17C);void*_tmp179=_tmp178;struct Cyc_List_List*_tmp17B;struct Cyc_Absyn_Aggrdecl**_tmp17A;switch(*((int*)_tmp179)){case 11: if((((((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp179)->f1).aggr_info).KnownAggr).tag == 2){_LL140: _tmp17A=(((((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp179)->f1).aggr_info).KnownAggr).val;_LL141: {
# 829
struct Cyc_Absyn_Aggrfield*sf=
_tmp17A == 0?0: Cyc_Absyn_lookup_decl_field(*_tmp17A,_tmp189);
if(sf == 0  || !(sf->tq).real_const)return;
goto _LL13F;}}else{goto _LL144;}case 12: _LL142: _tmp17B=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp179)->f2;_LL143: {
# 834
struct Cyc_Absyn_Aggrfield*sf=Cyc_Absyn_lookup_field(_tmp17B,_tmp189);
if(sf == 0  || !(sf->tq).real_const)return;
goto _LL13F;}default: _LL144: _LL145:
 goto _LL13F;}_LL13F:;}
# 839
goto _LL13A;}else{_LL13D: _LL13E:
 goto _LL13A;}_LL13A:;}
# 842
goto _LL108;case 19: _LL117: _tmp18B=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp15E)->f1;_LL118:
# 844
{void*_tmp17D=Cyc_Tcutil_compress((void*)_check_null(_tmp18B->topt));void*_tmp17E=_tmp17D;struct Cyc_Absyn_Tqual _tmp180;struct Cyc_Absyn_Tqual _tmp17F;switch(*((int*)_tmp17E)){case 5: _LL147: _tmp17F=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp17E)->f1).elt_tq;_LL148:
 _tmp180=_tmp17F;goto _LL14A;case 8: _LL149: _tmp180=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp17E)->f1).tq;_LL14A:
 if(!_tmp180.real_const)return;goto _LL146;default: _LL14B: _LL14C:
 goto _LL146;}_LL146:;}
# 849
goto _LL108;case 11: _LL119: _tmp18C=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp15E)->f1;_LL11A:
 _tmp18D=_tmp18C;goto _LL11C;case 12: _LL11B: _tmp18D=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp15E)->f1;_LL11C:
 Cyc_Tcexp_check_writable(te,_tmp18D);return;default: _LL11D: _LL11E:
 goto _LL108;}_LL108:;}
# 854
({struct Cyc_String_pa_PrintArg_struct _tmp190;_tmp190.tag=0;_tmp190.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_exp2string(e));({void*_tmp18E[1]={& _tmp190};Cyc_Tcutil_terr(e->loc,({const char*_tmp18F="attempt to write a const location: %s";_tag_dyneither(_tmp18F,sizeof(char),38);}),_tag_dyneither(_tmp18E,sizeof(void*),1));});});}
# 857
static void*Cyc_Tcexp_tcIncrement(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct Cyc_Absyn_Exp*e,enum Cyc_Absyn_Incrementor i){
# 860
struct _RegionHandle _tmp191=_new_region("temp");struct _RegionHandle*temp=& _tmp191;_push_region(temp);
Cyc_Tcexp_tcExpNoPromote(Cyc_Tcenv_enter_lhs(temp,te),0,e);
# 863
if(!Cyc_Absyn_is_lvalue(e)){
void*_tmp194=({void*_tmp192=0;Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp193="increment/decrement of non-lvalue";_tag_dyneither(_tmp193,sizeof(char),34);}),_tag_dyneither(_tmp192,sizeof(void*),0));});_npop_handler(0);return _tmp194;}
Cyc_Tcexp_check_writable(te,e);{
void*t=(void*)_check_null(e->topt);
# 868
if(!Cyc_Tcutil_is_numeric(e)){
void*telt=(void*)& Cyc_Absyn_VoidType_val;
if(Cyc_Tcutil_is_tagged_pointer_typ_elt(t,& telt) || 
Cyc_Tcutil_is_zero_pointer_typ_elt(t,& telt) && (i == Cyc_Absyn_PreInc  || i == Cyc_Absyn_PostInc)){
if(!Cyc_Tcutil_kind_leq(Cyc_Tcutil_typ_kind(telt),& Cyc_Tcutil_tmk))
({void*_tmp195=0;Cyc_Tcutil_terr(e->loc,({const char*_tmp196="can't perform arithmetic on abstract pointer type";_tag_dyneither(_tmp196,sizeof(char),50);}),_tag_dyneither(_tmp195,sizeof(void*),0));});
if(Cyc_Tcutil_is_noalias_pointer(t,0))
({void*_tmp197=0;Cyc_Tcutil_terr(e->loc,({const char*_tmp198="can't perform arithmetic on non-aliasing pointer type";_tag_dyneither(_tmp198,sizeof(char),54);}),_tag_dyneither(_tmp197,sizeof(void*),0));});}else{
# 877
({struct Cyc_String_pa_PrintArg_struct _tmp19B;_tmp19B.tag=0;_tmp19B.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t));({void*_tmp199[1]={& _tmp19B};Cyc_Tcutil_terr(e->loc,({const char*_tmp19A="expecting arithmetic or ? type but found %s";_tag_dyneither(_tmp19A,sizeof(char),44);}),_tag_dyneither(_tmp199,sizeof(void*),1));});});}}{
# 879
void*_tmp19C=t;_npop_handler(0);return _tmp19C;};};
# 860
;_pop_region(temp);}
# 883
static void*Cyc_Tcexp_tcConditional(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,struct Cyc_Absyn_Exp*e3){
# 885
struct _RegionHandle _tmp19D=_new_region("r");struct _RegionHandle*r=& _tmp19D;_push_region(r);
Cyc_Tcexp_tcTest(Cyc_Tcenv_clear_abstract_val_ok(r,te),e1,({const char*_tmp19E="conditional expression";_tag_dyneither(_tmp19E,sizeof(char),23);}));
# 888
Cyc_Tcexp_tcExp(te,topt,e2);
Cyc_Tcexp_tcExp(te,topt,e3);{
void*t;
if(Cyc_Tcenv_abstract_val_ok(te))
t=Cyc_Absyn_new_evar(& Cyc_Tcutil_tako,Cyc_Tcenv_lookup_opt_type_vars(te));else{
# 894
t=Cyc_Absyn_new_evar(& Cyc_Tcutil_tmko,Cyc_Tcenv_lookup_opt_type_vars(te));}{
struct Cyc_List_List _tmp19F=({struct Cyc_List_List _tmp1A7;_tmp1A7.hd=e3;_tmp1A7.tl=0;_tmp1A7;});
struct Cyc_List_List _tmp1A0=({struct Cyc_List_List _tmp1A6;_tmp1A6.hd=e2;_tmp1A6.tl=& _tmp19F;_tmp1A6;});
if(!Cyc_Tcutil_coerce_list(te,t,& _tmp1A0)){
({struct Cyc_String_pa_PrintArg_struct _tmp1A4;_tmp1A4.tag=0;_tmp1A4.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string((void*)_check_null(e3->topt)));({struct Cyc_String_pa_PrintArg_struct _tmp1A3;_tmp1A3.tag=0;_tmp1A3.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string((void*)_check_null(e2->topt)));({void*_tmp1A1[2]={& _tmp1A3,& _tmp1A4};Cyc_Tcutil_terr(loc,({const char*_tmp1A2="conditional clause types do not match: %s != %s";_tag_dyneither(_tmp1A2,sizeof(char),48);}),_tag_dyneither(_tmp1A1,sizeof(void*),2));});});});
Cyc_Tcutil_explain_failure();}{
# 902
void*_tmp1A5=t;_npop_handler(0);return _tmp1A5;};};};
# 885
;_pop_region(r);}
# 906
static void*Cyc_Tcexp_tcAnd(struct Cyc_Tcenv_Tenv*te,unsigned int loc,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
# 908
Cyc_Tcexp_tcTest(te,e1,({const char*_tmp1A8="logical-and expression";_tag_dyneither(_tmp1A8,sizeof(char),23);}));
Cyc_Tcexp_tcTest(te,e2,({const char*_tmp1A9="logical-and expression";_tag_dyneither(_tmp1A9,sizeof(char),23);}));
return Cyc_Absyn_sint_typ;}
# 914
static void*Cyc_Tcexp_tcOr(struct Cyc_Tcenv_Tenv*te,unsigned int loc,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
# 916
Cyc_Tcexp_tcTest(te,e1,({const char*_tmp1AA="logical-or expression";_tag_dyneither(_tmp1AA,sizeof(char),22);}));
Cyc_Tcexp_tcTest(te,e2,({const char*_tmp1AB="logical-or expression";_tag_dyneither(_tmp1AB,sizeof(char),22);}));
return Cyc_Absyn_sint_typ;}
# 922
static void*Cyc_Tcexp_tcAssignOp(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct Cyc_Absyn_Exp*e1,struct Cyc_Core_Opt*po,struct Cyc_Absyn_Exp*e2){
# 929
struct _RegionHandle _tmp1AC=_new_region("r");struct _RegionHandle*r=& _tmp1AC;_push_region(r);
Cyc_Tcexp_tcExpNoPromote(Cyc_Tcenv_enter_lhs(r,Cyc_Tcenv_enter_notreadctxt(r,te)),0,e1);{
# 932
void*t1=(void*)_check_null(e1->topt);
Cyc_Tcexp_tcExp(te,& t1,e2);{
void*t2=(void*)_check_null(e2->topt);
# 936
{void*_tmp1AD=Cyc_Tcutil_compress(t1);void*_tmp1AE=_tmp1AD;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp1AE)->tag == 8){_LL14E: _LL14F:
({void*_tmp1AF=0;Cyc_Tcutil_terr(loc,({const char*_tmp1B0="cannot assign to an array";_tag_dyneither(_tmp1B0,sizeof(char),26);}),_tag_dyneither(_tmp1AF,sizeof(void*),0));});goto _LL14D;}else{_LL150: _LL151:
 goto _LL14D;}_LL14D:;}
# 941
if(!Cyc_Tcutil_kind_leq(Cyc_Tcutil_typ_kind(t1),& Cyc_Tcutil_tmk))
({void*_tmp1B1=0;Cyc_Tcutil_terr(loc,({const char*_tmp1B2="type is abstract (can't determine size).";_tag_dyneither(_tmp1B2,sizeof(char),41);}),_tag_dyneither(_tmp1B1,sizeof(void*),0));});
# 945
if(!Cyc_Absyn_is_lvalue(e1)){
void*_tmp1B5=({void*_tmp1B3=0;Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp1B4="assignment to non-lvalue";_tag_dyneither(_tmp1B4,sizeof(char),25);}),_tag_dyneither(_tmp1B3,sizeof(void*),0));});_npop_handler(0);return _tmp1B5;}
Cyc_Tcexp_check_writable(te,e1);
if(po == 0){
struct _RegionHandle _tmp1B6=_new_region("temp");struct _RegionHandle*temp=& _tmp1B6;_push_region(temp);
if(Cyc_Tcutil_is_noalias_pointer_or_aggr(temp,t2) && !Cyc_Tcutil_is_noalias_path(temp,e2))
({void*_tmp1B7=0;Cyc_Tcutil_terr(e2->loc,({const char*_tmp1B8="Cannot consume non-unique paths; do swap instead";_tag_dyneither(_tmp1B8,sizeof(char),49);}),_tag_dyneither(_tmp1B7,sizeof(void*),0));});
# 953
if(!Cyc_Tcutil_coerce_assign(te,e2,t1)){
void*_tmp1B9=({struct Cyc_String_pa_PrintArg_struct _tmp1BE;_tmp1BE.tag=0;_tmp1BE.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string(t2));({struct Cyc_String_pa_PrintArg_struct _tmp1BD;_tmp1BD.tag=0;_tmp1BD.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t1));({void*_tmp1BB[2]={& _tmp1BD,& _tmp1BE};Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp1BC="type mismatch: %s != %s";_tag_dyneither(_tmp1BC,sizeof(char),24);}),_tag_dyneither(_tmp1BB,sizeof(void*),2));});});});
Cyc_Tcutil_unify(t1,t2);
Cyc_Tcutil_explain_failure();{
void*_tmp1BA=_tmp1B9;_npop_handler(1);return _tmp1BA;};}
# 949
;_pop_region(temp);}else{
# 961
enum Cyc_Absyn_Primop _tmp1BF=(enum Cyc_Absyn_Primop)po->v;
void*_tmp1C0=Cyc_Tcexp_tcBinPrimop(te,loc,0,_tmp1BF,e1,e2);
if(!(Cyc_Tcutil_unify(_tmp1C0,t1) || Cyc_Tcutil_coerceable(_tmp1C0) && Cyc_Tcutil_coerceable(t1))){
void*_tmp1C1=({struct Cyc_String_pa_PrintArg_struct _tmp1C6;_tmp1C6.tag=0;_tmp1C6.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
# 967
Cyc_Absynpp_typ2string(t2));({struct Cyc_String_pa_PrintArg_struct _tmp1C5;_tmp1C5.tag=0;_tmp1C5.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
# 966
Cyc_Absynpp_typ2string(t1));({void*_tmp1C3[2]={& _tmp1C5,& _tmp1C6};Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp1C4="Cannot use this operator in an assignment when the arguments have types %s and %s";_tag_dyneither(_tmp1C4,sizeof(char),82);}),_tag_dyneither(_tmp1C3,sizeof(void*),2));});});});
# 968
Cyc_Tcutil_unify(_tmp1C0,t1);
Cyc_Tcutil_explain_failure();{
void*_tmp1C2=_tmp1C1;_npop_handler(0);return _tmp1C2;};}{
# 972
void*_tmp1C7=_tmp1C0;_npop_handler(0);return _tmp1C7;};}{
# 974
void*_tmp1C8=t1;_npop_handler(0);return _tmp1C8;};};};
# 929
;_pop_region(r);}
# 978
static void*Cyc_Tcexp_tcSeqExp(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
struct _RegionHandle _tmp1C9=_new_region("r");struct _RegionHandle*r=& _tmp1C9;_push_region(r);
Cyc_Tcexp_tcExp(Cyc_Tcenv_clear_abstract_val_ok(r,te),0,e1);
Cyc_Tcexp_tcExp(Cyc_Tcenv_clear_abstract_val_ok(r,te),topt,e2);{
# 983
void*_tmp1CA=(void*)_check_null(e2->topt);_npop_handler(0);return _tmp1CA;};
# 979
;_pop_region(r);}
# 987
static struct Cyc_Absyn_Datatypefield*Cyc_Tcexp_tcInjection(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e,void*tu,struct _RegionHandle*r,struct Cyc_List_List*inst,struct Cyc_List_List*fs){
# 991
struct Cyc_List_List*fields;
void*t1=(void*)_check_null(e->topt);
# 994
{void*_tmp1CB=Cyc_Tcutil_compress(t1);void*_tmp1CC=_tmp1CB;switch(*((int*)_tmp1CC)){case 7: if(((struct Cyc_Absyn_FloatType_Absyn_Type_struct*)_tmp1CC)->f1 == 0){_LL153: _LL154:
 Cyc_Tcutil_unchecked_cast(te,e,Cyc_Absyn_float_typ(1),Cyc_Absyn_No_coercion);t1=Cyc_Absyn_float_typ(1);goto _LL152;}else{goto _LL159;}case 6: switch(((struct Cyc_Absyn_IntType_Absyn_Type_struct*)_tmp1CC)->f2){case Cyc_Absyn_Char_sz: _LL155: _LL156:
 goto _LL158;case Cyc_Absyn_Short_sz: _LL157: _LL158:
 Cyc_Tcutil_unchecked_cast(te,e,Cyc_Absyn_sint_typ,Cyc_Absyn_No_coercion);t1=Cyc_Absyn_sint_typ;goto _LL152;default: goto _LL159;}default: _LL159: _LL15A:
 goto _LL152;}_LL152:;}
# 1001
for(fields=fs;fields != 0;fields=fields->tl){
struct Cyc_Absyn_Datatypefield _tmp1CD=*((struct Cyc_Absyn_Datatypefield*)fields->hd);struct Cyc_Absyn_Datatypefield _tmp1CE=_tmp1CD;struct _tuple1*_tmp1D2;struct Cyc_List_List*_tmp1D1;unsigned int _tmp1D0;enum Cyc_Absyn_Scope _tmp1CF;_LL15C: _tmp1D2=_tmp1CE.name;_tmp1D1=_tmp1CE.typs;_tmp1D0=_tmp1CE.loc;_tmp1CF=_tmp1CE.sc;_LL15D:;
# 1004
if(_tmp1D1 == 0  || _tmp1D1->tl != 0)continue;{
void*t2=Cyc_Tcutil_rsubstitute(r,inst,(*((struct _tuple18*)_tmp1D1->hd)).f2);
# 1007
if(Cyc_Tcutil_unify(t1,t2))
return(struct Cyc_Absyn_Datatypefield*)fields->hd;};}
# 1012
for(fields=fs;fields != 0;fields=fields->tl){
struct Cyc_Absyn_Datatypefield _tmp1D3=*((struct Cyc_Absyn_Datatypefield*)fields->hd);struct Cyc_Absyn_Datatypefield _tmp1D4=_tmp1D3;struct _tuple1*_tmp1D8;struct Cyc_List_List*_tmp1D7;unsigned int _tmp1D6;enum Cyc_Absyn_Scope _tmp1D5;_LL15F: _tmp1D8=_tmp1D4.name;_tmp1D7=_tmp1D4.typs;_tmp1D6=_tmp1D4.loc;_tmp1D5=_tmp1D4.sc;_LL160:;
# 1015
if(_tmp1D7 == 0  || _tmp1D7->tl != 0)continue;{
void*t2=Cyc_Tcutil_rsubstitute(r,inst,(*((struct _tuple18*)_tmp1D7->hd)).f2);
# 1019
int bogus=0;
if(Cyc_Tcutil_coerce_arg(te,e,t2,& bogus))
return(struct Cyc_Absyn_Datatypefield*)fields->hd;};}
# 1024
({struct Cyc_String_pa_PrintArg_struct _tmp1DC;_tmp1DC.tag=0;_tmp1DC.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string(t1));({struct Cyc_String_pa_PrintArg_struct _tmp1DB;_tmp1DB.tag=0;_tmp1DB.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(tu));({void*_tmp1D9[2]={& _tmp1DB,& _tmp1DC};Cyc_Tcutil_terr(e->loc,({const char*_tmp1DA="can't find a field in %s to inject a value of type %s";_tag_dyneither(_tmp1DA,sizeof(char),54);}),_tag_dyneither(_tmp1D9,sizeof(void*),2));});});});
return 0;}
# 1030
static void*Cyc_Tcexp_tcFnCall(struct Cyc_Tcenv_Tenv*te_orig,unsigned int loc,void**topt,struct Cyc_Absyn_Exp*e,struct Cyc_List_List*args,struct Cyc_Absyn_VarargCallInfo**vararg_call_info,struct Cyc_List_List**alias_arg_exps){
# 1036
struct Cyc_List_List*_tmp1DD=args;
int _tmp1DE=0;
struct _RegionHandle _tmp1DF=_new_region("ter");struct _RegionHandle*ter=& _tmp1DF;_push_region(ter);
{struct Cyc_Tcenv_Tenv*_tmp1E0=Cyc_Tcenv_new_block(ter,loc,te_orig);
struct Cyc_Tcenv_Tenv*_tmp1E1=Cyc_Tcenv_clear_abstract_val_ok(ter,_tmp1E0);
Cyc_Tcexp_tcExp(_tmp1E1,0,e);{
void*t=Cyc_Tcutil_compress((void*)_check_null(e->topt));
# 1046
void*_tmp1E2=t;void*_tmp24A;struct Cyc_Absyn_Tqual _tmp249;void*_tmp248;union Cyc_Absyn_Constraint*_tmp247;union Cyc_Absyn_Constraint*_tmp246;union Cyc_Absyn_Constraint*_tmp245;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1E2)->tag == 5){_LL162: _tmp24A=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1E2)->f1).elt_typ;_tmp249=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1E2)->f1).elt_tq;_tmp248=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1E2)->f1).ptr_atts).rgn;_tmp247=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1E2)->f1).ptr_atts).nullable;_tmp246=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1E2)->f1).ptr_atts).bounds;_tmp245=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1E2)->f1).ptr_atts).zero_term;_LL163:
# 1051
 Cyc_Tcenv_check_rgn_accessible(_tmp1E1,loc,_tmp248);
# 1053
Cyc_Tcutil_check_nonzero_bound(loc,_tmp246);{
void*_tmp1E3=Cyc_Tcutil_compress(_tmp24A);void*_tmp1E4=_tmp1E3;struct Cyc_List_List*_tmp240;void*_tmp23F;struct Cyc_Absyn_Tqual _tmp23E;void*_tmp23D;struct Cyc_List_List*_tmp23C;int _tmp23B;struct Cyc_Absyn_VarargInfo*_tmp23A;struct Cyc_List_List*_tmp239;struct Cyc_List_List*_tmp238;struct Cyc_Absyn_Exp*_tmp237;struct Cyc_List_List*_tmp236;struct Cyc_Absyn_Exp*_tmp235;struct Cyc_List_List*_tmp234;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1E4)->tag == 9){_LL167: _tmp240=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1E4)->f1).tvars;_tmp23F=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1E4)->f1).effect;_tmp23E=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1E4)->f1).ret_tqual;_tmp23D=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1E4)->f1).ret_typ;_tmp23C=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1E4)->f1).args;_tmp23B=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1E4)->f1).c_varargs;_tmp23A=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1E4)->f1).cyc_varargs;_tmp239=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1E4)->f1).rgn_po;_tmp238=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1E4)->f1).attributes;_tmp237=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1E4)->f1).requires_clause;_tmp236=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1E4)->f1).requires_relns;_tmp235=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1E4)->f1).ensures_clause;_tmp234=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1E4)->f1).ensures_relns;_LL168:
# 1058
 if(_tmp240 != 0  || _tmp239 != 0)
({void*_tmp1E5=0;Cyc_Tcutil_terr(e->loc,({const char*_tmp1E6="function should have been instantiated prior to use -- probably a compiler bug";_tag_dyneither(_tmp1E6,sizeof(char),79);}),_tag_dyneither(_tmp1E5,sizeof(void*),0));});
# 1062
if(topt != 0)Cyc_Tcutil_unify(_tmp23D,*topt);
# 1064
while(_tmp1DD != 0  && _tmp23C != 0){
# 1066
int alias_coercion=0;
struct Cyc_Absyn_Exp*e1=(struct Cyc_Absyn_Exp*)_tmp1DD->hd;
void*t2=(*((struct _tuple9*)_tmp23C->hd)).f3;
Cyc_Tcexp_tcExp(_tmp1E1,& t2,e1);
if(!Cyc_Tcutil_coerce_arg(_tmp1E1,e1,t2,& alias_coercion)){
struct _dyneither_ptr s0=({const char*_tmp1F6="actual argument has type ";_tag_dyneither(_tmp1F6,sizeof(char),26);});
struct _dyneither_ptr s1=Cyc_Absynpp_typ2string((void*)_check_null(e1->topt));
struct _dyneither_ptr s2=({const char*_tmp1F5=" but formal has type ";_tag_dyneither(_tmp1F5,sizeof(char),22);});
struct _dyneither_ptr s3=Cyc_Absynpp_typ2string(t2);
if(((Cyc_strlen((struct _dyneither_ptr)s0)+ Cyc_strlen((struct _dyneither_ptr)s1))+ Cyc_strlen((struct _dyneither_ptr)s2))+ Cyc_strlen((struct _dyneither_ptr)s3)>= 70)
({void*_tmp1E7=0;Cyc_Tcutil_terr(e1->loc,(struct _dyneither_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp1ED;_tmp1ED.tag=0;_tmp1ED.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)s3);({struct Cyc_String_pa_PrintArg_struct _tmp1EC;_tmp1EC.tag=0;_tmp1EC.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)s2);({struct Cyc_String_pa_PrintArg_struct _tmp1EB;_tmp1EB.tag=0;_tmp1EB.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)s1);({struct Cyc_String_pa_PrintArg_struct _tmp1EA;_tmp1EA.tag=0;_tmp1EA.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)s0);({void*_tmp1E8[4]={& _tmp1EA,& _tmp1EB,& _tmp1EC,& _tmp1ED};Cyc_aprintf(({const char*_tmp1E9="%s\n\t%s\n%s\n\t%s.";_tag_dyneither(_tmp1E9,sizeof(char),15);}),_tag_dyneither(_tmp1E8,sizeof(void*),4));});});});});}),_tag_dyneither(_tmp1E7,sizeof(void*),0));});else{
# 1078
({void*_tmp1EE=0;Cyc_Tcutil_terr(e1->loc,(struct _dyneither_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp1F4;_tmp1F4.tag=0;_tmp1F4.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)s3);({struct Cyc_String_pa_PrintArg_struct _tmp1F3;_tmp1F3.tag=0;_tmp1F3.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)s2);({struct Cyc_String_pa_PrintArg_struct _tmp1F2;_tmp1F2.tag=0;_tmp1F2.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)s1);({struct Cyc_String_pa_PrintArg_struct _tmp1F1;_tmp1F1.tag=0;_tmp1F1.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)s0);({void*_tmp1EF[4]={& _tmp1F1,& _tmp1F2,& _tmp1F3,& _tmp1F4};Cyc_aprintf(({const char*_tmp1F0="%s%s%s%s.";_tag_dyneither(_tmp1F0,sizeof(char),10);}),_tag_dyneither(_tmp1EF,sizeof(void*),4));});});});});}),_tag_dyneither(_tmp1EE,sizeof(void*),0));});}
Cyc_Tcutil_unify((void*)_check_null(e1->topt),t2);
Cyc_Tcutil_explain_failure();}
# 1083
if(alias_coercion)
*alias_arg_exps=({struct Cyc_List_List*_tmp1F7=_cycalloc(sizeof(*_tmp1F7));_tmp1F7->hd=(void*)_tmp1DE;_tmp1F7->tl=*alias_arg_exps;_tmp1F7;});
if(Cyc_Tcutil_is_noalias_pointer_or_aggr(ter,t2) && !Cyc_Tcutil_is_noalias_path(ter,e1))
({void*_tmp1F8=0;Cyc_Tcutil_terr(e1->loc,({const char*_tmp1F9="Cannot consume non-unique paths; do swap instead";_tag_dyneither(_tmp1F9,sizeof(char),49);}),_tag_dyneither(_tmp1F8,sizeof(void*),0));});
_tmp1DD=_tmp1DD->tl;
_tmp23C=_tmp23C->tl;
++ _tmp1DE;}{
# 1094
int args_already_checked=0;
{struct Cyc_List_List*a=_tmp238;for(0;a != 0;a=a->tl){
void*_tmp1FA=(void*)a->hd;void*_tmp1FB=_tmp1FA;enum Cyc_Absyn_Format_Type _tmp209;int _tmp208;int _tmp207;if(((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp1FB)->tag == 19){_LL16C: _tmp209=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp1FB)->f1;_tmp208=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp1FB)->f2;_tmp207=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp1FB)->f3;_LL16D:
# 1098
{struct _handler_cons _tmp1FC;_push_handler(& _tmp1FC);{int _tmp1FE=0;if(setjmp(_tmp1FC.handler))_tmp1FE=1;if(!_tmp1FE){
# 1100
{struct Cyc_Absyn_Exp*_tmp1FF=((struct Cyc_Absyn_Exp*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth)(args,_tmp208 - 1);
# 1102
struct Cyc_Core_Opt*fmt_args;
if(_tmp207 == 0)
fmt_args=0;else{
# 1106
fmt_args=({struct Cyc_Core_Opt*_tmp200=_cycalloc(sizeof(*_tmp200));_tmp200->v=((struct Cyc_List_List*(*)(struct Cyc_List_List*x,int i))Cyc_List_nth_tail)(args,_tmp207 - 1);_tmp200;});}
args_already_checked=1;{
struct _RegionHandle _tmp201=_new_region("temp");struct _RegionHandle*temp=& _tmp201;_push_region(temp);
{enum Cyc_Absyn_Format_Type _tmp202=_tmp209;if(_tmp202 == Cyc_Absyn_Printf_ft){_LL171: _LL172:
# 1111
 Cyc_Tcexp_check_format_args(_tmp1E1,_tmp1FF,fmt_args,_tmp207 - 1,alias_arg_exps,temp,Cyc_Formatstr_get_format_typs);
# 1114
goto _LL170;}else{_LL173: _LL174:
# 1116
 Cyc_Tcexp_check_format_args(_tmp1E1,_tmp1FF,fmt_args,_tmp207 - 1,alias_arg_exps,temp,Cyc_Formatstr_get_scanf_typs);
# 1119
goto _LL170;}_LL170:;}
# 1109
;_pop_region(temp);};}
# 1100
;_pop_handler();}else{void*_tmp1FD=(void*)_exn_thrown;void*_tmp203=_tmp1FD;void*_tmp206;if(((struct Cyc_List_Nth_exn_struct*)_tmp203)->tag == Cyc_List_Nth){_LL176: _LL177:
# 1123
({void*_tmp204=0;Cyc_Tcutil_terr(loc,({const char*_tmp205="bad format arguments";_tag_dyneither(_tmp205,sizeof(char),21);}),_tag_dyneither(_tmp204,sizeof(void*),0));});goto _LL175;}else{_LL178: _tmp206=_tmp203;_LL179:(int)_rethrow(_tmp206);}_LL175:;}};}
# 1125
goto _LL16B;}else{_LL16E: _LL16F:
 goto _LL16B;}_LL16B:;}}
# 1129
if(_tmp23C != 0)({void*_tmp20A=0;Cyc_Tcutil_terr(loc,({const char*_tmp20B="too few arguments for function";_tag_dyneither(_tmp20B,sizeof(char),31);}),_tag_dyneither(_tmp20A,sizeof(void*),0));});else{
# 1131
if((_tmp1DD != 0  || _tmp23B) || _tmp23A != 0){
if(_tmp23B)
for(0;_tmp1DD != 0;_tmp1DD=_tmp1DD->tl){
Cyc_Tcexp_tcExp(_tmp1E1,0,(struct Cyc_Absyn_Exp*)_tmp1DD->hd);}else{
if(_tmp23A == 0)
({void*_tmp20C=0;Cyc_Tcutil_terr(loc,({const char*_tmp20D="too many arguments for function";_tag_dyneither(_tmp20D,sizeof(char),32);}),_tag_dyneither(_tmp20C,sizeof(void*),0));});else{
# 1138
struct Cyc_Absyn_VarargInfo _tmp20E=*_tmp23A;struct Cyc_Absyn_VarargInfo _tmp20F=_tmp20E;void*_tmp22E;int _tmp22D;_LL17B: _tmp22E=_tmp20F.type;_tmp22D=_tmp20F.inject;_LL17C:;{
struct Cyc_Absyn_VarargCallInfo*_tmp210=({struct Cyc_Absyn_VarargCallInfo*_tmp22C=_cycalloc(sizeof(*_tmp22C));_tmp22C->num_varargs=0;_tmp22C->injectors=0;_tmp22C->vai=_tmp23A;_tmp22C;});
# 1142
*vararg_call_info=_tmp210;
# 1144
if(!_tmp22D)
# 1146
for(0;_tmp1DD != 0;(_tmp1DD=_tmp1DD->tl,_tmp1DE ++)){
int alias_coercion=0;
struct Cyc_Absyn_Exp*e1=(struct Cyc_Absyn_Exp*)_tmp1DD->hd;
++ _tmp210->num_varargs;
Cyc_Tcexp_tcExp(_tmp1E1,& _tmp22E,e1);
if(!Cyc_Tcutil_coerce_arg(_tmp1E1,e1,_tmp22E,& alias_coercion)){
({struct Cyc_String_pa_PrintArg_struct _tmp214;_tmp214.tag=0;_tmp214.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string((void*)_check_null(e1->topt)));({struct Cyc_String_pa_PrintArg_struct _tmp213;_tmp213.tag=0;_tmp213.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(_tmp22E));({void*_tmp211[2]={& _tmp213,& _tmp214};Cyc_Tcutil_terr(loc,({const char*_tmp212="vararg requires type %s but argument has type %s";_tag_dyneither(_tmp212,sizeof(char),49);}),_tag_dyneither(_tmp211,sizeof(void*),2));});});});
Cyc_Tcutil_explain_failure();}
# 1156
if(alias_coercion)
*alias_arg_exps=({struct Cyc_List_List*_tmp215=_cycalloc(sizeof(*_tmp215));_tmp215->hd=(void*)_tmp1DE;_tmp215->tl=*alias_arg_exps;_tmp215;});
if(Cyc_Tcutil_is_noalias_pointer_or_aggr(ter,_tmp22E) && !
Cyc_Tcutil_is_noalias_path(ter,e1))
({void*_tmp216=0;Cyc_Tcutil_terr(e1->loc,({const char*_tmp217="Cannot consume non-unique paths; do swap instead";_tag_dyneither(_tmp217,sizeof(char),49);}),_tag_dyneither(_tmp216,sizeof(void*),0));});}else{
# 1165
void*_tmp218=Cyc_Tcutil_compress(Cyc_Tcutil_pointer_elt_type(_tmp22E));void*_tmp219=_tmp218;struct Cyc_Absyn_Datatypedecl*_tmp22B;struct Cyc_List_List*_tmp22A;if(((struct Cyc_Absyn_DatatypeType_Absyn_Type_struct*)_tmp219)->tag == 3){if((((((struct Cyc_Absyn_DatatypeType_Absyn_Type_struct*)_tmp219)->f1).datatype_info).KnownDatatype).tag == 2){_LL17E: _tmp22B=*(((((struct Cyc_Absyn_DatatypeType_Absyn_Type_struct*)_tmp219)->f1).datatype_info).KnownDatatype).val;_tmp22A=(((struct Cyc_Absyn_DatatypeType_Absyn_Type_struct*)_tmp219)->f1).targs;_LL17F: {
# 1169
struct Cyc_Absyn_Datatypedecl*_tmp21A=*Cyc_Tcenv_lookup_datatypedecl(_tmp1E1,loc,_tmp22B->name);
struct Cyc_List_List*fields=0;
if(_tmp21A->fields == 0)
({struct Cyc_String_pa_PrintArg_struct _tmp21D;_tmp21D.tag=0;_tmp21D.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string(_tmp22E));({void*_tmp21B[1]={& _tmp21D};Cyc_Tcutil_terr(loc,({const char*_tmp21C="can't inject into abstract datatype %s";_tag_dyneither(_tmp21C,sizeof(char),39);}),_tag_dyneither(_tmp21B,sizeof(void*),1));});});else{
fields=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(_tmp21A->fields))->v;}
# 1181
if(!Cyc_Tcutil_unify(Cyc_Tcutil_pointer_region(_tmp22E),Cyc_Tcenv_curr_rgn(_tmp1E1)))
({struct Cyc_String_pa_PrintArg_struct _tmp221;_tmp221.tag=0;_tmp221.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string(Cyc_Tcenv_curr_rgn(_tmp1E1)));({struct Cyc_String_pa_PrintArg_struct _tmp220;_tmp220.tag=0;_tmp220.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(_tmp22E));({void*_tmp21E[2]={& _tmp220,& _tmp221};Cyc_Tcutil_terr(loc,({const char*_tmp21F="can't unify pointer region for %s to curr_rgn %s";_tag_dyneither(_tmp21F,sizeof(char),49);}),_tag_dyneither(_tmp21E,sizeof(void*),2));});});});{
# 1185
struct _RegionHandle _tmp222=_new_region("rgn");struct _RegionHandle*rgn=& _tmp222;_push_region(rgn);{
struct Cyc_List_List*_tmp223=((struct Cyc_List_List*(*)(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_rzip)(rgn,rgn,_tmp21A->tvs,_tmp22A);
for(0;_tmp1DD != 0;_tmp1DD=_tmp1DD->tl){
++ _tmp210->num_varargs;{
struct Cyc_Absyn_Exp*e1=(struct Cyc_Absyn_Exp*)_tmp1DD->hd;
# 1191
if(!args_already_checked){
Cyc_Tcexp_tcExp(_tmp1E1,0,e1);
if(Cyc_Tcutil_is_noalias_pointer_or_aggr(rgn,(void*)_check_null(e1->topt)) && !
Cyc_Tcutil_is_noalias_path(rgn,e1))
({void*_tmp224=0;Cyc_Tcutil_terr(e1->loc,({const char*_tmp225="Cannot consume non-unique paths; do swap instead";_tag_dyneither(_tmp225,sizeof(char),49);}),_tag_dyneither(_tmp224,sizeof(void*),0));});}{
# 1197
struct Cyc_Absyn_Datatypefield*_tmp226=Cyc_Tcexp_tcInjection(_tmp1E1,e1,Cyc_Tcutil_pointer_elt_type(_tmp22E),rgn,_tmp223,fields);
# 1199
if(_tmp226 != 0)
_tmp210->injectors=
((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_append)(_tmp210->injectors,({struct Cyc_List_List*_tmp227=_cycalloc(sizeof(*_tmp227));_tmp227->hd=_tmp226;_tmp227->tl=0;_tmp227;}));};};}}
# 1205
_npop_handler(0);goto _LL17D;
# 1185
;_pop_region(rgn);};}}else{goto _LL180;}}else{_LL180: _LL181:
# 1206
({void*_tmp228=0;Cyc_Tcutil_terr(loc,({const char*_tmp229="bad inject vararg type";_tag_dyneither(_tmp229,sizeof(char),23);}),_tag_dyneither(_tmp228,sizeof(void*),0));});goto _LL17D;}_LL17D:;}};}}}}
# 1211
if(*alias_arg_exps == 0)
# 1220 "tcexp.cyc"
Cyc_Tcenv_check_effect_accessible(_tmp1E1,loc,(void*)_check_null(_tmp23F));{
# 1222
void*_tmp22F=_tmp23D;_npop_handler(0);return _tmp22F;};};}else{_LL169: _LL16A: {
void*_tmp233=({struct Cyc_String_pa_PrintArg_struct _tmp232;_tmp232.tag=0;_tmp232.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t));({void*_tmp230[1]={& _tmp232};Cyc_Tcexp_expr_err(_tmp1E1,loc,topt,({const char*_tmp231="expected pointer to function but found %s";_tag_dyneither(_tmp231,sizeof(char),42);}),_tag_dyneither(_tmp230,sizeof(void*),1));});});_npop_handler(0);return _tmp233;}}_LL166:;};}else{_LL164: _LL165: {
# 1225
void*_tmp244=({struct Cyc_String_pa_PrintArg_struct _tmp243;_tmp243.tag=0;_tmp243.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t));({void*_tmp241[1]={& _tmp243};Cyc_Tcexp_expr_err(_tmp1E1,loc,topt,({const char*_tmp242="expected pointer to function but found %s";_tag_dyneither(_tmp242,sizeof(char),42);}),_tag_dyneither(_tmp241,sizeof(void*),1));});});_npop_handler(0);return _tmp244;}}_LL161:;};}
# 1039 "tcexp.cyc"
;_pop_region(ter);}
# 1231 "tcexp.cyc"
static void*Cyc_Tcexp_tcThrow(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct Cyc_Absyn_Exp*e){
int bogus=0;
void*_tmp24B=Cyc_Absyn_exn_typ();
Cyc_Tcexp_tcExp(te,& _tmp24B,e);
if(!Cyc_Tcutil_coerce_arg(te,e,_tmp24B,& bogus))
({struct Cyc_String_pa_PrintArg_struct _tmp24F;_tmp24F.tag=0;_tmp24F.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string((void*)_check_null(e->topt)));({struct Cyc_String_pa_PrintArg_struct _tmp24E;_tmp24E.tag=0;_tmp24E.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
# 1236
Cyc_Absynpp_typ2string(_tmp24B));({void*_tmp24C[2]={& _tmp24E,& _tmp24F};Cyc_Tcutil_terr(loc,({const char*_tmp24D="expected %s but found %s";_tag_dyneither(_tmp24D,sizeof(char),25);}),_tag_dyneither(_tmp24C,sizeof(void*),2));});});});
# 1238
if(topt != 0)return*topt;
return Cyc_Absyn_wildtyp(Cyc_Tcenv_lookup_opt_type_vars(te));}
# 1243
static void*Cyc_Tcexp_tcInstantiate(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct Cyc_Absyn_Exp*e,struct Cyc_List_List*ts){
# 1245
Cyc_Tcexp_tcExpNoInst(te,0,e);
# 1247
e->topt=Cyc_Absyn_pointer_expand((void*)_check_null(e->topt),0);{
void*t1=Cyc_Tcutil_compress((void*)_check_null(e->topt));
{void*_tmp250=t1;void*_tmp272;struct Cyc_Absyn_Tqual _tmp271;void*_tmp270;union Cyc_Absyn_Constraint*_tmp26F;union Cyc_Absyn_Constraint*_tmp26E;union Cyc_Absyn_Constraint*_tmp26D;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp250)->tag == 5){_LL183: _tmp272=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp250)->f1).elt_typ;_tmp271=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp250)->f1).elt_tq;_tmp270=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp250)->f1).ptr_atts).rgn;_tmp26F=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp250)->f1).ptr_atts).nullable;_tmp26E=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp250)->f1).ptr_atts).bounds;_tmp26D=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp250)->f1).ptr_atts).zero_term;_LL184:
# 1251
{void*_tmp251=Cyc_Tcutil_compress(_tmp272);void*_tmp252=_tmp251;struct Cyc_List_List*_tmp26C;void*_tmp26B;struct Cyc_Absyn_Tqual _tmp26A;void*_tmp269;struct Cyc_List_List*_tmp268;int _tmp267;struct Cyc_Absyn_VarargInfo*_tmp266;struct Cyc_List_List*_tmp265;struct Cyc_List_List*_tmp264;struct Cyc_Absyn_Exp*_tmp263;struct Cyc_List_List*_tmp262;struct Cyc_Absyn_Exp*_tmp261;struct Cyc_List_List*_tmp260;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp252)->tag == 9){_LL188: _tmp26C=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp252)->f1).tvars;_tmp26B=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp252)->f1).effect;_tmp26A=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp252)->f1).ret_tqual;_tmp269=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp252)->f1).ret_typ;_tmp268=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp252)->f1).args;_tmp267=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp252)->f1).c_varargs;_tmp266=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp252)->f1).cyc_varargs;_tmp265=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp252)->f1).rgn_po;_tmp264=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp252)->f1).attributes;_tmp263=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp252)->f1).requires_clause;_tmp262=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp252)->f1).requires_relns;_tmp261=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp252)->f1).ensures_clause;_tmp260=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp252)->f1).ensures_relns;_LL189: {
# 1253
struct _RegionHandle _tmp253=_new_region("temp");struct _RegionHandle*temp=& _tmp253;_push_region(temp);
{struct Cyc_List_List*instantiation=0;
# 1256
for(0;ts != 0  && _tmp26C != 0;(ts=ts->tl,_tmp26C=_tmp26C->tl)){
struct Cyc_Absyn_Kind*k=Cyc_Tcutil_tvar_kind((struct Cyc_Absyn_Tvar*)_tmp26C->hd,& Cyc_Tcutil_bk);
Cyc_Tcutil_check_type(loc,te,Cyc_Tcenv_lookup_type_vars(te),k,1,1,(void*)ts->hd);
Cyc_Tcutil_check_no_qual(loc,(void*)ts->hd);
instantiation=({struct Cyc_List_List*_tmp254=_region_malloc(temp,sizeof(*_tmp254));_tmp254->hd=({struct _tuple13*_tmp255=_region_malloc(temp,sizeof(*_tmp255));_tmp255->f1=(struct Cyc_Absyn_Tvar*)_tmp26C->hd;_tmp255->f2=(void*)ts->hd;_tmp255;});_tmp254->tl=instantiation;_tmp254;});}
# 1263
if(ts != 0){
void*_tmp258=({void*_tmp256=0;Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp257="too many type variables in instantiation";_tag_dyneither(_tmp257,sizeof(char),41);}),_tag_dyneither(_tmp256,sizeof(void*),0));});_npop_handler(0);return _tmp258;}
# 1269
if(_tmp26C == 0){
_tmp265=Cyc_Tcutil_rsubst_rgnpo(temp,instantiation,_tmp265);
Cyc_Tcenv_check_rgn_partial_order(te,loc,_tmp265);
_tmp265=0;}{
# 1274
void*new_fn_typ=
Cyc_Tcutil_rsubstitute(temp,instantiation,(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp25D=_cycalloc(sizeof(*_tmp25D));_tmp25D[0]=({struct Cyc_Absyn_FnType_Absyn_Type_struct _tmp25E;_tmp25E.tag=9;_tmp25E.f1=({struct Cyc_Absyn_FnInfo _tmp25F;_tmp25F.tvars=_tmp26C;_tmp25F.effect=_tmp26B;_tmp25F.ret_tqual=_tmp26A;_tmp25F.ret_typ=_tmp269;_tmp25F.args=_tmp268;_tmp25F.c_varargs=_tmp267;_tmp25F.cyc_varargs=_tmp266;_tmp25F.rgn_po=_tmp265;_tmp25F.attributes=_tmp264;_tmp25F.requires_clause=_tmp263;_tmp25F.requires_relns=_tmp262;_tmp25F.ensures_clause=_tmp261;_tmp25F.ensures_relns=_tmp260;_tmp25F;});_tmp25E;});_tmp25D;}));
# 1279
void*_tmp25C=(void*)({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmp259=_cycalloc(sizeof(*_tmp259));_tmp259[0]=({struct Cyc_Absyn_PointerType_Absyn_Type_struct _tmp25A;_tmp25A.tag=5;_tmp25A.f1=({struct Cyc_Absyn_PtrInfo _tmp25B;_tmp25B.elt_typ=new_fn_typ;_tmp25B.elt_tq=_tmp271;_tmp25B.ptr_atts=({(_tmp25B.ptr_atts).rgn=_tmp270;(_tmp25B.ptr_atts).nullable=_tmp26F;(_tmp25B.ptr_atts).bounds=_tmp26E;(_tmp25B.ptr_atts).zero_term=_tmp26D;(_tmp25B.ptr_atts).ptrloc=0;_tmp25B.ptr_atts;});_tmp25B;});_tmp25A;});_tmp259;});_npop_handler(0);return _tmp25C;};}
# 1254
;_pop_region(temp);}}else{_LL18A: _LL18B:
# 1281
 goto _LL187;}_LL187:;}
# 1283
goto _LL182;}else{_LL185: _LL186:
 goto _LL182;}_LL182:;}
# 1286
return({struct Cyc_String_pa_PrintArg_struct _tmp275;_tmp275.tag=0;_tmp275.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string(t1));({void*_tmp273[1]={& _tmp275};Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp274="expecting polymorphic type but found %s";_tag_dyneither(_tmp274,sizeof(char),40);}),_tag_dyneither(_tmp273,sizeof(void*),1));});});};}
# 1291
static void*Cyc_Tcexp_tcCast(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,void*t,struct Cyc_Absyn_Exp*e,enum Cyc_Absyn_Coercion*c){
# 1293
Cyc_Tcutil_check_type(loc,te,Cyc_Tcenv_lookup_type_vars(te),
Cyc_Tcenv_abstract_val_ok(te)?& Cyc_Tcutil_tak:& Cyc_Tcutil_tmk,1,0,t);
Cyc_Tcutil_check_no_qual(loc,t);
# 1297
Cyc_Tcexp_tcExp(te,& t,e);{
void*t2=(void*)_check_null(e->topt);
if(Cyc_Tcutil_silent_castable(te,loc,t2,t))
*((enum Cyc_Absyn_Coercion*)_check_null(c))=Cyc_Absyn_No_coercion;else{
# 1302
enum Cyc_Absyn_Coercion crc=Cyc_Tcutil_castable(te,loc,t2,t);
if(crc != Cyc_Absyn_Unknown_coercion)
*((enum Cyc_Absyn_Coercion*)_check_null(c))=crc;else{
if(Cyc_Tcutil_zero_to_null(te,t,e))
*((enum Cyc_Absyn_Coercion*)_check_null(c))=Cyc_Absyn_No_coercion;else{
# 1309
Cyc_Tcutil_unify(t2,t);{
void*_tmp276=({struct Cyc_String_pa_PrintArg_struct _tmp27A;_tmp27A.tag=0;_tmp27A.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string(t));({struct Cyc_String_pa_PrintArg_struct _tmp279;_tmp279.tag=0;_tmp279.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t2));({void*_tmp277[2]={& _tmp279,& _tmp27A};Cyc_Tcexp_expr_err(te,loc,& t,({const char*_tmp278="cannot cast %s to %s";_tag_dyneither(_tmp278,sizeof(char),21);}),_tag_dyneither(_tmp277,sizeof(void*),2));});});});
Cyc_Tcutil_explain_failure();
return _tmp276;};}}}
# 1319
{struct _tuple0 _tmp27B=({struct _tuple0 _tmp286;_tmp286.f1=e->r;_tmp286.f2=Cyc_Tcutil_compress(t);_tmp286;});struct _tuple0 _tmp27C=_tmp27B;int _tmp285;union Cyc_Absyn_Constraint*_tmp284;union Cyc_Absyn_Constraint*_tmp283;union Cyc_Absyn_Constraint*_tmp282;if(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp27C.f1)->tag == 33){if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp27C.f2)->tag == 5){_LL18D: _tmp285=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp27C.f1)->f1).fat_result;_tmp284=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp27C.f2)->f1).ptr_atts).nullable;_tmp283=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp27C.f2)->f1).ptr_atts).bounds;_tmp282=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp27C.f2)->f1).ptr_atts).zero_term;_LL18E:
# 1323
 if((_tmp285  && !((int(*)(int y,union Cyc_Absyn_Constraint*x))Cyc_Absyn_conref_def)(0,_tmp282)) && ((int(*)(int y,union Cyc_Absyn_Constraint*x))Cyc_Absyn_conref_def)(0,_tmp284)){
void*_tmp27D=((void*(*)(void*y,union Cyc_Absyn_Constraint*x))Cyc_Absyn_conref_def)(Cyc_Absyn_bounds_one,_tmp283);void*_tmp27E=_tmp27D;struct Cyc_Absyn_Exp*_tmp281;if(((struct Cyc_Absyn_Upper_b_Absyn_Bounds_struct*)_tmp27E)->tag == 1){_LL192: _tmp281=((struct Cyc_Absyn_Upper_b_Absyn_Bounds_struct*)_tmp27E)->f1;_LL193:
# 1326
 if((Cyc_Evexp_eval_const_uint_exp(_tmp281)).f1 == 1)
({void*_tmp27F=0;Cyc_Tcutil_warn(loc,({const char*_tmp280="cast from ? pointer to * pointer will lose size information";_tag_dyneither(_tmp280,sizeof(char),60);}),_tag_dyneither(_tmp27F,sizeof(void*),0));});
goto _LL191;}else{_LL194: _LL195:
 goto _LL191;}_LL191:;}
# 1332
goto _LL18C;}else{goto _LL18F;}}else{_LL18F: _LL190:
 goto _LL18C;}_LL18C:;}
# 1335
return t;};}
# 1339
static void*Cyc_Tcexp_tcAddress(struct Cyc_Tcenv_Tenv*te,unsigned int loc,struct Cyc_Absyn_Exp*e0,void**topt,struct Cyc_Absyn_Exp*e){
# 1341
void**_tmp287=0;
struct Cyc_Absyn_Tqual _tmp288=Cyc_Absyn_empty_tqual(0);
int _tmp289=0;
if(topt != 0){
void*_tmp28A=Cyc_Tcutil_compress(*topt);void*_tmp28B=_tmp28A;void**_tmp28E;struct Cyc_Absyn_Tqual _tmp28D;union Cyc_Absyn_Constraint*_tmp28C;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp28B)->tag == 5){_LL197: _tmp28E=(void**)&(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp28B)->f1).elt_typ;_tmp28D=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp28B)->f1).elt_tq;_tmp28C=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp28B)->f1).ptr_atts).nullable;_LL198:
# 1347
 _tmp287=_tmp28E;
_tmp288=_tmp28D;
_tmp289=((int(*)(union Cyc_Absyn_Constraint*x))Cyc_Absyn_conref_val)(_tmp28C);
goto _LL196;}else{_LL199: _LL19A:
 goto _LL196;}_LL196:;}{
# 1361
struct _RegionHandle _tmp28F=_new_region("r");struct _RegionHandle*r=& _tmp28F;_push_region(r);
Cyc_Tcexp_tcExpNoInst(Cyc_Tcenv_enter_abstract_val_ok(r,Cyc_Tcenv_enter_lhs(r,Cyc_Tcenv_clear_notreadctxt(r,te))),_tmp287,e);
# 1365
if(Cyc_Tcutil_is_noalias_pointer_or_aggr(r,(void*)_check_null(e->topt)))
({void*_tmp290=0;Cyc_Tcutil_terr(e->loc,({const char*_tmp291="Cannot take the address of an alias-free path";_tag_dyneither(_tmp291,sizeof(char),46);}),_tag_dyneither(_tmp290,sizeof(void*),0));});
# 1372
{void*_tmp292=e->r;void*_tmp293=_tmp292;struct Cyc_Absyn_Exp*_tmp298;struct Cyc_Absyn_Exp*_tmp297;if(((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp293)->tag == 22){_LL19C: _tmp298=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp293)->f1;_tmp297=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp293)->f2;_LL19D:
# 1374
{void*_tmp294=Cyc_Tcutil_compress((void*)_check_null(_tmp298->topt));void*_tmp295=_tmp294;if(((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp295)->tag == 10){_LL1A1: _LL1A2:
 goto _LL1A0;}else{_LL1A3: _LL1A4:
# 1379
 e0->r=(Cyc_Absyn_add_exp(_tmp298,_tmp297,0))->r;{
void*_tmp296=Cyc_Tcexp_tcPlus(te,_tmp298,_tmp297);_npop_handler(0);return _tmp296;};}_LL1A0:;}
# 1382
goto _LL19B;}else{_LL19E: _LL19F:
 goto _LL19B;}_LL19B:;}
# 1387
{void*_tmp299=e->r;void*_tmp29A=_tmp299;switch(*((int*)_tmp29A)){case 20: if(((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp29A)->f3 == 1){_LL1A6: _LL1A7:
 goto _LL1A9;}else{goto _LL1AA;}case 21: if(((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp29A)->f3 == 1){_LL1A8: _LL1A9:
# 1390
({void*_tmp29B=0;Cyc_Tcutil_terr(e->loc,({const char*_tmp29C="cannot take the address of a @tagged union member";_tag_dyneither(_tmp29C,sizeof(char),50);}),_tag_dyneither(_tmp29B,sizeof(void*),0));});
goto _LL1A5;}else{goto _LL1AA;}default: _LL1AA: _LL1AB:
 goto _LL1A5;}_LL1A5:;}{
# 1396
struct _tuple14 _tmp29D=Cyc_Tcutil_addressof_props(te,e);struct _tuple14 _tmp29E=_tmp29D;int _tmp2A3;void*_tmp2A2;_LL1AD: _tmp2A3=_tmp29E.f1;_tmp2A2=_tmp29E.f2;_LL1AE:;
# 1398
if(Cyc_Tcutil_is_noalias_region(_tmp2A2,0))
({void*_tmp29F=0;Cyc_Tcutil_terr(e->loc,({const char*_tmp2A0="using & would manufacture an alias to an alias-free pointer";_tag_dyneither(_tmp2A0,sizeof(char),60);}),_tag_dyneither(_tmp29F,sizeof(void*),0));});{
# 1401
struct Cyc_Absyn_Tqual tq=Cyc_Absyn_empty_tqual(0);
if(_tmp2A3){
tq.print_const=1;
tq.real_const=1;}{
# 1407
void*t=_tmp289?
Cyc_Absyn_star_typ((void*)_check_null(e->topt),_tmp2A2,tq,Cyc_Absyn_false_conref):
 Cyc_Absyn_at_typ((void*)_check_null(e->topt),_tmp2A2,tq,Cyc_Absyn_false_conref);
void*_tmp2A1=t;_npop_handler(0);return _tmp2A1;};};};
# 1361
;_pop_region(r);};}
# 1414
static void*Cyc_Tcexp_tcSizeof(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,void*t){
if(te->allow_valueof)
# 1418
return Cyc_Absyn_uint_typ;
# 1420
Cyc_Tcutil_check_type(loc,te,Cyc_Tcenv_lookup_type_vars(te),& Cyc_Tcutil_tmk,1,0,t);
Cyc_Tcutil_check_no_qual(loc,t);
if(!Cyc_Evexp_okay_szofarg(t))
({struct Cyc_String_pa_PrintArg_struct _tmp2A6;_tmp2A6.tag=0;_tmp2A6.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string(t));({void*_tmp2A4[1]={& _tmp2A6};Cyc_Tcutil_terr(loc,({const char*_tmp2A5="sizeof applied to type %s, which has unknown size here";_tag_dyneither(_tmp2A5,sizeof(char),55);}),_tag_dyneither(_tmp2A4,sizeof(void*),1));});});
if(topt != 0){
void*_tmp2A7=Cyc_Tcutil_compress(*topt);void*_tmp2A8=_tmp2A7;void*_tmp2AE;void*_tmp2AD;if(((struct Cyc_Absyn_TagType_Absyn_Type_struct*)_tmp2A8)->tag == 19){_LL1B0: _tmp2AE=_tmp2A8;_tmp2AD=(void*)((struct Cyc_Absyn_TagType_Absyn_Type_struct*)_tmp2A8)->f1;_LL1B1: {
# 1429
struct Cyc_Absyn_Exp*_tmp2A9=Cyc_Absyn_sizeoftyp_exp(t,0);
struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_tmp2AA=({struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_tmp2AB=_cycalloc(sizeof(*_tmp2AB));_tmp2AB[0]=({struct Cyc_Absyn_ValueofType_Absyn_Type_struct _tmp2AC;_tmp2AC.tag=18;_tmp2AC.f1=_tmp2A9;_tmp2AC;});_tmp2AB;});
if(Cyc_Tcutil_unify(_tmp2AD,(void*)_tmp2AA))return _tmp2AE;
goto _LL1AF;}}else{_LL1B2: _LL1B3:
 goto _LL1AF;}_LL1AF:;}
# 1435
return Cyc_Absyn_uint_typ;}
# 1438
void*Cyc_Tcexp_structfield_type(struct _dyneither_ptr*n,struct Cyc_Absyn_Aggrfield*sf){
if(Cyc_strcmp((struct _dyneither_ptr)*n,(struct _dyneither_ptr)*sf->name)== 0)return sf->type;else{
return 0;}}
# 1445
static void*Cyc_Tcexp_tcOffsetof(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,void*t_orig,struct Cyc_List_List*fs){
# 1447
Cyc_Tcutil_check_type(loc,te,Cyc_Tcenv_lookup_type_vars(te),& Cyc_Tcutil_tmk,1,0,t_orig);
Cyc_Tcutil_check_no_qual(loc,t_orig);{
struct Cyc_List_List*_tmp2AF=fs;
void*_tmp2B0=t_orig;
for(0;_tmp2AF != 0;_tmp2AF=_tmp2AF->tl){
void*_tmp2B1=(void*)_tmp2AF->hd;
void*_tmp2B2=_tmp2B1;unsigned int _tmp2EC;struct _dyneither_ptr*_tmp2EB;if(((struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*)_tmp2B2)->tag == 0){_LL1B5: _tmp2EB=((struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*)_tmp2B2)->f1;_LL1B6: {
# 1455
int bad_type=1;
{void*_tmp2B3=Cyc_Tcutil_compress(_tmp2B0);void*_tmp2B4=_tmp2B3;struct Cyc_List_List*_tmp2BC;struct Cyc_Absyn_Aggrdecl**_tmp2BB;switch(*((int*)_tmp2B4)){case 11: if((((((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp2B4)->f1).aggr_info).KnownAggr).tag == 2){_LL1BA: _tmp2BB=(((((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp2B4)->f1).aggr_info).KnownAggr).val;_LL1BB:
# 1458
 if((*_tmp2BB)->impl == 0)goto _LL1B9;{
void*t2=((void*(*)(void*(*pred)(struct _dyneither_ptr*,struct Cyc_Absyn_Aggrfield*),struct _dyneither_ptr*env,struct Cyc_List_List*x))Cyc_List_find_c)(Cyc_Tcexp_structfield_type,_tmp2EB,((struct Cyc_Absyn_AggrdeclImpl*)_check_null((*_tmp2BB)->impl))->fields);
if(!((unsigned int)t2))
({struct Cyc_String_pa_PrintArg_struct _tmp2B7;_tmp2B7.tag=0;_tmp2B7.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*_tmp2EB);({void*_tmp2B5[1]={& _tmp2B7};Cyc_Tcutil_terr(loc,({const char*_tmp2B6="no field of struct/union has name %s";_tag_dyneither(_tmp2B6,sizeof(char),37);}),_tag_dyneither(_tmp2B5,sizeof(void*),1));});});else{
# 1463
_tmp2B0=t2;}
bad_type=0;
goto _LL1B9;};}else{goto _LL1BE;}case 12: _LL1BC: _tmp2BC=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp2B4)->f2;_LL1BD: {
# 1467
void*t2=((void*(*)(void*(*pred)(struct _dyneither_ptr*,struct Cyc_Absyn_Aggrfield*),struct _dyneither_ptr*env,struct Cyc_List_List*x))Cyc_List_find_c)(Cyc_Tcexp_structfield_type,_tmp2EB,_tmp2BC);
if(!((unsigned int)t2))
({struct Cyc_String_pa_PrintArg_struct _tmp2BA;_tmp2BA.tag=0;_tmp2BA.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*_tmp2EB);({void*_tmp2B8[1]={& _tmp2BA};Cyc_Tcutil_terr(loc,({const char*_tmp2B9="no field of struct/union has name %s";_tag_dyneither(_tmp2B9,sizeof(char),37);}),_tag_dyneither(_tmp2B8,sizeof(void*),1));});});else{
# 1471
_tmp2B0=t2;}
bad_type=0;
goto _LL1B9;}default: _LL1BE: _LL1BF:
 goto _LL1B9;}_LL1B9:;}
# 1476
if(bad_type){
if(_tmp2AF == fs)
({struct Cyc_String_pa_PrintArg_struct _tmp2BF;_tmp2BF.tag=0;_tmp2BF.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(_tmp2B0));({void*_tmp2BD[1]={& _tmp2BF};Cyc_Tcutil_terr(loc,({const char*_tmp2BE="%s is not a known struct/union type";_tag_dyneither(_tmp2BE,sizeof(char),36);}),_tag_dyneither(_tmp2BD,sizeof(void*),1));});});else{
# 1480
struct _dyneither_ptr _tmp2C0=({struct Cyc_String_pa_PrintArg_struct _tmp2D3;_tmp2D3.tag=0;_tmp2D3.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t_orig));({void*_tmp2D1[1]={& _tmp2D3};Cyc_aprintf(({const char*_tmp2D2="(%s)";_tag_dyneither(_tmp2D2,sizeof(char),5);}),_tag_dyneither(_tmp2D1,sizeof(void*),1));});});
struct Cyc_List_List*x;
for(x=fs;x != _tmp2AF;x=x->tl){
void*_tmp2C1=(void*)((struct Cyc_List_List*)_check_null(x))->hd;void*_tmp2C2=_tmp2C1;unsigned int _tmp2CC;struct _dyneither_ptr*_tmp2CB;if(((struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*)_tmp2C2)->tag == 0){_LL1C1: _tmp2CB=((struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*)_tmp2C2)->f1;_LL1C2:
# 1485
 _tmp2C0=({struct Cyc_String_pa_PrintArg_struct _tmp2C6;_tmp2C6.tag=0;_tmp2C6.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*_tmp2CB);({struct Cyc_String_pa_PrintArg_struct _tmp2C5;_tmp2C5.tag=0;_tmp2C5.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)_tmp2C0);({void*_tmp2C3[2]={& _tmp2C5,& _tmp2C6};Cyc_aprintf(({const char*_tmp2C4="%s.%s";_tag_dyneither(_tmp2C4,sizeof(char),6);}),_tag_dyneither(_tmp2C3,sizeof(void*),2));});});});goto _LL1C0;}else{_LL1C3: _tmp2CC=((struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct*)_tmp2C2)->f1;_LL1C4:
# 1487
 _tmp2C0=({struct Cyc_Int_pa_PrintArg_struct _tmp2CA;_tmp2CA.tag=1;_tmp2CA.f1=(unsigned long)((int)_tmp2CC);({struct Cyc_String_pa_PrintArg_struct _tmp2C9;_tmp2C9.tag=0;_tmp2C9.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)_tmp2C0);({void*_tmp2C7[2]={& _tmp2C9,& _tmp2CA};Cyc_aprintf(({const char*_tmp2C8="%s.%d";_tag_dyneither(_tmp2C8,sizeof(char),6);}),_tag_dyneither(_tmp2C7,sizeof(void*),2));});});});goto _LL1C0;}_LL1C0:;}
# 1489
({struct Cyc_String_pa_PrintArg_struct _tmp2D0;_tmp2D0.tag=0;_tmp2D0.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(_tmp2B0));({struct Cyc_String_pa_PrintArg_struct _tmp2CF;_tmp2CF.tag=0;_tmp2CF.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)_tmp2C0);({void*_tmp2CD[2]={& _tmp2CF,& _tmp2D0};Cyc_Tcutil_terr(loc,({const char*_tmp2CE="%s == %s is not a struct/union type";_tag_dyneither(_tmp2CE,sizeof(char),36);}),_tag_dyneither(_tmp2CD,sizeof(void*),2));});});});}}
# 1492
goto _LL1B4;}}else{_LL1B7: _tmp2EC=((struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct*)_tmp2B2)->f1;_LL1B8: {
# 1494
int bad_type=1;
{void*_tmp2D4=Cyc_Tcutil_compress(_tmp2B0);void*_tmp2D5=_tmp2D4;struct Cyc_Absyn_Datatypefield*_tmp2E7;struct Cyc_List_List*_tmp2E6;struct Cyc_List_List*_tmp2E5;struct Cyc_Absyn_Aggrdecl**_tmp2E4;switch(*((int*)_tmp2D5)){case 11: if((((((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp2D5)->f1).aggr_info).KnownAggr).tag == 2){_LL1C6: _tmp2E4=(((((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp2D5)->f1).aggr_info).KnownAggr).val;_LL1C7:
# 1497
 if((*_tmp2E4)->impl == 0)
goto _LL1C5;
_tmp2E5=((struct Cyc_Absyn_AggrdeclImpl*)_check_null((*_tmp2E4)->impl))->fields;goto _LL1C9;}else{goto _LL1CE;}case 12: _LL1C8: _tmp2E5=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp2D5)->f2;_LL1C9:
# 1501
 if(((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp2E5)<= _tmp2EC)
({struct Cyc_Int_pa_PrintArg_struct _tmp2D9;_tmp2D9.tag=1;_tmp2D9.f1=(unsigned long)((int)_tmp2EC);({struct Cyc_Int_pa_PrintArg_struct _tmp2D8;_tmp2D8.tag=1;_tmp2D8.f1=(unsigned long)
((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp2E5);({void*_tmp2D6[2]={& _tmp2D8,& _tmp2D9};Cyc_Tcutil_terr(loc,({const char*_tmp2D7="struct/union has too few components: %d <= %d";_tag_dyneither(_tmp2D7,sizeof(char),46);}),_tag_dyneither(_tmp2D6,sizeof(void*),2));});});});else{
# 1505
_tmp2B0=(((struct Cyc_Absyn_Aggrfield*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth)(_tmp2E5,(int)_tmp2EC))->type;}
bad_type=0;
goto _LL1C5;case 10: _LL1CA: _tmp2E6=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp2D5)->f1;_LL1CB:
# 1509
 if(((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp2E6)<= _tmp2EC)
({struct Cyc_Int_pa_PrintArg_struct _tmp2DD;_tmp2DD.tag=1;_tmp2DD.f1=(unsigned long)((int)_tmp2EC);({struct Cyc_Int_pa_PrintArg_struct _tmp2DC;_tmp2DC.tag=1;_tmp2DC.f1=(unsigned long)
((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp2E6);({void*_tmp2DA[2]={& _tmp2DC,& _tmp2DD};Cyc_Tcutil_terr(loc,({const char*_tmp2DB="tuple has too few components: %d <= %d";_tag_dyneither(_tmp2DB,sizeof(char),39);}),_tag_dyneither(_tmp2DA,sizeof(void*),2));});});});else{
# 1513
_tmp2B0=(*((struct _tuple18*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth)(_tmp2E6,(int)_tmp2EC)).f2;}
bad_type=0;
goto _LL1C5;case 4: if((((((struct Cyc_Absyn_DatatypeFieldType_Absyn_Type_struct*)_tmp2D5)->f1).field_info).KnownDatatypefield).tag == 2){_LL1CC: _tmp2E7=((((((struct Cyc_Absyn_DatatypeFieldType_Absyn_Type_struct*)_tmp2D5)->f1).field_info).KnownDatatypefield).val).f2;_LL1CD:
# 1517
 if(((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp2E7->typs)< _tmp2EC)
({struct Cyc_Int_pa_PrintArg_struct _tmp2E1;_tmp2E1.tag=1;_tmp2E1.f1=(unsigned long)((int)_tmp2EC);({struct Cyc_Int_pa_PrintArg_struct _tmp2E0;_tmp2E0.tag=1;_tmp2E0.f1=(unsigned long)
((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp2E7->typs);({void*_tmp2DE[2]={& _tmp2E0,& _tmp2E1};Cyc_Tcutil_terr(loc,({const char*_tmp2DF="datatype field has too few components: %d < %d";_tag_dyneither(_tmp2DF,sizeof(char),47);}),_tag_dyneither(_tmp2DE,sizeof(void*),2));});});});else{
# 1521
if(_tmp2EC != 0)
_tmp2B0=(*((struct _tuple18*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth)(_tmp2E7->typs,(int)(_tmp2EC - 1))).f2;else{
if(_tmp2AF->tl != 0)
({void*_tmp2E2=0;Cyc_Tcutil_terr(loc,({const char*_tmp2E3="datatype field index 0 refers to the tag; cannot be indexed through";_tag_dyneither(_tmp2E3,sizeof(char),68);}),_tag_dyneither(_tmp2E2,sizeof(void*),0));});}}
# 1526
bad_type=0;
goto _LL1C5;}else{goto _LL1CE;}default: _LL1CE: _LL1CF:
 goto _LL1C5;}_LL1C5:;}
# 1530
if(bad_type)
({struct Cyc_String_pa_PrintArg_struct _tmp2EA;_tmp2EA.tag=0;_tmp2EA.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(_tmp2B0));({void*_tmp2E8[1]={& _tmp2EA};Cyc_Tcutil_terr(loc,({const char*_tmp2E9="%s is not a known type";_tag_dyneither(_tmp2E9,sizeof(char),23);}),_tag_dyneither(_tmp2E8,sizeof(void*),1));});});
goto _LL1B4;}}_LL1B4:;}
# 1535
return Cyc_Absyn_uint_typ;};}
# 1539
static void*Cyc_Tcexp_tcDeref(struct Cyc_Tcenv_Tenv*te_orig,unsigned int loc,void**topt,struct Cyc_Absyn_Exp*e){
struct _RegionHandle _tmp2ED=_new_region("r");struct _RegionHandle*r=& _tmp2ED;_push_region(r);
{struct Cyc_Tcenv_Tenv*_tmp2EE=Cyc_Tcenv_clear_lhs(r,Cyc_Tcenv_clear_notreadctxt(r,te_orig));
Cyc_Tcexp_tcExp(_tmp2EE,0,e);{
void*t=Cyc_Tcutil_compress((void*)_check_null(e->topt));
void*_tmp2EF=t;void*_tmp307;void*_tmp306;union Cyc_Absyn_Constraint*_tmp305;union Cyc_Absyn_Constraint*_tmp304;switch(*((int*)_tmp2EF)){case 1: _LL1D1: _LL1D2: {
# 1546
struct Cyc_List_List*_tmp2F0=Cyc_Tcenv_lookup_type_vars(_tmp2EE);
void*_tmp2F1=Cyc_Absyn_new_evar(& Cyc_Tcutil_ako,({struct Cyc_Core_Opt*_tmp2FC=_cycalloc(sizeof(*_tmp2FC));_tmp2FC->v=_tmp2F0;_tmp2FC;}));
void*_tmp2F2=Cyc_Absyn_new_evar(& Cyc_Tcutil_trko,({struct Cyc_Core_Opt*_tmp2FB=_cycalloc(sizeof(*_tmp2FB));_tmp2FB->v=_tmp2F0;_tmp2FB;}));
union Cyc_Absyn_Constraint*_tmp2F3=((union Cyc_Absyn_Constraint*(*)())Cyc_Absyn_empty_conref)();
union Cyc_Absyn_Constraint*_tmp2F4=((union Cyc_Absyn_Constraint*(*)())Cyc_Absyn_empty_conref)();
struct Cyc_Absyn_PtrAtts _tmp2F5=({struct Cyc_Absyn_PtrAtts _tmp2FA;_tmp2FA.rgn=_tmp2F2;_tmp2FA.nullable=((union Cyc_Absyn_Constraint*(*)())Cyc_Absyn_empty_conref)();_tmp2FA.bounds=_tmp2F3;_tmp2FA.zero_term=_tmp2F4;_tmp2FA.ptrloc=0;_tmp2FA;});
struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmp2F6=({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmp2F7=_cycalloc(sizeof(*_tmp2F7));_tmp2F7[0]=({struct Cyc_Absyn_PointerType_Absyn_Type_struct _tmp2F8;_tmp2F8.tag=5;_tmp2F8.f1=({struct Cyc_Absyn_PtrInfo _tmp2F9;_tmp2F9.elt_typ=_tmp2F1;_tmp2F9.elt_tq=Cyc_Absyn_empty_tqual(0);_tmp2F9.ptr_atts=_tmp2F5;_tmp2F9;});_tmp2F8;});_tmp2F7;});
Cyc_Tcutil_unify(t,(void*)_tmp2F6);
_tmp307=_tmp2F1;_tmp306=_tmp2F2;_tmp305=_tmp2F3;_tmp304=_tmp2F4;goto _LL1D4;}case 5: _LL1D3: _tmp307=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2EF)->f1).elt_typ;_tmp306=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2EF)->f1).ptr_atts).rgn;_tmp305=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2EF)->f1).ptr_atts).bounds;_tmp304=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2EF)->f1).ptr_atts).zero_term;_LL1D4:
# 1556
 Cyc_Tcenv_check_rgn_accessible(_tmp2EE,loc,_tmp306);
Cyc_Tcutil_check_nonzero_bound(loc,_tmp305);
if(!Cyc_Tcutil_kind_leq(Cyc_Tcutil_typ_kind(_tmp307),& Cyc_Tcutil_tmk) && !Cyc_Tcenv_abstract_val_ok(_tmp2EE))
({void*_tmp2FD=0;Cyc_Tcutil_terr(loc,({const char*_tmp2FE="can't dereference abstract pointer type";_tag_dyneither(_tmp2FE,sizeof(char),40);}),_tag_dyneither(_tmp2FD,sizeof(void*),0));});{
void*_tmp2FF=_tmp307;_npop_handler(0);return _tmp2FF;};default: _LL1D5: _LL1D6: {
# 1562
void*_tmp303=({struct Cyc_String_pa_PrintArg_struct _tmp302;_tmp302.tag=0;_tmp302.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t));({void*_tmp300[1]={& _tmp302};Cyc_Tcexp_expr_err(_tmp2EE,loc,topt,({const char*_tmp301="expecting *, @, or ? type but found %s";_tag_dyneither(_tmp301,sizeof(char),39);}),_tag_dyneither(_tmp300,sizeof(void*),1));});});_npop_handler(0);return _tmp303;}}_LL1D0:;};}
# 1541
;_pop_region(r);}
# 1568
static void*Cyc_Tcexp_tcAggrMember(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct Cyc_Absyn_Exp*outer_e,struct Cyc_Absyn_Exp*e,struct _dyneither_ptr*f,int*is_tagged,int*is_read){
# 1572
struct _RegionHandle _tmp308=_new_region("r");struct _RegionHandle*r=& _tmp308;_push_region(r);
Cyc_Tcexp_tcExpNoPromote(Cyc_Tcenv_enter_abstract_val_ok(r,te),0,e);
# 1576
*is_read=!Cyc_Tcenv_in_notreadctxt(te);{
void*_tmp309=Cyc_Tcutil_compress((void*)_check_null(e->topt));void*_tmp30A=_tmp309;enum Cyc_Absyn_AggrKind _tmp32D;struct Cyc_List_List*_tmp32C;struct Cyc_Absyn_Aggrdecl*_tmp32B;struct Cyc_List_List*_tmp32A;switch(*((int*)_tmp30A)){case 11: if((((((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp30A)->f1).aggr_info).KnownAggr).tag == 2){_LL1D8: _tmp32B=*(((((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp30A)->f1).aggr_info).KnownAggr).val;_tmp32A=(((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp30A)->f1).targs;_LL1D9: {
# 1579
struct Cyc_Absyn_Aggrfield*_tmp30B=Cyc_Absyn_lookup_decl_field(_tmp32B,f);
if(_tmp30B == 0){
void*_tmp310=({struct Cyc_String_pa_PrintArg_struct _tmp30F;_tmp30F.tag=0;_tmp30F.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*f);({struct Cyc_String_pa_PrintArg_struct _tmp30E;_tmp30E.tag=0;_tmp30E.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_qvar2string(_tmp32B->name));({void*_tmp30C[2]={& _tmp30E,& _tmp30F};Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp30D="%s has no %s member";_tag_dyneither(_tmp30D,sizeof(char),20);}),_tag_dyneither(_tmp30C,sizeof(void*),2));});});});_npop_handler(0);return _tmp310;}
# 1584
if(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp32B->impl))->tagged)*is_tagged=1;{
void*t2=_tmp30B->type;
if(_tmp32A != 0){
struct _RegionHandle _tmp311=_new_region("rgn");struct _RegionHandle*rgn=& _tmp311;_push_region(rgn);
{struct Cyc_List_List*_tmp312=((struct Cyc_List_List*(*)(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_rzip)(rgn,rgn,_tmp32B->tvs,_tmp32A);
t2=Cyc_Tcutil_rsubstitute(rgn,_tmp312,_tmp30B->type);}
# 1588
;_pop_region(rgn);}
# 1594
if((((_tmp32B->kind == Cyc_Absyn_UnionA  && !((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp32B->impl))->tagged) && !Cyc_Tcutil_bits_only(t2)) && !Cyc_Tcenv_in_notreadctxt(te)) && _tmp30B->requires_clause == 0){
void*_tmp316=({struct Cyc_String_pa_PrintArg_struct _tmp315;_tmp315.tag=0;_tmp315.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*f);({void*_tmp313[1]={& _tmp315};Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp314="cannot read union member %s since it is not `bits-only'";_tag_dyneither(_tmp314,sizeof(char),56);}),_tag_dyneither(_tmp313,sizeof(void*),1));});});_npop_handler(0);return _tmp316;}
if(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp32B->impl))->exist_vars != 0){
# 1599
if(!Cyc_Tcutil_unify(t2,Cyc_Absyn_wildtyp(Cyc_Tcenv_lookup_opt_type_vars(te)))){
void*_tmp31A=({struct Cyc_String_pa_PrintArg_struct _tmp319;_tmp319.tag=0;_tmp319.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*f);({void*_tmp317[1]={& _tmp319};Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp318="must use pattern-matching to access field %s\n\tdue to existential type variables.";_tag_dyneither(_tmp318,sizeof(char),81);}),_tag_dyneither(_tmp317,sizeof(void*),1));});});_npop_handler(0);return _tmp31A;}}{
# 1602
void*_tmp31B=t2;_npop_handler(0);return _tmp31B;};};}}else{goto _LL1DC;}case 12: _LL1DA: _tmp32D=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp30A)->f1;_tmp32C=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp30A)->f2;_LL1DB: {
# 1604
struct Cyc_Absyn_Aggrfield*_tmp31C=Cyc_Absyn_lookup_field(_tmp32C,f);
if(_tmp31C == 0){
void*_tmp320=({struct Cyc_String_pa_PrintArg_struct _tmp31F;_tmp31F.tag=0;_tmp31F.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*f);({void*_tmp31D[1]={& _tmp31F};Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp31E="type has no %s member";_tag_dyneither(_tmp31E,sizeof(char),22);}),_tag_dyneither(_tmp31D,sizeof(void*),1));});});_npop_handler(0);return _tmp320;}
# 1609
if(((_tmp32D == Cyc_Absyn_UnionA  && !Cyc_Tcutil_bits_only(_tmp31C->type)) && !Cyc_Tcenv_in_notreadctxt(te)) && _tmp31C->requires_clause == 0){
# 1611
void*_tmp324=({struct Cyc_String_pa_PrintArg_struct _tmp323;_tmp323.tag=0;_tmp323.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*f);({void*_tmp321[1]={& _tmp323};Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp322="cannot read union member %s since it is not `bits-only'";_tag_dyneither(_tmp322,sizeof(char),56);}),_tag_dyneither(_tmp321,sizeof(void*),1));});});_npop_handler(0);return _tmp324;}{
void*_tmp325=_tmp31C->type;_npop_handler(0);return _tmp325;};}default: _LL1DC: _LL1DD: {
# 1614
void*_tmp329=({struct Cyc_String_pa_PrintArg_struct _tmp328;_tmp328.tag=0;_tmp328.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string((void*)_check_null(e->topt)));({void*_tmp326[1]={& _tmp328};Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp327="expecting struct or union, found %s";_tag_dyneither(_tmp327,sizeof(char),36);}),_tag_dyneither(_tmp326,sizeof(void*),1));});});_npop_handler(0);return _tmp329;}}_LL1D7:;};
# 1572
;_pop_region(r);}
# 1620
static void*Cyc_Tcexp_tcAggrArrow(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct Cyc_Absyn_Exp*e,struct _dyneither_ptr*f,int*is_tagged,int*is_read){
# 1623
struct _RegionHandle _tmp32E=_new_region("r");struct _RegionHandle*r=& _tmp32E;_push_region(r);
Cyc_Tcexp_tcExp(Cyc_Tcenv_enter_abstract_val_ok(r,Cyc_Tcenv_clear_lhs(r,Cyc_Tcenv_clear_notreadctxt(r,te))),0,e);
# 1628
*is_read=!Cyc_Tcenv_in_notreadctxt(te);
{void*_tmp32F=Cyc_Tcutil_compress((void*)_check_null(e->topt));void*_tmp330=_tmp32F;void*_tmp35C;void*_tmp35B;union Cyc_Absyn_Constraint*_tmp35A;union Cyc_Absyn_Constraint*_tmp359;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp330)->tag == 5){_LL1DF: _tmp35C=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp330)->f1).elt_typ;_tmp35B=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp330)->f1).ptr_atts).rgn;_tmp35A=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp330)->f1).ptr_atts).bounds;_tmp359=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp330)->f1).ptr_atts).zero_term;_LL1E0:
# 1631
 Cyc_Tcutil_check_nonzero_bound(loc,_tmp35A);
{void*_tmp331=Cyc_Tcutil_compress(_tmp35C);void*_tmp332=_tmp331;enum Cyc_Absyn_AggrKind _tmp358;struct Cyc_List_List*_tmp357;struct Cyc_Absyn_Aggrdecl*_tmp356;struct Cyc_List_List*_tmp355;switch(*((int*)_tmp332)){case 11: if((((((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp332)->f1).aggr_info).KnownAggr).tag == 2){_LL1E4: _tmp356=*(((((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp332)->f1).aggr_info).KnownAggr).val;_tmp355=(((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp332)->f1).targs;_LL1E5: {
# 1634
struct Cyc_Absyn_Aggrfield*_tmp333=Cyc_Absyn_lookup_decl_field(_tmp356,f);
if(_tmp333 == 0){
void*_tmp338=({struct Cyc_String_pa_PrintArg_struct _tmp337;_tmp337.tag=0;_tmp337.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*f);({struct Cyc_String_pa_PrintArg_struct _tmp336;_tmp336.tag=0;_tmp336.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_qvar2string(_tmp356->name));({void*_tmp334[2]={& _tmp336,& _tmp337};Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp335="type %s has no %s member";_tag_dyneither(_tmp335,sizeof(char),25);}),_tag_dyneither(_tmp334,sizeof(void*),2));});});});_npop_handler(0);return _tmp338;}
# 1639
if(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp356->impl))->tagged)*is_tagged=1;{
void*t3=_tmp333->type;
if(_tmp355 != 0){
struct _RegionHandle _tmp339=_new_region("rgn");struct _RegionHandle*rgn=& _tmp339;_push_region(rgn);
{struct Cyc_List_List*_tmp33A=((struct Cyc_List_List*(*)(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_rzip)(rgn,rgn,_tmp356->tvs,_tmp355);
t3=Cyc_Tcutil_rsubstitute(rgn,_tmp33A,_tmp333->type);}
# 1643
;_pop_region(rgn);}{
# 1647
struct Cyc_Absyn_Kind*_tmp33B=Cyc_Tcutil_typ_kind(t3);
# 1650
if(Cyc_Tcutil_kind_leq(& Cyc_Tcutil_ak,_tmp33B) && !Cyc_Tcenv_abstract_val_ok(te)){
void*_tmp33C=Cyc_Tcutil_compress(t3);void*_tmp33D=_tmp33C;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp33D)->tag == 8){_LL1EB: _LL1EC:
 goto _LL1EA;}else{_LL1ED: _LL1EE: {
# 1654
void*_tmp341=({struct Cyc_String_pa_PrintArg_struct _tmp340;_tmp340.tag=0;_tmp340.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*f);({void*_tmp33E[1]={& _tmp340};Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp33F="cannot get member %s since its type is abstract";_tag_dyneither(_tmp33F,sizeof(char),48);}),_tag_dyneither(_tmp33E,sizeof(void*),1));});});_npop_handler(0);return _tmp341;}}_LL1EA:;}
# 1659
if((((_tmp356->kind == Cyc_Absyn_UnionA  && !((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp356->impl))->tagged) && !
Cyc_Tcutil_bits_only(t3)) && !Cyc_Tcenv_in_notreadctxt(te)) && _tmp333->requires_clause == 0){
# 1662
void*_tmp345=({struct Cyc_String_pa_PrintArg_struct _tmp344;_tmp344.tag=0;_tmp344.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*f);({void*_tmp342[1]={& _tmp344};Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp343="cannot read union member %s since it is not `bits-only'";_tag_dyneither(_tmp343,sizeof(char),56);}),_tag_dyneither(_tmp342,sizeof(void*),1));});});_npop_handler(0);return _tmp345;}
if(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp356->impl))->exist_vars != 0){
if(!Cyc_Tcutil_unify(t3,Cyc_Absyn_wildtyp(Cyc_Tcenv_lookup_opt_type_vars(te)))){
void*_tmp349=({struct Cyc_String_pa_PrintArg_struct _tmp348;_tmp348.tag=0;_tmp348.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*f);({void*_tmp346[1]={& _tmp348};Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp347="must use pattern-matching to access field %s\n\tdue to extistential types";_tag_dyneither(_tmp347,sizeof(char),72);}),_tag_dyneither(_tmp346,sizeof(void*),1));});});_npop_handler(0);return _tmp349;}}{
# 1669
void*_tmp34A=t3;_npop_handler(0);return _tmp34A;};};};}}else{goto _LL1E8;}case 12: _LL1E6: _tmp358=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp332)->f1;_tmp357=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp332)->f2;_LL1E7: {
# 1671
struct Cyc_Absyn_Aggrfield*_tmp34B=Cyc_Absyn_lookup_field(_tmp357,f);
if(_tmp34B == 0){
void*_tmp34F=({struct Cyc_String_pa_PrintArg_struct _tmp34E;_tmp34E.tag=0;_tmp34E.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*f);({void*_tmp34C[1]={& _tmp34E};Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp34D="type has no %s field";_tag_dyneither(_tmp34D,sizeof(char),21);}),_tag_dyneither(_tmp34C,sizeof(void*),1));});});_npop_handler(0);return _tmp34F;}
# 1676
if((_tmp358 == Cyc_Absyn_UnionA  && !Cyc_Tcutil_bits_only(_tmp34B->type)) && !Cyc_Tcenv_in_notreadctxt(te)){
void*_tmp353=({struct Cyc_String_pa_PrintArg_struct _tmp352;_tmp352.tag=0;_tmp352.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*f);({void*_tmp350[1]={& _tmp352};Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp351="cannot read union member %s since it is not `bits-only'";_tag_dyneither(_tmp351,sizeof(char),56);}),_tag_dyneither(_tmp350,sizeof(void*),1));});});_npop_handler(0);return _tmp353;}{
void*_tmp354=_tmp34B->type;_npop_handler(0);return _tmp354;};}default: _LL1E8: _LL1E9:
 goto _LL1E3;}_LL1E3:;}
# 1681
goto _LL1DE;}else{_LL1E1: _LL1E2:
 goto _LL1DE;}_LL1DE:;}{
# 1684
void*_tmp360=({struct Cyc_String_pa_PrintArg_struct _tmp35F;_tmp35F.tag=0;_tmp35F.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string((void*)_check_null(e->topt)));({void*_tmp35D[1]={& _tmp35F};Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp35E="expecting struct or union pointer, found %s";_tag_dyneither(_tmp35E,sizeof(char),44);}),_tag_dyneither(_tmp35D,sizeof(void*),1));});});_npop_handler(0);return _tmp360;};
# 1623
;_pop_region(r);}
# 1689
static void*Cyc_Tcexp_ithTupleType(struct Cyc_Tcenv_Tenv*te,unsigned int loc,struct Cyc_List_List*ts,struct Cyc_Absyn_Exp*index){
# 1691
struct _tuple15 _tmp361=Cyc_Evexp_eval_const_uint_exp(index);struct _tuple15 _tmp362=_tmp361;unsigned int _tmp370;int _tmp36F;_LL1F0: _tmp370=_tmp362.f1;_tmp36F=_tmp362.f2;_LL1F1:;
if(!_tmp36F)
return({void*_tmp363=0;Cyc_Tcexp_expr_err(te,loc,0,({const char*_tmp364="tuple projection cannot use sizeof or offsetof";_tag_dyneither(_tmp364,sizeof(char),47);}),_tag_dyneither(_tmp363,sizeof(void*),0));});{
# 1695
struct _handler_cons _tmp365;_push_handler(& _tmp365);{int _tmp367=0;if(setjmp(_tmp365.handler))_tmp367=1;if(!_tmp367){
{void*_tmp368=(*((struct _tuple18*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth)(ts,(int)_tmp370)).f2;_npop_handler(0);return _tmp368;};_pop_handler();}else{void*_tmp366=(void*)_exn_thrown;void*_tmp369=_tmp366;void*_tmp36E;if(((struct Cyc_List_Nth_exn_struct*)_tmp369)->tag == Cyc_List_Nth){_LL1F3: _LL1F4:
# 1698
 return({struct Cyc_Int_pa_PrintArg_struct _tmp36D;_tmp36D.tag=1;_tmp36D.f1=(unsigned long)
((int(*)(struct Cyc_List_List*x))Cyc_List_length)(ts);({struct Cyc_Int_pa_PrintArg_struct _tmp36C;_tmp36C.tag=1;_tmp36C.f1=(unsigned long)((int)_tmp370);({void*_tmp36A[2]={& _tmp36C,& _tmp36D};Cyc_Tcexp_expr_err(te,loc,0,({const char*_tmp36B="index is %d but tuple has only %d fields";_tag_dyneither(_tmp36B,sizeof(char),41);}),_tag_dyneither(_tmp36A,sizeof(void*),2));});});});}else{_LL1F5: _tmp36E=_tmp369;_LL1F6:(int)_rethrow(_tmp36E);}_LL1F2:;}};};}
# 1703
static void*Cyc_Tcexp_tcSubscript(struct Cyc_Tcenv_Tenv*te_orig,unsigned int loc,void**topt,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
# 1705
struct _RegionHandle _tmp371=_new_region("r");struct _RegionHandle*r=& _tmp371;_push_region(r);
{struct Cyc_Tcenv_Tenv*_tmp372=Cyc_Tcenv_clear_lhs(r,Cyc_Tcenv_clear_notreadctxt(r,te_orig));
Cyc_Tcexp_tcExp(Cyc_Tcenv_clear_abstract_val_ok(r,_tmp372),0,e1);
Cyc_Tcexp_tcExp(Cyc_Tcenv_clear_abstract_val_ok(r,_tmp372),0,e2);{
void*t1=Cyc_Tcutil_compress((void*)_check_null(e1->topt));
void*t2=Cyc_Tcutil_compress((void*)_check_null(e2->topt));
if(!Cyc_Tcutil_coerce_sint_typ(_tmp372,e2)){
void*_tmp376=({struct Cyc_String_pa_PrintArg_struct _tmp375;_tmp375.tag=0;_tmp375.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string(t2));({void*_tmp373[1]={& _tmp375};Cyc_Tcexp_expr_err(_tmp372,e2->loc,topt,({const char*_tmp374="expecting int subscript, found %s";_tag_dyneither(_tmp374,sizeof(char),34);}),_tag_dyneither(_tmp373,sizeof(void*),1));});});_npop_handler(0);return _tmp376;}{
# 1716
void*_tmp377=t1;struct Cyc_List_List*_tmp398;void*_tmp397;struct Cyc_Absyn_Tqual _tmp396;void*_tmp395;union Cyc_Absyn_Constraint*_tmp394;union Cyc_Absyn_Constraint*_tmp393;switch(*((int*)_tmp377)){case 5: _LL1F8: _tmp397=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp377)->f1).elt_typ;_tmp396=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp377)->f1).elt_tq;_tmp395=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp377)->f1).ptr_atts).rgn;_tmp394=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp377)->f1).ptr_atts).bounds;_tmp393=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp377)->f1).ptr_atts).zero_term;_LL1F9:
# 1720
 if(((int(*)(int y,union Cyc_Absyn_Constraint*x))Cyc_Absyn_conref_def)(0,_tmp393)){
int emit_warning=1;
{void*_tmp378=((void*(*)(void*y,union Cyc_Absyn_Constraint*x))Cyc_Absyn_conref_def)((void*)& Cyc_Absyn_DynEither_b_val,_tmp394);void*_tmp379=_tmp378;struct Cyc_Absyn_Exp*_tmp382;if(((struct Cyc_Absyn_Upper_b_Absyn_Bounds_struct*)_tmp379)->tag == 1){_LL1FF: _tmp382=((struct Cyc_Absyn_Upper_b_Absyn_Bounds_struct*)_tmp379)->f1;_LL200:
# 1724
 if(Cyc_Tcutil_is_const_exp(e2)){
struct _tuple15 _tmp37A=Cyc_Evexp_eval_const_uint_exp(e2);struct _tuple15 _tmp37B=_tmp37A;unsigned int _tmp381;int _tmp380;_LL204: _tmp381=_tmp37B.f1;_tmp380=_tmp37B.f2;_LL205:;
if(_tmp380){
struct _tuple15 _tmp37C=Cyc_Evexp_eval_const_uint_exp(_tmp382);struct _tuple15 _tmp37D=_tmp37C;unsigned int _tmp37F;int _tmp37E;_LL207: _tmp37F=_tmp37D.f1;_tmp37E=_tmp37D.f2;_LL208:;
if(_tmp37E  && _tmp37F > _tmp381)emit_warning=0;}}
# 1731
goto _LL1FE;}else{_LL201: _LL202:
 emit_warning=0;goto _LL1FE;}_LL1FE:;}
# 1734
if(emit_warning)
({void*_tmp383=0;Cyc_Tcutil_warn(e2->loc,({const char*_tmp384="subscript on thin, zero-terminated pointer could be expensive.";_tag_dyneither(_tmp384,sizeof(char),63);}),_tag_dyneither(_tmp383,sizeof(void*),0));});}else{
# 1738
if(Cyc_Tcutil_is_const_exp(e2)){
struct _tuple15 _tmp385=Cyc_Evexp_eval_const_uint_exp(e2);struct _tuple15 _tmp386=_tmp385;unsigned int _tmp388;int _tmp387;_LL20A: _tmp388=_tmp386.f1;_tmp387=_tmp386.f2;_LL20B:;
if(_tmp387)
Cyc_Tcutil_check_bound(loc,_tmp388,_tmp394);}else{
# 1744
if(Cyc_Tcutil_is_bound_one(_tmp394) && !((int(*)(int y,union Cyc_Absyn_Constraint*x))Cyc_Absyn_conref_def)(0,_tmp393))
({void*_tmp389=0;Cyc_Tcutil_warn(e1->loc,({const char*_tmp38A="subscript applied to pointer to one object";_tag_dyneither(_tmp38A,sizeof(char),43);}),_tag_dyneither(_tmp389,sizeof(void*),0));});
Cyc_Tcutil_check_nonzero_bound(loc,_tmp394);}}
# 1749
Cyc_Tcenv_check_rgn_accessible(_tmp372,loc,_tmp395);
if(!Cyc_Tcutil_kind_leq(Cyc_Tcutil_typ_kind(_tmp397),& Cyc_Tcutil_tmk) && !Cyc_Tcenv_abstract_val_ok(_tmp372))
({void*_tmp38B=0;Cyc_Tcutil_terr(e1->loc,({const char*_tmp38C="can't subscript an abstract pointer";_tag_dyneither(_tmp38C,sizeof(char),36);}),_tag_dyneither(_tmp38B,sizeof(void*),0));});{
void*_tmp38D=_tmp397;_npop_handler(0);return _tmp38D;};case 10: _LL1FA: _tmp398=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp377)->f1;_LL1FB: {
void*_tmp38E=Cyc_Tcexp_ithTupleType(_tmp372,loc,_tmp398,e2);_npop_handler(0);return _tmp38E;}default: _LL1FC: _LL1FD: {
void*_tmp392=({struct Cyc_String_pa_PrintArg_struct _tmp391;_tmp391.tag=0;_tmp391.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t1));({void*_tmp38F[1]={& _tmp391};Cyc_Tcexp_expr_err(_tmp372,loc,topt,({const char*_tmp390="subscript applied to %s";_tag_dyneither(_tmp390,sizeof(char),24);}),_tag_dyneither(_tmp38F,sizeof(void*),1));});});_npop_handler(0);return _tmp392;}}_LL1F7:;};};}
# 1706
;_pop_region(r);}
# 1760
static void*Cyc_Tcexp_tcTuple(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct Cyc_List_List*es){
int done=0;
struct Cyc_List_List*fields=0;
if(topt != 0){
void*_tmp399=Cyc_Tcutil_compress(*topt);void*_tmp39A=_tmp399;struct Cyc_List_List*_tmp39F;if(((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp39A)->tag == 10){_LL20D: _tmp39F=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp39A)->f1;_LL20E:
# 1766
 if(((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp39F)!= ((int(*)(struct Cyc_List_List*x))Cyc_List_length)(es))
# 1769
goto _LL20C;
# 1771
for(0;es != 0;(es=es->tl,_tmp39F=_tmp39F->tl)){
struct _RegionHandle _tmp39B=_new_region("r");struct _RegionHandle*r=& _tmp39B;_push_region(r);
{int bogus=0;
void*_tmp39C=(*((struct _tuple18*)((struct Cyc_List_List*)_check_null(_tmp39F))->hd)).f2;
Cyc_Tcexp_tcExpInitializer(Cyc_Tcenv_clear_abstract_val_ok(r,te),& _tmp39C,(struct Cyc_Absyn_Exp*)es->hd);
# 1777
Cyc_Tcutil_coerce_arg(te,(struct Cyc_Absyn_Exp*)es->hd,(*((struct _tuple18*)_tmp39F->hd)).f2,& bogus);
fields=({struct Cyc_List_List*_tmp39D=_cycalloc(sizeof(*_tmp39D));_tmp39D->hd=({struct _tuple18*_tmp39E=_cycalloc(sizeof(*_tmp39E));_tmp39E->f1=(*((struct _tuple18*)_tmp39F->hd)).f1;_tmp39E->f2=(void*)_check_null(((struct Cyc_Absyn_Exp*)es->hd)->topt);_tmp39E;});_tmp39D->tl=fields;_tmp39D;});}
# 1773
;_pop_region(r);}
# 1781
done=1;
goto _LL20C;}else{_LL20F: _LL210:
 goto _LL20C;}_LL20C:;}
# 1785
if(!done)
for(0;es != 0;es=es->tl){
struct _RegionHandle _tmp3A0=_new_region("r");struct _RegionHandle*r=& _tmp3A0;_push_region(r);
Cyc_Tcexp_tcExpInitializer(Cyc_Tcenv_clear_abstract_val_ok(r,te),0,(struct Cyc_Absyn_Exp*)es->hd);
fields=({struct Cyc_List_List*_tmp3A1=_cycalloc(sizeof(*_tmp3A1));_tmp3A1->hd=({struct _tuple18*_tmp3A2=_cycalloc(sizeof(*_tmp3A2));_tmp3A2->f1=Cyc_Absyn_empty_tqual(0);_tmp3A2->f2=(void*)_check_null(((struct Cyc_Absyn_Exp*)es->hd)->topt);_tmp3A2;});_tmp3A1->tl=fields;_tmp3A1;});
# 1788
;_pop_region(r);}
# 1792
return(void*)({struct Cyc_Absyn_TupleType_Absyn_Type_struct*_tmp3A3=_cycalloc(sizeof(*_tmp3A3));_tmp3A3[0]=({struct Cyc_Absyn_TupleType_Absyn_Type_struct _tmp3A4;_tmp3A4.tag=10;_tmp3A4.f1=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(fields);_tmp3A4;});_tmp3A3;});}
# 1796
static void*Cyc_Tcexp_tcCompoundLit(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct _tuple9*t,struct Cyc_List_List*des){
# 1799
return({void*_tmp3A5=0;Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp3A6="tcCompoundLit";_tag_dyneither(_tmp3A6,sizeof(char),14);}),_tag_dyneither(_tmp3A5,sizeof(void*),0));});}
# 1809 "tcexp.cyc"
static void*Cyc_Tcexp_tcArray(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**elt_topt,struct Cyc_Absyn_Tqual*elt_tqopt,int zero_term,struct Cyc_List_List*des){
# 1812
void*res_t2;
struct _RegionHandle _tmp3A7=_new_region("r");struct _RegionHandle*r=& _tmp3A7;_push_region(r);{
int _tmp3A8=((int(*)(struct Cyc_List_List*x))Cyc_List_length)(des);
struct Cyc_List_List*es=((struct Cyc_List_List*(*)(struct _RegionHandle*,struct Cyc_Absyn_Exp*(*f)(struct _tuple17*),struct Cyc_List_List*x))Cyc_List_rmap)(r,(struct Cyc_Absyn_Exp*(*)(struct _tuple17*))Cyc_Core_snd,des);
void*res=Cyc_Absyn_new_evar(& Cyc_Tcutil_tmko,Cyc_Tcenv_lookup_opt_type_vars(te));
struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_tmp3A9=({struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_tmp3B0=_cycalloc(sizeof(*_tmp3B0));_tmp3B0[0]=({struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct _tmp3B1;_tmp3B1.tag=0;_tmp3B1.f1=Cyc_Absyn_Int_c(Cyc_Absyn_Unsigned,_tmp3A8);_tmp3B1;});_tmp3B0;});
struct Cyc_Absyn_Exp*sz_exp=Cyc_Absyn_new_exp((void*)_tmp3A9,loc);
# 1821
if(zero_term){
struct Cyc_Absyn_Exp*_tmp3AA=((struct Cyc_Absyn_Exp*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth)(es,_tmp3A8 - 1);
if(!Cyc_Tcutil_is_zero(_tmp3AA))
({void*_tmp3AB=0;Cyc_Tcutil_terr(_tmp3AA->loc,({const char*_tmp3AC="zero-terminated array doesn't end with zero.";_tag_dyneither(_tmp3AC,sizeof(char),45);}),_tag_dyneither(_tmp3AB,sizeof(void*),0));});}
# 1826
sz_exp->topt=Cyc_Absyn_uint_typ;
res_t2=Cyc_Absyn_array_typ(res,
(unsigned int)elt_tqopt?*elt_tqopt: Cyc_Absyn_empty_tqual(0),sz_exp,
zero_term?Cyc_Absyn_true_conref: Cyc_Absyn_false_conref,0);
# 1831
{struct Cyc_List_List*es2=es;for(0;es2 != 0;es2=es2->tl){
Cyc_Tcexp_tcExpInitializer(te,elt_topt,(struct Cyc_Absyn_Exp*)es2->hd);}}
# 1834
if(!Cyc_Tcutil_coerce_list(te,res,es))
# 1836
({struct Cyc_String_pa_PrintArg_struct _tmp3AF;_tmp3AF.tag=0;_tmp3AF.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string(res));({void*_tmp3AD[1]={& _tmp3AF};Cyc_Tcutil_terr(((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd)->loc,({const char*_tmp3AE="elements of array do not all have the same type (%s)";_tag_dyneither(_tmp3AE,sizeof(char),53);}),_tag_dyneither(_tmp3AD,sizeof(void*),1));});});}
# 1840
{int offset=0;for(0;des != 0;(offset ++,des=des->tl)){
struct Cyc_List_List*ds=(*((struct _tuple17*)des->hd)).f1;
if(ds != 0){
# 1845
void*_tmp3B2=(void*)ds->hd;void*_tmp3B3=_tmp3B2;struct Cyc_Absyn_Exp*_tmp3C0;if(((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)_tmp3B3)->tag == 1){_LL212: _LL213:
# 1847
({void*_tmp3B4=0;Cyc_Tcutil_terr(loc,({const char*_tmp3B5="only array index designators are supported";_tag_dyneither(_tmp3B5,sizeof(char),43);}),_tag_dyneither(_tmp3B4,sizeof(void*),0));});
goto _LL211;}else{_LL214: _tmp3C0=((struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*)_tmp3B3)->f1;_LL215:
# 1850
 Cyc_Tcexp_tcExpInitializer(te,0,_tmp3C0);{
struct _tuple15 _tmp3B6=Cyc_Evexp_eval_const_uint_exp(_tmp3C0);struct _tuple15 _tmp3B7=_tmp3B6;unsigned int _tmp3BF;int _tmp3BE;_LL217: _tmp3BF=_tmp3B7.f1;_tmp3BE=_tmp3B7.f2;_LL218:;
if(!_tmp3BE)
({void*_tmp3B8=0;Cyc_Tcutil_terr(_tmp3C0->loc,({const char*_tmp3B9="index designator cannot use sizeof or offsetof";_tag_dyneither(_tmp3B9,sizeof(char),47);}),_tag_dyneither(_tmp3B8,sizeof(void*),0));});else{
if(_tmp3BF != offset)
({struct Cyc_Int_pa_PrintArg_struct _tmp3BD;_tmp3BD.tag=1;_tmp3BD.f1=(unsigned long)((int)_tmp3BF);({struct Cyc_Int_pa_PrintArg_struct _tmp3BC;_tmp3BC.tag=1;_tmp3BC.f1=(unsigned long)offset;({void*_tmp3BA[2]={& _tmp3BC,& _tmp3BD};Cyc_Tcutil_terr(_tmp3C0->loc,({const char*_tmp3BB="expecting index designator %d but found %d";_tag_dyneither(_tmp3BB,sizeof(char),43);}),_tag_dyneither(_tmp3BA,sizeof(void*),2));});});});}
# 1857
goto _LL211;};}_LL211:;}}}{
# 1861
void*_tmp3C1=res_t2;_npop_handler(0);return _tmp3C1;};
# 1813
;_pop_region(r);}
# 1865
static void*Cyc_Tcexp_tcComprehension(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct Cyc_Absyn_Vardecl*vd,struct Cyc_Absyn_Exp*bound,struct Cyc_Absyn_Exp*body,int*is_zero_term){
# 1868
Cyc_Tcexp_tcExp(te,0,bound);
{void*_tmp3C2=Cyc_Tcutil_compress((void*)_check_null(bound->topt));void*_tmp3C3=_tmp3C2;void*_tmp3C8;if(((struct Cyc_Absyn_TagType_Absyn_Type_struct*)_tmp3C3)->tag == 19){_LL21A: _tmp3C8=(void*)((struct Cyc_Absyn_TagType_Absyn_Type_struct*)_tmp3C3)->f1;_LL21B:
# 1873
 if(Cyc_Tcenv_new_status(te)== Cyc_Tcenv_InNewAggr){
struct Cyc_Absyn_Exp*_tmp3C4=Cyc_Absyn_cast_exp(Cyc_Absyn_uint_typ,Cyc_Absyn_valueof_exp(_tmp3C8,0),0,Cyc_Absyn_No_coercion,0);
_tmp3C4->topt=bound->topt;
# 1878
bound=_tmp3C4;}
# 1880
goto _LL219;}else{_LL21C: _LL21D:
# 1882
 if(!Cyc_Tcutil_coerce_uint_typ(te,bound))
({struct Cyc_String_pa_PrintArg_struct _tmp3C7;_tmp3C7.tag=0;_tmp3C7.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string((void*)_check_null(bound->topt)));({void*_tmp3C5[1]={& _tmp3C7};Cyc_Tcutil_terr(bound->loc,({const char*_tmp3C6="expecting unsigned int, found %s";_tag_dyneither(_tmp3C6,sizeof(char),33);}),_tag_dyneither(_tmp3C5,sizeof(void*),1));});});}_LL219:;}
# 1887
if(!(vd->tq).real_const)
({void*_tmp3C9=0;((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(({const char*_tmp3CA="comprehension index variable is not declared const!";_tag_dyneither(_tmp3CA,sizeof(char),52);}),_tag_dyneither(_tmp3C9,sizeof(void*),0));});{
# 1890
struct _RegionHandle _tmp3CB=_new_region("r");struct _RegionHandle*r=& _tmp3CB;_push_region(r);
{struct Cyc_Tcenv_Tenv*te2=Cyc_Tcenv_copy_tenv(r,te);
if(te2->le != 0){
te2=Cyc_Tcenv_new_block(r,loc,te2);
te2=Cyc_Tcenv_add_local_var(r,loc,te2,vd);}{
# 1896
struct Cyc_Tcenv_Tenv*_tmp3CC=te2;
void**_tmp3CD=0;
struct Cyc_Absyn_Tqual*_tmp3CE=0;
union Cyc_Absyn_Constraint**_tmp3CF=0;
# 1901
if(topt != 0){
void*_tmp3D0=Cyc_Tcutil_compress(*topt);void*_tmp3D1=_tmp3D0;void*_tmp3DC;struct Cyc_Absyn_Tqual _tmp3DB;struct Cyc_Absyn_Exp*_tmp3DA;union Cyc_Absyn_Constraint*_tmp3D9;struct Cyc_Absyn_PtrInfo _tmp3D8;switch(*((int*)_tmp3D1)){case 5: _LL21F: _tmp3D8=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3D1)->f1;_LL220:
# 1904
 _tmp3CD=({void**_tmp3D2=_region_malloc(r,sizeof(*_tmp3D2));_tmp3D2[0]=_tmp3D8.elt_typ;_tmp3D2;});
_tmp3CE=({struct Cyc_Absyn_Tqual*_tmp3D3=_region_malloc(r,sizeof(*_tmp3D3));_tmp3D3[0]=_tmp3D8.elt_tq;_tmp3D3;});
_tmp3CF=({union Cyc_Absyn_Constraint**_tmp3D4=_region_malloc(r,sizeof(*_tmp3D4));_tmp3D4[0]=(_tmp3D8.ptr_atts).zero_term;_tmp3D4;});
goto _LL21E;case 8: _LL221: _tmp3DC=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3D1)->f1).elt_type;_tmp3DB=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3D1)->f1).tq;_tmp3DA=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3D1)->f1).num_elts;_tmp3D9=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3D1)->f1).zero_term;_LL222:
# 1909
 _tmp3CD=({void**_tmp3D5=_region_malloc(r,sizeof(*_tmp3D5));_tmp3D5[0]=_tmp3DC;_tmp3D5;});
_tmp3CE=({struct Cyc_Absyn_Tqual*_tmp3D6=_region_malloc(r,sizeof(*_tmp3D6));_tmp3D6[0]=_tmp3DB;_tmp3D6;});
_tmp3CF=({union Cyc_Absyn_Constraint**_tmp3D7=_region_malloc(r,sizeof(*_tmp3D7));_tmp3D7[0]=_tmp3D9;_tmp3D7;});
goto _LL21E;default: _LL223: _LL224:
# 1914
 goto _LL21E;}_LL21E:;}{
# 1917
void*t=Cyc_Tcexp_tcExp(_tmp3CC,_tmp3CD,body);
# 1919
struct _RegionHandle _tmp3DD=_new_region("temp");struct _RegionHandle*temp=& _tmp3DD;_push_region(temp);
if(Cyc_Tcutil_is_noalias_pointer_or_aggr(temp,t) && !Cyc_Tcutil_is_noalias_path(temp,body))
({void*_tmp3DE=0;Cyc_Tcutil_terr(body->loc,({const char*_tmp3DF="Cannot consume non-unique paths; do swap instead";_tag_dyneither(_tmp3DF,sizeof(char),49);}),_tag_dyneither(_tmp3DE,sizeof(void*),0));});
# 1923
if(_tmp3CC->le == 0){
# 1925
if(!Cyc_Tcutil_is_const_exp(bound))
({void*_tmp3E0=0;Cyc_Tcutil_terr(bound->loc,({const char*_tmp3E1="bound is not constant";_tag_dyneither(_tmp3E1,sizeof(char),22);}),_tag_dyneither(_tmp3E0,sizeof(void*),0));});
if(!Cyc_Tcutil_is_const_exp(body))
({void*_tmp3E2=0;Cyc_Tcutil_terr(bound->loc,({const char*_tmp3E3="body is not constant";_tag_dyneither(_tmp3E3,sizeof(char),21);}),_tag_dyneither(_tmp3E2,sizeof(void*),0));});}
# 1930
if(_tmp3CF != 0  && ((int(*)(int y,union Cyc_Absyn_Constraint*x))Cyc_Absyn_conref_def)(0,*_tmp3CF)){
# 1933
struct Cyc_Absyn_Exp*_tmp3E4=Cyc_Absyn_uint_exp(1,0);_tmp3E4->topt=Cyc_Absyn_uint_typ;
bound=Cyc_Absyn_add_exp(bound,_tmp3E4,0);bound->topt=Cyc_Absyn_uint_typ;
*is_zero_term=1;}{
# 1937
struct _RegionHandle _tmp3E5=_new_region("r");struct _RegionHandle*r=& _tmp3E5;_push_region(r);
if(Cyc_Tcutil_is_noalias_pointer_or_aggr(r,(void*)_check_null(body->topt)) && !
Cyc_Tcutil_is_noalias_path(r,body))
({void*_tmp3E6=0;Cyc_Tcutil_terr(body->loc,({const char*_tmp3E7="Cannot consume non-unique paths; do swap instead";_tag_dyneither(_tmp3E7,sizeof(char),49);}),_tag_dyneither(_tmp3E6,sizeof(void*),0));});{
# 1943
void*_tmp3E8=Cyc_Absyn_array_typ(t,_tmp3CE == 0?Cyc_Absyn_empty_tqual(0):*_tmp3CE,bound,
_tmp3CF == 0?Cyc_Absyn_false_conref:*_tmp3CF,0);
void*_tmp3E9=_tmp3E8;_npop_handler(2);return _tmp3E9;};
# 1937
;_pop_region(r);};
# 1919
;_pop_region(temp);};};}
# 1891
;_pop_region(r);};}
# 1950
static void*Cyc_Tcexp_tcComprehensionNoinit(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct Cyc_Absyn_Exp*bound,void*t,int*is_zero_term){
# 1953
Cyc_Tcexp_tcExp(te,0,bound);
{void*_tmp3EA=Cyc_Tcutil_compress((void*)_check_null(bound->topt));void*_tmp3EB=_tmp3EA;void*_tmp3F0;if(((struct Cyc_Absyn_TagType_Absyn_Type_struct*)_tmp3EB)->tag == 19){_LL226: _tmp3F0=(void*)((struct Cyc_Absyn_TagType_Absyn_Type_struct*)_tmp3EB)->f1;_LL227:
# 1958
 if(Cyc_Tcenv_new_status(te)== Cyc_Tcenv_InNewAggr){
struct Cyc_Absyn_Exp*_tmp3EC=Cyc_Absyn_cast_exp(Cyc_Absyn_uint_typ,Cyc_Absyn_valueof_exp(_tmp3F0,0),0,Cyc_Absyn_No_coercion,0);
_tmp3EC->topt=bound->topt;
# 1963
bound=_tmp3EC;}
# 1965
goto _LL225;}else{_LL228: _LL229:
# 1967
 if(!Cyc_Tcutil_coerce_uint_typ(te,bound))
({struct Cyc_String_pa_PrintArg_struct _tmp3EF;_tmp3EF.tag=0;_tmp3EF.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string((void*)_check_null(bound->topt)));({void*_tmp3ED[1]={& _tmp3EF};Cyc_Tcutil_terr(bound->loc,({const char*_tmp3EE="expecting unsigned int, found %s";_tag_dyneither(_tmp3EE,sizeof(char),33);}),_tag_dyneither(_tmp3ED,sizeof(void*),1));});});}_LL225:;}{
# 1972
struct _RegionHandle _tmp3F1=_new_region("r");struct _RegionHandle*r=& _tmp3F1;_push_region(r);
{void**_tmp3F2=0;
struct Cyc_Absyn_Tqual*_tmp3F3=0;
union Cyc_Absyn_Constraint**_tmp3F4=0;
# 1977
if(topt != 0){
void*_tmp3F5=Cyc_Tcutil_compress(*topt);void*_tmp3F6=_tmp3F5;void*_tmp401;struct Cyc_Absyn_Tqual _tmp400;struct Cyc_Absyn_Exp*_tmp3FF;union Cyc_Absyn_Constraint*_tmp3FE;struct Cyc_Absyn_PtrInfo _tmp3FD;switch(*((int*)_tmp3F6)){case 5: _LL22B: _tmp3FD=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3F6)->f1;_LL22C:
# 1980
 _tmp3F2=({void**_tmp3F7=_region_malloc(r,sizeof(*_tmp3F7));_tmp3F7[0]=_tmp3FD.elt_typ;_tmp3F7;});
_tmp3F3=({struct Cyc_Absyn_Tqual*_tmp3F8=_region_malloc(r,sizeof(*_tmp3F8));_tmp3F8[0]=_tmp3FD.elt_tq;_tmp3F8;});
_tmp3F4=({union Cyc_Absyn_Constraint**_tmp3F9=_region_malloc(r,sizeof(*_tmp3F9));_tmp3F9[0]=(_tmp3FD.ptr_atts).zero_term;_tmp3F9;});
goto _LL22A;case 8: _LL22D: _tmp401=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3F6)->f1).elt_type;_tmp400=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3F6)->f1).tq;_tmp3FF=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3F6)->f1).num_elts;_tmp3FE=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3F6)->f1).zero_term;_LL22E:
# 1985
 _tmp3F2=({void**_tmp3FA=_region_malloc(r,sizeof(*_tmp3FA));_tmp3FA[0]=_tmp401;_tmp3FA;});
_tmp3F3=({struct Cyc_Absyn_Tqual*_tmp3FB=_region_malloc(r,sizeof(*_tmp3FB));_tmp3FB[0]=_tmp400;_tmp3FB;});
_tmp3F4=({union Cyc_Absyn_Constraint**_tmp3FC=_region_malloc(r,sizeof(*_tmp3FC));_tmp3FC[0]=_tmp3FE;_tmp3FC;});
goto _LL22A;default: _LL22F: _LL230:
# 1990
 goto _LL22A;}_LL22A:;}
# 1993
Cyc_Tcutil_check_type(loc,te,Cyc_Tcenv_lookup_type_vars(te),& Cyc_Tcutil_tmk,1,1,t);
if(_tmp3F2 != 0)Cyc_Tcutil_unify(*_tmp3F2,t);
# 1996
if(te->le == 0){
if(!Cyc_Tcutil_is_const_exp(bound))
({void*_tmp402=0;Cyc_Tcutil_terr(bound->loc,({const char*_tmp403="bound is not constant";_tag_dyneither(_tmp403,sizeof(char),22);}),_tag_dyneither(_tmp402,sizeof(void*),0));});}
# 2000
if(_tmp3F4 != 0  && ((int(*)(int y,union Cyc_Absyn_Constraint*x))Cyc_Absyn_conref_def)(0,*_tmp3F4)){
# 2003
struct Cyc_Absyn_Exp*_tmp404=Cyc_Absyn_uint_exp(1,0);_tmp404->topt=Cyc_Absyn_uint_typ;
bound=Cyc_Absyn_add_exp(bound,_tmp404,0);bound->topt=Cyc_Absyn_uint_typ;
*is_zero_term=1;
# 2007
({void*_tmp405=0;Cyc_Tcutil_terr(loc,({const char*_tmp406="non-initializing comprehensions do not currently support @zeroterm arrays";_tag_dyneither(_tmp406,sizeof(char),74);}),_tag_dyneither(_tmp405,sizeof(void*),0));});}{
# 2010
void*_tmp407=Cyc_Absyn_array_typ(t,_tmp3F3 == 0?Cyc_Absyn_empty_tqual(0):*_tmp3F3,bound,
_tmp3F4 == 0?Cyc_Absyn_false_conref:*_tmp3F4,0);
void*_tmp408=_tmp407;_npop_handler(0);return _tmp408;};}
# 1973
;_pop_region(r);};}struct _tuple19{struct Cyc_Absyn_Aggrfield*f1;struct Cyc_Absyn_Exp*f2;};
# 2026 "tcexp.cyc"
static void*Cyc_Tcexp_tcAggregate(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct _tuple1**tn,struct Cyc_List_List**ts,struct Cyc_List_List*args,struct Cyc_Absyn_Aggrdecl**ad_opt){
# 2030
struct Cyc_Absyn_Aggrdecl**adptr;
struct Cyc_Absyn_Aggrdecl*ad;
if(*ad_opt != 0){
ad=(struct Cyc_Absyn_Aggrdecl*)_check_null(*ad_opt);
adptr=({struct Cyc_Absyn_Aggrdecl**_tmp409=_cycalloc(sizeof(*_tmp409));_tmp409[0]=ad;_tmp409;});}else{
# 2036
{struct _handler_cons _tmp40A;_push_handler(& _tmp40A);{int _tmp40C=0;if(setjmp(_tmp40A.handler))_tmp40C=1;if(!_tmp40C){adptr=Cyc_Tcenv_lookup_aggrdecl(te,loc,*tn);
ad=*adptr;
# 2036
;_pop_handler();}else{void*_tmp40B=(void*)_exn_thrown;void*_tmp40D=_tmp40B;void*_tmp411;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp40D)->tag == Cyc_Dict_Absent){_LL232: _LL233:
# 2039
({struct Cyc_String_pa_PrintArg_struct _tmp410;_tmp410.tag=0;_tmp410.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(*tn));({void*_tmp40E[1]={& _tmp410};Cyc_Tcutil_terr(loc,({const char*_tmp40F="unbound struct/union name %s";_tag_dyneither(_tmp40F,sizeof(char),29);}),_tag_dyneither(_tmp40E,sizeof(void*),1));});});
return topt != 0?*topt:(void*)& Cyc_Absyn_VoidType_val;}else{_LL234: _tmp411=_tmp40D;_LL235:(int)_rethrow(_tmp411);}_LL231:;}};}
# 2042
*ad_opt=ad;
*tn=ad->name;}
# 2045
if(ad->impl == 0){
({struct Cyc_String_pa_PrintArg_struct _tmp414;_tmp414.tag=0;_tmp414.f1=(struct _dyneither_ptr)(ad->kind == Cyc_Absyn_StructA?({const char*_tmp415="struct";_tag_dyneither(_tmp415,sizeof(char),7);}):({const char*_tmp416="union";_tag_dyneither(_tmp416,sizeof(char),6);}));({void*_tmp412[1]={& _tmp414};Cyc_Tcutil_terr(loc,({const char*_tmp413="can't construct abstract %s";_tag_dyneither(_tmp413,sizeof(char),28);}),_tag_dyneither(_tmp412,sizeof(void*),1));});});
return Cyc_Absyn_wildtyp(Cyc_Tcenv_lookup_opt_type_vars(te));}{
# 2049
struct _RegionHandle _tmp417=_new_region("rgn");struct _RegionHandle*rgn=& _tmp417;_push_region(rgn);
# 2054
{struct Cyc_Tcenv_Tenv*te2;
enum Cyc_Tcenv_NewStatus _tmp418=Cyc_Tcenv_new_status(te);
if(_tmp418 == Cyc_Tcenv_InNew)
te2=Cyc_Tcenv_set_new_status(rgn,Cyc_Tcenv_InNewAggr,te);else{
# 2063
te2=Cyc_Tcenv_set_new_status(rgn,_tmp418,te);}{
# 2065
struct _tuple12 _tmp419=({struct _tuple12 _tmp441;_tmp441.f1=Cyc_Tcenv_lookup_type_vars(te2);_tmp441.f2=rgn;_tmp441;});
struct Cyc_List_List*_tmp41A=((struct Cyc_List_List*(*)(struct _RegionHandle*,struct _tuple13*(*f)(struct _tuple12*,struct Cyc_Absyn_Tvar*),struct _tuple12*env,struct Cyc_List_List*x))Cyc_List_rmap_c)(rgn,Cyc_Tcutil_r_make_inst_var,& _tmp419,ad->tvs);
struct Cyc_List_List*_tmp41B=((struct Cyc_List_List*(*)(struct _RegionHandle*,struct _tuple13*(*f)(struct _tuple12*,struct Cyc_Absyn_Tvar*),struct _tuple12*env,struct Cyc_List_List*x))Cyc_List_rmap_c)(rgn,Cyc_Tcutil_r_make_inst_var,& _tmp419,((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->exist_vars);
struct Cyc_List_List*_tmp41C=((struct Cyc_List_List*(*)(void*(*f)(struct _tuple13*),struct Cyc_List_List*x))Cyc_List_map)((void*(*)(struct _tuple13*))Cyc_Core_snd,_tmp41A);
struct Cyc_List_List*_tmp41D=((struct Cyc_List_List*(*)(void*(*f)(struct _tuple13*),struct Cyc_List_List*x))Cyc_List_map)((void*(*)(struct _tuple13*))Cyc_Core_snd,_tmp41B);
struct Cyc_List_List*_tmp41E=((struct Cyc_List_List*(*)(struct _RegionHandle*,struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_rappend)(rgn,_tmp41A,_tmp41B);
void*res_typ;
# 2076
if(topt != 0){
void*_tmp41F=Cyc_Tcutil_compress(*topt);void*_tmp420=_tmp41F;struct Cyc_Absyn_Aggrdecl**_tmp426;struct Cyc_List_List*_tmp425;if(((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp420)->tag == 11){if((((((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp420)->f1).aggr_info).KnownAggr).tag == 2){_LL237: _tmp426=(((((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp420)->f1).aggr_info).KnownAggr).val;_tmp425=(((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp420)->f1).targs;_LL238:
# 2079
 if(*_tmp426 == *adptr){
{struct Cyc_List_List*_tmp421=_tmp41C;for(0;_tmp421 != 0  && _tmp425 != 0;
(_tmp421=_tmp421->tl,_tmp425=_tmp425->tl)){
Cyc_Tcutil_unify((void*)_tmp421->hd,(void*)_tmp425->hd);}}
# 2084
res_typ=*topt;
goto _LL236;}
# 2087
goto _LL23A;}else{goto _LL239;}}else{_LL239: _LL23A:
# 2089
 res_typ=(void*)({struct Cyc_Absyn_AggrType_Absyn_Type_struct*_tmp422=_cycalloc(sizeof(*_tmp422));_tmp422[0]=({struct Cyc_Absyn_AggrType_Absyn_Type_struct _tmp423;_tmp423.tag=11;_tmp423.f1=({struct Cyc_Absyn_AggrInfo _tmp424;_tmp424.aggr_info=Cyc_Absyn_KnownAggr(adptr);_tmp424.targs=_tmp41C;_tmp424;});_tmp423;});_tmp422;});}_LL236:;}else{
# 2092
res_typ=(void*)({struct Cyc_Absyn_AggrType_Absyn_Type_struct*_tmp427=_cycalloc(sizeof(*_tmp427));_tmp427[0]=({struct Cyc_Absyn_AggrType_Absyn_Type_struct _tmp428;_tmp428.tag=11;_tmp428.f1=({struct Cyc_Absyn_AggrInfo _tmp429;_tmp429.aggr_info=Cyc_Absyn_KnownAggr(adptr);_tmp429.targs=_tmp41C;_tmp429;});_tmp428;});_tmp427;});}{
# 2095
struct Cyc_List_List*_tmp42A=*ts;
struct Cyc_List_List*_tmp42B=_tmp41D;
while(_tmp42A != 0  && _tmp42B != 0){
# 2099
Cyc_Tcutil_check_type(loc,te2,Cyc_Tcenv_lookup_type_vars(te2),& Cyc_Tcutil_ak,1,0,(void*)_tmp42A->hd);
Cyc_Tcutil_check_no_qual(loc,(void*)_tmp42A->hd);
Cyc_Tcutil_unify((void*)_tmp42A->hd,(void*)_tmp42B->hd);
_tmp42A=_tmp42A->tl;
_tmp42B=_tmp42B->tl;}
# 2105
if(_tmp42A != 0)
({void*_tmp42C=0;Cyc_Tcutil_terr(loc,({const char*_tmp42D="too many explicit witness types";_tag_dyneither(_tmp42D,sizeof(char),32);}),_tag_dyneither(_tmp42C,sizeof(void*),0));});
# 2108
*ts=_tmp41D;{
# 2111
struct Cyc_List_List*fields=
((struct Cyc_List_List*(*)(struct _RegionHandle*rgn,unsigned int loc,struct Cyc_List_List*des,enum Cyc_Absyn_AggrKind,struct Cyc_List_List*fields))Cyc_Tcutil_resolve_aggregate_designators)(rgn,loc,args,ad->kind,((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields);
for(0;fields != 0;fields=fields->tl){
int bogus=0;
struct _tuple19*_tmp42E=(struct _tuple19*)fields->hd;struct _tuple19*_tmp42F=_tmp42E;struct Cyc_Absyn_Aggrfield*_tmp43B;struct Cyc_Absyn_Exp*_tmp43A;_LL23C: _tmp43B=_tmp42F->f1;_tmp43A=_tmp42F->f2;_LL23D:;{
void*_tmp430=Cyc_Tcutil_rsubstitute(rgn,_tmp41E,_tmp43B->type);
Cyc_Tcexp_tcExpInitializer(Cyc_Tcenv_clear_abstract_val_ok(rgn,te2),& _tmp430,_tmp43A);
# 2122
if(!Cyc_Tcutil_coerce_arg(te2,_tmp43A,_tmp430,& bogus)){
({struct Cyc_String_pa_PrintArg_struct _tmp437;_tmp437.tag=0;_tmp437.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
# 2126
Cyc_Absynpp_typ2string((void*)_check_null(_tmp43A->topt)));({struct Cyc_String_pa_PrintArg_struct _tmp436;_tmp436.tag=0;_tmp436.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
# 2125
Cyc_Absynpp_typ2string(_tmp430));({struct Cyc_String_pa_PrintArg_struct _tmp435;_tmp435.tag=0;_tmp435.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(*tn));({struct Cyc_String_pa_PrintArg_struct _tmp434;_tmp434.tag=0;_tmp434.f1=(struct _dyneither_ptr)(
# 2124
ad->kind == Cyc_Absyn_StructA?({const char*_tmp438="struct";_tag_dyneither(_tmp438,sizeof(char),7);}):({const char*_tmp439="union";_tag_dyneither(_tmp439,sizeof(char),6);}));({struct Cyc_String_pa_PrintArg_struct _tmp433;_tmp433.tag=0;_tmp433.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*_tmp43B->name);({void*_tmp431[5]={& _tmp433,& _tmp434,& _tmp435,& _tmp436,& _tmp437};Cyc_Tcutil_terr(_tmp43A->loc,({const char*_tmp432="field %s of %s %s expects type %s != %s";_tag_dyneither(_tmp432,sizeof(char),40);}),_tag_dyneither(_tmp431,sizeof(void*),5));});});});});});});
# 2127
Cyc_Tcutil_explain_failure();}};}{
# 2131
struct Cyc_List_List*_tmp43C=0;
{struct Cyc_List_List*_tmp43D=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->rgn_po;for(0;_tmp43D != 0;_tmp43D=_tmp43D->tl){
_tmp43C=({struct Cyc_List_List*_tmp43E=_cycalloc(sizeof(*_tmp43E));_tmp43E->hd=({struct _tuple0*_tmp43F=_cycalloc(sizeof(*_tmp43F));_tmp43F->f1=Cyc_Tcutil_rsubstitute(rgn,_tmp41E,(*((struct _tuple0*)_tmp43D->hd)).f1);_tmp43F->f2=
Cyc_Tcutil_rsubstitute(rgn,_tmp41E,(*((struct _tuple0*)_tmp43D->hd)).f2);_tmp43F;});_tmp43E->tl=_tmp43C;_tmp43E;});}}
# 2136
_tmp43C=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(_tmp43C);
Cyc_Tcenv_check_rgn_partial_order(te2,loc,_tmp43C);{
void*_tmp440=res_typ;_npop_handler(0);return _tmp440;};};};};};}
# 2054
;_pop_region(rgn);};}
# 2143
static void*Cyc_Tcexp_tcAnonStruct(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void*ts,struct Cyc_List_List*args){
# 2145
struct _RegionHandle _tmp442=_new_region("rgn");struct _RegionHandle*rgn=& _tmp442;_push_region(rgn);{
void*_tmp443=Cyc_Tcutil_compress(ts);void*_tmp444=_tmp443;enum Cyc_Absyn_AggrKind _tmp453;struct Cyc_List_List*_tmp452;if(((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp444)->tag == 12){_LL23F: _tmp453=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp444)->f1;_tmp452=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp444)->f2;_LL240:
# 2148
 if(_tmp453 == Cyc_Absyn_UnionA)
({void*_tmp445=0;Cyc_Tcutil_terr(loc,({const char*_tmp446="expecting struct but found union";_tag_dyneither(_tmp446,sizeof(char),33);}),_tag_dyneither(_tmp445,sizeof(void*),0));});{
struct Cyc_List_List*fields=
((struct Cyc_List_List*(*)(struct _RegionHandle*rgn,unsigned int loc,struct Cyc_List_List*des,enum Cyc_Absyn_AggrKind,struct Cyc_List_List*fields))Cyc_Tcutil_resolve_aggregate_designators)(rgn,loc,args,Cyc_Absyn_StructA,_tmp452);
for(0;fields != 0;fields=fields->tl){
int bogus=0;
struct _tuple19*_tmp447=(struct _tuple19*)fields->hd;struct _tuple19*_tmp448=_tmp447;struct Cyc_Absyn_Aggrfield*_tmp44F;struct Cyc_Absyn_Exp*_tmp44E;_LL244: _tmp44F=_tmp448->f1;_tmp44E=_tmp448->f2;_LL245:;
Cyc_Tcexp_tcExpInitializer(Cyc_Tcenv_clear_abstract_val_ok(rgn,te),& _tmp44F->type,_tmp44E);
# 2157
if(!Cyc_Tcutil_coerce_arg(te,_tmp44E,_tmp44F->type,& bogus)){
({struct Cyc_String_pa_PrintArg_struct _tmp44D;_tmp44D.tag=0;_tmp44D.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
# 2160
Cyc_Absynpp_typ2string((void*)_check_null(_tmp44E->topt)));({struct Cyc_String_pa_PrintArg_struct _tmp44C;_tmp44C.tag=0;_tmp44C.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
# 2159
Cyc_Absynpp_typ2string(_tmp44F->type));({struct Cyc_String_pa_PrintArg_struct _tmp44B;_tmp44B.tag=0;_tmp44B.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*_tmp44F->name);({void*_tmp449[3]={& _tmp44B,& _tmp44C,& _tmp44D};Cyc_Tcutil_terr(_tmp44E->loc,({const char*_tmp44A="field %s of struct expects type %s != %s";_tag_dyneither(_tmp44A,sizeof(char),41);}),_tag_dyneither(_tmp449,sizeof(void*),3));});});});});
# 2161
Cyc_Tcutil_explain_failure();}}
# 2164
goto _LL23E;};}else{_LL241: _LL242:
({void*_tmp450=0;((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(({const char*_tmp451="tcAnonStruct: wrong type";_tag_dyneither(_tmp451,sizeof(char),25);}),_tag_dyneither(_tmp450,sizeof(void*),0));});}_LL23E:;}{
# 2168
void*_tmp454=ts;_npop_handler(0);return _tmp454;};
# 2145
;_pop_region(rgn);}
# 2172
static void*Cyc_Tcexp_tcDatatype(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct Cyc_Absyn_Exp*e,struct Cyc_List_List*es,struct Cyc_Absyn_Datatypedecl*tud,struct Cyc_Absyn_Datatypefield*tuf){
# 2175
struct _RegionHandle _tmp455=_new_region("rgn");struct _RegionHandle*rgn=& _tmp455;_push_region(rgn);
{struct _tuple12 _tmp456=({struct _tuple12 _tmp46D;_tmp46D.f1=Cyc_Tcenv_lookup_type_vars(te);_tmp46D.f2=rgn;_tmp46D;});
struct Cyc_List_List*_tmp457=((struct Cyc_List_List*(*)(struct _RegionHandle*,struct _tuple13*(*f)(struct _tuple12*,struct Cyc_Absyn_Tvar*),struct _tuple12*env,struct Cyc_List_List*x))Cyc_List_rmap_c)(rgn,Cyc_Tcutil_r_make_inst_var,& _tmp456,tud->tvs);
struct Cyc_List_List*_tmp458=((struct Cyc_List_List*(*)(void*(*f)(struct _tuple13*),struct Cyc_List_List*x))Cyc_List_map)((void*(*)(struct _tuple13*))Cyc_Core_snd,_tmp457);
void*res=(void*)({struct Cyc_Absyn_DatatypeFieldType_Absyn_Type_struct*_tmp46A=_cycalloc(sizeof(*_tmp46A));_tmp46A[0]=({struct Cyc_Absyn_DatatypeFieldType_Absyn_Type_struct _tmp46B;_tmp46B.tag=4;_tmp46B.f1=({struct Cyc_Absyn_DatatypeFieldInfo _tmp46C;_tmp46C.field_info=
Cyc_Absyn_KnownDatatypefield(tud,tuf);_tmp46C.targs=_tmp458;_tmp46C;});_tmp46B;});_tmp46A;});
# 2183
if(topt != 0){
void*_tmp459=Cyc_Tcutil_compress(*topt);void*_tmp45A=_tmp459;if(((struct Cyc_Absyn_DatatypeFieldType_Absyn_Type_struct*)_tmp45A)->tag == 4){_LL247: _LL248:
 Cyc_Tcutil_unify(*topt,res);goto _LL246;}else{_LL249: _LL24A:
 goto _LL246;}_LL246:;}{
# 2189
struct Cyc_List_List*ts=tuf->typs;
for(0;es != 0  && ts != 0;(es=es->tl,ts=ts->tl)){
int bogus=0;
struct Cyc_Absyn_Exp*e=(struct Cyc_Absyn_Exp*)es->hd;
void*t=(*((struct _tuple18*)ts->hd)).f2;
if(_tmp457 != 0)t=Cyc_Tcutil_rsubstitute(rgn,_tmp457,t);
Cyc_Tcexp_tcExpInitializer(te,& t,e);
if(!Cyc_Tcutil_coerce_arg(te,e,t,& bogus)){
({struct Cyc_String_pa_PrintArg_struct _tmp45F;_tmp45F.tag=0;_tmp45F.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)(
# 2200
e->topt == 0?(struct _dyneither_ptr)({const char*_tmp460="?";_tag_dyneither(_tmp460,sizeof(char),2);}): Cyc_Absynpp_typ2string((void*)_check_null(e->topt))));({struct Cyc_String_pa_PrintArg_struct _tmp45E;_tmp45E.tag=0;_tmp45E.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
# 2199
Cyc_Absynpp_typ2string(t));({struct Cyc_String_pa_PrintArg_struct _tmp45D;_tmp45D.tag=0;_tmp45D.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_qvar2string(tuf->name));({void*_tmp45B[3]={& _tmp45D,& _tmp45E,& _tmp45F};Cyc_Tcutil_terr(e->loc,({const char*_tmp45C="datatype constructor %s expects argument of type %s but this argument has type %s";_tag_dyneither(_tmp45C,sizeof(char),82);}),_tag_dyneither(_tmp45B,sizeof(void*),3));});});});});
# 2201
Cyc_Tcutil_explain_failure();}}
# 2204
if(es != 0){
void*_tmp464=({struct Cyc_String_pa_PrintArg_struct _tmp463;_tmp463.tag=0;_tmp463.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
# 2207
Cyc_Absynpp_qvar2string(tuf->name));({void*_tmp461[1]={& _tmp463};Cyc_Tcexp_expr_err(te,((struct Cyc_Absyn_Exp*)es->hd)->loc,topt,({const char*_tmp462="too many arguments for datatype constructor %s";_tag_dyneither(_tmp462,sizeof(char),47);}),_tag_dyneither(_tmp461,sizeof(void*),1));});});_npop_handler(0);return _tmp464;}
if(ts != 0){
void*_tmp468=({struct Cyc_String_pa_PrintArg_struct _tmp467;_tmp467.tag=0;_tmp467.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_qvar2string(tuf->name));({void*_tmp465[1]={& _tmp467};Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp466="too few arguments for datatype constructor %s";_tag_dyneither(_tmp466,sizeof(char),46);}),_tag_dyneither(_tmp465,sizeof(void*),1));});});_npop_handler(0);return _tmp468;}{
void*_tmp469=res;_npop_handler(0);return _tmp469;};};}
# 2176
;_pop_region(rgn);}
# 2215
static void Cyc_Tcexp_check_malloc_type(int allow_zero,unsigned int loc,void**topt,void*t){
# 2217
if(Cyc_Tcutil_bits_only(t) || allow_zero  && Cyc_Tcutil_zeroable_type(t))return;
# 2219
if(topt != 0){
void*_tmp46E=Cyc_Tcutil_compress(*topt);void*_tmp46F=_tmp46E;void*_tmp470;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp46F)->tag == 5){_LL24C: _tmp470=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp46F)->f1).elt_typ;_LL24D:
# 2222
 Cyc_Tcutil_unify(_tmp470,t);
if(Cyc_Tcutil_bits_only(t) || allow_zero  && Cyc_Tcutil_zeroable_type(t))return;
goto _LL24B;}else{_LL24E: _LL24F:
 goto _LL24B;}_LL24B:;}
# 2228
({struct Cyc_String_pa_PrintArg_struct _tmp474;_tmp474.tag=0;_tmp474.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string(t));({struct Cyc_String_pa_PrintArg_struct _tmp473;_tmp473.tag=0;_tmp473.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)(allow_zero?(struct _dyneither_ptr)({const char*_tmp475="calloc";_tag_dyneither(_tmp475,sizeof(char),7);}):(struct _dyneither_ptr)({const char*_tmp476="malloc";_tag_dyneither(_tmp476,sizeof(char),7);})));({void*_tmp471[2]={& _tmp473,& _tmp474};Cyc_Tcutil_terr(loc,({const char*_tmp472="%s cannot be used with type %s\n\t(type needs initialization)";_tag_dyneither(_tmp472,sizeof(char),60);}),_tag_dyneither(_tmp471,sizeof(void*),2));});});});}
# 2232
static void*Cyc_Tcexp_mallocRgn(void*rgn){
# 2234
enum Cyc_Absyn_AliasQual _tmp477=(Cyc_Tcutil_typ_kind(Cyc_Tcutil_compress(rgn)))->aliasqual;enum Cyc_Absyn_AliasQual _tmp478=_tmp477;if(_tmp478 == Cyc_Absyn_Unique){_LL251: _LL252:
 return(void*)& Cyc_Absyn_UniqueRgn_val;}else{_LL253: _LL254:
 return(void*)& Cyc_Absyn_HeapRgn_val;}_LL250:;}
# 2240
static void*Cyc_Tcexp_tcMalloc(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct Cyc_Absyn_Exp**ropt,void***t,struct Cyc_Absyn_Exp**e,int*is_calloc,int*is_fat){
# 2245
void*rgn=(void*)& Cyc_Absyn_HeapRgn_val;
if(*ropt != 0){
# 2248
void*expected_type=(void*)({struct Cyc_Absyn_RgnHandleType_Absyn_Type_struct*_tmp47F=_cycalloc(sizeof(*_tmp47F));_tmp47F[0]=({struct Cyc_Absyn_RgnHandleType_Absyn_Type_struct _tmp480;_tmp480.tag=15;_tmp480.f1=
Cyc_Absyn_new_evar(& Cyc_Tcutil_trko,Cyc_Tcenv_lookup_opt_type_vars(te));_tmp480;});_tmp47F;});
void*handle_type=Cyc_Tcexp_tcExp(te,& expected_type,(struct Cyc_Absyn_Exp*)_check_null(*ropt));
void*_tmp479=Cyc_Tcutil_compress(handle_type);void*_tmp47A=_tmp479;void*_tmp47E;if(((struct Cyc_Absyn_RgnHandleType_Absyn_Type_struct*)_tmp47A)->tag == 15){_LL256: _tmp47E=(void*)((struct Cyc_Absyn_RgnHandleType_Absyn_Type_struct*)_tmp47A)->f1;_LL257:
# 2253
 rgn=_tmp47E;
Cyc_Tcenv_check_rgn_accessible(te,loc,rgn);
goto _LL255;}else{_LL258: _LL259:
# 2257
({struct Cyc_String_pa_PrintArg_struct _tmp47D;_tmp47D.tag=0;_tmp47D.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string(handle_type));({void*_tmp47B[1]={& _tmp47D};Cyc_Tcutil_terr(((struct Cyc_Absyn_Exp*)_check_null(*ropt))->loc,({const char*_tmp47C="expecting region_t type but found %s";_tag_dyneither(_tmp47C,sizeof(char),37);}),_tag_dyneither(_tmp47B,sizeof(void*),1));});});
goto _LL255;}_LL255:;}else{
# 2264
if(topt != 0){
void*optrgn=(void*)& Cyc_Absyn_VoidType_val;
if(Cyc_Tcutil_rgn_of_pointer(*topt,& optrgn)){
rgn=Cyc_Tcexp_mallocRgn(optrgn);
if(rgn == (void*)& Cyc_Absyn_UniqueRgn_val)*ropt=Cyc_Absyn_uniquergn_exp;}}}
# 2271
{struct _RegionHandle _tmp481=_new_region("r");struct _RegionHandle*r=& _tmp481;_push_region(r);
Cyc_Tcexp_tcExp(Cyc_Tcenv_clear_abstract_val_ok(r,te),& Cyc_Absyn_uint_typ,*e);;_pop_region(r);}{
# 2282 "tcexp.cyc"
void*elt_type;
struct Cyc_Absyn_Exp*num_elts;
int one_elt;
if(*is_calloc){
if(*t == 0)({void*_tmp482=0;((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(({const char*_tmp483="calloc with empty type";_tag_dyneither(_tmp483,sizeof(char),23);}),_tag_dyneither(_tmp482,sizeof(void*),0));});
elt_type=*((void**)_check_null(*t));
Cyc_Tcutil_check_type(loc,te,Cyc_Tcenv_lookup_type_vars(te),& Cyc_Tcutil_tmk,1,0,elt_type);
Cyc_Tcutil_check_no_qual(loc,elt_type);
Cyc_Tcexp_check_malloc_type(1,loc,topt,elt_type);
num_elts=*e;
one_elt=0;}else{
# 2294
void*er=(*e)->r;
retry_sizeof: {
void*_tmp484=er;struct Cyc_Absyn_Exp*_tmp49A;struct Cyc_Absyn_Exp*_tmp499;void*_tmp498;switch(*((int*)_tmp484)){case 16: _LL25B: _tmp498=(void*)((struct Cyc_Absyn_Sizeoftyp_e_Absyn_Raw_exp_struct*)_tmp484)->f1;_LL25C:
# 2298
 elt_type=_tmp498;
*t=({void**_tmp485=_cycalloc(sizeof(*_tmp485));_tmp485[0]=elt_type;_tmp485;});
num_elts=Cyc_Absyn_uint_exp(1,0);
Cyc_Tcexp_tcExp(te,& Cyc_Absyn_uint_typ,num_elts);
one_elt=1;
goto _LL25A;case 2: if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp484)->f1 == Cyc_Absyn_Times){if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp484)->f2 != 0){if(((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp484)->f2)->tl != 0){if(((struct Cyc_List_List*)((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp484)->f2)->tl)->tl == 0){_LL25D: _tmp49A=(struct Cyc_Absyn_Exp*)(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp484)->f2)->hd;_tmp499=(struct Cyc_Absyn_Exp*)((((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp484)->f2)->tl)->hd;_LL25E:
# 2305
{struct _tuple0 _tmp486=({struct _tuple0 _tmp48C;_tmp48C.f1=_tmp49A->r;_tmp48C.f2=_tmp499->r;_tmp48C;});struct _tuple0 _tmp487=_tmp486;void*_tmp48B;void*_tmp48A;if(((struct Cyc_Absyn_Sizeoftyp_e_Absyn_Raw_exp_struct*)_tmp487.f1)->tag == 16){_LL262: _tmp48A=(void*)((struct Cyc_Absyn_Sizeoftyp_e_Absyn_Raw_exp_struct*)_tmp487.f1)->f1;_LL263:
# 2307
 Cyc_Tcexp_check_malloc_type(0,loc,topt,_tmp48A);
elt_type=_tmp48A;
*t=({void**_tmp488=_cycalloc(sizeof(*_tmp488));_tmp488[0]=elt_type;_tmp488;});
num_elts=_tmp499;
one_elt=0;
goto _LL261;}else{if(((struct Cyc_Absyn_Sizeoftyp_e_Absyn_Raw_exp_struct*)_tmp487.f2)->tag == 16){_LL264: _tmp48B=(void*)((struct Cyc_Absyn_Sizeoftyp_e_Absyn_Raw_exp_struct*)_tmp487.f2)->f1;_LL265:
# 2314
 Cyc_Tcexp_check_malloc_type(0,loc,topt,_tmp48B);
elt_type=_tmp48B;
*t=({void**_tmp489=_cycalloc(sizeof(*_tmp489));_tmp489[0]=elt_type;_tmp489;});
num_elts=_tmp49A;
one_elt=0;
goto _LL261;}else{_LL266: _LL267:
 goto No_sizeof;}}_LL261:;}
# 2322
goto _LL25A;}else{goto _LL25F;}}else{goto _LL25F;}}else{goto _LL25F;}}else{goto _LL25F;}default: _LL25F: _LL260:
# 2324
 No_sizeof: {
# 2327
struct Cyc_Absyn_Exp*_tmp48D=*e;
{void*_tmp48E=_tmp48D->r;void*_tmp48F=_tmp48E;struct Cyc_Absyn_Exp*_tmp490;if(((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp48F)->tag == 13){_LL269: _tmp490=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp48F)->f2;_LL26A:
 _tmp48D=_tmp490;goto _LL268;}else{_LL26B: _LL26C:
 goto _LL268;}_LL268:;}
# 2332
{void*_tmp491=Cyc_Tcutil_compress((void*)_check_null(_tmp48D->topt));void*_tmp492=_tmp491;void*_tmp496;if(((struct Cyc_Absyn_TagType_Absyn_Type_struct*)_tmp492)->tag == 19){_LL26E: _tmp496=(void*)((struct Cyc_Absyn_TagType_Absyn_Type_struct*)_tmp492)->f1;_LL26F:
# 2334
{void*_tmp493=Cyc_Tcutil_compress(_tmp496);void*_tmp494=_tmp493;struct Cyc_Absyn_Exp*_tmp495;if(((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp494)->tag == 18){_LL273: _tmp495=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp494)->f1;_LL274:
# 2336
 er=_tmp495->r;goto retry_sizeof;}else{_LL275: _LL276:
 goto _LL272;}_LL272:;}
# 2339
goto _LL26D;}else{_LL270: _LL271:
 goto _LL26D;}_LL26D:;}
# 2342
elt_type=Cyc_Absyn_char_typ;
*t=({void**_tmp497=_cycalloc(sizeof(*_tmp497));_tmp497[0]=elt_type;_tmp497;});
num_elts=*e;
one_elt=0;}
# 2347
goto _LL25A;}_LL25A:;}}
# 2351
*is_fat=!one_elt;
# 2354
{void*_tmp49B=elt_type;struct Cyc_Absyn_Aggrdecl*_tmp49E;if(((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp49B)->tag == 11){if((((((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp49B)->f1).aggr_info).KnownAggr).tag == 2){_LL278: _tmp49E=*(((((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp49B)->f1).aggr_info).KnownAggr).val;_LL279:
# 2356
 if(_tmp49E->impl != 0  && ((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp49E->impl))->exist_vars != 0)
({void*_tmp49C=0;Cyc_Tcutil_terr(loc,({const char*_tmp49D="malloc with existential types not yet implemented";_tag_dyneither(_tmp49D,sizeof(char),50);}),_tag_dyneither(_tmp49C,sizeof(void*),0));});
goto _LL277;}else{goto _LL27A;}}else{_LL27A: _LL27B:
 goto _LL277;}_LL277:;}{
# 2363
void*(*_tmp49F)(void*t,void*rgn,struct Cyc_Absyn_Tqual tq,union Cyc_Absyn_Constraint*zero_term)=Cyc_Absyn_at_typ;
union Cyc_Absyn_Constraint*_tmp4A0=Cyc_Absyn_false_conref;
if(topt != 0){
void*_tmp4A1=Cyc_Tcutil_compress(*topt);void*_tmp4A2=_tmp4A1;union Cyc_Absyn_Constraint*_tmp4AF;union Cyc_Absyn_Constraint*_tmp4AE;union Cyc_Absyn_Constraint*_tmp4AD;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4A2)->tag == 5){_LL27D: _tmp4AF=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4A2)->f1).ptr_atts).nullable;_tmp4AE=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4A2)->f1).ptr_atts).bounds;_tmp4AD=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4A2)->f1).ptr_atts).zero_term;_LL27E:
# 2368
 _tmp4A0=_tmp4AD;
if(((int(*)(int y,union Cyc_Absyn_Constraint*x))Cyc_Absyn_conref_def)(0,_tmp4AF))
_tmp49F=Cyc_Absyn_star_typ;
# 2373
if(((int(*)(int y,union Cyc_Absyn_Constraint*x))Cyc_Absyn_conref_def)(0,_tmp4AD) && !(*is_calloc)){
({void*_tmp4A3=0;Cyc_Tcutil_warn(loc,({const char*_tmp4A4="converting malloc to calloc to ensure zero-termination";_tag_dyneither(_tmp4A4,sizeof(char),55);}),_tag_dyneither(_tmp4A3,sizeof(void*),0));});
*is_calloc=1;}
# 2379
{void*_tmp4A5=((void*(*)(void*y,union Cyc_Absyn_Constraint*x))Cyc_Absyn_conref_def)(Cyc_Absyn_bounds_one,_tmp4AE);void*_tmp4A6=_tmp4A5;struct Cyc_Absyn_Exp*_tmp4AC;if(((struct Cyc_Absyn_DynEither_b_Absyn_Bounds_struct*)_tmp4A6)->tag == 0){_LL282: _LL283:
 goto _LL281;}else{_LL284: _tmp4AC=((struct Cyc_Absyn_Upper_b_Absyn_Bounds_struct*)_tmp4A6)->f1;if(!one_elt){_LL285: {
# 2382
int _tmp4A7=Cyc_Evexp_c_can_eval(num_elts);
if(_tmp4A7  && Cyc_Evexp_same_const_exp(_tmp4AC,num_elts)){
*is_fat=0;
return Cyc_Absyn_atb_typ(elt_type,rgn,Cyc_Absyn_empty_tqual(0),((void*(*)(union Cyc_Absyn_Constraint*x))Cyc_Absyn_conref_val)(_tmp4AE),_tmp4A0);}
# 2388
{void*_tmp4A8=Cyc_Tcutil_compress((void*)_check_null(num_elts->topt));void*_tmp4A9=_tmp4A8;void*_tmp4AB;if(((struct Cyc_Absyn_TagType_Absyn_Type_struct*)_tmp4A9)->tag == 19){_LL289: _tmp4AB=(void*)((struct Cyc_Absyn_TagType_Absyn_Type_struct*)_tmp4A9)->f1;_LL28A: {
# 2390
struct Cyc_Absyn_Exp*_tmp4AA=Cyc_Absyn_cast_exp(Cyc_Absyn_uint_typ,Cyc_Absyn_valueof_exp(_tmp4AB,0),0,Cyc_Absyn_No_coercion,0);
# 2392
if(Cyc_Evexp_same_const_exp(_tmp4AA,_tmp4AC)){
*is_fat=0;
return Cyc_Absyn_atb_typ(elt_type,rgn,Cyc_Absyn_empty_tqual(0),((void*(*)(union Cyc_Absyn_Constraint*x))Cyc_Absyn_conref_val)(_tmp4AE),_tmp4A0);}
# 2397
goto _LL288;}}else{_LL28B: _LL28C:
 goto _LL288;}_LL288:;}
# 2400
goto _LL281;}}else{_LL286: _LL287:
 goto _LL281;}}_LL281:;}
# 2403
goto _LL27C;}else{_LL27F: _LL280:
 goto _LL27C;}_LL27C:;}
# 2406
if(!one_elt)_tmp49F=Cyc_Absyn_dyneither_typ;
return _tmp49F(elt_type,rgn,Cyc_Absyn_empty_tqual(0),_tmp4A0);};};}
# 2411
static void*Cyc_Tcexp_tcSwap(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
# 2417
struct _RegionHandle _tmp4B0=_new_region("r");struct _RegionHandle*r=& _tmp4B0;_push_region(r);{
struct Cyc_Tcenv_Tenv*_tmp4B1=Cyc_Tcenv_enter_lhs(r,Cyc_Tcenv_enter_notreadctxt(r,te));
Cyc_Tcexp_tcExpNoPromote(_tmp4B1,0,e1);{
void*_tmp4B2=(void*)_check_null(e1->topt);
Cyc_Tcexp_tcExpNoPromote(_tmp4B1,& _tmp4B2,e2);};}{
# 2423
void*t1=(void*)_check_null(e1->topt);
void*t2=(void*)_check_null(e2->topt);
# 2426
{void*_tmp4B3=Cyc_Tcutil_compress(t1);void*_tmp4B4=_tmp4B3;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp4B4)->tag == 8){_LL28E: _LL28F:
({void*_tmp4B5=0;Cyc_Tcutil_terr(loc,({const char*_tmp4B6="cannot assign to an array";_tag_dyneither(_tmp4B6,sizeof(char),26);}),_tag_dyneither(_tmp4B5,sizeof(void*),0));});goto _LL28D;}else{_LL290: _LL291:
 goto _LL28D;}_LL28D:;}
# 2431
if(!Cyc_Tcutil_is_boxed(t1) && !Cyc_Tcutil_is_pointer_type(t1))
({void*_tmp4B7=0;Cyc_Tcutil_terr(loc,({const char*_tmp4B8="Swap not allowed for non-pointer or non-word-sized types.";_tag_dyneither(_tmp4B8,sizeof(char),58);}),_tag_dyneither(_tmp4B7,sizeof(void*),0));});
# 2435
if(!Cyc_Absyn_is_lvalue(e1)){
void*_tmp4BB=({void*_tmp4B9=0;Cyc_Tcexp_expr_err(te,e1->loc,topt,({const char*_tmp4BA="swap non-lvalue";_tag_dyneither(_tmp4BA,sizeof(char),16);}),_tag_dyneither(_tmp4B9,sizeof(void*),0));});_npop_handler(0);return _tmp4BB;}
if(!Cyc_Absyn_is_lvalue(e2)){
void*_tmp4BE=({void*_tmp4BC=0;Cyc_Tcexp_expr_err(te,e2->loc,topt,({const char*_tmp4BD="swap non-lvalue";_tag_dyneither(_tmp4BD,sizeof(char),16);}),_tag_dyneither(_tmp4BC,sizeof(void*),0));});_npop_handler(0);return _tmp4BE;}{
# 2440
void*t_ign1=(void*)& Cyc_Absyn_VoidType_val;void*t_ign2=(void*)& Cyc_Absyn_VoidType_val;
int b_ign1=0;
if(Cyc_Tcutil_is_zero_ptr_deref(e1,& t_ign1,& b_ign1,& t_ign2)){
void*_tmp4C1=({void*_tmp4BF=0;Cyc_Tcexp_expr_err(te,e1->loc,topt,({const char*_tmp4C0="swap value in zeroterm array";_tag_dyneither(_tmp4C0,sizeof(char),29);}),_tag_dyneither(_tmp4BF,sizeof(void*),0));});_npop_handler(0);return _tmp4C1;}
if(Cyc_Tcutil_is_zero_ptr_deref(e2,& t_ign1,& b_ign1,& t_ign2)){
void*_tmp4C4=({void*_tmp4C2=0;Cyc_Tcexp_expr_err(te,e2->loc,topt,({const char*_tmp4C3="swap value in zeroterm array";_tag_dyneither(_tmp4C3,sizeof(char),29);}),_tag_dyneither(_tmp4C2,sizeof(void*),0));});_npop_handler(0);return _tmp4C4;}
# 2447
Cyc_Tcexp_check_writable(te,e1);
Cyc_Tcexp_check_writable(te,e2);
if(!Cyc_Tcutil_unify(t1,t2)){
void*_tmp4C5=({struct Cyc_String_pa_PrintArg_struct _tmp4CA;_tmp4CA.tag=0;_tmp4CA.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string(t2));({struct Cyc_String_pa_PrintArg_struct _tmp4C9;_tmp4C9.tag=0;_tmp4C9.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t1));({void*_tmp4C7[2]={& _tmp4C9,& _tmp4CA};Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp4C8="type mismatch: %s != %s";_tag_dyneither(_tmp4C8,sizeof(char),24);}),_tag_dyneither(_tmp4C7,sizeof(void*),2));});});});
void*_tmp4C6=_tmp4C5;_npop_handler(0);return _tmp4C6;}{
# 2454
void*_tmp4CB=(void*)& Cyc_Absyn_VoidType_val;_npop_handler(0);return _tmp4CB;};};};
# 2417
;_pop_region(r);}
# 2458
int Cyc_Tcexp_in_stmt_exp=0;
# 2460
static void*Cyc_Tcexp_tcStmtExp(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct Cyc_Absyn_Stmt*s){
struct _RegionHandle _tmp4CC=_new_region("r");struct _RegionHandle*r=& _tmp4CC;_push_region(r);{
int old_stmt_exp_state=Cyc_Tcexp_in_stmt_exp;
Cyc_Tcstmt_tcStmt(Cyc_Tcenv_clear_abstract_val_ok(r,te),s,1);
Cyc_Tcexp_in_stmt_exp=old_stmt_exp_state;}
# 2467
while(1){
void*_tmp4CD=s->r;void*_tmp4CE=_tmp4CD;struct Cyc_Absyn_Decl*_tmp4DB;struct Cyc_Absyn_Stmt*_tmp4DA;struct Cyc_Absyn_Stmt*_tmp4D9;struct Cyc_Absyn_Stmt*_tmp4D8;struct Cyc_Absyn_Exp*_tmp4D7;switch(*((int*)_tmp4CE)){case 1: _LL293: _tmp4D7=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp4CE)->f1;_LL294: {
# 2471
void*_tmp4CF=(void*)_check_null(_tmp4D7->topt);
if(!Cyc_Tcutil_unify(_tmp4CF,Cyc_Absyn_wildtyp(Cyc_Tcenv_lookup_opt_type_vars(te)))){
({struct Cyc_String_pa_PrintArg_struct _tmp4D2;_tmp4D2.tag=0;_tmp4D2.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string(_tmp4CF));({void*_tmp4D0[1]={& _tmp4D2};Cyc_Tcutil_terr(loc,({const char*_tmp4D1="statement expression returns type %s";_tag_dyneither(_tmp4D1,sizeof(char),37);}),_tag_dyneither(_tmp4D0,sizeof(void*),1));});});
Cyc_Tcutil_explain_failure();}{
# 2477
void*_tmp4D3=_tmp4CF;_npop_handler(0);return _tmp4D3;};}case 2: _LL295: _tmp4D9=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp4CE)->f1;_tmp4D8=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp4CE)->f2;_LL296:
 s=_tmp4D8;continue;case 12: _LL297: _tmp4DB=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp4CE)->f1;_tmp4DA=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp4CE)->f2;_LL298:
 s=_tmp4DA;continue;default: _LL299: _LL29A: {
# 2481
void*_tmp4D6=({void*_tmp4D4=0;Cyc_Tcexp_expr_err(te,loc,topt,({const char*_tmp4D5="statement expression must end with expression";_tag_dyneither(_tmp4D5,sizeof(char),46);}),_tag_dyneither(_tmp4D4,sizeof(void*),0));});_npop_handler(0);return _tmp4D6;}}_LL292:;}
# 2461
;_pop_region(r);}
# 2486
static void*Cyc_Tcexp_tcTagcheck(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct Cyc_Absyn_Exp*e,struct _dyneither_ptr*f){
void*t;
struct _RegionHandle _tmp4DC=_new_region("r");struct _RegionHandle*r=& _tmp4DC;_push_region(r);
t=Cyc_Tcutil_compress(Cyc_Tcexp_tcExp(Cyc_Tcenv_enter_abstract_val_ok(r,te),0,e));
# 2491
{void*_tmp4DD=t;struct Cyc_Absyn_Aggrdecl*_tmp4E1;if(((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp4DD)->tag == 11){if((((((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp4DD)->f1).aggr_info).KnownAggr).tag == 2){_LL29C: _tmp4E1=*(((((struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp4DD)->f1).aggr_info).KnownAggr).val;_LL29D:
# 2493
 if((_tmp4E1->kind == Cyc_Absyn_UnionA  && _tmp4E1->impl != 0) && ((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp4E1->impl))->tagged)goto _LL29B;
goto _LL29F;}else{goto _LL29E;}}else{_LL29E: _LL29F:
# 2496
({struct Cyc_String_pa_PrintArg_struct _tmp4E0;_tmp4E0.tag=0;_tmp4E0.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Absynpp_typ2string(t));({void*_tmp4DE[1]={& _tmp4E0};Cyc_Tcutil_terr(loc,({const char*_tmp4DF="expecting @tagged union but found %s";_tag_dyneither(_tmp4DF,sizeof(char),37);}),_tag_dyneither(_tmp4DE,sizeof(void*),1));});});
goto _LL29B;}_LL29B:;}{
# 2499
void*_tmp4E2=Cyc_Absyn_uint_typ;_npop_handler(0);return _tmp4E2;};
# 2488
;_pop_region(r);}
# 2503
static void*Cyc_Tcexp_tcNew(struct Cyc_Tcenv_Tenv*te,unsigned int loc,void**topt,struct Cyc_Absyn_Exp**rgn_handle,struct Cyc_Absyn_Exp*e,struct Cyc_Absyn_Exp*e1){
# 2507
void*rgn=(void*)& Cyc_Absyn_HeapRgn_val;
struct _RegionHandle _tmp4E3=_new_region("r");struct _RegionHandle*r=& _tmp4E3;_push_region(r);
{struct Cyc_Tcenv_Tenv*_tmp4E4=Cyc_Tcenv_clear_abstract_val_ok(r,Cyc_Tcenv_set_new_status(r,Cyc_Tcenv_InNew,te));
if(*rgn_handle != 0){
# 2512
void*expected_type=(void*)({struct Cyc_Absyn_RgnHandleType_Absyn_Type_struct*_tmp4EB=_cycalloc(sizeof(*_tmp4EB));_tmp4EB[0]=({struct Cyc_Absyn_RgnHandleType_Absyn_Type_struct _tmp4EC;_tmp4EC.tag=15;_tmp4EC.f1=
Cyc_Absyn_new_evar(& Cyc_Tcutil_trko,Cyc_Tcenv_lookup_opt_type_vars(_tmp4E4));_tmp4EC;});_tmp4EB;});
void*handle_type=Cyc_Tcexp_tcExp(_tmp4E4,& expected_type,(struct Cyc_Absyn_Exp*)_check_null(*rgn_handle));
void*_tmp4E5=Cyc_Tcutil_compress(handle_type);void*_tmp4E6=_tmp4E5;void*_tmp4EA;if(((struct Cyc_Absyn_RgnHandleType_Absyn_Type_struct*)_tmp4E6)->tag == 15){_LL2A1: _tmp4EA=(void*)((struct Cyc_Absyn_RgnHandleType_Absyn_Type_struct*)_tmp4E6)->f1;_LL2A2:
# 2517
 rgn=_tmp4EA;
Cyc_Tcenv_check_rgn_accessible(_tmp4E4,loc,rgn);
goto _LL2A0;}else{_LL2A3: _LL2A4:
# 2521
({struct Cyc_String_pa_PrintArg_struct _tmp4E9;_tmp4E9.tag=0;_tmp4E9.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)
Cyc_Absynpp_typ2string(handle_type));({void*_tmp4E7[1]={& _tmp4E9};Cyc_Tcutil_terr(((struct Cyc_Absyn_Exp*)_check_null(*rgn_handle))->loc,({const char*_tmp4E8="expecting region_t type but found %s";_tag_dyneither(_tmp4E8,sizeof(char),37);}),_tag_dyneither(_tmp4E7,sizeof(void*),1));});});
goto _LL2A0;}_LL2A0:;}else{
# 2528
if(topt != 0){
void*optrgn=(void*)& Cyc_Absyn_VoidType_val;
if(Cyc_Tcutil_rgn_of_pointer(*topt,& optrgn)){
rgn=Cyc_Tcexp_mallocRgn(optrgn);
if(rgn == (void*)& Cyc_Absyn_UniqueRgn_val)*rgn_handle=Cyc_Absyn_uniquergn_exp;}}}{
# 2536
void*_tmp4ED=e1->r;void*_tmp4EE=_tmp4ED;struct Cyc_List_List*_tmp513;struct Cyc_Core_Opt*_tmp512;struct Cyc_List_List*_tmp511;switch(*((int*)_tmp4EE)){case 26: _LL2A6: _LL2A7:
 goto _LL2A9;case 27: _LL2A8: _LL2A9: {
# 2541
void*res_typ=Cyc_Tcexp_tcExpNoPromote(_tmp4E4,topt,e1);
if(!Cyc_Tcutil_is_array(res_typ))
({void*_tmp4EF=0;((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(({const char*_tmp4F0="tcNew: comprehension returned non-array type";_tag_dyneither(_tmp4F0,sizeof(char),45);}),_tag_dyneither(_tmp4EF,sizeof(void*),0));});
res_typ=Cyc_Tcutil_promote_array(res_typ,rgn,1);
if(topt != 0){
if(!Cyc_Tcutil_unify(*topt,res_typ) && Cyc_Tcutil_silent_castable(_tmp4E4,loc,res_typ,*topt)){
e->topt=res_typ;
Cyc_Tcutil_unchecked_cast(_tmp4E4,e,*topt,Cyc_Absyn_Other_coercion);
res_typ=*topt;}}{
# 2552
void*_tmp4F1=res_typ;_npop_handler(0);return _tmp4F1;};}case 35: _LL2AA: _tmp512=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp4EE)->f1;_tmp511=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp4EE)->f2;_LL2AB:
# 2554
 e1->r=(void*)({struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_tmp4F2=_cycalloc(sizeof(*_tmp4F2));_tmp4F2[0]=({struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct _tmp4F3;_tmp4F3.tag=25;_tmp4F3.f1=_tmp511;_tmp4F3;});_tmp4F2;});
_tmp513=_tmp511;goto _LL2AD;case 25: _LL2AC: _tmp513=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp4EE)->f1;_LL2AD: {
# 2557
void**elt_typ_opt=0;
int zero_term=0;
if(topt != 0){
void*_tmp4F4=Cyc_Tcutil_compress(*topt);void*_tmp4F5=_tmp4F4;void**_tmp4F8;struct Cyc_Absyn_Tqual _tmp4F7;union Cyc_Absyn_Constraint*_tmp4F6;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4F5)->tag == 5){_LL2B5: _tmp4F8=(void**)&(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4F5)->f1).elt_typ;_tmp4F7=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4F5)->f1).elt_tq;_tmp4F6=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4F5)->f1).ptr_atts).zero_term;_LL2B6:
# 2563
 elt_typ_opt=_tmp4F8;
zero_term=((int(*)(int y,union Cyc_Absyn_Constraint*x))Cyc_Absyn_conref_def)(0,_tmp4F6);
goto _LL2B4;}else{_LL2B7: _LL2B8:
 goto _LL2B4;}_LL2B4:;}{
# 2569
void*res_typ=Cyc_Tcexp_tcArray(_tmp4E4,e1->loc,elt_typ_opt,0,zero_term,_tmp513);
e1->topt=res_typ;
if(!Cyc_Tcutil_is_array(res_typ))
({void*_tmp4F9=0;((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(({const char*_tmp4FA="tcExpNoPromote on Array_e returned non-array type";_tag_dyneither(_tmp4FA,sizeof(char),50);}),_tag_dyneither(_tmp4F9,sizeof(void*),0));});
res_typ=Cyc_Tcutil_promote_array(res_typ,rgn,0);
if(topt != 0){
# 2578
if(!Cyc_Tcutil_unify(*topt,res_typ) && Cyc_Tcutil_silent_castable(_tmp4E4,loc,res_typ,*topt)){
e->topt=res_typ;
Cyc_Tcutil_unchecked_cast(_tmp4E4,e,*topt,Cyc_Absyn_Other_coercion);
res_typ=*topt;}}{
# 2584
void*_tmp4FB=res_typ;_npop_handler(0);return _tmp4FB;};};}case 0: switch(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp4EE)->f1).Wstring_c).tag){case 8: _LL2AE: _LL2AF: {
# 2589
void*_tmp4FC=Cyc_Absyn_atb_typ(Cyc_Absyn_char_typ,rgn,Cyc_Absyn_const_tqual(0),(void*)& Cyc_Absyn_DynEither_b_val,Cyc_Absyn_true_conref);
# 2591
void*_tmp4FD=Cyc_Tcexp_tcExp(_tmp4E4,& _tmp4FC,e1);
void*_tmp500=Cyc_Absyn_atb_typ(_tmp4FD,rgn,Cyc_Absyn_empty_tqual(0),(void*)({struct Cyc_Absyn_Upper_b_Absyn_Bounds_struct*_tmp4FE=_cycalloc(sizeof(*_tmp4FE));_tmp4FE[0]=({struct Cyc_Absyn_Upper_b_Absyn_Bounds_struct _tmp4FF;_tmp4FF.tag=1;_tmp4FF.f1=Cyc_Absyn_uint_exp(1,0);_tmp4FF;});_tmp4FE;}),Cyc_Absyn_false_conref);_npop_handler(0);return _tmp500;}case 9: _LL2B0: _LL2B1: {
# 2596
void*_tmp501=Cyc_Absyn_atb_typ(Cyc_Absyn_wchar_typ(),rgn,Cyc_Absyn_const_tqual(0),(void*)& Cyc_Absyn_DynEither_b_val,Cyc_Absyn_true_conref);
# 2598
void*_tmp502=Cyc_Tcexp_tcExp(_tmp4E4,& _tmp501,e1);
void*_tmp505=Cyc_Absyn_atb_typ(_tmp502,rgn,Cyc_Absyn_empty_tqual(0),(void*)({struct Cyc_Absyn_Upper_b_Absyn_Bounds_struct*_tmp503=_cycalloc(sizeof(*_tmp503));_tmp503[0]=({struct Cyc_Absyn_Upper_b_Absyn_Bounds_struct _tmp504;_tmp504.tag=1;_tmp504.f1=Cyc_Absyn_uint_exp(1,0);_tmp504;});_tmp503;}),Cyc_Absyn_false_conref);_npop_handler(0);return _tmp505;}default: goto _LL2B2;}default: _LL2B2: _LL2B3:
# 2605
 RG: {
void*bogus=(void*)& Cyc_Absyn_VoidType_val;
void**topt2=0;
if(topt != 0){
void*_tmp506=Cyc_Tcutil_compress(*topt);void*_tmp507=_tmp506;void**_tmp509;struct Cyc_Absyn_Tqual _tmp508;switch(*((int*)_tmp507)){case 5: _LL2BA: _tmp509=(void**)&(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp507)->f1).elt_typ;_tmp508=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp507)->f1).elt_tq;_LL2BB:
# 2611
 topt2=_tmp509;goto _LL2B9;case 3: _LL2BC: _LL2BD:
# 2615
 bogus=*topt;
topt2=& bogus;
goto _LL2B9;default: _LL2BE: _LL2BF:
 goto _LL2B9;}_LL2B9:;}{
# 2621
void*telt=Cyc_Tcexp_tcExp(_tmp4E4,topt2,e1);
# 2623
struct _RegionHandle _tmp50A=_new_region("temp");struct _RegionHandle*temp=& _tmp50A;_push_region(temp);
if(Cyc_Tcutil_is_noalias_pointer_or_aggr(temp,telt) && !
Cyc_Tcutil_is_noalias_path(temp,e1))
({void*_tmp50B=0;Cyc_Tcutil_terr(e1->loc,({const char*_tmp50C="Cannot consume non-unique paths; do swap instead";_tag_dyneither(_tmp50C,sizeof(char),49);}),_tag_dyneither(_tmp50B,sizeof(void*),0));});{
# 2628
void*res_typ=(void*)({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmp50E=_cycalloc(sizeof(*_tmp50E));_tmp50E[0]=({struct Cyc_Absyn_PointerType_Absyn_Type_struct _tmp50F;_tmp50F.tag=5;_tmp50F.f1=({struct Cyc_Absyn_PtrInfo _tmp510;_tmp510.elt_typ=telt;_tmp510.elt_tq=
Cyc_Absyn_empty_tqual(0);_tmp510.ptr_atts=({(_tmp510.ptr_atts).rgn=rgn;(_tmp510.ptr_atts).nullable=
((union Cyc_Absyn_Constraint*(*)())Cyc_Absyn_empty_conref)();(_tmp510.ptr_atts).bounds=Cyc_Absyn_bounds_one_conref;(_tmp510.ptr_atts).zero_term=Cyc_Absyn_false_conref;(_tmp510.ptr_atts).ptrloc=0;_tmp510.ptr_atts;});_tmp510;});_tmp50F;});_tmp50E;});
# 2633
if(topt != 0){
if(!Cyc_Tcutil_unify(*topt,res_typ) && Cyc_Tcutil_silent_castable(_tmp4E4,loc,res_typ,*topt)){
e->topt=res_typ;
Cyc_Tcutil_unchecked_cast(_tmp4E4,e,*topt,Cyc_Absyn_Other_coercion);
res_typ=*topt;}}{
# 2640
void*_tmp50D=res_typ;_npop_handler(1);return _tmp50D;};};
# 2623
;_pop_region(temp);};}}_LL2A5:;};}
# 2509
;_pop_region(r);}
# 2647
void*Cyc_Tcexp_tcExp(struct Cyc_Tcenv_Tenv*te,void**topt,struct Cyc_Absyn_Exp*e){
void*t=Cyc_Tcutil_compress(Cyc_Tcexp_tcExpNoPromote(te,topt,e));
if(Cyc_Tcutil_is_array(t))
e->topt=(t=Cyc_Tcutil_promote_array(t,(Cyc_Tcutil_addressof_props(te,e)).f2,0));
return t;}
# 2657
void*Cyc_Tcexp_tcExpInitializer(struct Cyc_Tcenv_Tenv*te,void**topt,struct Cyc_Absyn_Exp*e){
void*t=Cyc_Tcexp_tcExpNoPromote(te,topt,e);
# 2661
struct _RegionHandle _tmp514=_new_region("temp");struct _RegionHandle*temp=& _tmp514;_push_region(temp);
if(Cyc_Tcutil_is_noalias_pointer_or_aggr(temp,t) && !Cyc_Tcutil_is_noalias_path(temp,e))
# 2667
({void*_tmp515=0;Cyc_Tcutil_terr(e->loc,({const char*_tmp516="Cannot consume non-unique paths; do swap instead";_tag_dyneither(_tmp516,sizeof(char),49);}),_tag_dyneither(_tmp515,sizeof(void*),0));});{
# 2670
void*_tmp517=e->r;void*_tmp518=_tmp517;switch(*((int*)_tmp518)){case 25: _LL2C1: _LL2C2:
 goto _LL2C4;case 26: _LL2C3: _LL2C4:
 goto _LL2C6;case 27: _LL2C5: _LL2C6:
 goto _LL2C8;case 0: switch(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp518)->f1).String_c).tag){case 9: _LL2C7: _LL2C8:
 goto _LL2CA;case 8: _LL2C9: _LL2CA: {
void*_tmp519=t;_npop_handler(0);return _tmp519;}default: goto _LL2CB;}default: _LL2CB: _LL2CC:
# 2677
 t=Cyc_Tcutil_compress(t);
if(Cyc_Tcutil_is_array(t)){
# 2680
t=Cyc_Tcutil_promote_array(t,(Cyc_Tcutil_addressof_props(te,e)).f2,0);
Cyc_Tcutil_unchecked_cast(te,e,t,Cyc_Absyn_Other_coercion);}{
# 2683
void*_tmp51A=t;_npop_handler(0);return _tmp51A;};}_LL2C0:;};
# 2661
;_pop_region(temp);}
# 2694 "tcexp.cyc"
static void*Cyc_Tcexp_tcExpNoPromote(struct Cyc_Tcenv_Tenv*te,void**topt,struct Cyc_Absyn_Exp*e){
{void*_tmp51B=e->r;void*_tmp51C=_tmp51B;struct Cyc_Absyn_Exp*_tmp541;if(((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp51C)->tag == 11){_LL2CE: _tmp541=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp51C)->f1;_LL2CF:
# 2698
 Cyc_Tcexp_tcExpNoInst(te,topt,_tmp541);
_tmp541->topt=Cyc_Absyn_pointer_expand((void*)_check_null(_tmp541->topt),0);
e->topt=_tmp541->topt;
goto _LL2CD;}else{_LL2D0: _LL2D1:
# 2704
 Cyc_Tcexp_tcExpNoInst(te,topt,e);
e->topt=Cyc_Absyn_pointer_expand((void*)_check_null(e->topt),0);
# 2707
{void*_tmp51D=Cyc_Tcutil_compress((void*)_check_null(e->topt));void*_tmp51E=_tmp51D;void*_tmp540;struct Cyc_Absyn_Tqual _tmp53F;void*_tmp53E;union Cyc_Absyn_Constraint*_tmp53D;union Cyc_Absyn_Constraint*_tmp53C;union Cyc_Absyn_Constraint*_tmp53B;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp51E)->tag == 5){_LL2D3: _tmp540=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp51E)->f1).elt_typ;_tmp53F=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp51E)->f1).elt_tq;_tmp53E=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp51E)->f1).ptr_atts).rgn;_tmp53D=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp51E)->f1).ptr_atts).nullable;_tmp53C=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp51E)->f1).ptr_atts).bounds;_tmp53B=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp51E)->f1).ptr_atts).zero_term;_LL2D4:
# 2709
{void*_tmp51F=Cyc_Tcutil_compress(_tmp540);void*_tmp520=_tmp51F;struct Cyc_List_List*_tmp53A;void*_tmp539;struct Cyc_Absyn_Tqual _tmp538;void*_tmp537;struct Cyc_List_List*_tmp536;int _tmp535;struct Cyc_Absyn_VarargInfo*_tmp534;struct Cyc_List_List*_tmp533;struct Cyc_List_List*_tmp532;struct Cyc_Absyn_Exp*_tmp531;struct Cyc_List_List*_tmp530;struct Cyc_Absyn_Exp*_tmp52F;struct Cyc_List_List*_tmp52E;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp520)->tag == 9){_LL2D8: _tmp53A=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp520)->f1).tvars;_tmp539=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp520)->f1).effect;_tmp538=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp520)->f1).ret_tqual;_tmp537=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp520)->f1).ret_typ;_tmp536=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp520)->f1).args;_tmp535=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp520)->f1).c_varargs;_tmp534=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp520)->f1).cyc_varargs;_tmp533=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp520)->f1).rgn_po;_tmp532=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp520)->f1).attributes;_tmp531=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp520)->f1).requires_clause;_tmp530=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp520)->f1).requires_relns;_tmp52F=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp520)->f1).ensures_clause;_tmp52E=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp520)->f1).ensures_relns;_LL2D9:
# 2711
 if(_tmp53A != 0){
struct _RegionHandle _tmp521=_new_region("rgn");struct _RegionHandle*rgn=& _tmp521;_push_region(rgn);
{struct _tuple12 _tmp522=({struct _tuple12 _tmp52D;_tmp52D.f1=Cyc_Tcenv_lookup_type_vars(te);_tmp52D.f2=rgn;_tmp52D;});
struct Cyc_List_List*inst=
((struct Cyc_List_List*(*)(struct _RegionHandle*,struct _tuple13*(*f)(struct _tuple12*,struct Cyc_Absyn_Tvar*),struct _tuple12*env,struct Cyc_List_List*x))Cyc_List_rmap_c)(rgn,Cyc_Tcutil_r_make_inst_var,& _tmp522,_tmp53A);
struct Cyc_List_List*ts=((struct Cyc_List_List*(*)(void*(*f)(struct _tuple13*),struct Cyc_List_List*x))Cyc_List_map)((void*(*)(struct _tuple13*))Cyc_Core_snd,inst);
# 2720
_tmp533=Cyc_Tcutil_rsubst_rgnpo(rgn,inst,_tmp533);
Cyc_Tcenv_check_rgn_partial_order(te,e->loc,_tmp533);{
void*ftyp=Cyc_Tcutil_rsubstitute(rgn,inst,(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp52A=_cycalloc(sizeof(*_tmp52A));_tmp52A[0]=({struct Cyc_Absyn_FnType_Absyn_Type_struct _tmp52B;_tmp52B.tag=9;_tmp52B.f1=({struct Cyc_Absyn_FnInfo _tmp52C;_tmp52C.tvars=0;_tmp52C.effect=_tmp539;_tmp52C.ret_tqual=_tmp538;_tmp52C.ret_typ=_tmp537;_tmp52C.args=_tmp536;_tmp52C.c_varargs=_tmp535;_tmp52C.cyc_varargs=_tmp534;_tmp52C.rgn_po=0;_tmp52C.attributes=_tmp532;_tmp52C.requires_clause=_tmp531;_tmp52C.requires_relns=_tmp530;_tmp52C.ensures_clause=_tmp52F;_tmp52C.ensures_relns=_tmp52E;_tmp52C;});_tmp52B;});_tmp52A;}));
# 2727
struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmp523=({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmp527=_cycalloc(sizeof(*_tmp527));_tmp527[0]=({struct Cyc_Absyn_PointerType_Absyn_Type_struct _tmp528;_tmp528.tag=5;_tmp528.f1=({struct Cyc_Absyn_PtrInfo _tmp529;_tmp529.elt_typ=ftyp;_tmp529.elt_tq=_tmp53F;_tmp529.ptr_atts=({(_tmp529.ptr_atts).rgn=_tmp53E;(_tmp529.ptr_atts).nullable=_tmp53D;(_tmp529.ptr_atts).bounds=_tmp53C;(_tmp529.ptr_atts).zero_term=_tmp53B;(_tmp529.ptr_atts).ptrloc=0;_tmp529.ptr_atts;});_tmp529;});_tmp528;});_tmp527;});
# 2729
struct Cyc_Absyn_Exp*_tmp524=Cyc_Absyn_copy_exp(e);
e->r=(void*)({struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*_tmp525=_cycalloc(sizeof(*_tmp525));_tmp525[0]=({struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct _tmp526;_tmp526.tag=12;_tmp526.f1=_tmp524;_tmp526.f2=ts;_tmp526;});_tmp525;});
e->topt=(void*)_tmp523;};}
# 2713
;_pop_region(rgn);}
# 2734
goto _LL2D7;}else{_LL2DA: _LL2DB:
 goto _LL2D7;}_LL2D7:;}
# 2737
goto _LL2D2;}else{_LL2D5: _LL2D6:
 goto _LL2D2;}_LL2D2:;}
# 2740
goto _LL2CD;}_LL2CD:;}
# 2742
return(void*)_check_null(e->topt);}
# 2750
static void Cyc_Tcexp_insert_alias_stmts(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e,struct Cyc_Absyn_Exp*fn_exp,struct Cyc_List_List*alias_arg_exps){
# 2752
struct _RegionHandle _tmp542=_new_region("r");struct _RegionHandle*r=& _tmp542;_push_region(r);
{struct Cyc_List_List*_tmp543=0;
# 2755
{void*_tmp544=fn_exp->r;void*_tmp545=_tmp544;struct Cyc_List_List*_tmp559;if(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp545)->tag == 9){_LL2DD: _tmp559=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_LL2DE:
# 2757
{void*_tmp546=e->r;void*_tmp547=_tmp546;struct Cyc_List_List*_tmp556;if(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp547)->tag == 9){_LL2E2: _tmp556=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp547)->f2;_LL2E3: {
# 2759
struct Cyc_List_List*_tmp548=alias_arg_exps;
int _tmp549=0;
while(_tmp548 != 0){
while(_tmp549 != (int)_tmp548->hd){
if(_tmp559 == 0)
({void*_tmp54A=0;((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)((struct _dyneither_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp54E;_tmp54E.tag=1;_tmp54E.f1=(unsigned long)((int)_tmp548->hd);({struct Cyc_Int_pa_PrintArg_struct _tmp54D;_tmp54D.tag=1;_tmp54D.f1=(unsigned long)_tmp549;({void*_tmp54B[2]={& _tmp54D,& _tmp54E};Cyc_aprintf(({const char*_tmp54C="bad count %d/%d for alias coercion!";_tag_dyneither(_tmp54C,sizeof(char),36);}),_tag_dyneither(_tmp54B,sizeof(void*),2));});});}),_tag_dyneither(_tmp54A,sizeof(void*),0));});
# 2766
++ _tmp549;
_tmp559=_tmp559->tl;
_tmp556=((struct Cyc_List_List*)_check_null(_tmp556))->tl;}{
# 2771
struct _tuple11 _tmp54F=Cyc_Tcutil_insert_alias((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(_tmp559))->hd,Cyc_Tcutil_copy_type((void*)_check_null(((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(_tmp556))->hd)->topt)));struct _tuple11 _tmp550=_tmp54F;struct Cyc_Absyn_Decl*_tmp553;struct Cyc_Absyn_Exp*_tmp552;_LL2E7: _tmp553=_tmp550.f1;_tmp552=_tmp550.f2;_LL2E8:;
_tmp559->hd=(void*)_tmp552;
_tmp543=({struct Cyc_List_List*_tmp551=_region_malloc(r,sizeof(*_tmp551));_tmp551->hd=_tmp553;_tmp551->tl=_tmp543;_tmp551;});
_tmp548=_tmp548->tl;};}
# 2776
goto _LL2E1;}}else{_LL2E4: _LL2E5:
({void*_tmp554=0;((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(({const char*_tmp555="not a function call!";_tag_dyneither(_tmp555,sizeof(char),21);}),_tag_dyneither(_tmp554,sizeof(void*),0));});}_LL2E1:;}
# 2779
goto _LL2DC;}else{_LL2DF: _LL2E0:
({void*_tmp557=0;((int(*)(struct _dyneither_ptr fmt,struct _dyneither_ptr ap))Cyc_Tcutil_impos)(({const char*_tmp558="not a function call!";_tag_dyneither(_tmp558,sizeof(char),21);}),_tag_dyneither(_tmp557,sizeof(void*),0));});}_LL2DC:;}
# 2784
while(_tmp543 != 0){
struct Cyc_Absyn_Decl*_tmp55A=(struct Cyc_Absyn_Decl*)_tmp543->hd;
fn_exp=Cyc_Absyn_stmt_exp(Cyc_Absyn_decl_stmt(_tmp55A,Cyc_Absyn_exp_stmt(fn_exp,e->loc),e->loc),e->loc);
_tmp543=_tmp543->tl;}
# 2791
e->topt=0;
e->r=fn_exp->r;}
# 2753
;_pop_region(r);}
# 2798
static void Cyc_Tcexp_tcExpNoInst(struct Cyc_Tcenv_Tenv*te,void**topt,struct Cyc_Absyn_Exp*e){
unsigned int loc=e->loc;
void*t;
# 2802
{void*_tmp55B=e->r;void*_tmp55C=_tmp55B;void*_tmp5C7;struct Cyc_Absyn_Exp*_tmp5C6;struct _dyneither_ptr*_tmp5C5;struct Cyc_Absyn_Exp*_tmp5C4;struct Cyc_Absyn_Exp*_tmp5C3;int*_tmp5C2;struct Cyc_Absyn_Exp**_tmp5C1;void***_tmp5C0;struct Cyc_Absyn_Exp**_tmp5BF;int*_tmp5BE;struct _tuple1**_tmp5BD;void*_tmp5BC;struct Cyc_Absyn_Enumfield*_tmp5BB;struct _tuple1**_tmp5BA;struct Cyc_Absyn_Enumdecl*_tmp5B9;struct Cyc_Absyn_Enumfield*_tmp5B8;struct Cyc_List_List*_tmp5B7;struct Cyc_Absyn_Datatypedecl*_tmp5B6;struct Cyc_Absyn_Datatypefield*_tmp5B5;void*_tmp5B4;struct Cyc_List_List*_tmp5B3;struct _tuple1**_tmp5B2;struct Cyc_List_List**_tmp5B1;struct Cyc_List_List*_tmp5B0;struct Cyc_Absyn_Aggrdecl**_tmp5AF;struct Cyc_Absyn_Exp*_tmp5AE;void*_tmp5AD;int*_tmp5AC;struct Cyc_Absyn_Vardecl*_tmp5AB;struct Cyc_Absyn_Exp*_tmp5AA;struct Cyc_Absyn_Exp*_tmp5A9;int*_tmp5A8;struct Cyc_Absyn_Stmt*_tmp5A7;struct Cyc_List_List*_tmp5A6;struct _tuple9*_tmp5A5;struct Cyc_List_List*_tmp5A4;struct Cyc_List_List*_tmp5A3;struct Cyc_Absyn_Exp*_tmp5A2;struct Cyc_Absyn_Exp*_tmp5A1;struct Cyc_Absyn_Exp*_tmp5A0;struct _dyneither_ptr*_tmp59F;int*_tmp59E;int*_tmp59D;struct Cyc_Absyn_Exp*_tmp59C;struct _dyneither_ptr*_tmp59B;int*_tmp59A;int*_tmp599;struct Cyc_Absyn_Exp*_tmp598;void*_tmp597;struct Cyc_List_List*_tmp596;void*_tmp595;struct Cyc_Absyn_Exp*_tmp594;struct Cyc_Absyn_Exp**_tmp593;struct Cyc_Absyn_Exp*_tmp592;struct Cyc_Absyn_Exp*_tmp591;void*_tmp590;struct Cyc_Absyn_Exp*_tmp58F;enum Cyc_Absyn_Coercion*_tmp58E;struct Cyc_Absyn_Exp*_tmp58D;struct Cyc_List_List*_tmp58C;struct Cyc_Absyn_Exp*_tmp58B;struct Cyc_Absyn_Exp*_tmp58A;struct Cyc_Absyn_Exp*_tmp589;struct Cyc_Absyn_Exp*_tmp588;struct Cyc_Absyn_Exp*_tmp587;struct Cyc_Absyn_Exp*_tmp586;struct Cyc_Absyn_Exp*_tmp585;struct Cyc_Absyn_Exp*_tmp584;struct Cyc_Absyn_Exp*_tmp583;struct Cyc_Absyn_Exp*_tmp582;struct Cyc_Absyn_Exp*_tmp581;struct Cyc_Core_Opt*_tmp580;struct Cyc_Absyn_Exp*_tmp57F;struct Cyc_Absyn_Exp*_tmp57E;enum Cyc_Absyn_Incrementor _tmp57D;enum Cyc_Absyn_Primop _tmp57C;struct Cyc_List_List*_tmp57B;void**_tmp57A;union Cyc_Absyn_Cnst*_tmp579;struct Cyc_Core_Opt*_tmp578;struct Cyc_List_List*_tmp577;struct Cyc_Absyn_Exp*_tmp576;struct Cyc_List_List*_tmp575;struct Cyc_Absyn_VarargCallInfo**_tmp574;struct Cyc_Absyn_Exp*_tmp573;struct Cyc_List_List*_tmp572;struct Cyc_Absyn_VarargCallInfo**_tmp571;int*_tmp570;struct Cyc_Absyn_Exp*_tmp56F;switch(*((int*)_tmp55C)){case 11: _LL2EA: _tmp56F=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_LL2EB:
# 2807
 Cyc_Tcexp_tcExpNoInst(te,0,_tmp56F);
return;case 9: _LL2EC: _tmp573=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp572=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_tmp571=(struct Cyc_Absyn_VarargCallInfo**)&((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp55C)->f3;_tmp570=(int*)&((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp55C)->f4;if(!(*_tmp570)){_LL2ED:
# 2813
 if(Cyc_Tcexp_resolve_unknown_fn(te,e,_tmp573,_tmp572)){
*_tmp570=1;
Cyc_Tcexp_tcExpNoInst(te,topt,e);}else{
# 2817
if(topt != 0)e->topt=*topt;else{
e->topt=Cyc_Absyn_wildtyp(Cyc_Tcenv_lookup_opt_type_vars(te));}}
# 2820
return;}else{_LL302: _tmp576=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp575=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_tmp574=(struct Cyc_Absyn_VarargCallInfo**)&((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp55C)->f3;_LL303: {
# 2851
struct Cyc_List_List*alias_arg_exps=0;
int ok=1;
struct Cyc_Absyn_Exp*fn_exp;
{struct _handler_cons _tmp55D;_push_handler(& _tmp55D);{int _tmp55F=0;if(setjmp(_tmp55D.handler))_tmp55F=1;if(!_tmp55F){
fn_exp=Cyc_Tcutil_deep_copy_exp(0,e);;_pop_handler();}else{void*_tmp55E=(void*)_exn_thrown;void*_tmp560=_tmp55E;void*_tmp562;struct _dyneither_ptr _tmp561;if(((struct Cyc_Core_Failure_exn_struct*)_tmp560)->tag == Cyc_Core_Failure){_LL33D: _tmp561=((struct Cyc_Core_Failure_exn_struct*)_tmp560)->f1;_LL33E:
# 2858
 ok=0;
fn_exp=e;
goto _LL33C;}else{_LL33F: _tmp562=_tmp560;_LL340:(int)_rethrow(_tmp562);}_LL33C:;}};}
# 2862
t=Cyc_Tcexp_tcFnCall(te,loc,topt,_tmp576,_tmp575,_tmp574,& alias_arg_exps);
if(alias_arg_exps != 0  && ok){
alias_arg_exps=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(alias_arg_exps);
Cyc_Tcexp_insert_alias_stmts(te,e,fn_exp,alias_arg_exps);
Cyc_Tcexp_tcExpNoInst(te,topt,e);
return;}
# 2869
goto _LL2E9;}}case 35: _LL2EE: _tmp578=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp577=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_LL2EF:
# 2824
 Cyc_Tcexp_resolve_unresolved_mem(te,loc,topt,e,_tmp577);
Cyc_Tcexp_tcExpNoInst(te,topt,e);
return;case 0: _LL2F0: _tmp579=(union Cyc_Absyn_Cnst*)&((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_LL2F1:
# 2829
 t=Cyc_Tcexp_tcConst(te,loc,topt,_tmp579,e);goto _LL2E9;case 1: _LL2F2: _tmp57A=(void**)&((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_LL2F3:
# 2831
 t=Cyc_Tcexp_tcVar(te,loc,topt,e,_tmp57A);goto _LL2E9;case 2: _LL2F4: _tmp57C=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp57B=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_LL2F5:
# 2833
 t=Cyc_Tcexp_tcPrimop(te,loc,topt,_tmp57C,_tmp57B);goto _LL2E9;case 4: _LL2F6: _tmp57E=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp57D=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_LL2F7:
# 2835
 t=Cyc_Tcexp_tcIncrement(te,loc,topt,_tmp57E,_tmp57D);goto _LL2E9;case 3: _LL2F8: _tmp581=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp580=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_tmp57F=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp55C)->f3;_LL2F9:
# 2837
 t=Cyc_Tcexp_tcAssignOp(te,loc,topt,_tmp581,_tmp580,_tmp57F);goto _LL2E9;case 5: _LL2FA: _tmp584=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp583=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_tmp582=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp55C)->f3;_LL2FB:
# 2839
 t=Cyc_Tcexp_tcConditional(te,loc,topt,_tmp584,_tmp583,_tmp582);goto _LL2E9;case 6: _LL2FC: _tmp586=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp585=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_LL2FD:
# 2841
 t=Cyc_Tcexp_tcAnd(te,loc,_tmp586,_tmp585);goto _LL2E9;case 7: _LL2FE: _tmp588=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp587=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_LL2FF:
# 2843
 t=Cyc_Tcexp_tcOr(te,loc,_tmp588,_tmp587);goto _LL2E9;case 8: _LL300: _tmp58A=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp589=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_LL301:
# 2845
 t=Cyc_Tcexp_tcSeqExp(te,loc,topt,_tmp58A,_tmp589);goto _LL2E9;case 10: _LL304: _tmp58B=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_LL305:
# 2871
 t=Cyc_Tcexp_tcThrow(te,loc,topt,_tmp58B);goto _LL2E9;case 12: _LL306: _tmp58D=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp58C=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_LL307:
# 2873
 t=Cyc_Tcexp_tcInstantiate(te,loc,topt,_tmp58D,_tmp58C);goto _LL2E9;case 13: _LL308: _tmp590=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp58F=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_tmp58E=(enum Cyc_Absyn_Coercion*)&((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp55C)->f4;_LL309:
# 2875
 t=Cyc_Tcexp_tcCast(te,loc,topt,_tmp590,_tmp58F,_tmp58E);goto _LL2E9;case 14: _LL30A: _tmp591=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_LL30B:
# 2877
 t=Cyc_Tcexp_tcAddress(te,loc,e,topt,_tmp591);goto _LL2E9;case 15: _LL30C: _tmp593=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp592=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_LL30D:
# 2879
 t=Cyc_Tcexp_tcNew(te,loc,topt,_tmp593,e,_tmp592);goto _LL2E9;case 17: _LL30E: _tmp594=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_LL30F: {
# 2881
void*_tmp563=Cyc_Tcexp_tcExpNoPromote(te,0,_tmp594);
t=Cyc_Tcexp_tcSizeof(te,loc,topt,_tmp563);goto _LL2E9;}case 16: _LL310: _tmp595=(void*)((struct Cyc_Absyn_Sizeoftyp_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_LL311:
# 2884
 t=Cyc_Tcexp_tcSizeof(te,loc,topt,_tmp595);goto _LL2E9;case 18: _LL312: _tmp597=(void*)((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp596=((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_LL313:
# 2886
 t=Cyc_Tcexp_tcOffsetof(te,loc,topt,_tmp597,_tmp596);goto _LL2E9;case 19: _LL314: _tmp598=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_LL315:
# 2888
 t=Cyc_Tcexp_tcDeref(te,loc,topt,_tmp598);goto _LL2E9;case 20: _LL316: _tmp59C=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp59B=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_tmp59A=(int*)&((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp55C)->f3;_tmp599=(int*)&((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp55C)->f4;_LL317:
# 2890
 t=Cyc_Tcexp_tcAggrMember(te,loc,topt,e,_tmp59C,_tmp59B,_tmp59A,_tmp599);goto _LL2E9;case 21: _LL318: _tmp5A0=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp59F=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_tmp59E=(int*)&((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp55C)->f3;_tmp59D=(int*)&((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp55C)->f4;_LL319:
# 2892
 t=Cyc_Tcexp_tcAggrArrow(te,loc,topt,_tmp5A0,_tmp59F,_tmp59E,_tmp59D);goto _LL2E9;case 22: _LL31A: _tmp5A2=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp5A1=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_LL31B:
# 2894
 t=Cyc_Tcexp_tcSubscript(te,loc,topt,_tmp5A2,_tmp5A1);goto _LL2E9;case 23: _LL31C: _tmp5A3=((struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_LL31D:
# 2896
 t=Cyc_Tcexp_tcTuple(te,loc,topt,_tmp5A3);goto _LL2E9;case 24: _LL31E: _tmp5A5=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp5A4=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_LL31F:
# 2898
 t=Cyc_Tcexp_tcCompoundLit(te,loc,topt,_tmp5A5,_tmp5A4);goto _LL2E9;case 25: _LL320: _tmp5A6=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_LL321: {
# 2902
void**elt_topt=0;
struct Cyc_Absyn_Tqual*elt_tqopt=0;
int zero_term=0;
if(topt != 0){
void*_tmp564=Cyc_Tcutil_compress(*topt);void*_tmp565=_tmp564;void**_tmp568;struct Cyc_Absyn_Tqual*_tmp567;union Cyc_Absyn_Constraint*_tmp566;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp565)->tag == 8){_LL342: _tmp568=(void**)&(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp565)->f1).elt_type;_tmp567=(struct Cyc_Absyn_Tqual*)&(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp565)->f1).tq;_tmp566=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp565)->f1).zero_term;_LL343:
# 2908
 elt_topt=_tmp568;
elt_tqopt=_tmp567;
zero_term=((int(*)(int y,union Cyc_Absyn_Constraint*x))Cyc_Absyn_conref_def)(0,_tmp566);
goto _LL341;}else{_LL344: _LL345:
 goto _LL341;}_LL341:;}
# 2915
t=Cyc_Tcexp_tcArray(te,loc,elt_topt,elt_tqopt,zero_term,_tmp5A6);goto _LL2E9;}case 36: _LL322: _tmp5A7=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_LL323:
# 2917
 t=Cyc_Tcexp_tcStmtExp(te,loc,topt,_tmp5A7);goto _LL2E9;case 26: _LL324: _tmp5AB=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp5AA=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_tmp5A9=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp55C)->f3;_tmp5A8=(int*)&((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp55C)->f4;_LL325:
# 2919
 t=Cyc_Tcexp_tcComprehension(te,loc,topt,_tmp5AB,_tmp5AA,_tmp5A9,_tmp5A8);goto _LL2E9;case 27: _LL326: _tmp5AE=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp5AD=(void*)((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_tmp5AC=(int*)&((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp55C)->f3;_LL327:
# 2921
 t=Cyc_Tcexp_tcComprehensionNoinit(te,loc,topt,_tmp5AE,_tmp5AD,_tmp5AC);goto _LL2E9;case 28: _LL328: _tmp5B2=(struct _tuple1**)&((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp5B1=(struct Cyc_List_List**)&((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_tmp5B0=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp55C)->f3;_tmp5AF=(struct Cyc_Absyn_Aggrdecl**)&((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp55C)->f4;_LL329:
# 2923
 t=Cyc_Tcexp_tcAggregate(te,loc,topt,_tmp5B2,_tmp5B1,_tmp5B0,_tmp5AF);goto _LL2E9;case 29: _LL32A: _tmp5B4=(void*)((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp5B3=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_LL32B:
# 2925
 t=Cyc_Tcexp_tcAnonStruct(te,loc,_tmp5B4,_tmp5B3);goto _LL2E9;case 30: _LL32C: _tmp5B7=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp5B6=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_tmp5B5=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp55C)->f3;_LL32D:
# 2927
 t=Cyc_Tcexp_tcDatatype(te,loc,topt,e,_tmp5B7,_tmp5B6,_tmp5B5);goto _LL2E9;case 31: _LL32E: _tmp5BA=(struct _tuple1**)&((struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp5B9=((struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_tmp5B8=((struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*)_tmp55C)->f3;_LL32F:
# 2929
*_tmp5BA=((struct Cyc_Absyn_Enumfield*)_check_null(_tmp5B8))->name;
t=(void*)({struct Cyc_Absyn_EnumType_Absyn_Type_struct*_tmp569=_cycalloc(sizeof(*_tmp569));_tmp569[0]=({struct Cyc_Absyn_EnumType_Absyn_Type_struct _tmp56A;_tmp56A.tag=13;_tmp56A.f1=((struct Cyc_Absyn_Enumdecl*)_check_null(_tmp5B9))->name;_tmp56A.f2=_tmp5B9;_tmp56A;});_tmp569;});goto _LL2E9;case 32: _LL330: _tmp5BD=(struct _tuple1**)&((struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp5BC=(void*)((struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_tmp5BB=((struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*)_tmp55C)->f3;_LL331:
# 2932
*_tmp5BD=((struct Cyc_Absyn_Enumfield*)_check_null(_tmp5BB))->name;
t=_tmp5BC;goto _LL2E9;case 33: _LL332: _tmp5C2=(int*)&(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp55C)->f1).is_calloc;_tmp5C1=(struct Cyc_Absyn_Exp**)&(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp55C)->f1).rgn;_tmp5C0=(void***)&(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp55C)->f1).elt_type;_tmp5BF=(struct Cyc_Absyn_Exp**)&(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp55C)->f1).num_elts;_tmp5BE=(int*)&(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp55C)->f1).fat_result;_LL333:
# 2935
 t=Cyc_Tcexp_tcMalloc(te,loc,topt,_tmp5C1,_tmp5C0,_tmp5BF,_tmp5C2,_tmp5BE);goto _LL2E9;case 34: _LL334: _tmp5C4=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp5C3=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_LL335:
# 2937
 t=Cyc_Tcexp_tcSwap(te,loc,topt,_tmp5C4,_tmp5C3);goto _LL2E9;case 37: _LL336: _tmp5C6=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_tmp5C5=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmp55C)->f2;_LL337:
# 2939
 t=Cyc_Tcexp_tcTagcheck(te,loc,topt,_tmp5C6,_tmp5C5);goto _LL2E9;case 38: _LL338: _tmp5C7=(void*)((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp55C)->f1;_LL339:
# 2941
 if(!te->allow_valueof)
({void*_tmp56B=0;Cyc_Tcutil_terr(e->loc,({const char*_tmp56C="valueof(-) can only occur within types";_tag_dyneither(_tmp56C,sizeof(char),39);}),_tag_dyneither(_tmp56B,sizeof(void*),0));});
# 2949
t=Cyc_Absyn_sint_typ;
goto _LL2E9;default: _LL33A: _LL33B:
# 2952
({void*_tmp56D=0;Cyc_Tcutil_terr(e->loc,({const char*_tmp56E="asm expressions cannot occur within Cyclone code.";_tag_dyneither(_tmp56E,sizeof(char),50);}),_tag_dyneither(_tmp56D,sizeof(void*),0));});
t=Cyc_Absyn_wildtyp(Cyc_Tcenv_lookup_opt_type_vars(te));}_LL2E9:;}
# 2955
e->topt=t;}
