#include <setjmp.h>
/* This is a C header file to be used by the output of the Cyclone to
   C translator.  The corresponding definitions are in file lib/runtime_*.c */
#ifndef _CYC_INCLUDE_H_
#define _CYC_INCLUDE_H_

/* Need one of these per thread (see runtime_stack.c). The runtime maintains 
   a stack that contains either _handler_cons structs or _RegionHandle structs.
   The tag is 0 for a handler_cons and 1 for a region handle.  */
struct _RuntimeStack {
  int tag; 
  struct _RuntimeStack *next;
  void (*cleanup)(struct _RuntimeStack *frame);
};

#ifndef offsetof
/* should be size_t, but int is fine. */
#define offsetof(t,n) ((int)(&(((t *)0)->n)))
#endif

/* Fat pointers */
struct _fat_ptr {
  unsigned char *curr; 
  unsigned char *base; 
  unsigned char *last_plus_one; 
};  

/* Discriminated Unions */
struct _xtunion_struct { char *tag; };

/* Regions */
struct _RegionPage
#ifdef CYC_REGION_PROFILE
{ unsigned total_bytes;
  unsigned free_bytes;
  /* MWH: wish we didn't have to include the stuff below ... */
  struct _RegionPage *next;
  char data[1];
}
#endif
; // abstract -- defined in runtime_memory.c
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
// A dynamic region is just a region handle.  The wrapper struct is for type
// abstraction.
struct Cyc_Core_DynamicRegion {
  struct _RegionHandle h;
};

struct _RegionHandle _new_region(const char*);
void* _region_malloc(struct _RegionHandle*, unsigned);
void* _region_calloc(struct _RegionHandle*, unsigned t, unsigned n);
void   _free_region(struct _RegionHandle*);
struct _RegionHandle*_open_dynregion(struct _DynRegionFrame*,struct _DynRegionHandle*);
void   _pop_dynregion();

/* Exceptions */
struct _handler_cons {
  struct _RuntimeStack s;
  jmp_buf handler;
};
void _push_handler(struct _handler_cons *);
void _push_region(struct _RegionHandle *);
void _npop_handler(int);
void _pop_handler();
void _pop_region();

#ifndef _throw
void* _throw_null_fn(const char*,unsigned);
void* _throw_arraybounds_fn(const char*,unsigned);
void* _throw_badalloc_fn(const char*,unsigned);
void* _throw_match_fn(const char*,unsigned);
void* _throw_fn(void*,const char*,unsigned);
void* _rethrow(void*);
#define _throw_null() (_throw_null_fn(__FILE__,__LINE__))
#define _throw_arraybounds() (_throw_arraybounds_fn(__FILE__,__LINE__))
#define _throw_badalloc() (_throw_badalloc_fn(__FILE__,__LINE__))
#define _throw_match() (_throw_match_fn(__FILE__,__LINE__))
#define _throw(e) (_throw_fn((e),__FILE__,__LINE__))
#endif

struct _xtunion_struct* Cyc_Core_get_exn_thrown();
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
#ifdef CYC_ANSI_OUTPUT
#define _INLINE  
#else
#define _INLINE inline
#endif

#ifdef NO_CYC_NULL_CHECKS
#define _check_null(ptr) (ptr)
#else
#define _check_null(ptr) \
  ({ void*_cks_null = (void*)(ptr); \
     if (!_cks_null) _throw_null(); \
     _cks_null; })
#endif

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_known_subscript_notnull(ptr,bound,elt_sz,index)\
   (((char*)ptr) + (elt_sz)*(index))
#ifdef NO_CYC_NULL_CHECKS
#define _check_known_subscript_null _check_known_subscript_notnull
#else
#define _check_known_subscript_null(ptr,bound,elt_sz,index) ({ \
  char*_cks_ptr = (char*)(ptr);\
  int _index = (index);\
  if (!_cks_ptr) _throw_null(); \
  _cks_ptr + (elt_sz)*_index; })
#endif
#define _zero_arr_plus_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_char_fn _zero_arr_plus_fn
#define _zero_arr_plus_short_fn _zero_arr_plus_fn
#define _zero_arr_plus_int_fn _zero_arr_plus_fn
#define _zero_arr_plus_float_fn _zero_arr_plus_fn
#define _zero_arr_plus_double_fn _zero_arr_plus_fn
#define _zero_arr_plus_longdouble_fn _zero_arr_plus_fn
#define _zero_arr_plus_voidstar_fn _zero_arr_plus_fn
#else
#define _check_known_subscript_null(ptr,bound,elt_sz,index) ({ \
  char*_cks_ptr = (char*)(ptr); \
  unsigned _cks_index = (index); \
  if (!_cks_ptr) _throw_null(); \
  if (_cks_index >= (bound)) _throw_arraybounds(); \
  _cks_ptr + (elt_sz)*_cks_index; })
#define _check_known_subscript_notnull(ptr,bound,elt_sz,index) ({ \
  char*_cks_ptr = (char*)(ptr); \
  unsigned _cks_index = (index); \
  if (_cks_index >= (bound)) _throw_arraybounds(); \
  _cks_ptr + (elt_sz)*_cks_index; })

/* _zero_arr_plus_*_fn(x,sz,i,filename,lineno) adds i to zero-terminated ptr
   x that has at least sz elements */
char* _zero_arr_plus_char_fn(char*,unsigned,int,const char*,unsigned);
short* _zero_arr_plus_short_fn(short*,unsigned,int,const char*,unsigned);
int* _zero_arr_plus_int_fn(int*,unsigned,int,const char*,unsigned);
float* _zero_arr_plus_float_fn(float*,unsigned,int,const char*,unsigned);
double* _zero_arr_plus_double_fn(double*,unsigned,int,const char*,unsigned);
long double* _zero_arr_plus_longdouble_fn(long double*,unsigned,int,const char*, unsigned);
void** _zero_arr_plus_voidstar_fn(void**,unsigned,int,const char*,unsigned);
#endif

/* _get_zero_arr_size_*(x,sz) returns the number of elements in a
   zero-terminated array that is NULL or has at least sz elements */
int _get_zero_arr_size_char(const char*,unsigned);
int _get_zero_arr_size_short(const short*,unsigned);
int _get_zero_arr_size_int(const int*,unsigned);
int _get_zero_arr_size_float(const float*,unsigned);
int _get_zero_arr_size_double(const double*,unsigned);
int _get_zero_arr_size_longdouble(const long double*,unsigned);
int _get_zero_arr_size_voidstar(const void**,unsigned);

/* _zero_arr_inplace_plus_*_fn(x,i,filename,lineno) sets
   zero-terminated pointer *x to *x + i */
