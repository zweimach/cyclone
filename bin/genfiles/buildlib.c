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
#endif
};

struct _DynRegionFrame {
  struct _RuntimeStack s;
  struct _DynRegionHandle *x;
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
# 69 "core.h"
void exit(int);struct Cyc_Core_NewRegion{struct _DynRegionHandle*dynregion;};
# 26 "cycboot.h"
typedef unsigned long Cyc_size_t;
# 33
typedef unsigned short Cyc_mode_t;
# 38
int Cyc_open(const char*,int,struct _dyneither_ptr);struct Cyc___cycFILE;
# 49
typedef struct Cyc___cycFILE Cyc_FILE;
# 51
extern struct Cyc___cycFILE*Cyc_stdout;
# 53
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 68
typedef void*Cyc_parg_t;
# 73
struct _dyneither_ptr Cyc_aprintf(struct _dyneither_ptr,struct _dyneither_ptr);
# 79
int Cyc_fclose(struct Cyc___cycFILE*);
# 88
int Cyc_fflush(struct Cyc___cycFILE*);
# 98
struct Cyc___cycFILE*Cyc_fopen(const char*,const char*);
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _dyneither_ptr,struct _dyneither_ptr);
# 104
int Cyc_fputc(int,struct Cyc___cycFILE*);
# 106
int Cyc_fputs(const char*,struct Cyc___cycFILE*);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned int*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _dyneither_ptr f1;};
# 127
typedef void*Cyc_sarg_t;
# 224 "cycboot.h"
int Cyc_vfprintf(struct Cyc___cycFILE*,struct _dyneither_ptr,struct _dyneither_ptr);extern char Cyc_FileCloseError[15];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14];struct Cyc_FileOpenError_exn_struct{char*tag;struct _dyneither_ptr f1;};
# 300 "cycboot.h"
int isspace(int);
# 310
int toupper(int);
# 318
int system(const char*);
# 321
int mkdir(const char*pathname,unsigned short mode);
# 324
int close(int);
int chdir(const char*);
struct _dyneither_ptr Cyc_getcwd(struct _dyneither_ptr buf,unsigned long size);
# 89 "core.h"
typedef unsigned int Cyc_Core_sizeof_t;struct Cyc_Core_Opt{void*v;};
# 93
typedef struct Cyc_Core_Opt*Cyc_Core_opt_t;extern char Cyc_Core_Invalid_argument[17];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _dyneither_ptr f1;};extern char Cyc_Core_Failure[8];struct Cyc_Core_Failure_exn_struct{char*tag;struct _dyneither_ptr f1;};extern char Cyc_Core_Impossible[11];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _dyneither_ptr f1;};extern char Cyc_Core_Not_found[10];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _dyneither_ptr f1;};
# 160 "core.h"
extern struct _RegionHandle*Cyc_Core_heap_region;
# 162
extern struct _RegionHandle*Cyc_Core_unique_region;extern char Cyc_Core_Open_Region[12];struct Cyc_Core_Open_Region_exn_struct{char*tag;};extern char Cyc_Core_Free_Region[12];struct Cyc_Core_Free_Region_exn_struct{char*tag;};
# 246 "core.h"
struct _dyneither_ptr Cstring_to_string(char*);
# 256
inline static void* arrcast(struct _dyneither_ptr dyn,unsigned int bd,unsigned int sz){
# 261
if(bd >> 20  || sz >> 12)
return 0;{
unsigned char*ptrbd=dyn.curr + bd * sz;
if(((ptrbd < dyn.curr  || dyn.curr == 0) || dyn.curr < dyn.base) || ptrbd > dyn.last_plus_one)
# 268
return 0;
return dyn.curr;};}extern char Cyc_Lexing_Error[6];struct Cyc_Lexing_Error_exn_struct{char*tag;struct _dyneither_ptr f1;};struct Cyc_Lexing_lexbuf{void(*refill_buff)(struct Cyc_Lexing_lexbuf*);void*refill_state;struct _dyneither_ptr lex_buffer;int lex_buffer_len;int lex_abs_pos;int lex_start_pos;int lex_curr_pos;int lex_last_pos;int lex_last_action;int lex_eof_reached;};
# 57 "lexing.h"
typedef struct Cyc_Lexing_lexbuf*Cyc_Lexing_Lexbuf;struct Cyc_Lexing_function_lexbuf_state{int(*read_fun)(struct _dyneither_ptr,int,void*);void*read_fun_state;};
# 64
typedef struct Cyc_Lexing_function_lexbuf_state*Cyc_Lexing_Function_lexbuf_state;struct Cyc_Lexing_lex_tables{struct _dyneither_ptr lex_base;struct _dyneither_ptr lex_backtrk;struct _dyneither_ptr lex_default;struct _dyneither_ptr lex_trans;struct _dyneither_ptr lex_check;};
# 74
typedef struct Cyc_Lexing_lex_tables*Cyc_Lexing_LexTables;
# 80
struct Cyc_Lexing_lexbuf*Cyc_Lexing_from_file(struct Cyc___cycFILE*);
# 84
struct _dyneither_ptr Cyc_Lexing_lexeme(struct Cyc_Lexing_lexbuf*);
char Cyc_Lexing_lexeme_char(struct Cyc_Lexing_lexbuf*,int);
int Cyc_Lexing_lexeme_start(struct Cyc_Lexing_lexbuf*);
int Cyc_Lexing_lexeme_end(struct Cyc_Lexing_lexbuf*);struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 39 "list.h"
typedef struct Cyc_List_List*Cyc_List_list_t;
# 49 "list.h"
typedef struct Cyc_List_List*Cyc_List_List_t;
# 54
struct Cyc_List_List*Cyc_List_list(struct _dyneither_ptr);
# 76
struct Cyc_List_List*Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 172
struct Cyc_List_List*Cyc_List_rev(struct Cyc_List_List*x);
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 184
struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4];struct Cyc_List_Nth_exn_struct{char*tag;};
# 322
int Cyc_List_mem(int(*compare)(void*,void*),struct Cyc_List_List*l,void*x);struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};
# 34 "iter.h"
typedef struct Cyc_Iter_Iter Cyc_Iter_iter_t;
# 37
int Cyc_Iter_next(struct Cyc_Iter_Iter,void*);struct Cyc_Set_Set;
# 40 "set.h"
typedef struct Cyc_Set_Set*Cyc_Set_set_t;
# 51 "set.h"
struct Cyc_Set_Set*Cyc_Set_empty(int(*cmp)(void*,void*));
# 63
struct Cyc_Set_Set*Cyc_Set_insert(struct Cyc_Set_Set*s,void*elt);
# 75
struct Cyc_Set_Set*Cyc_Set_union_two(struct Cyc_Set_Set*s1,struct Cyc_Set_Set*s2);
# 82
struct Cyc_Set_Set*Cyc_Set_diff(struct Cyc_Set_Set*s1,struct Cyc_Set_Set*s2);
# 85
struct Cyc_Set_Set*Cyc_Set_delete(struct Cyc_Set_Set*s,void*elt);
# 94
int Cyc_Set_cardinality(struct Cyc_Set_Set*s);
# 100
int Cyc_Set_member(struct Cyc_Set_Set*s,void*elt);extern char Cyc_Set_Absent[7];struct Cyc_Set_Absent_exn_struct{char*tag;};
# 141
struct Cyc_Iter_Iter Cyc_Set_make_iter(struct _RegionHandle*rgn,struct Cyc_Set_Set*s);
# 38 "string.h"
unsigned long Cyc_strlen(struct _dyneither_ptr s);
# 50 "string.h"
int Cyc_strptrcmp(struct _dyneither_ptr*s1,struct _dyneither_ptr*s2);
# 62
struct _dyneither_ptr Cyc_strconcat(struct _dyneither_ptr,struct _dyneither_ptr);
# 64
struct _dyneither_ptr Cyc_strconcat_l(struct Cyc_List_List*);
# 66
struct _dyneither_ptr Cyc_str_sepstr(struct Cyc_List_List*,struct _dyneither_ptr);
# 102 "string.h"
struct _dyneither_ptr Cyc_strdup(struct _dyneither_ptr src);
# 107
struct _dyneither_ptr Cyc_substring(struct _dyneither_ptr,int ofs,unsigned long n);struct Cyc_Hashtable_Table;
# 35 "hashtable.h"
typedef struct Cyc_Hashtable_Table*Cyc_Hashtable_table_t;
# 39
struct Cyc_Hashtable_Table*Cyc_Hashtable_create(int sz,int(*cmp)(void*,void*),int(*hash)(void*));
# 50
void Cyc_Hashtable_insert(struct Cyc_Hashtable_Table*t,void*key,void*val);
# 52
void*Cyc_Hashtable_lookup(struct Cyc_Hashtable_Table*t,void*key);
# 78
int Cyc_Hashtable_hash_stringptr(struct _dyneither_ptr*p);
# 30 "filename.h"
struct _dyneither_ptr Cyc_Filename_concat(struct _dyneither_ptr,struct _dyneither_ptr);
# 34
struct _dyneither_ptr Cyc_Filename_chop_extension(struct _dyneither_ptr);
# 40
struct _dyneither_ptr Cyc_Filename_dirname(struct _dyneither_ptr);
# 43
struct _dyneither_ptr Cyc_Filename_basename(struct _dyneither_ptr);struct Cyc_Lineno_Pos{struct _dyneither_ptr logical_file;struct _dyneither_ptr line;int line_no;int col;};
# 32 "lineno.h"
typedef struct Cyc_Lineno_Pos*Cyc_Lineno_pos_t;extern char Cyc_Position_Exit[5];struct Cyc_Position_Exit_exn_struct{char*tag;};
# 32 "position.h"
void Cyc_Position_reset_position(struct _dyneither_ptr);
# 37
typedef unsigned int Cyc_Position_seg_t;struct Cyc_Position_Lex_Position_Error_kind_struct{int tag;};struct Cyc_Position_Parse_Position_Error_kind_struct{int tag;};struct Cyc_Position_Elab_Position_Error_kind_struct{int tag;};
# 46
typedef void*Cyc_Position_error_kind_t;struct Cyc_Position_Error{struct _dyneither_ptr source;unsigned int seg;void*kind;struct _dyneither_ptr desc;};
# 53
typedef struct Cyc_Position_Error*Cyc_Position_error_t;extern char Cyc_Position_Nocontext[10];struct Cyc_Position_Nocontext_exn_struct{char*tag;};extern char Cyc_Arg_Bad[4];struct Cyc_Arg_Bad_exn_struct{char*tag;struct _dyneither_ptr f1;};extern char Cyc_Arg_Error[6];struct Cyc_Arg_Error_exn_struct{char*tag;};struct Cyc_Arg_Unit_spec_Arg_Spec_struct{int tag;void(*f1)();};struct Cyc_Arg_Flag_spec_Arg_Spec_struct{int tag;void(*f1)(struct _dyneither_ptr);};struct Cyc_Arg_FlagString_spec_Arg_Spec_struct{int tag;void(*f1)(struct _dyneither_ptr,struct _dyneither_ptr);};struct Cyc_Arg_Set_spec_Arg_Spec_struct{int tag;int*f1;};struct Cyc_Arg_Clear_spec_Arg_Spec_struct{int tag;int*f1;};struct Cyc_Arg_String_spec_Arg_Spec_struct{int tag;void(*f1)(struct _dyneither_ptr);};struct Cyc_Arg_Int_spec_Arg_Spec_struct{int tag;void(*f1)(int);};struct Cyc_Arg_Rest_spec_Arg_Spec_struct{int tag;void(*f1)(struct _dyneither_ptr);};
# 55 "arg.h"
typedef void*Cyc_Arg_spec_t;
# 57
typedef struct Cyc_List_List*Cyc_Arg_speclist_t;
# 66
void Cyc_Arg_usage(struct Cyc_List_List*,struct _dyneither_ptr);
# 71
void Cyc_Arg_parse(struct Cyc_List_List*specs,void(*anonfun)(struct _dyneither_ptr),struct _dyneither_ptr errmsg,struct _dyneither_ptr args);struct Cyc_Buffer_t;
# 46 "buffer.h"
typedef struct Cyc_Buffer_t*Cyc_Buffer_T;
# 49
struct Cyc_Buffer_t*Cyc_Buffer_create(unsigned int n);
# 57
struct _dyneither_ptr Cyc_Buffer_contents(struct Cyc_Buffer_t*);
# 70
void Cyc_Buffer_add_char(struct Cyc_Buffer_t*,char);
# 81 "buffer.h"
void Cyc_Buffer_add_string(struct Cyc_Buffer_t*,struct _dyneither_ptr);
# 80 "absyn.h"
typedef struct _dyneither_ptr*Cyc_Absyn_field_name_t;
typedef struct _dyneither_ptr*Cyc_Absyn_var_t;
typedef struct _dyneither_ptr*Cyc_Absyn_tvarname_t;
typedef struct _dyneither_ptr*Cyc_Absyn_var_opt_t;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};
# 92
typedef union Cyc_Absyn_Nmspace Cyc_Absyn_nmspace_t;
union Cyc_Absyn_Nmspace Cyc_Absyn_Loc_n;
union Cyc_Absyn_Nmspace Cyc_Absyn_Rel_n(struct Cyc_List_List*);
# 96
union Cyc_Absyn_Nmspace Cyc_Absyn_Abs_n(struct Cyc_List_List*ns,int C_scope);struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _dyneither_ptr*f2;};
# 99
typedef struct _tuple0*Cyc_Absyn_qvar_t;typedef struct _tuple0*Cyc_Absyn_qvar_opt_t;
typedef struct _tuple0*Cyc_Absyn_typedef_name_t;
typedef struct _tuple0*Cyc_Absyn_typedef_name_opt_t;
# 104
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
# 155
enum Cyc_Absyn_Scope{Cyc_Absyn_Static  = 0,Cyc_Absyn_Abstract  = 1,Cyc_Absyn_Public  = 2,Cyc_Absyn_Extern  = 3,Cyc_Absyn_ExternC  = 4,Cyc_Absyn_Register  = 5};struct Cyc_Absyn_Tqual{int print_const;int q_volatile;int q_restrict;int real_const;unsigned int loc;};
# 176
enum Cyc_Absyn_Size_of{Cyc_Absyn_Char_sz  = 0,Cyc_Absyn_Short_sz  = 1,Cyc_Absyn_Int_sz  = 2,Cyc_Absyn_Long_sz  = 3,Cyc_Absyn_LongLong_sz  = 4};
# 181
enum Cyc_Absyn_AliasQual{Cyc_Absyn_Aliasable  = 0,Cyc_Absyn_Unique  = 1,Cyc_Absyn_Top  = 2};
# 188
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind  = 0,Cyc_Absyn_MemKind  = 1,Cyc_Absyn_BoxKind  = 2,Cyc_Absyn_RgnKind  = 3,Cyc_Absyn_EffKind  = 4,Cyc_Absyn_IntKind  = 5};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};
# 208
enum Cyc_Absyn_Sign{Cyc_Absyn_Signed  = 0,Cyc_Absyn_Unsigned  = 1,Cyc_Absyn_None  = 2};
# 210
enum Cyc_Absyn_AggrKind{Cyc_Absyn_StructA  = 0,Cyc_Absyn_UnionA  = 1};struct _union_Constraint_Eq_constr{int tag;void*val;};struct _union_Constraint_Forward_constr{int tag;union Cyc_Absyn_Constraint*val;};struct _union_Constraint_No_constr{int tag;int val;};union Cyc_Absyn_Constraint{struct _union_Constraint_Eq_constr Eq_constr;struct _union_Constraint_Forward_constr Forward_constr;struct _union_Constraint_No_constr No_constr;};
# 219
typedef union Cyc_Absyn_Constraint*Cyc_Absyn_conref_t;struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _dyneither_ptr*name;int identity;void*kind;};struct Cyc_Absyn_DynEither_b_Absyn_Bounds_struct{int tag;};struct Cyc_Absyn_Upper_b_Absyn_Bounds_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_PtrLoc{unsigned int ptr_loc;unsigned int rgn_loc;unsigned int zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;union Cyc_Absyn_Constraint*nullable;union Cyc_Absyn_Constraint*bounds;union Cyc_Absyn_Constraint*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_typ;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_Numelts_ptrqual_Absyn_Pointer_qual_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Region_ptrqual_Absyn_Pointer_qual_struct{int tag;void*f1;};struct Cyc_Absyn_Thin_ptrqual_Absyn_Pointer_qual_struct{int tag;};struct Cyc_Absyn_Fat_ptrqual_Absyn_Pointer_qual_struct{int tag;};struct Cyc_Absyn_Zeroterm_ptrqual_Absyn_Pointer_qual_struct{int tag;};struct Cyc_Absyn_Nozeroterm_ptrqual_Absyn_Pointer_qual_struct{int tag;};struct Cyc_Absyn_Notnull_ptrqual_Absyn_Pointer_qual_struct{int tag;};struct Cyc_Absyn_Nullable_ptrqual_Absyn_Pointer_qual_struct{int tag;};
# 280
typedef void*Cyc_Absyn_pointer_qual_t;
typedef struct Cyc_List_List*Cyc_Absyn_pointer_quals_t;struct Cyc_Absyn_VarargInfo{struct _dyneither_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_typ;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple0*name;int is_extensible;};struct _union_DatatypeInfoU_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfoU_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfoU{struct _union_DatatypeInfoU_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfoU_KnownDatatype KnownDatatype;};struct Cyc_Absyn_DatatypeInfo{union Cyc_Absyn_DatatypeInfoU datatype_info;struct Cyc_List_List*targs;};struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple0*datatype_name;struct _tuple0*field_name;int is_extensible;};struct _union_DatatypeFieldInfoU_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple1{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfoU_KnownDatatypefield{int tag;struct _tuple1 val;};union Cyc_Absyn_DatatypeFieldInfoU{struct _union_DatatypeFieldInfoU_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfoU_KnownDatatypefield KnownDatatypefield;};struct Cyc_Absyn_DatatypeFieldInfo{union Cyc_Absyn_DatatypeFieldInfoU field_info;struct Cyc_List_List*targs;};struct _tuple2{enum Cyc_Absyn_AggrKind f1;struct _tuple0*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfoU_UnknownAggr{int tag;struct _tuple2 val;};struct _union_AggrInfoU_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfoU{struct _union_AggrInfoU_UnknownAggr UnknownAggr;struct _union_AggrInfoU_KnownAggr KnownAggr;};struct Cyc_Absyn_AggrInfo{union Cyc_Absyn_AggrInfoU aggr_info;struct Cyc_List_List*targs;};struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;union Cyc_Absyn_Constraint*zero_term;unsigned int zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};
# 373
typedef void*Cyc_Absyn_raw_type_decl_t;struct Cyc_Absyn_TypeDecl{void*r;unsigned int loc;};
# 378
typedef struct Cyc_Absyn_TypeDecl*Cyc_Absyn_type_decl_t;struct Cyc_Absyn_VoidType_Absyn_Type_struct{int tag;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_DatatypeType_Absyn_Type_struct{int tag;struct Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldType_Absyn_Type_struct{int tag;struct Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_IntType_Absyn_Type_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatType_Absyn_Type_struct{int tag;int f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AggrType_Absyn_Type_struct{int tag;struct Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_EnumType_Absyn_Type_struct{int tag;struct _tuple0*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_RgnHandleType_Absyn_Type_struct{int tag;void*f1;};struct Cyc_Absyn_DynRgnType_Absyn_Type_struct{int tag;void*f1;void*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TagType_Absyn_Type_struct{int tag;void*f1;};struct Cyc_Absyn_HeapRgn_Absyn_Type_struct{int tag;};struct Cyc_Absyn_UniqueRgn_Absyn_Type_struct{int tag;};struct Cyc_Absyn_RefCntRgn_Absyn_Type_struct{int tag;};struct Cyc_Absyn_AccessEff_Absyn_Type_struct{int tag;void*f1;};struct Cyc_Absyn_JoinEff_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_RgnsEff_Absyn_Type_struct{int tag;void*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned int f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;};
# 444 "absyn.h"
typedef void*Cyc_Absyn_funcparams_t;
# 447
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft  = 0,Cyc_Absyn_Scanf_ft  = 1};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noconsume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _dyneither_ptr f1;};struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;union Cyc_Absyn_Constraint*f1;unsigned int f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;union Cyc_Absyn_Constraint*f2;unsigned int f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned int f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned int f1;struct Cyc_List_List*f2;};
# 508
typedef void*Cyc_Absyn_type_modifier_t;struct _union_Cnst_Null_c{int tag;int val;};struct _tuple3{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple3 val;};struct _union_Cnst_Wchar_c{int tag;struct _dyneither_ptr val;};struct _tuple4{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple4 val;};struct _tuple5{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple6 val;};struct _tuple7{struct _dyneither_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple7 val;};struct _union_Cnst_String_c{int tag;struct _dyneither_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _dyneither_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 534
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus  = 0,Cyc_Absyn_Times  = 1,Cyc_Absyn_Minus  = 2,Cyc_Absyn_Div  = 3,Cyc_Absyn_Mod  = 4,Cyc_Absyn_Eq  = 5,Cyc_Absyn_Neq  = 6,Cyc_Absyn_Gt  = 7,Cyc_Absyn_Lt  = 8,Cyc_Absyn_Gte  = 9,Cyc_Absyn_Lte  = 10,Cyc_Absyn_Not  = 11,Cyc_Absyn_Bitnot  = 12,Cyc_Absyn_Bitand  = 13,Cyc_Absyn_Bitor  = 14,Cyc_Absyn_Bitxor  = 15,Cyc_Absyn_Bitlshift  = 16,Cyc_Absyn_Bitlrshift  = 17,Cyc_Absyn_Bitarshift  = 18,Cyc_Absyn_Numelts  = 19};
# 541
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc  = 0,Cyc_Absyn_PostInc  = 1,Cyc_Absyn_PreDec  = 2,Cyc_Absyn_PostDec  = 3};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _dyneither_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned int f1;};
# 559
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion  = 0,Cyc_Absyn_No_coercion  = 1,Cyc_Absyn_NonNull_to_Null  = 2,Cyc_Absyn_Other_coercion  = 3};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;struct _tuple0*f1;void*f2;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftyp_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _dyneither_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _dyneither_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple8{struct _dyneither_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple8*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct _tuple0*f1;struct Cyc_Absyn_Enumdecl*f2;struct Cyc_Absyn_Enumfield*f3;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;struct _tuple0*f1;void*f2;struct Cyc_Absyn_Enumfield*f3;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _dyneither_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _dyneither_ptr f2;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned int loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple9{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple9 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _dyneither_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple9 f2;struct _tuple9 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _dyneither_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple9 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ResetRegion_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Stmt{void*r;unsigned int loc;struct Cyc_List_List*non_local_preds;int try_depth;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _dyneither_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned int loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned int loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple0*name;struct Cyc_List_List*tvs;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_Absyn_Stmt*body;void*cached_typ;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;struct Cyc_List_List*attributes;};struct Cyc_Absyn_Aggrfield{struct _dyneither_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;};struct Cyc_Absyn_Datatypefield{struct _tuple0*name;struct Cyc_List_List*typs;unsigned int loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple0*name;struct Cyc_Absyn_Exp*tag;unsigned int loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple0*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;int f3;struct Cyc_Absyn_Exp*f4;};struct Cyc_Absyn_Alias_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _dyneither_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned int loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _dyneither_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 1100 "absyn.h"
struct Cyc_Absyn_Decl*Cyc_Absyn_new_decl(void*r,unsigned int loc);struct _tuple10{enum Cyc_Absyn_AggrKind f1;struct _tuple0*f2;};
# 1161
struct _tuple10 Cyc_Absyn_aggr_kinded_name(union Cyc_Absyn_AggrInfoU);struct Cyc_PP_Ppstate;
# 41 "pp.h"
typedef struct Cyc_PP_Ppstate*Cyc_PP_ppstate_t;struct Cyc_PP_Out;
# 43
typedef struct Cyc_PP_Out*Cyc_PP_out_t;struct Cyc_PP_Doc;
# 45
typedef struct Cyc_PP_Doc*Cyc_PP_doc_t;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 56 "absynpp.h"
void Cyc_Absynpp_set_params(struct Cyc_Absynpp_Params*fs);
# 58
extern struct Cyc_Absynpp_Params Cyc_Absynpp_cyc_params_r;
# 60
void Cyc_Absynpp_decllist2file(struct Cyc_List_List*tdl,struct Cyc___cycFILE*f);
# 32 "parse.h"
struct Cyc_List_List*Cyc_Parse_parse_file(struct Cyc___cycFILE*f);struct Cyc_FlatList{struct Cyc_FlatList*tl;};
# 36
typedef struct Cyc_FlatList*Cyc_flat_list_t;struct Cyc_Type_specifier{int Signed_spec: 1;int Unsigned_spec: 1;int Short_spec: 1;int Long_spec: 1;int Long_Long_spec: 1;int Valid_type_spec: 1;void*Type_spec;unsigned int loc;};
# 47
typedef struct Cyc_Type_specifier Cyc_type_specifier_t;struct Cyc_Declarator{struct _tuple0*id;struct Cyc_List_List*tms;};
# 52
typedef struct Cyc_Declarator Cyc_declarator_t;struct _tuple12{struct Cyc_Declarator f1;struct Cyc_Absyn_Exp*f2;};struct _tuple11{struct _tuple11*tl;struct _tuple12 hd;};
typedef struct _tuple11*Cyc_declarator_list_t;
# 55
enum Cyc_Storage_class{Cyc_Typedef_sc  = 0,Cyc_Extern_sc  = 1,Cyc_ExternC_sc  = 2,Cyc_Static_sc  = 3,Cyc_Auto_sc  = 4,Cyc_Register_sc  = 5,Cyc_Abstract_sc  = 6};
# 59
typedef enum Cyc_Storage_class Cyc_storage_class_t;struct Cyc_Declaration_spec{enum Cyc_Storage_class*sc;struct Cyc_Absyn_Tqual tq;struct Cyc_Type_specifier type_specs;int is_inline;struct Cyc_List_List*attributes;};
# 67
typedef struct Cyc_Declaration_spec Cyc_decl_spec_t;struct Cyc_Abstractdeclarator{struct Cyc_List_List*tms;};
# 71
typedef struct Cyc_Abstractdeclarator Cyc_abstractdeclarator_t;struct _union_YYSTYPE_Int_tok{int tag;struct _tuple5 val;};struct _union_YYSTYPE_Char_tok{int tag;char val;};struct _union_YYSTYPE_String_tok{int tag;struct _dyneither_ptr val;};struct _union_YYSTYPE_Stringopt_tok{int tag;struct Cyc_Core_Opt*val;};struct _union_YYSTYPE_QualId_tok{int tag;struct _tuple0*val;};struct _tuple13{int f1;struct _dyneither_ptr f2;};struct _union_YYSTYPE_Asm_tok{int tag;struct _tuple13 val;};struct _tuple14{unsigned int f1;union Cyc_Absyn_Constraint*f2;union Cyc_Absyn_Constraint*f3;};struct _union_YYSTYPE_YY1{int tag;struct _tuple14*val;};struct _union_YYSTYPE_YY2{int tag;union Cyc_Absyn_Constraint*val;};struct _union_YYSTYPE_YY3{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY4{int tag;struct Cyc_Absyn_Exp*val;};struct _union_YYSTYPE_YY5{int tag;struct Cyc_Absyn_Exp*val;};struct _union_YYSTYPE_YY6{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY7{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY8{int tag;enum Cyc_Absyn_Primop val;};struct _union_YYSTYPE_YY9{int tag;struct Cyc_Core_Opt*val;};struct _union_YYSTYPE_YY10{int tag;struct Cyc_Absyn_Stmt*val;};struct _union_YYSTYPE_YY11{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY12{int tag;struct Cyc_Absyn_Pat*val;};struct _tuple15{struct Cyc_List_List*f1;int f2;};struct _union_YYSTYPE_YY13{int tag;struct _tuple15*val;};struct _union_YYSTYPE_YY14{int tag;struct Cyc_List_List*val;};struct _tuple16{struct Cyc_List_List*f1;struct Cyc_Absyn_Pat*f2;};struct _union_YYSTYPE_YY15{int tag;struct _tuple16*val;};struct _union_YYSTYPE_YY16{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY17{int tag;struct _tuple15*val;};struct _union_YYSTYPE_YY18{int tag;struct Cyc_Absyn_Fndecl*val;};struct _union_YYSTYPE_YY19{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY20{int tag;struct Cyc_Declaration_spec val;};struct _union_YYSTYPE_YY21{int tag;struct _tuple12 val;};struct _union_YYSTYPE_YY22{int tag;struct _tuple11*val;};struct _union_YYSTYPE_YY23{int tag;enum Cyc_Storage_class*val;};struct _union_YYSTYPE_YY24{int tag;struct Cyc_Type_specifier val;};struct _union_YYSTYPE_YY25{int tag;enum Cyc_Absyn_AggrKind val;};struct _union_YYSTYPE_YY26{int tag;struct Cyc_Absyn_Tqual val;};struct _union_YYSTYPE_YY27{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY28{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY29{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY30{int tag;struct Cyc_Declarator val;};struct _tuple17{struct Cyc_Declarator f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct _union_YYSTYPE_YY31{int tag;struct _tuple17*val;};struct _union_YYSTYPE_YY32{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY33{int tag;struct Cyc_Abstractdeclarator val;};struct _union_YYSTYPE_YY34{int tag;int val;};struct _union_YYSTYPE_YY35{int tag;enum Cyc_Absyn_Scope val;};struct _union_YYSTYPE_YY36{int tag;struct Cyc_Absyn_Datatypefield*val;};struct _union_YYSTYPE_YY37{int tag;struct Cyc_List_List*val;};struct _tuple18{struct Cyc_Absyn_Tqual f1;struct Cyc_Type_specifier f2;struct Cyc_List_List*f3;};struct _union_YYSTYPE_YY38{int tag;struct _tuple18 val;};struct _union_YYSTYPE_YY39{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY40{int tag;struct _tuple8*val;};struct _union_YYSTYPE_YY41{int tag;struct Cyc_List_List*val;};struct _tuple19{struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;};struct _union_YYSTYPE_YY42{int tag;struct _tuple19*val;};struct _union_YYSTYPE_YY43{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY44{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY45{int tag;void*val;};struct _union_YYSTYPE_YY46{int tag;struct Cyc_Absyn_Kind*val;};struct _union_YYSTYPE_YY47{int tag;void*val;};struct _union_YYSTYPE_YY48{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY49{int tag;void*val;};struct _union_YYSTYPE_YY50{int tag;struct Cyc_Absyn_Enumfield*val;};struct _union_YYSTYPE_YY51{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY52{int tag;void*val;};struct _union_YYSTYPE_YY53{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY54{int tag;union Cyc_Absyn_Constraint*val;};struct _union_YYSTYPE_YY55{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY56{int tag;void*val;};struct _union_YYSTYPE_YY57{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YYINITIALSVAL{int tag;int val;};union Cyc_YYSTYPE{struct _union_YYSTYPE_Int_tok Int_tok;struct _union_YYSTYPE_Char_tok Char_tok;struct _union_YYSTYPE_String_tok String_tok;struct _union_YYSTYPE_Stringopt_tok Stringopt_tok;struct _union_YYSTYPE_QualId_tok QualId_tok;struct _union_YYSTYPE_Asm_tok Asm_tok;struct _union_YYSTYPE_YY1 YY1;struct _union_YYSTYPE_YY2 YY2;struct _union_YYSTYPE_YY3 YY3;struct _union_YYSTYPE_YY4 YY4;struct _union_YYSTYPE_YY5 YY5;struct _union_YYSTYPE_YY6 YY6;struct _union_YYSTYPE_YY7 YY7;struct _union_YYSTYPE_YY8 YY8;struct _union_YYSTYPE_YY9 YY9;struct _union_YYSTYPE_YY10 YY10;struct _union_YYSTYPE_YY11 YY11;struct _union_YYSTYPE_YY12 YY12;struct _union_YYSTYPE_YY13 YY13;struct _union_YYSTYPE_YY14 YY14;struct _union_YYSTYPE_YY15 YY15;struct _union_YYSTYPE_YY16 YY16;struct _union_YYSTYPE_YY17 YY17;struct _union_YYSTYPE_YY18 YY18;struct _union_YYSTYPE_YY19 YY19;struct _union_YYSTYPE_YY20 YY20;struct _union_YYSTYPE_YY21 YY21;struct _union_YYSTYPE_YY22 YY22;struct _union_YYSTYPE_YY23 YY23;struct _union_YYSTYPE_YY24 YY24;struct _union_YYSTYPE_YY25 YY25;struct _union_YYSTYPE_YY26 YY26;struct _union_YYSTYPE_YY27 YY27;struct _union_YYSTYPE_YY28 YY28;struct _union_YYSTYPE_YY29 YY29;struct _union_YYSTYPE_YY30 YY30;struct _union_YYSTYPE_YY31 YY31;struct _union_YYSTYPE_YY32 YY32;struct _union_YYSTYPE_YY33 YY33;struct _union_YYSTYPE_YY34 YY34;struct _union_YYSTYPE_YY35 YY35;struct _union_YYSTYPE_YY36 YY36;struct _union_YYSTYPE_YY37 YY37;struct _union_YYSTYPE_YY38 YY38;struct _union_YYSTYPE_YY39 YY39;struct _union_YYSTYPE_YY40 YY40;struct _union_YYSTYPE_YY41 YY41;struct _union_YYSTYPE_YY42 YY42;struct _union_YYSTYPE_YY43 YY43;struct _union_YYSTYPE_YY44 YY44;struct _union_YYSTYPE_YY45 YY45;struct _union_YYSTYPE_YY46 YY46;struct _union_YYSTYPE_YY47 YY47;struct _union_YYSTYPE_YY48 YY48;struct _union_YYSTYPE_YY49 YY49;struct _union_YYSTYPE_YY50 YY50;struct _union_YYSTYPE_YY51 YY51;struct _union_YYSTYPE_YY52 YY52;struct _union_YYSTYPE_YY53 YY53;struct _union_YYSTYPE_YY54 YY54;struct _union_YYSTYPE_YY55 YY55;struct _union_YYSTYPE_YY56 YY56;struct _union_YYSTYPE_YY57 YY57;struct _union_YYSTYPE_YYINITIALSVAL YYINITIALSVAL;};struct Cyc_Yyltype{int timestamp;int first_line;int first_column;int last_line;int last_column;};
# 78 "parse_tab.h"
typedef struct Cyc_Yyltype Cyc_yyltype;struct Cyc_Dict_T;
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
# 44 "tcenv.h"
typedef const struct Cyc_Tcenv_CList*Cyc_Tcenv_mclist_t;
typedef const struct Cyc_Tcenv_CList*const Cyc_Tcenv_clist_t;struct Cyc_Tcenv_VarRes_Tcenv_Resolved_struct{int tag;void*f1;};struct Cyc_Tcenv_AggrRes_Tcenv_Resolved_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Tcenv_DatatypeRes_Tcenv_Resolved_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct Cyc_Tcenv_EnumRes_Tcenv_Resolved_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Tcenv_AnonEnumRes_Tcenv_Resolved_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};
# 55
typedef void*Cyc_Tcenv_resolved_t;struct Cyc_Tcenv_Genv{struct _RegionHandle*grgn;struct Cyc_Set_Set*namespaces;struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;struct Cyc_List_List*availables;};
# 74
typedef struct Cyc_Tcenv_Genv*Cyc_Tcenv_genv_t;struct Cyc_Tcenv_Fenv;
# 78
typedef struct Cyc_Tcenv_Fenv*Cyc_Tcenv_fenv_t;struct Cyc_Tcenv_NotLoop_j_Tcenv_Jumpee_struct{int tag;};struct Cyc_Tcenv_CaseEnd_j_Tcenv_Jumpee_struct{int tag;};struct Cyc_Tcenv_FnEnd_j_Tcenv_Jumpee_struct{int tag;};struct Cyc_Tcenv_Stmt_j_Tcenv_Jumpee_struct{int tag;struct Cyc_Absyn_Stmt*f1;};
# 89
typedef void*Cyc_Tcenv_jumpee_t;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Dict_Dict ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof;};
# 100
typedef struct Cyc_Tcenv_Tenv*Cyc_Tcenv_tenv_t;
# 118 "tcenv.h"
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_tc_init(struct _RegionHandle*);
# 134
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew  = 0,Cyc_Tcenv_InNew  = 1,Cyc_Tcenv_InNewAggr  = 2};
# 34 "tc.h"
void Cyc_Tc_tc(struct _RegionHandle*,struct Cyc_Tcenv_Tenv*te,int var_default_init,struct Cyc_List_List*ds);
# 81 "buildlib.cyl"
void Cyc_Lex_lex_init(int use_cyclone_keywords);
# 89
extern char*Ccomp;
# 91
static int Cyc_do_setjmp=0;
# 93
struct Cyc___cycFILE*Cyc_log_file=0;
struct Cyc___cycFILE*Cyc_cstubs_file=0;
struct Cyc___cycFILE*Cyc_cycstubs_file=0;
# 97
int Cyc_log(struct _dyneither_ptr fmt,struct _dyneither_ptr ap){
# 100
if(Cyc_log_file == 0){
{const char*_tmp401;void*_tmp400;(_tmp400=0,Cyc_fprintf(Cyc_stderr,((_tmp401="Internal error: log file is NULL\n",_tag_dyneither(_tmp401,sizeof(char),34))),_tag_dyneither(_tmp400,sizeof(void*),0)));}
exit(1);}{
# 104
int _tmp2=Cyc_vfprintf((struct Cyc___cycFILE*)_check_null(Cyc_log_file),fmt,ap);
Cyc_fflush((struct Cyc___cycFILE*)_check_null(Cyc_log_file));
return _tmp2;};}
# 109
static struct _dyneither_ptr*Cyc_current_source=0;
static struct Cyc_List_List*Cyc_current_args=0;
static struct Cyc_Set_Set**Cyc_current_targets=0;
static void Cyc_add_target(struct _dyneither_ptr*sptr){
struct Cyc_Set_Set**_tmp402;Cyc_current_targets=((_tmp402=_cycalloc(sizeof(*_tmp402)),((_tmp402[0]=((struct Cyc_Set_Set*(*)(struct Cyc_Set_Set*s,struct _dyneither_ptr*elt))Cyc_Set_insert)(*((struct Cyc_Set_Set**)_check_null(Cyc_current_targets)),sptr),_tmp402))));}struct _tuple20{struct _dyneither_ptr*f1;struct Cyc_Set_Set*f2;};
# 116
typedef struct _tuple20*Cyc_line_t;
struct _tuple20*Cyc_line(struct Cyc_Lexing_lexbuf*);
int Cyc_macroname(struct Cyc_Lexing_lexbuf*);
int Cyc_args(struct Cyc_Lexing_lexbuf*);
int Cyc_token(struct Cyc_Lexing_lexbuf*);
int Cyc_string(struct Cyc_Lexing_lexbuf*);
# 123
struct Cyc___cycFILE*Cyc_slurp_out=0;
int Cyc_slurp(struct Cyc_Lexing_lexbuf*);
int Cyc_slurp_string(struct Cyc_Lexing_lexbuf*);
int Cyc_asmtok(struct Cyc_Lexing_lexbuf*);
int Cyc_asm_string(struct Cyc_Lexing_lexbuf*);
int Cyc_asm_comment(struct Cyc_Lexing_lexbuf*);struct _tuple21{struct _dyneither_ptr f1;struct _dyneither_ptr*f2;};
# 130
typedef struct _tuple21*Cyc_suck_line_t;
struct _tuple21*Cyc_suck_line(struct Cyc_Lexing_lexbuf*);
int Cyc_suck_macroname(struct Cyc_Lexing_lexbuf*);
int Cyc_suck_restofline(struct Cyc_Lexing_lexbuf*);
struct _dyneither_ptr Cyc_current_line={(void*)0,(void*)0,(void*)(0 + 0)};struct _tuple22{struct _dyneither_ptr f1;struct _dyneither_ptr f2;};
# 138
typedef struct _tuple22*Cyc_ifdefined_t;struct _tuple23{struct _dyneither_ptr f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;struct Cyc_List_List*f6;};
# 140
typedef struct _tuple23*Cyc_spec_t;
# 142
struct _tuple23*Cyc_spec(struct Cyc_Lexing_lexbuf*);
int Cyc_commands(struct Cyc_Lexing_lexbuf*);
int Cyc_snarfsymbols(struct Cyc_Lexing_lexbuf*);
int Cyc_block(struct Cyc_Lexing_lexbuf*);
int Cyc_block_string(struct Cyc_Lexing_lexbuf*);
int Cyc_block_comment(struct Cyc_Lexing_lexbuf*);
struct _dyneither_ptr Cyc_current_headerfile={(void*)0,(void*)0,(void*)(0 + 0)};
struct Cyc_List_List*Cyc_snarfed_symbols=0;
struct Cyc_List_List*Cyc_current_symbols=0;
struct Cyc_List_List*Cyc_current_cstubs=0;
struct Cyc_List_List*Cyc_current_cycstubs=0;
struct Cyc_List_List*Cyc_current_hstubs=0;
struct Cyc_List_List*Cyc_current_omit_symbols=0;
struct Cyc_List_List*Cyc_current_cpp=0;
struct Cyc_Buffer_t*Cyc_specbuf=0;
int Cyc_braces_to_match=0;
int Cyc_parens_to_match=0;
# 160
int Cyc_numdef=0;
# 162
static struct Cyc_List_List*Cyc_cppargs=0;
# 165
const int Cyc_lex_base[425]={0,0,75,192,305,310,311,166,312,91,27,384,28,523,637,715,283,325,92,- 3,0,- 1,- 2,- 8,- 3,1,- 2,323,- 4,2,166,- 5,605,797,312,- 6,- 7,- 4,16,- 3,29,11,835,- 3,910,13,- 14,223,12,- 2,216,20,26,28,33,23,48,70,54,64,74,100,91,107,94,370,386,111,103,95,122,122,375,414,111,111,153,377,1024,1139,525,176,210,228,214,216,217,242,498,1253,1368,- 9,654,- 10,224,245,508,1482,1597,684,- 8,720,- 11,432,510,515,1674,1751,1828,1909,434,465,550,1984,249,249,249,248,244,254,0,13,4,2065,5,628,2073,2138,660,49,467,6,2099,7,705,2161,2199,820,- 22,1051,1056,261,314,242,251,258,250,271,281,274,275,278,288,295,282,- 20,292,296,300,313,321,326,- 15,309,325,319,315,326,365,400,417,418,404,400,400,427,431,- 17,422,421,415,436,433,449,427,449,453,441,445,436,436,- 19,444,438,442,453,464,447,449,482,489,490,4,6,21,491,492,505,504,516,516,524,554,24,556,557,24,20,570,571,53,627,644,- 13,647,594,596,583,584,598,598,605,606,1,677,623,624,654,647,654,658,659,623,624,639,645,648,703,704,705,654,655,710,720,721,679,680,735,737,765,713,714,770,797,798,746,747,803,814,815,- 12,762,763,1030,- 21,1165,762,763,760,773,772,767,769,772,773,771,827,1144,819,820,818,832,1258,1170,871,872,862,864,862,875,1370,893,894,892,905,1375,- 7,- 8,8,1263,2231,9,996,2255,2293,1341,1279,- 49,1150,- 2,945,- 4,974,998,1028,980,1010,1023,1448,981,2320,2363,994,1055,993,996,2433,994,1057,- 36,- 42,- 37,2508,- 28,1002,- 40,- 25,1021,- 27,- 45,- 39,- 48,2583,2612,1467,1081,1091,1563,2622,2652,1582,2281,2685,2716,2754,1083,1093,2824,2862,1118,1157,1195,1206,1198,1241,- 6,- 34,1040,2794,- 47,- 30,- 32,- 46,- 29,- 31,- 33,1048,2902,1112,1117,2128,1119,1124,1125,1133,1134,1138,1146,1147,1191,2975,3059,- 23,- 16,- 18,2239,1192,- 24,- 41,- 38,- 35,- 26,1394,3141,2,3224,1191,15,1170,1174,1175,1173,1171,1185,1255};
const int Cyc_lex_backtrk[425]={- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,2,- 1,- 1,- 1,- 1,2,- 1,8,- 1,3,5,- 1,- 1,6,5,- 1,- 1,- 1,6,- 1,5,1,0,- 1,0,1,- 1,12,13,- 1,13,13,13,13,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,11,12,2,4,4,- 1,0,0,0,2,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,2,2,8,3,5,- 1,6,5,6,5,2,8,3,5,- 1,6,5,- 1,21,21,21,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,15,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,17,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,6,1,9,2,4,- 1,5,4,- 1,- 1,2,- 1,48,- 1,48,48,48,48,48,48,48,48,5,7,48,48,48,48,0,48,48,- 1,- 1,- 1,0,- 1,43,- 1,- 1,42,- 1,- 1,- 1,- 1,9,7,- 1,7,7,- 1,8,9,- 1,- 1,9,5,6,5,5,- 1,4,4,4,6,6,5,5,- 1,- 1,- 1,9,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,2,- 1,1,2,1,1,- 1,- 1,- 1,- 1,- 1,- 1,- 1};
const int Cyc_lex_default[425]={- 1,- 1,- 1,317,306,138,23,36,23,19,- 1,- 1,12,31,46,31,36,23,19,0,- 1,0,0,0,0,- 1,0,- 1,0,- 1,- 1,0,- 1,- 1,- 1,0,0,0,- 1,0,40,- 1,- 1,0,- 1,- 1,0,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,0,103,- 1,- 1,- 1,- 1,- 1,110,110,110,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,130,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,0,- 1,0,- 1,- 1,386,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,- 1,0,- 1,0,0,- 1,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,- 1,- 1,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,- 1,- 1,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1};
const int Cyc_lex_trans[3481]={0,0,0,0,0,0,0,0,0,0,22,19,28,417,19,28,19,28,100,19,45,45,45,45,45,22,45,0,0,0,0,0,21,234,21,418,203,22,- 1,- 1,22,- 1,- 1,45,204,45,224,22,415,415,415,415,415,415,415,415,415,415,31,103,22,205,114,40,214,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,31,217,218,221,415,130,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,413,413,413,413,413,413,413,413,413,413,121,20,74,67,54,55,56,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,57,58,59,60,413,61,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,62,63,37,318,319,318,318,319,39,22,64,65,68,69,70,129,34,34,34,34,34,34,34,34,71,72,318,320,321,75,76,322,323,324,104,104,325,326,104,327,328,329,330,331,331,331,331,331,331,331,331,331,332,77,333,334,335,104,19,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,19,- 1,- 1,337,336,101,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,307,338,37,308,139,139,24,24,139,131,122,94,84,81,85,38,82,86,28,87,24,29,83,25,309,88,95,139,96,22,26,26,21,21,115,116,117,140,118,119,120,142,192,193,26,35,35,35,35,35,35,35,35,178,172,163,30,30,30,30,30,30,30,30,66,66,156,149,66,73,73,78,78,73,150,78,151,152,153,154,66,66,310,155,66,157,158,66,132,123,141,39,73,22,78,159,143,144,145,160,146,161,27,66,147,31,162,21,73,73,164,165,73,148,166,167,168,113,113,113,113,113,113,113,113,113,113,- 1,32,- 1,- 1,73,- 1,22,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,- 1,169,- 1,- 1,113,- 1,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,89,89,28,170,89,171,100,173,174,175,97,97,104,104,97,176,104,112,112,177,179,112,180,89,181,105,105,80,80,105,19,80,21,97,182,104,183,184,185,186,112,187,188,189,190,191,271,194,105,195,80,196,112,112,91,197,112,198,199,21,21,21,106,107,106,106,106,106,106,106,106,106,106,106,21,112,200,201,202,206,207,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,208,209,210,211,106,212,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,47,47,35,213,47,215,216,33,33,33,33,33,33,33,33,33,33,92,92,219,220,92,222,47,33,33,33,33,33,33,128,128,128,128,128,128,128,128,48,223,92,225,- 1,226,- 1,227,228,99,99,229,49,99,230,231,232,233,33,33,33,33,33,33,35,35,35,35,35,35,35,35,99,235,236,237,265,- 1,238,- 1,41,41,239,260,41,101,101,255,250,101,243,240,50,241,244,245,246,51,52,247,248,249,46,41,53,251,252,253,101,137,137,137,137,137,137,137,137,254,93,42,42,42,42,42,42,42,42,42,42,242,256,257,258,93,259,28,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,102,100,261,262,42,263,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,264,91,43,266,267,102,268,33,33,33,33,33,33,33,33,33,33,269,270,272,273,288,283,279,33,33,33,33,33,33,35,35,35,35,35,35,35,35,280,281,282,36,284,285,286,44,44,44,44,44,44,44,44,44,44,21,33,33,33,33,33,33,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,287,289,290,291,44,292,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,301,296,297,28,298,299,300,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,302,303,304,305,44,411,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,78,78,410,408,78,- 1,273,273,385,376,273,315,315,315,315,315,315,315,315,351,383,346,341,78,343,344,409,293,293,273,345,293,275,275,- 1,380,275,274,384,79,79,79,79,79,79,79,79,79,79,350,293,381,382,379,407,275,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,347,348,349,339,79,387,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,31,80,80,294,155,80,287,287,276,191,287,274,412,295,412,412,404,102,277,100,100,35,35,278,80,270,223,275,275,287,46,275,293,293,340,412,293,19,162,403,79,79,79,79,79,79,79,79,79,79,275,100,100,35,35,293,31,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,177,138,21,31,79,31,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,89,89,276,31,89,292,292,294,419,292,316,316,277,420,316,421,422,295,36,423,36,375,- 1,89,424,21,316,316,292,0,316,0,0,316,0,21,31,0,0,90,90,90,90,90,90,90,90,90,90,316,36,21,36,375,0,375,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,0,0,0,0,90,375,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,91,92,92,300,300,92,0,300,305,305,0,0,305,31,31,31,31,31,31,31,31,0,0,0,92,0,300,412,0,412,412,305,0,0,28,0,0,0,0,35,90,90,90,90,90,90,90,90,90,90,412,0,0,0,0,0,0,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,0,0,0,0,90,0,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,97,97,0,377,97,378,378,378,378,378,378,378,378,378,378,0,0,0,0,357,0,357,0,97,358,358,358,358,358,358,358,358,358,358,0,0,0,0,0,98,98,98,98,98,98,98,98,98,98,0,0,0,0,0,0,0,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,0,0,0,0,98,0,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,36,99,99,0,0,99,358,358,358,358,358,358,358,358,358,358,0,0,0,0,361,0,361,0,99,362,362,362,362,362,362,362,362,362,362,0,0,0,0,0,98,98,98,98,98,98,98,98,98,98,0,0,0,0,0,0,0,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,0,0,0,0,98,0,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,108,108,108,108,108,108,108,108,108,108,108,108,22,0,0,0,0,0,0,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,0,0,0,0,108,0,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,109,108,108,108,108,108,108,108,108,108,108,22,0,0,0,0,0,0,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,0,0,0,0,108,0,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,22,0,0,0,0,0,0,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,- 1,0,0,- 1,108,0,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,0,0,0,0,109,109,109,109,109,109,109,109,109,109,109,109,111,0,0,0,0,0,0,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,0,0,0,0,109,0,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,113,113,113,113,113,113,113,113,113,113,0,0,0,0,0,0,0,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,28,0,0,124,113,0,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,0,0,28,0,0,133,125,125,125,125,125,125,125,125,127,127,127,127,127,127,127,127,127,127,0,0,0,0,0,0,0,127,127,127,127,127,127,0,0,0,134,134,134,134,134,134,134,134,0,0,0,0,0,0,0,31,0,0,- 1,0,138,0,0,127,127,127,127,127,127,405,405,405,405,405,405,405,405,0,126,127,127,127,127,127,127,127,127,127,127,31,0,0,0,0,0,0,127,127,127,127,127,127,136,136,136,136,136,136,136,136,136,136,135,0,0,0,0,0,0,136,136,136,136,136,136,0,0,0,127,127,127,127,127,127,19,0,0,311,0,0,136,136,136,136,136,136,136,136,136,136,0,136,136,136,136,136,136,136,136,136,136,136,136,0,0,0,0,0,0,0,0,138,312,312,312,312,312,312,312,312,406,406,406,406,406,406,406,406,0,136,136,136,136,136,136,0,314,314,314,314,314,314,314,314,314,314,0,0,0,0,0,0,0,314,314,314,314,314,314,0,0,28,362,362,362,362,362,362,362,362,362,362,0,0,314,314,314,314,314,314,314,314,314,314,313,314,314,314,314,314,314,314,314,314,314,314,314,0,0,352,0,363,363,363,363,363,363,363,363,364,364,0,0,0,0,0,0,0,0,0,0,0,354,314,314,314,314,314,314,365,0,0,0,0,0,0,0,0,366,0,0,367,352,0,353,353,353,353,353,353,353,353,353,353,354,0,0,0,0,0,0,365,0,0,0,354,0,0,0,0,366,0,355,367,0,0,0,0,0,0,0,356,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,354,0,0,0,0,0,0,355,0,0,0,0,0,0,0,0,356,342,342,342,342,342,342,342,342,342,342,0,0,0,0,0,0,0,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,0,0,0,0,342,0,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,0,0,0,0,0,0,0,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,0,0,0,0,342,0,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,359,359,359,359,359,359,359,359,359,359,0,0,0,0,0,0,0,0,0,0,0,360,93,0,0,0,0,352,93,353,353,353,353,353,353,353,353,353,353,358,358,358,358,358,358,358,358,358,358,0,354,0,0,360,93,0,0,355,0,0,93,91,0,0,0,0,356,91,0,359,359,359,359,359,359,359,359,359,359,0,0,0,354,0,0,0,0,0,0,355,360,93,0,91,0,0,0,93,356,91,0,0,362,362,362,362,362,362,362,362,362,362,0,0,0,0,0,0,0,0,0,0,360,93,93,0,0,0,0,93,93,352,0,363,363,363,363,363,363,363,363,364,364,0,0,0,0,0,0,0,0,0,0,0,354,0,93,0,0,0,0,373,93,0,0,0,0,0,0,352,374,364,364,364,364,364,364,364,364,364,364,0,0,0,0,0,354,0,0,0,0,0,354,373,0,0,0,0,0,371,0,0,374,0,0,0,0,0,372,0,0,378,378,378,378,378,378,378,378,378,378,0,0,0,354,0,0,0,0,0,0,371,360,93,0,0,0,0,0,93,372,368,368,368,368,368,368,368,368,368,368,0,0,0,0,0,0,0,368,368,368,368,368,368,360,93,0,0,0,0,0,93,0,0,0,0,0,0,0,368,368,368,368,368,368,368,368,368,368,0,368,368,368,368,368,368,368,368,368,368,368,368,0,0,0,388,0,369,0,0,389,0,0,0,0,0,370,0,0,390,390,390,390,390,390,390,390,0,368,368,368,368,368,368,391,0,0,0,0,369,0,0,0,0,0,0,0,0,370,0,0,0,0,0,0,0,0,0,0,0,0,0,0,392,0,0,0,0,393,394,0,0,0,395,0,0,0,0,0,0,0,396,0,0,0,397,0,398,0,399,0,400,401,401,401,401,401,401,401,401,401,401,0,0,0,0,0,0,0,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,0,0,0,0,0,0,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,402,0,0,0,0,0,0,0,0,401,401,401,401,401,401,401,401,401,401,0,0,0,0,0,0,0,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,0,0,0,0,0,0,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,19,0,0,414,0,0,0,413,413,413,413,413,413,413,413,413,413,0,0,0,0,0,0,0,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,0,0,0,0,413,0,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,416,0,0,0,0,0,0,0,415,415,415,415,415,415,415,415,415,415,0,0,0,0,0,0,0,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,0,0,0,0,415,0,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
const int Cyc_lex_check[3481]={- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,25,29,0,122,124,131,133,308,311,41,41,45,45,41,417,45,- 1,- 1,- 1,- 1,- 1,120,233,414,0,202,10,12,40,10,12,40,41,202,45,203,20,1,1,1,1,1,1,1,1,1,1,38,48,121,204,10,38,213,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,129,216,217,220,1,129,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,9,18,51,52,53,54,55,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,56,57,58,59,2,60,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,61,62,7,3,3,3,3,3,7,7,63,64,67,68,69,7,30,30,30,30,30,30,30,30,70,71,3,3,3,74,75,3,3,3,47,47,3,3,47,3,3,3,3,3,3,3,3,3,3,3,3,3,3,76,3,3,3,47,0,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,10,12,40,3,3,81,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,3,16,4,5,5,6,8,5,6,8,82,83,50,84,16,50,85,27,86,17,27,50,17,4,87,94,5,95,5,6,8,9,18,114,115,116,5,117,118,119,141,143,143,17,34,34,34,34,34,34,34,34,144,145,146,27,27,27,27,27,27,27,27,65,65,147,148,65,72,72,77,77,72,149,77,150,151,152,153,66,66,4,154,66,156,157,65,6,8,5,16,72,16,77,158,142,142,142,159,142,160,17,66,142,27,161,7,73,73,163,164,73,142,165,166,167,11,11,11,11,11,11,11,11,11,11,103,27,110,103,73,110,3,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,111,168,130,111,11,130,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,88,88,66,169,88,170,171,172,173,174,96,96,104,104,96,175,104,105,105,176,178,105,179,88,180,13,13,80,80,13,73,80,16,96,181,104,182,183,184,185,105,186,187,188,189,190,192,193,13,194,80,195,112,112,4,196,112,197,198,5,6,8,13,13,13,13,13,13,13,13,13,13,13,13,17,112,199,200,201,205,206,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,207,208,209,210,13,211,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,14,14,80,212,14,214,215,32,32,32,32,32,32,32,32,32,32,92,92,218,219,92,221,14,32,32,32,32,32,32,125,125,125,125,125,125,125,125,14,222,92,224,103,225,110,226,227,99,99,228,14,99,229,230,231,232,32,32,32,32,32,32,128,128,128,128,128,128,128,128,99,234,235,236,238,111,237,130,15,15,237,239,15,101,101,240,241,101,242,237,14,237,243,244,245,14,14,246,247,248,249,15,14,250,251,252,101,134,134,134,134,134,134,134,134,253,254,15,15,15,15,15,15,15,15,15,15,237,255,256,257,92,258,13,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,259,99,260,261,15,262,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,263,264,15,265,266,101,267,33,33,33,33,33,33,33,33,33,33,268,269,271,272,276,277,278,33,33,33,33,33,33,137,137,137,137,137,137,137,137,279,280,281,282,283,284,285,42,42,42,42,42,42,42,42,42,42,14,33,33,33,33,33,33,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,286,288,289,290,42,291,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,44,44,44,44,44,44,44,44,44,44,294,295,296,15,297,298,299,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,301,302,303,304,44,320,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,78,78,322,323,78,324,273,273,325,329,273,312,312,312,312,312,312,312,312,332,326,334,337,78,335,335,323,139,139,273,344,139,140,140,324,327,140,273,326,78,78,78,78,78,78,78,78,78,78,347,139,327,327,377,386,140,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,333,333,333,338,78,324,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,78,79,79,139,388,79,287,287,140,389,287,391,318,139,318,318,392,393,140,355,356,365,366,140,79,394,395,275,275,287,396,275,293,293,338,318,293,287,397,398,79,79,79,79,79,79,79,79,79,79,275,355,356,365,366,293,369,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,399,406,416,370,79,369,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,79,89,89,275,370,89,292,292,293,418,292,309,309,275,419,309,420,421,293,371,422,372,373,324,89,423,424,316,316,292,- 1,316,- 1,- 1,309,- 1,309,292,- 1,- 1,89,89,89,89,89,89,89,89,89,89,316,371,316,372,373,- 1,374,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,- 1,- 1,- 1,- 1,89,374,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,90,90,300,300,90,- 1,300,305,305,- 1,- 1,305,315,315,315,315,315,315,315,315,- 1,- 1,- 1,90,- 1,300,412,- 1,412,412,305,- 1,- 1,300,- 1,- 1,- 1,- 1,305,90,90,90,90,90,90,90,90,90,90,412,- 1,- 1,- 1,- 1,- 1,- 1,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,- 1,- 1,- 1,- 1,90,- 1,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,97,97,- 1,328,97,328,328,328,328,328,328,328,328,328,328,- 1,- 1,- 1,- 1,354,- 1,354,- 1,97,354,354,354,354,354,354,354,354,354,354,- 1,- 1,- 1,- 1,- 1,97,97,97,97,97,97,97,97,97,97,- 1,- 1,- 1,- 1,- 1,- 1,- 1,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,- 1,- 1,- 1,- 1,97,- 1,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,98,98,- 1,- 1,98,357,357,357,357,357,357,357,357,357,357,- 1,- 1,- 1,- 1,360,- 1,360,- 1,98,360,360,360,360,360,360,360,360,360,360,- 1,- 1,- 1,- 1,- 1,98,98,98,98,98,98,98,98,98,98,- 1,- 1,- 1,- 1,- 1,- 1,- 1,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,- 1,- 1,- 1,- 1,98,- 1,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,106,106,106,106,106,106,106,106,106,106,106,106,106,- 1,- 1,- 1,- 1,- 1,- 1,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,- 1,- 1,- 1,- 1,106,- 1,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,106,107,107,107,107,107,107,107,107,107,107,107,107,107,- 1,- 1,- 1,- 1,- 1,- 1,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,- 1,- 1,- 1,- 1,107,- 1,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,107,108,108,108,108,108,108,108,108,108,108,108,108,108,- 1,- 1,- 1,- 1,- 1,- 1,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,109,- 1,- 1,109,108,- 1,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,108,- 1,- 1,- 1,- 1,109,109,109,109,109,109,109,109,109,109,109,109,109,- 1,- 1,- 1,- 1,- 1,- 1,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,- 1,- 1,- 1,- 1,109,- 1,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,113,113,113,113,113,113,113,113,113,113,- 1,- 1,- 1,- 1,- 1,- 1,- 1,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,123,- 1,- 1,123,113,- 1,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,- 1,- 1,132,- 1,- 1,132,123,123,123,123,123,123,123,123,126,126,126,126,126,126,126,126,126,126,- 1,- 1,- 1,- 1,- 1,- 1,- 1,126,126,126,126,126,126,- 1,- 1,- 1,132,132,132,132,132,132,132,132,- 1,- 1,- 1,- 1,- 1,- 1,- 1,123,- 1,- 1,109,- 1,390,- 1,- 1,126,126,126,126,126,126,390,390,390,390,390,390,390,390,- 1,123,127,127,127,127,127,127,127,127,127,127,132,- 1,- 1,- 1,- 1,- 1,- 1,127,127,127,127,127,127,135,135,135,135,135,135,135,135,135,135,132,- 1,- 1,- 1,- 1,- 1,- 1,135,135,135,135,135,135,- 1,- 1,- 1,127,127,127,127,127,127,310,- 1,- 1,310,- 1,- 1,136,136,136,136,136,136,136,136,136,136,- 1,135,135,135,135,135,135,136,136,136,136,136,136,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,405,310,310,310,310,310,310,310,310,405,405,405,405,405,405,405,405,- 1,136,136,136,136,136,136,- 1,313,313,313,313,313,313,313,313,313,313,- 1,- 1,- 1,- 1,- 1,- 1,- 1,313,313,313,313,313,313,- 1,- 1,310,361,361,361,361,361,361,361,361,361,361,- 1,- 1,314,314,314,314,314,314,314,314,314,314,310,313,313,313,313,313,313,314,314,314,314,314,314,- 1,- 1,330,- 1,330,330,330,330,330,330,330,330,330,330,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,330,314,314,314,314,314,314,330,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,330,- 1,- 1,330,331,- 1,331,331,331,331,331,331,331,331,331,331,330,- 1,- 1,- 1,- 1,- 1,- 1,330,- 1,- 1,- 1,331,- 1,- 1,- 1,- 1,330,- 1,331,330,- 1,- 1,- 1,- 1,- 1,- 1,- 1,331,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,331,- 1,- 1,- 1,- 1,- 1,- 1,331,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,331,336,336,336,336,336,336,336,336,336,336,- 1,- 1,- 1,- 1,- 1,- 1,- 1,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,- 1,- 1,- 1,- 1,336,- 1,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,336,342,342,342,342,342,342,342,342,342,342,- 1,- 1,- 1,- 1,- 1,- 1,- 1,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,- 1,- 1,- 1,- 1,342,- 1,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,342,352,352,352,352,352,352,352,352,352,352,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,352,352,- 1,- 1,- 1,- 1,353,352,353,353,353,353,353,353,353,353,353,353,358,358,358,358,358,358,358,358,358,358,- 1,353,- 1,- 1,352,352,- 1,- 1,353,- 1,- 1,352,358,- 1,- 1,- 1,- 1,353,358,- 1,359,359,359,359,359,359,359,359,359,359,- 1,- 1,- 1,353,- 1,- 1,- 1,- 1,- 1,- 1,353,359,359,- 1,358,- 1,- 1,- 1,359,353,358,- 1,- 1,362,362,362,362,362,362,362,362,362,362,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,359,359,362,- 1,- 1,- 1,- 1,359,362,363,- 1,363,363,363,363,363,363,363,363,363,363,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,363,- 1,362,- 1,- 1,- 1,- 1,363,362,- 1,- 1,- 1,- 1,- 1,- 1,364,363,364,364,364,364,364,364,364,364,364,364,- 1,- 1,- 1,- 1,- 1,363,- 1,- 1,- 1,- 1,- 1,364,363,- 1,- 1,- 1,- 1,- 1,364,- 1,- 1,363,- 1,- 1,- 1,- 1,- 1,364,- 1,- 1,378,378,378,378,378,378,378,378,378,378,- 1,- 1,- 1,364,- 1,- 1,- 1,- 1,- 1,- 1,364,378,378,- 1,- 1,- 1,- 1,- 1,378,364,367,367,367,367,367,367,367,367,367,367,- 1,- 1,- 1,- 1,- 1,- 1,- 1,367,367,367,367,367,367,378,378,- 1,- 1,- 1,- 1,- 1,378,- 1,- 1,- 1,- 1,- 1,- 1,- 1,368,368,368,368,368,368,368,368,368,368,- 1,367,367,367,367,367,367,368,368,368,368,368,368,- 1,- 1,- 1,387,- 1,368,- 1,- 1,387,- 1,- 1,- 1,- 1,- 1,368,- 1,- 1,387,387,387,387,387,387,387,387,- 1,368,368,368,368,368,368,387,- 1,- 1,- 1,- 1,368,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,368,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,387,- 1,- 1,- 1,- 1,387,387,- 1,- 1,- 1,387,- 1,- 1,- 1,- 1,- 1,- 1,- 1,387,- 1,- 1,- 1,387,- 1,387,- 1,387,- 1,387,400,400,400,400,400,400,400,400,400,400,- 1,- 1,- 1,- 1,- 1,- 1,- 1,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,- 1,- 1,- 1,- 1,- 1,- 1,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,400,401,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,401,401,401,401,401,401,401,401,401,401,- 1,- 1,- 1,- 1,- 1,- 1,- 1,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,- 1,- 1,- 1,- 1,- 1,- 1,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,401,413,- 1,- 1,413,- 1,- 1,- 1,413,413,413,413,413,413,413,413,413,413,- 1,- 1,- 1,- 1,- 1,- 1,- 1,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,- 1,- 1,- 1,- 1,413,- 1,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,415,- 1,- 1,- 1,- 1,- 1,- 1,- 1,415,415,415,415,415,415,415,415,415,415,- 1,- 1,- 1,- 1,- 1,- 1,- 1,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,- 1,- 1,- 1,- 1,415,- 1,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,415,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1};
int Cyc_lex_engine(int start_state,struct Cyc_Lexing_lexbuf*lbuf){
# 172
int state;int base;int backtrk;
int c;
state=start_state;
# 176
if(state >= 0){
lbuf->lex_last_pos=(lbuf->lex_start_pos=lbuf->lex_curr_pos);
lbuf->lex_last_action=- 1;}else{
# 180
state=(- state)- 1;}
# 182
while(1){
base=Cyc_lex_base[_check_known_subscript_notnull(425,state)];
if(base < 0)return(- base)- 1;
backtrk=Cyc_lex_backtrk[_check_known_subscript_notnull(425,state)];
if(backtrk >= 0){
lbuf->lex_last_pos=lbuf->lex_curr_pos;
lbuf->lex_last_action=backtrk;}
# 190
if(lbuf->lex_curr_pos >= lbuf->lex_buffer_len){
if(!lbuf->lex_eof_reached)
return(- state)- 1;else{
# 194
c=256;}}else{
# 196
c=(int)*((char*)_check_dyneither_subscript(lbuf->lex_buffer,sizeof(char),lbuf->lex_curr_pos ++));
if(c == - 1)c=256;}
# 199
if(Cyc_lex_check[_check_known_subscript_notnull(3481,base + c)]== state)
state=Cyc_lex_trans[_check_known_subscript_notnull(3481,base + c)];else{
# 202
state=Cyc_lex_default[_check_known_subscript_notnull(425,state)];}
if(state < 0){
lbuf->lex_curr_pos=lbuf->lex_last_pos;
if(lbuf->lex_last_action == - 1){
struct Cyc_Lexing_Error_exn_struct _tmp408;const char*_tmp407;struct Cyc_Lexing_Error_exn_struct*_tmp406;(int)_throw((void*)((_tmp406=_cycalloc(sizeof(*_tmp406)),((_tmp406[0]=((_tmp408.tag=Cyc_Lexing_Error,((_tmp408.f1=((_tmp407="empty token",_tag_dyneither(_tmp407,sizeof(char),12))),_tmp408)))),_tmp406)))));}else{
# 208
return lbuf->lex_last_action;}}else{
# 211
if(c == 256)lbuf->lex_eof_reached=0;}}}
# 215
struct _tuple20*Cyc_line_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=Cyc_lex_engine(lexstate,lexbuf);
switch(lexstate){case 0: _LL0:
# 174 "buildlib.cyl"
 Cyc_macroname(lexbuf);
for(0;Cyc_current_args != 0;Cyc_current_args=((struct Cyc_List_List*)_check_null(Cyc_current_args))->tl){
struct Cyc_Set_Set**_tmp409;Cyc_current_targets=((_tmp409=_cycalloc(sizeof(*_tmp409)),((_tmp409[0]=((struct Cyc_Set_Set*(*)(struct Cyc_Set_Set*s,struct _dyneither_ptr*elt))Cyc_Set_delete)(*((struct Cyc_Set_Set**)_check_null(Cyc_current_targets)),(struct _dyneither_ptr*)((struct Cyc_List_List*)_check_null(Cyc_current_args))->hd),_tmp409))));}{
# 179
struct _tuple20*_tmp40A;return(_tmp40A=_cycalloc(sizeof(*_tmp40A)),((_tmp40A->f1=(struct _dyneither_ptr*)_check_null(Cyc_current_source),((_tmp40A->f2=*((struct Cyc_Set_Set**)_check_null(Cyc_current_targets)),_tmp40A)))));};case 1: _LL1:
# 182 "buildlib.cyl"
 return Cyc_line(lexbuf);case 2: _LL2:
# 184
 return 0;default: _LL3:
(lexbuf->refill_buff)(lexbuf);
return Cyc_line_rec(lexbuf,lexstate);}{
# 188
struct Cyc_Lexing_Error_exn_struct _tmp410;const char*_tmp40F;struct Cyc_Lexing_Error_exn_struct*_tmp40E;(int)_throw((void*)((_tmp40E=_cycalloc(sizeof(*_tmp40E)),((_tmp40E[0]=((_tmp410.tag=Cyc_Lexing_Error,((_tmp410.f1=((_tmp40F="some action didn't return!",_tag_dyneither(_tmp40F,sizeof(char),27))),_tmp410)))),_tmp40E)))));};}
# 190
struct _tuple20*Cyc_line(struct Cyc_Lexing_lexbuf*lexbuf){return Cyc_line_rec(lexbuf,0);}
int Cyc_macroname_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=Cyc_lex_engine(lexstate,lexbuf);
switch(lexstate){case 0: _LL5:
# 188 "buildlib.cyl"
{struct _dyneither_ptr*_tmp411;Cyc_current_source=((_tmp411=_cycalloc(sizeof(*_tmp411)),((_tmp411[0]=(struct _dyneither_ptr)Cyc_substring((struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf),0,(unsigned long)((
Cyc_Lexing_lexeme_end(lexbuf)- Cyc_Lexing_lexeme_start(lexbuf))- 2)),_tmp411))));}
Cyc_current_args=0;
{struct Cyc_Set_Set**_tmp412;Cyc_current_targets=((_tmp412=_cycalloc(sizeof(*_tmp412)),((_tmp412[0]=((struct Cyc_Set_Set*(*)(int(*cmp)(struct _dyneither_ptr*,struct _dyneither_ptr*)))Cyc_Set_empty)(Cyc_strptrcmp),_tmp412))));}
Cyc_token(lexbuf);
return 0;case 1: _LL6:
# 196
{struct _dyneither_ptr*_tmp413;Cyc_current_source=((_tmp413=_cycalloc(sizeof(*_tmp413)),((_tmp413[0]=(struct _dyneither_ptr)Cyc_substring((struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf),0,(unsigned long)((
Cyc_Lexing_lexeme_end(lexbuf)- Cyc_Lexing_lexeme_start(lexbuf))- 1)),_tmp413))));}
Cyc_current_args=0;
{struct Cyc_Set_Set**_tmp414;Cyc_current_targets=((_tmp414=_cycalloc(sizeof(*_tmp414)),((_tmp414[0]=((struct Cyc_Set_Set*(*)(int(*cmp)(struct _dyneither_ptr*,struct _dyneither_ptr*)))Cyc_Set_empty)(Cyc_strptrcmp),_tmp414))));}
Cyc_args(lexbuf);
return 0;case 2: _LL7:
# 204
{struct _dyneither_ptr*_tmp415;Cyc_current_source=((_tmp415=_cycalloc(sizeof(*_tmp415)),((_tmp415[0]=(struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf),_tmp415))));}
Cyc_current_args=0;
{struct Cyc_Set_Set**_tmp416;Cyc_current_targets=((_tmp416=_cycalloc(sizeof(*_tmp416)),((_tmp416[0]=((struct Cyc_Set_Set*(*)(int(*cmp)(struct _dyneither_ptr*,struct _dyneither_ptr*)))Cyc_Set_empty)(Cyc_strptrcmp),_tmp416))));}
Cyc_token(lexbuf);
return 0;default: _LL8:
# 210
(lexbuf->refill_buff)(lexbuf);
return Cyc_macroname_rec(lexbuf,lexstate);}{
# 213
struct Cyc_Lexing_Error_exn_struct _tmp41C;const char*_tmp41B;struct Cyc_Lexing_Error_exn_struct*_tmp41A;(int)_throw((void*)((_tmp41A=_cycalloc(sizeof(*_tmp41A)),((_tmp41A[0]=((_tmp41C.tag=Cyc_Lexing_Error,((_tmp41C.f1=((_tmp41B="some action didn't return!",_tag_dyneither(_tmp41B,sizeof(char),27))),_tmp41C)))),_tmp41A)))));};}
# 215
int Cyc_macroname(struct Cyc_Lexing_lexbuf*lexbuf){return Cyc_macroname_rec(lexbuf,1);}
int Cyc_args_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=Cyc_lex_engine(lexstate,lexbuf);
switch(lexstate){case 0: _LLA: {
# 213 "buildlib.cyl"
struct _dyneither_ptr*_tmp41D;struct _dyneither_ptr*_tmp15=(_tmp41D=_cycalloc(sizeof(*_tmp41D)),((_tmp41D[0]=(struct _dyneither_ptr)Cyc_substring((struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf),0,(unsigned long)((
Cyc_Lexing_lexeme_end(lexbuf)- Cyc_Lexing_lexeme_start(lexbuf))- 2)),_tmp41D)));
{struct Cyc_List_List*_tmp41E;Cyc_current_args=((_tmp41E=_cycalloc(sizeof(*_tmp41E)),((_tmp41E->hd=_tmp15,((_tmp41E->tl=Cyc_current_args,_tmp41E))))));}
return Cyc_args(lexbuf);}case 1: _LLB: {
# 219
struct _dyneither_ptr*_tmp41F;struct _dyneither_ptr*_tmp18=(_tmp41F=_cycalloc(sizeof(*_tmp41F)),((_tmp41F[0]=(struct _dyneither_ptr)Cyc_substring((struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf),0,(unsigned long)((
Cyc_Lexing_lexeme_end(lexbuf)- Cyc_Lexing_lexeme_start(lexbuf))- 1)),_tmp41F)));
{struct Cyc_List_List*_tmp420;Cyc_current_args=((_tmp420=_cycalloc(sizeof(*_tmp420)),((_tmp420->hd=_tmp18,((_tmp420->tl=Cyc_current_args,_tmp420))))));}
return Cyc_args(lexbuf);}case 2: _LLC: {
# 225
struct _dyneither_ptr*_tmp421;struct _dyneither_ptr*_tmp1B=(_tmp421=_cycalloc(sizeof(*_tmp421)),((_tmp421[0]=(struct _dyneither_ptr)Cyc_substring((struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf),0,(unsigned long)((
Cyc_Lexing_lexeme_end(lexbuf)- Cyc_Lexing_lexeme_start(lexbuf))- 1)),_tmp421)));
{struct Cyc_List_List*_tmp422;Cyc_current_args=((_tmp422=_cycalloc(sizeof(*_tmp422)),((_tmp422->hd=_tmp1B,((_tmp422->tl=Cyc_current_args,_tmp422))))));}
return Cyc_token(lexbuf);}default: _LLD:
# 230
(lexbuf->refill_buff)(lexbuf);
return Cyc_args_rec(lexbuf,lexstate);}{
# 233
struct Cyc_Lexing_Error_exn_struct _tmp428;const char*_tmp427;struct Cyc_Lexing_Error_exn_struct*_tmp426;(int)_throw((void*)((_tmp426=_cycalloc(sizeof(*_tmp426)),((_tmp426[0]=((_tmp428.tag=Cyc_Lexing_Error,((_tmp428.f1=((_tmp427="some action didn't return!",_tag_dyneither(_tmp427,sizeof(char),27))),_tmp428)))),_tmp426)))));};}
# 235
int Cyc_args(struct Cyc_Lexing_lexbuf*lexbuf){return Cyc_args_rec(lexbuf,2);}
int Cyc_token_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=Cyc_lex_engine(lexstate,lexbuf);
switch(lexstate){case 0: _LLF:
# 234 "buildlib.cyl"
{struct _dyneither_ptr*_tmp429;Cyc_add_target(((_tmp429=_cycalloc(sizeof(*_tmp429)),((_tmp429[0]=(struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf),_tmp429)))));}return Cyc_token(lexbuf);case 1: _LL10:
# 237 "buildlib.cyl"
 return 0;case 2: _LL11:
# 240 "buildlib.cyl"
 return Cyc_token(lexbuf);case 3: _LL12:
# 243 "buildlib.cyl"
 Cyc_string(lexbuf);return Cyc_token(lexbuf);case 4: _LL13:
# 246 "buildlib.cyl"
 return Cyc_token(lexbuf);case 5: _LL14:
# 248
 return Cyc_token(lexbuf);case 6: _LL15:
# 250
 return Cyc_token(lexbuf);case 7: _LL16:
# 252
 return Cyc_token(lexbuf);case 8: _LL17:
# 255 "buildlib.cyl"
 return Cyc_token(lexbuf);case 9: _LL18:
# 258 "buildlib.cyl"
 return Cyc_token(lexbuf);case 10: _LL19:
# 261 "buildlib.cyl"
 return Cyc_token(lexbuf);case 11: _LL1A:
# 263
 return Cyc_token(lexbuf);case 12: _LL1B:
# 265
 return Cyc_token(lexbuf);case 13: _LL1C:
# 267
 return Cyc_token(lexbuf);case 14: _LL1D:
# 269
 return Cyc_token(lexbuf);case 15: _LL1E:
# 271
 return Cyc_token(lexbuf);case 16: _LL1F:
# 273
 return Cyc_token(lexbuf);case 17: _LL20:
# 275
 return Cyc_token(lexbuf);case 18: _LL21:
# 277
 return Cyc_token(lexbuf);case 19: _LL22:
# 279
 return Cyc_token(lexbuf);case 20: _LL23:
# 281
 return Cyc_token(lexbuf);case 21: _LL24:
# 283
 return Cyc_token(lexbuf);case 22: _LL25:
# 285
 return Cyc_token(lexbuf);case 23: _LL26:
# 287
 return Cyc_token(lexbuf);case 24: _LL27:
# 290 "buildlib.cyl"
 return Cyc_token(lexbuf);case 25: _LL28:
# 292
 return Cyc_token(lexbuf);case 26: _LL29:
# 294
 return Cyc_token(lexbuf);case 27: _LL2A:
# 296
 return Cyc_token(lexbuf);case 28: _LL2B:
# 298
 return Cyc_token(lexbuf);case 29: _LL2C:
# 300
 return Cyc_token(lexbuf);case 30: _LL2D:
# 302
 return Cyc_token(lexbuf);case 31: _LL2E:
# 304
 return Cyc_token(lexbuf);case 32: _LL2F:
# 306
 return Cyc_token(lexbuf);case 33: _LL30:
# 308
 return Cyc_token(lexbuf);case 34: _LL31:
# 310
 return Cyc_token(lexbuf);case 35: _LL32:
# 312
 return Cyc_token(lexbuf);case 36: _LL33:
# 314
 return Cyc_token(lexbuf);case 37: _LL34:
# 316
 return Cyc_token(lexbuf);case 38: _LL35:
# 318
 return Cyc_token(lexbuf);case 39: _LL36:
# 320
 return Cyc_token(lexbuf);case 40: _LL37:
# 322
 return Cyc_token(lexbuf);case 41: _LL38:
# 324
 return Cyc_token(lexbuf);case 42: _LL39:
# 326
 return Cyc_token(lexbuf);case 43: _LL3A:
# 328
 return Cyc_token(lexbuf);case 44: _LL3B:
# 330
 return Cyc_token(lexbuf);case 45: _LL3C:
# 332
 return Cyc_token(lexbuf);case 46: _LL3D:
# 334
 return Cyc_token(lexbuf);case 47: _LL3E:
# 336
 return Cyc_token(lexbuf);case 48: _LL3F:
# 339 "buildlib.cyl"
 return Cyc_token(lexbuf);default: _LL40:
(lexbuf->refill_buff)(lexbuf);
return Cyc_token_rec(lexbuf,lexstate);}{
# 343
struct Cyc_Lexing_Error_exn_struct _tmp42F;const char*_tmp42E;struct Cyc_Lexing_Error_exn_struct*_tmp42D;(int)_throw((void*)((_tmp42D=_cycalloc(sizeof(*_tmp42D)),((_tmp42D[0]=((_tmp42F.tag=Cyc_Lexing_Error,((_tmp42F.f1=((_tmp42E="some action didn't return!",_tag_dyneither(_tmp42E,sizeof(char),27))),_tmp42F)))),_tmp42D)))));};}
# 345
int Cyc_token(struct Cyc_Lexing_lexbuf*lexbuf){return Cyc_token_rec(lexbuf,3);}
int Cyc_string_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=Cyc_lex_engine(lexstate,lexbuf);
switch(lexstate){case 0: _LL42:
# 344 "buildlib.cyl"
 return Cyc_string(lexbuf);case 1: _LL43:
# 345 "buildlib.cyl"
 return 0;case 2: _LL44:
# 346 "buildlib.cyl"
 return Cyc_string(lexbuf);case 3: _LL45:
# 347 "buildlib.cyl"
 return Cyc_string(lexbuf);case 4: _LL46:
# 350 "buildlib.cyl"
 return Cyc_string(lexbuf);case 5: _LL47:
# 353 "buildlib.cyl"
 return Cyc_string(lexbuf);case 6: _LL48:
# 355
 return Cyc_string(lexbuf);case 7: _LL49:
# 356 "buildlib.cyl"
 return 0;case 8: _LL4A:
# 357 "buildlib.cyl"
 return 0;case 9: _LL4B:
# 358 "buildlib.cyl"
 return Cyc_string(lexbuf);default: _LL4C:
(lexbuf->refill_buff)(lexbuf);
return Cyc_string_rec(lexbuf,lexstate);}{
# 362
struct Cyc_Lexing_Error_exn_struct _tmp435;const char*_tmp434;struct Cyc_Lexing_Error_exn_struct*_tmp433;(int)_throw((void*)((_tmp433=_cycalloc(sizeof(*_tmp433)),((_tmp433[0]=((_tmp435.tag=Cyc_Lexing_Error,((_tmp435.f1=((_tmp434="some action didn't return!",_tag_dyneither(_tmp434,sizeof(char),27))),_tmp435)))),_tmp433)))));};}
# 364
int Cyc_string(struct Cyc_Lexing_lexbuf*lexbuf){return Cyc_string_rec(lexbuf,4);}
int Cyc_slurp_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=Cyc_lex_engine(lexstate,lexbuf);
switch(lexstate){case 0: _LL4E:
# 367 "buildlib.cyl"
 return 0;case 1: _LL4F:
# 369
 Cyc_fputc((int)'"',(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));
while(Cyc_slurp_string(lexbuf)){;}
return 1;case 2: _LL50:
# 376 "buildlib.cyl"
 Cyc_fputs("*__IGNORE_FOR_CYCLONE_MALLOC(",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));
{const char*_tmp438;void*_tmp437;(_tmp437=0,Cyc_log(((_tmp438="Warning: declaration of malloc sidestepped\n",_tag_dyneither(_tmp438,sizeof(char),44))),_tag_dyneither(_tmp437,sizeof(void*),0)));}
return 1;case 3: _LL51:
# 382 "buildlib.cyl"
 Cyc_fputs(" __IGNORE_FOR_CYCLONE_MALLOC(",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));
{const char*_tmp43B;void*_tmp43A;(_tmp43A=0,Cyc_log(((_tmp43B="Warning: declaration of malloc sidestepped\n",_tag_dyneither(_tmp43B,sizeof(char),44))),_tag_dyneither(_tmp43A,sizeof(void*),0)));}
return 1;case 4: _LL52:
# 388 "buildlib.cyl"
 Cyc_fputs("*__IGNORE_FOR_CYCLONE_CALLOC(",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));
{const char*_tmp43E;void*_tmp43D;(_tmp43D=0,Cyc_log(((_tmp43E="Warning: declaration of calloc sidestepped\n",_tag_dyneither(_tmp43E,sizeof(char),44))),_tag_dyneither(_tmp43D,sizeof(void*),0)));}
return 1;case 5: _LL53:
# 394 "buildlib.cyl"
 Cyc_fputs(" __IGNORE_FOR_CYCLONE_CALLOC(",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));
{const char*_tmp441;void*_tmp440;(_tmp440=0,Cyc_log(((_tmp441="Warning: declaration of calloc sidestepped\n",_tag_dyneither(_tmp441,sizeof(char),44))),_tag_dyneither(_tmp440,sizeof(void*),0)));}
return 1;case 6: _LL54:
# 398
 Cyc_fputs("__region",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));
{const char*_tmp444;void*_tmp443;(_tmp443=0,Cyc_log(((_tmp444="Warning: use of region sidestepped\n",_tag_dyneither(_tmp444,sizeof(char),36))),_tag_dyneither(_tmp443,sizeof(void*),0)));}
return 1;case 7: _LL55:
# 402
{const char*_tmp447;void*_tmp446;(_tmp446=0,Cyc_log(((_tmp447="Warning: use of __extension__ deleted\n",_tag_dyneither(_tmp447,sizeof(char),39))),_tag_dyneither(_tmp446,sizeof(void*),0)));}
return 1;case 8: _LL56:
# 407 "buildlib.cyl"
{const char*_tmp44A;void*_tmp449;(_tmp449=0,Cyc_log(((_tmp44A="Warning: use of mode HI deleted\n",_tag_dyneither(_tmp44A,sizeof(char),33))),_tag_dyneither(_tmp449,sizeof(void*),0)));}
return 1;case 9: _LL57:
# 410
{const char*_tmp44D;void*_tmp44C;(_tmp44C=0,Cyc_log(((_tmp44D="Warning: use of mode SI deleted\n",_tag_dyneither(_tmp44D,sizeof(char),33))),_tag_dyneither(_tmp44C,sizeof(void*),0)));}
return 1;case 10: _LL58:
# 413
{const char*_tmp450;void*_tmp44F;(_tmp44F=0,Cyc_log(((_tmp450="Warning: use of mode QI deleted\n",_tag_dyneither(_tmp450,sizeof(char),33))),_tag_dyneither(_tmp44F,sizeof(void*),0)));}
return 1;case 11: _LL59:
# 416
{const char*_tmp453;void*_tmp452;(_tmp452=0,Cyc_log(((_tmp453="Warning: use of mode DI deleted\n",_tag_dyneither(_tmp453,sizeof(char),33))),_tag_dyneither(_tmp452,sizeof(void*),0)));}
return 1;case 12: _LL5A:
# 419
{const char*_tmp456;void*_tmp455;(_tmp455=0,Cyc_log(((_tmp456="Warning: use of mode DI deleted\n",_tag_dyneither(_tmp456,sizeof(char),33))),_tag_dyneither(_tmp455,sizeof(void*),0)));}
return 1;case 13: _LL5B:
# 422
{const char*_tmp459;void*_tmp458;(_tmp458=0,Cyc_log(((_tmp459="Warning: use of mode word deleted\n",_tag_dyneither(_tmp459,sizeof(char),35))),_tag_dyneither(_tmp458,sizeof(void*),0)));}
return 1;case 14: _LL5C:
# 425
 Cyc_fputs("inline",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));return 1;case 15: _LL5D:
# 427
 Cyc_fputs("inline",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));return 1;case 16: _LL5E:
# 429
 Cyc_fputs("const",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));return 1;case 17: _LL5F:
# 431
 Cyc_fputs("const",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));return 1;case 18: _LL60:
# 436 "buildlib.cyl"
 Cyc_fputs("int",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));return 1;case 19: _LL61:
# 438
 return 1;case 20: _LL62:
# 440
 Cyc_parens_to_match=1;
while(Cyc_asmtok(lexbuf)){;}
Cyc_fputs("0",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));
{const char*_tmp45C;void*_tmp45B;(_tmp45B=0,Cyc_log(((_tmp45C="Warning: replacing use of __asm__ with 0\n",_tag_dyneither(_tmp45C,sizeof(char),42))),_tag_dyneither(_tmp45B,sizeof(void*),0)));}
return 1;case 21: _LL63:
# 446
 Cyc_fputc((int)Cyc_Lexing_lexeme_char(lexbuf,0),(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));return 1;default: _LL64:
(lexbuf->refill_buff)(lexbuf);
return Cyc_slurp_rec(lexbuf,lexstate);}{
# 450
struct Cyc_Lexing_Error_exn_struct _tmp462;const char*_tmp461;struct Cyc_Lexing_Error_exn_struct*_tmp460;(int)_throw((void*)((_tmp460=_cycalloc(sizeof(*_tmp460)),((_tmp460[0]=((_tmp462.tag=Cyc_Lexing_Error,((_tmp462.f1=((_tmp461="some action didn't return!",_tag_dyneither(_tmp461,sizeof(char),27))),_tmp462)))),_tmp460)))));};}
# 452
int Cyc_slurp(struct Cyc_Lexing_lexbuf*lexbuf){return Cyc_slurp_rec(lexbuf,5);}
int Cyc_slurp_string_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=Cyc_lex_engine(lexstate,lexbuf);
switch(lexstate){case 0: _LL66:
# 450 "buildlib.cyl"
 return 0;case 1: _LL67:
# 452
 Cyc_fputc((int)'"',(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));return 0;case 2: _LL68:
# 454
{const char*_tmp465;void*_tmp464;(_tmp464=0,Cyc_log(((_tmp465="Warning: unclosed string\n",_tag_dyneither(_tmp465,sizeof(char),26))),_tag_dyneither(_tmp464,sizeof(void*),0)));}
{const char*_tmp469;void*_tmp468[1];struct Cyc_String_pa_PrintArg_struct _tmp467;(_tmp467.tag=0,((_tmp467.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf)),((_tmp468[0]=& _tmp467,Cyc_fprintf((struct Cyc___cycFILE*)_check_null(Cyc_slurp_out),((_tmp469="%s",_tag_dyneither(_tmp469,sizeof(char),3))),_tag_dyneither(_tmp468,sizeof(void*),1)))))));}return 1;case 3: _LL69:
# 457
{const char*_tmp46D;void*_tmp46C[1];struct Cyc_String_pa_PrintArg_struct _tmp46B;(_tmp46B.tag=0,((_tmp46B.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf)),((_tmp46C[0]=& _tmp46B,Cyc_fprintf((struct Cyc___cycFILE*)_check_null(Cyc_slurp_out),((_tmp46D="%s",_tag_dyneither(_tmp46D,sizeof(char),3))),_tag_dyneither(_tmp46C,sizeof(void*),1)))))));}return 1;case 4: _LL6A:
# 459
{const char*_tmp471;void*_tmp470[1];struct Cyc_String_pa_PrintArg_struct _tmp46F;(_tmp46F.tag=0,((_tmp46F.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf)),((_tmp470[0]=& _tmp46F,Cyc_fprintf((struct Cyc___cycFILE*)_check_null(Cyc_slurp_out),((_tmp471="%s",_tag_dyneither(_tmp471,sizeof(char),3))),_tag_dyneither(_tmp470,sizeof(void*),1)))))));}return 1;case 5: _LL6B:
# 461
{const char*_tmp475;void*_tmp474[1];struct Cyc_String_pa_PrintArg_struct _tmp473;(_tmp473.tag=0,((_tmp473.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf)),((_tmp474[0]=& _tmp473,Cyc_fprintf((struct Cyc___cycFILE*)_check_null(Cyc_slurp_out),((_tmp475="%s",_tag_dyneither(_tmp475,sizeof(char),3))),_tag_dyneither(_tmp474,sizeof(void*),1)))))));}return 1;case 6: _LL6C:
# 463
{const char*_tmp479;void*_tmp478[1];struct Cyc_String_pa_PrintArg_struct _tmp477;(_tmp477.tag=0,((_tmp477.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf)),((_tmp478[0]=& _tmp477,Cyc_fprintf((struct Cyc___cycFILE*)_check_null(Cyc_slurp_out),((_tmp479="%s",_tag_dyneither(_tmp479,sizeof(char),3))),_tag_dyneither(_tmp478,sizeof(void*),1)))))));}return 1;case 7: _LL6D:
# 465
{const char*_tmp47D;void*_tmp47C[1];struct Cyc_String_pa_PrintArg_struct _tmp47B;(_tmp47B.tag=0,((_tmp47B.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf)),((_tmp47C[0]=& _tmp47B,Cyc_fprintf((struct Cyc___cycFILE*)_check_null(Cyc_slurp_out),((_tmp47D="%s",_tag_dyneither(_tmp47D,sizeof(char),3))),_tag_dyneither(_tmp47C,sizeof(void*),1)))))));}return 1;case 8: _LL6E:
# 467
{const char*_tmp481;void*_tmp480[1];struct Cyc_String_pa_PrintArg_struct _tmp47F;(_tmp47F.tag=0,((_tmp47F.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf)),((_tmp480[0]=& _tmp47F,Cyc_fprintf((struct Cyc___cycFILE*)_check_null(Cyc_slurp_out),((_tmp481="%s",_tag_dyneither(_tmp481,sizeof(char),3))),_tag_dyneither(_tmp480,sizeof(void*),1)))))));}return 1;default: _LL6F:
(lexbuf->refill_buff)(lexbuf);
return Cyc_slurp_string_rec(lexbuf,lexstate);}{
# 471
struct Cyc_Lexing_Error_exn_struct _tmp487;const char*_tmp486;struct Cyc_Lexing_Error_exn_struct*_tmp485;(int)_throw((void*)((_tmp485=_cycalloc(sizeof(*_tmp485)),((_tmp485[0]=((_tmp487.tag=Cyc_Lexing_Error,((_tmp487.f1=((_tmp486="some action didn't return!",_tag_dyneither(_tmp486,sizeof(char),27))),_tmp487)))),_tmp485)))));};}
# 473
int Cyc_slurp_string(struct Cyc_Lexing_lexbuf*lexbuf){return Cyc_slurp_string_rec(lexbuf,6);}
int Cyc_asmtok_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=Cyc_lex_engine(lexstate,lexbuf);
switch(lexstate){case 0: _LL71:
# 477 "buildlib.cyl"
 return 0;case 1: _LL72:
# 479
 if(Cyc_parens_to_match == 1)return 0;
-- Cyc_parens_to_match;
return 1;case 2: _LL73:
# 483
 ++ Cyc_parens_to_match;
return 1;case 3: _LL74:
# 486
 while(Cyc_asm_string(lexbuf)){;}
return 1;case 4: _LL75:
# 489
 while(Cyc_asm_comment(lexbuf)){;}
return 1;case 5: _LL76:
# 492
 return 1;case 6: _LL77:
# 494
 return 1;default: _LL78:
(lexbuf->refill_buff)(lexbuf);
return Cyc_asmtok_rec(lexbuf,lexstate);}{
# 498
struct Cyc_Lexing_Error_exn_struct _tmp48D;const char*_tmp48C;struct Cyc_Lexing_Error_exn_struct*_tmp48B;(int)_throw((void*)((_tmp48B=_cycalloc(sizeof(*_tmp48B)),((_tmp48B[0]=((_tmp48D.tag=Cyc_Lexing_Error,((_tmp48D.f1=((_tmp48C="some action didn't return!",_tag_dyneither(_tmp48C,sizeof(char),27))),_tmp48D)))),_tmp48B)))));};}
# 500
int Cyc_asmtok(struct Cyc_Lexing_lexbuf*lexbuf){return Cyc_asmtok_rec(lexbuf,7);}
int Cyc_asm_string_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=Cyc_lex_engine(lexstate,lexbuf);
switch(lexstate){case 0: _LL7A:
# 498 "buildlib.cyl"
{const char*_tmp490;void*_tmp48F;(_tmp48F=0,Cyc_log(((_tmp490="Warning: unclosed string\n",_tag_dyneither(_tmp490,sizeof(char),26))),_tag_dyneither(_tmp48F,sizeof(void*),0)));}return 0;case 1: _LL7B:
# 500
 return 0;case 2: _LL7C:
# 502
{const char*_tmp493;void*_tmp492;(_tmp492=0,Cyc_log(((_tmp493="Warning: unclosed string\n",_tag_dyneither(_tmp493,sizeof(char),26))),_tag_dyneither(_tmp492,sizeof(void*),0)));}return 1;case 3: _LL7D:
# 504
 return 1;case 4: _LL7E:
# 506
 return 1;case 5: _LL7F:
# 508
 return 1;case 6: _LL80:
# 510
 return 1;case 7: _LL81:
# 512
 return 1;case 8: _LL82:
# 514
 return 1;default: _LL83:
(lexbuf->refill_buff)(lexbuf);
return Cyc_asm_string_rec(lexbuf,lexstate);}{
# 518
struct Cyc_Lexing_Error_exn_struct _tmp499;const char*_tmp498;struct Cyc_Lexing_Error_exn_struct*_tmp497;(int)_throw((void*)((_tmp497=_cycalloc(sizeof(*_tmp497)),((_tmp497[0]=((_tmp499.tag=Cyc_Lexing_Error,((_tmp499.f1=((_tmp498="some action didn't return!",_tag_dyneither(_tmp498,sizeof(char),27))),_tmp499)))),_tmp497)))));};}
# 520
int Cyc_asm_string(struct Cyc_Lexing_lexbuf*lexbuf){return Cyc_asm_string_rec(lexbuf,8);}
int Cyc_asm_comment_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=Cyc_lex_engine(lexstate,lexbuf);
switch(lexstate){case 0: _LL85:
# 518 "buildlib.cyl"
{const char*_tmp49C;void*_tmp49B;(_tmp49B=0,Cyc_log(((_tmp49C="Warning: unclosed comment\n",_tag_dyneither(_tmp49C,sizeof(char),27))),_tag_dyneither(_tmp49B,sizeof(void*),0)));}return 0;case 1: _LL86:
# 520
 return 0;case 2: _LL87:
# 522
 return 1;default: _LL88:
(lexbuf->refill_buff)(lexbuf);
return Cyc_asm_comment_rec(lexbuf,lexstate);}{
# 526
struct Cyc_Lexing_Error_exn_struct _tmp4A2;const char*_tmp4A1;struct Cyc_Lexing_Error_exn_struct*_tmp4A0;(int)_throw((void*)((_tmp4A0=_cycalloc(sizeof(*_tmp4A0)),((_tmp4A0[0]=((_tmp4A2.tag=Cyc_Lexing_Error,((_tmp4A2.f1=((_tmp4A1="some action didn't return!",_tag_dyneither(_tmp4A1,sizeof(char),27))),_tmp4A2)))),_tmp4A0)))));};}
# 528
int Cyc_asm_comment(struct Cyc_Lexing_lexbuf*lexbuf){return Cyc_asm_comment_rec(lexbuf,9);}
struct _tuple21*Cyc_suck_line_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=Cyc_lex_engine(lexstate,lexbuf);
switch(lexstate){case 0: _LL8A:
# 530 "buildlib.cyl"
{const char*_tmp4A3;Cyc_current_line=((_tmp4A3="#define ",_tag_dyneither(_tmp4A3,sizeof(char),9)));}
Cyc_suck_macroname(lexbuf);{
struct _tuple21*_tmp4A4;return(_tmp4A4=_cycalloc(sizeof(*_tmp4A4)),((_tmp4A4->f1=Cyc_current_line,((_tmp4A4->f2=(struct _dyneither_ptr*)_check_null(Cyc_current_source),_tmp4A4)))));};case 1: _LL8B:
# 534
 return Cyc_suck_line(lexbuf);case 2: _LL8C:
# 536
 return 0;default: _LL8D:
(lexbuf->refill_buff)(lexbuf);
return Cyc_suck_line_rec(lexbuf,lexstate);}{
# 540
struct Cyc_Lexing_Error_exn_struct _tmp4AA;const char*_tmp4A9;struct Cyc_Lexing_Error_exn_struct*_tmp4A8;(int)_throw((void*)((_tmp4A8=_cycalloc(sizeof(*_tmp4A8)),((_tmp4A8[0]=((_tmp4AA.tag=Cyc_Lexing_Error,((_tmp4AA.f1=((_tmp4A9="some action didn't return!",_tag_dyneither(_tmp4A9,sizeof(char),27))),_tmp4AA)))),_tmp4A8)))));};}
# 542
struct _tuple21*Cyc_suck_line(struct Cyc_Lexing_lexbuf*lexbuf){return Cyc_suck_line_rec(lexbuf,10);}
int Cyc_suck_macroname_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=Cyc_lex_engine(lexstate,lexbuf);
switch(lexstate){case 0: _LL8F:
# 540 "buildlib.cyl"
{struct _dyneither_ptr*_tmp4AB;Cyc_current_source=((_tmp4AB=_cycalloc(sizeof(*_tmp4AB)),((_tmp4AB[0]=(struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf),_tmp4AB))));}
Cyc_current_line=(struct _dyneither_ptr)Cyc_strconcat((struct _dyneither_ptr)Cyc_current_line,(struct _dyneither_ptr)*((struct _dyneither_ptr*)_check_null(Cyc_current_source)));
return Cyc_suck_restofline(lexbuf);default: _LL90:
# 544
(lexbuf->refill_buff)(lexbuf);
return Cyc_suck_macroname_rec(lexbuf,lexstate);}{
# 547
struct Cyc_Lexing_Error_exn_struct _tmp4B1;const char*_tmp4B0;struct Cyc_Lexing_Error_exn_struct*_tmp4AF;(int)_throw((void*)((_tmp4AF=_cycalloc(sizeof(*_tmp4AF)),((_tmp4AF[0]=((_tmp4B1.tag=Cyc_Lexing_Error,((_tmp4B1.f1=((_tmp4B0="some action didn't return!",_tag_dyneither(_tmp4B0,sizeof(char),27))),_tmp4B1)))),_tmp4AF)))));};}
# 549
int Cyc_suck_macroname(struct Cyc_Lexing_lexbuf*lexbuf){return Cyc_suck_macroname_rec(lexbuf,11);}
int Cyc_suck_restofline_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=Cyc_lex_engine(lexstate,lexbuf);
switch(lexstate){case 0: _LL92:
# 547 "buildlib.cyl"
 Cyc_current_line=(struct _dyneither_ptr)Cyc_strconcat((struct _dyneither_ptr)Cyc_current_line,(struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf));return 0;default: _LL93:
(lexbuf->refill_buff)(lexbuf);
return Cyc_suck_restofline_rec(lexbuf,lexstate);}{
# 551
struct Cyc_Lexing_Error_exn_struct _tmp4B7;const char*_tmp4B6;struct Cyc_Lexing_Error_exn_struct*_tmp4B5;(int)_throw((void*)((_tmp4B5=_cycalloc(sizeof(*_tmp4B5)),((_tmp4B5[0]=((_tmp4B7.tag=Cyc_Lexing_Error,((_tmp4B7.f1=((_tmp4B6="some action didn't return!",_tag_dyneither(_tmp4B6,sizeof(char),27))),_tmp4B7)))),_tmp4B5)))));};}
# 553
int Cyc_suck_restofline(struct Cyc_Lexing_lexbuf*lexbuf){return Cyc_suck_restofline_rec(lexbuf,12);}
struct _tuple23*Cyc_spec_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=Cyc_lex_engine(lexstate,lexbuf);
switch(lexstate){case 0: _LL95:
# 554 "buildlib.cyl"
 return Cyc_spec(lexbuf);case 1: _LL96:
# 556
 Cyc_current_headerfile=(struct _dyneither_ptr)
Cyc_substring((struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf),0,(unsigned long)((
Cyc_Lexing_lexeme_end(lexbuf)- Cyc_Lexing_lexeme_start(lexbuf))- 1));
Cyc_current_symbols=0;
Cyc_current_omit_symbols=0;
Cyc_current_cstubs=0;
Cyc_current_cycstubs=0;
Cyc_current_hstubs=0;
while(Cyc_commands(lexbuf)){;}
Cyc_current_hstubs=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_current_hstubs);
Cyc_current_cstubs=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_current_cstubs);
Cyc_current_cycstubs=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_current_cycstubs);{
struct _tuple23*_tmp4B8;return(_tmp4B8=_cycalloc(sizeof(*_tmp4B8)),((_tmp4B8->f1=Cyc_current_headerfile,((_tmp4B8->f2=Cyc_current_symbols,((_tmp4B8->f3=Cyc_current_omit_symbols,((_tmp4B8->f4=Cyc_current_hstubs,((_tmp4B8->f5=Cyc_current_cstubs,((_tmp4B8->f6=Cyc_current_cycstubs,_tmp4B8)))))))))))));};case 2: _LL97:
# 576
 return Cyc_spec(lexbuf);case 3: _LL98:
# 578
 return 0;case 4: _LL99:
# 580
{const char*_tmp4BC;void*_tmp4BB[1];struct Cyc_Int_pa_PrintArg_struct _tmp4BA;(_tmp4BA.tag=1,((_tmp4BA.f1=(unsigned long)((int)
# 582
Cyc_Lexing_lexeme_char(lexbuf,0)),((_tmp4BB[0]=& _tmp4BA,Cyc_fprintf(Cyc_stderr,((_tmp4BC="Error in .cys file: expected header file name, found '%c' instead\n",_tag_dyneither(_tmp4BC,sizeof(char),67))),_tag_dyneither(_tmp4BB,sizeof(void*),1)))))));}
return 0;default: _LL9A:
(lexbuf->refill_buff)(lexbuf);
return Cyc_spec_rec(lexbuf,lexstate);}{
# 587
struct Cyc_Lexing_Error_exn_struct _tmp4C2;const char*_tmp4C1;struct Cyc_Lexing_Error_exn_struct*_tmp4C0;(int)_throw((void*)((_tmp4C0=_cycalloc(sizeof(*_tmp4C0)),((_tmp4C0[0]=((_tmp4C2.tag=Cyc_Lexing_Error,((_tmp4C2.f1=((_tmp4C1="some action didn't return!",_tag_dyneither(_tmp4C1,sizeof(char),27))),_tmp4C2)))),_tmp4C0)))));};}
# 589
struct _tuple23*Cyc_spec(struct Cyc_Lexing_lexbuf*lexbuf){return Cyc_spec_rec(lexbuf,13);}
int Cyc_commands_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=Cyc_lex_engine(lexstate,lexbuf);
switch(lexstate){case 0: _LL9C:
# 587 "buildlib.cyl"
 return 0;case 1: _LL9D:
# 589
 return 0;case 2: _LL9E:
# 591
 Cyc_snarfed_symbols=0;
while(Cyc_snarfsymbols(lexbuf)){;}
Cyc_current_symbols=((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_append)(Cyc_snarfed_symbols,Cyc_current_symbols);
return 1;case 3: _LL9F:
# 596
 Cyc_snarfed_symbols=0;
while(Cyc_snarfsymbols(lexbuf)){;}
Cyc_current_omit_symbols=((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_append)(Cyc_snarfed_symbols,Cyc_current_omit_symbols);
return 1;case 4: _LLA0:
# 601
 Cyc_braces_to_match=1;
Cyc_specbuf=Cyc_Buffer_create(255);
while(Cyc_block(lexbuf)){;}{
struct _tuple22*_tmp4C3;struct _tuple22*x=(_tmp4C3=_cycalloc(sizeof(*_tmp4C3)),((_tmp4C3->f1=(struct _dyneither_ptr)_tag_dyneither(0,0,0),((_tmp4C3->f2=(struct _dyneither_ptr)
Cyc_Buffer_contents((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf)),_tmp4C3)))));
{struct Cyc_List_List*_tmp4C4;Cyc_current_hstubs=((_tmp4C4=_cycalloc(sizeof(*_tmp4C4)),((_tmp4C4->hd=x,((_tmp4C4->tl=Cyc_current_hstubs,_tmp4C4))))));}
return 1;};case 5: _LLA1: {
# 609
struct _dyneither_ptr _tmp83=Cyc_Lexing_lexeme(lexbuf);
{const char*_tmp4C5;_dyneither_ptr_inplace_plus(& _tmp83,sizeof(char),(int)Cyc_strlen(((_tmp4C5="hstub",_tag_dyneither(_tmp4C5,sizeof(char),6)))));}
while(isspace((int)*((char*)_check_dyneither_subscript(_tmp83,sizeof(char),0)))){_dyneither_ptr_inplace_plus(& _tmp83,sizeof(char),1);}{
struct _dyneither_ptr t=_tmp83;
while(!isspace((int)*((char*)_check_dyneither_subscript(t,sizeof(char),0)))){_dyneither_ptr_inplace_plus(& t,sizeof(char),1);}{
struct _dyneither_ptr _tmp85=Cyc_substring((struct _dyneither_ptr)_tmp83,0,(unsigned long)((t.curr - _tmp83.curr)/ sizeof(char)));
Cyc_braces_to_match=1;
Cyc_specbuf=Cyc_Buffer_create(255);
while(Cyc_block(lexbuf)){;}{
struct _tuple22*_tmp4C6;struct _tuple22*x=(_tmp4C6=_cycalloc(sizeof(*_tmp4C6)),((_tmp4C6->f1=(struct _dyneither_ptr)_tmp85,((_tmp4C6->f2=(struct _dyneither_ptr)
Cyc_Buffer_contents((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf)),_tmp4C6)))));
{struct Cyc_List_List*_tmp4C7;Cyc_current_hstubs=((_tmp4C7=_cycalloc(sizeof(*_tmp4C7)),((_tmp4C7->hd=x,((_tmp4C7->tl=Cyc_current_hstubs,_tmp4C7))))));}
return 1;};};};}case 6: _LLA2:
# 623
 Cyc_braces_to_match=1;
Cyc_specbuf=Cyc_Buffer_create(255);
while(Cyc_block(lexbuf)){;}{
struct _tuple22*_tmp4C8;struct _tuple22*x=(_tmp4C8=_cycalloc(sizeof(*_tmp4C8)),((_tmp4C8->f1=(struct _dyneither_ptr)_tag_dyneither(0,0,0),((_tmp4C8->f2=(struct _dyneither_ptr)
Cyc_Buffer_contents((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf)),_tmp4C8)))));
{struct Cyc_List_List*_tmp4C9;Cyc_current_cstubs=((_tmp4C9=_cycalloc(sizeof(*_tmp4C9)),((_tmp4C9->hd=x,((_tmp4C9->tl=Cyc_current_cstubs,_tmp4C9))))));}
return 1;};case 7: _LLA3: {
# 631
struct _dyneither_ptr _tmp8A=Cyc_Lexing_lexeme(lexbuf);
{const char*_tmp4CA;_dyneither_ptr_inplace_plus(& _tmp8A,sizeof(char),(int)Cyc_strlen(((_tmp4CA="cstub",_tag_dyneither(_tmp4CA,sizeof(char),6)))));}
while(isspace((int)*((char*)_check_dyneither_subscript(_tmp8A,sizeof(char),0)))){_dyneither_ptr_inplace_plus(& _tmp8A,sizeof(char),1);}{
struct _dyneither_ptr t=_tmp8A;
while(!isspace((int)*((char*)_check_dyneither_subscript(t,sizeof(char),0)))){_dyneither_ptr_inplace_plus(& t,sizeof(char),1);}{
struct _dyneither_ptr _tmp8C=Cyc_substring((struct _dyneither_ptr)_tmp8A,0,(unsigned long)((t.curr - _tmp8A.curr)/ sizeof(char)));
Cyc_braces_to_match=1;
Cyc_specbuf=Cyc_Buffer_create(255);
while(Cyc_block(lexbuf)){;}{
struct _tuple22*_tmp4CB;struct _tuple22*x=(_tmp4CB=_cycalloc(sizeof(*_tmp4CB)),((_tmp4CB->f1=(struct _dyneither_ptr)_tmp8C,((_tmp4CB->f2=(struct _dyneither_ptr)
Cyc_Buffer_contents((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf)),_tmp4CB)))));
{struct Cyc_List_List*_tmp4CC;Cyc_current_cstubs=((_tmp4CC=_cycalloc(sizeof(*_tmp4CC)),((_tmp4CC->hd=x,((_tmp4CC->tl=Cyc_current_cstubs,_tmp4CC))))));}
return 1;};};};}case 8: _LLA4:
# 645
 Cyc_braces_to_match=1;
Cyc_specbuf=Cyc_Buffer_create(255);
while(Cyc_block(lexbuf)){;}{
struct _tuple22*_tmp4CD;struct _tuple22*x=(_tmp4CD=_cycalloc(sizeof(*_tmp4CD)),((_tmp4CD->f1=(struct _dyneither_ptr)_tag_dyneither(0,0,0),((_tmp4CD->f2=(struct _dyneither_ptr)
Cyc_Buffer_contents((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf)),_tmp4CD)))));
{struct Cyc_List_List*_tmp4CE;Cyc_current_cycstubs=((_tmp4CE=_cycalloc(sizeof(*_tmp4CE)),((_tmp4CE->hd=x,((_tmp4CE->tl=Cyc_current_cycstubs,_tmp4CE))))));}
return 1;};case 9: _LLA5: {
# 653
struct _dyneither_ptr _tmp91=Cyc_Lexing_lexeme(lexbuf);
{const char*_tmp4CF;_dyneither_ptr_inplace_plus(& _tmp91,sizeof(char),(int)Cyc_strlen(((_tmp4CF="cycstub",_tag_dyneither(_tmp4CF,sizeof(char),8)))));}
while(isspace((int)*((char*)_check_dyneither_subscript(_tmp91,sizeof(char),0)))){_dyneither_ptr_inplace_plus(& _tmp91,sizeof(char),1);}{
struct _dyneither_ptr t=_tmp91;
while(!isspace((int)*((char*)_check_dyneither_subscript(t,sizeof(char),0)))){_dyneither_ptr_inplace_plus(& t,sizeof(char),1);}{
struct _dyneither_ptr _tmp93=Cyc_substring((struct _dyneither_ptr)_tmp91,0,(unsigned long)((t.curr - _tmp91.curr)/ sizeof(char)));
Cyc_braces_to_match=1;
Cyc_specbuf=Cyc_Buffer_create(255);
while(Cyc_block(lexbuf)){;}{
struct _tuple22*_tmp4D0;struct _tuple22*x=(_tmp4D0=_cycalloc(sizeof(*_tmp4D0)),((_tmp4D0->f1=(struct _dyneither_ptr)_tmp93,((_tmp4D0->f2=(struct _dyneither_ptr)
Cyc_Buffer_contents((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf)),_tmp4D0)))));
{struct Cyc_List_List*_tmp4D1;Cyc_current_cycstubs=((_tmp4D1=_cycalloc(sizeof(*_tmp4D1)),((_tmp4D1->hd=x,((_tmp4D1->tl=Cyc_current_cycstubs,_tmp4D1))))));}
return 1;};};};}case 10: _LLA6:
# 667
 Cyc_braces_to_match=1;
Cyc_specbuf=Cyc_Buffer_create(255);
while(Cyc_block(lexbuf)){;}{
struct _dyneither_ptr*_tmp4D2;struct _dyneither_ptr*x=(_tmp4D2=_cycalloc(sizeof(*_tmp4D2)),((_tmp4D2[0]=(struct _dyneither_ptr)Cyc_Buffer_contents((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf)),_tmp4D2)));
{struct Cyc_List_List*_tmp4D3;Cyc_current_cpp=((_tmp4D3=_cycalloc(sizeof(*_tmp4D3)),((_tmp4D3->hd=x,((_tmp4D3->tl=Cyc_current_cpp,_tmp4D3))))));}
return 1;};case 11: _LLA7:
# 674
 return 1;case 12: _LLA8:
# 676
 return 1;case 13: _LLA9:
# 678
{const char*_tmp4D7;void*_tmp4D6[1];struct Cyc_Int_pa_PrintArg_struct _tmp4D5;(_tmp4D5.tag=1,((_tmp4D5.f1=(unsigned long)((int)
# 680
Cyc_Lexing_lexeme_char(lexbuf,0)),((_tmp4D6[0]=& _tmp4D5,Cyc_fprintf(Cyc_stderr,((_tmp4D7="Error in .cys file: expected command, found '%c' instead\n",_tag_dyneither(_tmp4D7,sizeof(char),58))),_tag_dyneither(_tmp4D6,sizeof(void*),1)))))));}
return 0;default: _LLAA:
(lexbuf->refill_buff)(lexbuf);
return Cyc_commands_rec(lexbuf,lexstate);}{
# 685
struct Cyc_Lexing_Error_exn_struct _tmp4DD;const char*_tmp4DC;struct Cyc_Lexing_Error_exn_struct*_tmp4DB;(int)_throw((void*)((_tmp4DB=_cycalloc(sizeof(*_tmp4DB)),((_tmp4DB[0]=((_tmp4DD.tag=Cyc_Lexing_Error,((_tmp4DD.f1=((_tmp4DC="some action didn't return!",_tag_dyneither(_tmp4DC,sizeof(char),27))),_tmp4DD)))),_tmp4DB)))));};}
# 687
int Cyc_commands(struct Cyc_Lexing_lexbuf*lexbuf){return Cyc_commands_rec(lexbuf,14);}
int Cyc_snarfsymbols_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=Cyc_lex_engine(lexstate,lexbuf);
switch(lexstate){case 0: _LLAC:
# 690 "buildlib.cyl"
{struct _dyneither_ptr*_tmp4E0;struct Cyc_List_List*_tmp4DF;Cyc_snarfed_symbols=((_tmp4DF=_cycalloc(sizeof(*_tmp4DF)),((_tmp4DF->hd=((_tmp4E0=_cycalloc(sizeof(*_tmp4E0)),((_tmp4E0[0]=(struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf),_tmp4E0)))),((_tmp4DF->tl=Cyc_snarfed_symbols,_tmp4DF))))));}
return 1;case 1: _LLAD:
# 693
 return 1;case 2: _LLAE:
# 695
 return 0;case 3: _LLAF:
# 697
{const char*_tmp4E3;void*_tmp4E2;(_tmp4E2=0,Cyc_fprintf(Cyc_stderr,((_tmp4E3="Error in .cys file: unexpected end-of-file\n",_tag_dyneither(_tmp4E3,sizeof(char),44))),_tag_dyneither(_tmp4E2,sizeof(void*),0)));}
# 699
return 0;case 4: _LLB0:
# 701
{const char*_tmp4E7;void*_tmp4E6[1];struct Cyc_Int_pa_PrintArg_struct _tmp4E5;(_tmp4E5.tag=1,((_tmp4E5.f1=(unsigned long)((int)
# 703
Cyc_Lexing_lexeme_char(lexbuf,0)),((_tmp4E6[0]=& _tmp4E5,Cyc_fprintf(Cyc_stderr,((_tmp4E7="Error in .cys file: expected symbol, found '%c' instead\n",_tag_dyneither(_tmp4E7,sizeof(char),57))),_tag_dyneither(_tmp4E6,sizeof(void*),1)))))));}
return 0;default: _LLB1:
(lexbuf->refill_buff)(lexbuf);
return Cyc_snarfsymbols_rec(lexbuf,lexstate);}{
# 708
struct Cyc_Lexing_Error_exn_struct _tmp4ED;const char*_tmp4EC;struct Cyc_Lexing_Error_exn_struct*_tmp4EB;(int)_throw((void*)((_tmp4EB=_cycalloc(sizeof(*_tmp4EB)),((_tmp4EB[0]=((_tmp4ED.tag=Cyc_Lexing_Error,((_tmp4ED.f1=((_tmp4EC="some action didn't return!",_tag_dyneither(_tmp4EC,sizeof(char),27))),_tmp4ED)))),_tmp4EB)))));};}
# 710
int Cyc_snarfsymbols(struct Cyc_Lexing_lexbuf*lexbuf){return Cyc_snarfsymbols_rec(lexbuf,15);}
int Cyc_block_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=Cyc_lex_engine(lexstate,lexbuf);
switch(lexstate){case 0: _LLB3:
# 714 "buildlib.cyl"
{const char*_tmp4F0;void*_tmp4EF;(_tmp4EF=0,Cyc_log(((_tmp4F0="Warning: unclosed brace\n",_tag_dyneither(_tmp4F0,sizeof(char),25))),_tag_dyneither(_tmp4EF,sizeof(void*),0)));}return 0;case 1: _LLB4:
# 716
 if(Cyc_braces_to_match == 1)return 0;
-- Cyc_braces_to_match;
Cyc_Buffer_add_char((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf),'}');
return 1;case 2: _LLB5:
# 721
 ++ Cyc_braces_to_match;
Cyc_Buffer_add_char((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf),'{');
return 1;case 3: _LLB6:
# 725
 Cyc_Buffer_add_char((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf),'"');
while(Cyc_block_string(lexbuf)){;}
return 1;case 4: _LLB7:
# 729
{const char*_tmp4F1;Cyc_Buffer_add_string((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf),((_tmp4F1="/*",_tag_dyneither(_tmp4F1,sizeof(char),3))));}
while(Cyc_block_comment(lexbuf)){;}
return 1;case 5: _LLB8:
# 733
 Cyc_Buffer_add_string((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf),(struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf));
return 1;case 6: _LLB9:
# 736
 Cyc_Buffer_add_char((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf),Cyc_Lexing_lexeme_char(lexbuf,0));
return 1;default: _LLBA:
(lexbuf->refill_buff)(lexbuf);
return Cyc_block_rec(lexbuf,lexstate);}{
# 741
struct Cyc_Lexing_Error_exn_struct _tmp4F7;const char*_tmp4F6;struct Cyc_Lexing_Error_exn_struct*_tmp4F5;(int)_throw((void*)((_tmp4F5=_cycalloc(sizeof(*_tmp4F5)),((_tmp4F5[0]=((_tmp4F7.tag=Cyc_Lexing_Error,((_tmp4F7.f1=((_tmp4F6="some action didn't return!",_tag_dyneither(_tmp4F6,sizeof(char),27))),_tmp4F7)))),_tmp4F5)))));};}
# 743
int Cyc_block(struct Cyc_Lexing_lexbuf*lexbuf){return Cyc_block_rec(lexbuf,16);}
int Cyc_block_string_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=Cyc_lex_engine(lexstate,lexbuf);
switch(lexstate){case 0: _LLBC:
# 741 "buildlib.cyl"
{const char*_tmp4FA;void*_tmp4F9;(_tmp4F9=0,Cyc_log(((_tmp4FA="Warning: unclosed string\n",_tag_dyneither(_tmp4FA,sizeof(char),26))),_tag_dyneither(_tmp4F9,sizeof(void*),0)));}return 0;case 1: _LLBD:
# 743
 Cyc_Buffer_add_char((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf),'"');return 0;case 2: _LLBE:
# 745
{const char*_tmp4FD;void*_tmp4FC;(_tmp4FC=0,Cyc_log(((_tmp4FD="Warning: unclosed string\n",_tag_dyneither(_tmp4FD,sizeof(char),26))),_tag_dyneither(_tmp4FC,sizeof(void*),0)));}
Cyc_Buffer_add_string((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf),(struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf));
return 1;case 3: _LLBF:
# 749
 Cyc_Buffer_add_string((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf),(struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf));
return 1;case 4: _LLC0:
# 752
 Cyc_Buffer_add_string((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf),(struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf));
return 1;case 5: _LLC1:
# 755
 Cyc_Buffer_add_string((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf),(struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf));
return 1;case 6: _LLC2:
# 758
 Cyc_Buffer_add_string((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf),(struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf));
return 1;case 7: _LLC3:
# 761
 Cyc_Buffer_add_string((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf),(struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf));
return 1;case 8: _LLC4:
# 764
 Cyc_Buffer_add_string((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf),(struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf));
return 1;default: _LLC5:
(lexbuf->refill_buff)(lexbuf);
return Cyc_block_string_rec(lexbuf,lexstate);}{
# 769
struct Cyc_Lexing_Error_exn_struct _tmp503;const char*_tmp502;struct Cyc_Lexing_Error_exn_struct*_tmp501;(int)_throw((void*)((_tmp501=_cycalloc(sizeof(*_tmp501)),((_tmp501[0]=((_tmp503.tag=Cyc_Lexing_Error,((_tmp503.f1=((_tmp502="some action didn't return!",_tag_dyneither(_tmp502,sizeof(char),27))),_tmp503)))),_tmp501)))));};}
# 771
int Cyc_block_string(struct Cyc_Lexing_lexbuf*lexbuf){return Cyc_block_string_rec(lexbuf,17);}
int Cyc_block_comment_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=Cyc_lex_engine(lexstate,lexbuf);
switch(lexstate){case 0: _LLC7:
# 769 "buildlib.cyl"
{const char*_tmp506;void*_tmp505;(_tmp505=0,Cyc_log(((_tmp506="Warning: unclosed comment\n",_tag_dyneither(_tmp506,sizeof(char),27))),_tag_dyneither(_tmp505,sizeof(void*),0)));}return 0;case 1: _LLC8:
# 771
{const char*_tmp507;Cyc_Buffer_add_string((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf),((_tmp507="*/",_tag_dyneither(_tmp507,sizeof(char),3))));}return 0;case 2: _LLC9:
# 773
 Cyc_Buffer_add_string((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf),(struct _dyneither_ptr)Cyc_Lexing_lexeme(lexbuf));
return 1;default: _LLCA:
(lexbuf->refill_buff)(lexbuf);
return Cyc_block_comment_rec(lexbuf,lexstate);}{
# 778
struct Cyc_Lexing_Error_exn_struct _tmp50D;const char*_tmp50C;struct Cyc_Lexing_Error_exn_struct*_tmp50B;(int)_throw((void*)((_tmp50B=_cycalloc(sizeof(*_tmp50B)),((_tmp50B[0]=((_tmp50D.tag=Cyc_Lexing_Error,((_tmp50D.f1=((_tmp50C="some action didn't return!",_tag_dyneither(_tmp50C,sizeof(char),27))),_tmp50D)))),_tmp50B)))));};}
# 780
int Cyc_block_comment(struct Cyc_Lexing_lexbuf*lexbuf){return Cyc_block_comment_rec(lexbuf,18);}
# 783 "buildlib.cyl"
typedef struct Cyc_Hashtable_Table*Cyc_dep_t;
# 786
void Cyc_scan_type(void*t,struct Cyc_Hashtable_Table*dep);struct _tuple24{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
void Cyc_scan_exp(struct Cyc_Absyn_Exp*e,struct Cyc_Hashtable_Table*dep){
void*_tmpBB=((struct Cyc_Absyn_Exp*)_check_null(e))->r;void*_tmpBC=_tmpBB;struct _dyneither_ptr*_tmpBE;struct Cyc_List_List*_tmpC0;struct Cyc_Absyn_Exp*_tmpC2;struct Cyc_Absyn_Exp*_tmpC3;struct Cyc_Absyn_Exp*_tmpC5;struct Cyc_Absyn_Exp*_tmpC6;struct Cyc_Absyn_Exp*_tmpC8;struct Cyc_Absyn_Exp*_tmpC9;struct Cyc_Absyn_Exp*_tmpCB;struct Cyc_Absyn_Exp*_tmpCD;struct Cyc_Absyn_Exp*_tmpCF;struct Cyc_Absyn_Exp*_tmpD1;struct Cyc_Absyn_Exp*_tmpD3;struct Cyc_Absyn_Exp*_tmpD4;struct Cyc_Absyn_Exp*_tmpD5;struct Cyc_Absyn_Exp*_tmpD7;struct Cyc_Absyn_Exp*_tmpD8;struct Cyc_Absyn_Exp*_tmpDA;struct Cyc_Absyn_Exp*_tmpDB;struct Cyc_Absyn_Exp*_tmpDD;struct Cyc_List_List*_tmpDE;void*_tmpE0;struct Cyc_Absyn_Exp*_tmpE1;int _tmpE3;struct Cyc_Absyn_Exp*_tmpE4;void**_tmpE5;struct Cyc_Absyn_Exp*_tmpE6;struct Cyc_Absyn_Exp*_tmpE8;void*_tmpEA;void*_tmpEC;struct Cyc_Absyn_Exp*_tmpEE;struct _dyneither_ptr*_tmpEF;struct Cyc_Absyn_Exp*_tmpF1;struct _dyneither_ptr*_tmpF2;void*_tmpF4;struct Cyc_List_List*_tmpF5;struct Cyc_List_List*_tmpF8;_LLCD: {struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*_tmpBD=(struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpBD->tag != 1)goto _LLCF;else{_tmpBE=(_tmpBD->f1)->f2;}}_LLCE:
# 790
 Cyc_add_target(_tmpBE);
return;_LLCF: {struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*_tmpBF=(struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpBF->tag != 2)goto _LLD1;else{_tmpC0=_tmpBF->f2;}}_LLD0:
# 793
 for(0;_tmpC0 != 0;_tmpC0=_tmpC0->tl){
Cyc_scan_exp((struct Cyc_Absyn_Exp*)_tmpC0->hd,dep);}
# 796
return;_LLD1: {struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*_tmpC1=(struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpC1->tag != 22)goto _LLD3;else{_tmpC2=_tmpC1->f1;_tmpC3=_tmpC1->f2;}}_LLD2:
# 798
 _tmpC5=_tmpC2;_tmpC6=_tmpC3;goto _LLD4;_LLD3: {struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*_tmpC4=(struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpC4->tag != 8)goto _LLD5;else{_tmpC5=_tmpC4->f1;_tmpC6=_tmpC4->f2;}}_LLD4:
# 800
 _tmpC8=_tmpC5;_tmpC9=_tmpC6;goto _LLD6;_LLD5: {struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*_tmpC7=(struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpC7->tag != 3)goto _LLD7;else{_tmpC8=_tmpC7->f1;_tmpC9=_tmpC7->f3;}}_LLD6:
# 802
 Cyc_scan_exp(_tmpC8,dep);
Cyc_scan_exp(_tmpC9,dep);
return;_LLD7: {struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*_tmpCA=(struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpCA->tag != 19)goto _LLD9;else{_tmpCB=_tmpCA->f1;}}_LLD8:
# 806
 _tmpCD=_tmpCB;goto _LLDA;_LLD9: {struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*_tmpCC=(struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpCC->tag != 17)goto _LLDB;else{_tmpCD=_tmpCC->f1;}}_LLDA:
# 808
 _tmpCF=_tmpCD;goto _LLDC;_LLDB: {struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*_tmpCE=(struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpCE->tag != 14)goto _LLDD;else{_tmpCF=_tmpCE->f1;}}_LLDC:
# 810
 _tmpD1=_tmpCF;goto _LLDE;_LLDD: {struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*_tmpD0=(struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpD0->tag != 4)goto _LLDF;else{_tmpD1=_tmpD0->f1;}}_LLDE:
# 812
 Cyc_scan_exp(_tmpD1,dep);
return;_LLDF: {struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*_tmpD2=(struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpD2->tag != 5)goto _LLE1;else{_tmpD3=_tmpD2->f1;_tmpD4=_tmpD2->f2;_tmpD5=_tmpD2->f3;}}_LLE0:
# 815
 Cyc_scan_exp(_tmpD3,dep);
Cyc_scan_exp(_tmpD4,dep);
Cyc_scan_exp(_tmpD5,dep);
return;_LLE1: {struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*_tmpD6=(struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpD6->tag != 6)goto _LLE3;else{_tmpD7=_tmpD6->f1;_tmpD8=_tmpD6->f2;}}_LLE2:
 _tmpDA=_tmpD7;_tmpDB=_tmpD8;goto _LLE4;_LLE3: {struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*_tmpD9=(struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpD9->tag != 7)goto _LLE5;else{_tmpDA=_tmpD9->f1;_tmpDB=_tmpD9->f2;}}_LLE4:
# 821
 Cyc_scan_exp(_tmpDA,dep);
Cyc_scan_exp(_tmpDB,dep);
return;_LLE5: {struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_tmpDC=(struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpDC->tag != 9)goto _LLE7;else{_tmpDD=_tmpDC->f1;_tmpDE=_tmpDC->f2;}}_LLE6:
# 825
 Cyc_scan_exp(_tmpDD,dep);
for(0;_tmpDE != 0;_tmpDE=_tmpDE->tl){
Cyc_scan_exp((struct Cyc_Absyn_Exp*)_tmpDE->hd,dep);}
# 829
return;_LLE7: {struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_tmpDF=(struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpDF->tag != 13)goto _LLE9;else{_tmpE0=(void*)_tmpDF->f1;_tmpE1=_tmpDF->f2;}}_LLE8:
# 831
 Cyc_scan_type(_tmpE0,dep);
Cyc_scan_exp(_tmpE1,dep);
return;_LLE9: {struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*_tmpE2=(struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpE2->tag != 33)goto _LLEB;else{_tmpE3=(_tmpE2->f1).is_calloc;_tmpE4=(_tmpE2->f1).rgn;_tmpE5=(_tmpE2->f1).elt_type;_tmpE6=(_tmpE2->f1).num_elts;}}_LLEA:
# 835
 if(_tmpE4 != 0)Cyc_scan_exp(_tmpE4,dep);
if(_tmpE5 != 0)Cyc_scan_type(*_tmpE5,dep);
Cyc_scan_exp(_tmpE6,dep);
return;_LLEB: {struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*_tmpE7=(struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpE7->tag != 37)goto _LLED;else{_tmpE8=_tmpE7->f1;}}_LLEC:
# 840
 Cyc_scan_exp(_tmpE8,dep);return;_LLED: {struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*_tmpE9=(struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpE9->tag != 38)goto _LLEF;else{_tmpEA=(void*)_tmpE9->f1;}}_LLEE:
 _tmpEC=_tmpEA;goto _LLF0;_LLEF: {struct Cyc_Absyn_Sizeoftyp_e_Absyn_Raw_exp_struct*_tmpEB=(struct Cyc_Absyn_Sizeoftyp_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpEB->tag != 16)goto _LLF1;else{_tmpEC=(void*)_tmpEB->f1;}}_LLF0:
# 843
 Cyc_scan_type(_tmpEC,dep);
return;_LLF1: {struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_tmpED=(struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpED->tag != 20)goto _LLF3;else{_tmpEE=_tmpED->f1;_tmpEF=_tmpED->f2;}}_LLF2:
# 846
 _tmpF1=_tmpEE;_tmpF2=_tmpEF;goto _LLF4;_LLF3: {struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_tmpF0=(struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpF0->tag != 21)goto _LLF5;else{_tmpF1=_tmpF0->f1;_tmpF2=_tmpF0->f2;}}_LLF4:
# 848
 Cyc_scan_exp(_tmpF1,dep);
Cyc_add_target(_tmpF2);
return;_LLF5: {struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*_tmpF3=(struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpF3->tag != 18)goto _LLF7;else{_tmpF4=(void*)_tmpF3->f1;_tmpF5=_tmpF3->f2;}}_LLF6:
# 852
 Cyc_scan_type(_tmpF4,dep);
# 854
{void*_tmp10A=(void*)((struct Cyc_List_List*)_check_null(_tmpF5))->hd;void*_tmp10B=_tmp10A;struct _dyneither_ptr*_tmp10D;_LL11E: {struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*_tmp10C=(struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*)_tmp10B;if(_tmp10C->tag != 0)goto _LL120;else{_tmp10D=_tmp10C->f1;}}_LL11F:
 Cyc_add_target(_tmp10D);goto _LL11D;_LL120: {struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct*_tmp10E=(struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct*)_tmp10B;if(_tmp10E->tag != 1)goto _LL11D;}_LL121:
 goto _LL11D;_LL11D:;}
# 858
return;_LLF7: {struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_tmpF6=(struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpF6->tag != 0)goto _LLF9;}_LLF8:
# 860
 return;_LLF9: {struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_tmpF7=(struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpF7->tag != 35)goto _LLFB;else{_tmpF8=_tmpF7->f2;}}_LLFA:
# 862
 for(0;_tmpF8 != 0;_tmpF8=_tmpF8->tl){
struct _tuple24*_tmp10F=(struct _tuple24*)_tmpF8->hd;struct Cyc_Absyn_Exp*_tmp111;struct _tuple24*_tmp110=_tmp10F;_tmp111=_tmp110->f2;
Cyc_scan_exp(_tmp111,dep);}
# 866
return;_LLFB: {struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*_tmpF9=(struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpF9->tag != 39)goto _LLFD;}_LLFC:
 return;_LLFD: {struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*_tmpFA=(struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpFA->tag != 34)goto _LLFF;}_LLFE:
# 869
{const char*_tmp510;void*_tmp50F;(_tmp50F=0,Cyc_fprintf(Cyc_stderr,((_tmp510="Error: unexpected Swap_e\n",_tag_dyneither(_tmp510,sizeof(char),26))),_tag_dyneither(_tmp50F,sizeof(void*),0)));}
exit(1);return;_LLFF: {struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*_tmpFB=(struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpFB->tag != 36)goto _LL101;}_LL100:
# 872
{const char*_tmp513;void*_tmp512;(_tmp512=0,Cyc_fprintf(Cyc_stderr,((_tmp513="Error: unexpected Stmt_e\n",_tag_dyneither(_tmp513,sizeof(char),26))),_tag_dyneither(_tmp512,sizeof(void*),0)));}
exit(1);return;_LL101: {struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*_tmpFC=(struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpFC->tag != 10)goto _LL103;}_LL102:
# 875
{const char*_tmp516;void*_tmp515;(_tmp515=0,Cyc_fprintf(Cyc_stderr,((_tmp516="Error: unexpected Throw_e\n",_tag_dyneither(_tmp516,sizeof(char),27))),_tag_dyneither(_tmp515,sizeof(void*),0)));}
exit(1);return;_LL103: {struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*_tmpFD=(struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpFD->tag != 11)goto _LL105;}_LL104:
# 878
{const char*_tmp519;void*_tmp518;(_tmp518=0,Cyc_fprintf(Cyc_stderr,((_tmp519="Error: unexpected NoInstantiate_e\n",_tag_dyneither(_tmp519,sizeof(char),35))),_tag_dyneither(_tmp518,sizeof(void*),0)));}
exit(1);return;_LL105: {struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*_tmpFE=(struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpFE->tag != 12)goto _LL107;}_LL106:
# 881
{const char*_tmp51C;void*_tmp51B;(_tmp51B=0,Cyc_fprintf(Cyc_stderr,((_tmp51C="Error: unexpected Instantiate_e\n",_tag_dyneither(_tmp51C,sizeof(char),33))),_tag_dyneither(_tmp51B,sizeof(void*),0)));}
exit(1);return;_LL107: {struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*_tmpFF=(struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmpFF->tag != 15)goto _LL109;}_LL108:
# 884
{const char*_tmp51F;void*_tmp51E;(_tmp51E=0,Cyc_fprintf(Cyc_stderr,((_tmp51F="Error: unexpected New_e\n",_tag_dyneither(_tmp51F,sizeof(char),25))),_tag_dyneither(_tmp51E,sizeof(void*),0)));}
exit(1);return;_LL109: {struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*_tmp100=(struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmp100->tag != 23)goto _LL10B;}_LL10A:
# 887
{const char*_tmp522;void*_tmp521;(_tmp521=0,Cyc_fprintf(Cyc_stderr,((_tmp522="Error: unexpected Tuple_e\n",_tag_dyneither(_tmp522,sizeof(char),27))),_tag_dyneither(_tmp521,sizeof(void*),0)));}
exit(1);return;_LL10B: {struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*_tmp101=(struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmp101->tag != 24)goto _LL10D;}_LL10C:
# 890
{const char*_tmp525;void*_tmp524;(_tmp524=0,Cyc_fprintf(Cyc_stderr,((_tmp525="Error: unexpected CompoundLit_e\n",_tag_dyneither(_tmp525,sizeof(char),33))),_tag_dyneither(_tmp524,sizeof(void*),0)));}
exit(1);return;_LL10D: {struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_tmp102=(struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmp102->tag != 25)goto _LL10F;}_LL10E:
# 893
{const char*_tmp528;void*_tmp527;(_tmp527=0,Cyc_fprintf(Cyc_stderr,((_tmp528="Error: unexpected Array_e\n",_tag_dyneither(_tmp528,sizeof(char),27))),_tag_dyneither(_tmp527,sizeof(void*),0)));}
exit(1);return;_LL10F: {struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*_tmp103=(struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmp103->tag != 26)goto _LL111;}_LL110:
# 896
{const char*_tmp52B;void*_tmp52A;(_tmp52A=0,Cyc_fprintf(Cyc_stderr,((_tmp52B="Error: unexpected Comprehension_e\n",_tag_dyneither(_tmp52B,sizeof(char),35))),_tag_dyneither(_tmp52A,sizeof(void*),0)));}
exit(1);return;_LL111: {struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*_tmp104=(struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmp104->tag != 27)goto _LL113;}_LL112:
# 899
{const char*_tmp52E;void*_tmp52D;(_tmp52D=0,Cyc_fprintf(Cyc_stderr,((_tmp52E="Error: unexpected ComprehensionNoinit_e\n",_tag_dyneither(_tmp52E,sizeof(char),41))),_tag_dyneither(_tmp52D,sizeof(void*),0)));}
exit(1);return;_LL113: {struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_tmp105=(struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmp105->tag != 28)goto _LL115;}_LL114:
# 902
{const char*_tmp531;void*_tmp530;(_tmp530=0,Cyc_fprintf(Cyc_stderr,((_tmp531="Error: unexpected Aggregate_e\n",_tag_dyneither(_tmp531,sizeof(char),31))),_tag_dyneither(_tmp530,sizeof(void*),0)));}
exit(1);return;_LL115: {struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*_tmp106=(struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmp106->tag != 29)goto _LL117;}_LL116:
# 905
{const char*_tmp534;void*_tmp533;(_tmp533=0,Cyc_fprintf(Cyc_stderr,((_tmp534="Error: unexpected AnonStruct_e\n",_tag_dyneither(_tmp534,sizeof(char),32))),_tag_dyneither(_tmp533,sizeof(void*),0)));}
exit(1);return;_LL117: {struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*_tmp107=(struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmp107->tag != 30)goto _LL119;}_LL118:
# 908
{const char*_tmp537;void*_tmp536;(_tmp536=0,Cyc_fprintf(Cyc_stderr,((_tmp537="Error: unexpected Datatype_e\n",_tag_dyneither(_tmp537,sizeof(char),30))),_tag_dyneither(_tmp536,sizeof(void*),0)));}
exit(1);return;_LL119: {struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*_tmp108=(struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmp108->tag != 31)goto _LL11B;}_LL11A:
# 911
{const char*_tmp53A;void*_tmp539;(_tmp539=0,Cyc_fprintf(Cyc_stderr,((_tmp53A="Error: unexpected Enum_e\n",_tag_dyneither(_tmp53A,sizeof(char),26))),_tag_dyneither(_tmp539,sizeof(void*),0)));}
exit(1);return;_LL11B: {struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*_tmp109=(struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*)_tmpBC;if(_tmp109->tag != 32)goto _LLCC;}_LL11C:
# 914
{const char*_tmp53D;void*_tmp53C;(_tmp53C=0,Cyc_fprintf(Cyc_stderr,((_tmp53D="Error: unexpected AnonEnum_e\n",_tag_dyneither(_tmp53D,sizeof(char),30))),_tag_dyneither(_tmp53C,sizeof(void*),0)));}
exit(1);return;_LLCC:;}
# 919
void Cyc_scan_exp_opt(struct Cyc_Absyn_Exp*eo,struct Cyc_Hashtable_Table*dep){
if((unsigned int)eo)Cyc_scan_exp(eo,dep);
return;}
# 924
void Cyc_scan_decl(struct Cyc_Absyn_Decl*d,struct Cyc_Hashtable_Table*dep);
void Cyc_scan_type(void*t,struct Cyc_Hashtable_Table*dep){
void*_tmp132=t;struct Cyc_Absyn_PtrInfo _tmp137;void*_tmp139;struct Cyc_Absyn_Exp*_tmp13A;union Cyc_Absyn_Constraint*_tmp13B;struct Cyc_Absyn_FnInfo _tmp13D;struct Cyc_List_List*_tmp13F;union Cyc_Absyn_AggrInfoU _tmp142;struct _dyneither_ptr*_tmp144;struct _dyneither_ptr*_tmp146;struct Cyc_Absyn_Aggrdecl*_tmp149;struct Cyc_Absyn_Enumdecl*_tmp14C;struct Cyc_Absyn_Datatypedecl*_tmp14F;_LL123: {struct Cyc_Absyn_VoidType_Absyn_Type_struct*_tmp133=(struct Cyc_Absyn_VoidType_Absyn_Type_struct*)_tmp132;if(_tmp133->tag != 0)goto _LL125;}_LL124:
 goto _LL126;_LL125: {struct Cyc_Absyn_IntType_Absyn_Type_struct*_tmp134=(struct Cyc_Absyn_IntType_Absyn_Type_struct*)_tmp132;if(_tmp134->tag != 6)goto _LL127;}_LL126:
 goto _LL128;_LL127: {struct Cyc_Absyn_FloatType_Absyn_Type_struct*_tmp135=(struct Cyc_Absyn_FloatType_Absyn_Type_struct*)_tmp132;if(_tmp135->tag != 7)goto _LL129;}_LL128:
# 930
 return;_LL129: {struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmp136=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp132;if(_tmp136->tag != 5)goto _LL12B;else{_tmp137=_tmp136->f1;}}_LL12A:
# 933
 Cyc_scan_type(_tmp137.elt_typ,dep);
return;_LL12B: {struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_tmp138=(struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp132;if(_tmp138->tag != 8)goto _LL12D;else{_tmp139=(_tmp138->f1).elt_type;_tmp13A=(_tmp138->f1).num_elts;_tmp13B=(_tmp138->f1).zero_term;}}_LL12C:
# 936
 Cyc_scan_type(_tmp139,dep);
Cyc_scan_exp_opt(_tmp13A,dep);
return;_LL12D: {struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp13C=(struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp132;if(_tmp13C->tag != 9)goto _LL12F;else{_tmp13D=_tmp13C->f1;}}_LL12E:
# 940
 Cyc_scan_type(_tmp13D.ret_typ,dep);
{struct Cyc_List_List*_tmp15F=_tmp13D.args;for(0;_tmp15F != 0;_tmp15F=_tmp15F->tl){
struct _tuple8*_tmp160=(struct _tuple8*)_tmp15F->hd;void*_tmp162;struct _tuple8*_tmp161=_tmp160;_tmp162=_tmp161->f3;
Cyc_scan_type(_tmp162,dep);}}
# 945
if(_tmp13D.cyc_varargs != 0)
Cyc_scan_type((_tmp13D.cyc_varargs)->type,dep);
return;_LL12F: {struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_tmp13E=(struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp132;if(_tmp13E->tag != 12)goto _LL131;else{_tmp13F=_tmp13E->f2;}}_LL130:
# 949
 for(0;_tmp13F != 0;_tmp13F=_tmp13F->tl){
Cyc_scan_type(((struct Cyc_Absyn_Aggrfield*)_tmp13F->hd)->type,dep);
Cyc_scan_exp_opt(((struct Cyc_Absyn_Aggrfield*)_tmp13F->hd)->width,dep);}
# 953
return;_LL131: {struct Cyc_Absyn_AnonEnumType_Absyn_Type_struct*_tmp140=(struct Cyc_Absyn_AnonEnumType_Absyn_Type_struct*)_tmp132;if(_tmp140->tag != 14)goto _LL133;}_LL132:
# 955
 return;_LL133: {struct Cyc_Absyn_AggrType_Absyn_Type_struct*_tmp141=(struct Cyc_Absyn_AggrType_Absyn_Type_struct*)_tmp132;if(_tmp141->tag != 11)goto _LL135;else{_tmp142=(_tmp141->f1).aggr_info;}}_LL134: {
# 957
struct _tuple10 _tmp163=Cyc_Absyn_aggr_kinded_name(_tmp142);struct _dyneither_ptr*_tmp165;struct _tuple10 _tmp164=_tmp163;_tmp165=(_tmp164.f2)->f2;
_tmp144=_tmp165;goto _LL136;}_LL135: {struct Cyc_Absyn_EnumType_Absyn_Type_struct*_tmp143=(struct Cyc_Absyn_EnumType_Absyn_Type_struct*)_tmp132;if(_tmp143->tag != 13)goto _LL137;else{_tmp144=(_tmp143->f1)->f2;}}_LL136:
# 960
 _tmp146=_tmp144;goto _LL138;_LL137: {struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_tmp145=(struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp132;if(_tmp145->tag != 17)goto _LL139;else{_tmp146=(_tmp145->f1)->f2;}}_LL138:
# 962
 Cyc_add_target(_tmp146);
return;_LL139: {struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*_tmp147=(struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp132;if(_tmp147->tag != 26)goto _LL13B;else{struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct*_tmp148=(struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct*)(_tmp147->f1)->r;if(_tmp148->tag != 0)goto _LL13B;else{_tmp149=_tmp148->f1;}}}_LL13A:
# 966
{struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct _tmp540;struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_tmp53F;Cyc_scan_decl(Cyc_Absyn_new_decl((void*)((_tmp53F=_cycalloc(sizeof(*_tmp53F)),((_tmp53F[0]=((_tmp540.tag=6,((_tmp540.f1=_tmp149,_tmp540)))),_tmp53F)))),0),dep);}{
struct _tuple0*_tmp168=_tmp149->name;struct _dyneither_ptr*_tmp16A;struct _tuple0*_tmp169=_tmp168;_tmp16A=_tmp169->f2;
Cyc_add_target(_tmp16A);
return;};_LL13B: {struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*_tmp14A=(struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp132;if(_tmp14A->tag != 26)goto _LL13D;else{struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct*_tmp14B=(struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct*)(_tmp14A->f1)->r;if(_tmp14B->tag != 1)goto _LL13D;else{_tmp14C=_tmp14B->f1;}}}_LL13C:
# 972
{struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct _tmp543;struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*_tmp542;Cyc_scan_decl(Cyc_Absyn_new_decl((void*)((_tmp542=_cycalloc(sizeof(*_tmp542)),((_tmp542[0]=((_tmp543.tag=8,((_tmp543.f1=_tmp14C,_tmp543)))),_tmp542)))),0),dep);}{
struct _tuple0*_tmp16D=_tmp14C->name;struct _dyneither_ptr*_tmp16F;struct _tuple0*_tmp16E=_tmp16D;_tmp16F=_tmp16E->f2;
Cyc_add_target(_tmp16F);
return;};_LL13D: {struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*_tmp14D=(struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp132;if(_tmp14D->tag != 26)goto _LL13F;else{struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct*_tmp14E=(struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct*)(_tmp14D->f1)->r;if(_tmp14E->tag != 2)goto _LL13F;else{_tmp14F=_tmp14E->f1;}}}_LL13E:
# 978
{const char*_tmp546;void*_tmp545;(_tmp545=0,Cyc_fprintf(Cyc_stderr,((_tmp546="Error: unexpected Datatype declaration\n",_tag_dyneither(_tmp546,sizeof(char),40))),_tag_dyneither(_tmp545,sizeof(void*),0)));}
exit(1);return;_LL13F: {struct Cyc_Absyn_Evar_Absyn_Type_struct*_tmp150=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp132;if(_tmp150->tag != 1)goto _LL141;}_LL140:
# 981
{const char*_tmp549;void*_tmp548;(_tmp548=0,Cyc_fprintf(Cyc_stderr,((_tmp549="Error: unexpected Evar\n",_tag_dyneither(_tmp549,sizeof(char),24))),_tag_dyneither(_tmp548,sizeof(void*),0)));}
exit(1);return;_LL141: {struct Cyc_Absyn_VarType_Absyn_Type_struct*_tmp151=(struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp132;if(_tmp151->tag != 2)goto _LL143;}_LL142:
# 984
{const char*_tmp54C;void*_tmp54B;(_tmp54B=0,Cyc_fprintf(Cyc_stderr,((_tmp54C="Error: unexpected VarType\n",_tag_dyneither(_tmp54C,sizeof(char),27))),_tag_dyneither(_tmp54B,sizeof(void*),0)));}
exit(1);return;_LL143: {struct Cyc_Absyn_DatatypeType_Absyn_Type_struct*_tmp152=(struct Cyc_Absyn_DatatypeType_Absyn_Type_struct*)_tmp132;if(_tmp152->tag != 3)goto _LL145;}_LL144:
# 987
{const char*_tmp54F;void*_tmp54E;(_tmp54E=0,Cyc_fprintf(Cyc_stderr,((_tmp54F="Error: unexpected DatatypeType\n",_tag_dyneither(_tmp54F,sizeof(char),32))),_tag_dyneither(_tmp54E,sizeof(void*),0)));}
exit(1);return;_LL145: {struct Cyc_Absyn_DatatypeFieldType_Absyn_Type_struct*_tmp153=(struct Cyc_Absyn_DatatypeFieldType_Absyn_Type_struct*)_tmp132;if(_tmp153->tag != 4)goto _LL147;}_LL146:
# 990
{const char*_tmp552;void*_tmp551;(_tmp551=0,Cyc_fprintf(Cyc_stderr,((_tmp552="Error: unexpected DatatypeFieldType\n",_tag_dyneither(_tmp552,sizeof(char),37))),_tag_dyneither(_tmp551,sizeof(void*),0)));}
exit(1);return;_LL147: {struct Cyc_Absyn_TupleType_Absyn_Type_struct*_tmp154=(struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp132;if(_tmp154->tag != 10)goto _LL149;}_LL148:
# 993
{const char*_tmp555;void*_tmp554;(_tmp554=0,Cyc_fprintf(Cyc_stderr,((_tmp555="Error: unexpected TupleType\n",_tag_dyneither(_tmp555,sizeof(char),29))),_tag_dyneither(_tmp554,sizeof(void*),0)));}
exit(1);return;_LL149: {struct Cyc_Absyn_RgnHandleType_Absyn_Type_struct*_tmp155=(struct Cyc_Absyn_RgnHandleType_Absyn_Type_struct*)_tmp132;if(_tmp155->tag != 15)goto _LL14B;}_LL14A:
# 996
{const char*_tmp558;void*_tmp557;(_tmp557=0,Cyc_fprintf(Cyc_stderr,((_tmp558="Error: unexpected RgnHandleType\n",_tag_dyneither(_tmp558,sizeof(char),33))),_tag_dyneither(_tmp557,sizeof(void*),0)));}
exit(1);return;_LL14B: {struct Cyc_Absyn_DynRgnType_Absyn_Type_struct*_tmp156=(struct Cyc_Absyn_DynRgnType_Absyn_Type_struct*)_tmp132;if(_tmp156->tag != 16)goto _LL14D;}_LL14C:
# 999
{const char*_tmp55B;void*_tmp55A;(_tmp55A=0,Cyc_fprintf(Cyc_stderr,((_tmp55B="Error: unexpected DynRgnType\n",_tag_dyneither(_tmp55B,sizeof(char),30))),_tag_dyneither(_tmp55A,sizeof(void*),0)));}
exit(1);return;_LL14D: {struct Cyc_Absyn_HeapRgn_Absyn_Type_struct*_tmp157=(struct Cyc_Absyn_HeapRgn_Absyn_Type_struct*)_tmp132;if(_tmp157->tag != 20)goto _LL14F;}_LL14E:
# 1002
{const char*_tmp55E;void*_tmp55D;(_tmp55D=0,Cyc_fprintf(Cyc_stderr,((_tmp55E="Error: unexpected HeapRgn\n",_tag_dyneither(_tmp55E,sizeof(char),27))),_tag_dyneither(_tmp55D,sizeof(void*),0)));}
exit(1);return;_LL14F: {struct Cyc_Absyn_UniqueRgn_Absyn_Type_struct*_tmp158=(struct Cyc_Absyn_UniqueRgn_Absyn_Type_struct*)_tmp132;if(_tmp158->tag != 21)goto _LL151;}_LL150:
# 1005
{const char*_tmp561;void*_tmp560;(_tmp560=0,Cyc_fprintf(Cyc_stderr,((_tmp561="Error: unexpected UniqueRgn\n",_tag_dyneither(_tmp561,sizeof(char),29))),_tag_dyneither(_tmp560,sizeof(void*),0)));}
exit(1);return;_LL151: {struct Cyc_Absyn_RefCntRgn_Absyn_Type_struct*_tmp159=(struct Cyc_Absyn_RefCntRgn_Absyn_Type_struct*)_tmp132;if(_tmp159->tag != 22)goto _LL153;}_LL152:
# 1008
{const char*_tmp564;void*_tmp563;(_tmp563=0,Cyc_fprintf(Cyc_stderr,((_tmp564="Error: unexpected RefCntRgn\n",_tag_dyneither(_tmp564,sizeof(char),29))),_tag_dyneither(_tmp563,sizeof(void*),0)));}
exit(1);return;_LL153: {struct Cyc_Absyn_AccessEff_Absyn_Type_struct*_tmp15A=(struct Cyc_Absyn_AccessEff_Absyn_Type_struct*)_tmp132;if(_tmp15A->tag != 23)goto _LL155;}_LL154:
# 1011
{const char*_tmp567;void*_tmp566;(_tmp566=0,Cyc_fprintf(Cyc_stderr,((_tmp567="Error: unexpected AccessEff\n",_tag_dyneither(_tmp567,sizeof(char),29))),_tag_dyneither(_tmp566,sizeof(void*),0)));}
exit(1);return;_LL155: {struct Cyc_Absyn_JoinEff_Absyn_Type_struct*_tmp15B=(struct Cyc_Absyn_JoinEff_Absyn_Type_struct*)_tmp132;if(_tmp15B->tag != 24)goto _LL157;}_LL156:
# 1014
{const char*_tmp56A;void*_tmp569;(_tmp569=0,Cyc_fprintf(Cyc_stderr,((_tmp56A="Error: unexpected JoinEff\n",_tag_dyneither(_tmp56A,sizeof(char),27))),_tag_dyneither(_tmp569,sizeof(void*),0)));}
exit(1);return;_LL157: {struct Cyc_Absyn_RgnsEff_Absyn_Type_struct*_tmp15C=(struct Cyc_Absyn_RgnsEff_Absyn_Type_struct*)_tmp132;if(_tmp15C->tag != 25)goto _LL159;}_LL158:
# 1017
{const char*_tmp56D;void*_tmp56C;(_tmp56C=0,Cyc_fprintf(Cyc_stderr,((_tmp56D="Error: unexpected RgnsEff\n",_tag_dyneither(_tmp56D,sizeof(char),27))),_tag_dyneither(_tmp56C,sizeof(void*),0)));}
exit(1);return;_LL159: {struct Cyc_Absyn_TagType_Absyn_Type_struct*_tmp15D=(struct Cyc_Absyn_TagType_Absyn_Type_struct*)_tmp132;if(_tmp15D->tag != 19)goto _LL15B;}_LL15A:
# 1020
{const char*_tmp570;void*_tmp56F;(_tmp56F=0,Cyc_fprintf(Cyc_stderr,((_tmp570="Error: unexpected tag_t\n",_tag_dyneither(_tmp570,sizeof(char),25))),_tag_dyneither(_tmp56F,sizeof(void*),0)));}
exit(1);return;_LL15B: {struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_tmp15E=(struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp132;if(_tmp15E->tag != 18)goto _LL122;}_LL15C:
# 1023
{const char*_tmp573;void*_tmp572;(_tmp572=0,Cyc_fprintf(Cyc_stderr,((_tmp573="Error: unexpected valueof_t\n",_tag_dyneither(_tmp573,sizeof(char),29))),_tag_dyneither(_tmp572,sizeof(void*),0)));}
exit(1);return;_LL122:;}
# 1028
void Cyc_scan_decl(struct Cyc_Absyn_Decl*d,struct Cyc_Hashtable_Table*dep){
struct Cyc_Set_Set**_tmp190=Cyc_current_targets;
struct _dyneither_ptr*_tmp191=Cyc_current_source;
{struct Cyc_Set_Set**_tmp574;Cyc_current_targets=((_tmp574=_cycalloc(sizeof(*_tmp574)),((_tmp574[0]=((struct Cyc_Set_Set*(*)(int(*cmp)(struct _dyneither_ptr*,struct _dyneither_ptr*)))Cyc_Set_empty)(Cyc_strptrcmp),_tmp574))));}
{void*_tmp193=d->r;void*_tmp194=_tmp193;struct Cyc_Absyn_Vardecl*_tmp196;struct Cyc_Absyn_Fndecl*_tmp198;struct Cyc_Absyn_Aggrdecl*_tmp19A;struct Cyc_Absyn_Enumdecl*_tmp19C;struct Cyc_Absyn_Typedefdecl*_tmp19E;_LL15E: {struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmp195=(struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp194;if(_tmp195->tag != 0)goto _LL160;else{_tmp196=_tmp195->f1;}}_LL15F: {
# 1034
struct _tuple0*_tmp1AA=_tmp196->name;struct _dyneither_ptr*_tmp1AC;struct _tuple0*_tmp1AB=_tmp1AA;_tmp1AC=_tmp1AB->f2;
Cyc_current_source=_tmp1AC;
Cyc_scan_type(_tmp196->type,dep);
Cyc_scan_exp_opt(_tmp196->initializer,dep);
goto _LL15D;}_LL160: {struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*_tmp197=(struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp194;if(_tmp197->tag != 1)goto _LL162;else{_tmp198=_tmp197->f1;}}_LL161: {
# 1040
struct _tuple0*_tmp1AD=_tmp198->name;struct _dyneither_ptr*_tmp1AF;struct _tuple0*_tmp1AE=_tmp1AD;_tmp1AF=_tmp1AE->f2;
Cyc_current_source=_tmp1AF;
Cyc_scan_type(_tmp198->ret_type,dep);
{struct Cyc_List_List*_tmp1B0=_tmp198->args;for(0;_tmp1B0 != 0;_tmp1B0=_tmp1B0->tl){
struct _tuple8*_tmp1B1=(struct _tuple8*)_tmp1B0->hd;void*_tmp1B3;struct _tuple8*_tmp1B2=_tmp1B1;_tmp1B3=_tmp1B2->f3;
Cyc_scan_type(_tmp1B3,dep);}}
# 1047
if(_tmp198->cyc_varargs != 0)
Cyc_scan_type(((struct Cyc_Absyn_VarargInfo*)_check_null(_tmp198->cyc_varargs))->type,dep);
if(_tmp198->is_inline){
const char*_tmp577;void*_tmp576;(_tmp576=0,Cyc_fprintf(Cyc_stderr,((_tmp577="Warning: ignoring inline function\n",_tag_dyneither(_tmp577,sizeof(char),35))),_tag_dyneither(_tmp576,sizeof(void*),0)));}
# 1052
goto _LL15D;}_LL162: {struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_tmp199=(struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_tmp194;if(_tmp199->tag != 6)goto _LL164;else{_tmp19A=_tmp199->f1;}}_LL163: {
# 1054
struct _tuple0*_tmp1B6=_tmp19A->name;struct _dyneither_ptr*_tmp1B8;struct _tuple0*_tmp1B7=_tmp1B6;_tmp1B8=_tmp1B7->f2;
Cyc_current_source=_tmp1B8;
if((unsigned int)_tmp19A->impl){
{struct Cyc_List_List*_tmp1B9=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp19A->impl))->fields;for(0;_tmp1B9 != 0;_tmp1B9=_tmp1B9->tl){
struct Cyc_Absyn_Aggrfield*_tmp1BA=(struct Cyc_Absyn_Aggrfield*)_tmp1B9->hd;
Cyc_scan_type(_tmp1BA->type,dep);
Cyc_scan_exp_opt(_tmp1BA->width,dep);}}{
# 1064
struct Cyc_List_List*_tmp1BB=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(_tmp19A->impl))->fields;for(0;_tmp1BB != 0;_tmp1BB=_tmp1BB->tl){;}};}
# 1068
goto _LL15D;}_LL164: {struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*_tmp19B=(struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp194;if(_tmp19B->tag != 8)goto _LL166;else{_tmp19C=_tmp19B->f1;}}_LL165: {
# 1070
struct _tuple0*_tmp1BC=_tmp19C->name;struct _dyneither_ptr*_tmp1BE;struct _tuple0*_tmp1BD=_tmp1BC;_tmp1BE=_tmp1BD->f2;
Cyc_current_source=_tmp1BE;
if((unsigned int)_tmp19C->fields){
{struct Cyc_List_List*_tmp1BF=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(_tmp19C->fields))->v;for(0;_tmp1BF != 0;_tmp1BF=_tmp1BF->tl){
struct Cyc_Absyn_Enumfield*_tmp1C0=(struct Cyc_Absyn_Enumfield*)_tmp1BF->hd;
Cyc_scan_exp_opt(_tmp1C0->tag,dep);}}{
# 1079
struct Cyc_List_List*_tmp1C1=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(_tmp19C->fields))->v;for(0;_tmp1C1 != 0;_tmp1C1=_tmp1C1->tl){;}};}
# 1083
goto _LL15D;}_LL166: {struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*_tmp19D=(struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp194;if(_tmp19D->tag != 9)goto _LL168;else{_tmp19E=_tmp19D->f1;}}_LL167: {
# 1085
struct _tuple0*_tmp1C2=_tmp19E->name;struct _dyneither_ptr*_tmp1C4;struct _tuple0*_tmp1C3=_tmp1C2;_tmp1C4=_tmp1C3->f2;
Cyc_current_source=_tmp1C4;
if((unsigned int)_tmp19E->defn)
Cyc_scan_type((void*)_check_null(_tmp19E->defn),dep);
goto _LL15D;}_LL168: {struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*_tmp19F=(struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp194;if(_tmp19F->tag != 4)goto _LL16A;}_LL169:
# 1091
{const char*_tmp57A;void*_tmp579;(_tmp579=0,Cyc_fprintf(Cyc_stderr,((_tmp57A="Error: unexpected region declaration",_tag_dyneither(_tmp57A,sizeof(char),37))),_tag_dyneither(_tmp579,sizeof(void*),0)));}
exit(1);_LL16A: {struct Cyc_Absyn_Alias_d_Absyn_Raw_decl_struct*_tmp1A0=(struct Cyc_Absyn_Alias_d_Absyn_Raw_decl_struct*)_tmp194;if(_tmp1A0->tag != 5)goto _LL16C;}_LL16B:
# 1094
{const char*_tmp57D;void*_tmp57C;(_tmp57C=0,Cyc_fprintf(Cyc_stderr,((_tmp57D="Error: unexpected alias declaration",_tag_dyneither(_tmp57D,sizeof(char),36))),_tag_dyneither(_tmp57C,sizeof(void*),0)));}
exit(1);_LL16C: {struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct*_tmp1A1=(struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct*)_tmp194;if(_tmp1A1->tag != 14)goto _LL16E;}_LL16D:
# 1097
{const char*_tmp580;void*_tmp57F;(_tmp57F=0,Cyc_fprintf(Cyc_stderr,((_tmp580="Error: unexpected __cyclone_port_on__",_tag_dyneither(_tmp580,sizeof(char),38))),_tag_dyneither(_tmp57F,sizeof(void*),0)));}
exit(1);_LL16E: {struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct*_tmp1A2=(struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct*)_tmp194;if(_tmp1A2->tag != 15)goto _LL170;}_LL16F:
# 1100
{const char*_tmp583;void*_tmp582;(_tmp582=0,Cyc_fprintf(Cyc_stderr,((_tmp583="Error: unexpected __cyclone_port_off__",_tag_dyneither(_tmp583,sizeof(char),39))),_tag_dyneither(_tmp582,sizeof(void*),0)));}
exit(1);_LL170: {struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*_tmp1A3=(struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmp194;if(_tmp1A3->tag != 2)goto _LL172;}_LL171:
# 1103
{const char*_tmp586;void*_tmp585;(_tmp585=0,Cyc_fprintf(Cyc_stderr,((_tmp586="Error: unexpected let declaration\n",_tag_dyneither(_tmp586,sizeof(char),35))),_tag_dyneither(_tmp585,sizeof(void*),0)));}
exit(1);_LL172: {struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*_tmp1A4=(struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*)_tmp194;if(_tmp1A4->tag != 7)goto _LL174;}_LL173:
# 1106
{const char*_tmp589;void*_tmp588;(_tmp588=0,Cyc_fprintf(Cyc_stderr,((_tmp589="Error: unexpected datatype declaration\n",_tag_dyneither(_tmp589,sizeof(char),40))),_tag_dyneither(_tmp588,sizeof(void*),0)));}
exit(1);_LL174: {struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct*_tmp1A5=(struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct*)_tmp194;if(_tmp1A5->tag != 3)goto _LL176;}_LL175:
# 1109
{const char*_tmp58C;void*_tmp58B;(_tmp58B=0,Cyc_fprintf(Cyc_stderr,((_tmp58C="Error: unexpected let declaration\n",_tag_dyneither(_tmp58C,sizeof(char),35))),_tag_dyneither(_tmp58B,sizeof(void*),0)));}
exit(1);_LL176: {struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*_tmp1A6=(struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp194;if(_tmp1A6->tag != 10)goto _LL178;}_LL177:
# 1112
{const char*_tmp58F;void*_tmp58E;(_tmp58E=0,Cyc_fprintf(Cyc_stderr,((_tmp58F="Error: unexpected namespace declaration\n",_tag_dyneither(_tmp58F,sizeof(char),41))),_tag_dyneither(_tmp58E,sizeof(void*),0)));}
exit(1);_LL178: {struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*_tmp1A7=(struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmp194;if(_tmp1A7->tag != 11)goto _LL17A;}_LL179:
# 1115
{const char*_tmp592;void*_tmp591;(_tmp591=0,Cyc_fprintf(Cyc_stderr,((_tmp592="Error: unexpected using declaration\n",_tag_dyneither(_tmp592,sizeof(char),37))),_tag_dyneither(_tmp591,sizeof(void*),0)));}
exit(1);_LL17A: {struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*_tmp1A8=(struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*)_tmp194;if(_tmp1A8->tag != 12)goto _LL17C;}_LL17B:
# 1118
{const char*_tmp595;void*_tmp594;(_tmp594=0,Cyc_fprintf(Cyc_stderr,((_tmp595="Error: unexpected extern \"C\" declaration\n",_tag_dyneither(_tmp595,sizeof(char),42))),_tag_dyneither(_tmp594,sizeof(void*),0)));}
exit(1);_LL17C: {struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*_tmp1A9=(struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp194;if(_tmp1A9->tag != 13)goto _LL15D;}_LL17D:
# 1121
{const char*_tmp598;void*_tmp597;(_tmp597=0,Cyc_fprintf(Cyc_stderr,((_tmp598="Error: unexpected extern \"C include\" declaration\n",_tag_dyneither(_tmp598,sizeof(char),50))),_tag_dyneither(_tmp597,sizeof(void*),0)));}
exit(1);_LL15D:;}{
# 1129
struct Cyc_Set_Set*old;
struct _dyneither_ptr*_tmp1DB=(struct _dyneither_ptr*)_check_null(Cyc_current_source);
{struct _handler_cons _tmp1DC;_push_handler(& _tmp1DC);{int _tmp1DE=0;if(setjmp(_tmp1DC.handler))_tmp1DE=1;if(!_tmp1DE){
old=((struct Cyc_Set_Set*(*)(struct Cyc_Hashtable_Table*t,struct _dyneither_ptr*key))Cyc_Hashtable_lookup)(dep,_tmp1DB);;_pop_handler();}else{void*_tmp1DD=(void*)_exn_thrown;void*_tmp1E0=_tmp1DD;void*_tmp1E2;_LL17F: {struct Cyc_Core_Not_found_exn_struct*_tmp1E1=(struct Cyc_Core_Not_found_exn_struct*)_tmp1E0;if(_tmp1E1->tag != Cyc_Core_Not_found)goto _LL181;}_LL180:
# 1134
 old=((struct Cyc_Set_Set*(*)(int(*cmp)(struct _dyneither_ptr*,struct _dyneither_ptr*)))Cyc_Set_empty)(Cyc_strptrcmp);goto _LL17E;_LL181: _tmp1E2=_tmp1E0;_LL182:(void)_throw(_tmp1E2);_LL17E:;}};}{
# 1136
struct Cyc_Set_Set*_tmp1E3=((struct Cyc_Set_Set*(*)(struct Cyc_Set_Set*s1,struct Cyc_Set_Set*s2))Cyc_Set_union_two)(*((struct Cyc_Set_Set**)_check_null(Cyc_current_targets)),old);
((void(*)(struct Cyc_Hashtable_Table*t,struct _dyneither_ptr*key,struct Cyc_Set_Set*val))Cyc_Hashtable_insert)(dep,_tmp1DB,_tmp1E3);
# 1139
Cyc_current_targets=_tmp190;
Cyc_current_source=_tmp191;};};}
# 1143
struct Cyc_Hashtable_Table*Cyc_new_deps(){
return((struct Cyc_Hashtable_Table*(*)(int sz,int(*cmp)(struct _dyneither_ptr*,struct _dyneither_ptr*),int(*hash)(struct _dyneither_ptr*)))Cyc_Hashtable_create)(107,Cyc_strptrcmp,Cyc_Hashtable_hash_stringptr);}
# 1147
struct Cyc_Set_Set*Cyc_find(struct Cyc_Hashtable_Table*t,struct _dyneither_ptr*x){
struct _handler_cons _tmp1E4;_push_handler(& _tmp1E4);{int _tmp1E6=0;if(setjmp(_tmp1E4.handler))_tmp1E6=1;if(!_tmp1E6){{struct Cyc_Set_Set*_tmp1E7=((struct Cyc_Set_Set*(*)(struct Cyc_Hashtable_Table*t,struct _dyneither_ptr*key))Cyc_Hashtable_lookup)(t,x);_npop_handler(0);return _tmp1E7;};_pop_handler();}else{void*_tmp1E5=(void*)_exn_thrown;void*_tmp1E9=_tmp1E5;void*_tmp1EB;_LL184: {struct Cyc_Core_Not_found_exn_struct*_tmp1EA=(struct Cyc_Core_Not_found_exn_struct*)_tmp1E9;if(_tmp1EA->tag != Cyc_Core_Not_found)goto _LL186;}_LL185:
# 1150
 return((struct Cyc_Set_Set*(*)(int(*cmp)(struct _dyneither_ptr*,struct _dyneither_ptr*)))Cyc_Set_empty)(Cyc_strptrcmp);_LL186: _tmp1EB=_tmp1E9;_LL187:(void)_throw(_tmp1EB);_LL183:;}};}
# 1154
struct Cyc_Set_Set*Cyc_reachable(struct Cyc_List_List*init,struct Cyc_Hashtable_Table*t){
# 1165 "buildlib.cyl"
struct Cyc_Set_Set*emptyset=((struct Cyc_Set_Set*(*)(int(*cmp)(struct _dyneither_ptr*,struct _dyneither_ptr*)))Cyc_Set_empty)(Cyc_strptrcmp);
struct Cyc_Set_Set*curr;
for(curr=emptyset;init != 0;init=init->tl){
curr=((struct Cyc_Set_Set*(*)(struct Cyc_Set_Set*s,struct _dyneither_ptr*elt))Cyc_Set_insert)(curr,(struct _dyneither_ptr*)init->hd);}{
struct Cyc_Set_Set*_tmp1EC=curr;
# 1171
struct _dyneither_ptr*_tmp1ED=_init_dyneither_ptr(_cycalloc(sizeof(struct _dyneither_ptr)),"",sizeof(char),1);
while(((int(*)(struct Cyc_Set_Set*s))Cyc_Set_cardinality)(_tmp1EC)> 0){
struct Cyc_Set_Set*_tmp1EE=emptyset;
struct Cyc_Iter_Iter _tmp1EF=((struct Cyc_Iter_Iter(*)(struct _RegionHandle*rgn,struct Cyc_Set_Set*s))Cyc_Set_make_iter)(Cyc_Core_heap_region,_tmp1EC);
while(((int(*)(struct Cyc_Iter_Iter,struct _dyneither_ptr**))Cyc_Iter_next)(_tmp1EF,& _tmp1ED)){
_tmp1EE=((struct Cyc_Set_Set*(*)(struct Cyc_Set_Set*s1,struct Cyc_Set_Set*s2))Cyc_Set_union_two)(_tmp1EE,Cyc_find(t,_tmp1ED));}
_tmp1EC=((struct Cyc_Set_Set*(*)(struct Cyc_Set_Set*s1,struct Cyc_Set_Set*s2))Cyc_Set_diff)(_tmp1EE,curr);
curr=((struct Cyc_Set_Set*(*)(struct Cyc_Set_Set*s1,struct Cyc_Set_Set*s2))Cyc_Set_union_two)(curr,_tmp1EC);}
# 1180
return curr;};}
# 1183
enum Cyc_buildlib_mode{Cyc_NORMAL  = 0,Cyc_GATHER  = 1,Cyc_GATHERSCRIPT  = 2,Cyc_FINISH  = 3};
static enum Cyc_buildlib_mode Cyc_mode=Cyc_NORMAL;
static int Cyc_gathering(){
return Cyc_mode == Cyc_GATHER  || Cyc_mode == Cyc_GATHERSCRIPT;}
# 1189
static struct Cyc___cycFILE*Cyc_script_file=0;
int Cyc_prscript(struct _dyneither_ptr fmt,struct _dyneither_ptr ap){
# 1193
if(Cyc_script_file == 0){
{const char*_tmp59B;void*_tmp59A;(_tmp59A=0,Cyc_fprintf(Cyc_stderr,((_tmp59B="Internal error: script file is NULL\n",_tag_dyneither(_tmp59B,sizeof(char),37))),_tag_dyneither(_tmp59A,sizeof(void*),0)));}
exit(1);}
# 1197
return Cyc_vfprintf((struct Cyc___cycFILE*)_check_null(Cyc_script_file),fmt,ap);}
# 1200
int Cyc_force_directory(struct _dyneither_ptr d){
if(Cyc_mode == Cyc_GATHERSCRIPT){
const char*_tmp5A0;void*_tmp59F[2];struct Cyc_String_pa_PrintArg_struct _tmp59E;struct Cyc_String_pa_PrintArg_struct _tmp59D;(_tmp59D.tag=0,((_tmp59D.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)d),((_tmp59E.tag=0,((_tmp59E.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)d),((_tmp59F[0]=& _tmp59E,((_tmp59F[1]=& _tmp59D,Cyc_prscript(((_tmp5A0="if ! test -e %s; then mkdir %s; fi\n",_tag_dyneither(_tmp5A0,sizeof(char),36))),_tag_dyneither(_tmp59F,sizeof(void*),2)))))))))))));}else{
# 1207
unsigned short _tmp5A1[0];int _tmp1F7=Cyc_open((const char*)_check_null(_untag_dyneither_ptr(d,sizeof(char),1)),0,_tag_dyneither(_tmp5A1,sizeof(unsigned short),0));
if(_tmp1F7 == - 1){
if(mkdir((const char*)_untag_dyneither_ptr(d,sizeof(char),1),448)== - 1){
{const char*_tmp5A5;void*_tmp5A4[1];struct Cyc_String_pa_PrintArg_struct _tmp5A3;(_tmp5A3.tag=0,((_tmp5A3.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)d),((_tmp5A4[0]=& _tmp5A3,Cyc_fprintf(Cyc_stderr,((_tmp5A5="Error: could not create directory %s\n",_tag_dyneither(_tmp5A5,sizeof(char),38))),_tag_dyneither(_tmp5A4,sizeof(void*),1)))))));}
return 1;}}else{
# 1214
close(_tmp1F7);}}
# 1216
return 0;}
# 1219
int Cyc_force_directory_prefixes(struct _dyneither_ptr file){
# 1223
struct _dyneither_ptr _tmp1FC=Cyc_strdup((struct _dyneither_ptr)file);
# 1225
struct Cyc_List_List*_tmp1FD=0;
while(1){
_tmp1FC=Cyc_Filename_dirname((struct _dyneither_ptr)_tmp1FC);
if(Cyc_strlen((struct _dyneither_ptr)_tmp1FC)== 0)break;{
struct _dyneither_ptr*_tmp5A8;struct Cyc_List_List*_tmp5A7;_tmp1FD=((_tmp5A7=_cycalloc(sizeof(*_tmp5A7)),((_tmp5A7->hd=((_tmp5A8=_cycalloc(sizeof(*_tmp5A8)),((_tmp5A8[0]=(struct _dyneither_ptr)_tmp1FC,_tmp5A8)))),((_tmp5A7->tl=_tmp1FD,_tmp5A7))))));};}
# 1232
for(0;_tmp1FD != 0;_tmp1FD=_tmp1FD->tl){
if(Cyc_force_directory(*((struct _dyneither_ptr*)_tmp1FD->hd)))return 1;}
# 1235
return 0;}char Cyc_NO_SUPPORT[11]="NO_SUPPORT";struct Cyc_NO_SUPPORT_exn_struct{char*tag;struct _dyneither_ptr f1;};
# 1242
static int Cyc_is_other_special(char c){
switch(c){case '\\': _LL188:
 goto _LL189;case '"': _LL189:
 goto _LL18A;case ';': _LL18A:
 goto _LL18B;case '&': _LL18B:
 goto _LL18C;case '(': _LL18C:
 goto _LL18D;case ')': _LL18D:
 goto _LL18E;case '|': _LL18E:
 goto _LL18F;case '^': _LL18F:
 goto _LL190;case '<': _LL190:
 goto _LL191;case '>': _LL191:
 goto _LL192;case '\n': _LL192:
# 1257
 goto _LL193;case '\t': _LL193:
 return 1;default: _LL194:
 return 0;}}static struct _dyneither_ptr Cyc_sh_escape_string(struct _dyneither_ptr s);static void _tmp5B1(unsigned int*_tmp5B0,unsigned int*_tmp5AF,char**_tmp5AD){for(*_tmp5B0=0;*_tmp5B0 < *_tmp5AF;(*_tmp5B0)++){(*_tmp5AD)[*_tmp5B0]=(char)'\000';}}
# 1263
static struct _dyneither_ptr Cyc_sh_escape_string(struct _dyneither_ptr s){
unsigned long _tmp200=Cyc_strlen((struct _dyneither_ptr)s);
# 1267
int _tmp201=0;
int _tmp202=0;
{int i=0;for(0;i < _tmp200;++ i){
char _tmp203=*((const char*)_check_dyneither_subscript(s,sizeof(char),i));
if(_tmp203 == '\'')++ _tmp201;else{
if(Cyc_is_other_special(_tmp203))++ _tmp202;}}}
# 1276
if(_tmp201 == 0  && _tmp202 == 0)
return s;
# 1280
if(_tmp201 == 0){
struct _dyneither_ptr*_tmp5AB;struct _dyneither_ptr*_tmp5AA[3];return(struct _dyneither_ptr)Cyc_strconcat_l(((_tmp5AA[2]=_init_dyneither_ptr(_cycalloc(sizeof(struct _dyneither_ptr)),"'",sizeof(char),2),((_tmp5AA[1]=((_tmp5AB=_cycalloc(sizeof(*_tmp5AB)),((_tmp5AB[0]=(struct _dyneither_ptr)s,_tmp5AB)))),((_tmp5AA[0]=_init_dyneither_ptr(_cycalloc(sizeof(struct _dyneither_ptr)),"'",sizeof(char),2),((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp5AA,sizeof(struct _dyneither_ptr*),3)))))))));}{
# 1284
unsigned long _tmp208=(_tmp200 + _tmp201)+ _tmp202;
unsigned int _tmp5B0;unsigned int _tmp5AF;struct _dyneither_ptr _tmp5AE;char*_tmp5AD;unsigned int _tmp5AC;struct _dyneither_ptr s2=(_tmp5AC=_tmp208 + 1,((_tmp5AD=(char*)_cycalloc_atomic(_check_times(sizeof(char),_tmp5AC + 1)),((_tmp5AE=_tag_dyneither(_tmp5AD,sizeof(char),_tmp5AC + 1),((((_tmp5AF=_tmp5AC,((_tmp5B1(& _tmp5B0,& _tmp5AF,& _tmp5AD),_tmp5AD[_tmp5AF]=(char)0)))),_tmp5AE)))))));
int _tmp209=0;
int _tmp20A=0;
for(0;_tmp209 < _tmp200;++ _tmp209){
char _tmp20B=*((const char*)_check_dyneither_subscript(s,sizeof(char),_tmp209));
if(_tmp20B == '\''  || Cyc_is_other_special(_tmp20B)){
char _tmp5B4;char _tmp5B3;struct _dyneither_ptr _tmp5B2;(_tmp5B2=_dyneither_ptr_plus(s2,sizeof(char),_tmp20A ++),((_tmp5B3=*((char*)_check_dyneither_subscript(_tmp5B2,sizeof(char),0)),((_tmp5B4='\\',((_get_dyneither_size(_tmp5B2,sizeof(char))== 1  && (_tmp5B3 == '\000'  && _tmp5B4 != '\000')?_throw_arraybounds(): 1,*((char*)_tmp5B2.curr)=_tmp5B4)))))));}{
char _tmp5B7;char _tmp5B6;struct _dyneither_ptr _tmp5B5;(_tmp5B5=_dyneither_ptr_plus(s2,sizeof(char),_tmp20A ++),((_tmp5B6=*((char*)_check_dyneither_subscript(_tmp5B5,sizeof(char),0)),((_tmp5B7=_tmp20B,((_get_dyneither_size(_tmp5B5,sizeof(char))== 1  && (_tmp5B6 == '\000'  && _tmp5B7 != '\000')?_throw_arraybounds(): 1,*((char*)_tmp5B5.curr)=_tmp5B7)))))));};}
# 1294
return(struct _dyneither_ptr)s2;};}
# 1296
static struct _dyneither_ptr*Cyc_sh_escape_stringptr(struct _dyneither_ptr*sp){
struct _dyneither_ptr*_tmp5B8;return(_tmp5B8=_cycalloc(sizeof(*_tmp5B8)),((_tmp5B8[0]=Cyc_sh_escape_string(*sp),_tmp5B8)));}struct _tuple25{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};
# 1301
int Cyc_process_file(const char*filename,struct Cyc_List_List*start_symbols,struct Cyc_List_List*omit_symbols,struct Cyc_List_List*hstubs,struct Cyc_List_List*cstubs,struct Cyc_List_List*cycstubs){
# 1307
struct Cyc___cycFILE*maybe;
struct Cyc___cycFILE*in_file;
struct Cyc___cycFILE*out_file;
int errorcode=0;
# 1312
{const char*_tmp5BE;void*_tmp5BD[1];const char*_tmp5BC;struct Cyc_String_pa_PrintArg_struct _tmp5BB;(_tmp5BB.tag=0,((_tmp5BB.f1=(struct _dyneither_ptr)(
(_tmp5BC=filename,_tag_dyneither(_tmp5BC,sizeof(char),_get_zero_arr_size_char((void*)_tmp5BC,1)))),((_tmp5BD[0]=& _tmp5BB,Cyc_fprintf(Cyc_stderr,((_tmp5BE="********************************* %s...\n",_tag_dyneither(_tmp5BE,sizeof(char),41))),_tag_dyneither(_tmp5BD,sizeof(void*),1)))))));}
# 1315
if(!Cyc_gathering()){const char*_tmp5C4;void*_tmp5C3[1];const char*_tmp5C2;struct Cyc_String_pa_PrintArg_struct _tmp5C1;(_tmp5C1.tag=0,((_tmp5C1.f1=(struct _dyneither_ptr)((_tmp5C2=filename,_tag_dyneither(_tmp5C2,sizeof(char),_get_zero_arr_size_char((void*)_tmp5C2,1)))),((_tmp5C3[0]=& _tmp5C1,Cyc_log(((_tmp5C4="\n%s:\n",_tag_dyneither(_tmp5C4,sizeof(char),6))),_tag_dyneither(_tmp5C3,sizeof(void*),1)))))));}{
# 1327 "buildlib.cyl"
const char*_tmp5C5;struct _dyneither_ptr _tmp21F=Cyc_Filename_basename(((_tmp5C5=filename,_tag_dyneither(_tmp5C5,sizeof(char),_get_zero_arr_size_char((void*)_tmp5C5,1)))));
const char*_tmp5C6;struct _dyneither_ptr _tmp220=Cyc_Filename_dirname(((_tmp5C6=filename,_tag_dyneither(_tmp5C6,sizeof(char),_get_zero_arr_size_char((void*)_tmp5C6,1)))));
struct _dyneither_ptr _tmp221=Cyc_Filename_chop_extension((struct _dyneither_ptr)_tmp21F);
const char*_tmp5C7;const char*_tmp222=(const char*)_untag_dyneither_ptr(Cyc_strconcat((struct _dyneither_ptr)_tmp221,((_tmp5C7=".iA",_tag_dyneither(_tmp5C7,sizeof(char),4)))),sizeof(char),1);
const char*_tmp5CF;void*_tmp5CE[1];struct Cyc_String_pa_PrintArg_struct _tmp5CD;const char*_tmp5CB;void*_tmp5CA[1];struct Cyc_String_pa_PrintArg_struct _tmp5C9;const char*_tmp223=(const char*)_untag_dyneither_ptr(_get_dyneither_size(_tmp220,sizeof(char))== 0?(_tmp5CD.tag=0,((_tmp5CD.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)_tmp221),((_tmp5CE[0]=& _tmp5CD,Cyc_aprintf(((_tmp5CF="%s.iB",_tag_dyneither(_tmp5CF,sizeof(char),6))),_tag_dyneither(_tmp5CE,sizeof(void*),1))))))): Cyc_Filename_concat((struct _dyneither_ptr)_tmp220,(struct _dyneither_ptr)((_tmp5C9.tag=0,((_tmp5C9.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)_tmp221),((_tmp5CA[0]=& _tmp5C9,Cyc_aprintf(((_tmp5CB="%s.iB",_tag_dyneither(_tmp5CB,sizeof(char),6))),_tag_dyneither(_tmp5CA,sizeof(void*),1))))))))),sizeof(char),1);
# 1335
const char*_tmp5D7;void*_tmp5D6[1];struct Cyc_String_pa_PrintArg_struct _tmp5D5;const char*_tmp5D3;void*_tmp5D2[1];struct Cyc_String_pa_PrintArg_struct _tmp5D1;const char*_tmp224=(const char*)_untag_dyneither_ptr(_get_dyneither_size(_tmp220,sizeof(char))== 0?(_tmp5D5.tag=0,((_tmp5D5.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)_tmp221),((_tmp5D6[0]=& _tmp5D5,Cyc_aprintf(((_tmp5D7="%s.iC",_tag_dyneither(_tmp5D7,sizeof(char),6))),_tag_dyneither(_tmp5D6,sizeof(void*),1))))))): Cyc_Filename_concat((struct _dyneither_ptr)_tmp220,(struct _dyneither_ptr)((_tmp5D1.tag=0,((_tmp5D1.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)_tmp221),((_tmp5D2[0]=& _tmp5D1,Cyc_aprintf(((_tmp5D3="%s.iC",_tag_dyneither(_tmp5D3,sizeof(char),6))),_tag_dyneither(_tmp5D2,sizeof(void*),1))))))))),sizeof(char),1);
# 1339
const char*_tmp5DF;void*_tmp5DE[1];struct Cyc_String_pa_PrintArg_struct _tmp5DD;const char*_tmp5DB;void*_tmp5DA[1];struct Cyc_String_pa_PrintArg_struct _tmp5D9;const char*_tmp225=(const char*)_untag_dyneither_ptr(_get_dyneither_size(_tmp220,sizeof(char))== 0?(_tmp5DD.tag=0,((_tmp5DD.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)_tmp221),((_tmp5DE[0]=& _tmp5DD,Cyc_aprintf(((_tmp5DF="%s.iD",_tag_dyneither(_tmp5DF,sizeof(char),6))),_tag_dyneither(_tmp5DE,sizeof(void*),1))))))): Cyc_Filename_concat((struct _dyneither_ptr)_tmp220,(struct _dyneither_ptr)((_tmp5D9.tag=0,((_tmp5D9.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)_tmp221),((_tmp5DA[0]=& _tmp5D9,Cyc_aprintf(((_tmp5DB="%s.iD",_tag_dyneither(_tmp5DB,sizeof(char),6))),_tag_dyneither(_tmp5DA,sizeof(void*),1))))))))),sizeof(char),1);
# 1344
{struct _handler_cons _tmp226;_push_handler(& _tmp226);{int _tmp228=0;if(setjmp(_tmp226.handler))_tmp228=1;if(!_tmp228){
# 1347
{const char*_tmp5E0;if(Cyc_force_directory_prefixes(((_tmp5E0=filename,_tag_dyneither(_tmp5E0,sizeof(char),_get_zero_arr_size_char((void*)_tmp5E0,1)))))){
int _tmp22A=1;_npop_handler(0);return _tmp22A;}}
# 1352
if(Cyc_mode != Cyc_FINISH){
Cyc_current_cpp=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_current_cpp);
if(Cyc_mode == Cyc_GATHERSCRIPT){
{const char*_tmp5E6;void*_tmp5E5[1];const char*_tmp5E4;struct Cyc_String_pa_PrintArg_struct _tmp5E3;(_tmp5E3.tag=0,((_tmp5E3.f1=(struct _dyneither_ptr)((_tmp5E4=_tmp222,_tag_dyneither(_tmp5E4,sizeof(char),_get_zero_arr_size_char((void*)_tmp5E4,1)))),((_tmp5E5[0]=& _tmp5E3,Cyc_prscript(((_tmp5E6="cat >%s <<XXX\n",_tag_dyneither(_tmp5E6,sizeof(char),15))),_tag_dyneither(_tmp5E5,sizeof(void*),1)))))));}
{struct Cyc_List_List*_tmp22F=Cyc_current_cpp;for(0;_tmp22F != 0;_tmp22F=_tmp22F->tl){
const char*_tmp5EA;void*_tmp5E9[1];struct Cyc_String_pa_PrintArg_struct _tmp5E8;(_tmp5E8.tag=0,((_tmp5E8.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*((struct _dyneither_ptr*)_tmp22F->hd)),((_tmp5E9[0]=& _tmp5E8,Cyc_prscript(((_tmp5EA="%s",_tag_dyneither(_tmp5EA,sizeof(char),3))),_tag_dyneither(_tmp5E9,sizeof(void*),1)))))));}}
{const char*_tmp5F0;void*_tmp5EF[1];const char*_tmp5EE;struct Cyc_String_pa_PrintArg_struct _tmp5ED;(_tmp5ED.tag=0,((_tmp5ED.f1=(struct _dyneither_ptr)((_tmp5EE=filename,_tag_dyneither(_tmp5EE,sizeof(char),_get_zero_arr_size_char((void*)_tmp5EE,1)))),((_tmp5EF[0]=& _tmp5ED,Cyc_prscript(((_tmp5F0="#include <%s>\n",_tag_dyneither(_tmp5F0,sizeof(char),15))),_tag_dyneither(_tmp5EF,sizeof(void*),1)))))));}
{const char*_tmp5F3;void*_tmp5F2;(_tmp5F2=0,Cyc_prscript(((_tmp5F3="XXX\n",_tag_dyneither(_tmp5F3,sizeof(char),5))),_tag_dyneither(_tmp5F2,sizeof(void*),0)));}
{const char*_tmp5FC;void*_tmp5FB[2];const char*_tmp5FA;struct Cyc_String_pa_PrintArg_struct _tmp5F9;const char*_tmp5F8;struct Cyc_String_pa_PrintArg_struct _tmp5F7;(_tmp5F7.tag=0,((_tmp5F7.f1=(struct _dyneither_ptr)((_tmp5F8=_tmp222,_tag_dyneither(_tmp5F8,sizeof(char),_get_zero_arr_size_char((void*)_tmp5F8,1)))),((_tmp5F9.tag=0,((_tmp5F9.f1=(struct _dyneither_ptr)((_tmp5FA=_tmp223,_tag_dyneither(_tmp5FA,sizeof(char),_get_zero_arr_size_char((void*)_tmp5FA,1)))),((_tmp5FB[0]=& _tmp5F9,((_tmp5FB[1]=& _tmp5F7,Cyc_prscript(((_tmp5FC="$GCC -E -dM -o %s -x c %s && \\\n",_tag_dyneither(_tmp5FC,sizeof(char),32))),_tag_dyneither(_tmp5FB,sizeof(void*),2)))))))))))));}
{const char*_tmp605;void*_tmp604[2];const char*_tmp603;struct Cyc_String_pa_PrintArg_struct _tmp602;const char*_tmp601;struct Cyc_String_pa_PrintArg_struct _tmp600;(_tmp600.tag=0,((_tmp600.f1=(struct _dyneither_ptr)((_tmp601=_tmp222,_tag_dyneither(_tmp601,sizeof(char),_get_zero_arr_size_char((void*)_tmp601,1)))),((_tmp602.tag=0,((_tmp602.f1=(struct _dyneither_ptr)((_tmp603=_tmp224,_tag_dyneither(_tmp603,sizeof(char),_get_zero_arr_size_char((void*)_tmp603,1)))),((_tmp604[0]=& _tmp602,((_tmp604[1]=& _tmp600,Cyc_prscript(((_tmp605="$GCC -E     -o %s -x c %s;\n",_tag_dyneither(_tmp605,sizeof(char),28))),_tag_dyneither(_tmp604,sizeof(void*),2)))))))))))));}{
const char*_tmp60B;void*_tmp60A[1];const char*_tmp609;struct Cyc_String_pa_PrintArg_struct _tmp608;(_tmp608.tag=0,((_tmp608.f1=(struct _dyneither_ptr)((_tmp609=_tmp222,_tag_dyneither(_tmp609,sizeof(char),_get_zero_arr_size_char((void*)_tmp609,1)))),((_tmp60A[0]=& _tmp608,Cyc_prscript(((_tmp60B="rm %s\n",_tag_dyneither(_tmp60B,sizeof(char),7))),_tag_dyneither(_tmp60A,sizeof(void*),1)))))));};}else{
# 1365
maybe=Cyc_fopen(_tmp222,"w");
if(!((unsigned int)maybe)){
{const char*_tmp611;void*_tmp610[1];const char*_tmp60F;struct Cyc_String_pa_PrintArg_struct _tmp60E;(_tmp60E.tag=0,((_tmp60E.f1=(struct _dyneither_ptr)((_tmp60F=_tmp222,_tag_dyneither(_tmp60F,sizeof(char),_get_zero_arr_size_char((void*)_tmp60F,1)))),((_tmp610[0]=& _tmp60E,Cyc_fprintf(Cyc_stderr,((_tmp611="Error: could not create file %s\n",_tag_dyneither(_tmp611,sizeof(char),33))),_tag_dyneither(_tmp610,sizeof(void*),1)))))));}{
int _tmp24D=1;_npop_handler(0);return _tmp24D;};}
# 1370
out_file=maybe;
{struct Cyc_List_List*_tmp24E=Cyc_current_cpp;for(0;_tmp24E != 0;_tmp24E=_tmp24E->tl){
Cyc_fputs((const char*)_untag_dyneither_ptr(*((struct _dyneither_ptr*)_tmp24E->hd),sizeof(char),1),out_file);}}
{const char*_tmp617;void*_tmp616[1];const char*_tmp615;struct Cyc_String_pa_PrintArg_struct _tmp614;(_tmp614.tag=0,((_tmp614.f1=(struct _dyneither_ptr)((_tmp615=filename,_tag_dyneither(_tmp615,sizeof(char),_get_zero_arr_size_char((void*)_tmp615,1)))),((_tmp616[0]=& _tmp614,Cyc_fprintf(out_file,((_tmp617="#include <%s>\n",_tag_dyneither(_tmp617,sizeof(char),15))),_tag_dyneither(_tmp616,sizeof(void*),1)))))));}
Cyc_fclose(out_file);{
struct _dyneither_ptr _tmp253=Cstring_to_string(Ccomp);
const char*_tmp61E;struct _dyneither_ptr*_tmp61D;const char*_tmp61C;struct Cyc_List_List*_tmp61B;struct _dyneither_ptr _tmp254=
Cyc_str_sepstr(((_tmp61B=_cycalloc(sizeof(*_tmp61B)),((_tmp61B->hd=((_tmp61D=_cycalloc(sizeof(*_tmp61D)),((_tmp61D[0]=(struct _dyneither_ptr)((_tmp61C="",_tag_dyneither(_tmp61C,sizeof(char),1))),_tmp61D)))),((_tmp61B->tl=
((struct Cyc_List_List*(*)(struct _dyneither_ptr*(*f)(struct _dyneither_ptr*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_sh_escape_stringptr,((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_rev)(Cyc_cppargs)),_tmp61B)))))),(
(_tmp61E=" ",_tag_dyneither(_tmp61E,sizeof(char),2))));
const char*_tmp629;void*_tmp628[4];struct Cyc_String_pa_PrintArg_struct _tmp627;const char*_tmp626;struct Cyc_String_pa_PrintArg_struct _tmp625;struct Cyc_String_pa_PrintArg_struct _tmp624;const char*_tmp623;struct Cyc_String_pa_PrintArg_struct _tmp622;char*cmd=(char*)_untag_dyneither_ptr(((_tmp622.tag=0,((_tmp622.f1=(struct _dyneither_ptr)((_tmp623=_tmp222,_tag_dyneither(_tmp623,sizeof(char),_get_zero_arr_size_char((void*)_tmp623,1)))),((_tmp624.tag=0,((_tmp624.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)_tmp254),((_tmp625.tag=0,((_tmp625.f1=(struct _dyneither_ptr)((_tmp626=_tmp223,_tag_dyneither(_tmp626,sizeof(char),_get_zero_arr_size_char((void*)_tmp626,1)))),((_tmp627.tag=0,((_tmp627.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)_tmp253),((_tmp628[0]=& _tmp627,((_tmp628[1]=& _tmp625,((_tmp628[2]=& _tmp624,((_tmp628[3]=& _tmp622,Cyc_aprintf(((_tmp629="%s -E -dM -o %s -x c %s %s",_tag_dyneither(_tmp629,sizeof(char),27))),_tag_dyneither(_tmp628,sizeof(void*),4)))))))))))))))))))))))))),sizeof(char),1);
# 1382
if(!system((const char*)cmd)){
# 1385
{const char*_tmp634;void*_tmp633[4];struct Cyc_String_pa_PrintArg_struct _tmp632;const char*_tmp631;struct Cyc_String_pa_PrintArg_struct _tmp630;struct Cyc_String_pa_PrintArg_struct _tmp62F;const char*_tmp62E;struct Cyc_String_pa_PrintArg_struct _tmp62D;cmd=(char*)_untag_dyneither_ptr(((_tmp62D.tag=0,((_tmp62D.f1=(struct _dyneither_ptr)((_tmp62E=_tmp222,_tag_dyneither(_tmp62E,sizeof(char),_get_zero_arr_size_char((void*)_tmp62E,1)))),((_tmp62F.tag=0,((_tmp62F.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)_tmp254),((_tmp630.tag=0,((_tmp630.f1=(struct _dyneither_ptr)((_tmp631=_tmp224,_tag_dyneither(_tmp631,sizeof(char),_get_zero_arr_size_char((void*)_tmp631,1)))),((_tmp632.tag=0,((_tmp632.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)_tmp253),((_tmp633[0]=& _tmp632,((_tmp633[1]=& _tmp630,((_tmp633[2]=& _tmp62F,((_tmp633[3]=& _tmp62D,Cyc_aprintf(((_tmp634="%s -E -o %s -x c %s %s",_tag_dyneither(_tmp634,sizeof(char),23))),_tag_dyneither(_tmp633,sizeof(void*),4)))))))))))))))))))))))))),sizeof(char),1);}
# 1387
system((const char*)cmd);}};}}
# 1392
if(Cyc_gathering()){int _tmp269=0;_npop_handler(0);return _tmp269;}{
# 1395
struct Cyc_Hashtable_Table*t=Cyc_new_deps();
maybe=Cyc_fopen(_tmp223,"r");
if(!((unsigned int)maybe)){struct Cyc_NO_SUPPORT_exn_struct _tmp645;const char*_tmp644;void*_tmp643[1];const char*_tmp642;struct Cyc_String_pa_PrintArg_struct _tmp641;struct Cyc_NO_SUPPORT_exn_struct*_tmp640;(int)_throw((void*)((_tmp640=_cycalloc(sizeof(*_tmp640)),((_tmp640[0]=((_tmp645.tag=Cyc_NO_SUPPORT,((_tmp645.f1=(struct _dyneither_ptr)((_tmp641.tag=0,((_tmp641.f1=(struct _dyneither_ptr)((_tmp642=_tmp223,_tag_dyneither(_tmp642,sizeof(char),_get_zero_arr_size_char((void*)_tmp642,1)))),((_tmp643[0]=& _tmp641,Cyc_aprintf(((_tmp644="can't open macrosfile %s",_tag_dyneither(_tmp644,sizeof(char),25))),_tag_dyneither(_tmp643,sizeof(void*),1)))))))),_tmp645)))),_tmp640)))));}
# 1400
in_file=maybe;{
struct Cyc_Lexing_lexbuf*_tmp270=Cyc_Lexing_from_file(in_file);
struct _tuple20*entry;
while((entry=((struct _tuple20*(*)(struct Cyc_Lexing_lexbuf*lexbuf))Cyc_line)(_tmp270))!= 0){
struct _tuple20*_tmp271=(struct _tuple20*)_check_null(entry);struct _dyneither_ptr*_tmp273;struct Cyc_Set_Set*_tmp274;struct _tuple20*_tmp272=_tmp271;_tmp273=_tmp272->f1;_tmp274=_tmp272->f2;
((void(*)(struct Cyc_Hashtable_Table*t,struct _dyneither_ptr*key,struct Cyc_Set_Set*val))Cyc_Hashtable_insert)(t,_tmp273,_tmp274);}
# 1409
Cyc_fclose(in_file);
# 1412
maybe=Cyc_fopen(_tmp224,"r");
if(!((unsigned int)maybe)){struct Cyc_NO_SUPPORT_exn_struct _tmp656;const char*_tmp655;void*_tmp654[1];const char*_tmp653;struct Cyc_String_pa_PrintArg_struct _tmp652;struct Cyc_NO_SUPPORT_exn_struct*_tmp651;(int)_throw((void*)((_tmp651=_cycalloc(sizeof(*_tmp651)),((_tmp651[0]=((_tmp656.tag=Cyc_NO_SUPPORT,((_tmp656.f1=(struct _dyneither_ptr)((_tmp652.tag=0,((_tmp652.f1=(struct _dyneither_ptr)((_tmp653=_tmp224,_tag_dyneither(_tmp653,sizeof(char),_get_zero_arr_size_char((void*)_tmp653,1)))),((_tmp654[0]=& _tmp652,Cyc_aprintf(((_tmp655="can't open declsfile %s",_tag_dyneither(_tmp655,sizeof(char),24))),_tag_dyneither(_tmp654,sizeof(void*),1)))))))),_tmp656)))),_tmp651)))));}
# 1416
in_file=maybe;
_tmp270=Cyc_Lexing_from_file(in_file);
Cyc_slurp_out=Cyc_fopen(_tmp225,"w");
if(!((unsigned int)Cyc_slurp_out)){int _tmp27B=1;_npop_handler(0);return _tmp27B;}
while(((int(*)(struct Cyc_Lexing_lexbuf*lexbuf))Cyc_slurp)(_tmp270)){;}
Cyc_fclose(in_file);
Cyc_fclose((struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));
if(Cyc_mode != Cyc_FINISH)
;
# 1426
maybe=Cyc_fopen(_tmp225,"r");
if(!((unsigned int)maybe)){int _tmp27C=1;_npop_handler(0);return _tmp27C;}
in_file=maybe;
{const char*_tmp657;Cyc_Position_reset_position(((_tmp657=_tmp225,_tag_dyneither(_tmp657,sizeof(char),_get_zero_arr_size_char((void*)_tmp657,1)))));}
Cyc_Lex_lex_init(0);{
struct Cyc_List_List*_tmp27E=Cyc_Parse_parse_file(in_file);
Cyc_Lex_lex_init(0);
Cyc_fclose(in_file);
# 1436
{struct Cyc_List_List*_tmp27F=_tmp27E;for(0;_tmp27F != 0;_tmp27F=_tmp27F->tl){
Cyc_scan_decl((struct Cyc_Absyn_Decl*)_tmp27F->hd,t);}}{
# 1440
struct Cyc_Set_Set*_tmp280=Cyc_reachable(start_symbols,t);
# 1443
struct Cyc_List_List*_tmp281=0;
struct Cyc_Set_Set*defined_symbols=((struct Cyc_Set_Set*(*)(int(*cmp)(struct _dyneither_ptr*,struct _dyneither_ptr*)))Cyc_Set_empty)(Cyc_strptrcmp);
{struct Cyc_List_List*_tmp282=_tmp27E;for(0;_tmp282 != 0;_tmp282=_tmp282->tl){
struct Cyc_Absyn_Decl*_tmp283=(struct Cyc_Absyn_Decl*)_tmp282->hd;
struct _dyneither_ptr*name;
{void*_tmp284=_tmp283->r;void*_tmp285=_tmp284;struct Cyc_Absyn_Vardecl*_tmp287;struct Cyc_Absyn_Fndecl*_tmp289;struct Cyc_Absyn_Aggrdecl*_tmp28B;struct Cyc_Absyn_Enumdecl*_tmp28D;struct Cyc_Absyn_Typedefdecl*_tmp28F;_LL197: {struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmp286=(struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp285;if(_tmp286->tag != 0)goto _LL199;else{_tmp287=_tmp286->f1;}}_LL198: {
# 1452
struct _tuple0*_tmp29B=_tmp287->name;struct _dyneither_ptr*_tmp29D;struct _tuple0*_tmp29C=_tmp29B;_tmp29D=_tmp29C->f2;
defined_symbols=((struct Cyc_Set_Set*(*)(struct Cyc_Set_Set*s,struct _dyneither_ptr*elt))Cyc_Set_insert)(defined_symbols,_tmp29D);
if(((int(*)(int(*compare)(struct _dyneither_ptr*,struct _dyneither_ptr*),struct Cyc_List_List*l,struct _dyneither_ptr*x))Cyc_List_mem)(Cyc_strptrcmp,omit_symbols,_tmp29D))name=0;else{
name=_tmp29D;}
goto _LL196;}_LL199: {struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*_tmp288=(struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp285;if(_tmp288->tag != 1)goto _LL19B;else{_tmp289=_tmp288->f1;}}_LL19A: {
# 1458
struct _tuple0*_tmp29E=_tmp289->name;struct _dyneither_ptr*_tmp2A0;struct _tuple0*_tmp29F=_tmp29E;_tmp2A0=_tmp29F->f2;
defined_symbols=((struct Cyc_Set_Set*(*)(struct Cyc_Set_Set*s,struct _dyneither_ptr*elt))Cyc_Set_insert)(defined_symbols,_tmp2A0);
if(((int(*)(int(*compare)(struct _dyneither_ptr*,struct _dyneither_ptr*),struct Cyc_List_List*l,struct _dyneither_ptr*x))Cyc_List_mem)(Cyc_strptrcmp,omit_symbols,_tmp2A0))name=0;else{
name=_tmp2A0;}
goto _LL196;}_LL19B: {struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_tmp28A=(struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_tmp285;if(_tmp28A->tag != 6)goto _LL19D;else{_tmp28B=_tmp28A->f1;}}_LL19C: {
# 1464
struct _tuple0*_tmp2A1=_tmp28B->name;struct _dyneither_ptr*_tmp2A3;struct _tuple0*_tmp2A2=_tmp2A1;_tmp2A3=_tmp2A2->f2;
name=_tmp2A3;
goto _LL196;}_LL19D: {struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*_tmp28C=(struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp285;if(_tmp28C->tag != 8)goto _LL19F;else{_tmp28D=_tmp28C->f1;}}_LL19E: {
# 1468
struct _tuple0*_tmp2A4=_tmp28D->name;struct _dyneither_ptr*_tmp2A6;struct _tuple0*_tmp2A5=_tmp2A4;_tmp2A6=_tmp2A5->f2;
name=_tmp2A6;
# 1472
if(name != 0  && ((int(*)(struct Cyc_Set_Set*s,struct _dyneither_ptr*elt))Cyc_Set_member)(_tmp280,name)){
struct Cyc_List_List*_tmp658;_tmp281=((_tmp658=_cycalloc(sizeof(*_tmp658)),((_tmp658->hd=_tmp283,((_tmp658->tl=_tmp281,_tmp658))))));}else{
# 1475
if((unsigned int)_tmp28D->fields){
struct Cyc_List_List*_tmp2A8=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(_tmp28D->fields))->v;for(0;_tmp2A8 != 0;_tmp2A8=_tmp2A8->tl){
struct Cyc_Absyn_Enumfield*_tmp2A9=(struct Cyc_Absyn_Enumfield*)_tmp2A8->hd;
struct _tuple0*_tmp2AA=_tmp2A9->name;struct _dyneither_ptr*_tmp2AC;struct _tuple0*_tmp2AB=_tmp2AA;_tmp2AC=_tmp2AB->f2;
if(((int(*)(struct Cyc_Set_Set*s,struct _dyneither_ptr*elt))Cyc_Set_member)(_tmp280,_tmp2AC)){
{struct Cyc_List_List*_tmp659;_tmp281=((_tmp659=_cycalloc(sizeof(*_tmp659)),((_tmp659->hd=_tmp283,((_tmp659->tl=_tmp281,_tmp659))))));}
break;}}}}
# 1485
name=0;
goto _LL196;}_LL19F: {struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*_tmp28E=(struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp285;if(_tmp28E->tag != 9)goto _LL1A1;else{_tmp28F=_tmp28E->f1;}}_LL1A0: {
# 1488
struct _tuple0*_tmp2AE=_tmp28F->name;struct _dyneither_ptr*_tmp2B0;struct _tuple0*_tmp2AF=_tmp2AE;_tmp2B0=_tmp2AF->f2;
name=_tmp2B0;
goto _LL196;}_LL1A1: {struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct*_tmp290=(struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct*)_tmp285;if(_tmp290->tag != 14)goto _LL1A3;}_LL1A2:
 goto _LL1A4;_LL1A3: {struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct*_tmp291=(struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct*)_tmp285;if(_tmp291->tag != 15)goto _LL1A5;}_LL1A4:
 goto _LL1A6;_LL1A5: {struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*_tmp292=(struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmp285;if(_tmp292->tag != 2)goto _LL1A7;}_LL1A6:
 goto _LL1A8;_LL1A7: {struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*_tmp293=(struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*)_tmp285;if(_tmp293->tag != 7)goto _LL1A9;}_LL1A8:
 goto _LL1AA;_LL1A9: {struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct*_tmp294=(struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct*)_tmp285;if(_tmp294->tag != 3)goto _LL1AB;}_LL1AA:
 goto _LL1AC;_LL1AB: {struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*_tmp295=(struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp285;if(_tmp295->tag != 10)goto _LL1AD;}_LL1AC:
 goto _LL1AE;_LL1AD: {struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*_tmp296=(struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmp285;if(_tmp296->tag != 11)goto _LL1AF;}_LL1AE:
 goto _LL1B0;_LL1AF: {struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*_tmp297=(struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*)_tmp285;if(_tmp297->tag != 12)goto _LL1B1;}_LL1B0:
 goto _LL1B2;_LL1B1: {struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*_tmp298=(struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp285;if(_tmp298->tag != 13)goto _LL1B3;}_LL1B2:
 goto _LL1B4;_LL1B3: {struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*_tmp299=(struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp285;if(_tmp299->tag != 4)goto _LL1B5;}_LL1B4:
 goto _LL1B6;_LL1B5: {struct Cyc_Absyn_Alias_d_Absyn_Raw_decl_struct*_tmp29A=(struct Cyc_Absyn_Alias_d_Absyn_Raw_decl_struct*)_tmp285;if(_tmp29A->tag != 5)goto _LL196;}_LL1B6:
# 1502
 name=0;
goto _LL196;_LL196:;}
# 1505
if(name != 0  && ((int(*)(struct Cyc_Set_Set*s,struct _dyneither_ptr*elt))Cyc_Set_member)(_tmp280,name)){
struct Cyc_List_List*_tmp65A;_tmp281=((_tmp65A=_cycalloc(sizeof(*_tmp65A)),((_tmp65A->hd=_tmp283,((_tmp65A->tl=_tmp281,_tmp65A))))));}}}
# 1510
if(!Cyc_do_setjmp){
maybe=Cyc_fopen(filename,"w");
if(!((unsigned int)maybe)){int _tmp2B2=1;_npop_handler(0);return _tmp2B2;}
out_file=maybe;}else{
out_file=Cyc_stdout;}{
const char*_tmp660;void*_tmp65F[1];const char*_tmp65E;struct Cyc_String_pa_PrintArg_struct _tmp65D;struct _dyneither_ptr ifdefmacro=(_tmp65D.tag=0,((_tmp65D.f1=(struct _dyneither_ptr)((_tmp65E=filename,_tag_dyneither(_tmp65E,sizeof(char),_get_zero_arr_size_char((void*)_tmp65E,1)))),((_tmp65F[0]=& _tmp65D,Cyc_aprintf(((_tmp660="_%s_",_tag_dyneither(_tmp660,sizeof(char),5))),_tag_dyneither(_tmp65F,sizeof(void*),1)))))));
{int _tmp2B3=0;for(0;_tmp2B3 < _get_dyneither_size(ifdefmacro,sizeof(char));++ _tmp2B3){
if(((char*)ifdefmacro.curr)[_tmp2B3]== '.'  || ((char*)ifdefmacro.curr)[_tmp2B3]== '/'){
char _tmp663;char _tmp662;struct _dyneither_ptr _tmp661;(_tmp661=_dyneither_ptr_plus(ifdefmacro,sizeof(char),_tmp2B3),((_tmp662=*((char*)_check_dyneither_subscript(_tmp661,sizeof(char),0)),((_tmp663='_',((_get_dyneither_size(_tmp661,sizeof(char))== 1  && (_tmp662 == '\000'  && _tmp663 != '\000')?_throw_arraybounds(): 1,*((char*)_tmp661.curr)=_tmp663)))))));}else{
if(((char*)ifdefmacro.curr)[_tmp2B3]!= '_'  && ((char*)ifdefmacro.curr)[_tmp2B3]!= '/'){
char _tmp666;char _tmp665;struct _dyneither_ptr _tmp664;(_tmp664=_dyneither_ptr_plus(ifdefmacro,sizeof(char),_tmp2B3),((_tmp665=*((char*)_check_dyneither_subscript(_tmp664,sizeof(char),0)),((_tmp666=(char)toupper((int)((char*)ifdefmacro.curr)[_tmp2B3]),((_get_dyneither_size(_tmp664,sizeof(char))== 1  && (_tmp665 == '\000'  && _tmp666 != '\000')?_throw_arraybounds(): 1,*((char*)_tmp664.curr)=_tmp666)))))));}}}}
# 1522
{const char*_tmp66B;void*_tmp66A[2];struct Cyc_String_pa_PrintArg_struct _tmp669;struct Cyc_String_pa_PrintArg_struct _tmp668;(_tmp668.tag=0,((_tmp668.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)ifdefmacro),((_tmp669.tag=0,((_tmp669.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)ifdefmacro),((_tmp66A[0]=& _tmp669,((_tmp66A[1]=& _tmp668,Cyc_fprintf(out_file,((_tmp66B="#ifndef %s\n#define %s\n",_tag_dyneither(_tmp66B,sizeof(char),23))),_tag_dyneither(_tmp66A,sizeof(void*),2)))))))))))));}{
# 1529
struct Cyc_List_List*_tmp2BE=0;
struct Cyc_List_List*_tmp2BF=0;
{struct Cyc_List_List*_tmp2C0=_tmp281;for(0;_tmp2C0 != 0;_tmp2C0=_tmp2C0->tl){
struct Cyc_Absyn_Decl*_tmp2C1=(struct Cyc_Absyn_Decl*)_tmp2C0->hd;
int _tmp2C2=0;
struct _dyneither_ptr*name;
{void*_tmp2C3=_tmp2C1->r;void*_tmp2C4=_tmp2C3;struct Cyc_Absyn_Vardecl*_tmp2C6;struct Cyc_Absyn_Fndecl*_tmp2C8;struct Cyc_Absyn_Aggrdecl*_tmp2CA;struct Cyc_Absyn_Enumdecl*_tmp2CC;struct Cyc_Absyn_Typedefdecl*_tmp2CE;_LL1B8: {struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmp2C5=(struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp2C4;if(_tmp2C5->tag != 0)goto _LL1BA;else{_tmp2C6=_tmp2C5->f1;}}_LL1B9: {
# 1537
struct _tuple0*_tmp2DA=_tmp2C6->name;struct _dyneither_ptr*_tmp2DC;struct _tuple0*_tmp2DB=_tmp2DA;_tmp2DC=_tmp2DB->f2;
name=_tmp2DC;
goto _LL1B7;}_LL1BA: {struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*_tmp2C7=(struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp2C4;if(_tmp2C7->tag != 1)goto _LL1BC;else{_tmp2C8=_tmp2C7->f1;}}_LL1BB: {
# 1541
struct _tuple0*_tmp2DD=_tmp2C8->name;struct _dyneither_ptr*_tmp2DF;struct _tuple0*_tmp2DE=_tmp2DD;_tmp2DF=_tmp2DE->f2;
name=_tmp2DF;
goto _LL1B7;}_LL1BC: {struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_tmp2C9=(struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_tmp2C4;if(_tmp2C9->tag != 6)goto _LL1BE;else{_tmp2CA=_tmp2C9->f1;}}_LL1BD: {
# 1545
struct _tuple0*_tmp2E0=_tmp2CA->name;struct _dyneither_ptr*_tmp2E2;struct _tuple0*_tmp2E1=_tmp2E0;_tmp2E2=_tmp2E1->f2;
name=_tmp2E2;
goto _LL1B7;}_LL1BE: {struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*_tmp2CB=(struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp2C4;if(_tmp2CB->tag != 8)goto _LL1C0;else{_tmp2CC=_tmp2CB->f1;}}_LL1BF: {
# 1549
struct _tuple0*_tmp2E3=_tmp2CC->name;struct _dyneither_ptr*_tmp2E5;struct _tuple0*_tmp2E4=_tmp2E3;_tmp2E5=_tmp2E4->f2;
name=_tmp2E5;
goto _LL1B7;}_LL1C0: {struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*_tmp2CD=(struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp2C4;if(_tmp2CD->tag != 9)goto _LL1C2;else{_tmp2CE=_tmp2CD->f1;}}_LL1C1: {
# 1553
struct _tuple0*_tmp2E6=_tmp2CE->name;struct _dyneither_ptr*_tmp2E8;struct _tuple0*_tmp2E7=_tmp2E6;_tmp2E8=_tmp2E7->f2;
name=_tmp2E8;
goto _LL1B7;}_LL1C2: {struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*_tmp2CF=(struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp2C4;if(_tmp2CF->tag != 4)goto _LL1C4;}_LL1C3:
 goto _LL1C5;_LL1C4: {struct Cyc_Absyn_Alias_d_Absyn_Raw_decl_struct*_tmp2D0=(struct Cyc_Absyn_Alias_d_Absyn_Raw_decl_struct*)_tmp2C4;if(_tmp2D0->tag != 5)goto _LL1C6;}_LL1C5:
 goto _LL1C7;_LL1C6: {struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct*_tmp2D1=(struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct*)_tmp2C4;if(_tmp2D1->tag != 14)goto _LL1C8;}_LL1C7:
 goto _LL1C9;_LL1C8: {struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct*_tmp2D2=(struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct*)_tmp2C4;if(_tmp2D2->tag != 15)goto _LL1CA;}_LL1C9:
 goto _LL1CB;_LL1CA: {struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*_tmp2D3=(struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmp2C4;if(_tmp2D3->tag != 2)goto _LL1CC;}_LL1CB:
 goto _LL1CD;_LL1CC: {struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*_tmp2D4=(struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*)_tmp2C4;if(_tmp2D4->tag != 7)goto _LL1CE;}_LL1CD:
 goto _LL1CF;_LL1CE: {struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct*_tmp2D5=(struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct*)_tmp2C4;if(_tmp2D5->tag != 3)goto _LL1D0;}_LL1CF:
 goto _LL1D1;_LL1D0: {struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*_tmp2D6=(struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp2C4;if(_tmp2D6->tag != 10)goto _LL1D2;}_LL1D1:
 goto _LL1D3;_LL1D2: {struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*_tmp2D7=(struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmp2C4;if(_tmp2D7->tag != 11)goto _LL1D4;}_LL1D3:
 goto _LL1D5;_LL1D4: {struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*_tmp2D8=(struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*)_tmp2C4;if(_tmp2D8->tag != 12)goto _LL1D6;}_LL1D5:
 goto _LL1D7;_LL1D6: {struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*_tmp2D9=(struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp2C4;if(_tmp2D9->tag != 13)goto _LL1B7;}_LL1D7:
# 1567
 name=0;
goto _LL1B7;_LL1B7:;}
# 1570
if(!((unsigned int)name) && !_tmp2C2)continue;
{struct Cyc_List_List*_tmp66C;_tmp2BE=((_tmp66C=_cycalloc(sizeof(*_tmp66C)),((_tmp66C->hd=_tmp2C1,((_tmp66C->tl=_tmp2BE,_tmp66C))))));}{
struct Cyc_List_List*_tmp66D;_tmp2BF=((_tmp66D=_cycalloc(sizeof(*_tmp66D)),((_tmp66D->hd=name,((_tmp66D->tl=_tmp2BF,_tmp66D))))));};}}
# 1576
{struct _handler_cons _tmp2EB;_push_handler(& _tmp2EB);{int _tmp2ED=0;if(setjmp(_tmp2EB.handler))_tmp2ED=1;if(!_tmp2ED){
{struct _RegionHandle _tmp2EE=_new_region("tc_rgn");struct _RegionHandle*tc_rgn=& _tmp2EE;_push_region(tc_rgn);
{struct Cyc_Tcenv_Tenv*_tmp2EF=Cyc_Tcenv_tc_init(tc_rgn);
Cyc_Tc_tc(tc_rgn,_tmp2EF,1,_tmp2BE);}
# 1578
;_pop_region(tc_rgn);}
# 1577
;_pop_handler();}else{void*_tmp2EC=(void*)_exn_thrown;void*_tmp2F1=_tmp2EC;void*_tmp2F2;_LL1D9:;_LL1DA:
# 1583
{struct Cyc_NO_SUPPORT_exn_struct _tmp673;const char*_tmp672;struct Cyc_NO_SUPPORT_exn_struct*_tmp671;(int)_throw((void*)((_tmp671=_cycalloc(sizeof(*_tmp671)),((_tmp671[0]=((_tmp673.tag=Cyc_NO_SUPPORT,((_tmp673.f1=((_tmp672="can't typecheck acquired declarations",_tag_dyneither(_tmp672,sizeof(char),38))),_tmp673)))),_tmp671)))));}
goto _LL1D8;_LL1DB: _tmp2F2=_tmp2F1;_LL1DC:(void)_throw(_tmp2F2);_LL1D8:;}};}
# 1588
{struct _tuple25 _tmp674;struct _tuple25 _tmp2F6=(_tmp674.f1=_tmp2BE,((_tmp674.f2=_tmp2BF,_tmp674)));struct Cyc_List_List*_tmp2F8;struct Cyc_List_List*_tmp2F9;struct _tuple25 _tmp2F7=_tmp2F6;_tmp2F8=_tmp2F7.f1;_tmp2F9=_tmp2F7.f2;for(0;
_tmp2F8 != 0  && _tmp2F9 != 0;(_tmp2F8=_tmp2F8->tl,_tmp2F9=_tmp2F9->tl)){
struct Cyc_Absyn_Decl*_tmp2FA=(struct Cyc_Absyn_Decl*)_tmp2F8->hd;
struct _dyneither_ptr*_tmp2FB=(struct _dyneither_ptr*)_tmp2F9->hd;
int _tmp2FC=0;
if(!((unsigned int)_tmp2FB))
_tmp2FC=1;
# 1597
Cyc_Absynpp_set_params(& Cyc_Absynpp_cyc_params_r);
if((unsigned int)_tmp2FB){
{const char*_tmp678;void*_tmp677[1];struct Cyc_String_pa_PrintArg_struct _tmp676;ifdefmacro=((_tmp676.tag=0,((_tmp676.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*_tmp2FB),((_tmp677[0]=& _tmp676,Cyc_aprintf(((_tmp678="_%s_def_",_tag_dyneither(_tmp678,sizeof(char),9))),_tag_dyneither(_tmp677,sizeof(void*),1))))))));}
{const char*_tmp67C;void*_tmp67B[1];struct Cyc_String_pa_PrintArg_struct _tmp67A;(_tmp67A.tag=0,((_tmp67A.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)ifdefmacro),((_tmp67B[0]=& _tmp67A,Cyc_fprintf(out_file,((_tmp67C="#ifndef %s\n",_tag_dyneither(_tmp67C,sizeof(char),12))),_tag_dyneither(_tmp67B,sizeof(void*),1)))))));}
{const char*_tmp680;void*_tmp67F[1];struct Cyc_String_pa_PrintArg_struct _tmp67E;(_tmp67E.tag=0,((_tmp67E.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)ifdefmacro),((_tmp67F[0]=& _tmp67E,Cyc_fprintf(out_file,((_tmp680="#define %s\n",_tag_dyneither(_tmp680,sizeof(char),12))),_tag_dyneither(_tmp67F,sizeof(void*),1)))))));}
# 1603
{struct Cyc_Absyn_Decl*_tmp681[1];Cyc_Absynpp_decllist2file(((_tmp681[0]=_tmp2FA,((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp681,sizeof(struct Cyc_Absyn_Decl*),1)))),out_file);}{
const char*_tmp684;void*_tmp683;(_tmp683=0,Cyc_fprintf(out_file,((_tmp684="#endif\n",_tag_dyneither(_tmp684,sizeof(char),8))),_tag_dyneither(_tmp683,sizeof(void*),0)));};}else{
# 1608
struct Cyc_Absyn_Decl*_tmp685[1];Cyc_Absynpp_decllist2file(((_tmp685[0]=_tmp2FA,((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp685,sizeof(struct Cyc_Absyn_Decl*),1)))),out_file);}}}
# 1613
maybe=Cyc_fopen(_tmp223,"r");
if(!((unsigned int)maybe)){struct Cyc_NO_SUPPORT_exn_struct _tmp696;const char*_tmp695;void*_tmp694[1];const char*_tmp693;struct Cyc_String_pa_PrintArg_struct _tmp692;struct Cyc_NO_SUPPORT_exn_struct*_tmp691;(int)_throw((void*)((_tmp691=_cycalloc(sizeof(*_tmp691)),((_tmp691[0]=((_tmp696.tag=Cyc_NO_SUPPORT,((_tmp696.f1=(struct _dyneither_ptr)((_tmp692.tag=0,((_tmp692.f1=(struct _dyneither_ptr)((_tmp693=_tmp223,_tag_dyneither(_tmp693,sizeof(char),_get_zero_arr_size_char((void*)_tmp693,1)))),((_tmp694[0]=& _tmp692,Cyc_aprintf(((_tmp695="can't open macrosfile %s",_tag_dyneither(_tmp695,sizeof(char),25))),_tag_dyneither(_tmp694,sizeof(void*),1)))))))),_tmp696)))),_tmp691)))));}
# 1616
in_file=maybe;
_tmp270=Cyc_Lexing_from_file(in_file);{
struct _tuple21*entry2;
while((entry2=((struct _tuple21*(*)(struct Cyc_Lexing_lexbuf*lexbuf))Cyc_suck_line)(_tmp270))!= 0){
struct _tuple21*_tmp311=(struct _tuple21*)_check_null(entry2);struct _dyneither_ptr _tmp313;struct _dyneither_ptr*_tmp314;struct _tuple21*_tmp312=_tmp311;_tmp313=_tmp312->f1;_tmp314=_tmp312->f2;
if(((int(*)(struct Cyc_Set_Set*s,struct _dyneither_ptr*elt))Cyc_Set_member)(_tmp280,_tmp314)){
{const char*_tmp69A;void*_tmp699[1];struct Cyc_String_pa_PrintArg_struct _tmp698;(_tmp698.tag=0,((_tmp698.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)*_tmp314),((_tmp699[0]=& _tmp698,Cyc_fprintf(out_file,((_tmp69A="#ifndef %s\n",_tag_dyneither(_tmp69A,sizeof(char),12))),_tag_dyneither(_tmp699,sizeof(void*),1)))))));}
{const char*_tmp69E;void*_tmp69D[1];struct Cyc_String_pa_PrintArg_struct _tmp69C;(_tmp69C.tag=0,((_tmp69C.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)_tmp313),((_tmp69D[0]=& _tmp69C,Cyc_fprintf(out_file,((_tmp69E="%s\n",_tag_dyneither(_tmp69E,sizeof(char),4))),_tag_dyneither(_tmp69D,sizeof(void*),1)))))));}{
const char*_tmp6A1;void*_tmp6A0;(_tmp6A0=0,Cyc_fprintf(out_file,((_tmp6A1="#endif\n",_tag_dyneither(_tmp6A1,sizeof(char),8))),_tag_dyneither(_tmp6A0,sizeof(void*),0)));};}}
# 1627
Cyc_fclose(in_file);
if(Cyc_mode != Cyc_FINISH);
# 1630
if(hstubs != 0){
struct Cyc_List_List*_tmp31D=hstubs;for(0;_tmp31D != 0;_tmp31D=_tmp31D->tl){
struct _tuple22*_tmp31E=(struct _tuple22*)_tmp31D->hd;struct _dyneither_ptr _tmp320;struct _dyneither_ptr _tmp321;struct _tuple22*_tmp31F=_tmp31E;_tmp320=_tmp31F->f1;_tmp321=_tmp31F->f2;{
struct _dyneither_ptr*_tmp6A2;if((char*)_tmp321.curr != (char*)(_tag_dyneither(0,0,0)).curr  && (
(char*)_tmp320.curr == (char*)(_tag_dyneither(0,0,0)).curr  || ((int(*)(struct Cyc_Set_Set*s,struct _dyneither_ptr*elt))Cyc_Set_member)(defined_symbols,((_tmp6A2=_cycalloc(sizeof(*_tmp6A2)),((_tmp6A2[0]=_tmp320,_tmp6A2)))))))
# 1636
Cyc_fputs((const char*)_untag_dyneither_ptr(_tmp321,sizeof(char),1),out_file);else{
# 1638
const char*_tmp6A6;void*_tmp6A5[1];struct Cyc_String_pa_PrintArg_struct _tmp6A4;(_tmp6A4.tag=0,((_tmp6A4.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)_tmp320),((_tmp6A5[0]=& _tmp6A4,Cyc_log(((_tmp6A6="%s is not supported on this platform\n",_tag_dyneither(_tmp6A6,sizeof(char),38))),_tag_dyneither(_tmp6A5,sizeof(void*),1)))))));}};}}
# 1641
{const char*_tmp6A9;void*_tmp6A8;(_tmp6A8=0,Cyc_fprintf(out_file,((_tmp6A9="#endif\n",_tag_dyneither(_tmp6A9,sizeof(char),8))),_tag_dyneither(_tmp6A8,sizeof(void*),0)));}
if(Cyc_do_setjmp){int _tmp328=0;_npop_handler(0);return _tmp328;}else{
Cyc_fclose(out_file);}
# 1646
if(cstubs != 0){
out_file=(struct Cyc___cycFILE*)_check_null(Cyc_cstubs_file);{
struct Cyc_List_List*_tmp329=cstubs;for(0;_tmp329 != 0;_tmp329=_tmp329->tl){
struct _tuple22*_tmp32A=(struct _tuple22*)_tmp329->hd;struct _dyneither_ptr _tmp32C;struct _dyneither_ptr _tmp32D;struct _tuple22*_tmp32B=_tmp32A;_tmp32C=_tmp32B->f1;_tmp32D=_tmp32B->f2;{
struct _dyneither_ptr*_tmp6AA;if((char*)_tmp32D.curr != (char*)(_tag_dyneither(0,0,0)).curr  && (
(char*)_tmp32C.curr == (char*)(_tag_dyneither(0,0,0)).curr  || ((int(*)(struct Cyc_Set_Set*s,struct _dyneither_ptr*elt))Cyc_Set_member)(defined_symbols,((_tmp6AA=_cycalloc(sizeof(*_tmp6AA)),((_tmp6AA[0]=_tmp32C,_tmp6AA)))))))
Cyc_fputs((const char*)_untag_dyneither_ptr(_tmp32D,sizeof(char),1),out_file);};}};}
# 1657
out_file=(struct Cyc___cycFILE*)_check_null(Cyc_cycstubs_file);
# 1659
{const char*_tmp6B0;void*_tmp6AF[1];const char*_tmp6AE;struct Cyc_String_pa_PrintArg_struct _tmp6AD;(_tmp6AD.tag=0,((_tmp6AD.f1=(struct _dyneither_ptr)((_tmp6AE=filename,_tag_dyneither(_tmp6AE,sizeof(char),_get_zero_arr_size_char((void*)_tmp6AE,1)))),((_tmp6AF[0]=& _tmp6AD,Cyc_fprintf(out_file,((_tmp6B0="#include <%s>\n\n",_tag_dyneither(_tmp6B0,sizeof(char),16))),_tag_dyneither(_tmp6AF,sizeof(void*),1)))))));}
if(cycstubs != 0){
out_file=(struct Cyc___cycFILE*)_check_null(Cyc_cycstubs_file);
{struct Cyc_List_List*_tmp333=cycstubs;for(0;_tmp333 != 0;_tmp333=_tmp333->tl){
struct _tuple22*_tmp334=(struct _tuple22*)_tmp333->hd;struct _dyneither_ptr _tmp336;struct _dyneither_ptr _tmp337;struct _tuple22*_tmp335=_tmp334;_tmp336=_tmp335->f1;_tmp337=_tmp335->f2;{
struct _dyneither_ptr*_tmp6B1;if((char*)_tmp337.curr != (char*)(_tag_dyneither(0,0,0)).curr  && (
(char*)_tmp336.curr == (char*)(_tag_dyneither(0,0,0)).curr  || ((int(*)(struct Cyc_Set_Set*s,struct _dyneither_ptr*elt))Cyc_Set_member)(defined_symbols,((_tmp6B1=_cycalloc(sizeof(*_tmp6B1)),((_tmp6B1[0]=_tmp336,_tmp6B1)))))))
Cyc_fputs((const char*)_untag_dyneither_ptr(_tmp337,sizeof(char),1),out_file);};}}{
# 1668
const char*_tmp6B4;void*_tmp6B3;(_tmp6B3=0,Cyc_fprintf(out_file,((_tmp6B4="\n",_tag_dyneither(_tmp6B4,sizeof(char),2))),_tag_dyneither(_tmp6B3,sizeof(void*),0)));};}{
# 1671
int _tmp33B=0;_npop_handler(0);return _tmp33B;};};};};};};};};
# 1347
;_pop_handler();}else{void*_tmp227=(void*)_exn_thrown;void*_tmp341=_tmp227;struct _dyneither_ptr _tmp343;struct _dyneither_ptr _tmp346;struct _dyneither_ptr _tmp348;struct _dyneither_ptr _tmp34B;void*_tmp34C;_LL1DE: {struct Cyc_Core_Impossible_exn_struct*_tmp342=(struct Cyc_Core_Impossible_exn_struct*)_tmp341;if(_tmp342->tag != Cyc_Core_Impossible)goto _LL1E0;else{_tmp343=_tmp342->f1;}}_LL1DF:
# 1675
{const char*_tmp6B8;void*_tmp6B7[1];struct Cyc_String_pa_PrintArg_struct _tmp6B6;(_tmp6B6.tag=0,((_tmp6B6.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)_tmp343),((_tmp6B7[0]=& _tmp6B6,Cyc_fprintf(Cyc_stderr,((_tmp6B8="Got Core::Impossible(%s)\n",_tag_dyneither(_tmp6B8,sizeof(char),26))),_tag_dyneither(_tmp6B7,sizeof(void*),1)))))));}goto _LL1DD;_LL1E0: {struct Cyc_Dict_Absent_exn_struct*_tmp344=(struct Cyc_Dict_Absent_exn_struct*)_tmp341;if(_tmp344->tag != Cyc_Dict_Absent)goto _LL1E2;}_LL1E1:
# 1677
{const char*_tmp6BB;void*_tmp6BA;(_tmp6BA=0,Cyc_fprintf(Cyc_stderr,((_tmp6BB="Got Dict::Absent\n",_tag_dyneither(_tmp6BB,sizeof(char),18))),_tag_dyneither(_tmp6BA,sizeof(void*),0)));}goto _LL1DD;_LL1E2: {struct Cyc_Core_Failure_exn_struct*_tmp345=(struct Cyc_Core_Failure_exn_struct*)_tmp341;if(_tmp345->tag != Cyc_Core_Failure)goto _LL1E4;else{_tmp346=_tmp345->f1;}}_LL1E3:
# 1679
{const char*_tmp6BF;void*_tmp6BE[1];struct Cyc_String_pa_PrintArg_struct _tmp6BD;(_tmp6BD.tag=0,((_tmp6BD.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)_tmp346),((_tmp6BE[0]=& _tmp6BD,Cyc_fprintf(Cyc_stderr,((_tmp6BF="Got Core::Failure(%s)\n",_tag_dyneither(_tmp6BF,sizeof(char),23))),_tag_dyneither(_tmp6BE,sizeof(void*),1)))))));}goto _LL1DD;_LL1E4: {struct Cyc_Core_Invalid_argument_exn_struct*_tmp347=(struct Cyc_Core_Invalid_argument_exn_struct*)_tmp341;if(_tmp347->tag != Cyc_Core_Invalid_argument)goto _LL1E6;else{_tmp348=_tmp347->f1;}}_LL1E5:
# 1681
{const char*_tmp6C3;void*_tmp6C2[1];struct Cyc_String_pa_PrintArg_struct _tmp6C1;(_tmp6C1.tag=0,((_tmp6C1.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)_tmp348),((_tmp6C2[0]=& _tmp6C1,Cyc_fprintf(Cyc_stderr,((_tmp6C3="Got Invalid_argument(%s)\n",_tag_dyneither(_tmp6C3,sizeof(char),26))),_tag_dyneither(_tmp6C2,sizeof(void*),1)))))));}goto _LL1DD;_LL1E6: {struct Cyc_Core_Not_found_exn_struct*_tmp349=(struct Cyc_Core_Not_found_exn_struct*)_tmp341;if(_tmp349->tag != Cyc_Core_Not_found)goto _LL1E8;}_LL1E7:
# 1683
{const char*_tmp6C6;void*_tmp6C5;(_tmp6C5=0,Cyc_fprintf(Cyc_stderr,((_tmp6C6="Got Not_found\n",_tag_dyneither(_tmp6C6,sizeof(char),15))),_tag_dyneither(_tmp6C5,sizeof(void*),0)));}goto _LL1DD;_LL1E8: {struct Cyc_NO_SUPPORT_exn_struct*_tmp34A=(struct Cyc_NO_SUPPORT_exn_struct*)_tmp341;if(_tmp34A->tag != Cyc_NO_SUPPORT)goto _LL1EA;else{_tmp34B=_tmp34A->f1;}}_LL1E9:
# 1685
{const char*_tmp6CA;void*_tmp6C9[1];struct Cyc_String_pa_PrintArg_struct _tmp6C8;(_tmp6C8.tag=0,((_tmp6C8.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)_tmp34B),((_tmp6C9[0]=& _tmp6C8,Cyc_fprintf(Cyc_stderr,((_tmp6CA="No support because %s\n",_tag_dyneither(_tmp6CA,sizeof(char),23))),_tag_dyneither(_tmp6C9,sizeof(void*),1)))))));}goto _LL1DD;_LL1EA:;_LL1EB:
# 1687
{const char*_tmp6CD;void*_tmp6CC;(_tmp6CC=0,Cyc_fprintf(Cyc_stderr,((_tmp6CD="Got unknown exception\n",_tag_dyneither(_tmp6CD,sizeof(char),23))),_tag_dyneither(_tmp6CC,sizeof(void*),0)));}goto _LL1DD;_LL1EC: _tmp34C=_tmp341;_LL1ED:(void)_throw(_tmp34C);_LL1DD:;}};}
# 1692
maybe=Cyc_fopen(filename,"w");
if(!((unsigned int)maybe)){
{const char*_tmp6D3;void*_tmp6D2[1];const char*_tmp6D1;struct Cyc_String_pa_PrintArg_struct _tmp6D0;(_tmp6D0.tag=0,((_tmp6D0.f1=(struct _dyneither_ptr)((_tmp6D1=filename,_tag_dyneither(_tmp6D1,sizeof(char),_get_zero_arr_size_char((void*)_tmp6D1,1)))),((_tmp6D2[0]=& _tmp6D0,Cyc_fprintf(Cyc_stderr,((_tmp6D3="Error: could not create file %s\n",_tag_dyneither(_tmp6D3,sizeof(char),33))),_tag_dyneither(_tmp6D2,sizeof(void*),1)))))));}
return 1;}
# 1697
out_file=maybe;
{const char*_tmp6D9;void*_tmp6D8[1];const char*_tmp6D7;struct Cyc_String_pa_PrintArg_struct _tmp6D6;(_tmp6D6.tag=0,((_tmp6D6.f1=(struct _dyneither_ptr)(
# 1700
(_tmp6D7=filename,_tag_dyneither(_tmp6D7,sizeof(char),_get_zero_arr_size_char((void*)_tmp6D7,1)))),((_tmp6D8[0]=& _tmp6D6,Cyc_fprintf(out_file,((_tmp6D9="#error -- %s is not supported on this platform\n",_tag_dyneither(_tmp6D9,sizeof(char),48))),_tag_dyneither(_tmp6D8,sizeof(void*),1)))))));}
Cyc_fclose(out_file);
{const char*_tmp6DF;void*_tmp6DE[1];const char*_tmp6DD;struct Cyc_String_pa_PrintArg_struct _tmp6DC;(_tmp6DC.tag=0,((_tmp6DC.f1=(struct _dyneither_ptr)(
(_tmp6DD=filename,_tag_dyneither(_tmp6DD,sizeof(char),_get_zero_arr_size_char((void*)_tmp6DD,1)))),((_tmp6DE[0]=& _tmp6DC,Cyc_fprintf(Cyc_stderr,((_tmp6DF="Warning: %s will not be supported on this platform\n",_tag_dyneither(_tmp6DF,sizeof(char),52))),_tag_dyneither(_tmp6DE,sizeof(void*),1)))))));}
{const char*_tmp6E2;void*_tmp6E1;(_tmp6E1=0,Cyc_log(((_tmp6E2="Not supported on this platform\n",_tag_dyneither(_tmp6E2,sizeof(char),32))),_tag_dyneither(_tmp6E1,sizeof(void*),0)));}
# 1711
return 0;};}int Cyc_process_specfile(const char*file,const char*dir);static void _tmp6ED(unsigned int*_tmp6EC,unsigned int*_tmp6EB,char**_tmp6EA){for(*_tmp6EC=0;*_tmp6EC < *_tmp6EB;(*_tmp6EC)++){(*_tmp6EA)[*_tmp6EC]=(char)'\000';}}
# 1715
int Cyc_process_specfile(const char*file,const char*dir){
struct Cyc___cycFILE*_tmp382=Cyc_fopen(file,"r");
if(!((unsigned int)_tmp382)){
{const char*_tmp6E8;void*_tmp6E7[1];const char*_tmp6E6;struct Cyc_String_pa_PrintArg_struct _tmp6E5;(_tmp6E5.tag=0,((_tmp6E5.f1=(struct _dyneither_ptr)((_tmp6E6=file,_tag_dyneither(_tmp6E6,sizeof(char),_get_zero_arr_size_char((void*)_tmp6E6,1)))),((_tmp6E7[0]=& _tmp6E5,Cyc_fprintf(Cyc_stderr,((_tmp6E8="Error: could not open %s\n",_tag_dyneither(_tmp6E8,sizeof(char),26))),_tag_dyneither(_tmp6E7,sizeof(void*),1)))))));}
return 1;}{
# 1721
struct Cyc___cycFILE*_tmp387=_tmp382;
# 1725
char*_tmp6F2;unsigned int _tmp6F1;char*_tmp6F0;unsigned int _tmp6EF;unsigned int _tmp6EE;struct _dyneither_ptr buf=(_tmp6F2=((_tmp6F1=(unsigned int)1024,((_tmp6F0=(char*)_cycalloc_atomic(_check_times(sizeof(char),_tmp6F1 + 1)),((((_tmp6EF=_tmp6F1,((_tmp6ED(& _tmp6EE,& _tmp6EF,& _tmp6F0),_tmp6F0[_tmp6EF]=(char)0)))),_tmp6F0)))))),_tag_dyneither(_tmp6F2,sizeof(char),_get_zero_arr_size_char((void*)_tmp6F2,(unsigned int)1024 + 1)));
struct _dyneither_ptr _tmp388=Cyc_getcwd(buf,_get_dyneither_size(buf,sizeof(char)));
if(Cyc_mode != Cyc_GATHERSCRIPT){
if(chdir(dir)){
{const char*_tmp6F8;void*_tmp6F7[1];const char*_tmp6F6;struct Cyc_String_pa_PrintArg_struct _tmp6F5;(_tmp6F5.tag=0,((_tmp6F5.f1=(struct _dyneither_ptr)((_tmp6F6=dir,_tag_dyneither(_tmp6F6,sizeof(char),_get_zero_arr_size_char((void*)_tmp6F6,1)))),((_tmp6F7[0]=& _tmp6F5,Cyc_fprintf(Cyc_stderr,((_tmp6F8="Error: can't change directory to %s\n",_tag_dyneither(_tmp6F8,sizeof(char),37))),_tag_dyneither(_tmp6F7,sizeof(void*),1)))))));}
return 1;}}
# 1733
if(Cyc_mode == Cyc_GATHER){
# 1735
struct _dyneither_ptr _tmp38D=Cstring_to_string(Ccomp);
const char*_tmp6FC;void*_tmp6FB[1];struct Cyc_String_pa_PrintArg_struct _tmp6FA;system((const char*)_untag_dyneither_ptr(((_tmp6FA.tag=0,((_tmp6FA.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)_tmp38D),((_tmp6FB[0]=& _tmp6FA,Cyc_aprintf(((_tmp6FC="echo | %s -E -dM - -o INITMACROS.h\n",_tag_dyneither(_tmp6FC,sizeof(char),36))),_tag_dyneither(_tmp6FB,sizeof(void*),1)))))))),sizeof(char),1));}{
# 1739
struct Cyc_Lexing_lexbuf*_tmp391=Cyc_Lexing_from_file(_tmp387);
struct _tuple23*entry;
while((entry=((struct _tuple23*(*)(struct Cyc_Lexing_lexbuf*lexbuf))Cyc_spec)(_tmp391))!= 0){
struct _tuple23*_tmp392=(struct _tuple23*)_check_null(entry);struct _dyneither_ptr _tmp394;struct Cyc_List_List*_tmp395;struct Cyc_List_List*_tmp396;struct Cyc_List_List*_tmp397;struct Cyc_List_List*_tmp398;struct Cyc_List_List*_tmp399;struct _tuple23*_tmp393=_tmp392;_tmp394=_tmp393->f1;_tmp395=_tmp393->f2;_tmp396=_tmp393->f3;_tmp397=_tmp393->f4;_tmp398=_tmp393->f5;_tmp399=_tmp393->f6;
# 1744
if(Cyc_process_file((const char*)_untag_dyneither_ptr(_tmp394,sizeof(char),1),_tmp395,_tmp396,_tmp397,_tmp398,_tmp399))
# 1746
return 1;}
# 1748
Cyc_fclose(_tmp387);
# 1750
if(Cyc_mode != Cyc_GATHERSCRIPT){
if(chdir((const char*)((char*)_untag_dyneither_ptr(_tmp388,sizeof(char),1)))){
{const char*_tmp700;void*_tmp6FF[1];struct Cyc_String_pa_PrintArg_struct _tmp6FE;(_tmp6FE.tag=0,((_tmp6FE.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)_tmp388),((_tmp6FF[0]=& _tmp6FE,Cyc_fprintf(Cyc_stderr,((_tmp700="Error: could not change directory to %s\n",_tag_dyneither(_tmp700,sizeof(char),41))),_tag_dyneither(_tmp6FF,sizeof(void*),1)))))));}
return 1;}}
# 1756
return 0;};};}int Cyc_process_setjmp(const char*dir);static void _tmp705(unsigned int*_tmp704,unsigned int*_tmp703,char**_tmp702){for(*_tmp704=0;*_tmp704 < *_tmp703;(*_tmp704)++){(*_tmp702)[*_tmp704]=(char)'\000';}}
# 1760
int Cyc_process_setjmp(const char*dir){
# 1763
char*_tmp70A;unsigned int _tmp709;char*_tmp708;unsigned int _tmp707;unsigned int _tmp706;struct _dyneither_ptr buf=(_tmp70A=((_tmp709=(unsigned int)1024,((_tmp708=(char*)_cycalloc_atomic(_check_times(sizeof(char),_tmp709 + 1)),((((_tmp707=_tmp709,((_tmp705(& _tmp706,& _tmp707,& _tmp708),_tmp708[_tmp707]=(char)0)))),_tmp708)))))),_tag_dyneither(_tmp70A,sizeof(char),_get_zero_arr_size_char((void*)_tmp70A,(unsigned int)1024 + 1)));
struct _dyneither_ptr _tmp3A1=Cyc_getcwd(buf,_get_dyneither_size(buf,sizeof(char)));
if(chdir(dir)){
{const char*_tmp710;void*_tmp70F[1];const char*_tmp70E;struct Cyc_String_pa_PrintArg_struct _tmp70D;(_tmp70D.tag=0,((_tmp70D.f1=(struct _dyneither_ptr)((_tmp70E=dir,_tag_dyneither(_tmp70E,sizeof(char),_get_zero_arr_size_char((void*)_tmp70E,1)))),((_tmp70F[0]=& _tmp70D,Cyc_fprintf(Cyc_stderr,((_tmp710="Error: can't change directory to %s\n",_tag_dyneither(_tmp710,sizeof(char),37))),_tag_dyneither(_tmp70F,sizeof(void*),1)))))));}
return 1;}
# 1769
{struct _tuple22*_tmp71A;const char*_tmp719;const char*_tmp718;struct _tuple22*_tmp717[1];struct _dyneither_ptr*_tmp711[1];if(Cyc_process_file("setjmp.h",((_tmp711[0]=_init_dyneither_ptr(_cycalloc(sizeof(struct _dyneither_ptr)),"jmp_buf",sizeof(char),8),((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp711,sizeof(struct _dyneither_ptr*),1)))),0,(
(_tmp717[0]=((_tmp71A=_cycalloc(sizeof(*_tmp71A)),((_tmp71A->f1=((_tmp719="setjmp",_tag_dyneither(_tmp719,sizeof(char),7))),((_tmp71A->f2=((_tmp718="extern int setjmp(jmp_buf);\n",_tag_dyneither(_tmp718,sizeof(char),29))),_tmp71A)))))),((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp717,sizeof(struct _tuple22*),1)))),0,0))
# 1772
return 1;}
if(chdir((const char*)((char*)_untag_dyneither_ptr(_tmp3A1,sizeof(char),1)))){
{const char*_tmp71E;void*_tmp71D[1];struct Cyc_String_pa_PrintArg_struct _tmp71C;(_tmp71C.tag=0,((_tmp71C.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)_tmp3A1),((_tmp71D[0]=& _tmp71C,Cyc_fprintf(Cyc_stderr,((_tmp71E="Error: could not change directory to %s\n",_tag_dyneither(_tmp71E,sizeof(char),41))),_tag_dyneither(_tmp71D,sizeof(void*),1)))))));}
return 1;}
# 1777
return 0;}static char _tmp3B3[13]="BUILDLIB.OUT";
# 1781
static struct _dyneither_ptr Cyc_output_dir={_tmp3B3,_tmp3B3,_tmp3B3 + 13};
static void Cyc_set_output_dir(struct _dyneither_ptr s){
Cyc_output_dir=s;}
# 1785
static struct Cyc_List_List*Cyc_spec_files=0;
static void Cyc_add_spec_file(struct _dyneither_ptr s){
struct Cyc_List_List*_tmp71F;Cyc_spec_files=((_tmp71F=_cycalloc(sizeof(*_tmp71F)),((_tmp71F->hd=(const char*)_untag_dyneither_ptr(s,sizeof(char),1),((_tmp71F->tl=Cyc_spec_files,_tmp71F))))));}
# 1789
static void Cyc_set_GATHER(){
Cyc_mode=Cyc_GATHER;}
# 1792
static void Cyc_set_GATHERSCRIPT(){
Cyc_mode=Cyc_GATHERSCRIPT;}
# 1795
static void Cyc_set_FINISH(){
Cyc_mode=Cyc_FINISH;}
# 1798
static void Cyc_add_cpparg(struct _dyneither_ptr s){
struct _dyneither_ptr*_tmp722;struct Cyc_List_List*_tmp721;Cyc_cppargs=((_tmp721=_cycalloc(sizeof(*_tmp721)),((_tmp721->hd=((_tmp722=_cycalloc(sizeof(*_tmp722)),((_tmp722[0]=s,_tmp722)))),((_tmp721->tl=Cyc_cppargs,_tmp721))))));}
# 1801
static int Cyc_badparse=0;
static void Cyc_unsupported_option(struct _dyneither_ptr s){
{const char*_tmp726;void*_tmp725[1];struct Cyc_String_pa_PrintArg_struct _tmp724;(_tmp724.tag=0,((_tmp724.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)s),((_tmp725[0]=& _tmp724,Cyc_fprintf(Cyc_stderr,((_tmp726="Unsupported option %s\n",_tag_dyneither(_tmp726,sizeof(char),23))),_tag_dyneither(_tmp725,sizeof(void*),1)))))));}
Cyc_badparse=1;}
# 1810
void GC_blacklist_warn_clear();struct _tuple26{struct _dyneither_ptr f1;int f2;struct _dyneither_ptr f3;void*f4;struct _dyneither_ptr f5;};
int Cyc_main(int argc,struct _dyneither_ptr argv){
GC_blacklist_warn_clear();{
# 1814
struct _tuple26*_tmp793;const char*_tmp792;const char*_tmp791;struct Cyc_Arg_String_spec_Arg_Spec_struct _tmp790;struct Cyc_Arg_String_spec_Arg_Spec_struct*_tmp78F;const char*_tmp78E;struct _tuple26*_tmp78D;const char*_tmp78C;const char*_tmp78B;struct Cyc_Arg_Unit_spec_Arg_Spec_struct _tmp78A;struct Cyc_Arg_Unit_spec_Arg_Spec_struct*_tmp789;const char*_tmp788;struct _tuple26*_tmp787;const char*_tmp786;const char*_tmp785;struct Cyc_Arg_Unit_spec_Arg_Spec_struct _tmp784;struct Cyc_Arg_Unit_spec_Arg_Spec_struct*_tmp783;const char*_tmp782;struct _tuple26*_tmp781;const char*_tmp780;const char*_tmp77F;struct Cyc_Arg_Unit_spec_Arg_Spec_struct _tmp77E;struct Cyc_Arg_Unit_spec_Arg_Spec_struct*_tmp77D;const char*_tmp77C;struct _tuple26*_tmp77B;const char*_tmp77A;const char*_tmp779;struct Cyc_Arg_Set_spec_Arg_Spec_struct _tmp778;struct Cyc_Arg_Set_spec_Arg_Spec_struct*_tmp777;const char*_tmp776;struct _tuple26*_tmp775;const char*_tmp774;const char*_tmp773;struct Cyc_Arg_Flag_spec_Arg_Spec_struct _tmp772;struct Cyc_Arg_Flag_spec_Arg_Spec_struct*_tmp771;const char*_tmp770;struct _tuple26*_tmp76F[6];struct Cyc_List_List*options=
(_tmp76F[5]=(
# 1833
(_tmp775=_cycalloc(sizeof(*_tmp775)),((_tmp775->f1=((_tmp774="-",_tag_dyneither(_tmp774,sizeof(char),2))),((_tmp775->f2=1,((_tmp775->f3=((_tmp773="",_tag_dyneither(_tmp773,sizeof(char),1))),((_tmp775->f4=(void*)(
(_tmp771=_cycalloc(sizeof(*_tmp771)),((_tmp771[0]=((_tmp772.tag=1,((_tmp772.f1=Cyc_add_cpparg,_tmp772)))),_tmp771)))),((_tmp775->f5=(
(_tmp770="",_tag_dyneither(_tmp770,sizeof(char),1))),_tmp775)))))))))))),((_tmp76F[4]=(
# 1827
(_tmp77B=_cycalloc(sizeof(*_tmp77B)),((_tmp77B->f1=((_tmp77A="-setjmp",_tag_dyneither(_tmp77A,sizeof(char),8))),((_tmp77B->f2=0,((_tmp77B->f3=((_tmp779="",_tag_dyneither(_tmp779,sizeof(char),1))),((_tmp77B->f4=(void*)(
(_tmp777=_cycalloc(sizeof(*_tmp777)),((_tmp777[0]=((_tmp778.tag=3,((_tmp778.f1=& Cyc_do_setjmp,_tmp778)))),_tmp777)))),((_tmp77B->f5=(
# 1832
(_tmp776="Produce the jmp_buf and setjmp declarations on the standard output, for use by the Cyclone compiler special file cyc_setjmp.h.  Cannot be used with -gather, -gatherscript, or specfiles.",_tag_dyneither(_tmp776,sizeof(char),186))),_tmp77B)))))))))))),((_tmp76F[3]=(
# 1824
(_tmp781=_cycalloc(sizeof(*_tmp781)),((_tmp781->f1=((_tmp780="-finish",_tag_dyneither(_tmp780,sizeof(char),8))),((_tmp781->f2=0,((_tmp781->f3=((_tmp77F="",_tag_dyneither(_tmp77F,sizeof(char),1))),((_tmp781->f4=(void*)(
(_tmp77D=_cycalloc(sizeof(*_tmp77D)),((_tmp77D[0]=((_tmp77E.tag=0,((_tmp77E.f1=Cyc_set_FINISH,_tmp77E)))),_tmp77D)))),((_tmp781->f5=(
(_tmp77C="Produce Cyclone headers from pre-gathered C library info",_tag_dyneither(_tmp77C,sizeof(char),57))),_tmp781)))))))))))),((_tmp76F[2]=(
# 1821
(_tmp787=_cycalloc(sizeof(*_tmp787)),((_tmp787->f1=((_tmp786="-gatherscript",_tag_dyneither(_tmp786,sizeof(char),14))),((_tmp787->f2=0,((_tmp787->f3=((_tmp785="",_tag_dyneither(_tmp785,sizeof(char),1))),((_tmp787->f4=(void*)(
(_tmp783=_cycalloc(sizeof(*_tmp783)),((_tmp783[0]=((_tmp784.tag=0,((_tmp784.f1=Cyc_set_GATHERSCRIPT,_tmp784)))),_tmp783)))),((_tmp787->f5=(
(_tmp782="Produce a script to gather C library info",_tag_dyneither(_tmp782,sizeof(char),42))),_tmp787)))))))))))),((_tmp76F[1]=(
# 1818
(_tmp78D=_cycalloc(sizeof(*_tmp78D)),((_tmp78D->f1=((_tmp78C="-gather",_tag_dyneither(_tmp78C,sizeof(char),8))),((_tmp78D->f2=0,((_tmp78D->f3=((_tmp78B="",_tag_dyneither(_tmp78B,sizeof(char),1))),((_tmp78D->f4=(void*)(
(_tmp789=_cycalloc(sizeof(*_tmp789)),((_tmp789[0]=((_tmp78A.tag=0,((_tmp78A.f1=Cyc_set_GATHER,_tmp78A)))),_tmp789)))),((_tmp78D->f5=(
(_tmp788="Gather C library info but don't produce Cyclone headers",_tag_dyneither(_tmp788,sizeof(char),56))),_tmp78D)))))))))))),((_tmp76F[0]=(
# 1815
(_tmp793=_cycalloc(sizeof(*_tmp793)),((_tmp793->f1=((_tmp792="-d",_tag_dyneither(_tmp792,sizeof(char),3))),((_tmp793->f2=0,((_tmp793->f3=((_tmp791=" <file>",_tag_dyneither(_tmp791,sizeof(char),8))),((_tmp793->f4=(void*)(
(_tmp78F=_cycalloc(sizeof(*_tmp78F)),((_tmp78F[0]=((_tmp790.tag=5,((_tmp790.f1=Cyc_set_output_dir,_tmp790)))),_tmp78F)))),((_tmp793->f5=(
(_tmp78E="Set the output directory to <file>",_tag_dyneither(_tmp78E,sizeof(char),35))),_tmp793)))))))))))),((struct Cyc_List_List*(*)(struct _dyneither_ptr))Cyc_List_list)(_tag_dyneither(_tmp76F,sizeof(struct _tuple26*),6)))))))))))));
# 1837
{const char*_tmp794;Cyc_Arg_parse(options,Cyc_add_spec_file,((_tmp794="Options:",_tag_dyneither(_tmp794,sizeof(char),9))),argv);}
if((((Cyc_badparse  || 
!Cyc_do_setjmp  && Cyc_spec_files == 0) || 
Cyc_do_setjmp  && Cyc_spec_files != 0) || 
Cyc_do_setjmp  && Cyc_mode == Cyc_GATHER) || 
Cyc_do_setjmp  && Cyc_mode == Cyc_GATHERSCRIPT){
{const char*_tmp795;Cyc_Arg_usage(options,(
# 1845
(_tmp795="Usage: buildlib [options] specfile1 specfile2 ...\nOptions:",_tag_dyneither(_tmp795,sizeof(char),59))));}
return 1;}
# 1849
if(Cyc_mode == Cyc_GATHERSCRIPT){
Cyc_script_file=Cyc_fopen("BUILDLIB.sh","w");
if(!((unsigned int)Cyc_script_file)){
{const char*_tmp798;void*_tmp797;(_tmp797=0,Cyc_fprintf(Cyc_stderr,((_tmp798="Could not create file BUILDLIB.sh\n",_tag_dyneither(_tmp798,sizeof(char),35))),_tag_dyneither(_tmp797,sizeof(void*),0)));}
exit(1);}
# 1855
{const char*_tmp79B;void*_tmp79A;(_tmp79A=0,Cyc_prscript(((_tmp79B="#!/bin/sh\n",_tag_dyneither(_tmp79B,sizeof(char),11))),_tag_dyneither(_tmp79A,sizeof(void*),0)));}{
const char*_tmp79E;void*_tmp79D;(_tmp79D=0,Cyc_prscript(((_tmp79E="GCC=\"gcc\"\n",_tag_dyneither(_tmp79E,sizeof(char),11))),_tag_dyneither(_tmp79D,sizeof(void*),0)));};}
# 1860
if(Cyc_force_directory_prefixes(Cyc_output_dir) || Cyc_force_directory(Cyc_output_dir)){
{const char*_tmp7A2;void*_tmp7A1[1];struct Cyc_String_pa_PrintArg_struct _tmp7A0;(_tmp7A0.tag=0,((_tmp7A0.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_output_dir),((_tmp7A1[0]=& _tmp7A0,Cyc_fprintf(Cyc_stderr,((_tmp7A2="Error: could not create directory %s\n",_tag_dyneither(_tmp7A2,sizeof(char),38))),_tag_dyneither(_tmp7A1,sizeof(void*),1)))))));}
return 1;}
# 1865
if(Cyc_mode == Cyc_GATHERSCRIPT){
{const char*_tmp7A6;void*_tmp7A5[1];struct Cyc_String_pa_PrintArg_struct _tmp7A4;(_tmp7A4.tag=0,((_tmp7A4.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_output_dir),((_tmp7A5[0]=& _tmp7A4,Cyc_prscript(((_tmp7A6="cd %s\n",_tag_dyneither(_tmp7A6,sizeof(char),7))),_tag_dyneither(_tmp7A5,sizeof(void*),1)))))));}{
const char*_tmp7A9;void*_tmp7A8;(_tmp7A8=0,Cyc_prscript(((_tmp7A9="echo | $GCC -E -dM - -o INITMACROS.h\n",_tag_dyneither(_tmp7A9,sizeof(char),38))),_tag_dyneither(_tmp7A8,sizeof(void*),0)));};}
# 1870
if(!Cyc_gathering()){
# 1873
{const char*_tmp7AA;Cyc_log_file=Cyc_fopen((const char*)_untag_dyneither_ptr(Cyc_Filename_concat(Cyc_output_dir,((_tmp7AA="BUILDLIB.LOG",_tag_dyneither(_tmp7AA,sizeof(char),13)))),sizeof(char),1),"w");}
if(!((unsigned int)Cyc_log_file)){
{const char*_tmp7AE;void*_tmp7AD[1];struct Cyc_String_pa_PrintArg_struct _tmp7AC;(_tmp7AC.tag=0,((_tmp7AC.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_output_dir),((_tmp7AD[0]=& _tmp7AC,Cyc_fprintf(Cyc_stderr,((_tmp7AE="Error: could not create log file in directory %s\n",_tag_dyneither(_tmp7AE,sizeof(char),50))),_tag_dyneither(_tmp7AD,sizeof(void*),1)))))));}
return 1;}
# 1879
if(!Cyc_do_setjmp){
# 1881
{const char*_tmp7AF;Cyc_cstubs_file=Cyc_fopen((const char*)_untag_dyneither_ptr(Cyc_Filename_concat(Cyc_output_dir,((_tmp7AF="cstubs.c",_tag_dyneither(_tmp7AF,sizeof(char),9)))),sizeof(char),1),"w");}
if(!((unsigned int)Cyc_cstubs_file)){
{const char*_tmp7B3;void*_tmp7B2[1];struct Cyc_String_pa_PrintArg_struct _tmp7B1;(_tmp7B1.tag=0,((_tmp7B1.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_output_dir),((_tmp7B2[0]=& _tmp7B1,Cyc_fprintf(Cyc_stderr,((_tmp7B3="Error: could not create cstubs.c in directory %s\n",_tag_dyneither(_tmp7B3,sizeof(char),50))),_tag_dyneither(_tmp7B2,sizeof(void*),1)))))));}
return 1;}
# 1888
{const char*_tmp7B4;Cyc_cycstubs_file=Cyc_fopen((const char*)_untag_dyneither_ptr(Cyc_Filename_concat(Cyc_output_dir,((_tmp7B4="cycstubs.cyc",_tag_dyneither(_tmp7B4,sizeof(char),13)))),sizeof(char),1),"w");}
if(!((unsigned int)Cyc_cycstubs_file)){
{const char*_tmp7B8;void*_tmp7B7[1];struct Cyc_String_pa_PrintArg_struct _tmp7B6;(_tmp7B6.tag=0,((_tmp7B6.f1=(struct _dyneither_ptr)((struct _dyneither_ptr)Cyc_output_dir),((_tmp7B7[0]=& _tmp7B6,Cyc_fprintf(Cyc_stderr,((_tmp7B8="Error: could not create cycstubs.c in directory %s\n",_tag_dyneither(_tmp7B8,sizeof(char),52))),_tag_dyneither(_tmp7B7,sizeof(void*),1)))))));}
# 1893
return 1;}{
# 1895
const char*_tmp7BB;void*_tmp7BA;(_tmp7BA=0,Cyc_fprintf((struct Cyc___cycFILE*)_check_null(Cyc_cycstubs_file),((_tmp7BB="#include <core.h>\nusing Core;\n\n",_tag_dyneither(_tmp7BB,sizeof(char),32))),_tag_dyneither(_tmp7BA,sizeof(void*),0)));};}}{
# 1902
const char*outdir=(const char*)_untag_dyneither_ptr(Cyc_output_dir,sizeof(char),1);
if(Cyc_do_setjmp  && Cyc_process_setjmp(outdir))
return 1;else{
# 1908
for(0;Cyc_spec_files != 0;Cyc_spec_files=((struct Cyc_List_List*)_check_null(Cyc_spec_files))->tl){
if(Cyc_process_specfile((const char*)((struct Cyc_List_List*)_check_null(Cyc_spec_files))->hd,outdir)){
{const char*_tmp7BE;void*_tmp7BD;(_tmp7BD=0,Cyc_fprintf(Cyc_stderr,((_tmp7BE="FATAL ERROR -- QUIT!\n",_tag_dyneither(_tmp7BE,sizeof(char),22))),_tag_dyneither(_tmp7BD,sizeof(void*),0)));}
exit(1);}}}
# 1916
if(Cyc_mode == Cyc_GATHERSCRIPT)
Cyc_fclose((struct Cyc___cycFILE*)_check_null(Cyc_script_file));else{
# 1919
if(!Cyc_gathering()){
Cyc_fclose((struct Cyc___cycFILE*)_check_null(Cyc_log_file));
if(!Cyc_do_setjmp){
Cyc_fclose((struct Cyc___cycFILE*)_check_null(Cyc_cstubs_file));
Cyc_fclose((struct Cyc___cycFILE*)_check_null(Cyc_cycstubs_file));}}}
# 1927
return 0;};};}