char* _zero_arr_inplace_plus_char_fn(char**,int,const char*,unsigned);
short* _zero_arr_inplace_plus_short_fn(short**,int,const char*,unsigned);
int* _zero_arr_inplace_plus_int(int**,int,const char*,unsigned);
float* _zero_arr_inplace_plus_float_fn(float**,int,const char*,unsigned);
double* _zero_arr_inplace_plus_double_fn(double**,int,const char*,unsigned);
long double* _zero_arr_inplace_plus_longdouble_fn(long double**,int,const char*,unsigned);
void** _zero_arr_inplace_plus_voidstar_fn(void***,int,const char*,unsigned);
/* like the previous functions, but does post-addition (as in e++) */
char* _zero_arr_inplace_plus_post_char_fn(char**,int,const char*,unsigned);
short* _zero_arr_inplace_plus_post_short_fn(short**x,int,const char*,unsigned);
int* _zero_arr_inplace_plus_post_int_fn(int**,int,const char*,unsigned);
float* _zero_arr_inplace_plus_post_float_fn(float**,int,const char*,unsigned);
double* _zero_arr_inplace_plus_post_double_fn(double**,int,const char*,unsigned);
long double* _zero_arr_inplace_plus_post_longdouble_fn(long double**,int,const char *,unsigned);
void** _zero_arr_inplace_plus_post_voidstar_fn(void***,int,const char*,unsigned);
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
#define _zero_arr_inplace_plus_char(x,i) \
  _zero_arr_inplace_plus_char_fn((char **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_short(x,i) \
  _zero_arr_inplace_plus_short_fn((short **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_int(x,i) \
  _zero_arr_inplace_plus_int_fn((int **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_float(x,i) \
  _zero_arr_inplace_plus_float_fn((float **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_double(x,i) \
  _zero_arr_inplace_plus_double_fn((double **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_longdouble(x,i) \
  _zero_arr_inplace_plus_longdouble_fn((long double **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_voidstar(x,i) \
  _zero_arr_inplace_plus_voidstar_fn((void ***)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_char(x,i) \
  _zero_arr_inplace_plus_post_char_fn((char **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_short(x,i) \
  _zero_arr_inplace_plus_post_short_fn((short **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_int(x,i) \
  _zero_arr_inplace_plus_post_int_fn((int **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_float(x,i) \
  _zero_arr_inplace_plus_post_float_fn((float **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_double(x,i) \
  _zero_arr_inplace_plus_post_double_fn((double **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_longdouble(x,i) \
  _zero_arr_inplace_plus_post_longdouble_fn((long double **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_voidstar(x,i) \
  _zero_arr_inplace_plus_post_voidstar_fn((void***)(x),(i),__FILE__,__LINE__)

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_fat_subscript(arr,elt_sz,index) ((arr).curr + (elt_sz) * (index))
#define _untag_fat_ptr(arr,elt_sz,num_elts) ((arr).curr)
#else
#define _check_fat_subscript(arr,elt_sz,index) ({ \
  struct _fat_ptr _cus_arr = (arr); \
  unsigned char *_cus_ans = _cus_arr.curr + (elt_sz) * (index); \
  /* JGM: not needed! if (!_cus_arr.base) _throw_null();*/ \
  if (_cus_ans < _cus_arr.base || _cus_ans >= _cus_arr.last_plus_one) \
    _throw_arraybounds(); \
  _cus_ans; })
#define _untag_fat_ptr(arr,elt_sz,num_elts) ({ \
  struct _fat_ptr _arr = (arr); \
  unsigned char *_curr = _arr.curr; \
  if ((_curr < _arr.base || _curr + (elt_sz) * (num_elts) > _arr.last_plus_one) &&\
      _curr != (unsigned char *)0) \
    _throw_arraybounds(); \
  _curr; })
#endif

#define _tag_fat(tcurr,elt_sz,num_elts) ({ \
  struct _fat_ptr _ans; \
  unsigned _num_elts = (num_elts);\
  _ans.base = _ans.curr = (void*)(tcurr); \
  /* JGM: if we're tagging NULL, ignore num_elts */ \
  _ans.last_plus_one = _ans.base ? (_ans.base + (elt_sz) * _num_elts) : 0; \
  _ans; })

#define _get_fat_size(arr,elt_sz) \
  ({struct _fat_ptr _arr = (arr); \
    unsigned char *_arr_curr=_arr.curr; \
    unsigned char *_arr_last=_arr.last_plus_one; \
    (_arr_curr < _arr.base || _arr_curr >= _arr_last) ? 0 : \
    ((_arr_last - _arr_curr) / (elt_sz));})

#define _fat_ptr_plus(arr,elt_sz,change) ({ \
  struct _fat_ptr _ans = (arr); \
  int _change = (change);\
  _ans.curr += (elt_sz) * _change;\
  _ans; })
#define _fat_ptr_inplace_plus(arr_ptr,elt_sz,change) ({ \
  struct _fat_ptr * _arr_ptr = (arr_ptr); \
  _arr_ptr->curr += (elt_sz) * (change);\
  *_arr_ptr; })
#define _fat_ptr_inplace_plus_post(arr_ptr,elt_sz,change) ({ \
  struct _fat_ptr * _arr_ptr = (arr_ptr); \
  struct _fat_ptr _ans = *_arr_ptr; \
  _arr_ptr->curr += (elt_sz) * (change);\
  _ans; })

//Not a macro since initialization order matters. Defined in runtime_zeroterm.c.
struct _fat_ptr _fat_ptr_decrease_size(struct _fat_ptr,unsigned sz,unsigned numelts);

/* Allocation */
void* GC_malloc(int);
void* GC_malloc_atomic(int);
void* GC_calloc(unsigned,unsigned);
void* GC_calloc_atomic(unsigned,unsigned);
// bound the allocation size to be < MAX_ALLOC_SIZE. See macros below for usage.
#define MAX_MALLOC_SIZE (1 << 28)
void* _bounded_GC_malloc(int,const char*,int);
void* _bounded_GC_malloc_atomic(int,const char*,int);
void* _bounded_GC_calloc(unsigned,unsigned,const char*,int);
void* _bounded_GC_calloc_atomic(unsigned,unsigned,const char*,int);
/* these macros are overridden below ifdef CYC_REGION_PROFILE */
#ifndef CYC_REGION_PROFILE
#define _cycalloc(n) _bounded_GC_malloc(n,__FILE__,__LINE__)
#define _cycalloc_atomic(n) _bounded_GC_malloc_atomic(n,__FILE__,__LINE__)
#define _cyccalloc(n,s) _bounded_GC_calloc(n,s,__FILE__,__LINE__)
#define _cyccalloc_atomic(n,s) _bounded_GC_calloc_atomic(n,s,__FILE__,__LINE__)
#endif

static _INLINE unsigned int _check_times(unsigned x, unsigned y) {
  unsigned long long whole_ans = 
    ((unsigned long long) x)*((unsigned long long)y);
  unsigned word_ans = (unsigned)whole_ans;
  if(word_ans < whole_ans || word_ans > MAX_MALLOC_SIZE)
    _throw_badalloc();
  return word_ans;
}

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

#ifdef CYC_REGION_PROFILE
/* see macros below for usage. defined in runtime_memory.c */
void* _profile_GC_malloc(int,const char*,const char*,int);
void* _profile_GC_malloc_atomic(int,const char*,const char*,int);
void* _profile_GC_calloc(unsigned,unsigned,const char*,const char*,int);
void* _profile_GC_calloc_atomic(unsigned,unsigned,const char*,const char*,int);
void* _profile_region_malloc(struct _RegionHandle*,unsigned,const char*,const char*,int);
void* _profile_region_calloc(struct _RegionHandle*,unsigned,unsigned,const char *,const char*,int);
struct _RegionHandle _profile_new_region(const char*,const char*,const char*,int);
void _profile_free_region(struct _RegionHandle*,const char*,const char*,int);
#ifndef RUNTIME_CYC
#define _new_region(n) _profile_new_region(n,__FILE__,__FUNCTION__,__LINE__)
#define _free_region(r) _profile_free_region(r,__FILE__,__FUNCTION__,__LINE__)
#define _region_malloc(rh,n) _profile_region_malloc(rh,n,__FILE__,__FUNCTION__,__LINE__)
#define _region_calloc(rh,n,t) _profile_region_calloc(rh,n,t,__FILE__,__FUNCTION__,__LINE__)
#  endif
#define _cycalloc(n) _profile_GC_malloc(n,__FILE__,__FUNCTION__,__LINE__)
#define _cycalloc_atomic(n) _profile_GC_malloc_atomic(n,__FILE__,__FUNCTION__,__LINE__)
#define _cyccalloc(n,s) _profile_GC_calloc(n,s,__FILE__,__FUNCTION__,__LINE__)
#define _cyccalloc_atomic(n,s) _profile_GC_calloc_atomic(n,s,__FILE__,__FUNCTION__,__LINE__)
#endif
#endif
 struct Cyc_Core_Opt{void*v;};
# 126 "core.h"
int Cyc_Core_ptrcmp(void*,void*);extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 168
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 54 "list.h"
struct Cyc_List_List*Cyc_List_list(struct _fat_ptr);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 319
int Cyc_List_memq(struct Cyc_List_List*l,void*x);
# 383
int Cyc_List_list_cmp(int(*cmp)(void*,void*),struct Cyc_List_List*l1,struct Cyc_List_List*l2);
# 37 "position.h"
struct _fat_ptr Cyc_Position_string_of_segment(unsigned);struct Cyc_Position_Error;struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
# 158 "absyn.h"
enum Cyc_Absyn_Scope{Cyc_Absyn_Static =0U,Cyc_Absyn_Abstract =1U,Cyc_Absyn_Public =2U,Cyc_Absyn_Extern =3U,Cyc_Absyn_ExternC =4U,Cyc_Absyn_Register =5U};struct Cyc_Absyn_Tqual{int print_const: 1;int q_volatile: 1;int q_restrict: 1;int real_const: 1;unsigned loc;};
# 179
enum Cyc_Absyn_Size_of{Cyc_Absyn_Char_sz =0U,Cyc_Absyn_Short_sz =1U,Cyc_Absyn_Int_sz =2U,Cyc_Absyn_Long_sz =3U,Cyc_Absyn_LongLong_sz =4U};
enum Cyc_Absyn_Sign{Cyc_Absyn_Signed =0U,Cyc_Absyn_Unsigned =1U,Cyc_Absyn_None =2U};
enum Cyc_Absyn_AggrKind{Cyc_Absyn_StructA =0U,Cyc_Absyn_UnionA =1U};
# 184
enum Cyc_Absyn_AliasQual{Cyc_Absyn_Aliasable =0U,Cyc_Absyn_Unique =1U,Cyc_Absyn_Top =2U};
# 189
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_RgnKind =3U,Cyc_Absyn_EffKind =4U,Cyc_Absyn_IntKind =5U,Cyc_Absyn_BoolKind =6U,Cyc_Absyn_PtrBndKind =7U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple0*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple0*datatype_name;struct _tuple0*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple1{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple1 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};struct _tuple2{enum Cyc_Absyn_AggrKind f1;struct _tuple0*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple2 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TagCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_UniqueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RefCntCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TrueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FalseCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FatCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple0*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _fat_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_Absyn_Exp*f6;struct Cyc_Absyn_Exp*f7;};
# 412 "absyn.h"
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft =0U,Cyc_Absyn_Scanf_ft =1U};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;void*f1;unsigned f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;unsigned f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned f1;struct Cyc_List_List*f2;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple3{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple3 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple4{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple4 val;};struct _tuple5{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple6 val;};struct _tuple7{struct _fat_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple7 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 501
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U};
# 508
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned f1;};
# 526
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple8{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple8*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple9{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple9 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple9 f2;struct _tuple9 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple9 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple0*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple0*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple0*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple0*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple10{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple10*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 905 "absyn.h"
void*Cyc_Absyn_int_type(enum Cyc_Absyn_Sign,enum Cyc_Absyn_Size_of);
# 987
struct Cyc_Absyn_Exp*Cyc_Absyn_int_exp(enum Cyc_Absyn_Sign,int,unsigned);struct Cyc_Hashtable_Table;
# 39 "hashtable.h"
struct Cyc_Hashtable_Table*Cyc_Hashtable_create(int sz,int(*cmp)(void*,void*),int(*hash)(void*));
# 50
void Cyc_Hashtable_insert(struct Cyc_Hashtable_Table*t,void*key,void*val);
# 52
void*Cyc_Hashtable_lookup(struct Cyc_Hashtable_Table*t,void*key);
# 56
void**Cyc_Hashtable_lookup_opt(struct Cyc_Hashtable_Table*t,void*key);
# 59
void**Cyc_Hashtable_lookup_other_opt(struct Cyc_Hashtable_Table*t,void*key,int(*cmp)(void*,void*),int(*hash)(void*));struct Cyc_JumpAnalysis_Jump_Anal_Result{struct Cyc_Hashtable_Table*pop_tables;struct Cyc_Hashtable_Table*succ_tables;struct Cyc_Hashtable_Table*pat_pop_tables;};
# 27 "vcgen.h"
void Cyc_Vcgen_vcgen_topdecls(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_List_List*ds);struct Cyc___cycFILE;
# 53 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 69 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*);
# 71
struct _fat_ptr Cyc_Absynpp_qvar2string(struct _tuple0*);
# 73
struct _fat_ptr Cyc_Absynpp_prim2string(enum Cyc_Absyn_Primop p);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct Cyc_RgnOrder_RgnPO;
# 30 "tcutil.h"
void*Cyc_Tcutil_impos(struct _fat_ptr,struct _fat_ptr);
# 59
int Cyc_Tcutil_is_noreturn_fn_type(void*);
# 107
void*Cyc_Tcutil_compress(void*);
# 273
int Cyc_Tcutil_is_const_exp(struct Cyc_Absyn_Exp*);
# 46 "evexp.h"
int Cyc_Evexp_exp_cmp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2);struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};
# 62 "dict.h"
struct Cyc_Dict_Dict Cyc_Dict_empty(int(*cmp)(void*,void*));
# 87
struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict d,void*k,void*v);
# 126 "dict.h"
int Cyc_Dict_lookup_bool(struct Cyc_Dict_Dict d,void*k,void**ans);
# 193
struct Cyc_Dict_Dict Cyc_Dict_union_two_c(void*(*f)(void*,void*,void*,void*),void*env,struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2);
# 49 "string.h"
int Cyc_strcmp(struct _fat_ptr s1,struct _fat_ptr s2);
# 35 "warn.h"
void Cyc_Warn_err(unsigned,struct _fat_ptr fmt,struct _fat_ptr);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Qvar_Warn_Warg_struct{int tag;struct _tuple0*f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_TypOpt_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Stmt_Warn_Warg_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Warn_Aggrdecl_Warn_Warg_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Warn_Tvar_Warn_Warg_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Warn_KindBound_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Kind_Warn_Warg_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Warn_Attribute_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Vardecl_Warn_Warg_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Warn_Int_Warn_Warg_struct{int tag;int f1;};struct Cyc_Vcgen_Env;struct Cyc_Vcgen_Const_Vcgen_Term_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Vcgen_Var_Vcgen_Term_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Vcgen_LogicVar_Vcgen_Term_struct{int tag;struct Cyc_Absyn_Vardecl*f1;int f2;};struct Cyc_Vcgen_Primop_Vcgen_Term_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};
# 118 "vcgen.cyc"
struct _fat_ptr Cyc_Vcgen_term2string(void*t){
void*_tmp0=t;void*_tmp2;enum Cyc_Absyn_Primop _tmp1;void*_tmp5;void*_tmp4;enum Cyc_Absyn_Primop _tmp3;int _tmp7;struct Cyc_Absyn_Vardecl*_tmp6;int _tmp8;struct Cyc_Absyn_Vardecl*_tmp9;struct Cyc_Absyn_Exp*_tmpA;switch(*((int*)_tmp0)){case 0U: _LL1: _tmpA=((struct Cyc_Vcgen_Const_Vcgen_Term_struct*)_tmp0)->f1;_LL2: {struct Cyc_Absyn_Exp*c=_tmpA;
return Cyc_Absynpp_exp2string(c);}case 1U: _LL3: _tmp9=((struct Cyc_Vcgen_Var_Vcgen_Term_struct*)_tmp0)->f1;_LL4: {struct Cyc_Absyn_Vardecl*vd=_tmp9;
return Cyc_Absynpp_qvar2string(vd->name);}case 2U: if(((struct Cyc_Vcgen_LogicVar_Vcgen_Term_struct*)_tmp0)->f1 == 0){_LL5: _tmp8=((struct Cyc_Vcgen_LogicVar_Vcgen_Term_struct*)_tmp0)->f2;_LL6: {int i=_tmp8;
return(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmpD=({struct Cyc_Int_pa_PrintArg_struct _tmp2C5;_tmp2C5.tag=1U,_tmp2C5.f1=(unsigned long)i;_tmp2C5;});void*_tmpB[1U];_tmpB[0]=& _tmpD;({struct _fat_ptr _tmp316=({const char*_tmpC="_X%d";_tag_fat(_tmpC,sizeof(char),5U);});Cyc_aprintf(_tmp316,_tag_fat(_tmpB,sizeof(void*),1U));});});}}else{_LL7: _tmp6=((struct Cyc_Vcgen_LogicVar_Vcgen_Term_struct*)_tmp0)->f1;_tmp7=((struct Cyc_Vcgen_LogicVar_Vcgen_Term_struct*)_tmp0)->f2;_LL8: {struct Cyc_Absyn_Vardecl*vd=_tmp6;int i=_tmp7;
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp10=({struct Cyc_String_pa_PrintArg_struct _tmp2C7;_tmp2C7.tag=0U,({struct _fat_ptr _tmp317=(struct _fat_ptr)((struct _fat_ptr)Cyc_Absynpp_qvar2string(((struct Cyc_Absyn_Vardecl*)_check_null(vd))->name));_tmp2C7.f1=_tmp317;});_tmp2C7;});struct Cyc_Int_pa_PrintArg_struct _tmp11=({struct Cyc_Int_pa_PrintArg_struct _tmp2C6;_tmp2C6.tag=1U,_tmp2C6.f1=(unsigned long)i;_tmp2C6;});void*_tmpE[2U];_tmpE[0]=& _tmp10,_tmpE[1]=& _tmp11;({struct _fat_ptr _tmp318=({const char*_tmpF="_%s%d";_tag_fat(_tmpF,sizeof(char),6U);});Cyc_aprintf(_tmp318,_tag_fat(_tmpE,sizeof(void*),2U));});});}}default: if(((struct Cyc_Vcgen_Primop_Vcgen_Term_struct*)_tmp0)->f2 != 0){if(((struct Cyc_List_List*)((struct Cyc_Vcgen_Primop_Vcgen_Term_struct*)_tmp0)->f2)->tl != 0){if(((struct Cyc_List_List*)((struct Cyc_List_List*)((struct Cyc_Vcgen_Primop_Vcgen_Term_struct*)_tmp0)->f2)->tl)->tl == 0){_LL9: _tmp3=((struct Cyc_Vcgen_Primop_Vcgen_Term_struct*)_tmp0)->f1;_tmp4=(void*)(((struct Cyc_Vcgen_Primop_Vcgen_Term_struct*)_tmp0)->f2)->hd;_tmp5=(void*)((((struct Cyc_Vcgen_Primop_Vcgen_Term_struct*)_tmp0)->f2)->tl)->hd;_LLA: {enum Cyc_Absyn_Primop p=_tmp3;void*t1=_tmp4;void*t2=_tmp5;
# 125
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp14=({struct Cyc_String_pa_PrintArg_struct _tmp2CA;_tmp2CA.tag=0U,({struct _fat_ptr _tmp319=(struct _fat_ptr)((struct _fat_ptr)Cyc_Vcgen_term2string(t1));_tmp2CA.f1=_tmp319;});_tmp2CA;});struct Cyc_String_pa_PrintArg_struct _tmp15=({struct Cyc_String_pa_PrintArg_struct _tmp2C9;_tmp2C9.tag=0U,({
struct _fat_ptr _tmp31A=(struct _fat_ptr)((struct _fat_ptr)Cyc_Absynpp_prim2string(p));_tmp2C9.f1=_tmp31A;});_tmp2C9;});struct Cyc_String_pa_PrintArg_struct _tmp16=({struct Cyc_String_pa_PrintArg_struct _tmp2C8;_tmp2C8.tag=0U,({struct _fat_ptr _tmp31B=(struct _fat_ptr)((struct _fat_ptr)Cyc_Vcgen_term2string(t2));_tmp2C8.f1=_tmp31B;});_tmp2C8;});void*_tmp12[3U];_tmp12[0]=& _tmp14,_tmp12[1]=& _tmp15,_tmp12[2]=& _tmp16;({struct _fat_ptr _tmp31C=({const char*_tmp13="(%s%s%s)";_tag_fat(_tmp13,sizeof(char),9U);});Cyc_aprintf(_tmp31C,_tag_fat(_tmp12,sizeof(void*),3U));});});}}else{goto _LLB;}}else{_LLB: _tmp1=((struct Cyc_Vcgen_Primop_Vcgen_Term_struct*)_tmp0)->f1;_tmp2=(void*)(((struct Cyc_Vcgen_Primop_Vcgen_Term_struct*)_tmp0)->f2)->hd;_LLC: {enum Cyc_Absyn_Primop p=_tmp1;void*t=_tmp2;
# 128
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp19=({struct Cyc_String_pa_PrintArg_struct _tmp2CC;_tmp2CC.tag=0U,({struct _fat_ptr _tmp31D=(struct _fat_ptr)((struct _fat_ptr)Cyc_Absynpp_prim2string(p));_tmp2CC.f1=_tmp31D;});_tmp2CC;});struct Cyc_String_pa_PrintArg_struct _tmp1A=({struct Cyc_String_pa_PrintArg_struct _tmp2CB;_tmp2CB.tag=0U,({struct _fat_ptr _tmp31E=(struct _fat_ptr)((struct _fat_ptr)Cyc_Vcgen_term2string(t));_tmp2CB.f1=_tmp31E;});_tmp2CB;});void*_tmp17[2U];_tmp17[0]=& _tmp19,_tmp17[1]=& _tmp1A;({struct _fat_ptr _tmp31F=({const char*_tmp18="%s(%s)";_tag_fat(_tmp18,sizeof(char),7U);});Cyc_aprintf(_tmp31F,_tag_fat(_tmp17,sizeof(void*),2U));});});}}}else{_LLD: _LLE:
({void*_tmp1B=0U;({struct _fat_ptr _tmp320=({const char*_tmp1C="term2string primop args";_tag_fat(_tmp1C,sizeof(char),24U);});((int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos)(_tmp320,_tag_fat(_tmp1B,sizeof(void*),0U));});});}}_LL0:;}
# 133
static void*Cyc_Vcgen_cnst(struct Cyc_Absyn_Exp*c){return(void*)({struct Cyc_Vcgen_Const_Vcgen_Term_struct*_tmp1D=_cycalloc(sizeof(*_tmp1D));_tmp1D->tag=0U,_tmp1D->f1=c;_tmp1D;});}
static void*Cyc_Vcgen_zero(){
static void*z=0;
if(z == 0){
struct Cyc_Absyn_Exp*_tmp1E=Cyc_Absyn_int_exp(Cyc_Absyn_None,0,0U);struct Cyc_Absyn_Exp*e=_tmp1E;
({void*_tmp321=Cyc_Absyn_int_type(Cyc_Absyn_None,Cyc_Absyn_Int_sz);e->topt=_tmp321;});{
void*_tmp1F=Cyc_Vcgen_cnst(e);void*r=_tmp1F;
z=r;
return r;}}else{
return(void*)_check_null(z);}}
# 144
static void*Cyc_Vcgen_one(){
static void*z=0;
if(z == 0){
struct Cyc_Absyn_Exp*_tmp20=Cyc_Absyn_int_exp(Cyc_Absyn_None,1,0U);struct Cyc_Absyn_Exp*e=_tmp20;
({void*_tmp322=Cyc_Absyn_int_type(Cyc_Absyn_None,Cyc_Absyn_Int_sz);e->topt=_tmp322;});{
void*_tmp21=Cyc_Vcgen_cnst(e);void*r=_tmp21;
z=r;
return r;}}else{
return(void*)_check_null(z);}}
# 155
static void*Cyc_Vcgen_var(struct Cyc_Absyn_Vardecl*vd){return(void*)({struct Cyc_Vcgen_Var_Vcgen_Term_struct*_tmp22=_cycalloc(sizeof(*_tmp22));_tmp22->tag=1U,_tmp22->f1=vd;_tmp22;});}
static void*Cyc_Vcgen_primop(enum Cyc_Absyn_Primop p,struct Cyc_List_List*ts){
return(void*)({struct Cyc_Vcgen_Primop_Vcgen_Term_struct*_tmp23=_cycalloc(sizeof(*_tmp23));_tmp23->tag=3U,_tmp23->f1=p,_tmp23->f2=ts;_tmp23;});}
# 159
static void*Cyc_Vcgen_fresh_var(){
static int counter=0;
return(void*)({struct Cyc_Vcgen_LogicVar_Vcgen_Term_struct*_tmp24=_cycalloc(sizeof(*_tmp24));_tmp24->tag=2U,_tmp24->f1=0,_tmp24->f2=counter ++;_tmp24;});}struct _tuple11{void*f1;void*f2;};
# 164
int Cyc_Vcgen_cmp_term(void*t1,void*t2){
struct _tuple11 _tmp25=({struct _tuple11 _tmp2CD;_tmp2CD.f1=t1,_tmp2CD.f2=t2;_tmp2CD;});struct _tuple11 _stmttmp0=_tmp25;struct _tuple11 _tmp26=_stmttmp0;struct Cyc_List_List*_tmp2A;enum Cyc_Absyn_Primop _tmp29;struct Cyc_List_List*_tmp28;enum Cyc_Absyn_Primop _tmp27;int _tmp2E;struct Cyc_Absyn_Vardecl*_tmp2D;int _tmp2C;struct Cyc_Absyn_Vardecl*_tmp2B;struct Cyc_Absyn_Vardecl*_tmp30;struct Cyc_Absyn_Vardecl*_tmp2F;struct Cyc_Absyn_Exp*_tmp32;struct Cyc_Absyn_Exp*_tmp31;if(((struct Cyc_Vcgen_Const_Vcgen_Term_struct*)_tmp26.f1)->tag == 0U){if(((struct Cyc_Vcgen_Const_Vcgen_Term_struct*)_tmp26.f2)->tag == 0U){_LL1: _tmp31=((struct Cyc_Vcgen_Const_Vcgen_Term_struct*)_tmp26.f1)->f1;_tmp32=((struct Cyc_Vcgen_Const_Vcgen_Term_struct*)_tmp26.f2)->f1;_LL2: {struct Cyc_Absyn_Exp*c1=_tmp31;struct Cyc_Absyn_Exp*c2=_tmp32;
return Cyc_Evexp_exp_cmp(c1,c2);}}else{_LL3: _LL4:
 return - 1;}}else{if(((struct Cyc_Vcgen_Const_Vcgen_Term_struct*)_tmp26.f2)->tag == 0U){_LL5: _LL6:
 return 1;}else{if(((struct Cyc_Vcgen_Var_Vcgen_Term_struct*)_tmp26.f1)->tag == 1U){if(((struct Cyc_Vcgen_Var_Vcgen_Term_struct*)_tmp26.f2)->tag == 1U){_LL7: _tmp2F=((struct Cyc_Vcgen_Var_Vcgen_Term_struct*)_tmp26.f1)->f1;_tmp30=((struct Cyc_Vcgen_Var_Vcgen_Term_struct*)_tmp26.f2)->f1;_LL8: {struct Cyc_Absyn_Vardecl*vd1=_tmp2F;struct Cyc_Absyn_Vardecl*vd2=_tmp30;
return(int)vd1 - (int)vd2;}}else{_LL9: _LLA:
 return - 1;}}else{if(((struct Cyc_Vcgen_Var_Vcgen_Term_struct*)_tmp26.f2)->tag == 1U){_LLB: _LLC:
 return 1;}else{if(((struct Cyc_Vcgen_LogicVar_Vcgen_Term_struct*)_tmp26.f1)->tag == 2U){if(((struct Cyc_Vcgen_LogicVar_Vcgen_Term_struct*)_tmp26.f2)->tag == 2U){_LLD: _tmp2B=((struct Cyc_Vcgen_LogicVar_Vcgen_Term_struct*)_tmp26.f1)->f1;_tmp2C=((struct Cyc_Vcgen_LogicVar_Vcgen_Term_struct*)_tmp26.f1)->f2;_tmp2D=((struct Cyc_Vcgen_LogicVar_Vcgen_Term_struct*)_tmp26.f2)->f1;_tmp2E=((struct Cyc_Vcgen_LogicVar_Vcgen_Term_struct*)_tmp26.f2)->f2;_LLE: {struct Cyc_Absyn_Vardecl*vd1=_tmp2B;int i=_tmp2C;struct Cyc_Absyn_Vardecl*vd2=_tmp2D;int j=_tmp2E;
# 173
if(vd1 == vd2)return i - j;
return(int)vd1 - (int)vd2;}}else{_LLF: _LL10:
 return - 1;}}else{if(((struct Cyc_Vcgen_LogicVar_Vcgen_Term_struct*)_tmp26.f2)->tag == 2U){_LL11: _LL12:
 return 1;}else{_LL13: _tmp27=((struct Cyc_Vcgen_Primop_Vcgen_Term_struct*)_tmp26.f1)->f1;_tmp28=((struct Cyc_Vcgen_Primop_Vcgen_Term_struct*)_tmp26.f1)->f2;_tmp29=((struct Cyc_Vcgen_Primop_Vcgen_Term_struct*)_tmp26.f2)->f1;_tmp2A=((struct Cyc_Vcgen_Primop_Vcgen_Term_struct*)_tmp26.f2)->f2;_LL14: {enum Cyc_Absyn_Primop p1=_tmp27;struct Cyc_List_List*ts1=_tmp28;enum Cyc_Absyn_Primop p2=_tmp29;struct Cyc_List_List*ts2=_tmp2A;
# 178
if((int)p1 != (int)p2)return(int)p1 - (int)p2;
return((int(*)(int(*cmp)(void*,void*),struct Cyc_List_List*l1,struct Cyc_List_List*l2))Cyc_List_list_cmp)(Cyc_Vcgen_cmp_term,ts1,ts2);}}}}}}}_LL0:;}
# 183
int Cyc_Vcgen_same_term(void*t1,void*t2){return Cyc_Vcgen_cmp_term(t1,t2)== 0;}
# 188
enum Cyc_Vcgen_Primreln{Cyc_Vcgen_Eq =0U,Cyc_Vcgen_Neq =1U,Cyc_Vcgen_Lt =2U,Cyc_Vcgen_Lte =3U};struct Cyc_Vcgen_True_Vcgen_Assn_struct{int tag;};struct Cyc_Vcgen_False_Vcgen_Assn_struct{int tag;};struct Cyc_Vcgen_Prim_Vcgen_Assn_struct{int tag;void*f1;enum Cyc_Vcgen_Primreln f2;void*f3;};struct Cyc_Vcgen_And_Vcgen_Assn_struct{int tag;void*f1;void*f2;};struct Cyc_Vcgen_Or_Vcgen_Assn_struct{int tag;void*f1;void*f2;};struct Cyc_Vcgen_Subst_Vcgen_Assn_struct{int tag;struct Cyc_Absyn_Vardecl*f1;void*f2;void*f3;};struct Cyc_Vcgen_Kill_Vcgen_Assn_struct{int tag;void*f1;};
# 202
static unsigned Cyc_Vcgen_assn_size_loop(void*a,unsigned c,struct Cyc_List_List**prev){
# 204
if(((int(*)(struct Cyc_List_List*l,void*x))Cyc_List_memq)(*prev,a))return c;
({struct Cyc_List_List*_tmp323=({struct Cyc_List_List*_tmp33=_cycalloc(sizeof(*_tmp33));_tmp33->hd=a,_tmp33->tl=*prev;_tmp33;});*prev=_tmp323;});{
void*_tmp34=a;void*_tmp35;void*_tmp38;void*_tmp37;struct Cyc_Absyn_Vardecl*_tmp36;void*_tmp3A;void*_tmp39;void*_tmp3C;void*_tmp3B;switch(*((int*)_tmp34)){case 0U: _LL1: _LL2:
 return c;case 1U: _LL3: _LL4:
 return c;case 2U: _LL5: _LL6:
 return(unsigned)1 + c;case 3U: _LL7: _tmp3B=(void*)((struct Cyc_Vcgen_And_Vcgen_Assn_struct*)_tmp34)->f1;_tmp3C=(void*)((struct Cyc_Vcgen_And_Vcgen_Assn_struct*)_tmp34)->f2;_LL8: {void*a1=_tmp3B;void*a2=_tmp3C;
_tmp39=a1;_tmp3A=a2;goto _LLA;}case 4U: _LL9: _tmp39=(void*)((struct Cyc_Vcgen_Or_Vcgen_Assn_struct*)_tmp34)->f1;_tmp3A=(void*)((struct Cyc_Vcgen_Or_Vcgen_Assn_struct*)_tmp34)->f2;_LLA: {void*a1=_tmp39;void*a2=_tmp3A;
# 212
return({void*_tmp325=a1;unsigned _tmp324=Cyc_Vcgen_assn_size_loop(a2,(unsigned)1 + c,prev);Cyc_Vcgen_assn_size_loop(_tmp325,_tmp324,prev);});}case 5U: _LLB: _tmp36=((struct Cyc_Vcgen_Subst_Vcgen_Assn_struct*)_tmp34)->f1;_tmp37=(void*)((struct Cyc_Vcgen_Subst_Vcgen_Assn_struct*)_tmp34)->f2;_tmp38=(void*)((struct Cyc_Vcgen_Subst_Vcgen_Assn_struct*)_tmp34)->f3;_LLC: {struct Cyc_Absyn_Vardecl*vd=_tmp36;void*t=_tmp37;void*a=_tmp38;
# 214
return Cyc_Vcgen_assn_size_loop(a,c + (unsigned)1,prev);}default: _LLD: _tmp35=(void*)((struct Cyc_Vcgen_Kill_Vcgen_Assn_struct*)_tmp34)->f1;_LLE: {void*a=_tmp35;
return Cyc_Vcgen_assn_size_loop(a,c + (unsigned)1,prev);}}_LL0:;}}
# 220
static unsigned Cyc_Vcgen_max_assn_size=0U;
# 223
static unsigned Cyc_Vcgen_assn_size(void*a){
struct Cyc_List_List*prev=0;
unsigned _tmp3D=Cyc_Vcgen_assn_size_loop(a,0U,& prev);unsigned c=_tmp3D;
if(c > Cyc_Vcgen_max_assn_size)
Cyc_Vcgen_max_assn_size=c;
# 229
return c;}
# 234
static struct _fat_ptr Cyc_Vcgen_a2string(void*a,int prec,int*size){
void*_tmp3E=a;void*_tmp3F;void*_tmp42;void*_tmp41;struct Cyc_Absyn_Vardecl*_tmp40;void*_tmp44;void*_tmp43;void*_tmp46;void*_tmp45;void*_tmp49;enum Cyc_Vcgen_Primreln _tmp48;void*_tmp47;switch(*((int*)_tmp3E)){case 0U: _LL1: _LL2:
 return({const char*_tmp4A="true";_tag_fat(_tmp4A,sizeof(char),5U);});case 1U: _LL3: _LL4:
 return({const char*_tmp4B="false";_tag_fat(_tmp4B,sizeof(char),6U);});case 2U: _LL5: _tmp47=(void*)((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmp3E)->f1;_tmp48=((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmp3E)->f2;_tmp49=(void*)((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmp3E)->f3;_LL6: {void*t1=_tmp47;enum Cyc_Vcgen_Primreln p=_tmp48;void*t2=_tmp49;
# 239
*size +=1;{
struct _fat_ptr ps;
{enum Cyc_Vcgen_Primreln _tmp4C=p;switch(_tmp4C){case Cyc_Vcgen_Eq: _LL10: _LL11:
 ps=({const char*_tmp4D="==";_tag_fat(_tmp4D,sizeof(char),3U);});goto _LLF;case Cyc_Vcgen_Neq: _LL12: _LL13:
 ps=({const char*_tmp4E="!=";_tag_fat(_tmp4E,sizeof(char),3U);});goto _LLF;case Cyc_Vcgen_Lt: _LL14: _LL15:
 ps=({const char*_tmp4F="<";_tag_fat(_tmp4F,sizeof(char),2U);});goto _LLF;case Cyc_Vcgen_Lte: _LL16: _LL17:
 ps=({const char*_tmp50="<=";_tag_fat(_tmp50,sizeof(char),3U);});goto _LLF;default: _LL18: _LL19:
({void*_tmp51=0U;({struct _fat_ptr _tmp326=({const char*_tmp52="assn2string primop";_tag_fat(_tmp52,sizeof(char),19U);});((int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos)(_tmp326,_tag_fat(_tmp51,sizeof(void*),0U));});});}_LLF:;}
# 248
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp55=({struct Cyc_String_pa_PrintArg_struct _tmp2D0;_tmp2D0.tag=0U,({struct _fat_ptr _tmp327=(struct _fat_ptr)((struct _fat_ptr)Cyc_Vcgen_term2string(t1));_tmp2D0.f1=_tmp327;});_tmp2D0;});struct Cyc_String_pa_PrintArg_struct _tmp56=({struct Cyc_String_pa_PrintArg_struct _tmp2CF;_tmp2CF.tag=0U,_tmp2CF.f1=(struct _fat_ptr)((struct _fat_ptr)ps);_tmp2CF;});struct Cyc_String_pa_PrintArg_struct _tmp57=({struct Cyc_String_pa_PrintArg_struct _tmp2CE;_tmp2CE.tag=0U,({struct _fat_ptr _tmp328=(struct _fat_ptr)((struct _fat_ptr)Cyc_Vcgen_term2string(t2));_tmp2CE.f1=_tmp328;});_tmp2CE;});void*_tmp53[3U];_tmp53[0]=& _tmp55,_tmp53[1]=& _tmp56,_tmp53[2]=& _tmp57;({struct _fat_ptr _tmp329=({const char*_tmp54="%s%s%s";_tag_fat(_tmp54,sizeof(char),7U);});Cyc_aprintf(_tmp329,_tag_fat(_tmp53,sizeof(void*),3U));});});}}case 4U: _LL7: _tmp45=(void*)((struct Cyc_Vcgen_Or_Vcgen_Assn_struct*)_tmp3E)->f1;_tmp46=(void*)((struct Cyc_Vcgen_Or_Vcgen_Assn_struct*)_tmp3E)->f2;_LL8: {void*a1=_tmp45;void*a2=_tmp46;
# 250
*size +=1;
if(prec <= 5)
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp5A=({struct Cyc_String_pa_PrintArg_struct _tmp2D2;_tmp2D2.tag=0U,({struct _fat_ptr _tmp32A=(struct _fat_ptr)((struct _fat_ptr)Cyc_Vcgen_a2string(a1,5,size));_tmp2D2.f1=_tmp32A;});_tmp2D2;});struct Cyc_String_pa_PrintArg_struct _tmp5B=({struct Cyc_String_pa_PrintArg_struct _tmp2D1;_tmp2D1.tag=0U,({struct _fat_ptr _tmp32B=(struct _fat_ptr)((struct _fat_ptr)Cyc_Vcgen_a2string(a2,5,size));_tmp2D1.f1=_tmp32B;});_tmp2D1;});void*_tmp58[2U];_tmp58[0]=& _tmp5A,_tmp58[1]=& _tmp5B;({struct _fat_ptr _tmp32C=({const char*_tmp59="%s || %s";_tag_fat(_tmp59,sizeof(char),9U);});Cyc_aprintf(_tmp32C,_tag_fat(_tmp58,sizeof(void*),2U));});});else{
# 254
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp5E=({struct Cyc_String_pa_PrintArg_struct _tmp2D4;_tmp2D4.tag=0U,({struct _fat_ptr _tmp32D=(struct _fat_ptr)((struct _fat_ptr)Cyc_Vcgen_a2string(a1,5,size));_tmp2D4.f1=_tmp32D;});_tmp2D4;});struct Cyc_String_pa_PrintArg_struct _tmp5F=({struct Cyc_String_pa_PrintArg_struct _tmp2D3;_tmp2D3.tag=0U,({struct _fat_ptr _tmp32E=(struct _fat_ptr)((struct _fat_ptr)Cyc_Vcgen_a2string(a2,5,size));_tmp2D3.f1=_tmp32E;});_tmp2D3;});void*_tmp5C[2U];_tmp5C[0]=& _tmp5E,_tmp5C[1]=& _tmp5F;({struct _fat_ptr _tmp32F=({const char*_tmp5D="(%s || %s)";_tag_fat(_tmp5D,sizeof(char),11U);});Cyc_aprintf(_tmp32F,_tag_fat(_tmp5C,sizeof(void*),2U));});});}}case 3U: _LL9: _tmp43=(void*)((struct Cyc_Vcgen_And_Vcgen_Assn_struct*)_tmp3E)->f1;_tmp44=(void*)((struct Cyc_Vcgen_And_Vcgen_Assn_struct*)_tmp3E)->f2;_LLA: {void*a1=_tmp43;void*a2=_tmp44;
# 256
*size +=1;
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp62=({struct Cyc_String_pa_PrintArg_struct _tmp2D6;_tmp2D6.tag=0U,({struct _fat_ptr _tmp330=(struct _fat_ptr)((struct _fat_ptr)Cyc_Vcgen_a2string(a1,10,size));_tmp2D6.f1=_tmp330;});_tmp2D6;});struct Cyc_String_pa_PrintArg_struct _tmp63=({struct Cyc_String_pa_PrintArg_struct _tmp2D5;_tmp2D5.tag=0U,({struct _fat_ptr _tmp331=(struct _fat_ptr)((struct _fat_ptr)Cyc_Vcgen_a2string(a2,10,size));_tmp2D5.f1=_tmp331;});_tmp2D5;});void*_tmp60[2U];_tmp60[0]=& _tmp62,_tmp60[1]=& _tmp63;({struct _fat_ptr _tmp332=({const char*_tmp61="%s && %s";_tag_fat(_tmp61,sizeof(char),9U);});Cyc_aprintf(_tmp332,_tag_fat(_tmp60,sizeof(void*),2U));});});}case 5U: _LLB: _tmp40=((struct Cyc_Vcgen_Subst_Vcgen_Assn_struct*)_tmp3E)->f1;_tmp41=(void*)((struct Cyc_Vcgen_Subst_Vcgen_Assn_struct*)_tmp3E)->f2;_tmp42=(void*)((struct Cyc_Vcgen_Subst_Vcgen_Assn_struct*)_tmp3E)->f3;_LLC: {struct Cyc_Absyn_Vardecl*vd=_tmp40;void*t=_tmp41;void*a=_tmp42;
# 259
*size +=1;
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp66=({struct Cyc_String_pa_PrintArg_struct _tmp2D9;_tmp2D9.tag=0U,({struct _fat_ptr _tmp333=(struct _fat_ptr)((struct _fat_ptr)Cyc_Absynpp_qvar2string(vd->name));_tmp2D9.f1=_tmp333;});_tmp2D9;});struct Cyc_String_pa_PrintArg_struct _tmp67=({struct Cyc_String_pa_PrintArg_struct _tmp2D8;_tmp2D8.tag=0U,({
struct _fat_ptr _tmp334=(struct _fat_ptr)((struct _fat_ptr)Cyc_Vcgen_term2string(t));_tmp2D8.f1=_tmp334;});_tmp2D8;});struct Cyc_String_pa_PrintArg_struct _tmp68=({struct Cyc_String_pa_PrintArg_struct _tmp2D7;_tmp2D7.tag=0U,({struct _fat_ptr _tmp335=(struct _fat_ptr)((struct _fat_ptr)Cyc_Vcgen_a2string(a,0,size));_tmp2D7.f1=_tmp335;});_tmp2D7;});void*_tmp64[3U];_tmp64[0]=& _tmp66,_tmp64[1]=& _tmp67,_tmp64[2]=& _tmp68;({struct _fat_ptr _tmp336=({const char*_tmp65="Subst[%s:=%s](%s)";_tag_fat(_tmp65,sizeof(char),18U);});Cyc_aprintf(_tmp336,_tag_fat(_tmp64,sizeof(void*),3U));});});}default: _LLD: _tmp3F=(void*)((struct Cyc_Vcgen_Kill_Vcgen_Assn_struct*)_tmp3E)->f1;_LLE: {void*a=_tmp3F;
# 263
*size +=1;
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp6B=({struct Cyc_String_pa_PrintArg_struct _tmp2DA;_tmp2DA.tag=0U,({struct _fat_ptr _tmp337=(struct _fat_ptr)((struct _fat_ptr)Cyc_Vcgen_a2string(a,0,size));_tmp2DA.f1=_tmp337;});_tmp2DA;});void*_tmp69[1U];_tmp69[0]=& _tmp6B;({struct _fat_ptr _tmp338=({const char*_tmp6A="Kill(%s)";_tag_fat(_tmp6A,sizeof(char),9U);});Cyc_aprintf(_tmp338,_tag_fat(_tmp69,sizeof(void*),1U));});});}}_LL0:;}
# 268
static int Cyc_Vcgen_tree_size=0;
# 271
struct _fat_ptr Cyc_Vcgen_assn2string(void*a){
Cyc_Vcgen_tree_size=0;
return Cyc_Vcgen_a2string(a,0,& Cyc_Vcgen_tree_size);}
# 277
static struct Cyc_Vcgen_True_Vcgen_Assn_struct Cyc_Vcgen_true_assn={0U};
static struct Cyc_Vcgen_False_Vcgen_Assn_struct Cyc_Vcgen_false_assn={1U};
# 289 "vcgen.cyc"
static int Cyc_Vcgen_assnhash(void*a){
void*_tmp6C=a;void*_tmp6D;void*_tmp70;void*_tmp6F;struct Cyc_Absyn_Vardecl*_tmp6E;void*_tmp72;void*_tmp71;void*_tmp74;void*_tmp73;void*_tmp77;enum Cyc_Vcgen_Primreln _tmp76;void*_tmp75;switch(*((int*)_tmp6C)){case 1U: _LL1: _LL2:
 return 0;case 0U: _LL3: _LL4:
 return 1;case 2U: _LL5: _tmp75=(void*)((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmp6C)->f1;_tmp76=((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmp6C)->f2;_tmp77=(void*)((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmp6C)->f3;_LL6: {void*t1=_tmp75;enum Cyc_Vcgen_Primreln p=_tmp76;void*t2=_tmp77;
# 294
return(int)(((unsigned)t1 ^ (unsigned)p)^ (unsigned)t2);}case 3U: _LL7: _tmp73=(void*)((struct Cyc_Vcgen_And_Vcgen_Assn_struct*)_tmp6C)->f1;_tmp74=(void*)((struct Cyc_Vcgen_And_Vcgen_Assn_struct*)_tmp6C)->f2;_LL8: {void*a1=_tmp73;void*a2=_tmp74;
return(int)((unsigned)a1 ^ (unsigned)a2);}case 4U: _LL9: _tmp71=(void*)((struct Cyc_Vcgen_Or_Vcgen_Assn_struct*)_tmp6C)->f1;_tmp72=(void*)((struct Cyc_Vcgen_Or_Vcgen_Assn_struct*)_tmp6C)->f2;_LLA: {void*a1=_tmp71;void*a2=_tmp72;
return(int)((unsigned)a1 ^ (unsigned)a2 | (unsigned)1);}case 5U: _LLB: _tmp6E=((struct Cyc_Vcgen_Subst_Vcgen_Assn_struct*)_tmp6C)->f1;_tmp6F=(void*)((struct Cyc_Vcgen_Subst_Vcgen_Assn_struct*)_tmp6C)->f2;_tmp70=(void*)((struct Cyc_Vcgen_Subst_Vcgen_Assn_struct*)_tmp6C)->f3;_LLC: {struct Cyc_Absyn_Vardecl*vd=_tmp6E;void*t=_tmp6F;void*a=_tmp70;
return(int)(((unsigned)a ^ (unsigned)vd)^ (unsigned)t | (unsigned)3);}default: _LLD: _tmp6D=(void*)((struct Cyc_Vcgen_Kill_Vcgen_Assn_struct*)_tmp6C)->f1;_LLE: {void*a=_tmp6D;
# 299
return(int)((unsigned)a + (unsigned)37);}}_LL0:;}
# 305
static int Cyc_Vcgen_assncmp(void*a1,void*a2){
if(a1 == a2)return 0;{
struct _tuple11 _tmp78=({struct _tuple11 _tmp2DB;_tmp2DB.f1=a1,_tmp2DB.f2=a2;_tmp2DB;});struct _tuple11 _stmttmp1=_tmp78;struct _tuple11 _tmp79=_stmttmp1;void*_tmp7F;void*_tmp7E;struct Cyc_Absyn_Vardecl*_tmp7D;void*_tmp7C;void*_tmp7B;struct Cyc_Absyn_Vardecl*_tmp7A;void*_tmp81;void*_tmp80;void*_tmp87;enum Cyc_Vcgen_Primreln _tmp86;void*_tmp85;void*_tmp84;enum Cyc_Vcgen_Primreln _tmp83;void*_tmp82;void*_tmp8B;void*_tmp8A;void*_tmp89;void*_tmp88;void*_tmp8F;void*_tmp8E;void*_tmp8D;void*_tmp8C;if(((struct Cyc_Vcgen_True_Vcgen_Assn_struct*)_tmp79.f1)->tag == 0U){if(((struct Cyc_Vcgen_True_Vcgen_Assn_struct*)_tmp79.f2)->tag == 0U){_LL1: _LL2:
 return 0;}else{_LL3: _LL4:
 return - 1;}}else{if(((struct Cyc_Vcgen_True_Vcgen_Assn_struct*)_tmp79.f2)->tag == 0U){_LL5: _LL6:
 return 1;}else{if(((struct Cyc_Vcgen_False_Vcgen_Assn_struct*)_tmp79.f1)->tag == 1U){if(((struct Cyc_Vcgen_False_Vcgen_Assn_struct*)_tmp79.f2)->tag == 1U){_LL7: _LL8:
 return 0;}else{_LL9: _LLA:
 return - 1;}}else{if(((struct Cyc_Vcgen_False_Vcgen_Assn_struct*)_tmp79.f2)->tag == 1U){_LLB: _LLC:
 return 1;}else{switch(*((int*)_tmp79.f1)){case 4U: if(((struct Cyc_Vcgen_Or_Vcgen_Assn_struct*)_tmp79.f2)->tag == 4U){_LLD: _tmp8C=(void*)((struct Cyc_Vcgen_Or_Vcgen_Assn_struct*)_tmp79.f1)->f1;_tmp8D=(void*)((struct Cyc_Vcgen_Or_Vcgen_Assn_struct*)_tmp79.f1)->f2;_tmp8E=(void*)((struct Cyc_Vcgen_Or_Vcgen_Assn_struct*)_tmp79.f2)->f1;_tmp8F=(void*)((struct Cyc_Vcgen_Or_Vcgen_Assn_struct*)_tmp79.f2)->f2;_LLE: {void*a11=_tmp8C;void*a12=_tmp8D;void*a21=_tmp8E;void*a22=_tmp8F;
_tmp88=a11;_tmp89=a12;_tmp8A=a21;_tmp8B=a22;goto _LL10;}}else{_LL11: _LL12:
# 319
 return - 1;}case 3U: switch(*((int*)_tmp79.f2)){case 3U: _LLF: _tmp88=(void*)((struct Cyc_Vcgen_And_Vcgen_Assn_struct*)_tmp79.f1)->f1;_tmp89=(void*)((struct Cyc_Vcgen_And_Vcgen_Assn_struct*)_tmp79.f1)->f2;_tmp8A=(void*)((struct Cyc_Vcgen_And_Vcgen_Assn_struct*)_tmp79.f2)->f1;_tmp8B=(void*)((struct Cyc_Vcgen_And_Vcgen_Assn_struct*)_tmp79.f2)->f2;_LL10: {void*a11=_tmp88;void*a12=_tmp89;void*a21=_tmp8A;void*a22=_tmp8B;
# 316
int c=Cyc_Vcgen_assncmp(a11,a21);
if(c != 0)return c;
return Cyc_Vcgen_assncmp(a12,a22);}case 4U: goto _LL13;default: _LL15: _LL16:
# 321
 return - 1;}default: switch(*((int*)_tmp79.f2)){case 4U: _LL13: _LL14:
# 320
 return 1;case 3U: _LL17: _LL18:
# 322
 return 1;default: if(((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmp79.f1)->tag == 2U){if(((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmp79.f2)->tag == 2U){_LL19: _tmp82=(void*)((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmp79.f1)->f1;_tmp83=((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmp79.f1)->f2;_tmp84=(void*)((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmp79.f1)->f3;_tmp85=(void*)((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmp79.f2)->f1;_tmp86=((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmp79.f2)->f2;_tmp87=(void*)((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmp79.f2)->f3;_LL1A: {void*t11=_tmp82;enum Cyc_Vcgen_Primreln p1=_tmp83;void*t12=_tmp84;void*t21=_tmp85;enum Cyc_Vcgen_Primreln p2=_tmp86;void*t22=_tmp87;
# 324
int c=(int)p1 - (int)p2;
if(c != 0)return c;
c=Cyc_Vcgen_cmp_term(t11,t21);
if(c != 0)return c;
return Cyc_Vcgen_cmp_term(t12,t22);}}else{_LL1B: _LL1C:
 return - 1;}}else{if(((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmp79.f2)->tag == 2U){_LL1D: _LL1E:
 return 1;}else{if(((struct Cyc_Vcgen_Kill_Vcgen_Assn_struct*)_tmp79.f1)->tag == 6U){if(((struct Cyc_Vcgen_Kill_Vcgen_Assn_struct*)_tmp79.f2)->tag == 6U){_LL1F: _tmp80=(void*)((struct Cyc_Vcgen_Kill_Vcgen_Assn_struct*)_tmp79.f1)->f1;_tmp81=(void*)((struct Cyc_Vcgen_Kill_Vcgen_Assn_struct*)_tmp79.f2)->f1;_LL20: {void*a1=_tmp80;void*a2=_tmp81;
return Cyc_Vcgen_assncmp(a1,a2);}}else{_LL21: _LL22:
 return - 1;}}else{if(((struct Cyc_Vcgen_Kill_Vcgen_Assn_struct*)_tmp79.f2)->tag == 6U){_LL23: _LL24:
 return 1;}else{_LL25: _tmp7A=((struct Cyc_Vcgen_Subst_Vcgen_Assn_struct*)_tmp79.f1)->f1;_tmp7B=(void*)((struct Cyc_Vcgen_Subst_Vcgen_Assn_struct*)_tmp79.f1)->f2;_tmp7C=(void*)((struct Cyc_Vcgen_Subst_Vcgen_Assn_struct*)_tmp79.f1)->f3;_tmp7D=((struct Cyc_Vcgen_Subst_Vcgen_Assn_struct*)_tmp79.f2)->f1;_tmp7E=(void*)((struct Cyc_Vcgen_Subst_Vcgen_Assn_struct*)_tmp79.f2)->f2;_tmp7F=(void*)((struct Cyc_Vcgen_Subst_Vcgen_Assn_struct*)_tmp79.f2)->f3;_LL26: {struct Cyc_Absyn_Vardecl*vd1=_tmp7A;void*t1=_tmp7B;void*a1=_tmp7C;struct Cyc_Absyn_Vardecl*vd2=_tmp7D;void*t2=_tmp7E;void*a2=_tmp7F;
# 335
int c=(int)vd1 - (int)vd2;
if(c != 0)return c;
c=Cyc_Vcgen_cmp_term(t1,t2);
if(c != 0)return c;
return Cyc_Vcgen_assncmp(a1,a2);}}}}}}}}}}}_LL0:;}}
# 346
static struct Cyc_Hashtable_Table*Cyc_Vcgen_hash_cons_table=0;struct _tuple12{void*f1;int f2;};
# 351
static int Cyc_Vcgen_is_reduced(void*a){
struct _tuple12**_tmp90=((struct _tuple12**(*)(struct Cyc_Hashtable_Table*t,void*key))Cyc_Hashtable_lookup_opt)((struct Cyc_Hashtable_Table*)_check_null(Cyc_Vcgen_hash_cons_table),a);struct _tuple12**resopt=_tmp90;
if(resopt == 0)return 0;{
struct _tuple12*_tmp91=*resopt;struct _tuple12*_stmttmp2=_tmp91;struct _tuple12*_tmp92=_stmttmp2;int _tmp93;_LL1: _tmp93=_tmp92->f2;_LL2: {int b=_tmp93;
return b;}}}
# 361
static struct _tuple12 Cyc_Vcgen_copy_assn(void*a){
void*_tmp94=a;void*_tmp95;void*_tmp98;void*_tmp97;struct Cyc_Absyn_Vardecl*_tmp96;void*_tmp9B;enum Cyc_Vcgen_Primreln _tmp9A;void*_tmp99;void*_tmp9D;void*_tmp9C;void*_tmp9F;void*_tmp9E;switch(*((int*)_tmp94)){case 0U: _LL1: _LL2:
 return({struct _tuple12 _tmp2DC;_tmp2DC.f1=(void*)& Cyc_Vcgen_true_assn,_tmp2DC.f2=1;_tmp2DC;});case 1U: _LL3: _LL4:
 return({struct _tuple12 _tmp2DD;_tmp2DD.f1=(void*)& Cyc_Vcgen_false_assn,_tmp2DD.f2=0;_tmp2DD;});case 3U: _LL5: _tmp9E=(void*)((struct Cyc_Vcgen_And_Vcgen_Assn_struct*)_tmp94)->f1;_tmp9F=(void*)((struct Cyc_Vcgen_And_Vcgen_Assn_struct*)_tmp94)->f2;_LL6: {void*a1=_tmp9E;void*a2=_tmp9F;
# 366
return({struct _tuple12 _tmp2DE;({void*_tmp33A=(void*)({struct Cyc_Vcgen_And_Vcgen_Assn_struct*_tmpA0=_cycalloc(sizeof(*_tmpA0));_tmpA0->tag=3U,_tmpA0->f1=a1,_tmpA0->f2=a2;_tmpA0;});_tmp2DE.f1=_tmp33A;}),({int _tmp339=Cyc_Vcgen_is_reduced(a1)&& Cyc_Vcgen_is_reduced(a2);_tmp2DE.f2=_tmp339;});_tmp2DE;});}case 4U: _LL7: _tmp9C=(void*)((struct Cyc_Vcgen_Or_Vcgen_Assn_struct*)_tmp94)->f1;_tmp9D=(void*)((struct Cyc_Vcgen_Or_Vcgen_Assn_struct*)_tmp94)->f2;_LL8: {void*a1=_tmp9C;void*a2=_tmp9D;
# 368
return({struct _tuple12 _tmp2DF;({void*_tmp33C=(void*)({struct Cyc_Vcgen_Or_Vcgen_Assn_struct*_tmpA1=_cycalloc(sizeof(*_tmpA1));_tmpA1->tag=4U,_tmpA1->f1=a1,_tmpA1->f2=a2;_tmpA1;});_tmp2DF.f1=_tmp33C;}),({int _tmp33B=Cyc_Vcgen_is_reduced(a1)&& Cyc_Vcgen_is_reduced(a2);_tmp2DF.f2=_tmp33B;});_tmp2DF;});}case 2U: _LL9: _tmp99=(void*)((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmp94)->f1;_tmp9A=((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmp94)->f2;_tmp9B=(void*)((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmp94)->f3;_LLA: {void*t1=_tmp99;enum Cyc_Vcgen_Primreln p=_tmp9A;void*t2=_tmp9B;
return({struct _tuple12 _tmp2E0;({void*_tmp33D=(void*)({struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*_tmpA2=_cycalloc(sizeof(*_tmpA2));_tmpA2->tag=2U,_tmpA2->f1=t1,_tmpA2->f2=p,_tmpA2->f3=t2;_tmpA2;});_tmp2E0.f1=_tmp33D;}),_tmp2E0.f2=1;_tmp2E0;});}case 5U: _LLB: _tmp96=((struct Cyc_Vcgen_Subst_Vcgen_Assn_struct*)_tmp94)->f1;_tmp97=(void*)((struct Cyc_Vcgen_Subst_Vcgen_Assn_struct*)_tmp94)->f2;_tmp98=(void*)((struct Cyc_Vcgen_Subst_Vcgen_Assn_struct*)_tmp94)->f3;_LLC: {struct Cyc_Absyn_Vardecl*vd1=_tmp96;void*t1=_tmp97;void*a1=_tmp98;
return({struct _tuple12 _tmp2E1;({void*_tmp33E=(void*)({struct Cyc_Vcgen_Subst_Vcgen_Assn_struct*_tmpA3=_cycalloc(sizeof(*_tmpA3));_tmpA3->tag=5U,_tmpA3->f1=vd1,_tmpA3->f2=t1,_tmpA3->f3=a1;_tmpA3;});_tmp2E1.f1=_tmp33E;}),_tmp2E1.f2=0;_tmp2E1;});}default: _LLD: _tmp95=(void*)((struct Cyc_Vcgen_Kill_Vcgen_Assn_struct*)_tmp94)->f1;_LLE: {void*a=_tmp95;
return({struct _tuple12 _tmp2E2;({void*_tmp33F=(void*)({struct Cyc_Vcgen_Kill_Vcgen_Assn_struct*_tmpA4=_cycalloc(sizeof(*_tmpA4));_tmpA4->tag=6U,_tmpA4->f1=a;_tmpA4;});_tmp2E2.f1=_tmp33F;}),_tmp2E2.f2=0;_tmp2E2;});}}_LL0:;}
# 379
static void*Cyc_Vcgen_hash_cons(void*a){
struct Cyc_Hashtable_Table*h;
if(Cyc_Vcgen_hash_cons_table == 0){
h=((struct Cyc_Hashtable_Table*(*)(int sz,int(*cmp)(void*,void*),int(*hash)(void*)))Cyc_Hashtable_create)(123,Cyc_Vcgen_assncmp,Cyc_Vcgen_assnhash);
Cyc_Vcgen_hash_cons_table=h;
# 386
({struct Cyc_Hashtable_Table*_tmp340=h;((void(*)(struct Cyc_Hashtable_Table*t,void*key,struct _tuple12*val))Cyc_Hashtable_insert)(_tmp340,(void*)& Cyc_Vcgen_true_assn,({struct _tuple12*_tmpA5=_cycalloc(sizeof(*_tmpA5));_tmpA5->f1=(void*)& Cyc_Vcgen_true_assn,_tmpA5->f2=1;_tmpA5;}));});
({struct Cyc_Hashtable_Table*_tmp341=h;((void(*)(struct Cyc_Hashtable_Table*t,void*key,struct _tuple12*val))Cyc_Hashtable_insert)(_tmp341,(void*)& Cyc_Vcgen_false_assn,({struct _tuple12*_tmpA6=_cycalloc(sizeof(*_tmpA6));_tmpA6->f1=(void*)& Cyc_Vcgen_false_assn,_tmpA6->f2=1;_tmpA6;}));});}else{
# 389
h=(struct Cyc_Hashtable_Table*)_check_null(Cyc_Vcgen_hash_cons_table);}{
struct _tuple12**_tmpA7=((struct _tuple12**(*)(struct Cyc_Hashtable_Table*t,void*key,int(*cmp)(void*,void*),int(*hash)(void*)))Cyc_Hashtable_lookup_other_opt)(h,a,Cyc_Vcgen_assncmp,Cyc_Vcgen_assnhash);struct _tuple12**resopt=_tmpA7;
if(resopt == 0){
struct _tuple12 _tmpA8=Cyc_Vcgen_copy_assn(a);struct _tuple12 _stmttmp3=_tmpA8;struct _tuple12 _tmpA9=_stmttmp3;int _tmpAB;void*_tmpAA;_LL1: _tmpAA=_tmpA9.f1;_tmpAB=_tmpA9.f2;_LL2: {void*res=_tmpAA;int reduced=_tmpAB;
({struct Cyc_Hashtable_Table*_tmp343=h;void*_tmp342=res;((void(*)(struct Cyc_Hashtable_Table*t,void*key,struct _tuple12*val))Cyc_Hashtable_insert)(_tmp343,_tmp342,({struct _tuple12*_tmpAC=_cycalloc(sizeof(*_tmpAC));_tmpAC->f1=res,_tmpAC->f2=reduced;_tmpAC;}));});
return res;}}else{
# 396
struct _tuple12*_tmpAD=*resopt;struct _tuple12*_stmttmp4=_tmpAD;struct _tuple12*_tmpAE=_stmttmp4;void*_tmpAF;_LL4: _tmpAF=_tmpAE->f1;_LL5: {void*res=_tmpAF;
return res;}}}}
# 402
static void*Cyc_Vcgen_kill(void*a){
void*_tmpB0=a;switch(*((int*)_tmpB0)){case 0U: _LL1: _LL2:
 goto _LL4;case 1U: _LL3: _LL4:
 goto _LL6;case 6U: _LL5: _LL6:
 return a;default: _LL7: _LL8: {
# 408
struct Cyc_Vcgen_Kill_Vcgen_Assn_struct _tmpB1=({struct Cyc_Vcgen_Kill_Vcgen_Assn_struct _tmp2E3;_tmp2E3.tag=6U,_tmp2E3.f1=a;_tmp2E3;});struct Cyc_Vcgen_Kill_Vcgen_Assn_struct p=_tmpB1;
return Cyc_Vcgen_hash_cons((void*)& p);}}_LL0:;}
# 414
static void*Cyc_Vcgen_and(void*a1,void*a2){
if(a1 == a2)return a1;{
struct _tuple11 _tmpB2=({struct _tuple11 _tmp2E5;_tmp2E5.f1=a1,_tmp2E5.f2=a2;_tmp2E5;});struct _tuple11 _stmttmp5=_tmpB2;struct _tuple11 _tmpB3=_stmttmp5;void*_tmpB5;void*_tmpB4;if(((struct Cyc_Vcgen_True_Vcgen_Assn_struct*)_tmpB3.f1)->tag == 0U){_LL1: _LL2:
 return a2;}else{if(((struct Cyc_Vcgen_True_Vcgen_Assn_struct*)_tmpB3.f2)->tag == 0U){_LL3: _LL4:
 return a1;}else{if(((struct Cyc_Vcgen_False_Vcgen_Assn_struct*)_tmpB3.f1)->tag == 1U){_LL5: _LL6:
 return a1;}else{if(((struct Cyc_Vcgen_False_Vcgen_Assn_struct*)_tmpB3.f2)->tag == 1U){_LL7: _LL8:
 return a2;}else{if(((struct Cyc_Vcgen_Kill_Vcgen_Assn_struct*)_tmpB3.f1)->tag == 6U){if(((struct Cyc_Vcgen_Kill_Vcgen_Assn_struct*)_tmpB3.f2)->tag == 6U){_LL9: _tmpB4=(void*)((struct Cyc_Vcgen_Kill_Vcgen_Assn_struct*)_tmpB3.f1)->f1;_tmpB5=(void*)((struct Cyc_Vcgen_Kill_Vcgen_Assn_struct*)_tmpB3.f2)->f1;_LLA: {void*a1=_tmpB4;void*a2=_tmpB5;
# 423
return Cyc_Vcgen_kill(Cyc_Vcgen_and(a1,a2));}}else{goto _LLB;}}else{_LLB: _LLC:
# 428
 if((int)a1 > (int)a2)
({void*_tmpB6=a1;void*_tmpB7=a2;a1=_tmpB7;a2=_tmpB6;});{
struct Cyc_Vcgen_And_Vcgen_Assn_struct _tmpB8=({struct Cyc_Vcgen_And_Vcgen_Assn_struct _tmp2E4;_tmp2E4.tag=3U,_tmp2E4.f1=a1,_tmp2E4.f2=a2;_tmp2E4;});struct Cyc_Vcgen_And_Vcgen_Assn_struct a=_tmpB8;
return Cyc_Vcgen_hash_cons((void*)& a);}}}}}}_LL0:;}}
# 436
static void*Cyc_Vcgen_or(void*a1,void*a2){
if(a1 == a2)return a1;{
struct _tuple11 _tmpB9=({struct _tuple11 _tmp2E7;_tmp2E7.f1=a1,_tmp2E7.f2=a2;_tmp2E7;});struct _tuple11 _stmttmp6=_tmpB9;struct _tuple11 _tmpBA=_stmttmp6;void*_tmpBC;void*_tmpBB;if(((struct Cyc_Vcgen_True_Vcgen_Assn_struct*)_tmpBA.f1)->tag == 0U){_LL1: _LL2:
 return a1;}else{if(((struct Cyc_Vcgen_True_Vcgen_Assn_struct*)_tmpBA.f2)->tag == 0U){_LL3: _LL4:
 return a2;}else{if(((struct Cyc_Vcgen_False_Vcgen_Assn_struct*)_tmpBA.f1)->tag == 1U){_LL5: _LL6:
 return a2;}else{if(((struct Cyc_Vcgen_False_Vcgen_Assn_struct*)_tmpBA.f2)->tag == 1U){_LL7: _LL8:
 return a1;}else{if(((struct Cyc_Vcgen_Kill_Vcgen_Assn_struct*)_tmpBA.f1)->tag == 6U){if(((struct Cyc_Vcgen_Kill_Vcgen_Assn_struct*)_tmpBA.f2)->tag == 6U){_LL9: _tmpBB=(void*)((struct Cyc_Vcgen_Kill_Vcgen_Assn_struct*)_tmpBA.f1)->f1;_tmpBC=(void*)((struct Cyc_Vcgen_Kill_Vcgen_Assn_struct*)_tmpBA.f2)->f1;_LLA: {void*a1=_tmpBB;void*a2=_tmpBC;
# 445
return Cyc_Vcgen_kill(Cyc_Vcgen_or(a1,a2));}}else{goto _LLB;}}else{_LLB: _LLC:
# 448
 if((int)a1 > (int)a2)
({void*_tmpBD=a1;void*_tmpBE=a2;a1=_tmpBE;a2=_tmpBD;});{
struct Cyc_Vcgen_Or_Vcgen_Assn_struct _tmpBF=({struct Cyc_Vcgen_Or_Vcgen_Assn_struct _tmp2E6;_tmp2E6.tag=4U,_tmp2E6.f1=a1,_tmp2E6.f2=a2;_tmp2E6;});struct Cyc_Vcgen_Or_Vcgen_Assn_struct a=_tmpBF;
return Cyc_Vcgen_hash_cons((void*)& a);}}}}}}_LL0:;}}
# 456
static void*Cyc_Vcgen_subst(struct Cyc_Absyn_Vardecl*x,void*newx,void*a){
void*_tmpC0=a;switch(*((int*)_tmpC0)){case 0U: _LL1: _LL2:
 goto _LL4;case 1U: _LL3: _LL4:
 return a;default: _LL5: _LL6: {
# 461
struct Cyc_Vcgen_Subst_Vcgen_Assn_struct _tmpC1=({struct Cyc_Vcgen_Subst_Vcgen_Assn_struct _tmp2E8;_tmp2E8.tag=5U,_tmp2E8.f1=x,_tmp2E8.f2=newx,_tmp2E8.f3=a;_tmp2E8;});struct Cyc_Vcgen_Subst_Vcgen_Assn_struct p=_tmpC1;
return Cyc_Vcgen_hash_cons((void*)& p);}}_LL0:;}
# 467
static void*Cyc_Vcgen_prim(void*t1,enum Cyc_Vcgen_Primreln p,void*t2){
struct Cyc_Vcgen_Prim_Vcgen_Assn_struct _tmpC2=({struct Cyc_Vcgen_Prim_Vcgen_Assn_struct _tmp2E9;_tmp2E9.tag=2U,_tmp2E9.f1=t1,_tmp2E9.f2=p,_tmp2E9.f3=t2;_tmp2E9;});struct Cyc_Vcgen_Prim_Vcgen_Assn_struct ptr=_tmpC2;
return Cyc_Vcgen_hash_cons((void*)& ptr);}
# 477
static void*Cyc_Vcgen_eq(void*t1,void*t2){if(t1 == 0 || t2 == 0)return(void*)& Cyc_Vcgen_true_assn;return Cyc_Vcgen_prim(t1,Cyc_Vcgen_Eq,t2);}
static void*Cyc_Vcgen_neq(void*t1,void*t2){if(t1 == 0 || t2 == 0)return(void*)& Cyc_Vcgen_true_assn;return Cyc_Vcgen_prim(t1,Cyc_Vcgen_Neq,t2);}
static void*Cyc_Vcgen_lt(void*t1,void*t2){if(t1 == 0 || t2 == 0)return(void*)& Cyc_Vcgen_true_assn;return Cyc_Vcgen_prim(t1,Cyc_Vcgen_Lt,t2);}
static void*Cyc_Vcgen_lte(void*t1,void*t2){if(t1 == 0 || t2 == 0)return(void*)& Cyc_Vcgen_true_assn;return Cyc_Vcgen_prim(t1,Cyc_Vcgen_Lte,t2);}struct _tuple13{struct Cyc_Absyn_Vardecl*f1;void*f2;};
# 502 "vcgen.cyc"
static void*Cyc_Vcgen_reduce_term(void*t,struct Cyc_List_List*non_esc_subst,struct Cyc_List_List**esc_subst){
# 505
void*_tmpC3=t;struct Cyc_Absyn_Vardecl*_tmpC4;struct Cyc_List_List*_tmpC6;enum Cyc_Absyn_Primop _tmpC5;switch(*((int*)_tmpC3)){case 0U: _LL1: _LL2:
 goto _LL4;case 2U: _LL3: _LL4:
 return t;case 3U: _LL5: _tmpC5=((struct Cyc_Vcgen_Primop_Vcgen_Term_struct*)_tmpC3)->f1;_tmpC6=((struct Cyc_Vcgen_Primop_Vcgen_Term_struct*)_tmpC3)->f2;_LL6: {enum Cyc_Absyn_Primop p=_tmpC5;struct Cyc_List_List*ts=_tmpC6;
# 509
struct Cyc_List_List*res=0;
int change=0;
{struct Cyc_List_List*_tmpC7=ts;struct Cyc_List_List*ptr=_tmpC7;for(0;ptr != 0;ptr=ptr->tl){
void*_tmpC8=(void*)ptr->hd;void*old_term=_tmpC8;
void*_tmpC9=Cyc_Vcgen_reduce_term(old_term,non_esc_subst,esc_subst);void*new_term=_tmpC9;
res=({struct Cyc_List_List*_tmpCA=_cycalloc(sizeof(*_tmpCA));_tmpCA->hd=new_term,_tmpCA->tl=res;_tmpCA;});
if(new_term != old_term)change=1;}}
# 517
if(!change)return t;
return(void*)({struct Cyc_Vcgen_Primop_Vcgen_Term_struct*_tmpCB=_cycalloc(sizeof(*_tmpCB));_tmpCB->tag=3U,_tmpCB->f1=p,({struct Cyc_List_List*_tmp344=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(res);_tmpCB->f2=_tmp344;});_tmpCB;});}default: _LL7: _tmpC4=((struct Cyc_Vcgen_Var_Vcgen_Term_struct*)_tmpC3)->f1;_LL8: {struct Cyc_Absyn_Vardecl*vd=_tmpC4;
# 520
if(vd->escapes && esc_subst != 0){
# 523
struct Cyc_List_List*_tmpCC=*esc_subst;struct Cyc_List_List*s=_tmpCC;
for(0;s != 0;s=s->tl){
struct _tuple13 _tmpCD=*((struct _tuple13*)s->hd);struct _tuple13 _stmttmp7=_tmpCD;struct _tuple13 _tmpCE=_stmttmp7;void*_tmpD0;struct Cyc_Absyn_Vardecl*_tmpCF;_LLA: _tmpCF=_tmpCE.f1;_tmpD0=_tmpCE.f2;_LLB: {struct Cyc_Absyn_Vardecl*vd2=_tmpCF;void*t2=_tmpD0;
if(vd2 == vd)return t;}}{
# 530
void*_tmpD1=Cyc_Vcgen_fresh_var();void*t=_tmpD1;
({struct Cyc_List_List*_tmp346=({struct Cyc_List_List*_tmpD3=_cycalloc(sizeof(*_tmpD3));({struct _tuple13*_tmp345=({struct _tuple13*_tmpD2=_cycalloc(sizeof(*_tmpD2));_tmpD2->f1=vd,_tmpD2->f2=t;_tmpD2;});_tmpD3->hd=_tmp345;}),_tmpD3->tl=*esc_subst;_tmpD3;});*esc_subst=_tmp346;});
return Cyc_Vcgen_fresh_var();}}else{
if(!vd->escapes)
# 535
for(0;non_esc_subst != 0;non_esc_subst=non_esc_subst->tl){
struct _tuple13 _tmpD4=*((struct _tuple13*)non_esc_subst->hd);struct _tuple13 _stmttmp8=_tmpD4;struct _tuple13 _tmpD5=_stmttmp8;void*_tmpD7;struct Cyc_Absyn_Vardecl*_tmpD6;_LLD: _tmpD6=_tmpD5.f1;_tmpD7=_tmpD5.f2;_LLE: {struct Cyc_Absyn_Vardecl*vd2=_tmpD6;void*t2=_tmpD7;
if(vd == vd2)return t2;}}}
# 540
return t;}}_LL0:;}
# 544
static void*Cyc_Vcgen_reduce_assn(void*a,struct Cyc_List_List*non_esc_subst,struct Cyc_List_List**esc_subst){
# 547
void*_tmpD8=a;void*_tmpDB;void*_tmpDA;struct Cyc_Absyn_Vardecl*_tmpD9;void*_tmpDC;void*_tmpDE;void*_tmpDD;void*_tmpE0;void*_tmpDF;void*_tmpE3;enum Cyc_Vcgen_Primreln _tmpE2;void*_tmpE1;switch(*((int*)_tmpD8)){case 0U: _LL1: _LL2:
 goto _LL4;case 1U: _LL3: _LL4:
 return a;case 2U: _LL5: _tmpE1=(void*)((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmpD8)->f1;_tmpE2=((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmpD8)->f2;_tmpE3=(void*)((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmpD8)->f3;_LL6: {void*t1=_tmpE1;enum Cyc_Vcgen_Primreln p=_tmpE2;void*t2=_tmpE3;
# 551
void*_tmpE4=Cyc_Vcgen_reduce_term(t1,non_esc_subst,esc_subst);void*newt1=_tmpE4;
void*_tmpE5=Cyc_Vcgen_reduce_term(t2,non_esc_subst,esc_subst);void*newt2=_tmpE5;
if(t1 == newt1 && t2 == newt2)return a;
return Cyc_Vcgen_prim(newt1,p,newt2);}case 3U: _LL7: _tmpDF=(void*)((struct Cyc_Vcgen_And_Vcgen_Assn_struct*)_tmpD8)->f1;_tmpE0=(void*)((struct Cyc_Vcgen_And_Vcgen_Assn_struct*)_tmpD8)->f2;_LL8: {void*a1=_tmpDF;void*a2=_tmpE0;
# 556
void*_tmpE6=Cyc_Vcgen_reduce_assn(a1,non_esc_subst,esc_subst);void*newa1=_tmpE6;
void*_tmpE7=Cyc_Vcgen_reduce_assn(a2,non_esc_subst,esc_subst);void*newa2=_tmpE7;
if(a1 == newa1 && a2 == newa2)return a;
return Cyc_Vcgen_and(newa1,newa2);}case 4U: _LL9: _tmpDD=(void*)((struct Cyc_Vcgen_Or_Vcgen_Assn_struct*)_tmpD8)->f1;_tmpDE=(void*)((struct Cyc_Vcgen_Or_Vcgen_Assn_struct*)_tmpD8)->f2;_LLA: {void*a1=_tmpDD;void*a2=_tmpDE;
# 561
void*_tmpE8=Cyc_Vcgen_reduce_assn(a1,non_esc_subst,esc_subst);void*newa1=_tmpE8;
void*_tmpE9=Cyc_Vcgen_reduce_assn(a2,non_esc_subst,esc_subst);void*newa2=_tmpE9;
if(a1 == newa1 && a2 == newa2)return a;
return Cyc_Vcgen_or(newa1,newa2);}case 6U: _LLB: _tmpDC=(void*)((struct Cyc_Vcgen_Kill_Vcgen_Assn_struct*)_tmpD8)->f1;_LLC: {void*a=_tmpDC;
# 568
struct Cyc_List_List*new_esc_subst=0;
return Cyc_Vcgen_reduce_assn(a,non_esc_subst,& new_esc_subst);}default: _LLD: _tmpD9=((struct Cyc_Vcgen_Subst_Vcgen_Assn_struct*)_tmpD8)->f1;_tmpDA=(void*)((struct Cyc_Vcgen_Subst_Vcgen_Assn_struct*)_tmpD8)->f2;_tmpDB=(void*)((struct Cyc_Vcgen_Subst_Vcgen_Assn_struct*)_tmpD8)->f3;_LLE: {struct Cyc_Absyn_Vardecl*vd=_tmpD9;void*t=_tmpDA;void*a=_tmpDB;
# 572
t=Cyc_Vcgen_reduce_term(t,non_esc_subst,esc_subst);
non_esc_subst=({struct Cyc_List_List*_tmpEB=_cycalloc(sizeof(*_tmpEB));({struct _tuple13*_tmp347=({struct _tuple13*_tmpEA=_cycalloc(sizeof(*_tmpEA));_tmpEA->f1=vd,_tmpEA->f2=t;_tmpEA;});_tmpEB->hd=_tmp347;}),_tmpEB->tl=non_esc_subst;_tmpEB;});
return Cyc_Vcgen_reduce_assn(a,non_esc_subst,esc_subst);}}_LL0:;}
# 579
static void*Cyc_Vcgen_reduce(void*a){
# 581
if(Cyc_Vcgen_is_reduced(a))return a;
return Cyc_Vcgen_reduce_assn(a,0,0);}
# 586
static void*Cyc_Vcgen_subst_term(void*t,struct Cyc_Absyn_Vardecl*x,void*newx){
void*_tmpEC=t;struct Cyc_Absyn_Vardecl*_tmpED;struct Cyc_List_List*_tmpEF;enum Cyc_Absyn_Primop _tmpEE;switch(*((int*)_tmpEC)){case 0U: _LL1: _LL2:
 goto _LL4;case 2U: _LL3: _LL4:
 return t;case 3U: _LL5: _tmpEE=((struct Cyc_Vcgen_Primop_Vcgen_Term_struct*)_tmpEC)->f1;_tmpEF=((struct Cyc_Vcgen_Primop_Vcgen_Term_struct*)_tmpEC)->f2;_LL6: {enum Cyc_Absyn_Primop p=_tmpEE;struct Cyc_List_List*ts=_tmpEF;
# 591
struct Cyc_List_List*res=0;
int change=0;
{struct Cyc_List_List*_tmpF0=ts;struct Cyc_List_List*ptr=_tmpF0;for(0;ptr != 0;ptr=ptr->tl){
void*_tmpF1=(void*)ptr->hd;void*old_term=_tmpF1;
void*_tmpF2=Cyc_Vcgen_subst_term(old_term,x,newx);void*new_term=_tmpF2;
res=({struct Cyc_List_List*_tmpF3=_cycalloc(sizeof(*_tmpF3));_tmpF3->hd=new_term,_tmpF3->tl=res;_tmpF3;});
if(new_term != old_term)change=1;}}
# 599
if(!change)return t;
return(void*)({struct Cyc_Vcgen_Primop_Vcgen_Term_struct*_tmpF4=_cycalloc(sizeof(*_tmpF4));_tmpF4->tag=3U,_tmpF4->f1=p,({struct Cyc_List_List*_tmp348=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(res);_tmpF4->f2=_tmp348;});_tmpF4;});}default: _LL7: _tmpED=((struct Cyc_Vcgen_Var_Vcgen_Term_struct*)_tmpEC)->f1;_LL8: {struct Cyc_Absyn_Vardecl*vd=_tmpED;
# 602
if(vd == x)return newx;
return t;}}_LL0:;}
# 608
static void*Cyc_Vcgen_kill_mem_term(void*t){
struct Cyc_List_List*_tmpF5=0;struct Cyc_List_List*esc_subst=_tmpF5;
return Cyc_Vcgen_reduce_term(t,0,& esc_subst);}struct _tuple14{enum Cyc_Vcgen_Primreln f1;enum Cyc_Vcgen_Primreln f2;};
# 618
static int Cyc_Vcgen_find_ctxt(void*ctxt,void*t1,enum Cyc_Vcgen_Primreln p1,void*u1){
void*_tmpF6=ctxt;void*_tmpF9;enum Cyc_Vcgen_Primreln _tmpF8;void*_tmpF7;void*_tmpFB;void*_tmpFA;void*_tmpFD;void*_tmpFC;switch(*((int*)_tmpF6)){case 1U: _LL1: _LL2:
 return 1;case 0U: _LL3: _LL4:
 return 0;case 3U: _LL5: _tmpFC=(void*)((struct Cyc_Vcgen_And_Vcgen_Assn_struct*)_tmpF6)->f1;_tmpFD=(void*)((struct Cyc_Vcgen_And_Vcgen_Assn_struct*)_tmpF6)->f2;_LL6: {void*a1=_tmpFC;void*a2=_tmpFD;
return Cyc_Vcgen_find_ctxt(a1,t1,p1,u1)|| Cyc_Vcgen_find_ctxt(a2,t1,p1,u1);}case 4U: _LL7: _tmpFA=(void*)((struct Cyc_Vcgen_Or_Vcgen_Assn_struct*)_tmpF6)->f1;_tmpFB=(void*)((struct Cyc_Vcgen_Or_Vcgen_Assn_struct*)_tmpF6)->f2;_LL8: {void*a1=_tmpFA;void*a2=_tmpFB;
return Cyc_Vcgen_find_ctxt(a1,t1,p1,u1)&& Cyc_Vcgen_find_ctxt(a2,t1,p1,u1);}case 2U: _LL9: _tmpF7=(void*)((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmpF6)->f1;_tmpF8=((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmpF6)->f2;_tmpF9=(void*)((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmpF6)->f3;_LLA: {void*t2=_tmpF7;enum Cyc_Vcgen_Primreln p2=_tmpF8;void*u2=_tmpF9;
# 625
struct _tuple14 _tmpFE=({struct _tuple14 _tmp2EA;_tmp2EA.f1=p2,_tmp2EA.f2=p1;_tmp2EA;});struct _tuple14 _stmttmp9=_tmpFE;struct _tuple14 _tmpFF=_stmttmp9;switch(_tmpFF.f1){case Cyc_Vcgen_Eq: switch(_tmpFF.f2){case Cyc_Vcgen_Eq: _LLE: _LLF:
 goto _LL11;case Cyc_Vcgen_Lte: _LL10: _LL11:
# 628
 return Cyc_Vcgen_same_term(t1,t2)&& Cyc_Vcgen_same_term(u1,u2)||
 Cyc_Vcgen_same_term(t1,u2)&& Cyc_Vcgen_same_term(u1,t2);default: goto _LL1C;}case Cyc_Vcgen_Lt: switch(_tmpFF.f2){case Cyc_Vcgen_Neq: _LL12: _LL13:
# 631
 return Cyc_Vcgen_same_term(t1,t2)&& Cyc_Vcgen_same_term(u1,u2)||
 Cyc_Vcgen_same_term(t1,u2)&& Cyc_Vcgen_same_term(u1,t2);case Cyc_Vcgen_Lt: _LL14: _LL15:
 goto _LL17;case Cyc_Vcgen_Lte: _LL16: _LL17:
 goto _LL19;default: goto _LL1C;}case Cyc_Vcgen_Lte: if(_tmpFF.f2 == Cyc_Vcgen_Lte){_LL18: _LL19:
 goto _LL1B;}else{goto _LL1C;}case Cyc_Vcgen_Neq: if(_tmpFF.f2 == Cyc_Vcgen_Neq){_LL1A: _LL1B:
# 637
 return Cyc_Vcgen_same_term(t1,t2)&& Cyc_Vcgen_same_term(u1,u2);}else{goto _LL1C;}default: _LL1C: _LL1D:
 return 0;}_LLD:;}default: _LLB: _LLC:
# 640
({void*_tmp100=0U;({struct _fat_ptr _tmp349=({const char*_tmp101="Vcgen::find_ctxt";_tag_fat(_tmp101,sizeof(char),17U);});((int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos)(_tmp349,_tag_fat(_tmp100,sizeof(void*),0U));});});}_LL0:;}
# 647
static int Cyc_Vcgen_simple_prove(void*ctxt,void*a){
if(ctxt == a)return 1;{
void*_tmp102=a;void*_tmp105;enum Cyc_Vcgen_Primreln _tmp104;void*_tmp103;void*_tmp107;void*_tmp106;void*_tmp109;void*_tmp108;switch(*((int*)_tmp102)){case 0U: _LL1: _LL2:
 return 1;case 1U: _LL3: _LL4:
 return 0;case 3U: _LL5: _tmp108=(void*)((struct Cyc_Vcgen_And_Vcgen_Assn_struct*)_tmp102)->f1;_tmp109=(void*)((struct Cyc_Vcgen_And_Vcgen_Assn_struct*)_tmp102)->f2;_LL6: {void*a1=_tmp108;void*a2=_tmp109;
return Cyc_Vcgen_simple_prove(ctxt,a1)&& Cyc_Vcgen_simple_prove(ctxt,a2);}case 4U: _LL7: _tmp106=(void*)((struct Cyc_Vcgen_Or_Vcgen_Assn_struct*)_tmp102)->f1;_tmp107=(void*)((struct Cyc_Vcgen_Or_Vcgen_Assn_struct*)_tmp102)->f2;_LL8: {void*a1=_tmp106;void*a2=_tmp107;
return Cyc_Vcgen_simple_prove(ctxt,a1)|| Cyc_Vcgen_simple_prove(ctxt,a2);}case 2U: _LL9: _tmp103=(void*)((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmp102)->f1;_tmp104=((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmp102)->f2;_tmp105=(void*)((struct Cyc_Vcgen_Prim_Vcgen_Assn_struct*)_tmp102)->f3;_LLA: {void*t1=_tmp103;enum Cyc_Vcgen_Primreln p=_tmp104;void*t2=_tmp105;
return({void*_tmp34C=Cyc_Vcgen_reduce(ctxt);void*_tmp34B=t1;enum Cyc_Vcgen_Primreln _tmp34A=p;Cyc_Vcgen_find_ctxt(_tmp34C,_tmp34B,_tmp34A,t2);});}default: _LLB: _LLC:
({void*_tmp10A=0U;({struct _fat_ptr _tmp34D=({const char*_tmp10B="Vcgen::simple_prove";_tag_fat(_tmp10B,sizeof(char),20U);});((int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos)(_tmp34D,_tag_fat(_tmp10A,sizeof(void*),0U));});});}_LL0:;}}
# 659
static int Cyc_Vcgen_constraint_prove(void*ctxt,void*a){
# 661
return 0;}
# 666
static int Cyc_Vcgen_implies(void*a1,void*a2){
return({void*_tmp34E=a1;Cyc_Vcgen_simple_prove(_tmp34E,Cyc_Vcgen_reduce(a2));})|| Cyc_Vcgen_constraint_prove(a1,a2);}
# 676
static void*Cyc_Vcgen_weaken(void*ctxt,void*a){
a=Cyc_Vcgen_reduce(a);
if(Cyc_Vcgen_implies(ctxt,a))return a;{
void*_tmp10C=a;void*_tmp10E;void*_tmp10D;void*_tmp110;void*_tmp10F;switch(*((int*)_tmp10C)){case 3U: _LL1: _tmp10F=(void*)((struct Cyc_Vcgen_And_Vcgen_Assn_struct*)_tmp10C)->f1;_tmp110=(void*)((struct Cyc_Vcgen_And_Vcgen_Assn_struct*)_tmp10C)->f2;_LL2: {void*a1=_tmp10F;void*a2=_tmp110;
# 681
void*_tmp111=Cyc_Vcgen_weaken(ctxt,a1);void*b1=_tmp111;
void*_tmp112=Cyc_Vcgen_weaken(ctxt,a2);void*b2=_tmp112;
if(Cyc_Vcgen_implies(b1,b2))return b1;
if(Cyc_Vcgen_implies(b2,b1))return b2;
return Cyc_Vcgen_and(b1,b2);}case 4U: _LL3: _tmp10D=(void*)((struct Cyc_Vcgen_Or_Vcgen_Assn_struct*)_tmp10C)->f1;_tmp10E=(void*)((struct Cyc_Vcgen_Or_Vcgen_Assn_struct*)_tmp10C)->f2;_LL4: {void*a1=_tmp10D;void*a2=_tmp10E;
# 687
void*_tmp113=Cyc_Vcgen_weaken(a2,a1);void*b1=_tmp113;
void*_tmp114=Cyc_Vcgen_weaken(a1,a2);void*b2=_tmp114;
void*_tmp115=({void*_tmp34F=ctxt;Cyc_Vcgen_weaken(_tmp34F,Cyc_Vcgen_and(b1,b2));});void*c=_tmp115;
return c;}case 0U: _LL5: _LL6:
 goto _LL8;case 1U: _LL7: _LL8:
 goto _LLA;case 2U: _LL9: _LLA:
 return(void*)& Cyc_Vcgen_true_assn;default: _LLB: _LLC:
({void*_tmp116=0U;({struct _fat_ptr _tmp350=({const char*_tmp117="Vcgen::weaken";_tag_fat(_tmp117,sizeof(char),14U);});((int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos)(_tmp350,_tag_fat(_tmp116,sizeof(void*),0U));});});}_LL0:;}}
# 704
static void*Cyc_Vcgen_widen_assn(void*a1,void*a2){
return Cyc_Vcgen_weaken(a1,a2);}
# 709
static void*Cyc_Vcgen_fresh_nonescvar(struct Cyc_Vcgen_Env*env,struct Cyc_Absyn_Vardecl*vd);
# 712
static void*Cyc_Vcgen_do_assign(struct Cyc_Vcgen_Env*env,void*a,void*tgt,void*src){
void*_tmp118=tgt;struct Cyc_Absyn_Vardecl*_tmp119;if(_tmp118 == 0){_LL1: _LL2:
# 717
 return Cyc_Vcgen_kill(a);}else{if(((struct Cyc_Vcgen_Var_Vcgen_Term_struct*)_tmp118)->tag == 1U){_LL3: _tmp119=((struct Cyc_Vcgen_Var_Vcgen_Term_struct*)_tmp118)->f1;_LL4: {struct Cyc_Absyn_Vardecl*vd=_tmp119;
# 723
if(vd->escapes){
a=Cyc_Vcgen_kill(a);
src=src == 0?0: Cyc_Vcgen_kill_mem_term(src);}else{
# 727
void*_tmp11A=Cyc_Vcgen_fresh_nonescvar(env,vd);void*v2=_tmp11A;
a=Cyc_Vcgen_subst(vd,v2,a);
src=src == 0?0: Cyc_Vcgen_subst_term(src,vd,v2);}
# 732
return({void*_tmp351=a;Cyc_Vcgen_and(_tmp351,Cyc_Vcgen_eq(tgt,src));});}}else{_LL5: _LL6:
({void*_tmp11B=0U;({struct _fat_ptr _tmp352=({const char*_tmp11C="vcgen do_assign";_tag_fat(_tmp11C,sizeof(char),16U);});((int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos)(_tmp352,_tag_fat(_tmp11B,sizeof(void*),0U));});});}}_LL0:;}struct Cyc_Vcgen_SharedEnv{struct Cyc_Hashtable_Table*assn_table;struct Cyc_Hashtable_Table*succ_table;void*try_assn;void**exp_stmt;void*res_term;void*res_assn;};struct Cyc_Vcgen_Env{struct Cyc_Vcgen_SharedEnv*shared;struct Cyc_Dict_Dict state_counter;};
# 771 "vcgen.cyc"
static int Cyc_Vcgen_hash_ptr(void*s){return(int)s;}
# 773
static struct Cyc_Vcgen_Env*Cyc_Vcgen_initial_env(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_Absyn_Fndecl*fd){
# 775
struct Cyc_Hashtable_Table*_tmp11D=((struct Cyc_Hashtable_Table*(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Fndecl*key))Cyc_Hashtable_lookup)(tables->succ_tables,fd);struct Cyc_Hashtable_Table*succ_table=_tmp11D;
struct Cyc_Hashtable_Table*_tmp11E=((struct Cyc_Hashtable_Table*(*)(int sz,int(*cmp)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*),int(*hash)(struct Cyc_Absyn_Stmt*)))Cyc_Hashtable_create)(57,(int(*)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*))Cyc_Core_ptrcmp,(int(*)(struct Cyc_Absyn_Stmt*s))Cyc_Vcgen_hash_ptr);struct Cyc_Hashtable_Table*assn_table=_tmp11E;
struct Cyc_Vcgen_SharedEnv*_tmp11F=({struct Cyc_Vcgen_SharedEnv*_tmp122=_cycalloc(sizeof(*_tmp122));_tmp122->assn_table=assn_table,_tmp122->succ_table=succ_table,_tmp122->try_assn=(void*)& Cyc_Vcgen_false_assn,_tmp122->exp_stmt=0,_tmp122->res_term=0,_tmp122->res_assn=(void*)& Cyc_Vcgen_false_assn;_tmp122;});struct Cyc_Vcgen_SharedEnv*senv=_tmp11F;
# 783
struct Cyc_Dict_Dict _tmp120=((struct Cyc_Dict_Dict(*)(int(*cmp)(struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Vardecl*)))Cyc_Dict_empty)((int(*)(struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Vardecl*))Cyc_Core_ptrcmp);struct Cyc_Dict_Dict d=_tmp120;
return({struct Cyc_Vcgen_Env*_tmp121=_cycalloc(sizeof(*_tmp121));_tmp121->shared=senv,_tmp121->state_counter=d;_tmp121;});}
# 787
static struct Cyc_Vcgen_Env*Cyc_Vcgen_copy_env(struct Cyc_Vcgen_Env*env){
return({struct Cyc_Vcgen_Env*_tmp123=_cycalloc(sizeof(*_tmp123));*_tmp123=*env;_tmp123;});}
# 793
static int Cyc_Vcgen_max_counter(int dummy,struct Cyc_Absyn_Vardecl*vd,int i,int j){
return i < j?j: i;}
# 797
static void Cyc_Vcgen_merge_env(struct Cyc_Vcgen_Env*old_env,struct Cyc_Vcgen_Env*new_env){
({struct Cyc_Dict_Dict _tmp353=((struct Cyc_Dict_Dict(*)(int(*f)(int,struct Cyc_Absyn_Vardecl*,int,int),int env,struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2))Cyc_Dict_union_two_c)(Cyc_Vcgen_max_counter,0,old_env->state_counter,new_env->state_counter);old_env->state_counter=_tmp353;});}
# 805
static void*Cyc_Vcgen_fresh_nonescvar(struct Cyc_Vcgen_Env*env,struct Cyc_Absyn_Vardecl*vd){
int res=0;
((int(*)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Vardecl*k,int*ans))Cyc_Dict_lookup_bool)(env->state_counter,vd,& res);
({struct Cyc_Dict_Dict _tmp354=((struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Vardecl*k,int v))Cyc_Dict_insert)(env->state_counter,vd,res + 1);env->state_counter=_tmp354;});
return(void*)({struct Cyc_Vcgen_LogicVar_Vcgen_Term_struct*_tmp124=_cycalloc(sizeof(*_tmp124));_tmp124->tag=2U,_tmp124->f1=vd,_tmp124->f2=res;_tmp124;});}
# 814
static void*Cyc_Vcgen_pre_stmt_update(struct Cyc_Vcgen_Env*env,struct Cyc_Absyn_Stmt*s,void*ain){
void***_tmp125=((void***(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key))Cyc_Hashtable_lookup_opt)((env->shared)->assn_table,s);void***popt=_tmp125;
if(popt != 0){
void**p=*popt;
void*_tmp126=Cyc_Vcgen_or(ain,*p);void*new_precondition=_tmp126;
*p=new_precondition;
return new_precondition;}else{
# 822
return ain;}}
# 826
static void*Cyc_Vcgen_get_assn(struct Cyc_Vcgen_Env*env,struct Cyc_Absyn_Stmt*s){
void***popt=((void***(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key))Cyc_Hashtable_lookup_opt)((env->shared)->assn_table,s);
if(popt != 0)
return*(*popt);else{
# 831
return(void*)& Cyc_Vcgen_false_assn;}}
# 836
static void*Cyc_Vcgen_loop_assn(struct Cyc_Vcgen_Env*env,struct Cyc_Absyn_Stmt*s,void*ain){
void***aprevopt=((void***(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key))Cyc_Hashtable_lookup_opt)((env->shared)->assn_table,s);
if(aprevopt != 0){
void**_tmp127=*aprevopt;void**aprev=_tmp127;
ain=Cyc_Vcgen_widen_assn(ain,*aprev);
*aprev=ain;}else{
# 843
({struct Cyc_Hashtable_Table*_tmp356=(env->shared)->assn_table;struct Cyc_Absyn_Stmt*_tmp355=s;((void(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key,void**val))Cyc_Hashtable_insert)(_tmp356,_tmp355,({void**_tmp128=_cycalloc(sizeof(*_tmp128));*_tmp128=ain;_tmp128;}));});}
# 845
return ain;}
# 850
static void*Cyc_Vcgen_forward_assn(struct Cyc_Vcgen_Env*env,struct Cyc_Absyn_Stmt*s,void*ain){
void***aprevopt=((void***(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key))Cyc_Hashtable_lookup_opt)((env->shared)->assn_table,s);
if(aprevopt != 0){
void**_tmp129=*aprevopt;void**aprev=_tmp129;
ain=Cyc_Vcgen_or(ain,*aprev);
*aprev=ain;}else{
# 857
({struct Cyc_Hashtable_Table*_tmp358=(env->shared)->assn_table;struct Cyc_Absyn_Stmt*_tmp357=s;((void(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key,void**val))Cyc_Hashtable_insert)(_tmp358,_tmp357,({void**_tmp12A=_cycalloc(sizeof(*_tmp12A));*_tmp12A=ain;_tmp12A;}));});}
# 859
return ain;}
# 864
static void Cyc_Vcgen_update_try_assn(struct Cyc_Vcgen_Env*env,void*a){
({void*_tmp359=Cyc_Vcgen_or((env->shared)->try_assn,a);(env->shared)->try_assn=_tmp359;});}
# 869
static void Cyc_Vcgen_update_return(struct Cyc_Vcgen_Env*env,void*t,void*a){
struct Cyc_Vcgen_SharedEnv*_tmp12B=env->shared;struct Cyc_Vcgen_SharedEnv*shared=_tmp12B;
if(shared->res_term != 0 || t != 0){
void*_tmp12C=Cyc_Vcgen_fresh_var();void*v=_tmp12C;
({void*_tmp35D=({
void*_tmp35C=({void*_tmp35A=a;Cyc_Vcgen_and(_tmp35A,Cyc_Vcgen_eq(v,t));});Cyc_Vcgen_or(_tmp35C,({void*_tmp35B=shared->res_assn;Cyc_Vcgen_and(_tmp35B,Cyc_Vcgen_eq(v,shared->res_term));}));});
# 873
shared->res_assn=_tmp35D;});}else{
# 876
({void*_tmp35E=Cyc_Vcgen_or(a,shared->res_assn);shared->res_assn=_tmp35E;});}}
# 885
static void*Cyc_Vcgen_vcgen_stmt_nodebug(struct Cyc_Vcgen_Env*env,struct Cyc_Absyn_Stmt*s,void*ain);
# 889
static struct _tuple11 Cyc_Vcgen_vcgen_rexp_nodebug(struct Cyc_Vcgen_Env*env,struct Cyc_Absyn_Exp*e,void*ain);
# 891
static struct _tuple11 Cyc_Vcgen_vcgen_rexp_always(struct Cyc_Vcgen_Env*env,struct Cyc_Absyn_Exp*e,void*ain);
# 893
static struct _tuple11 Cyc_Vcgen_vcgen_lexp(struct Cyc_Vcgen_Env*env,struct Cyc_Absyn_Exp*e,void*ain);
# 896
static struct _tuple11 Cyc_Vcgen_vcgen_test(struct Cyc_Vcgen_Env*env,struct Cyc_Absyn_Exp*e,void*ain);struct _tuple15{struct Cyc_List_List**f1;void*f2;};
# 900
static struct _tuple15 Cyc_Vcgen_vcgen_rexps(struct Cyc_Vcgen_Env*env,struct Cyc_List_List*es,void*a);
# 903
static void*Cyc_Vcgen_vcgen_rexps_nosave(struct Cyc_Vcgen_Env*env,struct Cyc_List_List*es,void*a);struct _tuple16{struct Cyc_List_List*f1;void*f2;};
# 905
static struct _tuple16 Cyc_Vcgen_vcgen_rexps_always(struct Cyc_Vcgen_Env*env,struct Cyc_List_List*es,void*ain);
# 907
static void*Cyc_Vcgen_vcgen_switch(struct Cyc_Vcgen_Env*env,struct Cyc_List_List*scs,void*ain);
# 909
static void*Cyc_Vcgen_vcgen_local_decl(struct Cyc_Vcgen_Env*env,struct Cyc_Absyn_Decl*d,void*a);struct _tuple17{struct Cyc_Absyn_Vardecl**f1;struct Cyc_Absyn_Exp*f2;};
# 980 "vcgen.cyc"
static void*Cyc_Vcgen_vcgen_stmt_nodebug(struct Cyc_Vcgen_Env*env,struct Cyc_Absyn_Stmt*s,void*ain){
# 983
ain=Cyc_Vcgen_pre_stmt_update(env,s,ain);
# 987
{void*_tmp12D=ain;if(((struct Cyc_Vcgen_False_Vcgen_Assn_struct*)_tmp12D)->tag == 1U){_LL1: _LL2:
 return ain;}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}{
# 991
void*_tmp12E=s->r;void*_stmttmpA=_tmp12E;void*_tmp12F=_stmttmpA;struct Cyc_List_List*_tmp131;struct Cyc_Absyn_Stmt*_tmp130;struct Cyc_Absyn_Stmt*_tmp132;struct Cyc_Absyn_Stmt*_tmp134;struct Cyc_Absyn_Decl*_tmp133;struct Cyc_Absyn_Switch_clause*_tmp136;struct Cyc_List_List*_tmp135;struct Cyc_List_List*_tmp138;struct Cyc_Absyn_Exp*_tmp137;struct Cyc_Absyn_Stmt*_tmp13B;struct Cyc_Absyn_Exp*_tmp13A;struct Cyc_Absyn_Stmt*_tmp139;struct Cyc_Absyn_Stmt*_tmp141;struct Cyc_Absyn_Stmt*_tmp140;struct Cyc_Absyn_Exp*_tmp13F;struct Cyc_Absyn_Stmt*_tmp13E;struct Cyc_Absyn_Exp*_tmp13D;struct Cyc_Absyn_Exp*_tmp13C;struct Cyc_Absyn_Stmt*_tmp144;struct Cyc_Absyn_Stmt*_tmp143;struct Cyc_Absyn_Exp*_tmp142;struct Cyc_Absyn_Stmt*_tmp147;struct Cyc_Absyn_Stmt*_tmp146;struct Cyc_Absyn_Exp*_tmp145;struct Cyc_Absyn_Exp*_tmp148;struct Cyc_Absyn_Stmt*_tmp14A;struct Cyc_Absyn_Stmt*_tmp149;struct Cyc_Absyn_Exp*_tmp14B;switch(*((int*)_tmp12F)){case 0U: _LL6: _LL7:
 return ain;case 1U: _LL8: _tmp14B=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp12F)->f1;_LL9: {struct Cyc_Absyn_Exp*e=_tmp14B;
# 994
struct _tuple11 _tmp14C=Cyc_Vcgen_vcgen_rexp_nodebug(env,e,ain);struct _tuple11 _stmttmpB=_tmp14C;struct _tuple11 _tmp14D=_stmttmpB;void*_tmp14F;void*_tmp14E;_LL2B: _tmp14E=_tmp14D.f1;_tmp14F=_tmp14D.f2;_LL2C: {void*t=_tmp14E;void*aout=_tmp14F;
# 997
if((env->shared)->exp_stmt != 0)
*((void**)_check_null((env->shared)->exp_stmt))=t;
return aout;}}case 2U: _LLA: _tmp149=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp12F)->f1;_tmp14A=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp12F)->f2;_LLB: {struct Cyc_Absyn_Stmt*s1=_tmp149;struct Cyc_Absyn_Stmt*s2=_tmp14A;
return({struct Cyc_Vcgen_Env*_tmp360=env;struct Cyc_Absyn_Stmt*_tmp35F=s2;Cyc_Vcgen_vcgen_stmt_nodebug(_tmp360,_tmp35F,Cyc_Vcgen_vcgen_stmt_nodebug(env,s1,ain));});}case 3U: if(((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmp12F)->f1 == 0){_LLC: _LLD:
# 1002
 Cyc_Vcgen_update_return(env,0,ain);
return(void*)& Cyc_Vcgen_false_assn;}else{_LLE: _tmp148=((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmp12F)->f1;_LLF: {struct Cyc_Absyn_Exp*e=_tmp148;
# 1005
struct _tuple11 _tmp150=Cyc_Vcgen_vcgen_rexp_nodebug(env,(struct Cyc_Absyn_Exp*)_check_null(e),ain);struct _tuple11 _stmttmpC=_tmp150;struct _tuple11 _tmp151=_stmttmpC;void*_tmp153;void*_tmp152;_LL2E: _tmp152=_tmp151.f1;_tmp153=_tmp151.f2;_LL2F: {void*t=_tmp152;void*aout=_tmp153;
Cyc_Vcgen_update_return(env,t,aout);
return(void*)& Cyc_Vcgen_false_assn;}}}case 4U: _LL10: _tmp145=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp12F)->f1;_tmp146=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp12F)->f2;_tmp147=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp12F)->f3;_LL11: {struct Cyc_Absyn_Exp*e1=_tmp145;struct Cyc_Absyn_Stmt*s1=_tmp146;struct Cyc_Absyn_Stmt*s2=_tmp147;
# 1009
struct _tuple11 _tmp154=Cyc_Vcgen_vcgen_test(env,e1,ain);struct _tuple11 _stmttmpD=_tmp154;struct _tuple11 _tmp155=_stmttmpD;void*_tmp157;void*_tmp156;_LL31: _tmp156=_tmp155.f1;_tmp157=_tmp155.f2;_LL32: {void*at=_tmp156;void*af=_tmp157;
struct Cyc_Vcgen_Env*_tmp158=Cyc_Vcgen_copy_env(env);struct Cyc_Vcgen_Env*env2=_tmp158;
at=Cyc_Vcgen_vcgen_stmt_nodebug(env,s1,at);
af=Cyc_Vcgen_vcgen_stmt_nodebug(env2,s2,af);
Cyc_Vcgen_merge_env(env,env2);
return Cyc_Vcgen_or(at,af);}}case 5U: _LL12: _tmp142=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp12F)->f1).f1;_tmp143=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp12F)->f1).f2;_tmp144=((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp12F)->f2;_LL13: {struct Cyc_Absyn_Exp*e=_tmp142;struct Cyc_Absyn_Stmt*dummy=_tmp143;struct Cyc_Absyn_Stmt*s=_tmp144;
# 1021
while(1){
ain=Cyc_Vcgen_loop_assn(env,dummy,ain);{
struct _tuple11 _tmp159=Cyc_Vcgen_vcgen_test(env,e,ain);struct _tuple11 _stmttmpE=_tmp159;struct _tuple11 _tmp15A=_stmttmpE;void*_tmp15C;void*_tmp15B;_LL34: _tmp15B=_tmp15A.f1;_tmp15C=_tmp15A.f2;_LL35: {void*at=_tmp15B;void*af=_tmp15C;
ain=Cyc_Vcgen_vcgen_stmt_nodebug(env,s,at);
if(({void*_tmp361=ain;Cyc_Vcgen_implies(_tmp361,Cyc_Vcgen_get_assn(env,dummy));}))return af;}}}}case 7U: _LL14: _LL15:
# 1027
 goto _LL17;{
# 1030
struct Cyc_Absyn_Stmt*_tmp15D=((struct Cyc_Absyn_Stmt*(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key))Cyc_Hashtable_lookup)((env->shared)->succ_table,s);struct Cyc_Absyn_Stmt*dest=_tmp15D;
Cyc_Vcgen_loop_assn(env,dest,ain);
return(void*)& Cyc_Vcgen_false_assn;}case 6U: _LL16: _LL17: {
# 1037
struct Cyc_Absyn_Stmt*_tmp15E=((struct Cyc_Absyn_Stmt*(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key))Cyc_Hashtable_lookup)((env->shared)->succ_table,s);struct Cyc_Absyn_Stmt*dest=_tmp15E;
if(dest == 0)
Cyc_Vcgen_update_return(env,0,ain);else{
# 1041
Cyc_Vcgen_forward_assn(env,dest,ain);}
return(void*)& Cyc_Vcgen_false_assn;}case 9U: _LL18: _tmp13C=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp12F)->f1;_tmp13D=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp12F)->f2).f1;_tmp13E=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp12F)->f2).f2;_tmp13F=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp12F)->f3).f1;_tmp140=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp12F)->f3).f2;_tmp141=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp12F)->f4;_LL19: {struct Cyc_Absyn_Exp*e1=_tmp13C;struct Cyc_Absyn_Exp*e2=_tmp13D;struct Cyc_Absyn_Stmt*dummy2=_tmp13E;struct Cyc_Absyn_Exp*e3=_tmp13F;struct Cyc_Absyn_Stmt*dummy3=_tmp140;struct Cyc_Absyn_Stmt*s=_tmp141;
# 1044
struct _tuple11 _tmp15F=Cyc_Vcgen_vcgen_rexp_nodebug(env,e2,ain);struct _tuple11 _stmttmpF=_tmp15F;struct _tuple11 _tmp160=_stmttmpF;void*_tmp161;_LL37: _tmp161=_tmp160.f2;_LL38: {void*a=_tmp161;
ain=a;
# 1047
while(1){
ain=Cyc_Vcgen_loop_assn(env,dummy2,ain);{
struct _tuple11 _tmp162=Cyc_Vcgen_vcgen_test(env,e2,ain);struct _tuple11 _stmttmp10=_tmp162;struct _tuple11 _tmp163=_stmttmp10;void*_tmp165;void*_tmp164;_LL3A: _tmp164=_tmp163.f1;_tmp165=_tmp163.f2;_LL3B: {void*at=_tmp164;void*af=_tmp165;
ain=Cyc_Vcgen_vcgen_stmt_nodebug(env,s,at);
# 1052
ain=Cyc_Vcgen_pre_stmt_update(env,dummy3,ain);
ain=(Cyc_Vcgen_vcgen_rexp_nodebug(env,e3,ain)).f2;
if(({void*_tmp362=ain;Cyc_Vcgen_implies(_tmp362,Cyc_Vcgen_get_assn(env,dummy2));}))return af;}}}}}case 14U: _LL1A: _tmp139=((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp12F)->f1;_tmp13A=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp12F)->f2).f1;_tmp13B=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp12F)->f2).f2;_LL1B: {struct Cyc_Absyn_Stmt*s=_tmp139;struct Cyc_Absyn_Exp*e=_tmp13A;struct Cyc_Absyn_Stmt*dummy=_tmp13B;
# 1058
while(1){
ain=Cyc_Vcgen_vcgen_stmt_nodebug(env,s,ain);{
struct _tuple11 _tmp166=Cyc_Vcgen_vcgen_test(env,e,ain);struct _tuple11 _stmttmp11=_tmp166;struct _tuple11 _tmp167=_stmttmp11;void*_tmp169;void*_tmp168;_LL3D: _tmp168=_tmp167.f1;_tmp169=_tmp167.f2;_LL3E: {void*at=_tmp168;void*af=_tmp169;
ain=({void*_tmp363=Cyc_Vcgen_get_assn(env,dummy);Cyc_Vcgen_widen_assn(_tmp363,ain);});
if(({void*_tmp364=ain;Cyc_Vcgen_implies(_tmp364,Cyc_Vcgen_get_assn(env,dummy));}))return af;
Cyc_Vcgen_loop_assn(env,dummy,ain);}}}}case 10U: _LL1C: _tmp137=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp12F)->f1;_tmp138=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp12F)->f2;_LL1D: {struct Cyc_Absyn_Exp*e=_tmp137;struct Cyc_List_List*scs=_tmp138;
# 1071
struct _tuple11 _tmp16A=Cyc_Vcgen_vcgen_rexp_nodebug(env,e,ain);struct _tuple11 _stmttmp12=_tmp16A;struct _tuple11 _tmp16B=_stmttmp12;void*_tmp16D;void*_tmp16C;_LL40: _tmp16C=_tmp16B.f1;_tmp16D=_tmp16B.f2;_LL41: {void*t=_tmp16C;void*a=_tmp16D;
return Cyc_Vcgen_vcgen_switch(env,scs,a);}}case 11U: if(((struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_tmp12F)->f2 == 0){_LL1E: _LL1F:
({void*_tmp16E=0U;({struct _fat_ptr _tmp365=({const char*_tmp16F="vcgen fallthru";_tag_fat(_tmp16F,sizeof(char),15U);});((int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos)(_tmp365,_tag_fat(_tmp16E,sizeof(void*),0U));});});}else{_LL20: _tmp135=((struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_tmp12F)->f1;_tmp136=*((struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_tmp12F)->f2;_LL21: {struct Cyc_List_List*es=_tmp135;struct Cyc_Absyn_Switch_clause*dest=_tmp136;
# 1076
struct _tuple16 _tmp170=Cyc_Vcgen_vcgen_rexps_always(env,es,ain);struct _tuple16 _stmttmp13=_tmp170;struct _tuple16 _tmp171=_stmttmp13;void*_tmp173;struct Cyc_List_List*_tmp172;_LL43: _tmp172=_tmp171.f1;_tmp173=_tmp171.f2;_LL44: {struct Cyc_List_List*ts=_tmp172;void*a=_tmp173;
# 1078
{struct Cyc_List_List*_tmp174=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(dest->pat_vars))->v;struct Cyc_List_List*x=_tmp174;for(0;x != 0;(x=x->tl,ts=((struct Cyc_List_List*)_check_null(ts))->tl)){
struct _tuple17*_tmp175=(struct _tuple17*)x->hd;struct _tuple17*_stmttmp14=_tmp175;struct _tuple17*_tmp176=_stmttmp14;struct Cyc_Absyn_Exp*_tmp178;struct Cyc_Absyn_Vardecl**_tmp177;_LL46: _tmp177=_tmp176->f1;_tmp178=_tmp176->f2;_LL47: {struct Cyc_Absyn_Vardecl**vd=_tmp177;struct Cyc_Absyn_Exp*ve=_tmp178;
if(vd != 0)
a=({struct Cyc_Vcgen_Env*_tmp368=env;void*_tmp367=a;void*_tmp366=(void*)({struct Cyc_Vcgen_Var_Vcgen_Term_struct*_tmp179=_cycalloc(sizeof(*_tmp179));_tmp179->tag=1U,_tmp179->f1=*vd;_tmp179;});Cyc_Vcgen_do_assign(_tmp368,_tmp367,_tmp366,(void*)((struct Cyc_List_List*)_check_null(ts))->hd);});}}}
# 1083
Cyc_Vcgen_forward_assn(env,dest->body,a);
return(void*)& Cyc_Vcgen_false_assn;}}}case 12U: _LL22: _tmp133=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp12F)->f1;_tmp134=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp12F)->f2;_LL23: {struct Cyc_Absyn_Decl*d=_tmp133;struct Cyc_Absyn_Stmt*s=_tmp134;
# 1086
return({struct Cyc_Vcgen_Env*_tmp36A=env;struct Cyc_Absyn_Stmt*_tmp369=s;Cyc_Vcgen_vcgen_stmt_nodebug(_tmp36A,_tmp369,Cyc_Vcgen_vcgen_local_decl(env,d,ain));});}case 13U: _LL24: _tmp132=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp12F)->f2;_LL25: {struct Cyc_Absyn_Stmt*s=_tmp132;
# 1092
return Cyc_Vcgen_vcgen_stmt_nodebug(env,s,(void*)& Cyc_Vcgen_true_assn);}case 8U: _LL26: _LL27:
 return(void*)& Cyc_Vcgen_false_assn;default: _LL28: _tmp130=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp12F)->f1;_tmp131=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp12F)->f2;_LL29: {struct Cyc_Absyn_Stmt*s=_tmp130;struct Cyc_List_List*scs=_tmp131;
# 1096
void*_tmp17A=(env->shared)->try_assn;void*old_try_assn=_tmp17A;
# 1098
(env->shared)->try_assn=(void*)& Cyc_Vcgen_false_assn;
# 1101
ain=Cyc_Vcgen_vcgen_stmt_nodebug(env,s,ain);{
# 1103
void*_tmp17B=(env->shared)->try_assn;void*new_try_assn=_tmp17B;
(env->shared)->try_assn=old_try_assn;
# 1107
return({void*_tmp36B=ain;Cyc_Vcgen_or(_tmp36B,Cyc_Vcgen_vcgen_switch(env,scs,new_try_assn));});}}}_LL5:;}}
# 1111
static void*Cyc_Vcgen_vcgen_switch(struct Cyc_Vcgen_Env*env,struct Cyc_List_List*scs,void*ain){
# 1115
void*res=(void*)& Cyc_Vcgen_false_assn;
for(0;scs != 0;scs=scs->tl){
struct Cyc_Absyn_Switch_clause*_tmp17C=(struct Cyc_Absyn_Switch_clause*)scs->hd;struct Cyc_Absyn_Switch_clause*sc=_tmp17C;
void*_tmp17D=ain;void*a=_tmp17D;
{struct Cyc_List_List*_tmp17E=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(sc->pat_vars))->v;struct Cyc_List_List*pvs=_tmp17E;for(0;pvs != 0;pvs=pvs->tl){
struct _tuple17*_tmp17F=(struct _tuple17*)pvs->hd;struct _tuple17*_stmttmp15=_tmp17F;struct _tuple17*_tmp180=_stmttmp15;struct Cyc_Absyn_Exp*_tmp182;struct Cyc_Absyn_Vardecl**_tmp181;_LL1: _tmp181=_tmp180->f1;_tmp182=_tmp180->f2;_LL2: {struct Cyc_Absyn_Vardecl**vdopt=_tmp181;struct Cyc_Absyn_Exp*eopt=_tmp182;
if(vdopt != 0){
if(eopt != 0)
a=({struct Cyc_Vcgen_Env*_tmp36E=env;void*_tmp36D=a;void*_tmp36C=(void*)({struct Cyc_Vcgen_Var_Vcgen_Term_struct*_tmp183=_cycalloc(sizeof(*_tmp183));_tmp183->tag=1U,_tmp183->f1=*vdopt;_tmp183;});Cyc_Vcgen_do_assign(_tmp36E,_tmp36D,_tmp36C,(Cyc_Vcgen_vcgen_rexp_nodebug(env,eopt,a)).f1);});else{
# 1125
a=({struct Cyc_Vcgen_Env*_tmp371=env;void*_tmp370=a;void*_tmp36F=(void*)({struct Cyc_Vcgen_Var_Vcgen_Term_struct*_tmp184=_cycalloc(sizeof(*_tmp184));_tmp184->tag=1U,_tmp184->f1=*vdopt;_tmp184;});Cyc_Vcgen_do_assign(_tmp371,_tmp370,_tmp36F,Cyc_Vcgen_fresh_var());});}}}}}
# 1128
if(sc->where_clause != 0)
a=(Cyc_Vcgen_vcgen_test(env,(struct Cyc_Absyn_Exp*)_check_null(sc->where_clause),a)).f1;
res=({void*_tmp372=res;Cyc_Vcgen_or(_tmp372,Cyc_Vcgen_vcgen_stmt_nodebug(env,sc->body,a));});}
# 1132
return res;}
# 1135
static void*Cyc_Vcgen_add_vardecl(struct Cyc_Vcgen_Env*env,struct Cyc_Absyn_Vardecl*vd,void*a){
# 1138
struct Cyc_Vcgen_Var_Vcgen_Term_struct*_tmp185=({struct Cyc_Vcgen_Var_Vcgen_Term_struct*_tmp18A=_cycalloc(sizeof(*_tmp18A));_tmp18A->tag=1U,_tmp18A->f1=vd;_tmp18A;});struct Cyc_Vcgen_Var_Vcgen_Term_struct*tv=_tmp185;
a=({struct Cyc_Vcgen_Env*_tmp375=env;void*_tmp374=a;void*_tmp373=(void*)tv;Cyc_Vcgen_do_assign(_tmp375,_tmp374,_tmp373,Cyc_Vcgen_fresh_var());});
if(vd->initializer != 0){
struct _tuple11 _tmp186=Cyc_Vcgen_vcgen_rexp_nodebug(env,(struct Cyc_Absyn_Exp*)_check_null(vd->initializer),a);struct _tuple11 _stmttmp16=_tmp186;struct _tuple11 _tmp187=_stmttmp16;void*_tmp189;void*_tmp188;_LL1: _tmp188=_tmp187.f1;_tmp189=_tmp187.f2;_LL2: {void*t=_tmp188;void*a1=_tmp189;
a=Cyc_Vcgen_do_assign(env,a1,(void*)tv,t);}}
# 1144
return a;}
# 1147
static void*Cyc_Vcgen_vcgen_local_decl(struct Cyc_Vcgen_Env*env,struct Cyc_Absyn_Decl*d,void*a){
void*_tmp18B=d->r;void*_stmttmp17=_tmp18B;void*_tmp18C=_stmttmp17;struct Cyc_Absyn_Exp*_tmp18E;struct Cyc_Absyn_Vardecl*_tmp18D;struct Cyc_List_List*_tmp18F;struct Cyc_Absyn_Exp*_tmp192;struct Cyc_Core_Opt*_tmp191;struct Cyc_Absyn_Pat*_tmp190;struct Cyc_Absyn_Vardecl*_tmp193;switch(*((int*)_tmp18C)){case 0U: _LL1: _tmp193=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp18C)->f1;_LL2: {struct Cyc_Absyn_Vardecl*vd=_tmp193;
return Cyc_Vcgen_add_vardecl(env,vd,a);}case 2U: _LL3: _tmp190=((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmp18C)->f1;_tmp191=((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmp18C)->f2;_tmp192=((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmp18C)->f3;_LL4: {struct Cyc_Absyn_Pat*p=_tmp190;struct Cyc_Core_Opt*vds=_tmp191;struct Cyc_Absyn_Exp*e=_tmp192;
# 1151
struct _tuple11 _tmp194=Cyc_Vcgen_vcgen_rexp_nodebug(env,e,a);struct _tuple11 _stmttmp18=_tmp194;struct _tuple11 _tmp195=_stmttmp18;void*_tmp196;_LLC: _tmp196=_tmp195.f2;_LLD: {void*a=_tmp196;
{struct Cyc_List_List*_tmp197=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(vds))->v;struct Cyc_List_List*vds=_tmp197;for(0;vds != 0;vds=vds->tl){
struct _tuple17 _tmp198=*((struct _tuple17*)vds->hd);struct _tuple17 _stmttmp19=_tmp198;struct _tuple17 _tmp199=_stmttmp19;struct Cyc_Absyn_Exp*_tmp19B;struct Cyc_Absyn_Vardecl**_tmp19A;_LLF: _tmp19A=_tmp199.f1;_tmp19B=_tmp199.f2;_LL10: {struct Cyc_Absyn_Vardecl**vdopt=_tmp19A;struct Cyc_Absyn_Exp*eopt=_tmp19B;
if(vdopt != 0){
if(eopt == 0)a=Cyc_Vcgen_add_vardecl(env,*vdopt,a);else{
# 1157
struct _tuple11 _tmp19C=Cyc_Vcgen_vcgen_rexp_nodebug(env,e,a);struct _tuple11 _stmttmp1A=_tmp19C;struct _tuple11 _tmp19D=_stmttmp1A;void*_tmp19F;void*_tmp19E;_LL12: _tmp19E=_tmp19D.f1;_tmp19F=_tmp19D.f2;_LL13: {void*t=_tmp19E;void*a1=_tmp19F;
struct Cyc_Vcgen_Var_Vcgen_Term_struct*_tmp1A0=({struct Cyc_Vcgen_Var_Vcgen_Term_struct*_tmp1A1=_cycalloc(sizeof(*_tmp1A1));_tmp1A1->tag=1U,_tmp1A1->f1=*vdopt;_tmp1A1;});struct Cyc_Vcgen_Var_Vcgen_Term_struct*tv=_tmp1A0;
a=Cyc_Vcgen_do_assign(env,a1,(void*)tv,t);}}}}}}
# 1163
return a;}}case 3U: _LL5: _tmp18F=((struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct*)_tmp18C)->f1;_LL6: {struct Cyc_List_List*vds=_tmp18F;
# 1165
for(0;vds != 0;vds=vds->tl){
a=Cyc_Vcgen_add_vardecl(env,(struct Cyc_Absyn_Vardecl*)vds->hd,a);}
return a;}case 4U: _LL7: _tmp18D=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp18C)->f2;_tmp18E=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp18C)->f3;_LL8: {struct Cyc_Absyn_Vardecl*vd=_tmp18D;struct Cyc_Absyn_Exp*eopt=_tmp18E;
# 1169
if(eopt != 0){
struct _tuple11 _tmp1A2=Cyc_Vcgen_vcgen_rexp_nodebug(env,eopt,a);struct _tuple11 _stmttmp1B=_tmp1A2;struct _tuple11 _tmp1A3=_stmttmp1B;void*_tmp1A5;void*_tmp1A4;_LL15: _tmp1A4=_tmp1A3.f1;_tmp1A5=_tmp1A3.f2;_LL16: {void*t=_tmp1A4;void*a1=_tmp1A5;
a=a1;}}
# 1174
a=Cyc_Vcgen_add_vardecl(env,vd,a);
return a;}default: _LL9: _LLA:
 return a;}_LL0:;}
# 1180
static struct _tuple11 Cyc_Vcgen_vcgen_test(struct Cyc_Vcgen_Env*env,struct Cyc_Absyn_Exp*e,void*ain){
void*_tmp1A6=e->r;void*_stmttmp1C=_tmp1A6;void*_tmp1A7=_stmttmp1C;struct Cyc_Absyn_Exp*_tmp1AA;struct Cyc_Absyn_Exp*_tmp1A9;enum Cyc_Absyn_Primop _tmp1A8;struct Cyc_Absyn_Exp*_tmp1AB;struct Cyc_Absyn_Exp*_tmp1AD;struct Cyc_Absyn_Exp*_tmp1AC;struct Cyc_Absyn_Exp*_tmp1AF;struct Cyc_Absyn_Exp*_tmp1AE;struct Cyc_Absyn_Exp*_tmp1B1;struct Cyc_Absyn_Exp*_tmp1B0;struct Cyc_Absyn_Exp*_tmp1B4;struct Cyc_Absyn_Exp*_tmp1B3;struct Cyc_Absyn_Exp*_tmp1B2;switch(*((int*)_tmp1A7)){case 6U: _LL1: _tmp1B2=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp1A7)->f1;_tmp1B3=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp1A7)->f2;_tmp1B4=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp1A7)->f3;_LL2: {struct Cyc_Absyn_Exp*e1=_tmp1B2;struct Cyc_Absyn_Exp*e2=_tmp1B3;struct Cyc_Absyn_Exp*e3=_tmp1B4;
# 1183
struct _tuple11 _tmp1B5=Cyc_Vcgen_vcgen_test(env,e1,ain);struct _tuple11 _stmttmp1D=_tmp1B5;struct _tuple11 _tmp1B6=_stmttmp1D;void*_tmp1B8;void*_tmp1B7;_LL10: _tmp1B7=_tmp1B6.f1;_tmp1B8=_tmp1B6.f2;_LL11: {void*a1t=_tmp1B7;void*a1f=_tmp1B8;
struct _tuple11 _tmp1B9=Cyc_Vcgen_vcgen_test(env,e2,a1t);struct _tuple11 _stmttmp1E=_tmp1B9;struct _tuple11 _tmp1BA=_stmttmp1E;void*_tmp1BC;void*_tmp1BB;_LL13: _tmp1BB=_tmp1BA.f1;_tmp1BC=_tmp1BA.f2;_LL14: {void*a2t=_tmp1BB;void*a2f=_tmp1BC;
struct _tuple11 _tmp1BD=Cyc_Vcgen_vcgen_test(env,e3,a1f);struct _tuple11 _stmttmp1F=_tmp1BD;struct _tuple11 _tmp1BE=_stmttmp1F;void*_tmp1C0;void*_tmp1BF;_LL16: _tmp1BF=_tmp1BE.f1;_tmp1C0=_tmp1BE.f2;_LL17: {void*a3t=_tmp1BF;void*a3f=_tmp1C0;
return({struct _tuple11 _tmp2EB;({void*_tmp377=Cyc_Vcgen_or(a2t,a3t);_tmp2EB.f1=_tmp377;}),({void*_tmp376=Cyc_Vcgen_or(a2f,a3f);_tmp2EB.f2=_tmp376;});_tmp2EB;});}}}}case 7U: _LL3: _tmp1B0=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp1A7)->f1;_tmp1B1=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp1A7)->f2;_LL4: {struct Cyc_Absyn_Exp*e1=_tmp1B0;struct Cyc_Absyn_Exp*e2=_tmp1B1;
# 1188
struct _tuple11 _tmp1C1=Cyc_Vcgen_vcgen_test(env,e1,ain);struct _tuple11 _stmttmp20=_tmp1C1;struct _tuple11 _tmp1C2=_stmttmp20;void*_tmp1C4;void*_tmp1C3;_LL19: _tmp1C3=_tmp1C2.f1;_tmp1C4=_tmp1C2.f2;_LL1A: {void*a1t=_tmp1C3;void*a1f=_tmp1C4;
struct _tuple11 _tmp1C5=Cyc_Vcgen_vcgen_test(env,e2,a1t);struct _tuple11 _stmttmp21=_tmp1C5;struct _tuple11 _tmp1C6=_stmttmp21;void*_tmp1C8;void*_tmp1C7;_LL1C: _tmp1C7=_tmp1C6.f1;_tmp1C8=_tmp1C6.f2;_LL1D: {void*a2t=_tmp1C7;void*a2f=_tmp1C8;
return({struct _tuple11 _tmp2EC;_tmp2EC.f1=a2t,({void*_tmp378=Cyc_Vcgen_or(a1f,a2f);_tmp2EC.f2=_tmp378;});_tmp2EC;});}}}case 8U: _LL5: _tmp1AE=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp1A7)->f1;_tmp1AF=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp1A7)->f2;_LL6: {struct Cyc_Absyn_Exp*e1=_tmp1AE;struct Cyc_Absyn_Exp*e2=_tmp1AF;
# 1192
struct _tuple11 _tmp1C9=Cyc_Vcgen_vcgen_test(env,e1,ain);struct _tuple11 _stmttmp22=_tmp1C9;struct _tuple11 _tmp1CA=_stmttmp22;void*_tmp1CC;void*_tmp1CB;_LL1F: _tmp1CB=_tmp1CA.f1;_tmp1CC=_tmp1CA.f2;_LL20: {void*a1t=_tmp1CB;void*a1f=_tmp1CC;
struct _tuple11 _tmp1CD=Cyc_Vcgen_vcgen_test(env,e2,a1f);struct _tuple11 _stmttmp23=_tmp1CD;struct _tuple11 _tmp1CE=_stmttmp23;void*_tmp1D0;void*_tmp1CF;_LL22: _tmp1CF=_tmp1CE.f1;_tmp1D0=_tmp1CE.f2;_LL23: {void*a2t=_tmp1CF;void*a2f=_tmp1D0;
return({struct _tuple11 _tmp2ED;({void*_tmp379=Cyc_Vcgen_or(a1t,a2t);_tmp2ED.f1=_tmp379;}),_tmp2ED.f2=a2f;_tmp2ED;});}}}case 9U: _LL7: _tmp1AC=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp1A7)->f1;_tmp1AD=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp1A7)->f2;_LL8: {struct Cyc_Absyn_Exp*e1=_tmp1AC;struct Cyc_Absyn_Exp*e2=_tmp1AD;
# 1196
struct _tuple11 _tmp1D1=Cyc_Vcgen_vcgen_rexp_nodebug(env,e1,ain);struct _tuple11 _stmttmp24=_tmp1D1;struct _tuple11 _tmp1D2=_stmttmp24;void*_tmp1D3;_LL25: _tmp1D3=_tmp1D2.f2;_LL26: {void*ain=_tmp1D3;
return Cyc_Vcgen_vcgen_test(env,e2,ain);}}case 3U: if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp1A7)->f1 == Cyc_Absyn_Not){if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp1A7)->f2 != 0){_LL9: _tmp1AB=(struct Cyc_Absyn_Exp*)(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp1A7)->f2)->hd;_LLA: {struct Cyc_Absyn_Exp*e1=_tmp1AB;
# 1199
struct _tuple11 _tmp1D4=Cyc_Vcgen_vcgen_test(env,e1,ain);struct _tuple11 _stmttmp25=_tmp1D4;struct _tuple11 _tmp1D5=_stmttmp25;void*_tmp1D7;void*_tmp1D6;_LL28: _tmp1D6=_tmp1D5.f1;_tmp1D7=_tmp1D5.f2;_LL29: {void*at=_tmp1D6;void*af=_tmp1D7;
return({struct _tuple11 _tmp2EE;_tmp2EE.f1=af,_tmp2EE.f2=at;_tmp2EE;});}}}else{goto _LLD;}}else{if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp1A7)->f2 != 0){if(((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp1A7)->f2)->tl != 0){if(((struct Cyc_List_List*)((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp1A7)->f2)->tl)->tl == 0){_LLB: _tmp1A8=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp1A7)->f1;_tmp1A9=(struct Cyc_Absyn_Exp*)(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp1A7)->f2)->hd;_tmp1AA=(struct Cyc_Absyn_Exp*)((((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp1A7)->f2)->tl)->hd;_LLC: {enum Cyc_Absyn_Primop p=_tmp1A8;struct Cyc_Absyn_Exp*e1=_tmp1A9;struct Cyc_Absyn_Exp*e2=_tmp1AA;
# 1202
struct _tuple11 _tmp1D8=Cyc_Vcgen_vcgen_rexp_nodebug(env,e1,ain);struct _tuple11 _stmttmp26=_tmp1D8;struct _tuple11 _tmp1D9=_stmttmp26;void*_tmp1DB;void*_tmp1DA;_LL2B: _tmp1DA=_tmp1D9.f1;_tmp1DB=_tmp1D9.f2;_LL2C: {void*t1=_tmp1DA;void*a1=_tmp1DB;
struct _tuple11 _tmp1DC=Cyc_Vcgen_vcgen_rexp_nodebug(env,e2,a1);struct _tuple11 _stmttmp27=_tmp1DC;struct _tuple11 _tmp1DD=_stmttmp27;void*_tmp1DF;void*_tmp1DE;_LL2E: _tmp1DE=_tmp1DD.f1;_tmp1DF=_tmp1DD.f2;_LL2F: {void*t2=_tmp1DE;void*a2=_tmp1DF;
void*_tmp1E0=a2;void*at=_tmp1E0;
void*_tmp1E1=a2;void*af=_tmp1E1;
{enum Cyc_Absyn_Primop _tmp1E2=p;switch(_tmp1E2){case Cyc_Absyn_Eq: _LL31: _LL32:
 at=({void*_tmp37A=at;Cyc_Vcgen_and(_tmp37A,Cyc_Vcgen_eq(t1,t2));});af=({void*_tmp37B=af;Cyc_Vcgen_and(_tmp37B,Cyc_Vcgen_neq(t1,t2));});goto _LL30;case Cyc_Absyn_Neq: _LL33: _LL34:
 at=({void*_tmp37C=at;Cyc_Vcgen_and(_tmp37C,Cyc_Vcgen_neq(t1,t2));});af=({void*_tmp37D=af;Cyc_Vcgen_and(_tmp37D,Cyc_Vcgen_eq(t1,t2));});goto _LL30;case Cyc_Absyn_Lt: _LL35: _LL36:
 at=({void*_tmp37E=at;Cyc_Vcgen_and(_tmp37E,Cyc_Vcgen_lt(t1,t2));});af=({void*_tmp37F=af;Cyc_Vcgen_and(_tmp37F,Cyc_Vcgen_lte(t2,t1));});goto _LL30;case Cyc_Absyn_Lte: _LL37: _LL38:
 at=({void*_tmp380=at;Cyc_Vcgen_and(_tmp380,Cyc_Vcgen_lte(t1,t2));});af=({void*_tmp381=af;Cyc_Vcgen_and(_tmp381,Cyc_Vcgen_lt(t2,t1));});goto _LL30;case Cyc_Absyn_Gt: _LL39: _LL3A:
 at=({void*_tmp382=at;Cyc_Vcgen_and(_tmp382,Cyc_Vcgen_lt(t2,t1));});af=({void*_tmp383=af;Cyc_Vcgen_and(_tmp383,Cyc_Vcgen_lte(t1,t2));});goto _LL30;case Cyc_Absyn_Gte: _LL3B: _LL3C:
 at=({void*_tmp384=at;Cyc_Vcgen_and(_tmp384,Cyc_Vcgen_lte(t2,t1));});af=({void*_tmp385=af;Cyc_Vcgen_and(_tmp385,Cyc_Vcgen_lt(t1,t2));});goto _LL30;default: _LL3D: _LL3E:
 goto _LL30;}_LL30:;}
# 1215
return({struct _tuple11 _tmp2EF;_tmp2EF.f1=at,_tmp2EF.f2=af;_tmp2EF;});}}}}else{goto _LLD;}}else{goto _LLD;}}else{goto _LLD;}}default: _LLD: _LLE: {
# 1217
struct _tuple11 _tmp1E3=Cyc_Vcgen_vcgen_rexp_nodebug(env,e,ain);struct _tuple11 _stmttmp28=_tmp1E3;struct _tuple11 _tmp1E4=_stmttmp28;void*_tmp1E6;void*_tmp1E5;_LL40: _tmp1E5=_tmp1E4.f1;_tmp1E6=_tmp1E4.f2;_LL41: {void*t=_tmp1E5;void*aout=_tmp1E6;
return({struct _tuple11 _tmp2F0;({void*_tmp38B=({void*_tmp38A=aout;Cyc_Vcgen_and(_tmp38A,({void*_tmp389=t;Cyc_Vcgen_neq(_tmp389,Cyc_Vcgen_zero());}));});_tmp2F0.f1=_tmp38B;}),({void*_tmp388=({void*_tmp387=aout;Cyc_Vcgen_and(_tmp387,({void*_tmp386=t;Cyc_Vcgen_eq(_tmp386,Cyc_Vcgen_zero());}));});_tmp2F0.f2=_tmp388;});_tmp2F0;});}}}_LL0:;}
# 1222
static void*Cyc_Vcgen_vcgen_rexps_nosave(struct Cyc_Vcgen_Env*env,struct Cyc_List_List*es,void*a){
for(0;es != 0;es=es->tl){
a=(Cyc_Vcgen_vcgen_rexp_nodebug(env,(struct Cyc_Absyn_Exp*)es->hd,a)).f2;}
return a;}
# 1228
static struct _tuple15 Cyc_Vcgen_vcgen_rexps(struct Cyc_Vcgen_Env*env,struct Cyc_List_List*es,void*a){
struct Cyc_List_List*res=0;
int valid=1;
for(0;es != 0;es=es->tl){
struct _tuple11 _tmp1E7=Cyc_Vcgen_vcgen_rexp_nodebug(env,(struct Cyc_Absyn_Exp*)es->hd,a);struct _tuple11 _stmttmp29=_tmp1E7;struct _tuple11 _tmp1E8=_stmttmp29;void*_tmp1EA;void*_tmp1E9;_LL1: _tmp1E9=_tmp1E8.f1;_tmp1EA=_tmp1E8.f2;_LL2: {void*t=_tmp1E9;void*aout=_tmp1EA;
a=aout;
if(valid && t != 0)
res=({struct Cyc_List_List*_tmp1EB=_cycalloc(sizeof(*_tmp1EB));_tmp1EB->hd=t,_tmp1EB->tl=res;_tmp1EB;});else{
# 1237
valid=0;}}}
# 1240
return({struct _tuple15 _tmp2F1;valid?({struct Cyc_List_List**_tmp38D=({struct Cyc_List_List**_tmp1EC=_cycalloc(sizeof(*_tmp1EC));({struct Cyc_List_List*_tmp38C=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(res);*_tmp1EC=_tmp38C;});_tmp1EC;});_tmp2F1.f1=_tmp38D;}):(_tmp2F1.f1=0),_tmp2F1.f2=a;_tmp2F1;});}
# 1243
static struct _tuple11 Cyc_Vcgen_vcgen_rexp_always(struct Cyc_Vcgen_Env*env,struct Cyc_Absyn_Exp*e,void*ain){
struct _tuple11 _tmp1ED=Cyc_Vcgen_vcgen_rexp_nodebug(env,e,ain);struct _tuple11 _stmttmp2A=_tmp1ED;struct _tuple11 _tmp1EE=_stmttmp2A;void*_tmp1F0;void*_tmp1EF;_LL1: _tmp1EF=_tmp1EE.f1;_tmp1F0=_tmp1EE.f2;_LL2: {void*topt=_tmp1EF;void*a=_tmp1F0;
return({struct _tuple11 _tmp2F2;topt == 0?({void*_tmp38E=Cyc_Vcgen_fresh_var();_tmp2F2.f1=_tmp38E;}):(_tmp2F2.f1=topt),_tmp2F2.f2=a;_tmp2F2;});}}
# 1248
static struct _tuple16 Cyc_Vcgen_vcgen_rexps_always(struct Cyc_Vcgen_Env*env,struct Cyc_List_List*es,void*ain){
struct Cyc_List_List*res=0;
for(0;es != 0;es=es->tl){
struct _tuple11 _tmp1F1=Cyc_Vcgen_vcgen_rexp_always(env,(struct Cyc_Absyn_Exp*)es->hd,ain);struct _tuple11 _stmttmp2B=_tmp1F1;struct _tuple11 _tmp1F2=_stmttmp2B;void*_tmp1F4;void*_tmp1F3;_LL1: _tmp1F3=_tmp1F2.f1;_tmp1F4=_tmp1F2.f2;_LL2: {void*t=_tmp1F3;void*a=_tmp1F4;
ain=a;
res=({struct Cyc_List_List*_tmp1F5=_cycalloc(sizeof(*_tmp1F5));_tmp1F5->hd=t,_tmp1F5->tl=res;_tmp1F5;});}}
# 1255
return({struct _tuple16 _tmp2F3;_tmp2F3.f1=res,_tmp2F3.f2=ain;_tmp2F3;});}
# 1258
static struct Cyc_Absyn_Vardecl*Cyc_Vcgen_bind2vardecl(void*b){
void*_tmp1F6=b;struct Cyc_Absyn_Vardecl*_tmp1F7;struct Cyc_Absyn_Vardecl*_tmp1F8;struct Cyc_Absyn_Vardecl*_tmp1F9;struct Cyc_Absyn_Fndecl*_tmp1FA;struct Cyc_Absyn_Vardecl*_tmp1FB;switch(*((int*)_tmp1F6)){case 1U: _LL1: _tmp1FB=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)_tmp1F6)->f1;_LL2: {struct Cyc_Absyn_Vardecl*vd=_tmp1FB;
return vd;}case 2U: _LL3: _tmp1FA=((struct Cyc_Absyn_Funname_b_Absyn_Binding_struct*)_tmp1F6)->f1;_LL4: {struct Cyc_Absyn_Fndecl*fd=_tmp1FA;
return(struct Cyc_Absyn_Vardecl*)_check_null(fd->fn_vardecl);}case 0U: _LL5: _LL6:
({void*_tmp1FC=0U;({struct _fat_ptr _tmp38F=({const char*_tmp1FD="vcgen bind2vardecl";_tag_fat(_tmp1FD,sizeof(char),19U);});((int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos)(_tmp38F,_tag_fat(_tmp1FC,sizeof(void*),0U));});});case 3U: _LL7: _tmp1F9=((struct Cyc_Absyn_Param_b_Absyn_Binding_struct*)_tmp1F6)->f1;_LL8: {struct Cyc_Absyn_Vardecl*vd=_tmp1F9;
return vd;}case 4U: _LL9: _tmp1F8=((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)_tmp1F6)->f1;_LLA: {struct Cyc_Absyn_Vardecl*vd=_tmp1F8;
return vd;}default: _LLB: _tmp1F7=((struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*)_tmp1F6)->f1;_LLC: {struct Cyc_Absyn_Vardecl*vd=_tmp1F7;
return vd;}}_LL0:;}
# 1269
static struct _tuple11 Cyc_Vcgen_vcgen_lexp(struct Cyc_Vcgen_Env*env,struct Cyc_Absyn_Exp*e,void*ain){
void*_tmp1FE=e->r;void*_stmttmp2C=_tmp1FE;void*_tmp1FF=_stmttmp2C;void*_tmp200;if(((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp1FF)->tag == 1U){_LL1: _tmp200=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp1FF)->f1;_LL2: {void*b=_tmp200;
# 1272
return({struct _tuple11 _tmp2F4;({void*_tmp391=(void*)({struct Cyc_Vcgen_Var_Vcgen_Term_struct*_tmp201=_cycalloc(sizeof(*_tmp201));_tmp201->tag=1U,({struct Cyc_Absyn_Vardecl*_tmp390=Cyc_Vcgen_bind2vardecl(b);_tmp201->f1=_tmp390;});_tmp201;});_tmp2F4.f1=_tmp391;}),_tmp2F4.f2=ain;_tmp2F4;});}}else{_LL3: _LL4:
 return({struct _tuple11 _tmp2F5;_tmp2F5.f1=0,({void*_tmp392=(Cyc_Vcgen_vcgen_rexp_nodebug(env,e,ain)).f2;_tmp2F5.f2=_tmp392;});_tmp2F5;});}_LL0:;}
# 1277
static void*Cyc_Vcgen_deref_lterm(struct Cyc_Vcgen_Env*env,void*lt,void*ain){
return lt;}struct _tuple18{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
# 1281
static struct _tuple11 Cyc_Vcgen_vcgen_rexp_nodebug(struct Cyc_Vcgen_Env*env,struct Cyc_Absyn_Exp*e,void*ain){
if(Cyc_Tcutil_is_const_exp(e))return({struct _tuple11 _tmp2F6;({void*_tmp393=(void*)({struct Cyc_Vcgen_Const_Vcgen_Term_struct*_tmp202=_cycalloc(sizeof(*_tmp202));_tmp202->tag=0U,_tmp202->f1=e;_tmp202;});_tmp2F6.f1=_tmp393;}),_tmp2F6.f2=ain;_tmp2F6;});{
void*_tmp203=e->r;void*_stmttmp2D=_tmp203;void*_tmp204=_stmttmp2D;struct Cyc_Absyn_Exp*_tmp205;struct _fat_ptr _tmp206;struct Cyc_Absyn_Exp*_tmp207;struct Cyc_Absyn_Stmt*_tmp208;struct Cyc_Absyn_Exp*_tmp20A;struct Cyc_Absyn_Exp*_tmp209;struct Cyc_Absyn_MallocInfo _tmp20B;struct Cyc_Absyn_Exp*_tmp20C;struct Cyc_Absyn_Exp*_tmp20F;struct Cyc_Absyn_Exp*_tmp20E;struct Cyc_Absyn_Vardecl*_tmp20D;struct Cyc_List_List*_tmp210;struct Cyc_List_List*_tmp211;struct Cyc_List_List*_tmp212;struct Cyc_List_List*_tmp213;struct Cyc_List_List*_tmp214;struct Cyc_List_List*_tmp215;struct Cyc_List_List*_tmp216;struct Cyc_Absyn_Exp*_tmp218;struct Cyc_Absyn_Exp*_tmp217;struct Cyc_Absyn_Exp*_tmp219;struct Cyc_Absyn_Exp*_tmp21A;struct Cyc_Absyn_Exp*_tmp21B;struct Cyc_Absyn_Exp*_tmp21C;struct Cyc_Absyn_Exp*_tmp21D;struct Cyc_Absyn_Exp*_tmp21F;struct Cyc_Absyn_Exp*_tmp21E;enum Cyc_Absyn_Coercion _tmp222;struct Cyc_Absyn_Exp*_tmp221;void*_tmp220;struct Cyc_Absyn_Exp*_tmp223;struct Cyc_Absyn_Exp*_tmp224;struct Cyc_Absyn_Exp*_tmp225;struct Cyc_List_List*_tmp227;struct Cyc_Absyn_Exp*_tmp226;struct Cyc_Absyn_Exp*_tmp229;struct Cyc_Absyn_Exp*_tmp228;struct Cyc_Absyn_Exp*_tmp22B;struct Cyc_Absyn_Exp*_tmp22A;struct Cyc_Absyn_Exp*_tmp22D;struct Cyc_Absyn_Exp*_tmp22C;struct Cyc_Absyn_Exp*_tmp230;struct Cyc_Absyn_Exp*_tmp22F;struct Cyc_Absyn_Exp*_tmp22E;enum Cyc_Absyn_Incrementor _tmp232;struct Cyc_Absyn_Exp*_tmp231;struct Cyc_Absyn_Exp*_tmp235;struct Cyc_Core_Opt*_tmp234;struct Cyc_Absyn_Exp*_tmp233;struct Cyc_List_List*_tmp237;enum Cyc_Absyn_Primop _tmp236;void*_tmp238;union Cyc_Absyn_Cnst _tmp239;switch(*((int*)_tmp204)){case 0U: _LL1: _tmp239=((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_LL2: {union Cyc_Absyn_Cnst c=_tmp239;
# 1286
return({struct _tuple11 _tmp2F7;({void*_tmp394=(void*)({struct Cyc_Vcgen_Const_Vcgen_Term_struct*_tmp23A=_cycalloc(sizeof(*_tmp23A));_tmp23A->tag=0U,_tmp23A->f1=e;_tmp23A;});_tmp2F7.f1=_tmp394;}),_tmp2F7.f2=ain;_tmp2F7;});}case 1U: _LL3: _tmp238=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_LL4: {void*b=_tmp238;
return({struct _tuple11 _tmp2F8;({void*_tmp396=(void*)({struct Cyc_Vcgen_Var_Vcgen_Term_struct*_tmp23B=_cycalloc(sizeof(*_tmp23B));_tmp23B->tag=1U,({struct Cyc_Absyn_Vardecl*_tmp395=Cyc_Vcgen_bind2vardecl(b);_tmp23B->f1=_tmp395;});_tmp23B;});_tmp2F8.f1=_tmp396;}),_tmp2F8.f2=ain;_tmp2F8;});}case 3U: _LL5: _tmp236=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_tmp237=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp204)->f2;_LL6: {enum Cyc_Absyn_Primop p=_tmp236;struct Cyc_List_List*es=_tmp237;
# 1289
struct _tuple15 _tmp23C=Cyc_Vcgen_vcgen_rexps(env,es,ain);struct _tuple15 _stmttmp2E=_tmp23C;struct _tuple15 _tmp23D=_stmttmp2E;void*_tmp23F;struct Cyc_List_List**_tmp23E;_LL58: _tmp23E=_tmp23D.f1;_tmp23F=_tmp23D.f2;_LL59: {struct Cyc_List_List**ts=_tmp23E;void*aout=_tmp23F;
void*t=0;
if(ts != 0)t=Cyc_Vcgen_primop(p,*ts);
return({struct _tuple11 _tmp2F9;_tmp2F9.f1=t,_tmp2F9.f2=aout;_tmp2F9;});}}case 4U: _LL7: _tmp233=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_tmp234=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp204)->f2;_tmp235=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp204)->f3;_LL8: {struct Cyc_Absyn_Exp*e1=_tmp233;struct Cyc_Core_Opt*popt=_tmp234;struct Cyc_Absyn_Exp*e2=_tmp235;
# 1294
struct _tuple11 _tmp240=Cyc_Vcgen_vcgen_lexp(env,e1,ain);struct _tuple11 _stmttmp2F=_tmp240;struct _tuple11 _tmp241=_stmttmp2F;void*_tmp243;void*_tmp242;_LL5B: _tmp242=_tmp241.f1;_tmp243=_tmp241.f2;_LL5C: {void*lt=_tmp242;void*a1=_tmp243;
struct _tuple11 _tmp244=Cyc_Vcgen_vcgen_rexp_nodebug(env,e2,a1);struct _tuple11 _stmttmp30=_tmp244;struct _tuple11 _tmp245=_stmttmp30;void*_tmp247;void*_tmp246;_LL5E: _tmp246=_tmp245.f1;_tmp247=_tmp245.f2;_LL5F: {void*t2=_tmp246;void*a2=_tmp247;
if(popt != 0){
void*_tmp248=Cyc_Vcgen_deref_lterm(env,lt,a2);void*t1=_tmp248;
if(t1 == 0 || t2 == 0)
t2=0;else{
# 1301
t2=({enum Cyc_Absyn_Primop _tmp397=(enum Cyc_Absyn_Primop)popt->v;Cyc_Vcgen_primop(_tmp397,({void*_tmp249[2U];_tmp249[0]=t1,_tmp249[1]=t2;((struct Cyc_List_List*(*)(struct _fat_ptr))Cyc_List_list)(_tag_fat(_tmp249,sizeof(void*),2U));}));});}}{
# 1303
void*_tmp24A=Cyc_Vcgen_do_assign(env,a2,lt,t2);void*a=_tmp24A;
return({struct _tuple11 _tmp2FA;_tmp2FA.f1=lt,_tmp2FA.f2=a;_tmp2FA;});}}}}case 5U: _LL9: _tmp231=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_tmp232=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp204)->f2;_LLA: {struct Cyc_Absyn_Exp*e=_tmp231;enum Cyc_Absyn_Incrementor i=_tmp232;
# 1306
struct _tuple11 _tmp24B=Cyc_Vcgen_vcgen_lexp(env,e,ain);struct _tuple11 _stmttmp31=_tmp24B;struct _tuple11 _tmp24C=_stmttmp31;void*_tmp24E;void*_tmp24D;_LL61: _tmp24D=_tmp24C.f1;_tmp24E=_tmp24C.f2;_LL62: {void*lt=_tmp24D;void*a1=_tmp24E;
void*_tmp24F=Cyc_Vcgen_deref_lterm(env,lt,a1);void*t1=_tmp24F;
void*res=0;
void*assn=0;
{enum Cyc_Absyn_Incrementor _tmp250=i;switch(_tmp250){case Cyc_Absyn_PreInc: _LL64: _LL65:
# 1312
 if(t1 != 0)assn=(void*)({struct Cyc_Vcgen_Primop_Vcgen_Term_struct*_tmp252=_cycalloc(sizeof(*_tmp252));_tmp252->tag=3U,_tmp252->f1=Cyc_Absyn_Plus,({struct Cyc_List_List*_tmp399=({void*_tmp251[2U];_tmp251[0]=t1,({void*_tmp398=Cyc_Vcgen_one();_tmp251[1]=_tmp398;});((struct Cyc_List_List*(*)(struct _fat_ptr))Cyc_List_list)(_tag_fat(_tmp251,sizeof(void*),2U));});_tmp252->f2=_tmp399;});_tmp252;});
res=assn;
goto _LL63;case Cyc_Absyn_PostInc: _LL66: _LL67:
# 1316
 res=t1;
if(t1 != 0)assn=(void*)({struct Cyc_Vcgen_Primop_Vcgen_Term_struct*_tmp254=_cycalloc(sizeof(*_tmp254));_tmp254->tag=3U,_tmp254->f1=Cyc_Absyn_Plus,({struct Cyc_List_List*_tmp39B=({void*_tmp253[2U];_tmp253[0]=t1,({void*_tmp39A=Cyc_Vcgen_one();_tmp253[1]=_tmp39A;});((struct Cyc_List_List*(*)(struct _fat_ptr))Cyc_List_list)(_tag_fat(_tmp253,sizeof(void*),2U));});_tmp254->f2=_tmp39B;});_tmp254;});
goto _LL63;case Cyc_Absyn_PreDec: _LL68: _LL69:
# 1320
 if(t1 != 0)assn=(void*)({struct Cyc_Vcgen_Primop_Vcgen_Term_struct*_tmp256=_cycalloc(sizeof(*_tmp256));_tmp256->tag=3U,_tmp256->f1=Cyc_Absyn_Minus,({struct Cyc_List_List*_tmp39D=({void*_tmp255[2U];_tmp255[0]=t1,({void*_tmp39C=Cyc_Vcgen_one();_tmp255[1]=_tmp39C;});((struct Cyc_List_List*(*)(struct _fat_ptr))Cyc_List_list)(_tag_fat(_tmp255,sizeof(void*),2U));});_tmp256->f2=_tmp39D;});_tmp256;});
res=assn;
goto _LL63;case Cyc_Absyn_PostDec: _LL6A: _LL6B:
# 1324
 res=t1;
if(t1 != 0)assn=(void*)({struct Cyc_Vcgen_Primop_Vcgen_Term_struct*_tmp258=_cycalloc(sizeof(*_tmp258));_tmp258->tag=3U,_tmp258->f1=Cyc_Absyn_Minus,({struct Cyc_List_List*_tmp39F=({void*_tmp257[2U];_tmp257[0]=t1,({void*_tmp39E=Cyc_Vcgen_one();_tmp257[1]=_tmp39E;});((struct Cyc_List_List*(*)(struct _fat_ptr))Cyc_List_list)(_tag_fat(_tmp257,sizeof(void*),2U));});_tmp258->f2=_tmp39F;});_tmp258;});
goto _LL63;default: _LL6C: _LL6D:
({void*_tmp259=0U;({struct _fat_ptr _tmp3A0=({const char*_tmp25A="vcgen vcgen_rexp increment_e";_tag_fat(_tmp25A,sizeof(char),29U);});((int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos)(_tmp3A0,_tag_fat(_tmp259,sizeof(void*),0U));});});}_LL63:;}{
# 1329
void*_tmp25B=Cyc_Vcgen_do_assign(env,a1,lt,assn);void*a=_tmp25B;
return({struct _tuple11 _tmp2FB;_tmp2FB.f1=res,_tmp2FB.f2=a;_tmp2FB;});}}}case 6U: _LLB: _tmp22E=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_tmp22F=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp204)->f2;_tmp230=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp204)->f3;_LLC: {struct Cyc_Absyn_Exp*e1=_tmp22E;struct Cyc_Absyn_Exp*e2=_tmp22F;struct Cyc_Absyn_Exp*e3=_tmp230;
# 1332
struct _tuple11 _tmp25C=Cyc_Vcgen_vcgen_test(env,e1,ain);struct _tuple11 _stmttmp32=_tmp25C;struct _tuple11 _tmp25D=_stmttmp32;void*_tmp25F;void*_tmp25E;_LL6F: _tmp25E=_tmp25D.f1;_tmp25F=_tmp25D.f2;_LL70: {void*at=_tmp25E;void*af=_tmp25F;
struct _tuple11 _tmp260=Cyc_Vcgen_vcgen_rexp_nodebug(env,e2,at);struct _tuple11 _stmttmp33=_tmp260;struct _tuple11 _tmp261=_stmttmp33;void*_tmp263;void*_tmp262;_LL72: _tmp262=_tmp261.f1;_tmp263=_tmp261.f2;_LL73: {void*t1=_tmp262;void*at=_tmp263;
struct _tuple11 _tmp264=Cyc_Vcgen_vcgen_rexp_nodebug(env,e3,af);struct _tuple11 _stmttmp34=_tmp264;struct _tuple11 _tmp265=_stmttmp34;void*_tmp267;void*_tmp266;_LL75: _tmp266=_tmp265.f1;_tmp267=_tmp265.f2;_LL76: {void*t2=_tmp266;void*af=_tmp267;
if(t1 != 0 && t2 != 0){
void*_tmp268=Cyc_Vcgen_fresh_var();void*v=_tmp268;
at=({void*_tmp3A1=at;Cyc_Vcgen_and(_tmp3A1,Cyc_Vcgen_eq(v,t1));});
af=({void*_tmp3A2=af;Cyc_Vcgen_and(_tmp3A2,Cyc_Vcgen_eq(v,t2));});
return({struct _tuple11 _tmp2FC;_tmp2FC.f1=v,({void*_tmp3A3=Cyc_Vcgen_or(at,af);_tmp2FC.f2=_tmp3A3;});_tmp2FC;});}else{
# 1341
return({struct _tuple11 _tmp2FD;_tmp2FD.f1=0,({void*_tmp3A4=Cyc_Vcgen_or(at,af);_tmp2FD.f2=_tmp3A4;});_tmp2FD;});}}}}}case 7U: _LLD: _tmp22C=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_tmp22D=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp204)->f2;_LLE: {struct Cyc_Absyn_Exp*e1=_tmp22C;struct Cyc_Absyn_Exp*e2=_tmp22D;
# 1343
struct _tuple11 _tmp269=Cyc_Vcgen_vcgen_test(env,e1,ain);struct _tuple11 _stmttmp35=_tmp269;struct _tuple11 _tmp26A=_stmttmp35;void*_tmp26C;void*_tmp26B;_LL78: _tmp26B=_tmp26A.f1;_tmp26C=_tmp26A.f2;_LL79: {void*at=_tmp26B;void*af=_tmp26C;
struct _tuple11 _tmp26D=Cyc_Vcgen_vcgen_rexp_nodebug(env,e2,at);struct _tuple11 _stmttmp36=_tmp26D;struct _tuple11 _tmp26E=_stmttmp36;void*_tmp270;void*_tmp26F;_LL7B: _tmp26F=_tmp26E.f1;_tmp270=_tmp26E.f2;_LL7C: {void*t=_tmp26F;void*at=_tmp270;
void*_tmp271=Cyc_Vcgen_fresh_var();void*v=_tmp271;
at=({void*_tmp3A5=at;Cyc_Vcgen_and(_tmp3A5,Cyc_Vcgen_eq(v,t));});
af=({void*_tmp3A7=af;Cyc_Vcgen_and(_tmp3A7,({void*_tmp3A6=v;Cyc_Vcgen_eq(_tmp3A6,Cyc_Vcgen_zero());}));});
return({struct _tuple11 _tmp2FE;_tmp2FE.f1=v,({void*_tmp3A8=Cyc_Vcgen_or(at,af);_tmp2FE.f2=_tmp3A8;});_tmp2FE;});}}}case 8U: _LLF: _tmp22A=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_tmp22B=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp204)->f2;_LL10: {struct Cyc_Absyn_Exp*e1=_tmp22A;struct Cyc_Absyn_Exp*e2=_tmp22B;
# 1350
struct _tuple11 _tmp272=Cyc_Vcgen_vcgen_test(env,e1,ain);struct _tuple11 _stmttmp37=_tmp272;struct _tuple11 _tmp273=_stmttmp37;void*_tmp275;void*_tmp274;_LL7E: _tmp274=_tmp273.f1;_tmp275=_tmp273.f2;_LL7F: {void*at=_tmp274;void*af=_tmp275;
struct _tuple11 _tmp276=Cyc_Vcgen_vcgen_rexp_nodebug(env,e2,af);struct _tuple11 _stmttmp38=_tmp276;struct _tuple11 _tmp277=_stmttmp38;void*_tmp279;void*_tmp278;_LL81: _tmp278=_tmp277.f1;_tmp279=_tmp277.f2;_LL82: {void*t=_tmp278;void*af=_tmp279;
void*_tmp27A=Cyc_Vcgen_fresh_var();void*v=_tmp27A;
at=({void*_tmp3AA=at;Cyc_Vcgen_and(_tmp3AA,({void*_tmp3A9=v;Cyc_Vcgen_eq(_tmp3A9,Cyc_Vcgen_one());}));});
af=({void*_tmp3AB=af;Cyc_Vcgen_and(_tmp3AB,Cyc_Vcgen_eq(v,t));});
return({struct _tuple11 _tmp2FF;_tmp2FF.f1=v,({void*_tmp3AC=Cyc_Vcgen_or(at,af);_tmp2FF.f2=_tmp3AC;});_tmp2FF;});}}}case 9U: _LL11: _tmp228=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_tmp229=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp204)->f2;_LL12: {struct Cyc_Absyn_Exp*e1=_tmp228;struct Cyc_Absyn_Exp*e2=_tmp229;
# 1357
struct _tuple11 _tmp27B=Cyc_Vcgen_vcgen_rexp_nodebug(env,e1,ain);struct _tuple11 _stmttmp39=_tmp27B;struct _tuple11 _tmp27C=_stmttmp39;void*_tmp27D;_LL84: _tmp27D=_tmp27C.f2;_LL85: {void*a=_tmp27D;
return Cyc_Vcgen_vcgen_rexp_nodebug(env,e2,a);}}case 10U: _LL13: _tmp226=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_tmp227=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp204)->f2;_LL14: {struct Cyc_Absyn_Exp*e=_tmp226;struct Cyc_List_List*es=_tmp227;
# 1360
ain=Cyc_Vcgen_vcgen_rexps_nosave(env,es,ain);{
struct _tuple11 _tmp27E=Cyc_Vcgen_vcgen_rexp_nodebug(env,e,ain);struct _tuple11 _stmttmp3A=_tmp27E;struct _tuple11 _tmp27F=_stmttmp3A;void*_tmp280;_LL87: _tmp280=_tmp27F.f2;_LL88: {void*a2=_tmp280;
# 1364
a2=Cyc_Vcgen_kill(a2);
# 1367
Cyc_Vcgen_update_try_assn(env,a2);
# 1369
if(Cyc_Tcutil_is_noreturn_fn_type((void*)_check_null(e->topt)))a2=(void*)& Cyc_Vcgen_false_assn;
return({struct _tuple11 _tmp300;_tmp300.f1=0,_tmp300.f2=a2;_tmp300;});}}}case 11U: _LL15: _tmp225=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_LL16: {struct Cyc_Absyn_Exp*e=_tmp225;
# 1372
struct _tuple11 _tmp281=Cyc_Vcgen_vcgen_rexp_nodebug(env,e,ain);struct _tuple11 _stmttmp3B=_tmp281;struct _tuple11 _tmp282=_stmttmp3B;void*_tmp283;_LL8A: _tmp283=_tmp282.f2;_LL8B: {void*a=_tmp283;
Cyc_Vcgen_update_try_assn(env,a);
return({struct _tuple11 _tmp301;_tmp301.f1=0,_tmp301.f2=(void*)& Cyc_Vcgen_false_assn;_tmp301;});}}case 12U: _LL17: _tmp224=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_LL18: {struct Cyc_Absyn_Exp*e=_tmp224;
return Cyc_Vcgen_vcgen_rexp_nodebug(env,e,ain);}case 13U: _LL19: _tmp223=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_LL1A: {struct Cyc_Absyn_Exp*e=_tmp223;
return Cyc_Vcgen_vcgen_rexp_nodebug(env,e,ain);}case 14U: _LL1B: _tmp220=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_tmp221=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp204)->f2;_tmp222=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp204)->f4;_LL1C: {void*t=_tmp220;struct Cyc_Absyn_Exp*e=_tmp221;enum Cyc_Absyn_Coercion c=_tmp222;
# 1380
struct _tuple11 _tmp284=Cyc_Vcgen_vcgen_rexp_nodebug(env,e,ain);struct _tuple11 _stmttmp3C=_tmp284;struct _tuple11 _tmp285=_stmttmp3C;void*_tmp287;void*_tmp286;_LL8D: _tmp286=_tmp285.f1;_tmp287=_tmp285.f2;_LL8E: {void*t=_tmp286;void*a=_tmp287;
if((int)c == (int)1U)
return({struct _tuple11 _tmp302;_tmp302.f1=t,_tmp302.f2=a;_tmp302;});else{
# 1384
Cyc_Vcgen_update_try_assn(env,ain);
return({struct _tuple11 _tmp303;_tmp303.f1=0,_tmp303.f2=a;_tmp303;});}}}case 16U: _LL1D: _tmp21E=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_tmp21F=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp204)->f2;_LL1E: {struct Cyc_Absyn_Exp*eopt=_tmp21E;struct Cyc_Absyn_Exp*e=_tmp21F;
# 1388
if(eopt != 0)
ain=(Cyc_Vcgen_vcgen_rexp_nodebug(env,eopt,ain)).f2;
return({struct _tuple11 _tmp304;_tmp304.f1=0,({void*_tmp3AD=(Cyc_Vcgen_vcgen_rexp_nodebug(env,e,ain)).f2;_tmp304.f2=_tmp3AD;});_tmp304;});}case 32U: _LL1F: _LL20:
# 1393
 goto _LL22;case 33U: _LL21: _LL22:
 goto _LL24;case 17U: _LL23: _LL24:
 goto _LL26;case 18U: _LL25: _LL26:
 goto _LL28;case 39U: _LL27: _LL28:
 goto _LL2A;case 19U: _LL29: _LL2A:
 return({struct _tuple11 _tmp305;_tmp305.f1=0,_tmp305.f2=ain;_tmp305;});case 20U: _LL2B: _tmp21D=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_LL2C: {struct Cyc_Absyn_Exp*e=_tmp21D;
# 1401
_tmp21C=e;goto _LL2E;}case 21U: _LL2D: _tmp21C=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_LL2E: {struct Cyc_Absyn_Exp*e=_tmp21C;
_tmp21B=e;goto _LL30;}case 22U: _LL2F: _tmp21B=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_LL30: {struct Cyc_Absyn_Exp*e=_tmp21B;
# 1404
ain=(Cyc_Vcgen_vcgen_rexp_nodebug(env,e,ain)).f2;
Cyc_Vcgen_update_try_assn(env,ain);
return({struct _tuple11 _tmp306;_tmp306.f1=0,_tmp306.f2=ain;_tmp306;});}case 38U: _LL31: _tmp21A=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_LL32: {struct Cyc_Absyn_Exp*e=_tmp21A;
# 1408
_tmp219=e;goto _LL34;}case 15U: _LL33: _tmp219=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_LL34: {struct Cyc_Absyn_Exp*e=_tmp219;
# 1410
return({struct _tuple11 _tmp307;_tmp307.f1=0,({void*_tmp3AE=(Cyc_Vcgen_vcgen_rexp_nodebug(env,e,ain)).f2;_tmp307.f2=_tmp3AE;});_tmp307;});}case 23U: _LL35: _tmp217=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_tmp218=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp204)->f2;_LL36: {struct Cyc_Absyn_Exp*e1=_tmp217;struct Cyc_Absyn_Exp*e2=_tmp218;
# 1413
struct _tuple11 _tmp288=Cyc_Vcgen_vcgen_rexp_nodebug(env,e1,ain);struct _tuple11 _stmttmp3D=_tmp288;struct _tuple11 _tmp289=_stmttmp3D;void*_tmp28B;void*_tmp28A;_LL90: _tmp28A=_tmp289.f1;_tmp28B=_tmp289.f2;_LL91: {void*t1=_tmp28A;void*a1=_tmp28B;
struct _tuple11 _tmp28C=Cyc_Vcgen_vcgen_rexp_nodebug(env,e2,a1);struct _tuple11 _stmttmp3E=_tmp28C;struct _tuple11 _tmp28D=_stmttmp3E;void*_tmp28F;void*_tmp28E;_LL93: _tmp28E=_tmp28D.f1;_tmp28F=_tmp28D.f2;_LL94: {void*t2=_tmp28E;void*a2=_tmp28F;
{void*_tmp290=Cyc_Tcutil_compress((void*)_check_null(e1->topt));void*_stmttmp3F=_tmp290;void*_tmp291=_stmttmp3F;if(((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp291)->tag == 6U){_LL96: _LL97:
 goto _LL95;}else{_LL98: _LL99:
# 1418
 if(t1 != 0){
Cyc_Vcgen_update_try_assn(env,ain);
a2=({void*_tmp3B1=a2;Cyc_Vcgen_and(_tmp3B1,({void*_tmp3B0=t2;Cyc_Vcgen_lt(_tmp3B0,(void*)({struct Cyc_Vcgen_Primop_Vcgen_Term_struct*_tmp293=_cycalloc(sizeof(*_tmp293));_tmp293->tag=3U,_tmp293->f1=Cyc_Absyn_Numelts,({struct Cyc_List_List*_tmp3AF=({void*_tmp292[1U];_tmp292[0]=t1;((struct Cyc_List_List*(*)(struct _fat_ptr))Cyc_List_list)(_tag_fat(_tmp292,sizeof(void*),1U));});_tmp293->f2=_tmp3AF;});_tmp293;}));}));});}}_LL95:;}
# 1423
return({struct _tuple11 _tmp308;_tmp308.f1=0,_tmp308.f2=a2;_tmp308;});}}}case 31U: _LL37: _tmp216=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_LL38: {struct Cyc_List_List*es=_tmp216;
# 1425
_tmp215=es;goto _LL3A;}case 24U: _LL39: _tmp215=((struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_LL3A: {struct Cyc_List_List*es=_tmp215;
return({struct _tuple11 _tmp309;_tmp309.f1=0,({void*_tmp3B2=Cyc_Vcgen_vcgen_rexps_nosave(env,es,ain);_tmp309.f2=_tmp3B2;});_tmp309;});}case 25U: _LL3B: _tmp214=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmp204)->f2;_LL3C: {struct Cyc_List_List*dles=_tmp214;
# 1428
_tmp213=dles;goto _LL3E;}case 26U: _LL3D: _tmp213=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_LL3E: {struct Cyc_List_List*dles=_tmp213;
_tmp212=dles;goto _LL40;}case 29U: _LL3F: _tmp212=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp204)->f3;_LL40: {struct Cyc_List_List*dles=_tmp212;
_tmp211=dles;goto _LL42;}case 36U: _LL41: _tmp211=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp204)->f2;_LL42: {struct Cyc_List_List*dles=_tmp211;
_tmp210=dles;goto _LL44;}case 30U: _LL43: _tmp210=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmp204)->f2;_LL44: {struct Cyc_List_List*dles=_tmp210;
# 1433
for(0;dles != 0;dles=dles->tl){
struct _tuple18*_tmp294=(struct _tuple18*)dles->hd;struct _tuple18*_stmttmp40=_tmp294;struct _tuple18*_tmp295=_stmttmp40;struct Cyc_Absyn_Exp*_tmp296;_LL9B: _tmp296=_tmp295->f2;_LL9C: {struct Cyc_Absyn_Exp*e=_tmp296;
ain=(Cyc_Vcgen_vcgen_rexp_nodebug(env,e,ain)).f2;}}
# 1437
return({struct _tuple11 _tmp30A;_tmp30A.f1=0,_tmp30A.f2=ain;_tmp30A;});}case 27U: _LL45: _tmp20D=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_tmp20E=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp204)->f2;_tmp20F=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp204)->f3;_LL46: {struct Cyc_Absyn_Vardecl*vd=_tmp20D;struct Cyc_Absyn_Exp*e1=_tmp20E;struct Cyc_Absyn_Exp*e2=_tmp20F;
# 1441
struct _tuple11 _tmp297=Cyc_Vcgen_vcgen_rexp_nodebug(env,e1,ain);struct _tuple11 _stmttmp41=_tmp297;struct _tuple11 _tmp298=_stmttmp41;void*_tmp29A;void*_tmp299;_LL9E: _tmp299=_tmp298.f1;_tmp29A=_tmp298.f2;_LL9F: {void*t1=_tmp299;void*a=_tmp29A;
void*_tmp29B=t1 == 0?Cyc_Vcgen_fresh_var(): t1;{void*t1=_tmp29B;
struct Cyc_Vcgen_Var_Vcgen_Term_struct*_tmp29C=({struct Cyc_Vcgen_Var_Vcgen_Term_struct*_tmp2A4=_cycalloc(sizeof(*_tmp2A4));_tmp2A4->tag=1U,_tmp2A4->f1=vd;_tmp2A4;});struct Cyc_Vcgen_Var_Vcgen_Term_struct*v=_tmp29C;
# 1445
a=({struct Cyc_Vcgen_Env*_tmp3B5=env;void*_tmp3B4=a;void*_tmp3B3=(void*)v;Cyc_Vcgen_do_assign(_tmp3B5,_tmp3B4,_tmp3B3,Cyc_Vcgen_zero());});
while(1){
void*_tmp29D=({void*_tmp3B6=a;Cyc_Vcgen_and(_tmp3B6,Cyc_Vcgen_lt((void*)v,t1));});void*at=_tmp29D;
void*_tmp29E=({void*_tmp3B7=a;Cyc_Vcgen_and(_tmp3B7,Cyc_Vcgen_lte(t1,(void*)v));});void*af=_tmp29E;
struct _tuple11 _tmp29F=Cyc_Vcgen_vcgen_rexp_nodebug(env,e2,at);struct _tuple11 _stmttmp42=_tmp29F;struct _tuple11 _tmp2A0=_stmttmp42;void*_tmp2A1;_LLA1: _tmp2A1=_tmp2A0.f2;_LLA2: {void*abody=_tmp2A1;
abody=({struct Cyc_Vcgen_Env*_tmp3BC=env;void*_tmp3BB=abody;void*_tmp3BA=(void*)v;Cyc_Vcgen_do_assign(_tmp3BC,_tmp3BB,_tmp3BA,(void*)({struct Cyc_Vcgen_Primop_Vcgen_Term_struct*_tmp2A3=_cycalloc(sizeof(*_tmp2A3));_tmp2A3->tag=3U,_tmp2A3->f1=Cyc_Absyn_Plus,({struct Cyc_List_List*_tmp3B9=({void*_tmp2A2[2U];_tmp2A2[0]=(void*)v,({void*_tmp3B8=Cyc_Vcgen_one();_tmp2A2[1]=_tmp3B8;});((struct Cyc_List_List*(*)(struct _fat_ptr))Cyc_List_list)(_tag_fat(_tmp2A2,sizeof(void*),2U));});_tmp2A3->f2=_tmp3B9;});_tmp2A3;}));});
if(Cyc_Vcgen_implies(abody,a)){a=af;break;}
a=Cyc_Vcgen_widen_assn(abody,a);}}
# 1454
return({struct _tuple11 _tmp30B;_tmp30B.f1=0,_tmp30B.f2=a;_tmp30B;});}}}case 28U: _LL47: _tmp20C=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_LL48: {struct Cyc_Absyn_Exp*e=_tmp20C;
# 1456
return({struct _tuple11 _tmp30C;_tmp30C.f1=0,({void*_tmp3BD=(Cyc_Vcgen_vcgen_rexp_nodebug(env,e,ain)).f2;_tmp30C.f2=_tmp3BD;});_tmp30C;});}case 34U: _LL49: _tmp20B=((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_LL4A: {struct Cyc_Absyn_MallocInfo mi=_tmp20B;
# 1458
if(mi.rgn != 0)
ain=(Cyc_Vcgen_vcgen_rexp_nodebug(env,mi.rgn,ain)).f2;
return({struct _tuple11 _tmp30D;_tmp30D.f1=0,({void*_tmp3BE=(Cyc_Vcgen_vcgen_rexp_nodebug(env,mi.num_elts,ain)).f2;_tmp30D.f2=_tmp3BE;});_tmp30D;});}case 35U: _LL4B: _tmp209=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_tmp20A=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp204)->f2;_LL4C: {struct Cyc_Absyn_Exp*e1=_tmp209;struct Cyc_Absyn_Exp*e2=_tmp20A;
# 1462
struct _tuple11 _tmp2A5=Cyc_Vcgen_vcgen_lexp(env,e1,ain);struct _tuple11 _stmttmp43=_tmp2A5;struct _tuple11 _tmp2A6=_stmttmp43;void*_tmp2A8;void*_tmp2A7;_LLA4: _tmp2A7=_tmp2A6.f1;_tmp2A8=_tmp2A6.f2;_LLA5: {void*lt1=_tmp2A7;void*a1=_tmp2A8;
Cyc_Vcgen_update_try_assn(env,a1);{
struct _tuple11 _tmp2A9=Cyc_Vcgen_vcgen_lexp(env,e2,a1);struct _tuple11 _stmttmp44=_tmp2A9;struct _tuple11 _tmp2AA=_stmttmp44;void*_tmp2AC;void*_tmp2AB;_LLA7: _tmp2AB=_tmp2AA.f1;_tmp2AC=_tmp2AA.f2;_LLA8: {void*lt2=_tmp2AB;void*a2=_tmp2AC;
Cyc_Vcgen_update_try_assn(env,a2);{
void*_tmp2AD=Cyc_Vcgen_deref_lterm(env,lt1,a2);void*t1=_tmp2AD;
void*_tmp2AE=Cyc_Vcgen_deref_lterm(env,lt2,a2);void*t2=_tmp2AE;
a2=Cyc_Vcgen_do_assign(env,a2,lt1,t2);
a2=Cyc_Vcgen_do_assign(env,a2,lt2,t1);
return({struct _tuple11 _tmp30E;_tmp30E.f1=0,_tmp30E.f2=a2;_tmp30E;});}}}}}case 37U: _LL4D: _tmp208=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_LL4E: {struct Cyc_Absyn_Stmt*s=_tmp208;
# 1472
void**_tmp2AF=(env->shared)->exp_stmt;void**old_exp_stmt=_tmp2AF;
({void**_tmp3BF=({void**_tmp2B0=_cycalloc(sizeof(*_tmp2B0));*_tmp2B0=0;_tmp2B0;});(env->shared)->exp_stmt=_tmp3BF;});{
void*_tmp2B1=Cyc_Vcgen_vcgen_stmt_nodebug(env,s,ain);void*aout=_tmp2B1;
void*_tmp2B2=*((void**)_check_null((env->shared)->exp_stmt));void*t=_tmp2B2;
(env->shared)->exp_stmt=old_exp_stmt;
return({struct _tuple11 _tmp30F;_tmp30F.f1=t,_tmp30F.f2=aout;_tmp30F;});}}case 40U: _LL4F: _LL50:
 return({struct _tuple11 _tmp310;_tmp310.f1=0,_tmp310.f2=ain;_tmp310;});case 41U: _LL51: _tmp207=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_LL52: {struct Cyc_Absyn_Exp*e=_tmp207;
return Cyc_Vcgen_vcgen_rexp_nodebug(env,e,ain);}case 2U: _LL53: _tmp206=((struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_LL54: {struct _fat_ptr s=_tmp206;
# 1481
if(({struct _fat_ptr _tmp3C0=(struct _fat_ptr)s;Cyc_strcmp(_tmp3C0,({const char*_tmp2B3="print_assn";_tag_fat(_tmp2B3,sizeof(char),11U);}));})){
struct _fat_ptr _tmp2B4=Cyc_Position_string_of_segment(e->loc);struct _fat_ptr seg_str=_tmp2B4;
({struct Cyc_String_pa_PrintArg_struct _tmp2B7=({struct Cyc_String_pa_PrintArg_struct _tmp312;_tmp312.tag=0U,_tmp312.f1=(struct _fat_ptr)((struct _fat_ptr)seg_str);_tmp312;});struct Cyc_String_pa_PrintArg_struct _tmp2B8=({struct Cyc_String_pa_PrintArg_struct _tmp311;_tmp311.tag=0U,({struct _fat_ptr _tmp3C1=(struct _fat_ptr)((struct _fat_ptr)Cyc_Vcgen_assn2string(ain));_tmp311.f1=_tmp3C1;});_tmp311;});void*_tmp2B5[2U];_tmp2B5[0]=& _tmp2B7,_tmp2B5[1]=& _tmp2B8;({struct Cyc___cycFILE*_tmp3C3=Cyc_stderr;struct _fat_ptr _tmp3C2=({const char*_tmp2B6="%s: current assn is %s\n";_tag_fat(_tmp2B6,sizeof(char),24U);});Cyc_fprintf(_tmp3C3,_tmp3C2,_tag_fat(_tmp2B5,sizeof(void*),2U));});});}
# 1485
return({struct _tuple11 _tmp313;({void*_tmp3C4=Cyc_Vcgen_zero();_tmp313.f1=_tmp3C4;}),_tmp313.f2=ain;_tmp313;});}default: _LL55: _tmp205=((struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct*)_tmp204)->f1;_LL56: {struct Cyc_Absyn_Exp*e1=_tmp205;
# 1487
struct _tuple11 _tmp2B9=Cyc_Vcgen_vcgen_test(env,e1,(void*)& Cyc_Vcgen_true_assn);struct _tuple11 _stmttmp45=_tmp2B9;struct _tuple11 _tmp2BA=_stmttmp45;void*_tmp2BB;_LLAA: _tmp2BB=_tmp2BA.f1;_LLAB: {void*at=_tmp2BB;
if(!Cyc_Vcgen_implies(ain,at))
({struct Cyc_String_pa_PrintArg_struct _tmp2BE=({struct Cyc_String_pa_PrintArg_struct _tmp314;_tmp314.tag=0U,({
struct _fat_ptr _tmp3C5=(struct _fat_ptr)((struct _fat_ptr)Cyc_Vcgen_assn2string(ain));_tmp314.f1=_tmp3C5;});_tmp314;});void*_tmp2BC[1U];_tmp2BC[0]=& _tmp2BE;({unsigned _tmp3C7=e->loc;struct _fat_ptr _tmp3C6=({const char*_tmp2BD="cannot prove @assert from %s";_tag_fat(_tmp2BD,sizeof(char),29U);});Cyc_Warn_err(_tmp3C7,_tmp3C6,_tag_fat(_tmp2BC,sizeof(void*),1U));});});
return({struct _tuple11 _tmp315;({void*_tmp3C9=Cyc_Vcgen_one();_tmp315.f1=_tmp3C9;}),({void*_tmp3C8=Cyc_Vcgen_and(ain,at);_tmp315.f2=_tmp3C8;});_tmp315;});}}}_LL0:;}}
# 1496
void Cyc_Vcgen_vcgen_topdecls(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_List_List*ds){
for(0;ds != 0;ds=ds->tl){
void*_tmp2BF=((struct Cyc_Absyn_Decl*)ds->hd)->r;void*_stmttmp46=_tmp2BF;void*_tmp2C0=_stmttmp46;struct Cyc_List_List*_tmp2C1;struct Cyc_List_List*_tmp2C2;struct Cyc_Absyn_Fndecl*_tmp2C3;switch(*((int*)_tmp2C0)){case 1U: _LL1: _tmp2C3=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp2C0)->f1;_LL2: {struct Cyc_Absyn_Fndecl*fd=_tmp2C3;
# 1502
struct Cyc_Vcgen_Env*_tmp2C4=Cyc_Vcgen_initial_env(tables,fd);struct Cyc_Vcgen_Env*env=_tmp2C4;
# 1504
Cyc_Vcgen_vcgen_stmt_nodebug(env,fd->body,(void*)& Cyc_Vcgen_true_assn);
Cyc_Vcgen_hash_cons_table=0;
goto _LL0;}case 9U: _LL3: _tmp2C2=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp2C0)->f2;_LL4: {struct Cyc_List_List*ds=_tmp2C2;
_tmp2C1=ds;goto _LL6;}case 10U: _LL5: _tmp2C1=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmp2C0)->f2;_LL6: {struct Cyc_List_List*ds=_tmp2C1;
Cyc_Vcgen_vcgen_topdecls(tables,ds);goto _LL0;}default: _LL7: _LL8:
 goto _LL0;}_LL0:;}}
