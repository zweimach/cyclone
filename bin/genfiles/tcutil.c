#include <setjmp.h>
/* This is a C header used by the output of the Cyclone to
   C translator.  Corresponding definitions are in file lib/runtime_*.c */
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
/* should be size_t but int is fine */
#define offsetof(t,n) ((int)(&(((t*)0)->n)))
#endif

/* Fat pointers */
struct _fat_ptr {
  unsigned char *curr; 
  unsigned char *base; 
  unsigned char *last_plus_one; 
};  

/* Regions */
struct _RegionPage
{ 
#ifdef CYC_REGION_PROFILE
  unsigned total_bytes;
  unsigned free_bytes;
#endif
  struct _RegionPage *next;
  char data[1];
};

struct _pool;
struct bget_region_key;
struct _RegionAllocFunctions;

struct _RegionHandle {
  struct _RuntimeStack s;
  struct _RegionPage *curr;
#if(defined(__linux__) && defined(__KERNEL__))
  struct _RegionPage *vpage;
#endif 
  struct _RegionAllocFunctions *fcns;
  char               *offset;
  char               *last_plus_one;
  struct _pool *released_ptrs;
  struct bget_region_key *key;
#ifdef CYC_REGION_PROFILE
  const char *name;
#endif
  unsigned used_bytes;
  unsigned wasted_bytes;
};


// A dynamic region is just a region handle.  The wrapper struct is for type
// abstraction.
struct Cyc_Core_DynamicRegion {
  struct _RegionHandle h;
};

/* Alias qualifier stuff */
typedef unsigned int _AliasQualHandle_t; // must match aqualt_type() in toc.cyc

struct _RegionHandle _new_region(unsigned int, const char*);
void* _region_malloc(struct _RegionHandle*, _AliasQualHandle_t, unsigned);
void* _region_calloc(struct _RegionHandle*, _AliasQualHandle_t, unsigned t, unsigned n);
void* _region_vmalloc(struct _RegionHandle*, unsigned);
void * _aqual_malloc(_AliasQualHandle_t aq, unsigned int s);
void * _aqual_calloc(_AliasQualHandle_t aq, unsigned int n, unsigned int t);
void _free_region(struct _RegionHandle*);

/* Exceptions */
struct _handler_cons {
  struct _RuntimeStack s;
  jmp_buf handler;
};
void _push_handler(struct _handler_cons*);
void _push_region(struct _RegionHandle*);
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

void* Cyc_Core_get_exn_thrown();
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
#ifdef NO_CYC_NULL_CHECKS
#define _check_null(ptr) (ptr)
#else
#define _check_null(ptr) \
  ({ typeof(ptr) _cks_null = (ptr); \
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
#define _zero_arr_plus_char_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_other_fn(t_sz,orig_x,orig_sz,orig_i,f,l)((orig_x)+(orig_i))
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
void* _zero_arr_plus_other_fn(unsigned,void*,unsigned,int,const char*,unsigned);
#endif

/* _get_zero_arr_size_*(x,sz) returns the number of elements in a
   zero-terminated array that is NULL or has at least sz elements */
unsigned _get_zero_arr_size_char(const char*,unsigned);
unsigned _get_zero_arr_size_other(unsigned,const void*,unsigned);

/* _zero_arr_inplace_plus_*_fn(x,i,filename,lineno) sets
   zero-terminated pointer *x to *x + i */
char* _zero_arr_inplace_plus_char_fn(char**,int,const char*,unsigned);
char* _zero_arr_inplace_plus_post_char_fn(char**,int,const char*,unsigned);
// note: must cast result in toc.cyc
void* _zero_arr_inplace_plus_other_fn(unsigned,void**,int,const char*,unsigned);
void* _zero_arr_inplace_plus_post_other_fn(unsigned,void**,int,const char*,unsigned);
#define _zero_arr_plus_char(x,s,i) \
  (_zero_arr_plus_char_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_inplace_plus_char(x,i) \
  _zero_arr_inplace_plus_char_fn((char**)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_char(x,i) \
  _zero_arr_inplace_plus_post_char_fn((char**)(x),(i),__FILE__,__LINE__)
#define _zero_arr_plus_other(t,x,s,i) \
  (_zero_arr_plus_other_fn(t,x,s,i,__FILE__,__LINE__))
#define _zero_arr_inplace_plus_other(t,x,i) \
  _zero_arr_inplace_plus_other_fn(t,(void**)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_other(t,x,i) \
  _zero_arr_inplace_plus_post_other_fn(t,(void**)(x),(i),__FILE__,__LINE__)

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_fat_subscript(arr,elt_sz,index) ((arr).curr + (elt_sz) * (index))
#define _untag_fat_ptr(arr,elt_sz,num_elts) ((arr).curr)
#define _untag_fat_ptr_check_bound(arr,elt_sz,num_elts) ((arr).curr)
#define _check_fat_at_base(arr) (arr)
#else
#define _check_fat_subscript(arr,elt_sz,index) ({ \
  struct _fat_ptr _cus_arr = (arr); \
  unsigned char *_cus_ans = _cus_arr.curr + (elt_sz) * (index); \
  /* JGM: not needed! if (!_cus_arr.base) _throw_null();*/ \
  if (_cus_ans < _cus_arr.base || _cus_ans >= _cus_arr.last_plus_one) \
    _throw_arraybounds(); \
  _cus_ans; })
#define _untag_fat_ptr(arr,elt_sz,num_elts) ((arr).curr)
#define _untag_fat_ptr_check_bound(arr,elt_sz,num_elts) ({ \
  struct _fat_ptr _arr = (arr); \
  unsigned char *_curr = _arr.curr; \
  if ((_curr < _arr.base || _curr + (elt_sz) * (num_elts) > _arr.last_plus_one) &&\
      _curr != (unsigned char*)0) \
    _throw_arraybounds(); \
  _curr; })
#define _check_fat_at_base(arr) ({ \
  struct _fat_ptr _arr = (arr); \
  if (_arr.base != _arr.curr) _throw_arraybounds(); \
  _arr; })
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

#ifdef CYC_GC_PTHREAD_REDIRECTS
# define pthread_create GC_pthread_create
# define pthread_sigmask GC_pthread_sigmask
# define pthread_join GC_pthread_join
# define pthread_detach GC_pthread_detach
# define dlopen GC_dlopen
#endif
/* Allocation */
void* GC_malloc(int);
void* GC_malloc_atomic(int);
void* GC_calloc(unsigned,unsigned);
void* GC_calloc_atomic(unsigned,unsigned);

#if(defined(__linux__) && defined(__KERNEL__))
void *cyc_vmalloc(unsigned);
void cyc_vfree(void*);
#endif
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

static inline unsigned int _check_times(unsigned x, unsigned y) {
  unsigned long long whole_ans = 
    ((unsigned long long) x)*((unsigned long long)y);
  unsigned word_ans = (unsigned)whole_ans;
  if(word_ans < whole_ans || word_ans > MAX_MALLOC_SIZE)
    _throw_badalloc();
  return word_ans;
}

#define _CYC_MAX_REGION_CONST 0
#define _CYC_MIN_ALIGNMENT (sizeof(double))

#ifdef CYC_REGION_PROFILE
extern int rgn_total_bytes;
#endif

static inline void*_fast_region_malloc(struct _RegionHandle*r, _AliasQualHandle_t aq, unsigned orig_s) {  
  if (r > (struct _RegionHandle*)_CYC_MAX_REGION_CONST && r->curr != 0) { 
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
  return _region_malloc(r,aq,orig_s); 
}

//doesn't make sense to fast malloc with reaps
#ifndef DISABLE_REAPS
#define _fast_region_malloc _region_malloc
#endif

#ifdef CYC_REGION_PROFILE
/* see macros below for usage. defined in runtime_memory.c */
void* _profile_GC_malloc(int,const char*,const char*,int);
void* _profile_GC_malloc_atomic(int,const char*,const char*,int);
void* _profile_GC_calloc(unsigned,unsigned,const char*,const char*,int);
void* _profile_GC_calloc_atomic(unsigned,unsigned,const char*,const char*,int);
void* _profile_region_malloc(struct _RegionHandle*,_AliasQualHandle_t,unsigned,const char*,const char*,int);
void* _profile_region_calloc(struct _RegionHandle*,_AliasQualHandle_t,unsigned,unsigned,const char *,const char*,int);
void * _profile_aqual_malloc(_AliasQualHandle_t aq, unsigned int s,const char *file, const char *func, int lineno);
void * _profile_aqual_calloc(_AliasQualHandle_t aq, unsigned int t1,unsigned int t2,const char *file, const char *func, int lineno);
struct _RegionHandle _profile_new_region(unsigned int i, const char*,const char*,const char*,int);
void _profile_free_region(struct _RegionHandle*,const char*,const char*,int);
#ifndef RUNTIME_CYC
#define _new_region(i,n) _profile_new_region(i,n,__FILE__,__FUNCTION__,__LINE__)
#define _free_region(r) _profile_free_region(r,__FILE__,__FUNCTION__,__LINE__)
#define _region_malloc(rh,aq,n) _profile_region_malloc(rh,aq,n,__FILE__,__FUNCTION__,__LINE__)
#define _region_calloc(rh,aq,n,t) _profile_region_calloc(rh,aq,n,t,__FILE__,__FUNCTION__,__LINE__)
#define _aqual_malloc(aq,n) _profile_aqual_malloc(aq,n,__FILE__,__FUNCTION__,__LINE__)
#define _aqual_calloc(aq,n,t) _profile_aqual_calloc(aq,n,t,__FILE__,__FUNCTION__,__LINE__)
#endif
#define _cycalloc(n) _profile_GC_malloc(n,__FILE__,__FUNCTION__,__LINE__)
#define _cycalloc_atomic(n) _profile_GC_malloc_atomic(n,__FILE__,__FUNCTION__,__LINE__)
#define _cyccalloc(n,s) _profile_GC_calloc(n,s,__FILE__,__FUNCTION__,__LINE__)
#define _cyccalloc_atomic(n,s) _profile_GC_calloc_atomic(n,s,__FILE__,__FUNCTION__,__LINE__)
#endif //CYC_REGION_PROFILE
#endif //_CYC_INCLUDE_H
 struct Cyc_Core_Opt{void*v;};
# 121 "core.h"
int Cyc_Core_intcmp(int,int);extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};
# 173
extern struct _RegionHandle*Cyc_Core_heap_region;struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 54 "list.h"
extern struct Cyc_List_List*Cyc_List_list(struct _fat_ptr);
# 61
extern int Cyc_List_length(struct Cyc_List_List*);
# 76
extern struct Cyc_List_List*Cyc_List_map(void*(*)(void*),struct Cyc_List_List*);
# 83
extern struct Cyc_List_List*Cyc_List_map_c(void*(*)(void*,void*),void*,struct Cyc_List_List*);
# 86
extern struct Cyc_List_List*Cyc_List_rmap_c(struct _RegionHandle*,void*(*)(void*,void*),void*,struct Cyc_List_List*);
# 94
extern struct Cyc_List_List*Cyc_List_map2(void*(*)(void*,void*),struct Cyc_List_List*,struct Cyc_List_List*);
# 133
extern void Cyc_List_iter(void(*)(void*),struct Cyc_List_List*);
# 135
extern void Cyc_List_iter_c(void(*)(void*,void*),void*,struct Cyc_List_List*);
# 161
extern struct Cyc_List_List*Cyc_List_revappend(struct Cyc_List_List*,struct Cyc_List_List*);
# 178
extern struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*);
# 220
extern struct Cyc_List_List*Cyc_List_rimp_merge_sort(int(*)(void*,void*),struct Cyc_List_List*);
# 254
extern int Cyc_List_forall_c(int(*)(void*,void*),void*,struct Cyc_List_List*);
# 258
extern int Cyc_List_exists(int(*)(void*),struct Cyc_List_List*);
# 261
extern int Cyc_List_exists_c(int(*)(void*,void*),void*,struct Cyc_List_List*);
# 270
extern struct Cyc_List_List*Cyc_List_zip(struct Cyc_List_List*,struct Cyc_List_List*);
# 276
extern struct Cyc_List_List*Cyc_List_rzip(struct _RegionHandle*,struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_List_List*);struct _tuple0{struct Cyc_List_List*f0;struct Cyc_List_List*f1;};
# 303
extern struct _tuple0 Cyc_List_rsplit(struct _RegionHandle*,struct _RegionHandle*,struct Cyc_List_List*);
# 322
extern int Cyc_List_mem(int(*)(void*,void*),struct Cyc_List_List*,void*);
# 336
extern void*Cyc_List_assoc_cmp(int(*)(void*,void*),struct Cyc_List_List*,void*);
# 383
extern int Cyc_List_list_cmp(int(*)(void*,void*),struct Cyc_List_List*,struct Cyc_List_List*);struct Cyc___cycFILE;
# 53 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};
# 73
extern struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);
# 100
extern int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);
# 38 "string.h"
extern unsigned long Cyc_strlen(struct _fat_ptr);
# 49 "string.h"
extern int Cyc_strcmp(struct _fat_ptr,struct _fat_ptr);
extern int Cyc_strptrcmp(struct _fat_ptr*,struct _fat_ptr*);struct Cyc_Relations_Reln;struct Cyc_AssnDef_ExistAssnFn;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple1{union Cyc_Absyn_Nmspace f0;struct _fat_ptr*f1;};
# 145 "absyn.h"
enum Cyc_Absyn_Scope{Cyc_Absyn_Static =0U,Cyc_Absyn_Abstract =1U,Cyc_Absyn_Public =2U,Cyc_Absyn_Extern =3U,Cyc_Absyn_ExternC =4U,Cyc_Absyn_Register =5U};struct Cyc_Absyn_Tqual{int print_const: 1;int q_volatile: 1;int q_restrict: 1;int real_const: 1;unsigned loc;};
# 166
enum Cyc_Absyn_Size_of{Cyc_Absyn_Char_sz =0U,Cyc_Absyn_Short_sz =1U,Cyc_Absyn_Int_sz =2U,Cyc_Absyn_Long_sz =3U,Cyc_Absyn_LongLong_sz =4U};
enum Cyc_Absyn_Sign{Cyc_Absyn_Signed =0U,Cyc_Absyn_Unsigned =1U,Cyc_Absyn_None =2U};
enum Cyc_Absyn_AggrKind{Cyc_Absyn_StructA =0U,Cyc_Absyn_UnionA =1U};
# 170
enum Cyc_Absyn_AliasQualVal{Cyc_Absyn_Aliasable_qual =0U,Cyc_Absyn_Unique_qual =1U,Cyc_Absyn_Refcnt_qual =2U,Cyc_Absyn_Restricted_qual =3U};
# 186 "absyn.h"
enum Cyc_Absyn_AliasHint{Cyc_Absyn_UniqueHint =0U,Cyc_Absyn_RefcntHint =1U,Cyc_Absyn_RestrictedHint =2U,Cyc_Absyn_NoHint =3U};
# 192
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_EffKind =3U,Cyc_Absyn_IntKind =4U,Cyc_Absyn_BoolKind =5U,Cyc_Absyn_PtrBndKind =6U,Cyc_Absyn_AqualKind =7U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasHint aliashint;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;void*aquals_bound;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*eff;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;void*autoreleased;void*aqual;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*qual_bnd;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_AssnDef_ExistAssnFn*requires_assn;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_AssnDef_ExistAssnFn*ensures_assn;struct Cyc_Absyn_Exp*throws_clause;struct Cyc_AssnDef_ExistAssnFn*throws_assn;struct Cyc_Absyn_Vardecl*return_value;struct Cyc_List_List*arg_vardecls;struct Cyc_List_List*effconstr;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple1*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};
# 313
union Cyc_Absyn_DatatypeInfo Cyc_Absyn_UnknownDatatype(struct Cyc_Absyn_UnknownDatatypeInfo);struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple1*datatype_name;struct _tuple1*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple2{struct Cyc_Absyn_Datatypedecl*f0;struct Cyc_Absyn_Datatypefield*f1;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple2 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};struct _tuple3{enum Cyc_Absyn_AggrKind f0;struct _tuple1*f1;struct Cyc_Core_Opt*f2;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple3 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};
# 333
union Cyc_Absyn_AggrInfo Cyc_Absyn_UnknownAggr(enum Cyc_Absyn_AggrKind,struct _tuple1*,struct Cyc_Core_Opt*);struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_AqualConstCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_AliasQualVal f1;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple1*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _fat_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct{int tag;void*f1;};struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct{int tag;void*f1;void*f2;};struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct{int tag;void*f1;void*f2;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_Cvar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;void*f4;const char*f5;const char*f6;int f7;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;int f2;struct Cyc_List_List*f3;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple4{enum Cyc_Absyn_Sign f0;char f1;};struct _union_Cnst_Char_c{int tag;struct _tuple4 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple5{enum Cyc_Absyn_Sign f0;short f1;};struct _union_Cnst_Short_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f0;int f1;};struct _union_Cnst_Int_c{int tag;struct _tuple6 val;};struct _tuple7{enum Cyc_Absyn_Sign f0;long long f1;};struct _union_Cnst_LongLong_c{int tag;struct _tuple7 val;};struct _tuple8{struct _fat_ptr f0;int f1;};struct _union_Cnst_Float_c{int tag;struct _tuple8 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 522 "absyn.h"
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U,Cyc_Absyn_Tagof =19U};
# 529
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};
# 547
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};
# 561
enum Cyc_Absyn_MallocKind{Cyc_Absyn_Malloc =0U,Cyc_Absyn_Calloc =1U,Cyc_Absyn_Vmalloc =2U};struct Cyc_Absyn_MallocInfo{enum Cyc_Absyn_MallocKind mknd;struct Cyc_Absyn_Exp*rgn;struct Cyc_Absyn_Exp*aqual;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple9{struct _fat_ptr*f0;struct Cyc_Absyn_Tqual f1;void*f2;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple9*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;int f2;struct Cyc_List_List*f3;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Assert_false_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;int is_proto;struct Cyc_Absyn_Exp*rename;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple1*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;enum Cyc_Absyn_Scope orig_scope;int escapes;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*qual_bnd;struct Cyc_List_List*fields;int tagged;struct Cyc_List_List*effconstr;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple1*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple1*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple1*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};
# 912 "absyn.h"
int Cyc_Absyn_qvar_cmp(struct _tuple1*,struct _tuple1*);
# 915
int Cyc_Absyn_tvar_cmp(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*);
# 920
extern union Cyc_Absyn_Nmspace Cyc_Absyn_Loc_n;
# 927
struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(unsigned);
# 933
void*Cyc_Absyn_compress(void*);
# 937
int Cyc_Absyn_type2bool(int,void*);
# 946
void*Cyc_Absyn_new_evar(struct Cyc_Core_Opt*,struct Cyc_Core_Opt*);
# 951
extern void*Cyc_Absyn_uint_type;extern void*Cyc_Absyn_ulong_type;extern void*Cyc_Absyn_ulonglong_type;
# 953
extern void*Cyc_Absyn_sint_type;extern void*Cyc_Absyn_slong_type;extern void*Cyc_Absyn_slonglong_type;
# 960
extern void*Cyc_Absyn_heap_rgn_type;
# 962
extern void*Cyc_Absyn_al_qual_type;
# 964
extern void*Cyc_Absyn_empty_effect;
# 966
extern void*Cyc_Absyn_true_type;extern void*Cyc_Absyn_false_type;
# 968
extern void*Cyc_Absyn_void_type;extern void*Cyc_Absyn_var_type(struct Cyc_Absyn_Tvar*);extern void*Cyc_Absyn_join_eff(struct Cyc_List_List*);extern void*Cyc_Absyn_regionsof_eff(void*);extern void*Cyc_Absyn_enum_type(struct _tuple1*,struct Cyc_Absyn_Enumdecl*);
# 999
extern void*Cyc_Absyn_fat_bound_type;
# 1001
void*Cyc_Absyn_thin_bounds_exp(struct Cyc_Absyn_Exp*);
# 1003
void*Cyc_Absyn_bounds_one (void);
void*Cyc_Absyn_cvar_type(struct Cyc_Core_Opt*);
# 1006
void*Cyc_Absyn_fatconst (void);
void*Cyc_Absyn_thinconst (void);
# 1009
void*Cyc_Absyn_pointer_type(struct Cyc_Absyn_PtrInfo);
# 1013
void*Cyc_Absyn_atb_type(void*,void*,void*,struct Cyc_Absyn_Tqual,void*,void*,void*);
# 1032
void*Cyc_Absyn_datatype_type(union Cyc_Absyn_DatatypeInfo,struct Cyc_List_List*);
# 1034
void*Cyc_Absyn_aggr_type(union Cyc_Absyn_AggrInfo,struct Cyc_List_List*);
# 1036
void*Cyc_Absyn_aqualsof_type(void*);
# 1049
struct Cyc_Absyn_Exp*Cyc_Absyn_new_exp(void*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_New_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_copy_exp(struct Cyc_Absyn_Exp*);
struct Cyc_Absyn_Exp*Cyc_Absyn_const_exp(union Cyc_Absyn_Cnst,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_null_exp(unsigned);
# 1059
struct Cyc_Absyn_Exp*Cyc_Absyn_uint_exp(unsigned,unsigned);
# 1066
struct Cyc_Absyn_Exp*Cyc_Absyn_varb_exp(void*,unsigned);
# 1069
struct Cyc_Absyn_Exp*Cyc_Absyn_pragma_exp(struct _fat_ptr,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_primop_exp(enum Cyc_Absyn_Primop,struct Cyc_List_List*,unsigned);
# 1073
struct Cyc_Absyn_Exp*Cyc_Absyn_swap_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1083
struct Cyc_Absyn_Exp*Cyc_Absyn_assignop_exp(struct Cyc_Absyn_Exp*,struct Cyc_Core_Opt*,struct Cyc_Absyn_Exp*,unsigned);
# 1085
struct Cyc_Absyn_Exp*Cyc_Absyn_increment_exp(struct Cyc_Absyn_Exp*,enum Cyc_Absyn_Incrementor,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_conditional_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_and_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_or_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_seq_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1092
struct Cyc_Absyn_Exp*Cyc_Absyn_throw_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_rethrow_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_noinstantiate_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_instantiate_exp(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_cast_exp(void*,struct Cyc_Absyn_Exp*,int,enum Cyc_Absyn_Coercion,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_address_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_sizeoftype_exp(void*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_sizeofexp_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_offsetof_exp(void*,struct Cyc_List_List*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_deref_exp(struct Cyc_Absyn_Exp*,unsigned);
# 1104
struct Cyc_Absyn_Exp*Cyc_Absyn_subscript_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1108
struct Cyc_Absyn_Exp*Cyc_Absyn_valueof_exp(void*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_asm_exp(int,struct _fat_ptr,struct Cyc_List_List*,struct Cyc_List_List*,struct Cyc_List_List*,unsigned);
# 1112
struct Cyc_Absyn_Exp*Cyc_Absyn_extension_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_assert_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_assert_false_exp(struct Cyc_Absyn_Exp*,unsigned);
# 1153
struct Cyc_Absyn_Decl*Cyc_Absyn_alias_decl(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Vardecl*Cyc_Absyn_new_vardecl(unsigned,struct _tuple1*,void*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*);
# 1199
struct Cyc_Absyn_Aggrfield*Cyc_Absyn_lookup_field(struct Cyc_List_List*,struct _fat_ptr*);
# 1201
struct Cyc_Absyn_Aggrfield*Cyc_Absyn_lookup_decl_field(struct Cyc_Absyn_Aggrdecl*,struct _fat_ptr*);
# 1213
struct Cyc_Absyn_Aggrdecl*Cyc_Absyn_get_known_aggrdecl(union Cyc_Absyn_AggrInfo);
# 63 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
# 67
struct _fat_ptr Cyc_Absynpp_kindbound2string(void*);
# 77
struct _fat_ptr Cyc_Absynpp_tvar2string(struct Cyc_Absyn_Tvar*);
# 29 "kinds.h"
extern struct Cyc_Absyn_Kind Cyc_Kinds_ak;
extern struct Cyc_Absyn_Kind Cyc_Kinds_bk;
extern struct Cyc_Absyn_Kind Cyc_Kinds_mk;
extern struct Cyc_Absyn_Kind Cyc_Kinds_ek;
extern struct Cyc_Absyn_Kind Cyc_Kinds_ik;
extern struct Cyc_Absyn_Kind Cyc_Kinds_boolk;
extern struct Cyc_Absyn_Kind Cyc_Kinds_ptrbk;
extern struct Cyc_Absyn_Kind Cyc_Kinds_aqk;
# 57 "kinds.h"
extern struct Cyc_Core_Opt Cyc_Kinds_eko;
extern struct Cyc_Core_Opt Cyc_Kinds_boolko;
extern struct Cyc_Core_Opt Cyc_Kinds_ptrbko;
# 76 "kinds.h"
struct Cyc_Core_Opt*Cyc_Kinds_kind_to_opt(struct Cyc_Absyn_Kind*);
# 78
void*Cyc_Kinds_copy_kindbound(void*);
# 83
struct Cyc_Absyn_Kind*Cyc_Kinds_tvar_kind(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Kind*);
struct Cyc_Core_Opt Cyc_Kinds_tvar_kind_opt(struct Cyc_Absyn_Tvar*);
# 40 "warn.h"
void*Cyc_Warn_impos(struct _fat_ptr,struct _fat_ptr);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Int_Warn_Warg_struct{int tag;int f1;};
# 67
void Cyc_Warn_err2(unsigned,struct _fat_ptr);
# 69
void Cyc_Warn_warn2(unsigned,struct _fat_ptr);
# 71
void*Cyc_Warn_impos2(struct _fat_ptr);
# 59 "flags.h"
extern int Cyc_Flags_warn_alias_coerce;struct _tuple12{unsigned f0;int f1;};
# 28 "evexp.h"
extern struct _tuple12 Cyc_Evexp_eval_const_uint_exp(struct Cyc_Absyn_Exp*);
# 50 "evexp.h"
extern int Cyc_Evexp_uint_exp_cmp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*);
# 29 "unify.h"
int Cyc_Unify_unify(void*,void*);
# 35
void Cyc_Unify_occurs(void*,struct _RegionHandle*,struct Cyc_List_List*,void*);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 41 "relations-ap.h"
extern union Cyc_Relations_RelnOp Cyc_Relations_RParam(unsigned);extern union Cyc_Relations_RelnOp Cyc_Relations_RParamNumelts(unsigned);extern union Cyc_Relations_RelnOp Cyc_Relations_RReturn (void);
# 50
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};
# 84
struct Cyc_List_List*Cyc_Relations_exp2relns(struct _RegionHandle*,struct Cyc_Absyn_Exp*);
# 71 "tcutil.h"
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp_constrain(void*,void*,int);
# 83
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*,void*);
# 93
void*Cyc_Tcutil_copy_type(void*);
# 96
struct Cyc_Absyn_Exp*Cyc_Tcutil_deep_copy_exp(int,struct Cyc_Absyn_Exp*);
# 100
struct Cyc_Absyn_Kind*Cyc_Tcutil_type_kind(void*);
void Cyc_Tcutil_unchecked_cast(struct Cyc_Absyn_Exp*,void*,enum Cyc_Absyn_Coercion);
# 103
int Cyc_Tcutil_coerce_sint_type(struct Cyc_Absyn_Exp*);
# 107
int Cyc_Tcutil_coerce_assign(struct Cyc_List_List*,struct Cyc_Absyn_Exp*,void*);
# 129 "tcutil.h"
int Cyc_Tcutil_typecmp(void*,void*);
# 132
void*Cyc_Tcutil_substitute(struct Cyc_List_List*,void*);
# 135
void*Cyc_Tcutil_rsubstitute(struct _RegionHandle*,struct Cyc_List_List*,void*);
# 196
int Cyc_Tcutil_is_noalias_pointer(struct Cyc_List_List*,void*,int);
# 201
int Cyc_Tcutil_is_noalias_path(struct Cyc_List_List*,struct Cyc_Absyn_Exp*);struct _tuple13{int f0;void*f1;};
# 213
void*Cyc_Tcutil_normalize_effect(void*);
# 216
struct Cyc_Absyn_Tvar*Cyc_Tcutil_new_tvar(void*);
# 227
int Cyc_Tcutil_is_const_exp(struct Cyc_Absyn_Exp*);
# 247
void*Cyc_Tcutil_promote_array(void*,void*,void*,int);
# 254
int Cyc_Tcutil_force_type2bool(int,void*);
# 54 "attributes.h"
int Cyc_Atts_same_atts(struct Cyc_List_List*,struct Cyc_List_List*);
# 68
int Cyc_Atts_fntype_att(void*);
# 79
int Cyc_Atts_attribute_cmp(void*,void*);struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};struct Cyc_Set_Set;
# 149 "assndef.h"
extern struct Cyc_Absyn_Vardecl*Cyc_AssnDef_memory;struct Cyc_AssnDef_AssnFn{struct Cyc_List_List*actuals;void*assn;};struct Cyc_AssnDef_ExistAssnFn{struct Cyc_AssnDef_AssnFn*af;struct Cyc_Set_Set*existvars;};struct Cyc_AssnDef_AssnMap{void*assn;struct Cyc_Dict_Dict map;};
# 273 "assndef.h"
extern struct Cyc_AssnDef_ExistAssnFn*Cyc_AssnDef_assnmap2existassnfn(struct Cyc_AssnDef_AssnMap,struct Cyc_List_List*);
# 331 "assndef.h"
extern struct Cyc_AssnDef_AssnMap*Cyc_AssnDef_subst_vardecls(struct Cyc_List_List*,struct Cyc_List_List*,struct Cyc_AssnDef_AssnMap*);
# 45 "vcgen.h"
extern struct Cyc_AssnDef_AssnMap Cyc_Vcgen_clause2assn(struct Cyc_Absyn_Exp*);
# 17 "bansheeif.h"
void*Cyc_BansheeIf_equality_constraint(void*,void*);
# 28
int Cyc_BansheeIf_add_constraint(unsigned,void*);
# 10 "subtype.h"
enum Cyc_Absyn_Coercion Cyc_Subtype_coercible_warn(unsigned,void*,void*);
# 45 "tcutil.cyc"
int Cyc_Tcutil_is_void_type(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);if(*((int*)_Tmp0)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1)==0)
return 1;else{goto _LL3;}}else{_LL3:
 return 0;};}
# 51
int Cyc_Tcutil_is_array_type(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);if(*((int*)_Tmp0)==5)
return 1;else{
return 0;};}
# 57
int Cyc_Tcutil_is_heap_rgn_type(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);if(*((int*)_Tmp0)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1)==6)
return 1;else{goto _LL3;}}else{_LL3:
 return 0;};}
# 63
int Cyc_Tcutil_is_pointer_type(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);if(*((int*)_Tmp0)==4)
return 1;else{
return 0;};}
# 70
int Cyc_Tcutil_is_char_type(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);if(*((int*)_Tmp0)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1)==1){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1)->f2==Cyc_Absyn_Char_sz)
return 1;else{goto _LL3;}}else{goto _LL3;}}else{_LL3:
 return 0;};}
# 77
int Cyc_Tcutil_is_any_int_type(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);if(*((int*)_Tmp0)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1)==1)
return 1;else{goto _LL3;}}else{_LL3:
 return 0;};}
# 84
int Cyc_Tcutil_is_any_float_type(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);if(*((int*)_Tmp0)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1)==2)
return 1;else{goto _LL3;}}else{_LL3:
 return 0;};}
# 91
int Cyc_Tcutil_is_integral_type(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);void*_Tmp1;if(*((int*)_Tmp0)==0){_Tmp1=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1;{void*c=_Tmp1;
# 94
switch(*((int*)c)){case 1:
 goto _LL9;case 5: _LL9:
 goto _LLB;case 19: _LLB:
 goto _LLD;case 20: _LLD:
 return 1;default:
 return 0;};}}else{
# 101
return 0;};}
# 104
int Cyc_Tcutil_is_signed_type(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);if(*((int*)_Tmp0)==0)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1)){case 1: if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1)->f1==Cyc_Absyn_Signed)
return 1;else{goto _LL5;}case 2:
 return 1;default: goto _LL5;}else{_LL5:
 return 0;};}
# 111
int Cyc_Tcutil_is_arithmetic_type(void*t){
return Cyc_Tcutil_is_integral_type(t)|| Cyc_Tcutil_is_any_float_type(t);}
# 114
int Cyc_Tcutil_is_strict_arithmetic_type(void*t){
return Cyc_Tcutil_is_any_int_type(t)|| Cyc_Tcutil_is_any_float_type(t);}
# 117
int Cyc_Tcutil_is_tag_type(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);if(*((int*)_Tmp0)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1)==5)
return 1;else{goto _LL3;}}else{_LL3:
 return 0;};}
# 123
int Cyc_Tcutil_is_function_type(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);if(*((int*)_Tmp0)==6)
return 1;else{
return 0;};}
# 129
int Cyc_Tcutil_is_typedef_type(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);if(*((int*)_Tmp0)==8)
return 1;else{
return 0;};}
# 135
int Cyc_Tcutil_is_boxed(void*t){
return(int)Cyc_Tcutil_type_kind(t)->kind==2;}
# 143
int Cyc_Tcutil_is_integral(struct Cyc_Absyn_Exp*e){
void*_Tmp0=Cyc_Absyn_compress(_check_null(e->topt));void*_Tmp1;if(*((int*)_Tmp0)==1)
return Cyc_Unify_unify(_check_null(e->topt),Cyc_Absyn_sint_type);else{_Tmp1=_Tmp0;{void*t=_Tmp1;
return Cyc_Tcutil_is_integral_type(t);}};}
# 151
int Cyc_Tcutil_is_numeric(struct Cyc_Absyn_Exp*e){
if(Cyc_Tcutil_is_integral(e))
return 1;{
void*_Tmp0=Cyc_Absyn_compress(_check_null(e->topt));if(*((int*)_Tmp0)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1)==2)
return 1;else{goto _LL3;}}else{_LL3:
 return 0;};}}
# 161
int Cyc_Tcutil_is_zeroterm_pointer_type(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);void*_Tmp1;if(*((int*)_Tmp0)==4){_Tmp1=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.zero_term;{void*ztl=_Tmp1;
# 164
return Cyc_Tcutil_force_type2bool(0,ztl);}}else{
return 0;};}
# 170
int Cyc_Tcutil_is_nullable_pointer_type(void*t,int def){
void*_Tmp0=Cyc_Absyn_compress(t);void*_Tmp1;if(*((int*)_Tmp0)==4){_Tmp1=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.nullable;{void*nbl=_Tmp1;
# 173
return Cyc_Tcutil_force_type2bool(def,nbl);}}else{
return 0;};}
# 178
int Cyc_Tcutil_is_cvar_type(void*cv){
void*_Tmp0=Cyc_Absyn_compress(cv);if(*((int*)_Tmp0)==3)
return 1;else{
return 0;};}
# 185
void*Cyc_Tcutil_ptrbnd_cvar_equivalent(void*t){
t=Cyc_Absyn_compress(t);
switch(*((int*)t)){case 3:
 return t;case 0: if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)t)->f1)==14)
return Cyc_Absyn_fatconst();else{goto _LL5;}default: _LL5:
 if(({void*_Tmp0=t;_Tmp0==Cyc_Absyn_bounds_one();}))return Cyc_Absyn_thinconst();else{return 0;}};}
# 195
int Cyc_Tcutil_is_fat_ptr(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);void*_Tmp1;if(*((int*)_Tmp0)==4){_Tmp1=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.bounds;{void*b=_Tmp1;
# 198
return Cyc_Unify_unify(Cyc_Absyn_fat_bound_type,b);}}else{
return 0;};}
# 205
int Cyc_Tcutil_is_fat_pointer_type_elt(void*t,void**elt_type_dest){
void*_Tmp0=Cyc_Absyn_compress(t);void*_Tmp1;void*_Tmp2;if(*((int*)_Tmp0)==4){_Tmp2=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.elt_type;_Tmp1=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.bounds;{void*elt_type=_Tmp2;void*b=_Tmp1;
# 208
if(!Cyc_Unify_unify(b,Cyc_Absyn_fat_bound_type))
return 0;
*elt_type_dest=elt_type;
return 1;}}else{
return 0;};}
# 218
int Cyc_Tcutil_is_zero_pointer_type_elt(void*t,void**elt_type_dest){
void*_Tmp0=Cyc_Absyn_compress(t);void*_Tmp1;void*_Tmp2;if(*((int*)_Tmp0)==4){_Tmp2=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.elt_type;_Tmp1=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.zero_term;{void*elt_type=_Tmp2;void*zt=_Tmp1;
# 221
*elt_type_dest=elt_type;
return Cyc_Absyn_type2bool(0,zt);}}else{
return 0;};}
# 230
int Cyc_Tcutil_is_zero_ptr_type(void*t,void**ptr_type,int*is_fat,void**elt_type){
# 232
void*_Tmp0=Cyc_Absyn_compress(t);struct Cyc_Absyn_Tqual _Tmp1;void*_Tmp2;void*_Tmp3;void*_Tmp4;switch(*((int*)_Tmp0)){case 4: _Tmp4=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.elt_type;_Tmp3=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.bounds;_Tmp2=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.zero_term;{void*elt=_Tmp4;void*bnds=_Tmp3;void*zt=_Tmp2;
# 234
if(!Cyc_Absyn_type2bool(0,zt))
return 0;
*ptr_type=t;
*elt_type=elt;
{void*_Tmp5=Cyc_Absyn_compress(bnds);if(*((int*)_Tmp5)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp5)->f1)==14){
*is_fat=1;goto _LL7;}else{goto _LLA;}}else{_LLA:
*is_fat=0;goto _LL7;}_LL7:;}
# 242
return 1;}case 5: _Tmp4=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.elt_type;_Tmp1=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.tq;_Tmp3=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.num_elts;_Tmp2=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.zero_term;{void*elt=_Tmp4;struct Cyc_Absyn_Tqual tq=_Tmp1;struct Cyc_Absyn_Exp*n=_Tmp3;void*zt=_Tmp2;
# 244
if(!Cyc_Absyn_type2bool(0,zt))
return 0;
*elt_type=elt;
*is_fat=0;
({void*_Tmp5=Cyc_Tcutil_promote_array(t,Cyc_Absyn_heap_rgn_type,Cyc_Absyn_al_qual_type,0);*ptr_type=_Tmp5;});
return 1;}default:
 return 0;};}
# 257
int Cyc_Tcutil_is_fat_pointer_type(void*t){
void*ignore=Cyc_Absyn_void_type;
return Cyc_Tcutil_is_fat_pointer_type_elt(t,& ignore);}
# 262
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp_constrain(void*,void*,int);
# 264
int Cyc_Tcutil_is_bound_one(void*b,int constrain){
if(Cyc_Tcutil_is_cvar_type(b)&& !constrain)
return 0;{
struct Cyc_Absyn_Exp*eopt=({void*_Tmp0=Cyc_Absyn_bounds_one();void*_Tmp1=b;Cyc_Tcutil_get_bounds_exp_constrain(_Tmp0,_Tmp1,constrain);});
if(eopt==0)return 0;{
struct _tuple12 _Tmp0=Cyc_Evexp_eval_const_uint_exp(eopt);int _Tmp1;unsigned _Tmp2;_Tmp2=_Tmp0.f0;_Tmp1=_Tmp0.f1;{unsigned i=_Tmp2;int known=_Tmp1;
return known && i==1U;}}}}
# 274
int Cyc_Tcutil_is_bits_only_type(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);void*_Tmp1;void*_Tmp2;switch(*((int*)_Tmp0)){case 0: _Tmp2=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1;_Tmp1=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f2;{void*c=_Tmp2;struct Cyc_List_List*ts=_Tmp1;
# 277
void*_Tmp3;switch(*((int*)c)){case 0:
 goto _LLD;case 1: _LLD:
 goto _LLF;case 2: _LLF:
 goto _LL11;case 3: _LL11:
 goto _LL13;case 20: _LL13:
 goto _LL15;case 19: _LL15:
 return 1;case 24: if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)c)->f1.UnknownAggr.tag==1)
# 285
return 0;else{_Tmp3=*((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)c)->f1.KnownAggr.val;{struct Cyc_Absyn_Aggrdecl*ad=_Tmp3;
# 287
if(ad->impl==0)
return 0;{
struct _RegionHandle _Tmp4=_new_region(0U,"rgn");struct _RegionHandle*rgn=& _Tmp4;_push_region(rgn);
{struct Cyc_List_List*inst=Cyc_List_rzip(rgn,rgn,ad->tvs,ts);
{struct Cyc_List_List*fs=_check_null(ad->impl)->fields;for(0;fs!=0;fs=fs->tl){
if(!Cyc_Tcutil_is_bits_only_type(Cyc_Tcutil_rsubstitute(rgn,inst,((struct Cyc_Absyn_Aggrfield*)fs->hd)->type))){
int _Tmp5=0;_npop_handler(0);return _Tmp5;}}}{
int _Tmp5=1;_npop_handler(0);return _Tmp5;}}
# 290
;_pop_region();}}}default:
# 295
 return 0;};}case 5: _Tmp2=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.elt_type;_Tmp1=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.zero_term;{void*t=_Tmp2;void*zero_term=_Tmp1;
# 300
return !Cyc_Absyn_type2bool(0,zero_term)&& Cyc_Tcutil_is_bits_only_type(t);}case 7: _Tmp2=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_Tmp0)->f3;{struct Cyc_List_List*fs=_Tmp2;
# 302
for(1;fs!=0;fs=fs->tl){
if(!Cyc_Tcutil_is_bits_only_type(((struct Cyc_Absyn_Aggrfield*)fs->hd)->type))
return 0;}
return 1;}default:
 return 0;};}
# 311
int Cyc_Tcutil_fast_tvar_cmp(struct Cyc_Absyn_Tvar*tv1,struct Cyc_Absyn_Tvar*tv2){
return tv1->identity - tv2->identity;}
# 315
static struct Cyc_Absyn_Tvar*Cyc_Tcutil_copy_tvar(struct Cyc_Absyn_Tvar*tv){
struct Cyc_Absyn_Tvar*_Tmp0=_cycalloc(sizeof(struct Cyc_Absyn_Tvar));_Tmp0->name=tv->name,_Tmp0->identity=-1,({void*_Tmp1=Cyc_Kinds_copy_kindbound(tv->kind);_Tmp0->kind=_Tmp1;}),_Tmp0->aquals_bound=tv->aquals_bound;return _Tmp0;}
# 318
static struct _tuple9*Cyc_Tcutil_copy_arg(struct _tuple9*arg){
void*_Tmp0;struct Cyc_Absyn_Tqual _Tmp1;void*_Tmp2;_Tmp2=arg->f0;_Tmp1=arg->f1;_Tmp0=arg->f2;{struct _fat_ptr*x=_Tmp2;struct Cyc_Absyn_Tqual y=_Tmp1;void*t=_Tmp0;
struct _tuple9*_Tmp3=_cycalloc(sizeof(struct _tuple9));_Tmp3->f0=x,_Tmp3->f1=y,({void*_Tmp4=Cyc_Tcutil_copy_type(t);_Tmp3->f2=_Tmp4;});return _Tmp3;}}struct _tuple14{struct Cyc_Absyn_Tqual f0;void*f1;};
# 322
static struct _tuple14*Cyc_Tcutil_copy_tqt(struct _tuple14*arg){
struct _tuple14*_Tmp0=_cycalloc(sizeof(struct _tuple14));_Tmp0->f0=(*arg).f0,({void*_Tmp1=Cyc_Tcutil_copy_type((*arg).f1);_Tmp0->f1=_Tmp1;});return _Tmp0;}
# 325
static struct Cyc_Absyn_Exp*Cyc_Tcutil_deep_copy_exp_opt(int preserve_types,struct Cyc_Absyn_Exp*e){
if(e==0)return 0;else{return Cyc_Tcutil_deep_copy_exp(preserve_types,e);}}
# 328
static struct Cyc_Absyn_Aggrfield*Cyc_Tcutil_copy_field(struct Cyc_Absyn_Aggrfield*f){
struct Cyc_Absyn_Aggrfield*_Tmp0=_cycalloc(sizeof(struct Cyc_Absyn_Aggrfield));_Tmp0->name=f->name,_Tmp0->tq=f->tq,({void*_Tmp1=Cyc_Tcutil_copy_type(f->type);_Tmp0->type=_Tmp1;}),_Tmp0->width=f->width,_Tmp0->attributes=f->attributes,({
struct Cyc_Absyn_Exp*_Tmp1=Cyc_Tcutil_deep_copy_exp_opt(1,f->requires_clause);_Tmp0->requires_clause=_Tmp1;});return _Tmp0;}struct _tuple15{void*f0;void*f1;};
# 332
static struct _tuple15*Cyc_Tcutil_copy_type_pair(struct _tuple15*x){
void*_Tmp0;void*_Tmp1;_Tmp1=x->f0;_Tmp0=x->f1;{void*r1=_Tmp1;void*r2=_Tmp0;
struct _tuple15*_Tmp2=_cycalloc(sizeof(struct _tuple15));({void*_Tmp3=Cyc_Tcutil_copy_type(r1);_Tmp2->f0=_Tmp3;}),({void*_Tmp3=Cyc_Tcutil_copy_type(r2);_Tmp2->f1=_Tmp3;});return _Tmp2;}}
# 336
static void*Cyc_Tcutil_copy_effconstr(void*fc){
void*_Tmp0;void*_Tmp1;switch(*((int*)fc)){case 0: _Tmp1=(void*)((struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*)fc)->f1;{void*e=_Tmp1;
# 339
return(void*)({struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*_Tmp2=_cycalloc(sizeof(struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct));_Tmp2->tag=0,({void*_Tmp3=Cyc_Tcutil_copy_type(e);_Tmp2->f1=_Tmp3;});_Tmp2;});}case 1: _Tmp1=(void*)((struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*)fc)->f1;_Tmp0=(void*)((struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*)fc)->f2;{void*e1=_Tmp1;void*e2=_Tmp0;
# 341
return(void*)({struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*_Tmp2=_cycalloc(sizeof(struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct));_Tmp2->tag=1,({void*_Tmp3=Cyc_Tcutil_copy_type(e1);_Tmp2->f1=_Tmp3;}),({void*_Tmp3=Cyc_Tcutil_copy_type(e2);_Tmp2->f2=_Tmp3;});_Tmp2;});}default: _Tmp1=(void*)((struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)fc)->f1;_Tmp0=(void*)((struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)fc)->f2;{void*e1=_Tmp1;void*e2=_Tmp0;
# 343
return(void*)({struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*_Tmp2=_cycalloc(sizeof(struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct));_Tmp2->tag=2,({void*_Tmp3=Cyc_Tcutil_copy_type(e1);_Tmp2->f1=_Tmp3;}),({void*_Tmp3=Cyc_Tcutil_copy_type(e2);_Tmp2->f2=_Tmp3;});_Tmp2;});}};}
# 346
static void*Cyc_Tcutil_tvar2type(struct Cyc_Absyn_Tvar*t){
return Cyc_Absyn_var_type(Cyc_Tcutil_copy_tvar(t));}
# 349
static struct Cyc_Absyn_Vardecl*Cyc_Tcutil_copy_vardecl_opt(struct Cyc_Absyn_Vardecl*vdopt){
if(vdopt==0)return 0;{
struct Cyc_Absyn_Vardecl*vd;vd=_cycalloc(sizeof(struct Cyc_Absyn_Vardecl)),*vd=*vdopt;
({void*_Tmp0=Cyc_Tcutil_copy_type(vd->type);vd->type=_Tmp0;});
({struct Cyc_Absyn_Exp*_Tmp0=Cyc_Tcutil_deep_copy_exp_opt(1,vd->initializer);vd->initializer=_Tmp0;});
if(vd->rgn!=0)
({void*_Tmp0=Cyc_Tcutil_copy_type(vd->rgn);vd->rgn=_Tmp0;});
return vd;}}
# 359
void*Cyc_Tcutil_copy_type(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);enum Cyc_Absyn_AggrKind _Tmp1;void*_Tmp2;void*_Tmp3;void*_Tmp4;void*_Tmp5;void*_Tmp6;void*_Tmp7;void*_Tmp8;void*_Tmp9;int _TmpA;unsigned _TmpB;void*_TmpC;void*_TmpD;void*_TmpE;void*_TmpF;void*_Tmp10;void*_Tmp11;struct Cyc_Absyn_Tqual _Tmp12;void*_Tmp13;void*_Tmp14;switch(*((int*)_Tmp0)){case 0: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f2==0){_Tmp14=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1;{void*c=_Tmp14;
return t;}}else{_Tmp14=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1;_Tmp13=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f2;{void*c=_Tmp14;struct Cyc_List_List*ts=_Tmp13;
return(void*)({struct Cyc_Absyn_AppType_Absyn_Type_struct*_Tmp15=_cycalloc(sizeof(struct Cyc_Absyn_AppType_Absyn_Type_struct));_Tmp15->tag=0,_Tmp15->f1=c,({struct Cyc_List_List*_Tmp16=Cyc_List_map(Cyc_Tcutil_copy_type,ts);_Tmp15->f2=_Tmp16;});_Tmp15;});}}case 3:
 return t;case 1:
 return t;case 2: _Tmp14=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Tvar*tv=_Tmp14;
return Cyc_Absyn_var_type(Cyc_Tcutil_copy_tvar(tv));}case 4: _Tmp14=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.elt_type;_Tmp12=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.elt_tq;_Tmp13=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.eff;_Tmp11=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.nullable;_Tmp10=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.bounds;_TmpF=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.zero_term;_TmpE=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.ptrloc;_TmpD=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.autoreleased;_TmpC=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.aqual;{void*elt=_Tmp14;struct Cyc_Absyn_Tqual tq=_Tmp12;void*rgn=_Tmp13;void*nbl=_Tmp11;void*bs=_Tmp10;void*zt=_TmpF;struct Cyc_Absyn_PtrLoc*loc=_TmpE;void*rel=_TmpD;void*aq=_TmpC;
# 367
void*elt2=Cyc_Tcutil_copy_type(elt);
void*rgn2=Cyc_Tcutil_copy_type(rgn);
void*nbl2=Cyc_Tcutil_copy_type(nbl);
struct Cyc_Absyn_Tqual tq2=tq;
# 372
void*bs2=Cyc_Tcutil_copy_type(bs);
void*zt2=Cyc_Tcutil_copy_type(zt);
void*rel2=Cyc_Tcutil_copy_type(rel);
return(void*)({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_Tmp15=_cycalloc(sizeof(struct Cyc_Absyn_PointerType_Absyn_Type_struct));_Tmp15->tag=4,_Tmp15->f1.elt_type=elt2,_Tmp15->f1.elt_tq=tq2,_Tmp15->f1.ptr_atts.eff=rgn2,_Tmp15->f1.ptr_atts.nullable=nbl2,_Tmp15->f1.ptr_atts.bounds=bs2,_Tmp15->f1.ptr_atts.zero_term=zt2,_Tmp15->f1.ptr_atts.ptrloc=loc,_Tmp15->f1.ptr_atts.autoreleased=rel2,_Tmp15->f1.ptr_atts.aqual=aq;_Tmp15;});}case 5: _Tmp14=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.elt_type;_Tmp12=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.tq;_Tmp13=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.num_elts;_Tmp11=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.zero_term;_TmpB=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.zt_loc;{void*et=_Tmp14;struct Cyc_Absyn_Tqual tq=_Tmp12;struct Cyc_Absyn_Exp*eo=_Tmp13;void*zt=_Tmp11;unsigned ztl=_TmpB;
# 377
return(void*)({struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_Tmp15=_cycalloc(sizeof(struct Cyc_Absyn_ArrayType_Absyn_Type_struct));_Tmp15->tag=5,({void*_Tmp16=Cyc_Tcutil_copy_type(et);_Tmp15->f1.elt_type=_Tmp16;}),_Tmp15->f1.tq=tq,({struct Cyc_Absyn_Exp*_Tmp16=Cyc_Tcutil_deep_copy_exp_opt(1,eo);_Tmp15->f1.num_elts=_Tmp16;}),({
void*_Tmp16=Cyc_Tcutil_copy_type(zt);_Tmp15->f1.zero_term=_Tmp16;}),_Tmp15->f1.zt_loc=ztl;_Tmp15;});}case 6: _Tmp14=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.tvars;_Tmp13=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.effect;_Tmp12=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.ret_tqual;_Tmp11=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.ret_type;_Tmp10=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.args;_TmpA=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.c_varargs;_TmpF=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.cyc_varargs;_TmpE=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.qual_bnd;_TmpD=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.attributes;_TmpC=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.requires_clause;_Tmp9=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.requires_assn;_Tmp8=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.ensures_clause;_Tmp7=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.ensures_assn;_Tmp6=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.throws_clause;_Tmp5=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.throws_assn;_Tmp4=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.return_value;_Tmp3=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.arg_vardecls;_Tmp2=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.effconstr;{struct Cyc_List_List*tvs=_Tmp14;void*effopt=_Tmp13;struct Cyc_Absyn_Tqual rt_tq=_Tmp12;void*rt=_Tmp11;struct Cyc_List_List*args=_Tmp10;int c_varargs=_TmpA;struct Cyc_Absyn_VarargInfo*cyc_varargs=_TmpF;struct Cyc_List_List*qb=_TmpE;struct Cyc_List_List*atts=_TmpD;struct Cyc_Absyn_Exp*req=_TmpC;struct Cyc_AssnDef_ExistAssnFn*req_rlns=_Tmp9;struct Cyc_Absyn_Exp*ens=_Tmp8;struct Cyc_AssnDef_ExistAssnFn*ens_rlns=_Tmp7;struct Cyc_Absyn_Exp*thrws=_Tmp6;struct Cyc_AssnDef_ExistAssnFn*thrws_rlns=_Tmp5;struct Cyc_Absyn_Vardecl*ret_val=_Tmp4;struct Cyc_List_List*argvds=_Tmp3;struct Cyc_List_List*effc=_Tmp2;
# 381
struct Cyc_List_List*tvs2=({struct Cyc_List_List*(*_Tmp15)(struct Cyc_Absyn_Tvar*(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*))Cyc_List_map;_Tmp15;})(Cyc_Tcutil_copy_tvar,tvs);
void*effopt2=effopt==0?0: Cyc_Tcutil_copy_type(effopt);
void*rt2=Cyc_Tcutil_copy_type(rt);
struct Cyc_List_List*args2=({struct Cyc_List_List*(*_Tmp15)(struct _tuple9*(*)(struct _tuple9*),struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct _tuple9*(*)(struct _tuple9*),struct Cyc_List_List*))Cyc_List_map;_Tmp15;})(Cyc_Tcutil_copy_arg,args);
int c_varargs2=c_varargs;
struct Cyc_Absyn_VarargInfo*cyc_varargs2=cyc_varargs==0?0:({struct Cyc_Absyn_VarargInfo*_Tmp15=_cycalloc(sizeof(struct Cyc_Absyn_VarargInfo));*_Tmp15=*cyc_varargs;_Tmp15;});
if(cyc_varargs!=0)
({void*_Tmp15=Cyc_Tcutil_copy_type(cyc_varargs->type);cyc_varargs2->type=_Tmp15;});{
struct Cyc_List_List*effc2=Cyc_List_map(Cyc_Tcutil_copy_effconstr,effc);
struct Cyc_List_List*qb2=({struct Cyc_List_List*(*_Tmp15)(struct _tuple15*(*)(struct _tuple15*),struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct _tuple15*(*)(struct _tuple15*),struct Cyc_List_List*))Cyc_List_map;_Tmp15;})(Cyc_Tcutil_copy_type_pair,qb);
struct Cyc_List_List*atts2=atts;
struct Cyc_Absyn_Exp*req2=Cyc_Tcutil_deep_copy_exp_opt(1,req);
struct Cyc_AssnDef_ExistAssnFn*req_rlns2=req_rlns;
struct Cyc_Absyn_Exp*ens2=Cyc_Tcutil_deep_copy_exp_opt(1,ens);
struct Cyc_AssnDef_ExistAssnFn*ens_rlns2=ens_rlns;
struct Cyc_Absyn_Exp*thrws2=Cyc_Tcutil_deep_copy_exp_opt(1,thrws);
struct Cyc_AssnDef_ExistAssnFn*thrws_rlns2=thrws_rlns;
# 402
struct Cyc_List_List*argvds2=({struct Cyc_List_List*(*_Tmp15)(struct Cyc_Absyn_Vardecl*(*)(struct Cyc_Absyn_Vardecl*),struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Vardecl*(*)(struct Cyc_Absyn_Vardecl*),struct Cyc_List_List*))Cyc_List_map;_Tmp15;})(Cyc_Tcutil_copy_vardecl_opt,argvds);
struct Cyc_Absyn_Vardecl*ret_val2=Cyc_Tcutil_copy_vardecl_opt(ret_val);
return(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_Tmp15=_cycalloc(sizeof(struct Cyc_Absyn_FnType_Absyn_Type_struct));_Tmp15->tag=6,_Tmp15->f1.tvars=tvs2,_Tmp15->f1.effect=effopt2,_Tmp15->f1.ret_tqual=rt_tq,_Tmp15->f1.ret_type=rt2,_Tmp15->f1.args=args2,_Tmp15->f1.c_varargs=c_varargs2,_Tmp15->f1.cyc_varargs=cyc_varargs2,_Tmp15->f1.qual_bnd=qb2,_Tmp15->f1.attributes=atts2,_Tmp15->f1.requires_clause=req2,_Tmp15->f1.requires_assn=req_rlns2,_Tmp15->f1.ensures_clause=ens2,_Tmp15->f1.ensures_assn=ens_rlns2,_Tmp15->f1.throws_clause=thrws2,_Tmp15->f1.throws_assn=thrws_rlns2,_Tmp15->f1.return_value=ret_val2,_Tmp15->f1.arg_vardecls=argvds2,_Tmp15->f1.effconstr=effc2;_Tmp15;});}}case 7: _Tmp1=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_Tmp0)->f2;_Tmp14=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_Tmp0)->f3;{enum Cyc_Absyn_AggrKind k=_Tmp1;int b=_TmpA;struct Cyc_List_List*fs=_Tmp14;
# 408
return(void*)({struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_Tmp15=_cycalloc(sizeof(struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct));_Tmp15->tag=7,_Tmp15->f1=k,_Tmp15->f2=b,({struct Cyc_List_List*_Tmp16=({struct Cyc_List_List*(*_Tmp17)(struct Cyc_Absyn_Aggrfield*(*)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Aggrfield*(*)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*))Cyc_List_map;_Tmp17;})(Cyc_Tcutil_copy_field,fs);_Tmp15->f3=_Tmp16;});_Tmp15;});}case 9: _Tmp14=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e=_Tmp14;
return(void*)({struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_Tmp15=_cycalloc(sizeof(struct Cyc_Absyn_ValueofType_Absyn_Type_struct));_Tmp15->tag=9,_Tmp15->f1=e;_Tmp15;});}case 11: _Tmp14=((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e=_Tmp14;
return(void*)({struct Cyc_Absyn_TypeofType_Absyn_Type_struct*_Tmp15=_cycalloc(sizeof(struct Cyc_Absyn_TypeofType_Absyn_Type_struct));_Tmp15->tag=11,_Tmp15->f1=e;_Tmp15;});}case 8: _Tmp14=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_Tmp0)->f1;_Tmp13=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_Tmp0)->f2;_Tmp11=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_Tmp0)->f3;{struct _tuple1*tdn=_Tmp14;struct Cyc_List_List*ts=_Tmp13;struct Cyc_Absyn_Typedefdecl*td=_Tmp11;
# 412
return(void*)({struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_Tmp15=_cycalloc(sizeof(struct Cyc_Absyn_TypedefType_Absyn_Type_struct));_Tmp15->tag=8,_Tmp15->f1=tdn,({struct Cyc_List_List*_Tmp16=Cyc_List_map(Cyc_Tcutil_copy_type,ts);_Tmp15->f2=_Tmp16;}),_Tmp15->f3=td,_Tmp15->f4=0;_Tmp15;});}default: switch(*((int*)((struct Cyc_Absyn_TypeDecl*)((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_Tmp0)->f1)->r)){case 0: _Tmp14=((struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct*)((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_Tmp0)->f1->r)->f1;{struct Cyc_Absyn_Aggrdecl*ad=_Tmp14;
# 415
struct Cyc_List_List*targs=({struct Cyc_List_List*(*_Tmp15)(void*(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*)=(struct Cyc_List_List*(*)(void*(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*))Cyc_List_map;_Tmp15;})(Cyc_Tcutil_tvar2type,ad->tvs);
union Cyc_Absyn_AggrInfo _Tmp15=Cyc_Absyn_UnknownAggr(ad->kind,ad->name,0);return Cyc_Absyn_aggr_type(_Tmp15,targs);}case 1: _Tmp14=((struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct*)((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_Tmp0)->f1->r)->f1;{struct Cyc_Absyn_Enumdecl*ed=_Tmp14;
# 418
return Cyc_Absyn_enum_type(ed->name,0);}default: _Tmp14=((struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct*)((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_Tmp0)->f1->r)->f1;{struct Cyc_Absyn_Datatypedecl*dd=_Tmp14;
# 420
struct Cyc_List_List*targs=({struct Cyc_List_List*(*_Tmp15)(void*(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*)=(struct Cyc_List_List*(*)(void*(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*))Cyc_List_map;_Tmp15;})(Cyc_Tcutil_tvar2type,dd->tvs);
union Cyc_Absyn_DatatypeInfo _Tmp15=Cyc_Absyn_UnknownDatatype(({struct Cyc_Absyn_UnknownDatatypeInfo _Tmp16;_Tmp16.name=dd->name,_Tmp16.is_extensible=0;_Tmp16;}));return Cyc_Absyn_datatype_type(_Tmp15,targs);}}};}
# 426
static void*Cyc_Tcutil_copy_designator(int preserve_types,void*d){
void*_Tmp0;if(*((int*)d)==0){_Tmp0=((struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*)d)->f1;{struct Cyc_Absyn_Exp*e=_Tmp0;
return(void*)({struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*_Tmp1=_cycalloc(sizeof(struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct));_Tmp1->tag=0,({struct Cyc_Absyn_Exp*_Tmp2=Cyc_Tcutil_deep_copy_exp(preserve_types,e);_Tmp1->f1=_Tmp2;});_Tmp1;});}}else{
return d;};}struct _tuple16{struct Cyc_List_List*f0;struct Cyc_Absyn_Exp*f1;};
# 432
static struct _tuple16*Cyc_Tcutil_copy_eds(int preserve_types,struct _tuple16*e){
# 434
struct _tuple16*_Tmp0=_cycalloc(sizeof(struct _tuple16));({struct Cyc_List_List*_Tmp1=({struct Cyc_List_List*(*_Tmp2)(void*(*)(int,void*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(void*(*)(int,void*),int,struct Cyc_List_List*))Cyc_List_map_c;_Tmp2;})(Cyc_Tcutil_copy_designator,preserve_types,(*e).f0);_Tmp0->f0=_Tmp1;}),({
struct Cyc_Absyn_Exp*_Tmp1=Cyc_Tcutil_deep_copy_exp(preserve_types,(*e).f1);_Tmp0->f1=_Tmp1;});return _Tmp0;}
# 438
struct Cyc_Absyn_Exp*Cyc_Tcutil_deep_copy_exp(int preserve_types,struct Cyc_Absyn_Exp*e){
struct Cyc_Absyn_Exp*(*DC)(int,struct Cyc_Absyn_Exp*)=Cyc_Tcutil_deep_copy_exp;
struct Cyc_Absyn_Exp*new_e;
int pt=preserve_types;
{void*_Tmp0=e->r;enum Cyc_Absyn_MallocKind _Tmp1;enum Cyc_Absyn_Coercion _Tmp2;int _Tmp3;int _Tmp4;void*_Tmp5;struct Cyc_Absyn_Tqual _Tmp6;void*_Tmp7;int _Tmp8;void*_Tmp9;void*_TmpA;enum Cyc_Absyn_Incrementor _TmpB;enum Cyc_Absyn_Primop _TmpC;struct _fat_ptr _TmpD;void*_TmpE;union Cyc_Absyn_Cnst _TmpF;switch(*((int*)_Tmp0)){case 0: _TmpF=((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{union Cyc_Absyn_Cnst c=_TmpF;
new_e=Cyc_Absyn_const_exp(c,e->loc);goto _LL0;}case 1: _TmpE=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{void*b=_TmpE;
new_e=Cyc_Absyn_varb_exp(b,e->loc);goto _LL0;}case 2: _TmpD=((struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct _fat_ptr p=_TmpD;
new_e=Cyc_Absyn_pragma_exp(p,e->loc);goto _LL0;}case 3: _TmpC=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpE=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{enum Cyc_Absyn_Primop p=_TmpC;struct Cyc_List_List*es=_TmpE;
new_e=({enum Cyc_Absyn_Primop _Tmp10=p;struct Cyc_List_List*_Tmp11=({struct Cyc_List_List*(*_Tmp12)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*))Cyc_List_map_c;_Tmp12;})(DC,pt,es);Cyc_Absyn_primop_exp(_Tmp10,_Tmp11,e->loc);});goto _LL0;}case 5: _TmpE=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpB=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_Absyn_Exp*e1=_TmpE;enum Cyc_Absyn_Incrementor i=_TmpB;
new_e=({struct Cyc_Absyn_Exp*_Tmp10=DC(pt,e1);enum Cyc_Absyn_Incrementor _Tmp11=i;Cyc_Absyn_increment_exp(_Tmp10,_Tmp11,e->loc);});goto _LL0;}case 7: _TmpE=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_Absyn_Exp*e1=_TmpE;struct Cyc_Absyn_Exp*e2=_TmpA;
new_e=({struct Cyc_Absyn_Exp*_Tmp10=DC(pt,e1);struct Cyc_Absyn_Exp*_Tmp11=DC(pt,e2);Cyc_Absyn_and_exp(_Tmp10,_Tmp11,e->loc);});goto _LL0;}case 8: _TmpE=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_Absyn_Exp*e1=_TmpE;struct Cyc_Absyn_Exp*e2=_TmpA;
new_e=({struct Cyc_Absyn_Exp*_Tmp10=DC(pt,e1);struct Cyc_Absyn_Exp*_Tmp11=DC(pt,e2);Cyc_Absyn_or_exp(_Tmp10,_Tmp11,e->loc);});goto _LL0;}case 9: _TmpE=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_Absyn_Exp*e1=_TmpE;struct Cyc_Absyn_Exp*e2=_TmpA;
new_e=({struct Cyc_Absyn_Exp*_Tmp10=DC(pt,e1);struct Cyc_Absyn_Exp*_Tmp11=DC(pt,e2);Cyc_Absyn_seq_exp(_Tmp10,_Tmp11,e->loc);});goto _LL0;}case 6: _TmpE=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_Tmp9=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;{struct Cyc_Absyn_Exp*e1=_TmpE;struct Cyc_Absyn_Exp*e2=_TmpA;struct Cyc_Absyn_Exp*e3=_Tmp9;
# 452
new_e=({struct Cyc_Absyn_Exp*_Tmp10=DC(pt,e1);struct Cyc_Absyn_Exp*_Tmp11=DC(pt,e2);struct Cyc_Absyn_Exp*_Tmp12=DC(pt,e3);Cyc_Absyn_conditional_exp(_Tmp10,_Tmp11,_Tmp12,e->loc);});goto _LL0;}case 4: _TmpE=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_Tmp9=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;{struct Cyc_Absyn_Exp*e1=_TmpE;struct Cyc_Core_Opt*po=_TmpA;struct Cyc_Absyn_Exp*e2=_Tmp9;
# 454
new_e=({struct Cyc_Absyn_Exp*_Tmp10=DC(pt,e1);struct Cyc_Core_Opt*_Tmp11=(unsigned)po?({struct Cyc_Core_Opt*_Tmp12=_cycalloc(sizeof(struct Cyc_Core_Opt));_Tmp12->v=(void*)po->v;_Tmp12;}): 0;struct Cyc_Absyn_Exp*_Tmp12=DC(pt,e2);Cyc_Absyn_assignop_exp(_Tmp10,_Tmp11,_Tmp12,e->loc);});
goto _LL0;}case 10: if(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_Tmp0)->f3!=0){_TmpE=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_Tmp8=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_Tmp0)->f3->num_varargs;_Tmp9=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_Tmp0)->f3->injectors;_Tmp7=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_Tmp0)->f3->vai->name;_Tmp6=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_Tmp0)->f3->vai->tq;_Tmp5=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_Tmp0)->f3->vai->type;_Tmp4=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_Tmp0)->f3->vai->inject;_Tmp3=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_Tmp0)->f4;{struct Cyc_Absyn_Exp*e1=_TmpE;struct Cyc_List_List*es=_TmpA;int n=_Tmp8;struct Cyc_List_List*is=_Tmp9;struct _fat_ptr*nm=_Tmp7;struct Cyc_Absyn_Tqual tq=_Tmp6;void*t=_Tmp5;int i=_Tmp4;int resolved=_Tmp3;
# 457
new_e=({void*_Tmp10=(void*)({struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_Tmp11=_cycalloc(sizeof(struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct));_Tmp11->tag=10,({
struct Cyc_Absyn_Exp*_Tmp12=DC(pt,e1);_Tmp11->f1=_Tmp12;}),({struct Cyc_List_List*_Tmp12=({struct Cyc_List_List*(*_Tmp13)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*))Cyc_List_map_c;_Tmp13;})(DC,pt,es);_Tmp11->f2=_Tmp12;}),({
struct Cyc_Absyn_VarargCallInfo*_Tmp12=({struct Cyc_Absyn_VarargCallInfo*_Tmp13=_cycalloc(sizeof(struct Cyc_Absyn_VarargCallInfo));_Tmp13->num_varargs=n,_Tmp13->injectors=is,({
struct Cyc_Absyn_VarargInfo*_Tmp14=({struct Cyc_Absyn_VarargInfo*_Tmp15=_cycalloc(sizeof(struct Cyc_Absyn_VarargInfo));_Tmp15->name=nm,_Tmp15->tq=tq,({void*_Tmp16=Cyc_Tcutil_copy_type(t);_Tmp15->type=_Tmp16;}),_Tmp15->inject=i;_Tmp15;});_Tmp13->vai=_Tmp14;});_Tmp13;});
# 459
_Tmp11->f3=_Tmp12;}),_Tmp11->f4=resolved;_Tmp11;});
# 457
Cyc_Absyn_new_exp(_Tmp10,e->loc);});
# 462
goto _LL0;}}else{_TmpE=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_Tmp9=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;_Tmp8=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_Tmp0)->f4;{struct Cyc_Absyn_Exp*e1=_TmpE;struct Cyc_List_List*es=_TmpA;struct Cyc_Absyn_VarargCallInfo*vci=_Tmp9;int resolved=_Tmp8;
# 464
new_e=({void*_Tmp10=(void*)({struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_Tmp11=_cycalloc(sizeof(struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct));_Tmp11->tag=10,({struct Cyc_Absyn_Exp*_Tmp12=DC(pt,e1);_Tmp11->f1=_Tmp12;}),({struct Cyc_List_List*_Tmp12=({struct Cyc_List_List*(*_Tmp13)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*))Cyc_List_map_c;_Tmp13;})(DC,pt,es);_Tmp11->f2=_Tmp12;}),_Tmp11->f3=vci,_Tmp11->f4=resolved;_Tmp11;});Cyc_Absyn_new_exp(_Tmp10,e->loc);});
goto _LL0;}}case 11: _TmpE=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp8=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_Absyn_Exp*e1=_TmpE;int b=_Tmp8;
# 467
new_e=b?({struct Cyc_Absyn_Exp*_Tmp10=DC(pt,e1);Cyc_Absyn_rethrow_exp(_Tmp10,e->loc);}):({struct Cyc_Absyn_Exp*_Tmp10=DC(pt,e1);Cyc_Absyn_throw_exp(_Tmp10,e->loc);});
goto _LL0;}case 12: _TmpE=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e1=_TmpE;
# 470
new_e=({struct Cyc_Absyn_Exp*_Tmp10=DC(pt,e1);Cyc_Absyn_noinstantiate_exp(_Tmp10,e->loc);});goto _LL0;}case 13: _TmpE=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_Absyn_Exp*e1=_TmpE;struct Cyc_List_List*ts=_TmpA;
# 472
new_e=({struct Cyc_Absyn_Exp*_Tmp10=DC(pt,e1);struct Cyc_List_List*_Tmp11=Cyc_List_map(Cyc_Tcutil_copy_type,ts);Cyc_Absyn_instantiate_exp(_Tmp10,_Tmp11,e->loc);});goto _LL0;}case 14: _TmpE=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_Tmp8=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;_Tmp2=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_Tmp0)->f4;{void*t=_TmpE;struct Cyc_Absyn_Exp*e1=_TmpA;int b=_Tmp8;enum Cyc_Absyn_Coercion c=_Tmp2;
# 474
new_e=({void*_Tmp10=Cyc_Tcutil_copy_type(t);struct Cyc_Absyn_Exp*_Tmp11=DC(pt,e1);int _Tmp12=b;enum Cyc_Absyn_Coercion _Tmp13=c;Cyc_Absyn_cast_exp(_Tmp10,_Tmp11,_Tmp12,_Tmp13,e->loc);});goto _LL0;}case 16: _TmpE=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_Tmp9=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;{struct Cyc_Absyn_Exp*eo=_TmpE;struct Cyc_Absyn_Exp*e1=_TmpA;struct Cyc_Absyn_Exp*qo=_Tmp9;
# 476
new_e=({struct Cyc_Absyn_Exp*_Tmp10=(unsigned)eo?DC(pt,eo): 0;struct Cyc_Absyn_Exp*_Tmp11=DC(pt,e1);struct Cyc_Absyn_Exp*_Tmp12=(unsigned)qo?DC(pt,qo): 0;Cyc_Absyn_New_exp(_Tmp10,_Tmp11,_Tmp12,e->loc);});goto _LL0;}case 15: _TmpE=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e1=_TmpE;
new_e=({struct Cyc_Absyn_Exp*_Tmp10=DC(pt,e1);Cyc_Absyn_address_exp(_Tmp10,e->loc);});goto _LL0;}case 17: _TmpE=(void*)((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{void*t=_TmpE;
new_e=({void*_Tmp10=Cyc_Tcutil_copy_type(t);Cyc_Absyn_sizeoftype_exp(_Tmp10,e->loc);});goto _LL0;}case 18: _TmpE=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e1=_TmpE;
new_e=({struct Cyc_Absyn_Exp*_Tmp10=DC(pt,e1);Cyc_Absyn_sizeofexp_exp(_Tmp10,e->loc);});goto _LL0;}case 19: _TmpE=(void*)((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{void*t=_TmpE;struct Cyc_List_List*ofs=_TmpA;
new_e=({void*_Tmp10=Cyc_Tcutil_copy_type(t);struct Cyc_List_List*_Tmp11=ofs;Cyc_Absyn_offsetof_exp(_Tmp10,_Tmp11,e->loc);});goto _LL0;}case 20: _TmpE=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e1=_TmpE;
new_e=({struct Cyc_Absyn_Exp*_Tmp10=DC(pt,e1);Cyc_Absyn_deref_exp(_Tmp10,e->loc);});goto _LL0;}case 40: _TmpE=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e1=_TmpE;
new_e=({struct Cyc_Absyn_Exp*_Tmp10=DC(pt,e1);Cyc_Absyn_extension_exp(_Tmp10,e->loc);});goto _LL0;}case 21: _TmpE=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_Tmp8=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;_Tmp4=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_Tmp0)->f4;{struct Cyc_Absyn_Exp*e1=_TmpE;struct _fat_ptr*n=_TmpA;int f1=_Tmp8;int f2=_Tmp4;
# 484
new_e=({void*_Tmp10=(void*)({struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_Tmp11=_cycalloc(sizeof(struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct));_Tmp11->tag=21,({struct Cyc_Absyn_Exp*_Tmp12=DC(pt,e1);_Tmp11->f1=_Tmp12;}),_Tmp11->f2=n,_Tmp11->f3=f1,_Tmp11->f4=f2;_Tmp11;});Cyc_Absyn_new_exp(_Tmp10,e->loc);});goto _LL0;}case 22: _TmpE=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_Tmp8=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;_Tmp4=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_Tmp0)->f4;{struct Cyc_Absyn_Exp*e1=_TmpE;struct _fat_ptr*n=_TmpA;int f1=_Tmp8;int f2=_Tmp4;
# 486
new_e=({void*_Tmp10=(void*)({struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_Tmp11=_cycalloc(sizeof(struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct));_Tmp11->tag=22,({struct Cyc_Absyn_Exp*_Tmp12=DC(pt,e1);_Tmp11->f1=_Tmp12;}),_Tmp11->f2=n,_Tmp11->f3=f1,_Tmp11->f4=f2;_Tmp11;});Cyc_Absyn_new_exp(_Tmp10,e->loc);});goto _LL0;}case 23: _TmpE=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_Absyn_Exp*e1=_TmpE;struct Cyc_Absyn_Exp*e2=_TmpA;
# 488
new_e=({struct Cyc_Absyn_Exp*_Tmp10=DC(pt,e1);struct Cyc_Absyn_Exp*_Tmp11=DC(pt,e2);Cyc_Absyn_subscript_exp(_Tmp10,_Tmp11,e->loc);});goto _LL0;}case 25: _TmpE=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_List_List*eds=_TmpE;
# 490
new_e=({void*_Tmp10=(void*)({struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_Tmp11=_cycalloc(sizeof(struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct));_Tmp11->tag=25,({struct Cyc_List_List*_Tmp12=({struct Cyc_List_List*(*_Tmp13)(struct _tuple16*(*)(int,struct _tuple16*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct _tuple16*(*)(int,struct _tuple16*),int,struct Cyc_List_List*))Cyc_List_map_c;_Tmp13;})(Cyc_Tcutil_copy_eds,pt,eds);_Tmp11->f1=_Tmp12;});_Tmp11;});Cyc_Absyn_new_exp(_Tmp10,e->loc);});goto _LL0;}case 30: _TmpE=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_Tmp9=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;{struct Cyc_List_List*es=_TmpE;struct Cyc_Absyn_Datatypedecl*dtd=_TmpA;struct Cyc_Absyn_Datatypefield*dtf=_Tmp9;
# 492
new_e=({void*_Tmp10=(void*)({struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*_Tmp11=_cycalloc(sizeof(struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct));_Tmp11->tag=30,({struct Cyc_List_List*_Tmp12=({struct Cyc_List_List*(*_Tmp13)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*))Cyc_List_map_c;_Tmp13;})(DC,pt,es);_Tmp11->f1=_Tmp12;}),_Tmp11->f2=dtd,_Tmp11->f3=dtf;_Tmp11;});Cyc_Absyn_new_exp(_Tmp10,e->loc);});goto _LL0;}case 26: _TmpE=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_Tmp9=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;_Tmp8=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_Tmp0)->f4;{struct Cyc_Absyn_Vardecl*vd=_TmpE;struct Cyc_Absyn_Exp*e1=_TmpA;struct Cyc_Absyn_Exp*e2=_Tmp9;int b=_Tmp8;
# 494
new_e=({void*_Tmp10=(void*)({struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*_Tmp11=_cycalloc(sizeof(struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct));_Tmp11->tag=26,_Tmp11->f1=vd,({struct Cyc_Absyn_Exp*_Tmp12=DC(pt,e1);_Tmp11->f2=_Tmp12;}),({struct Cyc_Absyn_Exp*_Tmp12=DC(pt,e2);_Tmp11->f3=_Tmp12;}),_Tmp11->f4=b;_Tmp11;});Cyc_Absyn_new_exp(_Tmp10,e->loc);});goto _LL0;}case 27: _TmpE=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=(void*)((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_Tmp8=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;{struct Cyc_Absyn_Exp*e=_TmpE;void*t=_TmpA;int b=_Tmp8;
# 496
new_e=({void*_Tmp10=(void*)({struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*_Tmp11=_cycalloc(sizeof(struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct));_Tmp11->tag=27,({struct Cyc_Absyn_Exp*_Tmp12=DC(pt,e);_Tmp11->f1=_Tmp12;}),({void*_Tmp12=Cyc_Tcutil_copy_type(t);_Tmp11->f2=_Tmp12;}),_Tmp11->f3=b;_Tmp11;});Cyc_Absyn_new_exp(_Tmp10,e->loc);});
goto _LL0;}case 28: _TmpE=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_Tmp9=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;_Tmp7=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_Tmp0)->f4;{struct _tuple1*n=_TmpE;struct Cyc_List_List*ts=_TmpA;struct Cyc_List_List*eds=_Tmp9;struct Cyc_Absyn_Aggrdecl*agr=_Tmp7;
# 499
new_e=({void*_Tmp10=(void*)({struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_Tmp11=_cycalloc(sizeof(struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct));_Tmp11->tag=28,_Tmp11->f1=n,({struct Cyc_List_List*_Tmp12=Cyc_List_map(Cyc_Tcutil_copy_type,ts);_Tmp11->f2=_Tmp12;}),({struct Cyc_List_List*_Tmp12=({struct Cyc_List_List*(*_Tmp13)(struct _tuple16*(*)(int,struct _tuple16*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct _tuple16*(*)(int,struct _tuple16*),int,struct Cyc_List_List*))Cyc_List_map_c;_Tmp13;})(Cyc_Tcutil_copy_eds,pt,eds);_Tmp11->f3=_Tmp12;}),_Tmp11->f4=agr;_Tmp11;});Cyc_Absyn_new_exp(_Tmp10,e->loc);});
# 501
goto _LL0;}case 29: _TmpE=(void*)((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp8=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_TmpA=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;{void*topt=_TmpE;int b=_Tmp8;struct Cyc_List_List*eds=_TmpA;
# 503
void*new_topt=0;
if(topt!=0)
new_topt=Cyc_Tcutil_copy_type(topt);
# 507
new_e=({void*_Tmp10=(void*)({struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*_Tmp11=_cycalloc(sizeof(struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct));_Tmp11->tag=29,_Tmp11->f1=new_topt,_Tmp11->f2=b,({struct Cyc_List_List*_Tmp12=({struct Cyc_List_List*(*_Tmp13)(struct _tuple16*(*)(int,struct _tuple16*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct _tuple16*(*)(int,struct _tuple16*),int,struct Cyc_List_List*))Cyc_List_map_c;_Tmp13;})(Cyc_Tcutil_copy_eds,pt,eds);_Tmp11->f3=_Tmp12;});_Tmp11;});Cyc_Absyn_new_exp(_Tmp10,e->loc);});
goto _LL0;}case 24: _TmpE=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_Tmp0)->f1->f0;_Tmp6=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_Tmp0)->f1->f1;_TmpA=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_Tmp0)->f1->f2;_Tmp9=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct _fat_ptr*vopt=_TmpE;struct Cyc_Absyn_Tqual tq=_Tmp6;void*t=_TmpA;struct Cyc_List_List*eds=_Tmp9;
# 510
new_e=({void*_Tmp10=(void*)({struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*_Tmp11=_cycalloc(sizeof(struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct));_Tmp11->tag=24,({struct _tuple9*_Tmp12=({struct _tuple9*_Tmp13=_cycalloc(sizeof(struct _tuple9));_Tmp13->f0=vopt,_Tmp13->f1=tq,({void*_Tmp14=Cyc_Tcutil_copy_type(t);_Tmp13->f2=_Tmp14;});_Tmp13;});_Tmp11->f1=_Tmp12;}),({
struct Cyc_List_List*_Tmp12=({struct Cyc_List_List*(*_Tmp13)(struct _tuple16*(*)(int,struct _tuple16*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct _tuple16*(*)(int,struct _tuple16*),int,struct Cyc_List_List*))Cyc_List_map_c;_Tmp13;})(Cyc_Tcutil_copy_eds,pt,eds);_Tmp11->f2=_Tmp12;});_Tmp11;});
# 510
Cyc_Absyn_new_exp(_Tmp10,e->loc);});
# 512
goto _LL0;}case 31: _TmpE=((struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_Absyn_Enumdecl*ed=_TmpE;struct Cyc_Absyn_Enumfield*ef=_TmpA;
new_e=e;goto _LL0;}case 32: _TmpE=(void*)((struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{void*t=_TmpE;struct Cyc_Absyn_Enumfield*ef=_TmpA;
# 515
new_e=({void*_Tmp10=(void*)({struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*_Tmp11=_cycalloc(sizeof(struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct));_Tmp11->tag=32,({void*_Tmp12=Cyc_Tcutil_copy_type(t);_Tmp11->f1=_Tmp12;}),_Tmp11->f2=ef;_Tmp11;});Cyc_Absyn_new_exp(_Tmp10,e->loc);});goto _LL0;}case 33: _Tmp1=((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_Tmp0)->f1.mknd;_TmpE=((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_Tmp0)->f1.rgn;_TmpA=((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_Tmp0)->f1.aqual;_Tmp9=((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_Tmp0)->f1.elt_type;_Tmp7=((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_Tmp0)->f1.num_elts;_Tmp8=((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_Tmp0)->f1.fat_result;_Tmp4=((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_Tmp0)->f1.inline_call;{enum Cyc_Absyn_MallocKind mk=_Tmp1;struct Cyc_Absyn_Exp*r=_TmpE;struct Cyc_Absyn_Exp*q=_TmpA;void**t=_Tmp9;struct Cyc_Absyn_Exp*n=_Tmp7;int res=_Tmp8;int inlc=_Tmp4;
# 517
struct Cyc_Absyn_Exp*e2=Cyc_Absyn_copy_exp(e);
struct Cyc_Absyn_Exp*r1=r;if(r!=0)r1=DC(pt,r);{
struct Cyc_Absyn_Exp*q1=q;if(q!=0)q1=DC(pt,q);{
void**t1=t;if(t!=0)t1=({void**_Tmp10=_cycalloc(sizeof(void*));({void*_Tmp11=Cyc_Tcutil_copy_type(*t);*_Tmp10=_Tmp11;});_Tmp10;});
({void*_Tmp10=(void*)({struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*_Tmp11=_cycalloc(sizeof(struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct));_Tmp11->tag=33,_Tmp11->f1.mknd=mk,_Tmp11->f1.rgn=r1,_Tmp11->f1.aqual=q1,_Tmp11->f1.elt_type=t1,_Tmp11->f1.num_elts=n,_Tmp11->f1.fat_result=res,_Tmp11->f1.inline_call=inlc;_Tmp11;});e2->r=_Tmp10;});
new_e=e2;
goto _LL0;}}}case 34: _TmpE=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_Absyn_Exp*e1=_TmpE;struct Cyc_Absyn_Exp*e2=_TmpA;
new_e=({struct Cyc_Absyn_Exp*_Tmp10=DC(pt,e1);struct Cyc_Absyn_Exp*_Tmp11=DC(pt,e2);Cyc_Absyn_swap_exp(_Tmp10,_Tmp11,e->loc);});goto _LL0;}case 35: _TmpE=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_Core_Opt*nopt=_TmpE;struct Cyc_List_List*eds=_TmpA;
# 526
struct Cyc_Core_Opt*nopt1=nopt;
if(nopt!=0)nopt1=({struct Cyc_Core_Opt*_Tmp10=_cycalloc(sizeof(struct Cyc_Core_Opt));_Tmp10->v=(struct _tuple1*)nopt->v;_Tmp10;});
new_e=({void*_Tmp10=(void*)({struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_Tmp11=_cycalloc(sizeof(struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct));_Tmp11->tag=35,_Tmp11->f1=nopt1,({struct Cyc_List_List*_Tmp12=({struct Cyc_List_List*(*_Tmp13)(struct _tuple16*(*)(int,struct _tuple16*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct _tuple16*(*)(int,struct _tuple16*),int,struct Cyc_List_List*))Cyc_List_map_c;_Tmp13;})(Cyc_Tcutil_copy_eds,pt,eds);_Tmp11->f2=_Tmp12;});_Tmp11;});Cyc_Absyn_new_exp(_Tmp10,e->loc);});
goto _LL0;}case 36:
# 531
 _throw((void*)({struct Cyc_Core_Failure_exn_struct*_Tmp10=_cycalloc(sizeof(struct Cyc_Core_Failure_exn_struct));_Tmp10->tag=Cyc_Core_Failure,_Tmp10->f1=_tag_fat("deep_copy: statement expressions unsupported",sizeof(char),45U);_Tmp10;}));case 37: _TmpE=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpA=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_Absyn_Exp*e1=_TmpE;struct _fat_ptr*fn=_TmpA;
# 533
new_e=({void*_Tmp10=(void*)({struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*_Tmp11=_cycalloc(sizeof(struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct));_Tmp11->tag=37,({struct Cyc_Absyn_Exp*_Tmp12=DC(pt,e1);_Tmp11->f1=_Tmp12;}),_Tmp11->f2=fn;_Tmp11;});Cyc_Absyn_new_exp(_Tmp10,e->loc);});goto _LL0;}case 38: _TmpE=(void*)((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{void*t=_TmpE;
new_e=({void*_Tmp10=Cyc_Tcutil_copy_type(t);Cyc_Absyn_valueof_exp(_Tmp10,e->loc);});goto _LL0;}case 39: _Tmp8=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_TmpD=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_TmpE=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;_TmpA=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_Tmp0)->f4;_Tmp9=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_Tmp0)->f5;{int v=_Tmp8;struct _fat_ptr t=_TmpD;struct Cyc_List_List*o=_TmpE;struct Cyc_List_List*i=_TmpA;struct Cyc_List_List*c=_Tmp9;
new_e=Cyc_Absyn_asm_exp(v,t,o,i,c,e->loc);goto _LL0;}case 41: _TmpE=((struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e1=_TmpE;
new_e=Cyc_Absyn_assert_exp(e1,e->loc);goto _LL0;}default: _TmpE=((struct Cyc_Absyn_Assert_false_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e1=_TmpE;
new_e=Cyc_Absyn_assert_false_exp(e1,e->loc);goto _LL0;}}_LL0:;}
# 540
if(preserve_types){
new_e->topt=e->topt;
new_e->annot=e->annot;}
# 544
return new_e;}struct _tuple17{struct Cyc_Absyn_Tvar*f0;void*f1;};
# 549
static struct Cyc_Absyn_Kind*Cyc_Tcutil_field_kind(void*field_type,struct Cyc_List_List*ts,struct Cyc_List_List*tvs){
# 551
struct Cyc_Absyn_Kind*k=Cyc_Tcutil_type_kind(field_type);
if(ts!=0 && k==& Cyc_Kinds_ak){
# 555
struct _RegionHandle _Tmp0=_new_region(0U,"r");struct _RegionHandle*r=& _Tmp0;_push_region(r);
{struct Cyc_List_List*inst=0;
# 558
for(1;tvs!=0;(tvs=tvs->tl,ts=ts->tl)){
struct Cyc_Absyn_Tvar*tv=(struct Cyc_Absyn_Tvar*)tvs->hd;
void*t=(void*)ts->hd;
enum Cyc_Absyn_KindQual _Tmp1=Cyc_Kinds_tvar_kind(tv,& Cyc_Kinds_bk)->kind;switch((int)_Tmp1){case Cyc_Absyn_IntKind:
 goto _LL4;case Cyc_Absyn_AnyKind: _LL4:
 inst=({struct Cyc_List_List*_Tmp2=_region_malloc(r,0U,sizeof(struct Cyc_List_List));({struct _tuple17*_Tmp3=({struct _tuple17*_Tmp4=_region_malloc(r,0U,sizeof(struct _tuple17));_Tmp4->f0=tv,_Tmp4->f1=t;_Tmp4;});_Tmp2->hd=_Tmp3;}),_Tmp2->tl=inst;_Tmp2;});goto _LL0;default:
 goto _LL0;}_LL0:;}
# 567
if(inst!=0){
field_type=({struct _RegionHandle*_Tmp1=r;struct Cyc_List_List*_Tmp2=Cyc_List_imp_rev(inst);Cyc_Tcutil_rsubstitute(_Tmp1,_Tmp2,field_type);});
k=Cyc_Tcutil_type_kind(field_type);}}
# 556
;_pop_region();}
# 572
return k;}
# 579
struct Cyc_Absyn_Kind*Cyc_Tcutil_type_kind(void*t){
# 581
void*_Tmp0=Cyc_Absyn_compress(t);struct Cyc_Absyn_PtrInfo _Tmp1;void*_Tmp2;void*_Tmp3;switch(*((int*)_Tmp0)){case 3: _Tmp3=((struct Cyc_Absyn_Cvar_Absyn_Type_struct*)_Tmp0)->f1;{struct Cyc_Core_Opt*k=_Tmp3;
_Tmp3=k;goto _LL4;}case 1: _Tmp3=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_Tmp0)->f1;_LL4: {struct Cyc_Core_Opt*k=_Tmp3;
return(struct Cyc_Absyn_Kind*)_check_null(k)->v;}case 2: _Tmp3=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Tvar*tv=_Tmp3;
return Cyc_Kinds_tvar_kind(tv,& Cyc_Kinds_bk);}case 0: _Tmp3=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1;_Tmp2=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f2;{void*c=_Tmp3;struct Cyc_List_List*ts=_Tmp2;
# 586
int _Tmp4;void*_Tmp5;enum Cyc_Absyn_AggrKind _Tmp6;void*_Tmp7;enum Cyc_Absyn_Size_of _Tmp8;switch(*((int*)c)){case 0:
 return& Cyc_Kinds_mk;case 1: _Tmp8=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)c)->f2;{enum Cyc_Absyn_Size_of sz=_Tmp8;
# 589
if((int)sz==2 ||(int)sz==3)return& Cyc_Kinds_bk;else{return& Cyc_Kinds_mk;}}case 3:
 goto _LL25;case 2: _LL25:
 return& Cyc_Kinds_mk;case 19:
 goto _LL29;case 20: _LL29:
 goto _LL2B;case 4: _LL2B:
 return& Cyc_Kinds_bk;case 18:
 return& Cyc_Kinds_bk;case 7:
 goto _LL31;case 8: _LL31:
 goto _LL33;case 6: _LL33:
 return& Cyc_Kinds_ek;case 21: _Tmp7=((struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct*)c)->f2;{struct Cyc_Absyn_Kind*k=_Tmp7;
return k;}case 5:
 return& Cyc_Kinds_bk;case 9:
 goto _LL3B;case 10: _LL3B:
 return& Cyc_Kinds_ek;case 12:
 goto _LL3F;case 11: _LL3F:
 return& Cyc_Kinds_boolk;case 13:
 goto _LL43;case 14: _LL43:
 return& Cyc_Kinds_ptrbk;case 17:
 goto _LL47;case 16: _LL47:
 goto _LL49;case 15: _LL49:
 return& Cyc_Kinds_aqk;case 22:
 return& Cyc_Kinds_ak;case 23: if(((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)c)->f1.KnownDatatypefield.tag==2)
return& Cyc_Kinds_mk;else{
# 613
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp9=({struct Cyc_Warn_String_Warn_Warg_struct _TmpA;_TmpA.tag=0,_TmpA.f1=_tag_fat("type_kind: Unresolved DatatypeFieldType",sizeof(char),40U);_TmpA;});void*_TmpA[1];_TmpA[0]=& _Tmp9;({int(*_TmpB)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_TmpB;})(_tag_fat(_TmpA,sizeof(void*),1));});}default: if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)c)->f1.UnknownAggr.tag==1)
# 616
return& Cyc_Kinds_ak;else{_Tmp6=(*((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)c)->f1.KnownAggr.val)->kind;_Tmp7=(*((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)c)->f1.KnownAggr.val)->tvs;_Tmp5=(*((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)c)->f1.KnownAggr.val)->impl;_Tmp4=(*((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)c)->f1.KnownAggr.val)->expected_mem_kind;{enum Cyc_Absyn_AggrKind strOrU=_Tmp6;struct Cyc_List_List*tvs=_Tmp7;struct Cyc_Absyn_AggrdeclImpl*i=_Tmp5;int expected_mem_kind=_Tmp4;
# 618
if(i==0){
if(expected_mem_kind)return& Cyc_Kinds_mk;else{return& Cyc_Kinds_ak;}}{
struct Cyc_List_List*fields=i->fields;
if(fields==0)return& Cyc_Kinds_mk;
# 623
if((int)strOrU==0){
for(1;_check_null(fields)->tl!=0;fields=fields->tl){
;}{
void*last_type=((struct Cyc_Absyn_Aggrfield*)fields->hd)->type;
struct Cyc_Absyn_Kind*k=Cyc_Tcutil_field_kind(last_type,ts,tvs);
if(k==& Cyc_Kinds_ak)return k;}}else{
# 632
for(1;fields!=0;fields=fields->tl){
void*type=((struct Cyc_Absyn_Aggrfield*)fields->hd)->type;
struct Cyc_Absyn_Kind*k=Cyc_Tcutil_field_kind(type,ts,tvs);
if(k==& Cyc_Kinds_ak)return k;}}
# 637
return& Cyc_Kinds_mk;}}}};}case 6:
# 639
 return& Cyc_Kinds_ak;case 7:
 return& Cyc_Kinds_mk;case 4: _Tmp1=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1;{struct Cyc_Absyn_PtrInfo pinfo=_Tmp1;
# 642
void*_Tmp4=Cyc_Absyn_compress(pinfo.ptr_atts.bounds);if(*((int*)_Tmp4)==0)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp4)->f1)){case 13:
# 644
 return& Cyc_Kinds_bk;case 14:
# 646
 goto _LL5A;default: goto _LL59;}else{_LL59: _LL5A:
# 648
 return& Cyc_Kinds_mk;};}case 9:
# 650
 return& Cyc_Kinds_ik;case 11:
# 654
 return& Cyc_Kinds_ak;case 5: _Tmp3=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.num_elts;{struct Cyc_Absyn_Exp*num_elts=_Tmp3;
# 656
if(num_elts==0 || Cyc_Tcutil_is_const_exp(num_elts))return& Cyc_Kinds_mk;
return& Cyc_Kinds_ak;}case 8: _Tmp3=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_Tmp0)->f3;{struct Cyc_Absyn_Typedefdecl*td=_Tmp3;
# 659
if(td==0 || td->kind==0)
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp4=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp5;_Tmp5.tag=0,_Tmp5.f1=_tag_fat("type_kind: typedef found: ",sizeof(char),27U);_Tmp5;});struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp5=({struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp6;_Tmp6.tag=2,_Tmp6.f1=(void*)t;_Tmp6;});void*_Tmp6[2];_Tmp6[0]=& _Tmp4,_Tmp6[1]=& _Tmp5;({int(*_Tmp7)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_Tmp7;})(_tag_fat(_Tmp6,sizeof(void*),2));});
return(struct Cyc_Absyn_Kind*)td->kind->v;}default: switch(*((int*)((struct Cyc_Absyn_TypeDecl*)((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_Tmp0)->f1)->r)){case 1:
 return& Cyc_Kinds_bk;case 0:
 goto _LL1C;default: _LL1C:
 return& Cyc_Kinds_ak;}};}
# 668
static void*Cyc_Tcutil_rgns_of(void*);
# 670
static void*Cyc_Tcutil_rgns_of_field(struct Cyc_Absyn_Aggrfield*af){
return Cyc_Tcutil_rgns_of(af->type);}
# 674
static struct _tuple17*Cyc_Tcutil_region_free_subst(struct Cyc_Absyn_Tvar*tv){
void*t;
{struct Cyc_Absyn_Kind*_Tmp0=Cyc_Kinds_tvar_kind(tv,& Cyc_Kinds_bk);switch((int)((struct Cyc_Absyn_Kind*)_Tmp0)->kind){case Cyc_Absyn_EffKind:
# 678
 t=Cyc_Absyn_heap_rgn_type;goto _LL0;case Cyc_Absyn_IntKind:
 t=(void*)({struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_Tmp1=_cycalloc(sizeof(struct Cyc_Absyn_ValueofType_Absyn_Type_struct));_Tmp1->tag=9,({struct Cyc_Absyn_Exp*_Tmp2=Cyc_Absyn_uint_exp(0U,0U);_Tmp1->f1=_Tmp2;});_Tmp1;});goto _LL0;case Cyc_Absyn_BoolKind:
 t=Cyc_Absyn_true_type;goto _LL0;case Cyc_Absyn_PtrBndKind:
 t=Cyc_Absyn_fat_bound_type;goto _LL0;default:
 t=Cyc_Absyn_sint_type;goto _LL0;}_LL0:;}{
# 684
struct _tuple17*_Tmp0=_cycalloc(sizeof(struct _tuple17));_Tmp0->f0=tv,_Tmp0->f1=t;return _Tmp0;}}
# 691
static void*Cyc_Tcutil_rgns_of(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);void*_Tmp1;void*_Tmp2;void*_Tmp3;void*_Tmp4;struct Cyc_Absyn_Tqual _Tmp5;void*_Tmp6;void*_Tmp7;switch(*((int*)_Tmp0)){case 0: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f2==0)
return Cyc_Absyn_empty_effect;else{if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1)==9)
return t;else{_Tmp7=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f2;{struct Cyc_List_List*ts=_Tmp7;
return Cyc_Tcutil_normalize_effect(Cyc_Absyn_join_eff(Cyc_List_map(Cyc_Tcutil_rgns_of,ts)));}}}case 3:
 goto _LLA;case 1: _LLA:
 goto _LLC;case 2: _LLC: {
# 699
struct Cyc_Absyn_Kind*_Tmp8=Cyc_Tcutil_type_kind(t);switch((int)((struct Cyc_Absyn_Kind*)_Tmp8)->kind){case Cyc_Absyn_EffKind:
# 701
 return t;case Cyc_Absyn_IntKind:
 return Cyc_Absyn_empty_effect;default:
 return Cyc_Absyn_regionsof_eff(t);};}case 4: _Tmp7=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.elt_type;_Tmp6=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.eff;{void*et=_Tmp7;void*r=_Tmp6;
# 707
return Cyc_Tcutil_normalize_effect(Cyc_Absyn_join_eff(({void*_Tmp8[2];_Tmp8[0]=r,({void*_Tmp9=Cyc_Tcutil_rgns_of(et);_Tmp8[1]=_Tmp9;});Cyc_List_list(_tag_fat(_Tmp8,sizeof(void*),2));})));}case 5: _Tmp7=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.elt_type;{void*et=_Tmp7;
# 709
return Cyc_Tcutil_normalize_effect(Cyc_Tcutil_rgns_of(et));}case 7: _Tmp7=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_Tmp0)->f3;{struct Cyc_List_List*fs=_Tmp7;
# 711
return Cyc_Tcutil_normalize_effect(Cyc_Absyn_join_eff(({struct Cyc_List_List*(*_Tmp8)(void*(*)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*)=(struct Cyc_List_List*(*)(void*(*)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*))Cyc_List_map;_Tmp8;})(Cyc_Tcutil_rgns_of_field,fs)));}case 6: _Tmp7=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.tvars;_Tmp6=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.effect;_Tmp5=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.ret_tqual;_Tmp4=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.ret_type;_Tmp3=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.args;_Tmp2=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.cyc_varargs;_Tmp1=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.qual_bnd;{struct Cyc_List_List*tvs=_Tmp7;void*eff=_Tmp6;struct Cyc_Absyn_Tqual rt_tq=_Tmp5;void*rt=_Tmp4;struct Cyc_List_List*args=_Tmp3;struct Cyc_Absyn_VarargInfo*cyc_varargs=_Tmp2;struct Cyc_List_List*rpo=_Tmp1;
# 720
void*e=({struct Cyc_List_List*_Tmp8=({struct Cyc_List_List*(*_Tmp9)(struct _tuple17*(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct _tuple17*(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*))Cyc_List_map;_Tmp9;})(Cyc_Tcutil_region_free_subst,tvs);Cyc_Tcutil_substitute(_Tmp8,_check_null(eff));});
return Cyc_Tcutil_normalize_effect(e);}case 8: _Tmp7=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_Tmp0)->f2;{struct Cyc_List_List*ts=_Tmp7;
# 723
return Cyc_Tcutil_normalize_effect(Cyc_Absyn_join_eff(Cyc_List_map(Cyc_Tcutil_rgns_of,ts)));}case 10:
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp8=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp9;_Tmp9.tag=0,_Tmp9.f1=_tag_fat("typedecl in rgns_of",sizeof(char),20U);_Tmp9;});void*_Tmp9[1];_Tmp9[0]=& _Tmp8;({int(*_TmpA)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_TmpA;})(_tag_fat(_Tmp9,sizeof(void*),1));});case 9:
 goto _LL1C;default: _LL1C:
 return Cyc_Absyn_empty_effect;};}
# 733
static void*Cyc_Tcutil_normalize_effect_dups(void*e){
e=Cyc_Absyn_compress(e);{
void*_Tmp0;if(*((int*)e)==0)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e)->f1)){case 9: _Tmp0=(struct Cyc_List_List**)&((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e)->f2;{struct Cyc_List_List**es=_Tmp0;
# 737
int redo_join=0;
{struct Cyc_List_List*effs=*es;for(0;effs!=0;effs=effs->tl){
void*eff=(void*)effs->hd;
({void*_Tmp1=(void*)Cyc_Absyn_compress(Cyc_Tcutil_normalize_effect(eff));effs->hd=_Tmp1;});{
void*_Tmp1=(void*)effs->hd;if(*((int*)_Tmp1)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp1)->f1)==9){
# 746
redo_join=1;goto _LL7;}else{goto _LLA;}}else{_LLA:
 goto _LL7;}_LL7:;}}}
# 750
if(!redo_join)return e;{
struct Cyc_List_List*effects=0;
{struct Cyc_List_List*effs=*es;for(0;effs!=0;effs=effs->tl){
void*_Tmp1=Cyc_Absyn_compress((void*)effs->hd);void*_Tmp2;if(*((int*)_Tmp1)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp1)->f1)==9){_Tmp2=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp1)->f2;{struct Cyc_List_List*nested_effs=_Tmp2;
# 755
effects=Cyc_List_revappend(nested_effs,effects);goto _LLC;}}else{goto _LLF;}}else{_LLF: _Tmp2=_Tmp1;{void*e=_Tmp2;
# 760
effects=({struct Cyc_List_List*_Tmp3=_cycalloc(sizeof(struct Cyc_List_List));_Tmp3->hd=e,_Tmp3->tl=effects;_Tmp3;});goto _LLC;}}_LLC:;}}
# 762
({struct Cyc_List_List*_Tmp1=Cyc_List_imp_rev(effects);*es=_Tmp1;});
return e;}}case 10: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e)->f2!=0){_Tmp0=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e)->f2->hd;{void*t=_Tmp0;
# 765
void*_Tmp1=Cyc_Absyn_compress(t);switch(*((int*)_Tmp1)){case 1:
 goto _LL15;case 2: _LL15:
 return e;default:
 return Cyc_Tcutil_rgns_of(t);};}}else{goto _LL5;}default: goto _LL5;}else{_LL5:
# 770
 return e;};}}
# 773
static void Cyc_Tcutil_imp_remove_dups(int(*)(void*,void*),struct Cyc_List_List*);
void*Cyc_Tcutil_normalize_effect(void*e){
e=Cyc_Tcutil_normalize_effect_dups(e);{
void*_Tmp0;if(*((int*)e)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e)->f1)==9){_Tmp0=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e)->f2;{struct Cyc_List_List*l=_Tmp0;
# 778
Cyc_Tcutil_imp_remove_dups(Cyc_Tcutil_typecmp,l);
goto _LL4;}}else{goto _LL3;}}else{_LL3: _LL4:
# 781
 return e;};}}
# 785
static int Cyc_Tcutil_type_equals(void*t1,void*t2){
return Cyc_Tcutil_typecmp(t1,t2)==0;}
# 790
static void*Cyc_Tcutil_dummy_fntype(void*eff){
struct Cyc_Absyn_FnType_Absyn_Type_struct*fntype;fntype=_cycalloc(sizeof(struct Cyc_Absyn_FnType_Absyn_Type_struct)),fntype->tag=6,fntype->f1.tvars=0,fntype->f1.effect=eff,({
struct Cyc_Absyn_Tqual _Tmp0=Cyc_Absyn_empty_tqual(0U);fntype->f1.ret_tqual=_Tmp0;}),fntype->f1.ret_type=Cyc_Absyn_void_type,fntype->f1.args=0,fntype->f1.c_varargs=0,fntype->f1.cyc_varargs=0,fntype->f1.qual_bnd=0,fntype->f1.attributes=0,fntype->f1.requires_clause=0,fntype->f1.requires_assn=0,fntype->f1.ensures_clause=0,fntype->f1.ensures_assn=0,fntype->f1.throws_clause=0,fntype->f1.throws_assn=0,fntype->f1.return_value=0,fntype->f1.arg_vardecls=0,fntype->f1.effconstr=0;{
# 805
void*_Tmp0=(void*)fntype;void*_Tmp1=Cyc_Absyn_heap_rgn_type;void*_Tmp2=Cyc_Absyn_al_qual_type;struct Cyc_Absyn_Tqual _Tmp3=Cyc_Absyn_empty_tqual(0U);void*_Tmp4=Cyc_Absyn_bounds_one();void*_Tmp5=Cyc_Absyn_false_type;return Cyc_Absyn_atb_type(_Tmp0,_Tmp1,_Tmp2,_Tmp3,_Tmp4,_Tmp5,Cyc_Absyn_false_type);}}
# 811
int Cyc_Tcutil_type_in_effect(int may_constrain_evars,void*t,void*e){
t=Cyc_Absyn_compress(t);
e=Cyc_Tcutil_normalize_effect(Cyc_Absyn_compress(e));{
void*_Tmp0;void*_Tmp1;void*_Tmp2;switch(*((int*)e)){case 0: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e)->f1)){case 6:
# 817
 if(may_constrain_evars)return Cyc_Unify_unify(t,Cyc_Absyn_heap_rgn_type);
return Cyc_Tcutil_typecmp(t,Cyc_Absyn_heap_rgn_type)==0;case 9: _Tmp2=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e)->f2;{struct Cyc_List_List*es=_Tmp2;
# 820
for(1;es!=0;es=es->tl){
if(Cyc_Tcutil_type_in_effect(may_constrain_evars,t,(void*)es->hd))
return 1;}
return 0;}case 10: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e)->f2!=0){_Tmp2=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e)->f2->hd;{void*t2=_Tmp2;
# 825
t2=Cyc_Absyn_compress(t2);
if(t==t2)return 1;
if(may_constrain_evars)return Cyc_Unify_unify(t,t2);{
void*_Tmp3=Cyc_Tcutil_rgns_of(t);void*_Tmp4;if(*((int*)_Tmp3)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp3)->f1)==10){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp3)->f2!=0){_Tmp4=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp3)->f2->hd;{void*t3=_Tmp4;
# 830
struct _tuple15 _Tmp5=({struct _tuple15 _Tmp6;({void*_Tmp7=Cyc_Absyn_compress(t3);_Tmp6.f0=_Tmp7;}),_Tmp6.f1=t2;_Tmp6;});void*_Tmp6;void*_Tmp7;if(*((int*)_Tmp5.f0)==2){if(*((int*)_Tmp5.f1)==2){_Tmp7=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_Tmp5.f0)->f1;_Tmp6=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_Tmp5.f1)->f1;{struct Cyc_Absyn_Tvar*tv1=_Tmp7;struct Cyc_Absyn_Tvar*tv2=_Tmp6;
return Cyc_Unify_unify(t,t2);}}else{goto _LL13;}}else{_LL13:
 return t3==t2;};}}else{goto _LLE;}}else{goto _LLE;}}else{_LLE: _Tmp4=_Tmp3;{void*e2=_Tmp4;
# 834
return Cyc_Tcutil_type_in_effect(may_constrain_evars,t,e2);}};}}}else{goto _LL9;}default: goto _LL9;}case 1: _Tmp2=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)e)->f1;_Tmp1=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)e)->f2;_Tmp0=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)e)->f4;{struct Cyc_Core_Opt*k=_Tmp2;void**p=(void**)_Tmp1;struct Cyc_Core_Opt*s=_Tmp0;
# 837
if(k==0 ||(int)((struct Cyc_Absyn_Kind*)k->v)->kind!=3)
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp3=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp4;_Tmp4.tag=0,_Tmp4.f1=_tag_fat("effect evar has wrong kind",sizeof(char),27U);_Tmp4;});void*_Tmp4[1];_Tmp4[0]=& _Tmp3;({int(*_Tmp5)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_Tmp5;})(_tag_fat(_Tmp4,sizeof(void*),1));});
if(!may_constrain_evars)return 0;{
# 842
void*ev=Cyc_Absyn_new_evar(& Cyc_Kinds_eko,s);
# 845
Cyc_Unify_occurs(ev,Cyc_Core_heap_region,(struct Cyc_List_List*)_check_null(s)->v,t);{
void*new_typ=Cyc_Absyn_join_eff(({void*_Tmp3[2];_Tmp3[0]=ev,({void*_Tmp4=Cyc_Absyn_regionsof_eff(t);_Tmp3[1]=_Tmp4;});Cyc_List_list(_tag_fat(_Tmp3,sizeof(void*),2));}));
*p=new_typ;
return 1;}}}default: _LL9:
 return Cyc_Tcutil_typecmp(t,e)==0;};}}
# 855
static int Cyc_Tcutil_variable_in_effect(int may_constrain_evars,struct Cyc_Absyn_Tvar*v,void*e){
e=Cyc_Absyn_compress(e);{
void*_Tmp0;void*_Tmp1;void*_Tmp2;switch(*((int*)e)){case 2: _Tmp2=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)e)->f1;{struct Cyc_Absyn_Tvar*v2=_Tmp2;
return Cyc_Absyn_tvar_cmp(v,v2)==0;}case 0: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e)->f1)){case 9: _Tmp2=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e)->f2;{struct Cyc_List_List*es=_Tmp2;
# 860
for(1;es!=0;es=es->tl){
if(Cyc_Tcutil_variable_in_effect(may_constrain_evars,v,(void*)es->hd))
return 1;}
return 0;}case 10: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e)->f2!=0){_Tmp2=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e)->f2->hd;{void*t=_Tmp2;
# 865
void*_Tmp3=Cyc_Tcutil_rgns_of(t);void*_Tmp4;if(*((int*)_Tmp3)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp3)->f1)==10){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp3)->f2!=0){_Tmp4=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp3)->f2->hd;{void*t2=_Tmp4;
# 867
if(!may_constrain_evars)return 0;{
void*_Tmp5=Cyc_Absyn_compress(t2);void*_Tmp6;void*_Tmp7;void*_Tmp8;if(*((int*)_Tmp5)==1){_Tmp8=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_Tmp5)->f1;_Tmp7=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_Tmp5)->f2;_Tmp6=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_Tmp5)->f4;{struct Cyc_Core_Opt*k=_Tmp8;void**p=(void**)_Tmp7;struct Cyc_Core_Opt*s=_Tmp6;
# 873
void*ev=Cyc_Absyn_new_evar(& Cyc_Kinds_eko,s);
# 875
if(!({int(*_Tmp9)(int(*)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct Cyc_Absyn_Tvar*)=(int(*)(int(*)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct Cyc_Absyn_Tvar*))Cyc_List_mem;_Tmp9;})(Cyc_Tcutil_fast_tvar_cmp,(struct Cyc_List_List*)s->v,v))return 0;
({void*_Tmp9=Cyc_Tcutil_dummy_fntype(Cyc_Absyn_join_eff(({void*_TmpA[2];_TmpA[0]=ev,({void*_TmpB=Cyc_Absyn_var_type(v);_TmpA[1]=_TmpB;});Cyc_List_list(_tag_fat(_TmpA,sizeof(void*),2));})));*p=_Tmp9;});
return 1;}}else{
return 0;};}}}else{goto _LLE;}}else{goto _LLE;}}else{_LLE: _Tmp4=_Tmp3;{void*e2=_Tmp4;
# 880
return Cyc_Tcutil_variable_in_effect(may_constrain_evars,v,e2);}};}}else{goto _LL9;}default: goto _LL9;}case 1: _Tmp2=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)e)->f1;_Tmp1=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)e)->f2;_Tmp0=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)e)->f4;{struct Cyc_Core_Opt*k=_Tmp2;void**p=(void**)_Tmp1;struct Cyc_Core_Opt*s=_Tmp0;
# 883
if(k==0 ||(int)((struct Cyc_Absyn_Kind*)k->v)->kind!=3)
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp3=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp4;_Tmp4.tag=0,_Tmp4.f1=_tag_fat("effect evar has wrong kind",sizeof(char),27U);_Tmp4;});void*_Tmp4[1];_Tmp4[0]=& _Tmp3;({int(*_Tmp5)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_Tmp5;})(_tag_fat(_Tmp4,sizeof(void*),1));});{
# 887
void*ev=Cyc_Absyn_new_evar(& Cyc_Kinds_eko,s);
# 889
if(!({int(*_Tmp3)(int(*)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct Cyc_Absyn_Tvar*)=(int(*)(int(*)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct Cyc_Absyn_Tvar*))Cyc_List_mem;_Tmp3;})(Cyc_Tcutil_fast_tvar_cmp,(struct Cyc_List_List*)s->v,v))
return 0;{
void*new_typ=Cyc_Absyn_join_eff(({void*_Tmp3[2];_Tmp3[0]=ev,({void*_Tmp4=Cyc_Absyn_var_type(v);_Tmp3[1]=_Tmp4;});Cyc_List_list(_tag_fat(_Tmp3,sizeof(void*),2));}));
*p=new_typ;
return 1;}}}default: _LL9:
 return 0;};}}
# 899
static int Cyc_Tcutil_evar_in_effect(void*evar,void*e){
e=Cyc_Absyn_compress(e);{
void*_Tmp0;switch(*((int*)e)){case 0: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e)->f1)){case 9: _Tmp0=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e)->f2;{struct Cyc_List_List*es=_Tmp0;
# 903
for(1;es!=0;es=es->tl){
if(Cyc_Tcutil_evar_in_effect(evar,(void*)es->hd))
return 1;}
return 0;}case 10: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e)->f2!=0){_Tmp0=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e)->f2->hd;{void*t=_Tmp0;
# 908
void*_Tmp1=Cyc_Tcutil_rgns_of(t);void*_Tmp2;if(*((int*)_Tmp1)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp1)->f1)==10){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp1)->f2!=0){_Tmp2=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp1)->f2->hd;{void*t2=_Tmp2;
return 0;}}else{goto _LLC;}}else{goto _LLC;}}else{_LLC: _Tmp2=_Tmp1;{void*e2=_Tmp2;
return Cyc_Tcutil_evar_in_effect(evar,e2);}};}}else{goto _LL7;}default: goto _LL7;}case 1:
# 912
 return evar==e;default: _LL7:
 return 0;};}}
# 918
static int Cyc_Tcutil_unify_effect_component(void*ev,void*eff){
{void*_Tmp0;void*_Tmp1;if(*((int*)ev)==1){_Tmp1=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)ev)->f2;_Tmp0=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)ev)->f4;{void**p=(void**)_Tmp1;struct Cyc_Core_Opt*s=_Tmp0;
# 921
{void*_Tmp2;if(*((int*)eff)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)eff)->f1)==9){_Tmp2=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)eff)->f2;{struct Cyc_List_List*l=_Tmp2;
# 923
for(1;l!=0;l=l->tl){
struct _handler_cons _Tmp3;_push_handler(& _Tmp3);{int _Tmp4=0;if(setjmp(_Tmp3.handler))_Tmp4=1;if(!_Tmp4){
Cyc_Unify_occurs(ev,Cyc_Core_heap_region,(struct Cyc_List_List*)s->v,(void*)l->hd);
*p=(void*)l->hd;{
int _Tmp5=1;_npop_handler(0);return _Tmp5;}
# 925
;_pop_handler();}else{void*_Tmp5=(void*)Cyc_Core_get_exn_thrown();
# 931
goto _LLA;_LLA:;}}}
# 934
goto _LL5;}}else{goto _LL8;}}else{_LL8:
# 937
 return 0;}_LL5:;}
# 939
goto _LL0;}}else{
# 941
({int(*_Tmp2)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Warn_impos;_Tmp2;})(_tag_fat("unify_effect_component expects an Evar",sizeof(char),39U),_tag_fat(0U,sizeof(void*),0));}_LL0:;}
# 943
return 0;}
# 955 "tcutil.cyc"
int Cyc_Tcutil_subset_effect(int may_constrain_evars,void*e1,void*e2){
# 958
if(e1==e2)return 1;
e1=Cyc_Absyn_compress(e1);{
void*_Tmp0;void*_Tmp1;switch(*((int*)e1)){case 0: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e1)->f1)){case 6:
# 962
 return Cyc_Tcutil_type_in_effect(may_constrain_evars,e1,e2);case 9: _Tmp1=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e1)->f2;{struct Cyc_List_List*es=_Tmp1;
# 964
for(1;es!=0;es=es->tl){
if(!Cyc_Tcutil_subset_effect(may_constrain_evars,(void*)es->hd,e2))
return 0;}
return 1;}case 10: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e1)->f2!=0){_Tmp1=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e1)->f2->hd;{void*t=_Tmp1;
# 970
void*_Tmp2=Cyc_Tcutil_rgns_of(t);void*_Tmp3;if(*((int*)_Tmp2)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp2)->f1)==10){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp2)->f2!=0){_Tmp3=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp2)->f2->hd;{void*t2=_Tmp3;
# 975
return Cyc_Tcutil_type_in_effect(may_constrain_evars,t2,e2)||
 may_constrain_evars && Cyc_Unify_unify(t2,Cyc_Absyn_sint_type);}}else{goto _LL10;}}else{goto _LL10;}}else{_LL10: _Tmp3=_Tmp2;{void*e=_Tmp3;
return Cyc_Tcutil_subset_effect(may_constrain_evars,e,e2);}};}}else{goto _LLB;}default: goto _LLB;}case 2: _Tmp1=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)e1)->f1;{struct Cyc_Absyn_Tvar*v=_Tmp1;
# 968
return Cyc_Tcutil_variable_in_effect(may_constrain_evars,v,e2);}case 1: _Tmp1=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)e1)->f2;_Tmp0=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)e1)->f4;{void**p=(void**)_Tmp1;struct Cyc_Core_Opt*s=_Tmp0;
# 980
int evar_occurs=Cyc_Tcutil_evar_in_effect(e1,e2);
# 982
if(!may_constrain_evars && evar_occurs)
return 1;
# 986
if(may_constrain_evars && !evar_occurs){
e2=Cyc_Tcutil_normalize_effect(e2);{
struct _handler_cons _Tmp2;_push_handler(& _Tmp2);{int _Tmp3=0;if(setjmp(_Tmp2.handler))_Tmp3=1;if(!_Tmp3){
Cyc_Unify_occurs(e1,Cyc_Core_heap_region,(struct Cyc_List_List*)_check_null(s)->v,e2);
*p=e2;{
int _Tmp4=1;_npop_handler(0);return _Tmp4;}
# 989
;_pop_handler();}else{void*_Tmp4=(void*)Cyc_Core_get_exn_thrown();
# 994
if(Cyc_Tcutil_type_in_effect(0,Cyc_Absyn_heap_rgn_type,e2)){
*p=Cyc_Absyn_heap_rgn_type;
return 1;}else{
# 999
return Cyc_Tcutil_unify_effect_component(e1,e2);};}}}}
# 1003
return 0;}default: _LLB:
 return Cyc_Tcutil_type_in_effect(may_constrain_evars,e1,e2);};}}
# 1009
static int Cyc_Tcutil_cmp_atomic_effect(void*t1,void*t2){
struct _tuple15 _Tmp0=({struct _tuple15 _Tmp1;({void*_Tmp2=Cyc_Absyn_compress(t1);_Tmp1.f0=_Tmp2;}),({void*_Tmp2=Cyc_Absyn_compress(t2);_Tmp1.f1=_Tmp2;});_Tmp1;});int _Tmp1;int _Tmp2;void*_Tmp3;void*_Tmp4;switch(*((int*)_Tmp0.f0)){case 0: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f0)->f1)){case 6:
# 1012
 return -1;case 10: switch(*((int*)_Tmp0.f1)){case 1:
# 1018
 return -1;case 0: if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f1)->f1)==6)goto _LL9;else{if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f0)->f2!=0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f1)->f1)==10){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f1)->f2!=0){_Tmp4=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f0)->f2->hd;_Tmp3=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f1)->f2->hd;{void*tt1=_Tmp4;void*tt2=_Tmp3;
# 1030
struct _tuple15 _Tmp5=({struct _tuple15 _Tmp6;({void*_Tmp7=Cyc_Tcutil_rgns_of(tt1);_Tmp6.f0=_Tmp7;}),({void*_Tmp7=Cyc_Tcutil_rgns_of(tt2);_Tmp6.f1=_Tmp7;});_Tmp6;});void*_Tmp6;void*_Tmp7;_Tmp7=_Tmp5.f0;_Tmp6=_Tmp5.f1;{void*rtt1=_Tmp7;void*rtt2=_Tmp6;
struct _tuple15 _Tmp8=({struct _tuple15 _Tmp9;_Tmp9.f0=rtt1,_Tmp9.f1=rtt2;_Tmp9;});void*_Tmp9;void*_TmpA;if(*((int*)_Tmp8.f0)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp8.f0)->f1)==10){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp8.f0)->f2!=0){if(*((int*)_Tmp8.f1)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp8.f1)->f1)==10){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp8.f1)->f2!=0){_TmpA=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp8.f0)->f2->hd;_Tmp9=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp8.f1)->f2->hd;{void*ev1=_TmpA;void*ev2=_Tmp9;
# 1033
return Cyc_Tcutil_cmp_atomic_effect(ev1,ev2);}}else{goto _LL1F;}}else{goto _LL1F;}}else{goto _LL1F;}}else{goto _LL1F;}}else{goto _LL1F;}}else{_LL1F:
# 1035
 return Cyc_Tcutil_cmp_atomic_effect(rtt1,rtt2);};}}}else{goto _LL17;}}else{goto _LL17;}}else{goto _LL17;}}case 2:
# 1022
 return 1;default: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f0)->f2!=0)goto _LL17;else{goto _LL17;}}default: if(*((int*)_Tmp0.f1)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f1)->f1)==6)goto _LL9;else{goto _LL17;}}else{goto _LL17;}}case 2: switch(*((int*)_Tmp0.f1)){case 0: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f1)->f1)){case 10:
# 1014
 return -1;case 6: goto _LL9;default: goto _LL17;}case 1:
# 1016
 return -1;case 2: _Tmp4=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_Tmp0.f0)->f1;_Tmp3=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_Tmp0.f1)->f1;{struct Cyc_Absyn_Tvar*tv1=_Tmp4;struct Cyc_Absyn_Tvar*tv2=_Tmp3;
# 1028
return tv1->identity - tv2->identity;}default: goto _LL17;}default: if(*((int*)_Tmp0.f1)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f1)->f1)==6){_LL9:
# 1020
 return 1;}else{if(*((int*)_Tmp0.f0)==1){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f1)->f1)==10)
# 1026
return 1;else{goto _LL17;}}else{goto _LL17;}}}else{if(*((int*)_Tmp0.f0)==1)switch(*((int*)_Tmp0.f1)){case 2:
# 1024
 return 1;case 1: _Tmp2=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_Tmp0.f0)->f3;_Tmp1=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_Tmp0.f1)->f3;{int ev1=_Tmp2;int ev2=_Tmp1;
# 1038
return ev1 - ev2;}default: goto _LL17;}else{_LL17:
# 1040
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp5=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp6;_Tmp6.tag=0,_Tmp6.f1=_tag_fat("Cannot canonicalize effects that contain typedefs",sizeof(char),50U);_Tmp6;});struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp6=({struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp7;_Tmp7.tag=2,_Tmp7.f1=(void*)t1;_Tmp7;});struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp7=({struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp8;_Tmp8.tag=2,_Tmp8.f1=(void*)t2;_Tmp8;});void*_Tmp8[3];_Tmp8[0]=& _Tmp5,_Tmp8[1]=& _Tmp6,_Tmp8[2]=& _Tmp7;({int(*_Tmp9)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_Tmp9;})(_tag_fat(_Tmp8,sizeof(void*),3));});}}};}
# 1045
static int Cyc_Tcutil_uncanonical_effect(void*a){
void*_Tmp0;switch(*((int*)a)){case 0: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)a)->f1)){case 6:
 goto _LL4;case 10: _Tmp0=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)a)->f2;{struct Cyc_List_List*l=_Tmp0;
# 1051
return Cyc_List_exists(Cyc_Tcutil_uncanonical_effect,l);}default: goto _LL9;}case 2: _LL4:
# 1048
 goto _LL6;case 1: _LL6:
 return 0;default: _LL9:
# 1053
 return 1;};}
# 1059
static int Cyc_Tcutil_canonical_join_effect(void*je){
{void*_Tmp0;if(*((int*)je)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)je)->f1)==9){_Tmp0=(struct Cyc_List_List**)&((struct Cyc_Absyn_AppType_Absyn_Type_struct*)je)->f2;{struct Cyc_List_List**l=(struct Cyc_List_List**)_Tmp0;
# 1062
if(!Cyc_List_exists(Cyc_Tcutil_uncanonical_effect,*l)){
({struct Cyc_List_List*_Tmp1=Cyc_List_rimp_merge_sort(Cyc_Tcutil_cmp_atomic_effect,*l);*l=_Tmp1;});
return 1;}
# 1066
goto _LL0;}}else{goto _LL3;}}else{_LL3:
# 1068
 goto _LL0;}_LL0:;}
# 1070
return 0;}
# 1074
static void*Cyc_Tcutil_close_rgnof_effect(void*,struct Cyc_List_List*);
static void*Cyc_Tcutil_close_canonical_join(void*,struct Cyc_List_List*);
static void*Cyc_Tcutil_close_effect(void*eff,struct Cyc_List_List*effconstr){
eff=Cyc_Tcutil_normalize_effect(eff);{
void*_Tmp0;if(*((int*)eff)==0)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)eff)->f1)){case 9: _Tmp0=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)eff)->f2;{struct Cyc_List_List*e_lst=_Tmp0;
# 1081
int can=Cyc_Tcutil_canonical_join_effect(eff);
if(can){
void*t=Cyc_Tcutil_close_canonical_join(eff,effconstr);
if(t!=eff)
return t;}{
# 1088
struct Cyc_List_List*tmplist=0;
{struct Cyc_List_List*i=e_lst;for(0;i!=0;i=i->tl){
tmplist=({struct Cyc_List_List*_Tmp1=_cycalloc(sizeof(struct Cyc_List_List));({void*_Tmp2=Cyc_Tcutil_close_effect((void*)i->hd,effconstr);_Tmp1->hd=_Tmp2;}),_Tmp1->tl=tmplist;_Tmp1;});}}
# 1093
return Cyc_Tcutil_normalize_effect(Cyc_Absyn_join_eff(tmplist));}}case 10: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)eff)->f2!=0){_Tmp0=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)eff)->f2->hd;{void*t2=_Tmp0;
# 1095
void*tt=Cyc_Tcutil_rgns_of(t2);
void*_Tmp1;if(*((int*)tt)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)tt)->f1)==10){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)tt)->f2!=0){_Tmp1=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)tt)->f2->hd;{void*t22=_Tmp1;
# 1098
void*to=Cyc_Tcutil_close_rgnof_effect(t2,effconstr);
if(to!=0)return to;else{return eff;}}}else{goto _LLA;}}else{goto _LLA;}}else{_LLA:
# 1101
 return tt;};}}else{goto _LL5;}default: goto _LL5;}else{_LL5:
# 1104
{struct Cyc_List_List*effc=effconstr;for(0;effc!=0;effc=effc->tl){
void*_Tmp1=(void*)effc->hd;void*_Tmp2;void*_Tmp3;if(*((int*)_Tmp1)==2){_Tmp3=(void*)((struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_Tmp1)->f1;_Tmp2=(void*)((struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_Tmp1)->f2;{void*lhs=_Tmp3;void*rhs=_Tmp2;
# 1107
if(({void*_Tmp4=Cyc_Tcutil_normalize_effect(lhs);Cyc_Tcutil_typecmp(_Tmp4,eff);})==0)
return Cyc_Tcutil_close_effect(rhs,effconstr);
# 1110
goto _LLC;}}else{
# 1112
goto _LLC;}_LLC:;}}
# 1115
return eff;};}}
# 1119
static void*Cyc_Tcutil_close_canonical_join(void*je,struct Cyc_List_List*effconstr){
# 1121
{struct Cyc_List_List*effc=effconstr;for(0;effc!=0;effc=effc->tl){
void*_Tmp0=(void*)effc->hd;void*_Tmp1;void*_Tmp2;if(*((int*)_Tmp0)==2){_Tmp2=(void*)((struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_Tmp0)->f1;_Tmp1=(void*)((struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_Tmp0)->f2;{void*lhs=_Tmp2;void*rhs=_Tmp1;
# 1124
lhs=Cyc_Tcutil_normalize_effect(lhs);
if(*((int*)lhs)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)lhs)->f1)==9){
# 1127
int can=Cyc_Tcutil_canonical_join_effect(lhs);
if(can && Cyc_Tcutil_typecmp(lhs,je)==0)
return Cyc_Tcutil_close_effect(rhs,effconstr);
# 1131
goto _LL5;}else{goto _LL8;}}else{_LL8:
# 1133
 goto _LL5;}_LL5:;
# 1135
goto _LL0;}}else{
# 1137
goto _LL0;}_LL0:;}}
# 1140
return je;}
# 1143
static void*Cyc_Tcutil_close_rgnof_effect(void*rgnof,struct Cyc_List_List*effconstr){
for(1;effconstr!=0;effconstr=effconstr->tl){
void*_Tmp0=(void*)effconstr->hd;void*_Tmp1;void*_Tmp2;if(*((int*)_Tmp0)==2){_Tmp2=(void*)((struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_Tmp0)->f1;_Tmp1=(void*)((struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_Tmp0)->f2;{void*lhs=_Tmp2;void*rhs=_Tmp1;
# 1147
{void*_Tmp3=Cyc_Absyn_compress(lhs);void*_Tmp4;if(*((int*)_Tmp3)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp3)->f1)==10){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp3)->f2!=0){_Tmp4=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp3)->f2->hd;{void*t1=_Tmp4;
# 1149
if(!Cyc_Tcutil_typecmp(t1,rgnof)){
void*_Tmp5=Cyc_Absyn_compress(rhs);return Cyc_Tcutil_close_effect(_Tmp5,effconstr);}
# 1152
goto _LL5;}}else{goto _LL8;}}else{goto _LL8;}}else{_LL8:
# 1154
 goto _LL5;}_LL5:;}
# 1156
goto _LL0;}}else{
# 1158
goto _LL0;}_LL0:;}
# 1161
return 0;}
# 1164
int Cyc_Tcutil_check_eff_in_cap(unsigned loc,int may_constrain_evars,void*eff,struct Cyc_List_List*effconstr,void*cap){
# 1167
eff=Cyc_Absyn_compress(eff);{
void*_Tmp0;if(*((int*)eff)==0)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)eff)->f1)){case 10: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)eff)->f2!=0){_Tmp0=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)eff)->f2->hd;{void*t1=_Tmp0;
# 1170
void*_Tmp1=Cyc_Tcutil_rgns_of(t1);void*_Tmp2;if(*((int*)_Tmp1)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp1)->f1)==10){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp1)->f2!=0){_Tmp2=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp1)->f2->hd;{void*t2=_Tmp2;
# 1173
void*e=({void*_Tmp3=Cyc_Absyn_regionsof_eff(t2);Cyc_Tcutil_close_effect(_Tmp3,effconstr);});
# 1175
return Cyc_Tcutil_subset_effect(may_constrain_evars,e,cap);}}else{goto _LLA;}}else{goto _LLA;}}else{_LLA: _Tmp2=_Tmp1;{void*e=_Tmp2;
# 1177
return Cyc_Tcutil_subset_effect(may_constrain_evars,e,cap);}};}}else{goto _LL5;}case 9: _Tmp0=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)eff)->f2;{struct Cyc_List_List*l=_Tmp0;
# 1180
for(1;l!=0;l=l->tl){
# 1182
if(!Cyc_Tcutil_check_eff_in_cap(loc,may_constrain_evars,(void*)l->hd,effconstr,cap))
return 0;}
# 1185
return 1;}default: goto _LL5;}else{_LL5:
# 1187
 return Cyc_Tcutil_subset_effect(may_constrain_evars,eff,cap)||({
int _Tmp1=may_constrain_evars;void*_Tmp2=Cyc_Tcutil_close_effect(eff,effconstr);Cyc_Tcutil_subset_effect(_Tmp1,_Tmp2,cap);});};}}
# 1192
static int Cyc_Tcutil_check_single_constraint(void*e,struct Cyc_List_List*env_ec){
{void*_Tmp0;if(*((int*)e)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e)->f1)==9){_Tmp0=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)e)->f2;{struct Cyc_List_List*l=_Tmp0;
# 1195
if(Cyc_List_length(l)==1)
e=(void*)l->hd;else{
# 1198
return 0;}
goto _LL0;}}else{goto _LL3;}}else{_LL3:
# 1201
 goto _LL0;}_LL0:;}
# 1203
for(1;env_ec!=0;env_ec=env_ec->tl){
void*_Tmp0=(void*)env_ec->hd;void*_Tmp1;if(*((int*)_Tmp0)==0){_Tmp1=(void*)((struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*)_Tmp0)->f1;{void*e1=_Tmp1;
# 1206
if(Cyc_Tcutil_typecmp(e1,e)==0)
return 1;
goto _LL5;}}else{
# 1210
goto _LL5;}_LL5:;}
# 1213
return 0;}struct _tuple18{void*f0;struct Cyc_List_List*f1;};
# 1217
static int Cyc_Tcutil_disjoint_effects(struct _tuple18*e,void*e2){
void*_Tmp0;void*_Tmp1;_Tmp1=e->f0;_Tmp0=e->f1;{void*e1=_Tmp1;struct Cyc_List_List*env_ec=_Tmp0;
for(1;env_ec!=0;env_ec=env_ec->tl){
void*_Tmp2=(void*)env_ec->hd;void*_Tmp3;void*_Tmp4;if(*((int*)_Tmp2)==1){_Tmp4=(void*)((struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*)_Tmp2)->f1;_Tmp3=(void*)((struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*)_Tmp2)->f2;{void*f1=_Tmp4;void*f2=_Tmp3;
# 1222
{struct _tuple15 _Tmp5=({struct _tuple15 _Tmp6;_Tmp6.f0=f1,_Tmp6.f1=f2;_Tmp6;});void*_Tmp6;void*_Tmp7;if(*((int*)_Tmp5.f0)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp5.f0)->f1)==9){if(*((int*)_Tmp5.f1)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp5.f1)->f1)==9){_Tmp7=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp5.f0)->f2;_Tmp6=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp5.f1)->f2;{struct Cyc_List_List*l1=_Tmp7;struct Cyc_List_List*l2=_Tmp6;
# 1224
if(Cyc_List_exists_c(Cyc_Tcutil_type_equals,e1,l1)&& Cyc_List_exists_c(Cyc_Tcutil_type_equals,e2,l2)||
 Cyc_List_exists_c(Cyc_Tcutil_type_equals,e2,l2)&& Cyc_List_exists_c(Cyc_Tcutil_type_equals,e1,l1))
return 1;
goto _LL8;}}else{goto _LLB;}}else{_LLB: _Tmp7=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp5.f0)->f2;_Tmp6=_Tmp5.f1;{struct Cyc_List_List*l1=_Tmp7;void*ff=_Tmp6;
_Tmp7=ff;_Tmp6=l1;goto _LLE;}}}else{if(*((int*)_Tmp5.f1)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp5.f1)->f1)==9)goto _LLD;else{goto _LLF;}}else{goto _LLF;}}}else{if(*((int*)_Tmp5.f1)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp5.f1)->f1)==9){_LLD: _Tmp7=_Tmp5.f0;_Tmp6=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp5.f1)->f2;_LLE: {void*ff=_Tmp7;struct Cyc_List_List*l1=_Tmp6;
# 1230
if(Cyc_Tcutil_typecmp(e1,ff)==0 && Cyc_List_exists_c(Cyc_Tcutil_type_equals,e2,l1)||
 Cyc_Tcutil_typecmp(e2,ff)==0 && Cyc_List_exists_c(Cyc_Tcutil_type_equals,e1,l1))
return 1;
goto _LL8;}}else{goto _LLF;}}else{_LLF:
# 1235
 if(Cyc_Tcutil_typecmp(e1,f1)==0 && Cyc_Tcutil_typecmp(e2,f2)==0 ||
 Cyc_Tcutil_typecmp(e1,f2)==0 && Cyc_Tcutil_typecmp(e2,f1)==0)
return 1;
goto _LL8;}}_LL8:;}
# 1240
goto _LL3;}}else{
# 1242
goto _LL3;}_LL3:;}
# 1245
return 0;}}
# 1248
static int Cyc_Tcutil_check_disjoint_constraint(void*e1,void*e2,struct Cyc_List_List*env_ec){
struct _tuple15 _Tmp0=({struct _tuple15 _Tmp1;_Tmp1.f0=e1,_Tmp1.f1=e2;_Tmp1;});void*_Tmp1;void*_Tmp2;if(*((int*)_Tmp0.f0)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f0)->f1)==9){if(*((int*)_Tmp0.f1)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f1)->f1)==9){_Tmp2=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f0)->f2;_Tmp1=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f1)->f2;{struct Cyc_List_List*l1=_Tmp2;struct Cyc_List_List*l2=_Tmp1;
# 1251
for(1;l1!=0;l1=l1->tl){
struct _tuple18 env=({struct _tuple18 _Tmp3;_Tmp3.f0=(void*)l1->hd,_Tmp3.f1=env_ec;_Tmp3;});
if(!({int(*_Tmp3)(int(*)(struct _tuple18*,void*),struct _tuple18*,struct Cyc_List_List*)=(int(*)(int(*)(struct _tuple18*,void*),struct _tuple18*,struct Cyc_List_List*))Cyc_List_forall_c;_Tmp3;})(Cyc_Tcutil_disjoint_effects,& env,l2))
return 0;}
# 1256
return 1;}}else{goto _LL3;}}else{_LL3: _Tmp2=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f0)->f2;{struct Cyc_List_List*l2=_Tmp2;
# 1258
return Cyc_Tcutil_check_disjoint_constraint(e2,e1,env_ec);}}}else{if(*((int*)_Tmp0.f1)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f1)->f1)==9)goto _LL5;else{goto _LL7;}}else{goto _LL7;}}}else{if(*((int*)_Tmp0.f1)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f1)->f1)==9){_LL5: _Tmp2=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f1)->f2;{struct Cyc_List_List*l2=_Tmp2;
# 1260
struct _tuple18 env=({struct _tuple18 _Tmp3;_Tmp3.f0=e1,_Tmp3.f1=env_ec;_Tmp3;});
return({int(*_Tmp3)(int(*)(struct _tuple18*,void*),struct _tuple18*,struct Cyc_List_List*)=(int(*)(int(*)(struct _tuple18*,void*),struct _tuple18*,struct Cyc_List_List*))Cyc_List_forall_c;_Tmp3;})(Cyc_Tcutil_disjoint_effects,& env,l2);}}else{goto _LL7;}}else{_LL7: {
# 1263
struct _tuple18 env=({struct _tuple18 _Tmp3;_Tmp3.f0=e1,_Tmp3.f1=env_ec;_Tmp3;});
return Cyc_Tcutil_disjoint_effects(& env,e2);}}};}
# 1268
int Cyc_Tcutil_satisfies_effect_constraints(unsigned loc,struct Cyc_List_List*ec,struct Cyc_List_List*env_ec){
for(1;ec!=0;ec=ec->tl){
void*_Tmp0=(void*)ec->hd;void*_Tmp1;void*_Tmp2;switch(*((int*)_Tmp0)){case 0: _Tmp2=(void*)((struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*)_Tmp0)->f1;{void*e=_Tmp2;
# 1272
e=Cyc_Absyn_compress(e);
if(!Cyc_Tcutil_check_single_constraint(e,env_ec)&& !({
void*_Tmp3=Cyc_Tcutil_close_effect(e,env_ec);Cyc_Tcutil_check_single_constraint(_Tmp3,env_ec);}))
return 0;
goto _LL0;}case 1: _Tmp2=(void*)((struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*)_Tmp0)->f1;_Tmp1=(void*)((struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*)_Tmp0)->f2;{void*e1=_Tmp2;void*e2=_Tmp1;
# 1278
if(!({void*_Tmp3=Cyc_Tcutil_normalize_effect(e1);void*_Tmp4=Cyc_Tcutil_normalize_effect(e2);Cyc_Tcutil_check_disjoint_constraint(_Tmp3,_Tmp4,env_ec);}))
return 0;
goto _LL0;}default: _Tmp2=(void*)((struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_Tmp0)->f1;_Tmp1=(void*)((struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_Tmp0)->f2;{void*e1=_Tmp2;void*e2=_Tmp1;
# 1282
if(!({void*_Tmp3=({void*_Tmp4=Cyc_Absyn_compress(e1);Cyc_Tcutil_close_effect(_Tmp4,env_ec);});Cyc_Tcutil_subset_effect(0,_Tmp3,Cyc_Absyn_compress(e2));}))
return 0;
goto _LL0;}}_LL0:;}
# 1287
return 1;}
# 1290
void*Cyc_Tcutil_evaluate_effect(struct Cyc_List_List*ec,void*eff){
return Cyc_Tcutil_close_effect(eff,ec);}
# 1294
int Cyc_Tcutil_cmp_effect_constraints(struct Cyc_List_List*ec0,struct Cyc_List_List*ec1){
# 1296
for(1;ec0!=0 && ec1!=0;(ec0=ec0->tl,ec1=ec1->tl)){
void*_Tmp0=(void*)ec0->hd;void*_Tmp1;void*_Tmp2;switch(*((int*)_Tmp0)){case 0: _Tmp2=(void*)((struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*)_Tmp0)->f1;{void*e01=_Tmp2;
# 1299
{void*_Tmp3=(void*)ec1->hd;void*_Tmp4;if(*((int*)_Tmp3)==0){_Tmp4=(void*)((struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*)_Tmp3)->f1;{void*e11=_Tmp4;
# 1301
if(!Cyc_Unify_unify(e01,e11))
return 1;
goto _LL7;}}else{
# 1305
return 1;}_LL7:;}
# 1307
goto _LL0;}case 1: _Tmp2=(void*)((struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*)_Tmp0)->f1;_Tmp1=(void*)((struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*)_Tmp0)->f2;{void*e01=_Tmp2;void*e02=_Tmp1;
# 1309
{void*_Tmp3=(void*)ec1->hd;void*_Tmp4;void*_Tmp5;if(*((int*)_Tmp3)==1){_Tmp5=(void*)((struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*)_Tmp3)->f1;_Tmp4=(void*)((struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*)_Tmp3)->f2;{void*e11=_Tmp5;void*e12=_Tmp4;
# 1311
if(!(Cyc_Unify_unify(e01,e11)&& Cyc_Unify_unify(e02,e12)))
return 1;
goto _LLC;}}else{
# 1315
return 1;}_LLC:;}
# 1317
goto _LL0;}default: _Tmp2=(void*)((struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_Tmp0)->f1;_Tmp1=(void*)((struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_Tmp0)->f2;{void*e01=_Tmp2;void*e02=_Tmp1;
# 1319
{void*_Tmp3=(void*)ec1->hd;void*_Tmp4;void*_Tmp5;if(*((int*)_Tmp3)==2){_Tmp5=(void*)((struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_Tmp3)->f1;_Tmp4=(void*)((struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_Tmp3)->f2;{void*e11=_Tmp5;void*e12=_Tmp4;
# 1321
if(!(Cyc_Unify_unify(e01,e11)&& Cyc_Unify_unify(e02,e12)))
return 1;
goto _LL11;}}else{
# 1325
return 1;}_LL11:;}
# 1327
goto _LL0;}}_LL0:;}
# 1330
return 0;}
# 1334
static int Cyc_Tcutil_tycon2int(void*t){
switch(*((int*)t)){case 1: switch((int)((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)t)->f1){case Cyc_Absyn_Unsigned: switch((int)((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)t)->f2){case Cyc_Absyn_Char_sz:
 return 501;case Cyc_Absyn_Short_sz:
# 1339
 return 504;case Cyc_Absyn_Int_sz:
# 1342
 return 507;case Cyc_Absyn_Long_sz:
# 1345
 return 507;case Cyc_Absyn_LongLong_sz:
# 1348
 return 513;default: goto _LL25;}case Cyc_Absyn_Signed: switch((int)((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)t)->f2){case Cyc_Absyn_Char_sz:
# 1337
 return 502;case Cyc_Absyn_Short_sz:
# 1340
 return 505;case Cyc_Absyn_Int_sz:
# 1343
 return 508;case Cyc_Absyn_Long_sz:
# 1346
 return 508;case Cyc_Absyn_LongLong_sz:
# 1349
 return 514;default: goto _LL25;}case Cyc_Absyn_None: switch((int)((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)t)->f2){case Cyc_Absyn_Char_sz:
# 1338
 return 503;case Cyc_Absyn_Short_sz:
# 1341
 return 506;case Cyc_Absyn_Int_sz:
# 1344
 return 509;case Cyc_Absyn_Long_sz:
# 1347
 return 509;case Cyc_Absyn_LongLong_sz:
# 1350
 return 515;default: goto _LL25;}default: goto _LL25;}case 2: switch((int)((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)t)->f1){case 0:
 return 516;case 1:
 return 517;default:
 return 518;}default: _LL25:
 return(int)*((const unsigned*)t);};}
# 1357
static int Cyc_Tcutil_type_case_number(void*t){
void*_Tmp0;if(*((int*)t)==0){_Tmp0=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)t)->f1;{void*c=_Tmp0;
return 1000 + Cyc_Tcutil_tycon2int(c);}}else{
return(int)*((const unsigned*)t);};}
# 1363
static int Cyc_Tcutil_star_cmp(int(*cmp)(void*,void*),void*a1,void*a2){
if(a1==a2)return 0;
if(a1==0 && a2!=0)return -1;
if(a1!=0 && a2==0)return 1;
return cmp(a1,a2);}
# 1369
static int Cyc_Tcutil_enumfield_cmp(struct Cyc_Absyn_Enumfield*e1,struct Cyc_Absyn_Enumfield*e2){
int qc=Cyc_Absyn_qvar_cmp(e1->name,e2->name);
if(qc!=0)return qc;
return({int(*_Tmp0)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*)=(int(*)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*))Cyc_Tcutil_star_cmp;_Tmp0;})(Cyc_Evexp_uint_exp_cmp,e1->tag,e2->tag);}
# 1374
static struct _tuple1*Cyc_Tcutil_get_datatype_qvar(union Cyc_Absyn_DatatypeInfo i){
void*_Tmp0;if(i.KnownDatatype.tag==2){_Tmp0=*i.KnownDatatype.val;{struct Cyc_Absyn_Datatypedecl*dd=_Tmp0;
return dd->name;}}else{_Tmp0=i.UnknownDatatype.val.name;{struct _tuple1*n=_Tmp0;
return n;}};}struct _tuple19{struct _tuple1*f0;struct _tuple1*f1;};
# 1380
static struct _tuple19 Cyc_Tcutil_get_datatype_field_qvars(union Cyc_Absyn_DatatypeFieldInfo i){
void*_Tmp0;void*_Tmp1;if(i.KnownDatatypefield.tag==2){_Tmp1=i.KnownDatatypefield.val.f0;_Tmp0=i.KnownDatatypefield.val.f1;{struct Cyc_Absyn_Datatypedecl*dd=_Tmp1;struct Cyc_Absyn_Datatypefield*df=_Tmp0;
# 1383
struct _tuple19 _Tmp2;_Tmp2.f0=dd->name,_Tmp2.f1=df->name;return _Tmp2;}}else{_Tmp1=i.UnknownDatatypefield.val.datatype_name;_Tmp0=i.UnknownDatatypefield.val.field_name;{struct _tuple1*d=_Tmp1;struct _tuple1*f=_Tmp0;
# 1385
struct _tuple19 _Tmp2;_Tmp2.f0=d,_Tmp2.f1=f;return _Tmp2;}};}struct _tuple20{enum Cyc_Absyn_AggrKind f0;struct _tuple1*f1;};
# 1388
static struct _tuple20 Cyc_Tcutil_get_aggr_kind_and_qvar(union Cyc_Absyn_AggrInfo i){
void*_Tmp0;enum Cyc_Absyn_AggrKind _Tmp1;if(i.UnknownAggr.tag==1){_Tmp1=i.UnknownAggr.val.f0;_Tmp0=i.UnknownAggr.val.f1;{enum Cyc_Absyn_AggrKind k=_Tmp1;struct _tuple1*n=_Tmp0;
struct _tuple20 _Tmp2;_Tmp2.f0=k,_Tmp2.f1=n;return _Tmp2;}}else{_Tmp0=*i.KnownAggr.val;{struct Cyc_Absyn_Aggrdecl*ad=_Tmp0;
struct _tuple20 _Tmp2;_Tmp2.f0=ad->kind,_Tmp2.f1=ad->name;return _Tmp2;}};}
# 1394
int Cyc_Tcutil_tycon_cmp(void*t1,void*t2){
if(t1==t2)return 0;{
int i1=Cyc_Tcutil_tycon2int(t1);
int i2=Cyc_Tcutil_tycon2int(t2);
if(i1!=i2)return i1 - i2;{
# 1400
struct _tuple15 _Tmp0=({struct _tuple15 _Tmp1;_Tmp1.f0=t1,_Tmp1.f1=t2;_Tmp1;});union Cyc_Absyn_AggrInfo _Tmp1;union Cyc_Absyn_AggrInfo _Tmp2;union Cyc_Absyn_DatatypeFieldInfo _Tmp3;union Cyc_Absyn_DatatypeFieldInfo _Tmp4;union Cyc_Absyn_DatatypeInfo _Tmp5;union Cyc_Absyn_DatatypeInfo _Tmp6;struct _fat_ptr _Tmp7;struct _fat_ptr _Tmp8;void*_Tmp9;void*_TmpA;switch(*((int*)_Tmp0.f0)){case 19: if(*((int*)_Tmp0.f1)==19){_TmpA=((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)_Tmp0.f0)->f1;_Tmp9=((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)_Tmp0.f1)->f1;{struct _tuple1*n1=_TmpA;struct _tuple1*n2=_Tmp9;
return Cyc_Absyn_qvar_cmp(n1,n2);}}else{goto _LLD;}case 21: if(*((int*)_Tmp0.f1)==21){_Tmp8=((struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct*)_Tmp0.f0)->f1;_Tmp7=((struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct*)_Tmp0.f1)->f1;{struct _fat_ptr s1=_Tmp8;struct _fat_ptr s2=_Tmp7;
return Cyc_strcmp(s1,s2);}}else{goto _LLD;}case 20: if(*((int*)_Tmp0.f1)==20){_TmpA=((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)_Tmp0.f0)->f1;_Tmp9=((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)_Tmp0.f1)->f1;{struct Cyc_List_List*fs1=_TmpA;struct Cyc_List_List*fs2=_Tmp9;
# 1404
return({int(*_TmpB)(int(*)(struct Cyc_Absyn_Enumfield*,struct Cyc_Absyn_Enumfield*),struct Cyc_List_List*,struct Cyc_List_List*)=(int(*)(int(*)(struct Cyc_Absyn_Enumfield*,struct Cyc_Absyn_Enumfield*),struct Cyc_List_List*,struct Cyc_List_List*))Cyc_List_list_cmp;_TmpB;})(Cyc_Tcutil_enumfield_cmp,fs1,fs2);}}else{goto _LLD;}case 22: if(*((int*)_Tmp0.f1)==22){_Tmp6=((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)_Tmp0.f0)->f1;_Tmp5=((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)_Tmp0.f1)->f1;{union Cyc_Absyn_DatatypeInfo info1=_Tmp6;union Cyc_Absyn_DatatypeInfo info2=_Tmp5;
# 1406
struct _tuple1*_TmpB=Cyc_Tcutil_get_datatype_qvar(info1);return Cyc_Absyn_qvar_cmp(_TmpB,Cyc_Tcutil_get_datatype_qvar(info2));}}else{goto _LLD;}case 23: if(*((int*)_Tmp0.f1)==23){_Tmp4=((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_Tmp0.f0)->f1;_Tmp3=((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_Tmp0.f1)->f1;{union Cyc_Absyn_DatatypeFieldInfo info1=_Tmp4;union Cyc_Absyn_DatatypeFieldInfo info2=_Tmp3;
# 1408
struct _tuple19 _TmpB=Cyc_Tcutil_get_datatype_field_qvars(info1);void*_TmpC;void*_TmpD;_TmpD=_TmpB.f0;_TmpC=_TmpB.f1;{struct _tuple1*d1=_TmpD;struct _tuple1*f1=_TmpC;
struct _tuple19 _TmpE=Cyc_Tcutil_get_datatype_field_qvars(info2);void*_TmpF;void*_Tmp10;_Tmp10=_TmpE.f0;_TmpF=_TmpE.f1;{struct _tuple1*d2=_Tmp10;struct _tuple1*f2=_TmpF;
int c=Cyc_Absyn_qvar_cmp(d1,d2);
if(c!=0)return c;
return Cyc_Absyn_qvar_cmp(f1,f2);}}}}else{goto _LLD;}case 24: if(*((int*)_Tmp0.f1)==24){_Tmp2=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_Tmp0.f0)->f1;_Tmp1=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_Tmp0.f1)->f1;{union Cyc_Absyn_AggrInfo info1=_Tmp2;union Cyc_Absyn_AggrInfo info2=_Tmp1;
# 1414
struct _tuple20 _TmpB=Cyc_Tcutil_get_aggr_kind_and_qvar(info1);void*_TmpC;enum Cyc_Absyn_AggrKind _TmpD;_TmpD=_TmpB.f0;_TmpC=_TmpB.f1;{enum Cyc_Absyn_AggrKind k1=_TmpD;struct _tuple1*q1=_TmpC;
struct _tuple20 _TmpE=Cyc_Tcutil_get_aggr_kind_and_qvar(info2);void*_TmpF;enum Cyc_Absyn_AggrKind _Tmp10;_Tmp10=_TmpE.f0;_TmpF=_TmpE.f1;{enum Cyc_Absyn_AggrKind k2=_Tmp10;struct _tuple1*q2=_TmpF;
int c=Cyc_Absyn_qvar_cmp(q1,q2);
if(c!=0)return c;
return(int)k1 - (int)k2;}}}}else{goto _LLD;}default: _LLD:
 return 0;};}}}
# 1423
static int Cyc_Tcutil_tqual_cmp(struct Cyc_Absyn_Tqual tq1,struct Cyc_Absyn_Tqual tq2){
int i1=(tq1.real_const + (tq1.q_volatile << 1))+ (tq1.q_restrict << 2);
int i2=(tq2.real_const + (tq2.q_volatile << 1))+ (tq2.q_restrict << 2);
return Cyc_Core_intcmp(i1,i2);}
# 1428
static int Cyc_Tcutil_tqual_type_cmp(struct _tuple14*tqt1,struct _tuple14*tqt2){
void*_Tmp0;struct Cyc_Absyn_Tqual _Tmp1;_Tmp1=tqt1->f0;_Tmp0=tqt1->f1;{struct Cyc_Absyn_Tqual tq1=_Tmp1;void*t1=_Tmp0;
void*_Tmp2;struct Cyc_Absyn_Tqual _Tmp3;_Tmp3=tqt2->f0;_Tmp2=tqt2->f1;{struct Cyc_Absyn_Tqual tq2=_Tmp3;void*t2=_Tmp2;
int tqc=Cyc_Tcutil_tqual_cmp(tq1,tq2);
if(tqc!=0)return tqc;
return Cyc_Tcutil_typecmp(t1,t2);}}}
# 1436
int Cyc_Tcutil_aggrfield_cmp(struct Cyc_Absyn_Aggrfield*f1,struct Cyc_Absyn_Aggrfield*f2){
int zsc=Cyc_strptrcmp(f1->name,f2->name);
if(zsc!=0)return zsc;{
int tqc=Cyc_Tcutil_tqual_cmp(f1->tq,f2->tq);
if(tqc!=0)return tqc;{
int tc=Cyc_Tcutil_typecmp(f1->type,f2->type);
if(tc!=0)return tc;{
int ac=Cyc_List_list_cmp(Cyc_Atts_attribute_cmp,f1->attributes,f2->attributes);
if(ac!=0)return ac;
ac=({int(*_Tmp0)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*)=(int(*)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*))Cyc_Tcutil_star_cmp;_Tmp0;})(Cyc_Evexp_uint_exp_cmp,f1->width,f2->width);
if(ac!=0)return ac;
return({int(*_Tmp0)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*)=(int(*)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*))Cyc_Tcutil_star_cmp;_Tmp0;})(Cyc_Evexp_uint_exp_cmp,f1->requires_clause,f2->requires_clause);}}}}
# 1453
int Cyc_Tcutil_typecmp(void*t1,void*t2){
t1=Cyc_Absyn_compress(t1);
t2=Cyc_Absyn_compress(t2);
if(t1==t2)return 0;{
int shallowcmp=({int _Tmp0=Cyc_Tcutil_type_case_number(t1);Cyc_Core_intcmp(_Tmp0,Cyc_Tcutil_type_case_number(t2));});
if(shallowcmp!=0)
return shallowcmp;{
# 1462
struct _tuple15 _Tmp0=({struct _tuple15 _Tmp1;_Tmp1.f0=t2,_Tmp1.f1=t1;_Tmp1;});const char*_Tmp1;int _Tmp2;const char*_Tmp3;int _Tmp4;enum Cyc_Absyn_AggrKind _Tmp5;enum Cyc_Absyn_AggrKind _Tmp6;struct Cyc_Absyn_FnInfo _Tmp7;struct Cyc_Absyn_FnInfo _Tmp8;void*_Tmp9;void*_TmpA;void*_TmpB;void*_TmpC;void*_TmpD;void*_TmpE;struct Cyc_Absyn_Tqual _TmpF;void*_Tmp10;void*_Tmp11;void*_Tmp12;void*_Tmp13;struct Cyc_Absyn_Tqual _Tmp14;void*_Tmp15;void*_Tmp16;void*_Tmp17;void*_Tmp18;switch(*((int*)_Tmp0.f0)){case 0: if(*((int*)_Tmp0.f1)==0){_Tmp18=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f0)->f1;_Tmp17=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f0)->f2;_Tmp16=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f1)->f1;_Tmp15=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f1)->f2;{void*c1=_Tmp18;struct Cyc_List_List*ts1=_Tmp17;void*c2=_Tmp16;struct Cyc_List_List*ts2=_Tmp15;
# 1464
int c=Cyc_Tcutil_tycon_cmp(c1,c2);
if(c!=0)return c;
return Cyc_List_list_cmp(Cyc_Tcutil_typecmp,ts1,ts2);}}else{goto _LL17;}case 1: if(*((int*)_Tmp0.f1)==1)
# 1468
return(int)t1 - (int)t2;else{goto _LL17;}case 2: if(*((int*)_Tmp0.f1)==2){_Tmp18=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_Tmp0.f0)->f1;_Tmp17=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_Tmp0.f1)->f1;{struct Cyc_Absyn_Tvar*tv2=_Tmp18;struct Cyc_Absyn_Tvar*tv1=_Tmp17;
# 1473
return Cyc_Core_intcmp(tv1->identity,tv2->identity);}}else{goto _LL17;}case 4: if(*((int*)_Tmp0.f1)==4){_Tmp18=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0.f0)->f1.elt_type;_Tmp14=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0.f0)->f1.elt_tq;_Tmp17=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0.f0)->f1.ptr_atts.eff;_Tmp16=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0.f0)->f1.ptr_atts.nullable;_Tmp15=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0.f0)->f1.ptr_atts.bounds;_Tmp13=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0.f0)->f1.ptr_atts.zero_term;_Tmp12=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0.f0)->f1.ptr_atts.autoreleased;_Tmp11=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0.f0)->f1.ptr_atts.aqual;_Tmp10=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0.f1)->f1.elt_type;_TmpF=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0.f1)->f1.elt_tq;_TmpE=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0.f1)->f1.ptr_atts.eff;_TmpD=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0.f1)->f1.ptr_atts.nullable;_TmpC=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0.f1)->f1.ptr_atts.bounds;_TmpB=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0.f1)->f1.ptr_atts.zero_term;_TmpA=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0.f1)->f1.ptr_atts.autoreleased;_Tmp9=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0.f1)->f1.ptr_atts.aqual;{void*t2a=_Tmp18;struct Cyc_Absyn_Tqual tqual2a=_Tmp14;void*rgn2=_Tmp17;void*null2a=_Tmp16;void*b2=_Tmp15;void*zt2=_Tmp13;void*rel2=_Tmp12;void*aq2=_Tmp11;void*t1a=_Tmp10;struct Cyc_Absyn_Tqual tqual1a=_TmpF;void*rgn1=_TmpE;void*null1a=_TmpD;void*b1=_TmpC;void*zt1=_TmpB;void*rel1=_TmpA;void*aq1=_Tmp9;
# 1477
int etc=Cyc_Tcutil_typecmp(t1a,t2a);
if(etc!=0)return etc;{
int rc=Cyc_Tcutil_typecmp(rgn1,rgn2);
if(rc!=0)return rc;{
int tqc=Cyc_Tcutil_tqual_cmp(tqual1a,tqual2a);
if(tqc!=0)return tqc;{
int cc=Cyc_Tcutil_typecmp(b1,b2);
if(cc!=0)return cc;{
int zc=Cyc_Tcutil_typecmp(zt1,zt2);
if(zc!=0)return zc;{
int relc=Cyc_Tcutil_typecmp(rel1,rel2);
if(relc!=0)return relc;{
int bc=Cyc_Tcutil_typecmp(b1,b2);
if(bc!=0)return bc;
if(aq1!=aq2)return 1;
return Cyc_Tcutil_typecmp(null1a,null2a);}}}}}}}}else{goto _LL17;}case 5: if(*((int*)_Tmp0.f1)==5){_Tmp18=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0.f0)->f1.elt_type;_Tmp14=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0.f0)->f1.tq;_Tmp17=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0.f0)->f1.num_elts;_Tmp16=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0.f0)->f1.zero_term;_Tmp15=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0.f1)->f1.elt_type;_TmpF=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0.f1)->f1.tq;_Tmp13=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0.f1)->f1.num_elts;_Tmp12=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0.f1)->f1.zero_term;{void*t2a=_Tmp18;struct Cyc_Absyn_Tqual tq2a=_Tmp14;struct Cyc_Absyn_Exp*e1=_Tmp17;void*zt1=_Tmp16;void*t1a=_Tmp15;struct Cyc_Absyn_Tqual tq1a=_TmpF;struct Cyc_Absyn_Exp*e2=_Tmp13;void*zt2=_Tmp12;
# 1496
int tqc=Cyc_Tcutil_tqual_cmp(tq1a,tq2a);
if(tqc!=0)return tqc;{
int tc=Cyc_Tcutil_typecmp(t1a,t2a);
if(tc!=0)return tc;{
int ztc=Cyc_Tcutil_typecmp(zt1,zt2);
if(ztc!=0)return ztc;
if(e1==e2)return 0;
if(e1==0 || e2==0)
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp19=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp1A;_Tmp1A.tag=0,_Tmp1A.f1=_tag_fat("missing expression in array index",sizeof(char),34U);_Tmp1A;});void*_Tmp1A[1];_Tmp1A[0]=& _Tmp19;({int(*_Tmp1B)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_Tmp1B;})(_tag_fat(_Tmp1A,sizeof(void*),1));});
return({int(*_Tmp19)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*)=(int(*)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*))Cyc_Tcutil_star_cmp;_Tmp19;})(Cyc_Evexp_uint_exp_cmp,e1,e2);}}}}else{goto _LL17;}case 6: if(*((int*)_Tmp0.f1)==6){_Tmp8=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0.f0)->f1;_Tmp7=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0.f1)->f1;{struct Cyc_Absyn_FnInfo f1=_Tmp8;struct Cyc_Absyn_FnInfo f2=_Tmp7;
# 1508
if(Cyc_Unify_unify(t1,t2))return 0;{
int r=Cyc_Tcutil_typecmp(f1.ret_type,f2.ret_type);
if(r!=0)return r;
r=Cyc_Tcutil_tqual_cmp(f1.ret_tqual,f2.ret_tqual);
if(r!=0)return r;{
struct Cyc_List_List*args1=f1.args;
struct Cyc_List_List*args2=f2.args;
for(1;args1!=0 && args2!=0;(args1=args1->tl,args2=args2->tl)){
struct _tuple9 _Tmp19=*((struct _tuple9*)args1->hd);void*_Tmp1A;struct Cyc_Absyn_Tqual _Tmp1B;_Tmp1B=_Tmp19.f1;_Tmp1A=_Tmp19.f2;{struct Cyc_Absyn_Tqual tq1=_Tmp1B;void*t1=_Tmp1A;
struct _tuple9 _Tmp1C=*((struct _tuple9*)args2->hd);void*_Tmp1D;struct Cyc_Absyn_Tqual _Tmp1E;_Tmp1E=_Tmp1C.f1;_Tmp1D=_Tmp1C.f2;{struct Cyc_Absyn_Tqual tq2=_Tmp1E;void*t2=_Tmp1D;
r=Cyc_Tcutil_tqual_cmp(tq1,tq2);
if(r!=0)return r;
r=Cyc_Tcutil_typecmp(t1,t2);
if(r!=0)return r;}}}
# 1523
if(args1!=0)return 1;
if(args2!=0)return -1;
if(f1.c_varargs && !f2.c_varargs)return 1;
if(!f1.c_varargs && f2.c_varargs)return -1;
if(f1.cyc_varargs!=0 && f2.cyc_varargs==0)return 1;
if(f1.cyc_varargs==0 && f2.cyc_varargs!=0)return -1;
if(f1.cyc_varargs!=0 && f2.cyc_varargs!=0){
r=Cyc_Tcutil_tqual_cmp(f1.cyc_varargs->tq,f2.cyc_varargs->tq);
if(r!=0)return r;
r=Cyc_Tcutil_typecmp(f1.cyc_varargs->type,f2.cyc_varargs->type);
if(r!=0)return r;
if(f1.cyc_varargs->inject && !f2.cyc_varargs->inject)return 1;
if(!f1.cyc_varargs->inject && f2.cyc_varargs->inject)return -1;}
# 1537
r=Cyc_Tcutil_star_cmp(Cyc_Tcutil_typecmp,f1.effect,f2.effect);
if(r!=0)return r;
# 1549 "tcutil.cyc"
if(Cyc_Tcutil_cmp_effect_constraints(f1.effconstr,f2.effconstr))
return 1;
r=({int(*_Tmp19)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*)=(int(*)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*))Cyc_Tcutil_star_cmp;_Tmp19;})(Cyc_Evexp_uint_exp_cmp,f1.requires_clause,f2.requires_clause);
if(r!=0)return r;
r=({int(*_Tmp19)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*)=(int(*)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*))Cyc_Tcutil_star_cmp;_Tmp19;})(Cyc_Evexp_uint_exp_cmp,f1.ensures_clause,f2.ensures_clause);
if(r!=0)return r;
# 1556
if(Cyc_Atts_same_atts(f1.attributes,f2.attributes))
# 1559
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp19=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp1A;_Tmp1A.tag=0,_Tmp1A.f1=_tag_fat("typecmp: function type comparison should never get here!",sizeof(char),57U);_Tmp1A;});void*_Tmp1A[1];_Tmp1A[0]=& _Tmp19;({int(*_Tmp1B)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_Tmp1B;})(_tag_fat(_Tmp1A,sizeof(void*),1));});
# 1561
return 1;}}}}else{goto _LL17;}case 7: if(*((int*)_Tmp0.f1)==7){_Tmp6=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_Tmp0.f0)->f1;_Tmp18=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_Tmp0.f0)->f3;_Tmp5=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_Tmp0.f1)->f1;_Tmp17=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_Tmp0.f1)->f3;{enum Cyc_Absyn_AggrKind k2=_Tmp6;struct Cyc_List_List*fs2=_Tmp18;enum Cyc_Absyn_AggrKind k1=_Tmp5;struct Cyc_List_List*fs1=_Tmp17;
# 1563
if((int)k1!=(int)k2){
if((int)k1==0)return -1;else{return 1;}}
return({int(*_Tmp19)(int(*)(struct Cyc_Absyn_Aggrfield*,struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*,struct Cyc_List_List*)=(int(*)(int(*)(struct Cyc_Absyn_Aggrfield*,struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*,struct Cyc_List_List*))Cyc_List_list_cmp;_Tmp19;})(Cyc_Tcutil_aggrfield_cmp,fs1,fs2);}}else{goto _LL17;}case 9: if(*((int*)_Tmp0.f1)==9){_Tmp18=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_Tmp0.f0)->f1;_Tmp17=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_Tmp0.f1)->f1;{struct Cyc_Absyn_Exp*e1=_Tmp18;struct Cyc_Absyn_Exp*e2=_Tmp17;
# 1567
_Tmp18=e1;_Tmp17=e2;goto _LL12;}}else{goto _LL17;}case 11: if(*((int*)_Tmp0.f1)==11){_Tmp18=((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_Tmp0.f0)->f1;_Tmp17=((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_Tmp0.f1)->f1;_LL12: {struct Cyc_Absyn_Exp*e1=_Tmp18;struct Cyc_Absyn_Exp*e2=_Tmp17;
return Cyc_Evexp_uint_exp_cmp(e1,e2);}}else{goto _LL17;}case 8: if(*((int*)_Tmp0.f1)==8){_Tmp18=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_Tmp0.f0)->f1;_Tmp17=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_Tmp0.f0)->f2;_Tmp16=(void*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_Tmp0.f0)->f4;_Tmp15=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_Tmp0.f1)->f1;_Tmp13=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_Tmp0.f1)->f2;_Tmp12=(void*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_Tmp0.f1)->f4;{struct _tuple1*n1=_Tmp18;struct Cyc_List_List*ts1=_Tmp17;void*def1=_Tmp16;struct _tuple1*n2=_Tmp15;struct Cyc_List_List*ts2=_Tmp13;void*def2=_Tmp12;
# 1571
if(def1!=0 && def2!=0)
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp19=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp1A;_Tmp1A.tag=0,_Tmp1A.f1=_tag_fat("typecmp: compress did not remove typedefs",sizeof(char),42U);_Tmp1A;});void*_Tmp1A[1];_Tmp1A[0]=& _Tmp19;({int(*_Tmp1B)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_Tmp1B;})(_tag_fat(_Tmp1A,sizeof(void*),1));});{
int i=Cyc_Absyn_qvar_cmp(n1,n2);if(i!=0)return i;
return Cyc_List_list_cmp(Cyc_Tcutil_typecmp,ts1,ts2);}}}else{goto _LL17;}case 3: if(*((int*)_Tmp0.f1)==3){_Tmp4=((struct Cyc_Absyn_Cvar_Absyn_Type_struct*)_Tmp0.f0)->f3;_Tmp3=((struct Cyc_Absyn_Cvar_Absyn_Type_struct*)_Tmp0.f0)->f5;_Tmp2=((struct Cyc_Absyn_Cvar_Absyn_Type_struct*)_Tmp0.f1)->f3;_Tmp1=((struct Cyc_Absyn_Cvar_Absyn_Type_struct*)_Tmp0.f1)->f5;{int id1=_Tmp4;const char*n1=_Tmp3;int id2=_Tmp2;const char*n2=_Tmp1;
# 1576
return Cyc_strcmp(({const char*_Tmp19=n1;_tag_fat((void*)_Tmp19,sizeof(char),_get_zero_arr_size_char((void*)_Tmp19,1U));}),({const char*_Tmp19=n2;_tag_fat((void*)_Tmp19,sizeof(char),_get_zero_arr_size_char((void*)_Tmp19,1U));}));}}else{goto _LL17;}default: _LL17:
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp19=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp1A;_Tmp1A.tag=0,_Tmp1A.f1=_tag_fat("Unmatched case in typecmp",sizeof(char),26U);_Tmp1A;});void*_Tmp1A[1];_Tmp1A[0]=& _Tmp19;({int(*_Tmp1B)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_Tmp1B;})(_tag_fat(_Tmp1A,sizeof(void*),1));});};}}}
# 1585
static int Cyc_Tcutil_will_lose_precision(void*t1,void*t2){
struct _tuple15 _Tmp0=({struct _tuple15 _Tmp1;({void*_Tmp2=Cyc_Absyn_compress(t1);_Tmp1.f0=_Tmp2;}),({void*_Tmp2=Cyc_Absyn_compress(t2);_Tmp1.f1=_Tmp2;});_Tmp1;});void*_Tmp1;void*_Tmp2;if(*((int*)_Tmp0.f0)==0){if(*((int*)_Tmp0.f1)==0){_Tmp2=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f0)->f1;_Tmp1=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f1)->f1;{void*c1=_Tmp2;void*c2=_Tmp1;
# 1588
struct _tuple15 _Tmp3=({struct _tuple15 _Tmp4;_Tmp4.f0=c1,_Tmp4.f1=c2;_Tmp4;});int _Tmp4;int _Tmp5;switch(*((int*)_Tmp3.f0)){case 2: switch(*((int*)_Tmp3.f1)){case 2: _Tmp5=((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_Tmp3.f0)->f1;_Tmp4=((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_Tmp3.f1)->f1;{int i1=_Tmp5;int i2=_Tmp4;
return i2 < i1;}case 1:
 goto _LLB;case 5: _LLB:
 return 1;default: goto _LL26;}case 1: switch((int)((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f0)->f2){case Cyc_Absyn_LongLong_sz: if(*((int*)_Tmp3.f1)==1){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f1)->f2==Cyc_Absyn_LongLong_sz)
return 0;else{goto _LLE;}}else{_LLE:
 return 1;}case Cyc_Absyn_Long_sz: switch(*((int*)_Tmp3.f1)){case 1: switch((int)((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f1)->f2){case Cyc_Absyn_Int_sz:
# 1596
 goto _LL13;case Cyc_Absyn_Short_sz: _LL19:
# 1601
 goto _LL1B;case Cyc_Absyn_Char_sz: _LL1F:
# 1604
 goto _LL21;default: goto _LL26;}case 2: if(((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_Tmp3.f1)->f1==0)
# 1599
goto _LL17;else{goto _LL26;}default: goto _LL26;}case Cyc_Absyn_Int_sz: switch(*((int*)_Tmp3.f1)){case 1: switch((int)((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f1)->f2){case Cyc_Absyn_Long_sz: _LL13:
# 1597
 return 0;case Cyc_Absyn_Short_sz: _LL1B:
# 1602
 goto _LL1D;case Cyc_Absyn_Char_sz: _LL21:
# 1605
 goto _LL23;default: goto _LL26;}case 2: if(((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_Tmp3.f1)->f1==0){_LL17:
# 1600
 goto _LL19;}else{goto _LL26;}default: goto _LL26;}case Cyc_Absyn_Short_sz: if(*((int*)_Tmp3.f1)==1){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f1)->f2==Cyc_Absyn_Char_sz){_LL23:
# 1606
 goto _LL25;}else{goto _LL26;}}else{goto _LL26;}default: goto _LL26;}case 5: if(*((int*)_Tmp3.f1)==1)switch((int)((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f1)->f2){case Cyc_Absyn_Short_sz: _LL1D:
# 1603
 goto _LL1F;case Cyc_Absyn_Char_sz: _LL25:
# 1607
 return 1;default: goto _LL26;}else{goto _LL26;}default: _LL26:
# 1609
 return 0;};}}else{goto _LL3;}}else{_LL3:
# 1611
 return 0;};}
# 1615
void*Cyc_Tcutil_max_arithmetic_type(void*t1,void*t2){
{struct _tuple15 _Tmp0=({struct _tuple15 _Tmp1;({void*_Tmp2=Cyc_Absyn_compress(t1);_Tmp1.f0=_Tmp2;}),({void*_Tmp2=Cyc_Absyn_compress(t2);_Tmp1.f1=_Tmp2;});_Tmp1;});void*_Tmp1;void*_Tmp2;if(*((int*)_Tmp0.f0)==0){if(*((int*)_Tmp0.f1)==0){_Tmp2=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f0)->f1;_Tmp1=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0.f1)->f1;{void*c1=_Tmp2;void*c2=_Tmp1;
# 1618
{struct _tuple15 _Tmp3=({struct _tuple15 _Tmp4;_Tmp4.f0=c1,_Tmp4.f1=c2;_Tmp4;});int _Tmp4;int _Tmp5;if(*((int*)_Tmp3.f0)==2){if(*((int*)_Tmp3.f1)==2){_Tmp5=((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_Tmp3.f0)->f1;_Tmp4=((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_Tmp3.f1)->f1;{int i1=_Tmp5;int i2=_Tmp4;
# 1620
if(i1!=0 && i1!=1)return t1;
if(i2!=0 && i2!=1)return t2;
if(i1 >= i2)return t1;
return t2;}}else{
return t1;}}else{if(*((int*)_Tmp3.f1)==2)
return t2;else{if(*((int*)_Tmp3.f0)==1){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f0)->f1==Cyc_Absyn_Unsigned){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f0)->f2==Cyc_Absyn_LongLong_sz)
goto _LLF;else{if(*((int*)_Tmp3.f1)==1){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f1)->f1==Cyc_Absyn_Unsigned){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f1)->f2==Cyc_Absyn_LongLong_sz)goto _LLE;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f0)->f2==Cyc_Absyn_Long_sz)goto _LL14;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f1)->f2==Cyc_Absyn_Long_sz)goto _LL16;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f0)->f2==Cyc_Absyn_Int_sz)goto _LL1C;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f1)->f2==Cyc_Absyn_Int_sz)goto _LL1E;else{goto _LL24;}}}}}}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f1)->f2==Cyc_Absyn_LongLong_sz)goto _LL12;else{switch((int)((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f0)->f2){case Cyc_Absyn_Long_sz: goto _LL14;case Cyc_Absyn_Int_sz: goto _LL1C;default: if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f1)->f2==Cyc_Absyn_Long_sz)goto _LL22;else{goto _LL24;}}}}}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f0)->f2==Cyc_Absyn_Long_sz){_LL14:
# 1630
 goto _LL17;}else{if(*((int*)_Tmp3.f1)==5)goto _LL1A;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f0)->f2==Cyc_Absyn_Int_sz){_LL1C: _LL1D:
# 1635
 goto _LL1F;}else{goto _LL24;}}}}}}else{if(*((int*)_Tmp3.f1)==1){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f1)->f1==Cyc_Absyn_Unsigned){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f1)->f2==Cyc_Absyn_LongLong_sz)goto _LLE;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f0)->f2==Cyc_Absyn_LongLong_sz)goto _LL10;else{switch((int)((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f1)->f2){case Cyc_Absyn_Long_sz: goto _LL16;case Cyc_Absyn_Int_sz: goto _LL1E;default: if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f0)->f2==Cyc_Absyn_Long_sz)goto _LL20;else{goto _LL24;}}}}}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f0)->f2==Cyc_Absyn_LongLong_sz)goto _LL10;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f1)->f2==Cyc_Absyn_LongLong_sz)goto _LL12;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f0)->f2==Cyc_Absyn_Long_sz)goto _LL20;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f1)->f2==Cyc_Absyn_Long_sz)goto _LL22;else{goto _LL24;}}}}}}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f0)->f2==Cyc_Absyn_LongLong_sz){_LL10:
# 1628
 goto _LL13;}else{if(*((int*)_Tmp3.f1)==5)goto _LL1A;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f0)->f2==Cyc_Absyn_Long_sz){_LL20:
# 1637
 goto _LL23;}else{goto _LL24;}}}}}}else{if(*((int*)_Tmp3.f1)==1){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f1)->f1==Cyc_Absyn_Unsigned)switch((int)((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f1)->f2){case Cyc_Absyn_LongLong_sz: _LLE: _LLF:
# 1627
 return Cyc_Absyn_ulonglong_type;case Cyc_Absyn_Long_sz: _LL16: _LL17:
# 1631
 return Cyc_Absyn_ulong_type;default: if(*((int*)_Tmp3.f0)==5)goto _LL18;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f1)->f2==Cyc_Absyn_Int_sz){_LL1E: _LL1F:
# 1636
 return Cyc_Absyn_uint_type;}else{goto _LL24;}}}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f1)->f2==Cyc_Absyn_LongLong_sz){_LL12: _LL13:
# 1629
 return Cyc_Absyn_slonglong_type;}else{if(*((int*)_Tmp3.f0)==5)goto _LL18;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_Tmp3.f1)->f2==Cyc_Absyn_Long_sz){_LL22: _LL23:
# 1638
 return Cyc_Absyn_slong_type;}else{goto _LL24;}}}}}else{if(*((int*)_Tmp3.f0)==5){_LL18:
# 1633
 goto _LL1B;}else{if(*((int*)_Tmp3.f1)==5){_LL1A: _LL1B:
 goto _LL1D;}else{_LL24:
# 1639
 goto _LL5;}}}}}}_LL5:;}
# 1641
goto _LL0;}}else{goto _LL3;}}else{_LL3:
 goto _LL0;}_LL0:;}
# 1644
return Cyc_Absyn_sint_type;}
# 1649
int Cyc_Tcutil_coerce_list(struct Cyc_List_List*aqb,void*t,struct Cyc_List_List*es){
# 1652
struct Cyc_Core_Opt*max_arith_type=0;
{struct Cyc_List_List*el=es;for(0;el!=0;el=el->tl){
void*t1=Cyc_Absyn_compress(_check_null(((struct Cyc_Absyn_Exp*)el->hd)->topt));
if(Cyc_Tcutil_is_arithmetic_type(t1)){
if(max_arith_type==0 || Cyc_Tcutil_will_lose_precision(t1,(void*)max_arith_type->v))
max_arith_type=({struct Cyc_Core_Opt*_Tmp0=_cycalloc(sizeof(struct Cyc_Core_Opt));_Tmp0->v=t1;_Tmp0;});}}}
# 1660
if(max_arith_type!=0){
if(!Cyc_Unify_unify(t,(void*)max_arith_type->v))
return 0;}
# 1664
{struct Cyc_List_List*el=es;for(0;el!=0;el=el->tl){
if(!Cyc_Tcutil_coerce_assign(aqb,(struct Cyc_Absyn_Exp*)el->hd,t)){
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp0=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp1;_Tmp1.tag=0,_Tmp1.f1=_tag_fat("type mismatch: expecting ",sizeof(char),26U);_Tmp1;});struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp1=({struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp2;_Tmp2.tag=2,_Tmp2.f1=(void*)t;_Tmp2;});struct Cyc_Warn_String_Warn_Warg_struct _Tmp2=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp3;_Tmp3.tag=0,_Tmp3.f1=_tag_fat(" but found ",sizeof(char),12U);_Tmp3;});struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp3=({struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp4;_Tmp4.tag=2,_Tmp4.f1=(void*)
_check_null(((struct Cyc_Absyn_Exp*)el->hd)->topt);_Tmp4;});void*_Tmp4[4];_Tmp4[0]=& _Tmp0,_Tmp4[1]=& _Tmp1,_Tmp4[2]=& _Tmp2,_Tmp4[3]=& _Tmp3;Cyc_Warn_err2(((struct Cyc_Absyn_Exp*)el->hd)->loc,_tag_fat(_Tmp4,sizeof(void*),4));});
return 0;}}}
# 1670
return 1;}
# 1675
int Cyc_Tcutil_coerce_to_bool(struct Cyc_Absyn_Exp*e){
if(!Cyc_Tcutil_coerce_sint_type(e)){
void*_Tmp0=Cyc_Absyn_compress(_check_null(e->topt));if(*((int*)_Tmp0)==4){
Cyc_Tcutil_unchecked_cast(e,Cyc_Absyn_uint_type,3U);goto _LL0;}else{
return 0;}_LL0:;}
# 1681
return 1;}
# 1684
static int Cyc_Tcutil_coerce_Xint_type(void*Xint_type,struct Cyc_Absyn_Exp*e){
if(Cyc_Unify_unify(e->topt,Xint_type))
return 1;
# 1688
if(Cyc_Tcutil_is_integral_type(e->topt)){
if(Cyc_Tcutil_will_lose_precision(e->topt,Xint_type))
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp0=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp1;_Tmp1.tag=0,_Tmp1.f1=_tag_fat("integral size mismatch; conversion supplied",sizeof(char),44U);_Tmp1;});void*_Tmp1[1];_Tmp1[0]=& _Tmp0;Cyc_Warn_warn2(e->loc,_tag_fat(_Tmp1,sizeof(void*),1));});
Cyc_Tcutil_unchecked_cast(e,Xint_type,1U);
return 1;}
# 1694
return 0;}
# 1697
int Cyc_Tcutil_coerce_uint_type(struct Cyc_Absyn_Exp*e){
return Cyc_Tcutil_coerce_Xint_type(Cyc_Absyn_uint_type,e);}
# 1701
int Cyc_Tcutil_coerce_sint_type(struct Cyc_Absyn_Exp*e){
return Cyc_Tcutil_coerce_Xint_type(Cyc_Absyn_sint_type,e);}
# 1707
int Cyc_Tcutil_force_type2bool(int desired,void*t){
Cyc_Unify_unify(desired?Cyc_Absyn_true_type: Cyc_Absyn_false_type,t);
return Cyc_Absyn_type2bool(desired,t);}
# 1713
void*Cyc_Tcutil_force_bounds_one(void*t){
({void*_Tmp0=t;Cyc_Unify_unify(_Tmp0,Cyc_Absyn_bounds_one());});
return Cyc_Absyn_compress(t);}
# 1718
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_thin_bound(struct Cyc_List_List*ts){
void*t=Cyc_Absyn_compress((void*)_check_null(ts)->hd);
void*_Tmp0;if(*((int*)t)==9){_Tmp0=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)t)->f1;{struct Cyc_Absyn_Exp*e=_Tmp0;
return e;}}else{
# 1723
struct Cyc_Absyn_Exp*v=Cyc_Absyn_valueof_exp(t,0U);
v->topt=Cyc_Absyn_uint_type;
return v;};}
# 1729
void*Cyc_Tcutil_get_pointer_bounds(void*ptrtype){
void*_Tmp0;if(*((int*)ptrtype)==4){_Tmp0=(struct Cyc_Absyn_PtrAtts*)&((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)ptrtype)->f1.ptr_atts;{struct Cyc_Absyn_PtrAtts*pa=(struct Cyc_Absyn_PtrAtts*)_Tmp0;
# 1732
return pa->bounds;}}else{
# 1734
return 0;};}
# 1741
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp_constrain(void*def,void*b,int c){
if(Cyc_Tcutil_is_cvar_type(b)&& c){
if(!Cyc_BansheeIf_add_constraint(0U,({void*_Tmp0=_check_null(Cyc_Tcutil_ptrbnd_cvar_equivalent(b));Cyc_BansheeIf_equality_constraint(_Tmp0,
_check_null(Cyc_Tcutil_ptrbnd_cvar_equivalent(def)));})))
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp0=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp1;_Tmp1.tag=0,_Tmp1.f1=_tag_fat("get_bounds_exp -- unable to constrain cvar ",sizeof(char),44U);_Tmp1;});struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp1=({struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp2;_Tmp2.tag=2,_Tmp2.f1=(void*)b;_Tmp2;});void*_Tmp2[2];_Tmp2[0]=& _Tmp0,_Tmp2[1]=& _Tmp1;({int(*_Tmp3)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_Tmp3;})(_tag_fat(_Tmp2,sizeof(void*),2));});}
# 1748
Cyc_Unify_unify(def,b);{
void*_Tmp0=Cyc_Absyn_compress(b);void*_Tmp1;if(*((int*)_Tmp0)==0)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1)){case 14:
 return 0;case 13: _Tmp1=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f2;{struct Cyc_List_List*ts=_Tmp1;
return Cyc_Tcutil_get_thin_bound(ts);}default: goto _LL5;}else{_LL5:
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp2=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp3;_Tmp3.tag=0,_Tmp3.f1=_tag_fat("get_bounds_exp: ",sizeof(char),17U);_Tmp3;});struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp3=({struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp4;_Tmp4.tag=2,_Tmp4.f1=(void*)b;_Tmp4;});void*_Tmp4[2];_Tmp4[0]=& _Tmp2,_Tmp4[1]=& _Tmp3;({int(*_Tmp5)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_Tmp5;})(_tag_fat(_Tmp4,sizeof(void*),2));});};}}
# 1756
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*def,void*b){
return Cyc_Tcutil_get_bounds_exp_constrain(def,b,0);}
# 1760
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_ptr_bounds_exp(void*def,void*t){
void*_Tmp0=Cyc_Absyn_compress(t);void*_Tmp1;if(*((int*)_Tmp0)==4){_Tmp1=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.bounds;{void*b=_Tmp1;
# 1763
return Cyc_Tcutil_get_bounds_exp(def,b);}}else{
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp2=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp3;_Tmp3.tag=0,_Tmp3.f1=_tag_fat("get_ptr_bounds_exp not pointer: ",sizeof(char),33U);_Tmp3;});struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp3=({struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp4;_Tmp4.tag=2,_Tmp4.f1=(void*)t;_Tmp4;});void*_Tmp4[2];_Tmp4[0]=& _Tmp2,_Tmp4[1]=& _Tmp3;({int(*_Tmp5)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_Tmp5;})(_tag_fat(_Tmp4,sizeof(void*),2));});};}
# 1769
void*Cyc_Tcutil_any_bool(struct Cyc_List_List*tvs){
return Cyc_Absyn_new_evar(& Cyc_Kinds_boolko,({struct Cyc_Core_Opt*_Tmp0=_cycalloc(sizeof(struct Cyc_Core_Opt));_Tmp0->v=tvs;_Tmp0;}));}
# 1773
void*Cyc_Tcutil_any_bounds(struct Cyc_List_List*tvs){
return Cyc_Absyn_new_evar(& Cyc_Kinds_ptrbko,({struct Cyc_Core_Opt*_Tmp0=_cycalloc(sizeof(struct Cyc_Core_Opt));_Tmp0->v=tvs;_Tmp0;}));}
# 1777
void*Cyc_Tcutil_pointer_elt_type(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);void*_Tmp1;if(*((int*)_Tmp0)==4){_Tmp1=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.elt_type;{void*e=_Tmp1;
return e;}}else{
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp2=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp3;_Tmp3.tag=0,_Tmp3.f1=_tag_fat("pointer_elt_type",sizeof(char),17U);_Tmp3;});void*_Tmp3[1];_Tmp3[0]=& _Tmp2;({int(*_Tmp4)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_Tmp4;})(_tag_fat(_Tmp3,sizeof(void*),1));});};}
# 1783
void*Cyc_Tcutil_pointer_region(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);void*_Tmp1;if(*((int*)_Tmp0)==4){_Tmp1=(struct Cyc_Absyn_PtrAtts*)&((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts;{struct Cyc_Absyn_PtrAtts*p=(struct Cyc_Absyn_PtrAtts*)_Tmp1;
return p->eff;}}else{
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp2=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp3;_Tmp3.tag=0,_Tmp3.f1=_tag_fat("pointer_elt_type",sizeof(char),17U);_Tmp3;});void*_Tmp3[1];_Tmp3[0]=& _Tmp2;({int(*_Tmp4)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_Tmp4;})(_tag_fat(_Tmp3,sizeof(void*),1));});};}
# 1790
int Cyc_Tcutil_rgn_of_pointer(void*t,void**rgn){
void*_Tmp0=Cyc_Absyn_compress(t);void*_Tmp1;if(*((int*)_Tmp0)==4){_Tmp1=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.eff;{void*r=_Tmp1;
*rgn=r;return 1;}}else{
return 0;};}
# 1797
void*Cyc_Tcutil_pointer_aqual(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);void*_Tmp1;if(*((int*)_Tmp0)==4){_Tmp1=(struct Cyc_Absyn_PtrAtts*)&((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts;{struct Cyc_Absyn_PtrAtts*p=(struct Cyc_Absyn_PtrAtts*)_Tmp1;
return p->aqual;}}else{
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp2=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp3;_Tmp3.tag=0,_Tmp3.f1=_tag_fat("pointer_elt_type",sizeof(char),17U);_Tmp3;});void*_Tmp3[1];_Tmp3[0]=& _Tmp2;({int(*_Tmp4)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_Tmp4;})(_tag_fat(_Tmp3,sizeof(void*),1));});};}
# 1806
int Cyc_Tcutil_admits_zero(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);void*_Tmp1;void*_Tmp2;switch(*((int*)_Tmp0)){case 0: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1)){case 1:
 goto _LL4;case 2: _LL4:
 return 1;default: goto _LL7;}case 4: _Tmp2=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.nullable;_Tmp1=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.bounds;{void*nullable=_Tmp2;void*bounds=_Tmp1;
# 1814
return !Cyc_Unify_unify(Cyc_Absyn_fat_bound_type,bounds)&& Cyc_Tcutil_force_type2bool(0,nullable);}default: _LL7:
 return 0;};}
# 1820
int Cyc_Tcutil_is_zero(struct Cyc_Absyn_Exp*e){
void*_Tmp0=e->r;void*_Tmp1;void*_Tmp2;struct _fat_ptr _Tmp3;switch(*((int*)_Tmp0)){case 0: switch(((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_Tmp0)->f1.Wchar_c.tag){case 5: if(((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_Tmp0)->f1.Int_c.val.f1==0)
goto _LL4;else{goto _LLF;}case 2: if(((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_Tmp0)->f1.Char_c.val.f1==0){_LL4:
 goto _LL6;}else{goto _LLF;}case 4: if(((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_Tmp0)->f1.Short_c.val.f1==0){_LL6:
 goto _LL8;}else{goto _LLF;}case 6: if(((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_Tmp0)->f1.LongLong_c.val.f1==0){_LL8:
 goto _LLA;}else{goto _LLF;}case 3: _Tmp3=((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_Tmp0)->f1.Wchar_c.val;{struct _fat_ptr s=_Tmp3;
# 1828
unsigned long l=Cyc_strlen(s);
int i=0;
if(l >= 2U &&(int)((const char*)s.curr)[0]==92){
if((int)((const char*)s.curr)[1]==48)i=2;else{
if(((int)((const char*)s.curr)[1]==120 && l >= 3U)&&(int)((const char*)s.curr)[2]==48)i=3;else{
return 0;}}
for(1;(unsigned long)i < l;++ i){
if((int)((const char*)s.curr)[i]!=48)return 0;}
return 1;}
# 1838
return 0;}default: goto _LLF;}case 2: _LLA:
# 1826
 return 1;case 14: _Tmp2=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp1=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{void*t=_Tmp2;struct Cyc_Absyn_Exp*e2=_Tmp1;
# 1839
return Cyc_Tcutil_is_zero(e2)&& Cyc_Tcutil_admits_zero(t);}default: _LLF:
 return 0;};}
# 1846
int Cyc_Tcutil_zero_to_null(void*t2,struct Cyc_Absyn_Exp*e1){
if(!Cyc_Tcutil_is_zero(e1))
return 0;{
void*_Tmp0=Cyc_Absyn_compress(t2);void*_Tmp1;if(*((int*)_Tmp0)==4){_Tmp1=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.nullable;{void*nbl=_Tmp1;
# 1851
if(!Cyc_Tcutil_force_type2bool(1,nbl))
return 0;
({void*_Tmp2=Cyc_Absyn_null_exp(0U)->r;e1->r=_Tmp2;});
e1->topt=t2;
return 1;}}else{
return 0;};}}
# 1860
int Cyc_Tcutil_is_aliasable_qual(void*aq){
void*_Tmp0=Cyc_Absyn_compress(aq);void*_Tmp1;enum Cyc_Absyn_AliasQualVal _Tmp2;if(*((int*)_Tmp0)==0)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1)){case 16: _Tmp2=((struct Cyc_Absyn_AqualConstCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1)->f1;{enum Cyc_Absyn_AliasQualVal aqv=_Tmp2;
# 1863
return(int)aqv==0;}case 17: _Tmp1=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f2;{struct Cyc_List_List*tv_bnd=_Tmp1;
# 1865
return Cyc_Tcutil_is_aliasable_qual((void*)_check_null(_check_null(tv_bnd)->tl)->hd);}default: goto _LL5;}else{_LL5:
# 1867
 return 0;};}struct _tuple21{struct Cyc_Absyn_Decl*f0;struct Cyc_Absyn_Exp*f1;int f2;};
# 1876
struct _tuple21 Cyc_Tcutil_insert_alias(struct Cyc_Absyn_Exp*e,void*e_type){
static struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct eff_kb={0,& Cyc_Kinds_ek};
int success=1;
# 1880
static int counter=0;
struct _tuple1*v;v=_cycalloc(sizeof(struct _tuple1)),v->f0=Cyc_Absyn_Loc_n,({struct _fat_ptr*_Tmp0=({struct _fat_ptr*_Tmp1=_cycalloc(sizeof(struct _fat_ptr));({struct _fat_ptr _Tmp2=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _Tmp3=({struct Cyc_Int_pa_PrintArg_struct _Tmp4;_Tmp4.tag=1,_Tmp4.f1=(unsigned long)counter ++;_Tmp4;});void*_Tmp4[1];_Tmp4[0]=& _Tmp3;Cyc_aprintf(_tag_fat("__aliasvar%d",sizeof(char),13U),_tag_fat(_Tmp4,sizeof(void*),1));});*_Tmp1=_Tmp2;});_Tmp1;});v->f1=_Tmp0;});{
struct Cyc_Absyn_Vardecl*vd=Cyc_Absyn_new_vardecl(0U,v,e_type,e,0);
struct Cyc_Absyn_Exp*ve=({void*_Tmp0=(void*)({struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_Tmp1=_cycalloc(sizeof(struct Cyc_Absyn_Local_b_Absyn_Binding_struct));_Tmp1->tag=4,_Tmp1->f1=vd;_Tmp1;});Cyc_Absyn_varb_exp(_Tmp0,e->loc);});
# 1889
struct Cyc_Absyn_Tvar*tv=Cyc_Tcutil_new_tvar((void*)& eff_kb);
{void*_Tmp0=Cyc_Absyn_compress(e_type);void*_Tmp1;void*_Tmp2;struct Cyc_Absyn_Tqual _Tmp3;void*_Tmp4;if(*((int*)_Tmp0)==4){_Tmp4=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.elt_type;_Tmp3=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.elt_tq;_Tmp2=(void**)&((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.eff;_Tmp1=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.aqual;{void*et=_Tmp4;struct Cyc_Absyn_Tqual tq=_Tmp3;void**old_r=(void**)_Tmp2;void*aq=_Tmp1;
# 1892
{void*_Tmp5=Cyc_Absyn_compress(*old_r);void*_Tmp6;void*_Tmp7;if(*((int*)_Tmp5)==1){_Tmp7=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_Tmp5)->f2;_Tmp6=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_Tmp5)->f4;{void**topt=(void**)_Tmp7;struct Cyc_Core_Opt*ts=_Tmp6;
# 1894
void*new_r=Cyc_Absyn_var_type(tv);
*topt=new_r;
goto _LL5;}}else{
# 1912 "tcutil.cyc"
({void*_Tmp8=Cyc_Absyn_var_type(tv);*old_r=_Tmp8;});
goto _LL5;}_LL5:;}
# 1917
if(!Cyc_Tcutil_is_aliasable_qual(aq)){
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp5=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp6;_Tmp6.tag=0,_Tmp6.f1=_tag_fat("Automatic alias coercion of restricted pointers not supported",sizeof(char),62U);_Tmp6;});void*_Tmp6[1];_Tmp6[0]=& _Tmp5;Cyc_Warn_err2(e->loc,_tag_fat(_Tmp6,sizeof(void*),1));});
success=0;}
# 1921
goto _LL0;}}else{
goto _LL0;}_LL0:;}
# 1924
e->topt=0;
vd->initializer=0;{
# 1928
struct Cyc_Absyn_Decl*d=Cyc_Absyn_alias_decl(tv,vd,e,e->loc);
# 1930
struct _tuple21 _Tmp0;_Tmp0.f0=d,_Tmp0.f1=ve,_Tmp0.f2=success;return _Tmp0;}}}
# 1935
static int Cyc_Tcutil_can_insert_alias(struct Cyc_List_List*aqb,struct Cyc_Absyn_Exp*e,void*e_type,void*wants_type,unsigned loc){
# 1938
if((Cyc_Tcutil_is_noalias_path(aqb,e)&&
 Cyc_Tcutil_is_noalias_pointer(aqb,e_type,0))&&
 Cyc_Tcutil_is_pointer_type(e_type)){
# 1943
void*_Tmp0=Cyc_Absyn_compress(wants_type);void*_Tmp1;void*_Tmp2;if(*((int*)_Tmp0)==4){_Tmp2=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.eff;_Tmp1=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.aqual;{void*r2=_Tmp2;void*aq=_Tmp1;
# 1945
if(Cyc_Tcutil_is_heap_rgn_type(r2))return 0;{
struct Cyc_Absyn_Kind*k=Cyc_Tcutil_type_kind(r2);
return(int)k->kind==3 &&
 Cyc_Tcutil_is_aliasable_qual(aq);}}}else{
return 0;};}
# 1952
return 0;}
# 1956
int Cyc_Tcutil_coerce_arg(struct Cyc_List_List*aqb,struct Cyc_Absyn_Exp*e,void*t2,int*alias_coercion){
# 1959
void*t1=Cyc_Absyn_compress(_check_null(e->topt));
# 1962
if((Cyc_Tcutil_is_arithmetic_type(t2)&& Cyc_Tcutil_is_arithmetic_type(t1))&& !Cyc_Tcutil_is_tag_type(t2)){
# 1964
if(Cyc_Tcutil_will_lose_precision(t1,t2))
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp0=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp1;_Tmp1.tag=0,_Tmp1.f1=_tag_fat("integral size mismatch; ",sizeof(char),25U);_Tmp1;});struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp1=({struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp2;_Tmp2.tag=2,_Tmp2.f1=(void*)t1;_Tmp2;});struct Cyc_Warn_String_Warn_Warg_struct _Tmp2=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp3;_Tmp3.tag=0,_Tmp3.f1=
_tag_fat(" -> ",sizeof(char),5U);_Tmp3;});struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp3=({struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp4;_Tmp4.tag=2,_Tmp4.f1=(void*)t2;_Tmp4;});struct Cyc_Warn_String_Warn_Warg_struct _Tmp4=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp5;_Tmp5.tag=0,_Tmp5.f1=_tag_fat(" conversion supplied",sizeof(char),21U);_Tmp5;});void*_Tmp5[5];_Tmp5[0]=& _Tmp0,_Tmp5[1]=& _Tmp1,_Tmp5[2]=& _Tmp2,_Tmp5[3]=& _Tmp3,_Tmp5[4]=& _Tmp4;Cyc_Warn_warn2(e->loc,_tag_fat(_Tmp5,sizeof(void*),5));});
Cyc_Tcutil_unchecked_cast(e,t2,1U);
return 1;}
# 1971
if(Cyc_Tcutil_zero_to_null(t2,e))
return 1;{
# 1974
int do_alias_coercion=0;
# 1976
if((unsigned)alias_coercion)
# 1978
do_alias_coercion=Cyc_Tcutil_can_insert_alias(aqb,e,t1,t2,e->loc);{
# 1981
enum Cyc_Absyn_Coercion c=Cyc_Subtype_coercible_warn(e->loc,t1,t2);
# 1983
if((int)c!=0){
if((int)c!=1){
if((unsigned)alias_coercion)
*alias_coercion=do_alias_coercion;
Cyc_Tcutil_unchecked_cast(e,t2,c);}
# 1990
if((Cyc_Flags_warn_alias_coerce &&(unsigned)alias_coercion)&&*alias_coercion)
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp0=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp1;_Tmp1.tag=0,_Tmp1.f1=_tag_fat("implicit alias coercion for ",sizeof(char),29U);_Tmp1;});struct Cyc_Warn_Exp_Warn_Warg_struct _Tmp1=({struct Cyc_Warn_Exp_Warn_Warg_struct _Tmp2;_Tmp2.tag=4,_Tmp2.f1=e;_Tmp2;});struct Cyc_Warn_String_Warn_Warg_struct _Tmp2=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp3;_Tmp3.tag=0,_Tmp3.f1=_tag_fat(":",sizeof(char),2U);_Tmp3;});struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp3=({struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp4;_Tmp4.tag=2,_Tmp4.f1=(void*)t1;_Tmp4;});struct Cyc_Warn_String_Warn_Warg_struct _Tmp4=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp5;_Tmp5.tag=0,_Tmp5.f1=_tag_fat(" to ",sizeof(char),5U);_Tmp5;});struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp5=({struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp6;_Tmp6.tag=2,_Tmp6.f1=(void*)t2;_Tmp6;});void*_Tmp6[6];_Tmp6[0]=& _Tmp0,_Tmp6[1]=& _Tmp1,_Tmp6[2]=& _Tmp2,_Tmp6[3]=& _Tmp3,_Tmp6[4]=& _Tmp4,_Tmp6[5]=& _Tmp5;Cyc_Warn_warn2(e->loc,_tag_fat(_Tmp6,sizeof(void*),6));});
return 1;}
# 1994
return 0;}}}
# 2000
int Cyc_Tcutil_coerce_assign(struct Cyc_List_List*aqb,struct Cyc_Absyn_Exp*e,void*t){
# 2003
int bogus=0;
return Cyc_Tcutil_coerce_arg(aqb,e,t,& bogus);}
# 2008
void*Cyc_Tcutil_lookup_aquals_opt(struct Cyc_List_List*aquals_map,void*t){
void*aqt=t;
{void*_Tmp0;if(*((int*)t)==2){_Tmp0=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)t)->f1;{struct Cyc_Absyn_Tvar*tv=_Tmp0;
# 2012
struct Cyc_Core_Opt _Tmp1=Cyc_Kinds_tvar_kind_opt(tv);void*_Tmp2;_Tmp2=(struct Cyc_Absyn_Kind*)_Tmp1.v;{struct Cyc_Absyn_Kind*k=_Tmp2;
if((unsigned)k &&(int)k->kind!=7)
aqt=Cyc_Absyn_aqualsof_type(t);
# 2016
goto _LL0;}}}else{
# 2018
goto _LL0;}_LL0:;}{
# 2020
struct _handler_cons _Tmp0;_push_handler(& _Tmp0);{int _Tmp1=0;if(setjmp(_Tmp0.handler))_Tmp1=1;if(!_Tmp1){
{void*ret=Cyc_List_assoc_cmp(Cyc_Tcutil_typecmp,aquals_map,aqt);
void*_Tmp2=ret;_npop_handler(0);return _Tmp2;}
# 2021
;_pop_handler();}else{void*_Tmp2=(void*)Cyc_Core_get_exn_thrown();void*_Tmp3;if(((struct Cyc_Core_Not_found_exn_struct*)_Tmp2)->tag==Cyc_Core_Not_found)
# 2025
return 0;else{_Tmp3=_Tmp2;{void*exn=_Tmp3;_rethrow(exn);}};}}}}
# 2029
void*Cyc_Tcutil_lookup_aquals(struct Cyc_List_List*aquals_map,void*t){
void*aq=Cyc_Tcutil_lookup_aquals_opt(aquals_map,t);
if(aq==0)return Cyc_Absyn_al_qual_type;else{return aq;}}
# 2035
void Cyc_Tcutil_unchecked_cast(struct Cyc_Absyn_Exp*e,void*t,enum Cyc_Absyn_Coercion c){
if(Cyc_Unify_unify(_check_null(e->topt),t))
return;{
struct Cyc_Absyn_Exp*inner=Cyc_Absyn_copy_exp(e);
({void*_Tmp0=(void*)({struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_Tmp1=_cycalloc(sizeof(struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct));_Tmp1->tag=14,_Tmp1->f1=t,_Tmp1->f2=inner,_Tmp1->f3=0,_Tmp1->f4=c;_Tmp1;});e->r=_Tmp0;});
e->topt=t;}}
# 2044
static int Cyc_Tcutil_tvar_id_counter=0;
int Cyc_Tcutil_new_tvar_id (void){
return Cyc_Tcutil_tvar_id_counter ++;}
# 2049
static int Cyc_Tcutil_tvar_counter=0;
struct Cyc_Absyn_Tvar*Cyc_Tcutil_new_tvar(void*k){
struct Cyc_Absyn_Tvar*_Tmp0=_cycalloc(sizeof(struct Cyc_Absyn_Tvar));({struct _fat_ptr*_Tmp1=({struct _fat_ptr*_Tmp2=_cycalloc(sizeof(struct _fat_ptr));({struct _fat_ptr _Tmp3=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _Tmp4=({struct Cyc_Int_pa_PrintArg_struct _Tmp5;_Tmp5.tag=1,_Tmp5.f1=(unsigned long)Cyc_Tcutil_tvar_counter ++;_Tmp5;});void*_Tmp5[1];_Tmp5[0]=& _Tmp4;Cyc_aprintf(_tag_fat("#%d",sizeof(char),4U),_tag_fat(_Tmp5,sizeof(void*),1));});*_Tmp2=_Tmp3;});_Tmp2;});_Tmp0->name=_Tmp1;}),_Tmp0->identity=-1,_Tmp0->kind=k,_Tmp0->aquals_bound=0;return _Tmp0;}
# 2053
int Cyc_Tcutil_is_temp_tvar(struct Cyc_Absyn_Tvar*t){
return(int)*((const char*)_check_fat_subscript(t->name[0],sizeof(char),0U))==35;}
# 2058
void*Cyc_Tcutil_fndecl2type(struct Cyc_Absyn_Fndecl*fd){
if(fd->cached_type==0){
# 2065
struct Cyc_List_List*fn_type_atts=0;
{struct Cyc_List_List*atts=fd->i.attributes;for(0;atts!=0;atts=atts->tl){
if(Cyc_Atts_fntype_att((void*)atts->hd))
fn_type_atts=({struct Cyc_List_List*_Tmp0=_cycalloc(sizeof(struct Cyc_List_List));_Tmp0->hd=(void*)atts->hd,_Tmp0->tl=fn_type_atts;_Tmp0;});}}{
struct Cyc_Absyn_FnInfo type_info=fd->i;
type_info.attributes=fn_type_atts;
return(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_Tmp0=_cycalloc(sizeof(struct Cyc_Absyn_FnType_Absyn_Type_struct));_Tmp0->tag=6,_Tmp0->f1=type_info;_Tmp0;});}}
# 2073
return fd->cached_type;}
# 2079
static void Cyc_Tcutil_replace_rop(struct Cyc_List_List*args,union Cyc_Relations_RelnOp*rop){
# 2081
union Cyc_Relations_RelnOp _Tmp0=*rop;void*_Tmp1;switch(_Tmp0.RNumelts.tag){case 2: _Tmp1=_Tmp0.RVar.val;{struct Cyc_Absyn_Vardecl*vd=_Tmp1;
# 2083
struct _tuple1 _Tmp2=*vd->name;void*_Tmp3;union Cyc_Absyn_Nmspace _Tmp4;_Tmp4=_Tmp2.f0;_Tmp3=_Tmp2.f1;{union Cyc_Absyn_Nmspace nmspace=_Tmp4;struct _fat_ptr*var=_Tmp3;
if(!((int)(nmspace.Loc_n.tag==4)))goto _LL0;
if(Cyc_strcmp(*var,_tag_fat("return_value",sizeof(char),13U))==0){
({union Cyc_Relations_RelnOp _Tmp5=Cyc_Relations_RReturn();*rop=_Tmp5;});
goto _LL0;}{
# 2089
unsigned c=0U;
{struct Cyc_List_List*a=args;for(0;a!=0;(a=a->tl,c ++)){
struct _tuple9*_Tmp5=(struct _tuple9*)a->hd;void*_Tmp6;_Tmp6=_Tmp5->f0;{struct _fat_ptr*vopt=_Tmp6;
if(vopt!=0 && Cyc_strcmp(*var,*vopt)==0){
({union Cyc_Relations_RelnOp _Tmp7=Cyc_Relations_RParam(c);*rop=_Tmp7;});
break;}}}}
# 2097
goto _LL0;}}}case 3: _Tmp1=_Tmp0.RNumelts.val;{struct Cyc_Absyn_Vardecl*vd=_Tmp1;
# 2099
struct _tuple1 _Tmp2=*vd->name;void*_Tmp3;union Cyc_Absyn_Nmspace _Tmp4;_Tmp4=_Tmp2.f0;_Tmp3=_Tmp2.f1;{union Cyc_Absyn_Nmspace nmspace=_Tmp4;struct _fat_ptr*var=_Tmp3;
if(!((int)(nmspace.Loc_n.tag==4)))goto _LL0;{
unsigned c=0U;
{struct Cyc_List_List*a=args;for(0;a!=0;(a=a->tl,c ++)){
struct _tuple9*_Tmp5=(struct _tuple9*)a->hd;void*_Tmp6;_Tmp6=_Tmp5->f0;{struct _fat_ptr*vopt=_Tmp6;
if(vopt!=0 && Cyc_strcmp(*var,*vopt)==0){
({union Cyc_Relations_RelnOp _Tmp7=Cyc_Relations_RParamNumelts(c);*rop=_Tmp7;});
break;}}}}
# 2109
goto _LL0;}}}default:
 goto _LL0;}_LL0:;}
# 2114
void Cyc_Tcutil_replace_rops(struct Cyc_List_List*args,struct Cyc_Relations_Reln*r){
# 2116
Cyc_Tcutil_replace_rop(args,& r->rop1);
Cyc_Tcutil_replace_rop(args,& r->rop2);}
# 2120
static struct Cyc_List_List*Cyc_Tcutil_extract_relns(struct Cyc_List_List*args,struct Cyc_Absyn_Exp*e){
# 2122
if(e==0)return 0;{
struct Cyc_List_List*relns=Cyc_Relations_exp2relns(Cyc_Core_heap_region,e);
({void(*_Tmp0)(void(*)(struct Cyc_List_List*,struct Cyc_Relations_Reln*),struct Cyc_List_List*,struct Cyc_List_List*)=(void(*)(void(*)(struct Cyc_List_List*,struct Cyc_Relations_Reln*),struct Cyc_List_List*,struct Cyc_List_List*))Cyc_List_iter_c;_Tmp0;})(Cyc_Tcutil_replace_rops,args,relns);
return relns;}}
# 2129
static struct _tuple14*Cyc_Tcutil_map2_tq(struct _tuple14*pr,void*t){
void*_Tmp0;struct Cyc_Absyn_Tqual _Tmp1;_Tmp1=pr->f0;_Tmp0=pr->f1;{struct Cyc_Absyn_Tqual tq=_Tmp1;void*t2=_Tmp0;
if(t2==t)return pr;else{struct _tuple14*_Tmp2=_cycalloc(sizeof(struct _tuple14));_Tmp2->f0=tq,_Tmp2->f1=t;return _Tmp2;}}}struct _tuple22{struct _fat_ptr*f0;struct Cyc_Absyn_Tqual f1;};struct _tuple23{struct _tuple22*f0;void*f1;};
# 2133
static struct _tuple23*Cyc_Tcutil_substitute_f1(struct _RegionHandle*rgn,struct _tuple9*y){
# 2135
struct _tuple23*_Tmp0=_region_malloc(rgn,0U,sizeof(struct _tuple23));({struct _tuple22*_Tmp1=({struct _tuple22*_Tmp2=_region_malloc(rgn,0U,sizeof(struct _tuple22));_Tmp2->f0=(*y).f0,_Tmp2->f1=(*y).f1;_Tmp2;});_Tmp0->f0=_Tmp1;}),_Tmp0->f1=(*y).f2;return _Tmp0;}
# 2137
static struct _tuple9*Cyc_Tcutil_substitute_f2(struct _tuple9*orig_arg,void*t){
# 2139
struct _tuple9 _Tmp0=*orig_arg;void*_Tmp1;struct Cyc_Absyn_Tqual _Tmp2;void*_Tmp3;_Tmp3=_Tmp0.f0;_Tmp2=_Tmp0.f1;_Tmp1=_Tmp0.f2;{struct _fat_ptr*vopt_orig=_Tmp3;struct Cyc_Absyn_Tqual tq_orig=_Tmp2;void*t_orig=_Tmp1;
if(t==t_orig)return orig_arg;{
struct _tuple9*_Tmp4=_cycalloc(sizeof(struct _tuple9));_Tmp4->f0=vopt_orig,_Tmp4->f1=tq_orig,_Tmp4->f2=t;return _Tmp4;}}}
# 2143
static struct Cyc_List_List*Cyc_Tcutil_substs(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_List_List*);
# 2145
static struct Cyc_List_List*Cyc_Tcutil_substs_exps(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_List_List*);
# 2149
static struct Cyc_Absyn_Exp*Cyc_Tcutil_copye(struct Cyc_Absyn_Exp*old,void*r){
# 2151
struct Cyc_Absyn_Exp*_Tmp0=_cycalloc(sizeof(struct Cyc_Absyn_Exp));_Tmp0->topt=old->topt,_Tmp0->r=r,_Tmp0->loc=old->loc,_Tmp0->annot=old->annot;return _Tmp0;}
# 2156
struct Cyc_Absyn_Exp*Cyc_Tcutil_rsubsexp(struct _RegionHandle*r,struct Cyc_List_List*inst,struct Cyc_Absyn_Exp*e){
void*_Tmp0=e->r;int _Tmp1;enum Cyc_Absyn_Coercion _Tmp2;int _Tmp3;void*_Tmp4;void*_Tmp5;void*_Tmp6;enum Cyc_Absyn_Primop _Tmp7;switch(*((int*)_Tmp0)){case 0:
 goto _LL4;case 31: _LL4:
 goto _LL6;case 32: _LL6:
 goto _LL8;case 2: _LL8:
 goto _LLA;case 1: _LLA:
 return e;case 3: _Tmp7=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp6=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{enum Cyc_Absyn_Primop p=_Tmp7;struct Cyc_List_List*es=_Tmp6;
# 2165
if(Cyc_List_length(es)==1){
struct Cyc_Absyn_Exp*e1=(struct Cyc_Absyn_Exp*)_check_null(es)->hd;
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1==e1)return e;else{struct Cyc_Absyn_Exp*_Tmp8=e;return Cyc_Tcutil_copye(_Tmp8,(void*)({struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*_Tmp9=_cycalloc(sizeof(struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct));_Tmp9->tag=3,_Tmp9->f1=p,({struct Cyc_List_List*_TmpA=({struct Cyc_Absyn_Exp*_TmpB[1];_TmpB[0]=new_e1;Cyc_List_list(_tag_fat(_TmpB,sizeof(struct Cyc_Absyn_Exp*),1));});_Tmp9->f2=_TmpA;});_Tmp9;}));}}else{
if(Cyc_List_length(es)==2){
struct Cyc_Absyn_Exp*e1=(struct Cyc_Absyn_Exp*)_check_null(es)->hd;
struct Cyc_Absyn_Exp*e2=(struct Cyc_Absyn_Exp*)_check_null(es->tl)->hd;
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
struct Cyc_Absyn_Exp*new_e2=Cyc_Tcutil_rsubsexp(r,inst,e2);
if(new_e1==e1 && new_e2==e2)return e;{
struct Cyc_Absyn_Exp*_Tmp8=e;return Cyc_Tcutil_copye(_Tmp8,(void*)({struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*_Tmp9=_cycalloc(sizeof(struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct));_Tmp9->tag=3,_Tmp9->f1=p,({struct Cyc_List_List*_TmpA=({struct Cyc_Absyn_Exp*_TmpB[2];_TmpB[0]=new_e1,_TmpB[1]=new_e2;Cyc_List_list(_tag_fat(_TmpB,sizeof(struct Cyc_Absyn_Exp*),2));});_Tmp9->f2=_TmpA;});_Tmp9;}));}}}{
# 2177
struct Cyc_Warn_String_Warn_Warg_struct _Tmp8=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp9;_Tmp9.tag=0,_Tmp9.f1=_tag_fat("primop does not have 1 or 2 args!",sizeof(char),34U);_Tmp9;});void*_Tmp9[1];_Tmp9[0]=& _Tmp8;return({struct Cyc_Absyn_Exp*(*_TmpA)(struct _fat_ptr)=(struct Cyc_Absyn_Exp*(*)(struct _fat_ptr))Cyc_Warn_impos2;_TmpA;})(_tag_fat(_Tmp9,sizeof(void*),1));}}case 6: _Tmp6=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp5=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_Tmp4=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;{struct Cyc_Absyn_Exp*e1=_Tmp6;struct Cyc_Absyn_Exp*e2=_Tmp5;struct Cyc_Absyn_Exp*e3=_Tmp4;
# 2179
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
struct Cyc_Absyn_Exp*new_e2=Cyc_Tcutil_rsubsexp(r,inst,e2);
struct Cyc_Absyn_Exp*new_e3=Cyc_Tcutil_rsubsexp(r,inst,e3);
if((new_e1==e1 && new_e2==e2)&& new_e3==e3)return e;{
struct Cyc_Absyn_Exp*_Tmp8=e;return Cyc_Tcutil_copye(_Tmp8,(void*)({struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*_Tmp9=_cycalloc(sizeof(struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct));_Tmp9->tag=6,_Tmp9->f1=new_e1,_Tmp9->f2=new_e2,_Tmp9->f3=new_e3;_Tmp9;}));}}case 7: _Tmp6=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp5=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_Absyn_Exp*e1=_Tmp6;struct Cyc_Absyn_Exp*e2=_Tmp5;
# 2185
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
struct Cyc_Absyn_Exp*new_e2=Cyc_Tcutil_rsubsexp(r,inst,e2);
if(new_e1==e1 && new_e2==e2)return e;{
struct Cyc_Absyn_Exp*_Tmp8=e;return Cyc_Tcutil_copye(_Tmp8,(void*)({struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*_Tmp9=_cycalloc(sizeof(struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct));_Tmp9->tag=7,_Tmp9->f1=new_e1,_Tmp9->f2=new_e2;_Tmp9;}));}}case 8: _Tmp6=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp5=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_Absyn_Exp*e1=_Tmp6;struct Cyc_Absyn_Exp*e2=_Tmp5;
# 2190
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
struct Cyc_Absyn_Exp*new_e2=Cyc_Tcutil_rsubsexp(r,inst,e2);
if(new_e1==e1 && new_e2==e2)return e;{
struct Cyc_Absyn_Exp*_Tmp8=e;return Cyc_Tcutil_copye(_Tmp8,(void*)({struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*_Tmp9=_cycalloc(sizeof(struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct));_Tmp9->tag=8,_Tmp9->f1=new_e1,_Tmp9->f2=new_e2;_Tmp9;}));}}case 9: _Tmp6=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp5=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_Absyn_Exp*e1=_Tmp6;struct Cyc_Absyn_Exp*e2=_Tmp5;
# 2195
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
struct Cyc_Absyn_Exp*new_e2=Cyc_Tcutil_rsubsexp(r,inst,e2);
if(new_e1==e1 && new_e2==e2)return e;{
struct Cyc_Absyn_Exp*_Tmp8=e;return Cyc_Tcutil_copye(_Tmp8,(void*)({struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*_Tmp9=_cycalloc(sizeof(struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct));_Tmp9->tag=9,_Tmp9->f1=new_e1,_Tmp9->f2=new_e2;_Tmp9;}));}}case 10: _Tmp6=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp5=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_Tmp4=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;_Tmp3=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_Tmp0)->f4;{struct Cyc_Absyn_Exp*e1=_Tmp6;struct Cyc_List_List*args=_Tmp5;struct Cyc_Absyn_VarargCallInfo*vai=_Tmp4;int resolved=_Tmp3;
# 2200
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
struct Cyc_List_List*new_args=Cyc_Tcutil_substs_exps(r,inst,args);
# 2203
if(new_e1==e1 && new_args==args)return e;{
struct Cyc_Absyn_Exp*_Tmp8=e;return Cyc_Tcutil_copye(_Tmp8,(void*)({struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_Tmp9=_cycalloc(sizeof(struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct));_Tmp9->tag=10,_Tmp9->f1=new_e1,_Tmp9->f2=new_args,_Tmp9->f3=vai,_Tmp9->f4=resolved;_Tmp9;}));}}case 12: _Tmp6=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e1=_Tmp6;
# 2206
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1==e1)return e;{
struct Cyc_Absyn_Exp*_Tmp8=e;return Cyc_Tcutil_copye(_Tmp8,(void*)({struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*_Tmp9=_cycalloc(sizeof(struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct));_Tmp9->tag=12,_Tmp9->f1=new_e1;_Tmp9;}));}}case 13: _Tmp6=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp5=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_Absyn_Exp*e1=_Tmp6;struct Cyc_List_List*ts=_Tmp5;
# 2210
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
struct Cyc_List_List*new_ts=Cyc_Tcutil_substs(r,inst,ts);
if(new_e1==e1 && new_ts==ts)return e;{
struct Cyc_Absyn_Exp*_Tmp8=e;return Cyc_Tcutil_copye(_Tmp8,(void*)({struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*_Tmp9=_cycalloc(sizeof(struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct));_Tmp9->tag=13,_Tmp9->f1=e1,_Tmp9->f2=ts;_Tmp9;}));}}case 14: _Tmp6=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp5=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_Tmp3=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;_Tmp2=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_Tmp0)->f4;{void*t=_Tmp6;struct Cyc_Absyn_Exp*e1=_Tmp5;int b=_Tmp3;enum Cyc_Absyn_Coercion c=_Tmp2;
# 2215
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
void*new_typ=Cyc_Tcutil_rsubstitute(r,inst,t);
if(new_e1==e1 && new_typ==t)return e;{
struct Cyc_Absyn_Exp*_Tmp8=e;return Cyc_Tcutil_copye(_Tmp8,(void*)({struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_Tmp9=_cycalloc(sizeof(struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct));_Tmp9->tag=14,_Tmp9->f1=new_typ,_Tmp9->f2=new_e1,_Tmp9->f3=b,_Tmp9->f4=c;_Tmp9;}));}}case 15: _Tmp6=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e1=_Tmp6;
# 2220
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1==e1)return e;{
struct Cyc_Absyn_Exp*_Tmp8=e;return Cyc_Tcutil_copye(_Tmp8,(void*)({struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*_Tmp9=_cycalloc(sizeof(struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct));_Tmp9->tag=15,_Tmp9->f1=new_e1;_Tmp9;}));}}case 20: _Tmp6=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e1=_Tmp6;
# 2224
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1==e1)return e;{
struct Cyc_Absyn_Exp*_Tmp8=e;return Cyc_Tcutil_copye(_Tmp8,(void*)({struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*_Tmp9=_cycalloc(sizeof(struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct));_Tmp9->tag=20,_Tmp9->f1=new_e1;_Tmp9;}));}}case 21: _Tmp6=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp5=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_Tmp3=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;_Tmp1=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_Tmp0)->f4;{struct Cyc_Absyn_Exp*e1=_Tmp6;struct _fat_ptr*f=_Tmp5;int it=_Tmp3;int ir=_Tmp1;
# 2228
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1==e1)return e;{
struct Cyc_Absyn_Exp*_Tmp8=e;return Cyc_Tcutil_copye(_Tmp8,(void*)({struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_Tmp9=_cycalloc(sizeof(struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct));_Tmp9->tag=21,_Tmp9->f1=new_e1,_Tmp9->f2=f,_Tmp9->f3=it,_Tmp9->f4=ir;_Tmp9;}));}}case 22: _Tmp6=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp5=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_Tmp3=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;_Tmp1=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_Tmp0)->f4;{struct Cyc_Absyn_Exp*e1=_Tmp6;struct _fat_ptr*f=_Tmp5;int it=_Tmp3;int ir=_Tmp1;
# 2232
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1==e1)return e;{
struct Cyc_Absyn_Exp*_Tmp8=e;return Cyc_Tcutil_copye(_Tmp8,(void*)({struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_Tmp9=_cycalloc(sizeof(struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct));_Tmp9->tag=22,_Tmp9->f1=new_e1,_Tmp9->f2=f,_Tmp9->f3=it,_Tmp9->f4=ir;_Tmp9;}));}}case 23: _Tmp6=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp5=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_Absyn_Exp*e1=_Tmp6;struct Cyc_Absyn_Exp*e2=_Tmp5;
# 2236
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
struct Cyc_Absyn_Exp*new_e2=Cyc_Tcutil_rsubsexp(r,inst,e2);
if(new_e1==e1 && new_e2==e2)return e;{
struct Cyc_Absyn_Exp*_Tmp8=e;return Cyc_Tcutil_copye(_Tmp8,(void*)({struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*_Tmp9=_cycalloc(sizeof(struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct));_Tmp9->tag=23,_Tmp9->f1=new_e1,_Tmp9->f2=new_e2;_Tmp9;}));}}case 37: _Tmp6=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp5=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_Absyn_Exp*e1=_Tmp6;struct _fat_ptr*f=_Tmp5;
# 2241
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1==e1)return e;{
struct Cyc_Absyn_Exp*_Tmp8=e;return Cyc_Tcutil_copye(_Tmp8,(void*)({struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*_Tmp9=_cycalloc(sizeof(struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct));_Tmp9->tag=37,_Tmp9->f1=e1,_Tmp9->f2=f;_Tmp9;}));}}case 17: _Tmp6=(void*)((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{void*t=_Tmp6;
# 2245
void*new_typ=Cyc_Tcutil_rsubstitute(r,inst,t);
if(new_typ==t)return e;else{struct Cyc_Absyn_Exp*_Tmp8=e;return Cyc_Tcutil_copye(_Tmp8,(void*)({struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*_Tmp9=_cycalloc(sizeof(struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct));_Tmp9->tag=17,_Tmp9->f1=new_typ;_Tmp9;}));}}case 18: _Tmp6=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e1=_Tmp6;
# 2248
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1==e1)return e;else{struct Cyc_Absyn_Exp*_Tmp8=e;return Cyc_Tcutil_copye(_Tmp8,(void*)({struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*_Tmp9=_cycalloc(sizeof(struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct));_Tmp9->tag=18,_Tmp9->f1=new_e1;_Tmp9;}));}}case 40: _Tmp6=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e1=_Tmp6;
# 2251
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1==e1)return e;else{struct Cyc_Absyn_Exp*_Tmp8=e;return Cyc_Tcutil_copye(_Tmp8,(void*)({struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*_Tmp9=_cycalloc(sizeof(struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct));_Tmp9->tag=40,_Tmp9->f1=new_e1;_Tmp9;}));}}case 19: _Tmp6=(void*)((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp5=((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{void*t=_Tmp6;struct Cyc_List_List*f=_Tmp5;
# 2254
void*new_typ=Cyc_Tcutil_rsubstitute(r,inst,t);
if(new_typ==t)return e;else{struct Cyc_Absyn_Exp*_Tmp8=e;return Cyc_Tcutil_copye(_Tmp8,(void*)({struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*_Tmp9=_cycalloc(sizeof(struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct));_Tmp9->tag=19,_Tmp9->f1=new_typ,_Tmp9->f2=f;_Tmp9;}));}}case 38: _Tmp6=(void*)((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{void*t=_Tmp6;
# 2257
void*new_typ=Cyc_Tcutil_rsubstitute(r,inst,t);
if(new_typ==t)return e;{
# 2260
void*_Tmp8=Cyc_Absyn_compress(new_typ);void*_Tmp9;if(*((int*)_Tmp8)==9){_Tmp9=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_Tmp8)->f1;{struct Cyc_Absyn_Exp*e=_Tmp9;
return e;}}else{
struct Cyc_Absyn_Exp*_TmpA=e;return Cyc_Tcutil_copye(_TmpA,(void*)({struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*_TmpB=_cycalloc(sizeof(struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct));_TmpB->tag=38,_TmpB->f1=new_typ;_TmpB;}));};}}case 41: _Tmp6=((struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e1=_Tmp6;
# 2265
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1==e1)return e;else{struct Cyc_Absyn_Exp*_Tmp8=e;return Cyc_Tcutil_copye(_Tmp8,(void*)({struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct*_Tmp9=_cycalloc(sizeof(struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct));_Tmp9->tag=41,_Tmp9->f1=new_e1;_Tmp9;}));}}case 42: _Tmp6=((struct Cyc_Absyn_Assert_false_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e1=_Tmp6;
# 2268
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1==e1)return e;else{struct Cyc_Absyn_Exp*_Tmp8=e;return Cyc_Tcutil_copye(_Tmp8,(void*)({struct Cyc_Absyn_Assert_false_e_Absyn_Raw_exp_struct*_Tmp9=_cycalloc(sizeof(struct Cyc_Absyn_Assert_false_e_Absyn_Raw_exp_struct));_Tmp9->tag=42,_Tmp9->f1=new_e1;_Tmp9;}));}}case 25:
 goto _LL3A;case 26: _LL3A:
 goto _LL3C;case 28: _LL3C:
 goto _LL3E;case 29: _LL3E:
 goto _LL40;case 30: _LL40:
 goto _LL42;case 27: _LL42:
 goto _LL44;case 24: _LL44:
 goto _LL46;case 36: _LL46:
 goto _LL48;case 33: _LL48:
 goto _LL4A;case 16: _LL4A:
 goto _LL4C;case 34: _LL4C:
 goto _LL4E;case 4: _LL4E:
 goto _LL50;case 5: _LL50:
 goto _LL52;case 35: _LL52:
 goto _LL54;case 39: _LL54:
 goto _LL56;default: _LL56: {
# 2286
struct Cyc_Warn_String_Warn_Warg_struct _Tmp8=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp9;_Tmp9.tag=0,_Tmp9.f1=_tag_fat("non-type-level-expression in Tcutil::rsubsexp",sizeof(char),46U);_Tmp9;});void*_Tmp9[1];_Tmp9[0]=& _Tmp8;return({struct Cyc_Absyn_Exp*(*_TmpA)(struct _fat_ptr)=(struct Cyc_Absyn_Exp*(*)(struct _fat_ptr))Cyc_Warn_impos2;_TmpA;})(_tag_fat(_Tmp9,sizeof(void*),1));}};}
# 2290
static struct Cyc_List_List*Cyc_Tcutil_substs_exps(struct _RegionHandle*rgn,struct Cyc_List_List*inst,struct Cyc_List_List*es){
# 2293
if(es==0)
return 0;{
struct Cyc_Absyn_Exp*old_hd=(struct Cyc_Absyn_Exp*)es->hd;
struct Cyc_List_List*old_tl=es->tl;
struct Cyc_Absyn_Exp*new_hd=Cyc_Tcutil_rsubsexp(rgn,inst,old_hd);
struct Cyc_List_List*new_tl=Cyc_Tcutil_substs_exps(rgn,inst,old_tl);
if(old_hd==new_hd && old_tl==new_tl)
return es;{
struct Cyc_List_List*_Tmp0=_cycalloc(sizeof(struct Cyc_List_List));_Tmp0->hd=new_hd,_Tmp0->tl=new_tl;return _Tmp0;}}}
# 2305
static struct Cyc_Absyn_Exp*Cyc_Tcutil_rsubs_exp_opt(struct _RegionHandle*r,struct Cyc_List_List*inst,struct Cyc_Absyn_Exp*e){
# 2308
if(e==0)return 0;else{return Cyc_Tcutil_rsubsexp(r,inst,e);}}
# 2311
static struct Cyc_Absyn_Aggrfield*Cyc_Tcutil_subst_aggrfield(struct _RegionHandle*r,struct Cyc_List_List*inst,struct Cyc_Absyn_Aggrfield*f){
# 2314
void*t=f->type;
struct Cyc_Absyn_Exp*req=f->requires_clause;
void*new_typ=Cyc_Tcutil_rsubstitute(r,inst,t);
struct Cyc_Absyn_Exp*new_req=Cyc_Tcutil_rsubs_exp_opt(r,inst,req);
if(t==new_typ && req==new_req)
return f;{
struct Cyc_Absyn_Aggrfield*ans;ans=_cycalloc(sizeof(struct Cyc_Absyn_Aggrfield)),*ans=*f;
ans->type=new_typ;
ans->requires_clause=new_req;
return ans;}}
# 2326
static struct Cyc_List_List*Cyc_Tcutil_subst_aggrfields(struct _RegionHandle*r,struct Cyc_List_List*inst,struct Cyc_List_List*fs){
# 2329
if(fs==0)return 0;{
struct Cyc_Absyn_Aggrfield*f=(struct Cyc_Absyn_Aggrfield*)fs->hd;
struct Cyc_List_List*t=fs->tl;
struct Cyc_Absyn_Aggrfield*new_f=Cyc_Tcutil_subst_aggrfield(r,inst,f);
struct Cyc_List_List*new_typ=Cyc_Tcutil_subst_aggrfields(r,inst,t);
if(new_f==f && new_typ==t)return fs;else{struct Cyc_List_List*_Tmp0=_cycalloc(sizeof(struct Cyc_List_List));_Tmp0->hd=new_f,_Tmp0->tl=new_typ;return _Tmp0;}}}
# 2337
struct Cyc_List_List*Cyc_Tcutil_rsubst_type_assoc(struct _RegionHandle*rgn,struct Cyc_List_List*inst,struct Cyc_List_List*rgn_po){
# 2340
struct _tuple0 _Tmp0=Cyc_List_rsplit(rgn,rgn,rgn_po);void*_Tmp1;void*_Tmp2;_Tmp2=_Tmp0.f0;_Tmp1=_Tmp0.f1;{struct Cyc_List_List*rpo1a=_Tmp2;struct Cyc_List_List*rpo1b=_Tmp1;
struct Cyc_List_List*rpo2a=Cyc_Tcutil_substs(rgn,inst,rpo1a);
struct Cyc_List_List*rpo2b=Cyc_Tcutil_substs(rgn,inst,rpo1b);
if(rpo2a==rpo1a && rpo2b==rpo1b)return rgn_po;else{return Cyc_List_zip(rpo2a,rpo2b);}}}
# 2346
struct Cyc_List_List*Cyc_Tcutil_rsubst_effconstr(struct _RegionHandle*rgn,struct Cyc_List_List*inst,struct Cyc_List_List*ec){
# 2349
struct Cyc_List_List*slist=0;
for(1;ec!=0;ec=ec->tl){
void*_Tmp0=(void*)ec->hd;void*_Tmp1;void*_Tmp2;switch(*((int*)_Tmp0)){case 0: _Tmp2=(void*)((struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*)_Tmp0)->f1;{void*t=_Tmp2;
# 2353
void*nt=Cyc_Tcutil_rsubstitute(rgn,inst,t);
if(nt==t)
slist=({struct Cyc_List_List*_Tmp3=_cycalloc(sizeof(struct Cyc_List_List));_Tmp3->hd=(void*)ec->hd,_Tmp3->tl=slist;_Tmp3;});else{
# 2357
slist=({struct Cyc_List_List*_Tmp3=_cycalloc(sizeof(struct Cyc_List_List));({void*_Tmp4=(void*)({struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*_Tmp5=_cycalloc(sizeof(struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct));_Tmp5->tag=0,_Tmp5->f1=nt;_Tmp5;});_Tmp3->hd=_Tmp4;}),_Tmp3->tl=slist;_Tmp3;});}
goto _LL0;}case 1: _Tmp2=(void*)((struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*)_Tmp0)->f1;_Tmp1=(void*)((struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*)_Tmp0)->f2;{void*t1=_Tmp2;void*t2=_Tmp1;
# 2360
void*nt1=Cyc_Tcutil_rsubstitute(rgn,inst,t1);
void*nt2=Cyc_Tcutil_rsubstitute(rgn,inst,t2);
if(nt1==t1 && nt2==t2)
slist=({struct Cyc_List_List*_Tmp3=_cycalloc(sizeof(struct Cyc_List_List));_Tmp3->hd=(void*)ec->hd,_Tmp3->tl=slist;_Tmp3;});else{
# 2365
slist=({struct Cyc_List_List*_Tmp3=_cycalloc(sizeof(struct Cyc_List_List));({void*_Tmp4=(void*)({struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*_Tmp5=_cycalloc(sizeof(struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct));_Tmp5->tag=1,_Tmp5->f1=nt1,_Tmp5->f2=nt2;_Tmp5;});_Tmp3->hd=_Tmp4;}),_Tmp3->tl=slist;_Tmp3;});}
goto _LL0;}default: _Tmp2=(void*)((struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_Tmp0)->f1;_Tmp1=(void*)((struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_Tmp0)->f2;{void*t1=_Tmp2;void*t2=_Tmp1;
# 2368
void*nt1=Cyc_Tcutil_rsubstitute(rgn,inst,t1);
void*nt2=Cyc_Tcutil_rsubstitute(rgn,inst,t2);
if(nt1==t1 && nt2==t2)
slist=({struct Cyc_List_List*_Tmp3=_cycalloc(sizeof(struct Cyc_List_List));_Tmp3->hd=(void*)ec->hd,_Tmp3->tl=slist;_Tmp3;});else{
# 2373
slist=({struct Cyc_List_List*_Tmp3=_cycalloc(sizeof(struct Cyc_List_List));({void*_Tmp4=(void*)({struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*_Tmp5=_cycalloc(sizeof(struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct));_Tmp5->tag=2,_Tmp5->f1=nt1,_Tmp5->f2=nt2;_Tmp5;});_Tmp3->hd=_Tmp4;}),_Tmp3->tl=slist;_Tmp3;});}
goto _LL0;}}_LL0:;}
# 2377
return slist;}
# 2382
static int Cyc_Tcutil_skip_fntypes=0;
# 2384
void*Cyc_Tcutil_rsubstitute(struct _RegionHandle*rgn,struct Cyc_List_List*inst,void*t){
# 2387
void*_Tmp0=Cyc_Absyn_compress(t);enum Cyc_Absyn_AggrKind _Tmp1;void*_Tmp2;void*_Tmp3;void*_Tmp4;void*_Tmp5;void*_Tmp6;void*_Tmp7;int _Tmp8;void*_Tmp9;void*_TmpA;void*_TmpB;unsigned _TmpC;struct Cyc_Absyn_Tqual _TmpD;void*_TmpE;void*_TmpF;void*_Tmp10;void*_Tmp11;switch(*((int*)_Tmp0)){case 2: _Tmp11=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Tvar*v=_Tmp11;
# 2390
struct _handler_cons _Tmp12;_push_handler(& _Tmp12);{int _Tmp13=0;if(setjmp(_Tmp12.handler))_Tmp13=1;if(!_Tmp13){{void*_Tmp14=({void*(*_Tmp15)(int(*)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct Cyc_Absyn_Tvar*)=(void*(*)(int(*)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct Cyc_Absyn_Tvar*))Cyc_List_assoc_cmp;_Tmp15;})(Cyc_Absyn_tvar_cmp,inst,v);_npop_handler(0);return _Tmp14;};_pop_handler();}else{void*_Tmp14=(void*)Cyc_Core_get_exn_thrown();void*_Tmp15;if(((struct Cyc_Core_Not_found_exn_struct*)_Tmp14)->tag==Cyc_Core_Not_found)
return t;else{_Tmp15=_Tmp14;{void*exn=_Tmp15;_rethrow(exn);}};}}}case 8: _Tmp11=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_Tmp0)->f1;_Tmp10=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_Tmp0)->f2;_TmpF=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_Tmp0)->f3;_TmpE=(void*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_Tmp0)->f4;{struct _tuple1*n=_Tmp11;struct Cyc_List_List*ts=_Tmp10;struct Cyc_Absyn_Typedefdecl*td=_TmpF;void*topt=_TmpE;
# 2393
struct Cyc_List_List*new_ts=Cyc_Tcutil_substs(rgn,inst,ts);
if(new_ts==ts)return t;else{return(void*)({struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_Tmp12=_cycalloc(sizeof(struct Cyc_Absyn_TypedefType_Absyn_Type_struct));_Tmp12->tag=8,_Tmp12->f1=n,_Tmp12->f2=new_ts,_Tmp12->f3=td,_Tmp12->f4=topt;_Tmp12;});}}case 5: _Tmp11=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.elt_type;_TmpD=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.tq;_Tmp10=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.num_elts;_TmpF=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.zero_term;_TmpC=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.zt_loc;{void*t1=_Tmp11;struct Cyc_Absyn_Tqual tq=_TmpD;struct Cyc_Absyn_Exp*e=_Tmp10;void*zt=_TmpF;unsigned ztl=_TmpC;
# 2396
void*new_t1=Cyc_Tcutil_rsubstitute(rgn,inst,t1);
struct Cyc_Absyn_Exp*new_e=Cyc_Tcutil_rsubs_exp_opt(rgn,inst,e);
void*new_zt=Cyc_Tcutil_rsubstitute(rgn,inst,zt);
if((new_t1==t1 && new_e==e)&& new_zt==zt)return t;else{return(void*)({struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_Tmp12=_cycalloc(sizeof(struct Cyc_Absyn_ArrayType_Absyn_Type_struct));
_Tmp12->tag=5,_Tmp12->f1.elt_type=new_t1,_Tmp12->f1.tq=tq,_Tmp12->f1.num_elts=new_e,_Tmp12->f1.zero_term=new_zt,_Tmp12->f1.zt_loc=ztl;_Tmp12;});}}case 4: _Tmp11=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.elt_type;_TmpD=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.elt_tq;_Tmp10=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.eff;_TmpF=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.nullable;_TmpE=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.bounds;_TmpB=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.zero_term;_TmpA=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.autoreleased;_Tmp9=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.aqual;{void*t1=_Tmp11;struct Cyc_Absyn_Tqual tq=_TmpD;void*r=_Tmp10;void*n=_TmpF;void*b=_TmpE;void*zt=_TmpB;void*rel=_TmpA;void*aq=_Tmp9;
# 2402
void*new_t1=Cyc_Tcutil_rsubstitute(rgn,inst,t1);
void*new_r=Cyc_Tcutil_rsubstitute(rgn,inst,r);
void*new_b=Cyc_Tcutil_rsubstitute(rgn,inst,b);
void*new_zt=Cyc_Tcutil_rsubstitute(rgn,inst,zt);
void*new_rel=Cyc_Tcutil_rsubstitute(rgn,inst,rel);
void*new_aq=Cyc_Tcutil_rsubstitute(rgn,inst,aq);
if(((((new_t1==t1 && new_r==r)&& new_b==b)&& new_zt==zt)&& new_rel==rel)&& new_aq==aq)
return t;
return Cyc_Absyn_pointer_type(({struct Cyc_Absyn_PtrInfo _Tmp12;_Tmp12.elt_type=new_t1,_Tmp12.elt_tq=tq,_Tmp12.ptr_atts.eff=new_r,_Tmp12.ptr_atts.nullable=n,_Tmp12.ptr_atts.bounds=new_b,_Tmp12.ptr_atts.zero_term=new_zt,_Tmp12.ptr_atts.ptrloc=0,_Tmp12.ptr_atts.autoreleased=new_rel,_Tmp12.ptr_atts.aqual=new_aq;_Tmp12;}));}case 6: _Tmp11=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.tvars;_Tmp10=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.effect;_TmpD=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.ret_tqual;_TmpF=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.ret_type;_TmpE=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.args;_Tmp8=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.c_varargs;_TmpB=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.cyc_varargs;_TmpA=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.qual_bnd;_Tmp9=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.attributes;_Tmp7=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.requires_clause;_Tmp6=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.ensures_clause;_Tmp5=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.throws_clause;_Tmp4=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.return_value;_Tmp3=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.arg_vardecls;_Tmp2=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_Tmp0)->f1.effconstr;{struct Cyc_List_List*vs=_Tmp11;void*eff=_Tmp10;struct Cyc_Absyn_Tqual rtq=_TmpD;void*rtyp=_TmpF;struct Cyc_List_List*args=_TmpE;int c_varargs=_Tmp8;struct Cyc_Absyn_VarargInfo*cyc_varargs=_TmpB;struct Cyc_List_List*qb=_TmpA;struct Cyc_List_List*atts=_Tmp9;struct Cyc_Absyn_Exp*req=_Tmp7;struct Cyc_Absyn_Exp*ens=_Tmp6;struct Cyc_Absyn_Exp*thrws=_Tmp5;struct Cyc_Absyn_Vardecl*ret_value=_Tmp4;struct Cyc_List_List*argvds=_Tmp3;struct Cyc_List_List*effconstr=_Tmp2;
# 2413
if(Cyc_Tcutil_skip_fntypes)return t;
# 2415
{struct Cyc_List_List*p=vs;for(0;p!=0;p=p->tl){
inst=({struct Cyc_List_List*_Tmp12=_region_malloc(rgn,0U,sizeof(struct Cyc_List_List));({struct _tuple17*_Tmp13=({struct _tuple17*_Tmp14=_region_malloc(rgn,0U,sizeof(struct _tuple17));_Tmp14->f0=(struct Cyc_Absyn_Tvar*)p->hd,({void*_Tmp15=Cyc_Absyn_var_type((struct Cyc_Absyn_Tvar*)p->hd);_Tmp14->f1=_Tmp15;});_Tmp14;});_Tmp12->hd=_Tmp13;}),_Tmp12->tl=inst;_Tmp12;});}}{
struct _tuple0 _Tmp12=({struct _RegionHandle*_Tmp13=rgn;struct _RegionHandle*_Tmp14=rgn;Cyc_List_rsplit(_Tmp13,_Tmp14,
({struct Cyc_List_List*(*_Tmp15)(struct _RegionHandle*,struct _tuple23*(*)(struct _RegionHandle*,struct _tuple9*),struct _RegionHandle*,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct _RegionHandle*,struct _tuple23*(*)(struct _RegionHandle*,struct _tuple9*),struct _RegionHandle*,struct Cyc_List_List*))Cyc_List_rmap_c;_Tmp15;})(rgn,Cyc_Tcutil_substitute_f1,rgn,args));});
# 2417
void*_Tmp13;void*_Tmp14;_Tmp14=_Tmp12.f0;_Tmp13=_Tmp12.f1;{struct Cyc_List_List*qs=_Tmp14;struct Cyc_List_List*ts=_Tmp13;
# 2419
struct Cyc_List_List*args2=args;
struct Cyc_List_List*ts2=Cyc_Tcutil_substs(rgn,inst,ts);
if(ts2!=ts)
args2=({struct Cyc_List_List*(*_Tmp15)(struct _tuple9*(*)(struct _tuple9*,void*),struct Cyc_List_List*,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct _tuple9*(*)(struct _tuple9*,void*),struct Cyc_List_List*,struct Cyc_List_List*))Cyc_List_map2;_Tmp15;})(Cyc_Tcutil_substitute_f2,args,ts2);{
void*eff2=eff==0?0: Cyc_Tcutil_rsubstitute(rgn,inst,eff);
# 2425
struct Cyc_Absyn_VarargInfo*cyc_varargs2;
if(cyc_varargs==0)
cyc_varargs2=0;else{
# 2429
struct Cyc_Absyn_VarargInfo _Tmp15=*cyc_varargs;int _Tmp16;void*_Tmp17;struct Cyc_Absyn_Tqual _Tmp18;void*_Tmp19;_Tmp19=_Tmp15.name;_Tmp18=_Tmp15.tq;_Tmp17=_Tmp15.type;_Tmp16=_Tmp15.inject;{struct _fat_ptr*n=_Tmp19;struct Cyc_Absyn_Tqual tq=_Tmp18;void*t=_Tmp17;int i=_Tmp16;
void*t2=Cyc_Tcutil_rsubstitute(rgn,inst,t);
cyc_varargs2=t2==t?cyc_varargs:({struct Cyc_Absyn_VarargInfo*_Tmp1A=_cycalloc(sizeof(struct Cyc_Absyn_VarargInfo));_Tmp1A->name=n,_Tmp1A->tq=tq,_Tmp1A->type=t2,_Tmp1A->inject=i;_Tmp1A;});}}{
# 2433
struct Cyc_List_List*effconstr2=Cyc_Tcutil_rsubst_effconstr(rgn,inst,effconstr);
# 2435
struct Cyc_List_List*qb2=Cyc_Tcutil_rsubst_type_assoc(rgn,inst,qb);
struct Cyc_Absyn_Exp*req2=Cyc_Tcutil_rsubs_exp_opt(rgn,inst,req);
struct Cyc_Absyn_Exp*ens2=Cyc_Tcutil_rsubs_exp_opt(rgn,inst,ens);
struct Cyc_Absyn_Exp*thrws2=Cyc_Tcutil_rsubs_exp_opt(rgn,inst,thrws);
# 2444
struct Cyc_List_List*argvds2=0;
{struct Cyc_List_List*avs=argvds;for(0;avs!=0;avs=avs->tl){
struct Cyc_Absyn_Vardecl*av=(struct Cyc_Absyn_Vardecl*)avs->hd;
struct Cyc_Absyn_Vardecl*av2=0;
if(av!=0){
av2=({struct Cyc_Absyn_Vardecl*_Tmp15=_cycalloc(sizeof(struct Cyc_Absyn_Vardecl));*_Tmp15=*av;_Tmp15;});
({void*_Tmp15=Cyc_Tcutil_rsubstitute(rgn,inst,av->type);av2->type=_Tmp15;});}
# 2452
argvds2=({struct Cyc_List_List*_Tmp15=_cycalloc(sizeof(struct Cyc_List_List));_Tmp15->hd=av2,_Tmp15->tl=argvds2;_Tmp15;});}}
# 2454
argvds2=Cyc_List_imp_rev(argvds2);{
struct Cyc_Absyn_Vardecl*ret_value2=0;
if(ret_value!=0){
ret_value2=({struct Cyc_Absyn_Vardecl*_Tmp15=_cycalloc(sizeof(struct Cyc_Absyn_Vardecl));*_Tmp15=*ret_value;_Tmp15;});
({void*_Tmp15=Cyc_Tcutil_rsubstitute(rgn,inst,ret_value2->type);ret_value2->type=_Tmp15;});}{
# 2461
struct Cyc_AssnDef_ExistAssnFn*req_assn2=0;
struct Cyc_AssnDef_ExistAssnFn*ens_assn2=0;
struct Cyc_AssnDef_ExistAssnFn*thrws_assn2=0;
# 2465
struct Cyc_List_List*vds1;vds1=_cycalloc(sizeof(struct Cyc_List_List)),vds1->hd=ret_value,vds1->tl=argvds;{
struct Cyc_List_List*vds2;vds2=_cycalloc(sizeof(struct Cyc_List_List)),vds2->hd=ret_value2,vds2->tl=argvds2;{
struct Cyc_List_List*boundvds;boundvds=_cycalloc(sizeof(struct Cyc_List_List)),boundvds->hd=Cyc_AssnDef_memory,boundvds->tl=argvds2;
# 2469
if(req2!=0){
struct Cyc_AssnDef_AssnMap*req_am2;req_am2=_cycalloc(sizeof(struct Cyc_AssnDef_AssnMap)),({struct Cyc_AssnDef_AssnMap _Tmp15=Cyc_Vcgen_clause2assn(req2);*req_am2=_Tmp15;});
req_am2=Cyc_AssnDef_subst_vardecls(vds1,vds2,req_am2);
req_assn2=Cyc_AssnDef_assnmap2existassnfn(*_check_null(req_am2),boundvds);}
# 2474
if(ens2!=0){
struct Cyc_AssnDef_AssnMap*ens_am2;ens_am2=_cycalloc(sizeof(struct Cyc_AssnDef_AssnMap)),({struct Cyc_AssnDef_AssnMap _Tmp15=Cyc_Vcgen_clause2assn(ens2);*ens_am2=_Tmp15;});
ens_am2=Cyc_AssnDef_subst_vardecls(vds1,vds2,ens_am2);
({struct Cyc_List_List*_Tmp15=_cycalloc(sizeof(struct Cyc_List_List));_Tmp15->hd=ret_value2,_Tmp15->tl=boundvds;_Tmp15;});
ens_assn2=({struct Cyc_AssnDef_AssnMap _Tmp15=*_check_null(ens_am2);Cyc_AssnDef_assnmap2existassnfn(_Tmp15,({struct Cyc_List_List*_Tmp16=_cycalloc(sizeof(struct Cyc_List_List));_Tmp16->hd=ret_value2,_Tmp16->tl=boundvds;_Tmp16;}));});}
# 2480
if(thrws2!=0){
struct Cyc_AssnDef_AssnMap*thrws_am2;thrws_am2=_cycalloc(sizeof(struct Cyc_AssnDef_AssnMap)),({struct Cyc_AssnDef_AssnMap _Tmp15=Cyc_Vcgen_clause2assn(thrws2);*thrws_am2=_Tmp15;});
thrws_am2=Cyc_AssnDef_subst_vardecls(vds1,vds2,thrws_am2);
thrws_assn2=Cyc_AssnDef_assnmap2existassnfn(*_check_null(thrws_am2),boundvds);}
# 2485
return(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_Tmp15=_cycalloc(sizeof(struct Cyc_Absyn_FnType_Absyn_Type_struct));_Tmp15->tag=6,_Tmp15->f1.tvars=vs,_Tmp15->f1.effect=eff2,_Tmp15->f1.ret_tqual=rtq,({void*_Tmp16=Cyc_Tcutil_rsubstitute(rgn,inst,rtyp);_Tmp15->f1.ret_type=_Tmp16;}),_Tmp15->f1.args=args2,_Tmp15->f1.c_varargs=c_varargs,_Tmp15->f1.cyc_varargs=cyc_varargs2,_Tmp15->f1.qual_bnd=qb2,_Tmp15->f1.attributes=atts,_Tmp15->f1.requires_clause=req2,_Tmp15->f1.requires_assn=req_assn2,_Tmp15->f1.ensures_clause=ens2,_Tmp15->f1.ensures_assn=ens_assn2,_Tmp15->f1.throws_clause=thrws2,_Tmp15->f1.throws_assn=thrws_assn2,_Tmp15->f1.return_value=ret_value2,_Tmp15->f1.arg_vardecls=argvds2,_Tmp15->f1.effconstr=effconstr2;_Tmp15;});}}}}}}}}}case 7: _Tmp1=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_Tmp0)->f1;_Tmp8=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_Tmp0)->f2;_Tmp11=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_Tmp0)->f3;{enum Cyc_Absyn_AggrKind k=_Tmp1;int b=_Tmp8;struct Cyc_List_List*fs=_Tmp11;
# 2490
struct Cyc_List_List*new_fs=Cyc_Tcutil_subst_aggrfields(rgn,inst,fs);
if(fs==new_fs)return t;else{return(void*)({struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_Tmp12=_cycalloc(sizeof(struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct));_Tmp12->tag=7,_Tmp12->f1=k,_Tmp12->f2=b,_Tmp12->f3=new_fs;_Tmp12;});}}case 3: _Tmp11=(void*)((struct Cyc_Absyn_Cvar_Absyn_Type_struct*)_Tmp0)->f2;{void*r=_Tmp11;
_Tmp11=r;goto _LL10;}case 1: _Tmp11=(void*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_Tmp0)->f2;_LL10: {void*r=_Tmp11;
# 2494
if(r==0)return t;else{return Cyc_Tcutil_rsubstitute(rgn,inst,r);}}case 0: _Tmp11=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1;_Tmp10=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f2;{void*c=_Tmp11;struct Cyc_List_List*ts=_Tmp10;
# 2499
struct Cyc_List_List*new_ts=Cyc_Tcutil_substs(rgn,inst,ts);
if(ts==new_ts)return t;else{return(void*)({struct Cyc_Absyn_AppType_Absyn_Type_struct*_Tmp12=_cycalloc(sizeof(struct Cyc_Absyn_AppType_Absyn_Type_struct));_Tmp12->tag=0,_Tmp12->f1=c,_Tmp12->f2=new_ts;_Tmp12;});}}case 9: _Tmp11=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e=_Tmp11;
# 2502
struct Cyc_Absyn_Exp*new_e=Cyc_Tcutil_rsubsexp(rgn,inst,e);
if(new_e==e)return t;else{return(void*)({struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_Tmp12=_cycalloc(sizeof(struct Cyc_Absyn_ValueofType_Absyn_Type_struct));_Tmp12->tag=9,_Tmp12->f1=new_e;_Tmp12;});}}case 11: _Tmp11=((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e=_Tmp11;
# 2505
struct Cyc_Absyn_Exp*new_e=Cyc_Tcutil_rsubsexp(rgn,inst,e);
if(new_e==e)return t;else{return(void*)({struct Cyc_Absyn_TypeofType_Absyn_Type_struct*_Tmp12=_cycalloc(sizeof(struct Cyc_Absyn_TypeofType_Absyn_Type_struct));_Tmp12->tag=11,_Tmp12->f1=new_e;_Tmp12;});}}default:
# 2508
 if(Cyc_Tcutil_skip_fntypes)return t;
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp12=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp13;_Tmp13.tag=0,_Tmp13.f1=_tag_fat("found typedecltype in rsubs",sizeof(char),28U);_Tmp13;});void*_Tmp13[1];_Tmp13[0]=& _Tmp12;({int(*_Tmp14)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_Tmp14;})(_tag_fat(_Tmp13,sizeof(void*),1));});};}
# 2513
static struct Cyc_List_List*Cyc_Tcutil_substs(struct _RegionHandle*rgn,struct Cyc_List_List*inst,struct Cyc_List_List*ts){
# 2516
if(ts==0)
return 0;{
void*old_hd=(void*)ts->hd;
struct Cyc_List_List*old_tl=ts->tl;
void*new_hd=Cyc_Tcutil_rsubstitute(rgn,inst,old_hd);
struct Cyc_List_List*new_tl=Cyc_Tcutil_substs(rgn,inst,old_tl);
if(old_hd==new_hd && old_tl==new_tl)
return ts;{
struct Cyc_List_List*_Tmp0=_cycalloc(sizeof(struct Cyc_List_List));_Tmp0->hd=new_hd,_Tmp0->tl=new_tl;return _Tmp0;}}}
# 2527
void*Cyc_Tcutil_substitute(struct Cyc_List_List*inst,void*t){
if(inst==0)return t;else{return Cyc_Tcutil_rsubstitute(Cyc_Core_heap_region,inst,t);}}
# 2531
void*Cyc_Tcutil_substitute_nofun(struct Cyc_List_List*inst,void*t){
Cyc_Tcutil_skip_fntypes=1;{
void*new_typ=Cyc_Tcutil_substitute(inst,t);
Cyc_Tcutil_skip_fntypes=0;
return new_typ;}}
# 2539
struct _tuple17*Cyc_Tcutil_make_inst_var(struct Cyc_List_List*s,struct Cyc_Absyn_Tvar*tv){
struct Cyc_Core_Opt*k=Cyc_Kinds_kind_to_opt(Cyc_Kinds_tvar_kind(tv,& Cyc_Kinds_bk));
struct _tuple17*_Tmp0=_cycalloc(sizeof(struct _tuple17));_Tmp0->f0=tv,({void*_Tmp1=({struct Cyc_Core_Opt*_Tmp2=k;Cyc_Absyn_new_evar(_Tmp2,({struct Cyc_Core_Opt*_Tmp3=_cycalloc(sizeof(struct Cyc_Core_Opt));_Tmp3->v=s;_Tmp3;}));});_Tmp0->f1=_Tmp1;});return _Tmp0;}struct _tuple24{struct Cyc_List_List*f0;struct _RegionHandle*f1;};
# 2544
struct _tuple17*Cyc_Tcutil_r_make_inst_var(struct _tuple24*env,struct Cyc_Absyn_Tvar*tv){
# 2546
void*_Tmp0;void*_Tmp1;_Tmp1=env->f0;_Tmp0=env->f1;{struct Cyc_List_List*s=_Tmp1;struct _RegionHandle*rgn=_Tmp0;
struct Cyc_Core_Opt*k=Cyc_Kinds_kind_to_opt(Cyc_Kinds_tvar_kind(tv,& Cyc_Kinds_bk));
struct _tuple17*_Tmp2=_region_malloc(rgn,0U,sizeof(struct _tuple17));_Tmp2->f0=tv,({void*_Tmp3=({struct Cyc_Core_Opt*_Tmp4=k;Cyc_Absyn_new_evar(_Tmp4,({struct Cyc_Core_Opt*_Tmp5=_cycalloc(sizeof(struct Cyc_Core_Opt));_Tmp5->v=s;_Tmp5;}));});_Tmp2->f1=_Tmp3;});return _Tmp2;}}
# 2551
void Cyc_Tcutil_check_bitfield(unsigned loc,void*field_type,struct Cyc_Absyn_Exp*width,struct _fat_ptr*fn){
# 2553
if(width!=0){
unsigned w=0U;
if(!Cyc_Tcutil_is_const_exp(width))
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp0=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp1;_Tmp1.tag=0,_Tmp1.f1=_tag_fat("bitfield ",sizeof(char),10U);_Tmp1;});struct Cyc_Warn_String_Warn_Warg_struct _Tmp1=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp2;_Tmp2.tag=0,_Tmp2.f1=*fn;_Tmp2;});struct Cyc_Warn_String_Warn_Warg_struct _Tmp2=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp3;_Tmp3.tag=0,_Tmp3.f1=_tag_fat(" does not have constant width",sizeof(char),30U);_Tmp3;});void*_Tmp3[3];_Tmp3[0]=& _Tmp0,_Tmp3[1]=& _Tmp1,_Tmp3[2]=& _Tmp2;Cyc_Warn_err2(loc,_tag_fat(_Tmp3,sizeof(void*),3));});else{
# 2558
struct _tuple12 _Tmp0=Cyc_Evexp_eval_const_uint_exp(width);int _Tmp1;unsigned _Tmp2;_Tmp2=_Tmp0.f0;_Tmp1=_Tmp0.f1;{unsigned i=_Tmp2;int known=_Tmp1;
if(!known)
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp3=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp4;_Tmp4.tag=0,_Tmp4.f1=_tag_fat("cannot evaluate bitfield width at compile time",sizeof(char),47U);_Tmp4;});void*_Tmp4[1];_Tmp4[0]=& _Tmp3;Cyc_Warn_warn2(loc,_tag_fat(_Tmp4,sizeof(void*),1));});
if((int)i < 0)
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp3=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp4;_Tmp4.tag=0,_Tmp4.f1=_tag_fat("bitfield has negative width",sizeof(char),28U);_Tmp4;});void*_Tmp4[1];_Tmp4[0]=& _Tmp3;Cyc_Warn_err2(loc,_tag_fat(_Tmp4,sizeof(void*),1));});
w=i;}}{
# 2565
void*_Tmp0=Cyc_Absyn_compress(field_type);enum Cyc_Absyn_Size_of _Tmp1;if(*((int*)_Tmp0)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1)==1){_Tmp1=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1)->f2;{enum Cyc_Absyn_Size_of b=_Tmp1;
# 2568
int bad;
switch((int)b){case Cyc_Absyn_Char_sz:
 bad=w > 8U;goto _LL8;case Cyc_Absyn_Short_sz:
 bad=w > 16U;goto _LL8;case Cyc_Absyn_Long_sz:
 goto _LL10;case Cyc_Absyn_Int_sz: _LL10:
 bad=w > 32U;goto _LL8;case Cyc_Absyn_LongLong_sz:
 goto _LL14;default: _LL14:
 bad=w > 64U;goto _LL8;}_LL8:;
# 2577
if(bad)
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp2=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp3;_Tmp3.tag=0,_Tmp3.f1=_tag_fat("bitfield larger than type",sizeof(char),26U);_Tmp3;});void*_Tmp3[1];_Tmp3[0]=& _Tmp2;Cyc_Warn_warn2(loc,_tag_fat(_Tmp3,sizeof(void*),1));});
goto _LL3;}}else{goto _LL6;}}else{_LL6:
# 2581
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp2=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp3;_Tmp3.tag=0,_Tmp3.f1=_tag_fat("bitfield ",sizeof(char),10U);_Tmp3;});struct Cyc_Warn_String_Warn_Warg_struct _Tmp3=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp4;_Tmp4.tag=0,_Tmp4.f1=*fn;_Tmp4;});struct Cyc_Warn_String_Warn_Warg_struct _Tmp4=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp5;_Tmp5.tag=0,_Tmp5.f1=_tag_fat(" must have integral type but has type ",sizeof(char),39U);_Tmp5;});struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp5=({struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp6;_Tmp6.tag=2,_Tmp6.f1=(void*)field_type;_Tmp6;});void*_Tmp6[4];_Tmp6[0]=& _Tmp2,_Tmp6[1]=& _Tmp3,_Tmp6[2]=& _Tmp4,_Tmp6[3]=& _Tmp5;Cyc_Warn_err2(loc,_tag_fat(_Tmp6,sizeof(void*),4));});
# 2583
goto _LL3;}_LL3:;}}}
# 2590
int Cyc_Tcutil_extract_const_from_typedef(unsigned loc,int declared_const,void*t){
void*_Tmp0;void*_Tmp1;if(*((int*)t)==8){_Tmp1=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)t)->f3;_Tmp0=(void*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)t)->f4;{struct Cyc_Absyn_Typedefdecl*td=_Tmp1;void*tdopt=_Tmp0;
# 2593
if(_check_null(td)->tq.real_const || td->tq.print_const){
if(declared_const)({struct Cyc_Warn_String_Warn_Warg_struct _Tmp2=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp3;_Tmp3.tag=0,_Tmp3.f1=_tag_fat("extra const",sizeof(char),12U);_Tmp3;});void*_Tmp3[1];_Tmp3[0]=& _Tmp2;Cyc_Warn_warn2(loc,_tag_fat(_Tmp3,sizeof(void*),1));});
return 1;}
# 2598
if((unsigned)tdopt)
return Cyc_Tcutil_extract_const_from_typedef(loc,declared_const,tdopt);
return declared_const;}}else{
return declared_const;};}
# 2605
void Cyc_Tcutil_add_tvar_identity(struct Cyc_Absyn_Tvar*tv){
if(tv->identity==-1)
({int _Tmp0=Cyc_Tcutil_new_tvar_id();tv->identity=_Tmp0;});}
# 2609
void Cyc_Tcutil_add_tvar_identities(struct Cyc_List_List*tvs){
({void(*_Tmp0)(void(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*)=(void(*)(void(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*))Cyc_List_iter;_Tmp0;})(Cyc_Tcutil_add_tvar_identity,tvs);}
# 2615
static void Cyc_Tcutil_check_unique_unsorted(int(*cmp)(void*,void*),struct Cyc_List_List*vs,unsigned loc,struct _fat_ptr(*a2string)(void*),struct _fat_ptr msg){
# 2618
for(1;vs!=0;vs=vs->tl){
struct Cyc_List_List*vs2=vs->tl;for(0;vs2!=0;vs2=vs2->tl){
if(cmp(vs->hd,vs2->hd)==0)
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp0=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp1;_Tmp1.tag=0,_Tmp1.f1=msg;_Tmp1;});struct Cyc_Warn_String_Warn_Warg_struct _Tmp1=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp2;_Tmp2.tag=0,_Tmp2.f1=_tag_fat(": ",sizeof(char),3U);_Tmp2;});struct Cyc_Warn_String_Warn_Warg_struct _Tmp2=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp3;_Tmp3.tag=0,({struct _fat_ptr _Tmp4=a2string(vs->hd);_Tmp3.f1=_Tmp4;});_Tmp3;});void*_Tmp3[3];_Tmp3[0]=& _Tmp0,_Tmp3[1]=& _Tmp1,_Tmp3[2]=& _Tmp2;Cyc_Warn_err2(loc,_tag_fat(_Tmp3,sizeof(void*),3));});}}}
# 2623
static struct _fat_ptr Cyc_Tcutil_strptr2string(struct _fat_ptr*s){
return*s;}
# 2626
void Cyc_Tcutil_check_unique_vars(struct Cyc_List_List*vs,unsigned loc,struct _fat_ptr msg){
({void(*_Tmp0)(int(*)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*,unsigned,struct _fat_ptr(*)(struct _fat_ptr*),struct _fat_ptr)=(void(*)(int(*)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*,unsigned,struct _fat_ptr(*)(struct _fat_ptr*),struct _fat_ptr))Cyc_Tcutil_check_unique_unsorted;_Tmp0;})(Cyc_strptrcmp,vs,loc,Cyc_Tcutil_strptr2string,msg);}
# 2629
void Cyc_Tcutil_check_unique_tvars(unsigned loc,struct Cyc_List_List*tvs){
({void(*_Tmp0)(int(*)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,unsigned,struct _fat_ptr(*)(struct Cyc_Absyn_Tvar*),struct _fat_ptr)=(void(*)(int(*)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,unsigned,struct _fat_ptr(*)(struct Cyc_Absyn_Tvar*),struct _fat_ptr))Cyc_Tcutil_check_unique_unsorted;_Tmp0;})(Cyc_Absyn_tvar_cmp,tvs,loc,Cyc_Absynpp_tvar2string,
_tag_fat("duplicate type variable",sizeof(char),24U));}
# 2633
static void Cyc_Tcutil_imp_remove_dups(int(*cmp)(void*,void*),struct Cyc_List_List*l){
struct Cyc_List_List*l1=l;for(0;l1!=0;l1=l1->tl){
struct Cyc_List_List*prev=l1;
struct Cyc_List_List*l2=l1->tl;
while(l2!=0){
if(!cmp(l1->hd,l2->hd)){
prev->tl=l2->tl;
l2->tl=0;
l2=prev->tl;}else{
# 2644
prev=prev->tl;
l2=l2->tl;}
# 2638
1U;}}}struct _tuple25{struct Cyc_Absyn_Aggrfield*f0;int f1;};struct _tuple26{struct Cyc_List_List*f0;void*f1;};struct _tuple27{struct Cyc_Absyn_Aggrfield*f0;void*f1;};
# 2660 "tcutil.cyc"
struct Cyc_List_List*Cyc_Tcutil_resolve_aggregate_designators(struct _RegionHandle*rgn,unsigned loc,struct Cyc_List_List*des,enum Cyc_Absyn_AggrKind aggr_kind,struct Cyc_List_List*sdfields){
# 2665
struct _RegionHandle _Tmp0=_new_region(0U,"temp");struct _RegionHandle*temp=& _Tmp0;_push_region(temp);
# 2669
{struct Cyc_List_List*fields=0;
{struct Cyc_List_List*sd_fields=sdfields;for(0;sd_fields!=0;sd_fields=sd_fields->tl){
if(Cyc_strcmp(*((struct Cyc_Absyn_Aggrfield*)sd_fields->hd)->name,_tag_fat("",sizeof(char),1U))!=0)
fields=({struct Cyc_List_List*_Tmp1=_region_malloc(temp,0U,sizeof(struct Cyc_List_List));({struct _tuple25*_Tmp2=({struct _tuple25*_Tmp3=_region_malloc(temp,0U,sizeof(struct _tuple25));_Tmp3->f0=(struct Cyc_Absyn_Aggrfield*)sd_fields->hd,_Tmp3->f1=0;_Tmp3;});_Tmp1->hd=_Tmp2;}),_Tmp1->tl=fields;_Tmp1;});}}
fields=Cyc_List_imp_rev(fields);{
# 2675
struct _fat_ptr aggr_str=(int)aggr_kind==0?_tag_fat("struct",sizeof(char),7U): _tag_fat("union",sizeof(char),6U);
# 2678
struct Cyc_List_List*ans=0;
for(1;des!=0;des=des->tl){
struct _tuple26*_Tmp1=(struct _tuple26*)des->hd;void*_Tmp2;void*_Tmp3;_Tmp3=_Tmp1->f0;_Tmp2=(void*)_Tmp1->f1;{struct Cyc_List_List*dl=_Tmp3;void*a=_Tmp2;
if(dl==0){
# 2683
struct Cyc_List_List*fields2=fields;
for(1;fields2!=0;fields2=fields2->tl){
if(!(*((struct _tuple25*)fields2->hd)).f1){
(*((struct _tuple25*)fields2->hd)).f1=1;
({struct Cyc_List_List*_Tmp4=({struct Cyc_List_List*_Tmp5=_cycalloc(sizeof(struct Cyc_List_List));({void*_Tmp6=(void*)({struct Cyc_Absyn_FieldName_Absyn_Designator_struct*_Tmp7=_cycalloc(sizeof(struct Cyc_Absyn_FieldName_Absyn_Designator_struct));_Tmp7->tag=1,_Tmp7->f1=(*((struct _tuple25*)fields2->hd)).f0->name;_Tmp7;});_Tmp5->hd=_Tmp6;}),_Tmp5->tl=0;_Tmp5;});(*((struct _tuple26*)des->hd)).f0=_Tmp4;});
ans=({struct Cyc_List_List*_Tmp4=_region_malloc(rgn,0U,sizeof(struct Cyc_List_List));({struct _tuple27*_Tmp5=({struct _tuple27*_Tmp6=_region_malloc(rgn,0U,sizeof(struct _tuple27));_Tmp6->f0=(*((struct _tuple25*)fields2->hd)).f0,_Tmp6->f1=a;_Tmp6;});_Tmp4->hd=_Tmp5;}),_Tmp4->tl=ans;_Tmp4;});
break;}}
# 2691
if(fields2==0)
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp4=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp5;_Tmp5.tag=0,_Tmp5.f1=_tag_fat("too many arguments to ",sizeof(char),23U);_Tmp5;});struct Cyc_Warn_String_Warn_Warg_struct _Tmp5=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp6;_Tmp6.tag=0,_Tmp6.f1=aggr_str;_Tmp6;});void*_Tmp6[2];_Tmp6[0]=& _Tmp4,_Tmp6[1]=& _Tmp5;Cyc_Warn_err2(loc,_tag_fat(_Tmp6,sizeof(void*),2));});}else{
if(dl->tl!=0)
# 2695
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp4=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp5;_Tmp5.tag=0,_Tmp5.f1=_tag_fat("multiple designators are not yet supported",sizeof(char),43U);_Tmp5;});void*_Tmp5[1];_Tmp5[0]=& _Tmp4;Cyc_Warn_err2(loc,_tag_fat(_Tmp5,sizeof(void*),1));});else{
# 2698
void*_Tmp4=(void*)dl->hd;void*_Tmp5;if(*((int*)_Tmp4)==0){
# 2700
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp6=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp7;_Tmp7.tag=0,_Tmp7.f1=_tag_fat("array designator used in argument to ",sizeof(char),38U);_Tmp7;});struct Cyc_Warn_String_Warn_Warg_struct _Tmp7=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp8;_Tmp8.tag=0,_Tmp8.f1=aggr_str;_Tmp8;});void*_Tmp8[2];_Tmp8[0]=& _Tmp6,_Tmp8[1]=& _Tmp7;Cyc_Warn_err2(loc,_tag_fat(_Tmp8,sizeof(void*),2));});
goto _LL3;}else{_Tmp5=((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)_Tmp4)->f1;{struct _fat_ptr*v=_Tmp5;
# 2703
struct Cyc_List_List*fields2=fields;
for(1;fields2!=0;fields2=fields2->tl){
if(Cyc_strptrcmp(v,(*((struct _tuple25*)fields2->hd)).f0->name)==0){
if((*((struct _tuple25*)fields2->hd)).f1)
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp6=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp7;_Tmp7.tag=0,_Tmp7.f1=_tag_fat("member ",sizeof(char),8U);_Tmp7;});struct Cyc_Warn_String_Warn_Warg_struct _Tmp7=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp8;_Tmp8.tag=0,_Tmp8.f1=*v;_Tmp8;});struct Cyc_Warn_String_Warn_Warg_struct _Tmp8=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp9;_Tmp9.tag=0,_Tmp9.f1=_tag_fat(" has already been used as an argument",sizeof(char),38U);_Tmp9;});void*_Tmp9[3];_Tmp9[0]=& _Tmp6,_Tmp9[1]=& _Tmp7,_Tmp9[2]=& _Tmp8;Cyc_Warn_err2(loc,_tag_fat(_Tmp9,sizeof(void*),3));});
(*((struct _tuple25*)fields2->hd)).f1=1;
ans=({struct Cyc_List_List*_Tmp6=_region_malloc(rgn,0U,sizeof(struct Cyc_List_List));({struct _tuple27*_Tmp7=({struct _tuple27*_Tmp8=_region_malloc(rgn,0U,sizeof(struct _tuple27));_Tmp8->f0=(*((struct _tuple25*)fields2->hd)).f0,_Tmp8->f1=a;_Tmp8;});_Tmp6->hd=_Tmp7;}),_Tmp6->tl=ans;_Tmp6;});
break;}}
# 2712
if(fields2==0)
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp6=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp7;_Tmp7.tag=0,_Tmp7.f1=_tag_fat("bad field designator ",sizeof(char),22U);_Tmp7;});struct Cyc_Warn_String_Warn_Warg_struct _Tmp7=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp8;_Tmp8.tag=0,_Tmp8.f1=*v;_Tmp8;});void*_Tmp8[2];_Tmp8[0]=& _Tmp6,_Tmp8[1]=& _Tmp7;Cyc_Warn_err2(loc,_tag_fat(_Tmp8,sizeof(void*),2));});
goto _LL3;}}_LL3:;}}}}
# 2719
if((int)aggr_kind==0){
if(({int _Tmp1=Cyc_List_length(ans);_Tmp1 < Cyc_List_length(fields);}))
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp1=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp2;_Tmp2.tag=0,_Tmp2.f1=_tag_fat("too few arguments to struct",sizeof(char),28U);_Tmp2;});void*_Tmp2[1];_Tmp2[0]=& _Tmp1;Cyc_Warn_err2(loc,_tag_fat(_Tmp2,sizeof(void*),1));});}else{
if(Cyc_List_length(ans)!=1)
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp1=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp2;_Tmp2.tag=0,_Tmp2.f1=_tag_fat("union expression should have one member",sizeof(char),40U);_Tmp2;});void*_Tmp2[1];_Tmp2[0]=& _Tmp1;Cyc_Warn_err2(loc,_tag_fat(_Tmp2,sizeof(void*),1));});}{
struct Cyc_List_List*_Tmp1=Cyc_List_imp_rev(ans);_npop_handler(0);return _Tmp1;}}}
# 2669
;_pop_region();}
# 2730
int Cyc_Tcutil_is_zero_ptr_deref(struct Cyc_Absyn_Exp*e1,void**ptr_type,int*is_fat,void**elt_type){
# 2732
void*_Tmp0=e1->r;void*_Tmp1;switch(*((int*)_Tmp0)){case 14:
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp2=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp3;_Tmp3.tag=0,_Tmp3.f1=_tag_fat("we have a cast in a lhs: ",sizeof(char),26U);_Tmp3;});struct Cyc_Warn_Exp_Warn_Warg_struct _Tmp3=({struct Cyc_Warn_Exp_Warn_Warg_struct _Tmp4;_Tmp4.tag=4,_Tmp4.f1=e1;_Tmp4;});void*_Tmp4[2];_Tmp4[0]=& _Tmp2,_Tmp4[1]=& _Tmp3;({int(*_Tmp5)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_Tmp5;})(_tag_fat(_Tmp4,sizeof(void*),2));});case 20: _Tmp1=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e1a=_Tmp1;
_Tmp1=e1a;goto _LL6;}case 23: _Tmp1=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_LL6: {struct Cyc_Absyn_Exp*e1a=_Tmp1;
# 2736
return Cyc_Tcutil_is_zero_ptr_type(_check_null(e1a->topt),ptr_type,is_fat,elt_type);}case 22: _Tmp1=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e1a=_Tmp1;
_Tmp1=e1a;goto _LLA;}case 21: _Tmp1=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_LLA: {struct Cyc_Absyn_Exp*e1a=_Tmp1;
# 2740
if(Cyc_Tcutil_is_zero_ptr_type(_check_null(e1a->topt),ptr_type,is_fat,elt_type))
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp2=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp3;_Tmp3.tag=0,_Tmp3.f1=_tag_fat("found zero pointer aggregate member assignment: ",sizeof(char),49U);_Tmp3;});struct Cyc_Warn_Exp_Warn_Warg_struct _Tmp3=({struct Cyc_Warn_Exp_Warn_Warg_struct _Tmp4;_Tmp4.tag=4,_Tmp4.f1=e1;_Tmp4;});void*_Tmp4[2];_Tmp4[0]=& _Tmp2,_Tmp4[1]=& _Tmp3;({int(*_Tmp5)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_Tmp5;})(_tag_fat(_Tmp4,sizeof(void*),2));});
return 0;}case 13: _Tmp1=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e1a=_Tmp1;
_Tmp1=e1a;goto _LLE;}case 12: _Tmp1=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_LLE: {struct Cyc_Absyn_Exp*e1a=_Tmp1;
# 2745
if(Cyc_Tcutil_is_zero_ptr_type(_check_null(e1a->topt),ptr_type,is_fat,elt_type))
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp2=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp3;_Tmp3.tag=0,_Tmp3.f1=_tag_fat("found zero pointer instantiate/noinstantiate: ",sizeof(char),47U);_Tmp3;});struct Cyc_Warn_Exp_Warn_Warg_struct _Tmp3=({struct Cyc_Warn_Exp_Warn_Warg_struct _Tmp4;_Tmp4.tag=4,_Tmp4.f1=e1;_Tmp4;});void*_Tmp4[2];_Tmp4[0]=& _Tmp2,_Tmp4[1]=& _Tmp3;({int(*_Tmp5)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_Tmp5;})(_tag_fat(_Tmp4,sizeof(void*),2));});
return 0;}case 1:
 return 0;default:
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp2=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp3;_Tmp3.tag=0,_Tmp3.f1=_tag_fat("found bad lhs in is_zero_ptr_deref: ",sizeof(char),37U);_Tmp3;});struct Cyc_Warn_Exp_Warn_Warg_struct _Tmp3=({struct Cyc_Warn_Exp_Warn_Warg_struct _Tmp4;_Tmp4.tag=4,_Tmp4.f1=e1;_Tmp4;});void*_Tmp4[2];_Tmp4[0]=& _Tmp2,_Tmp4[1]=& _Tmp3;({int(*_Tmp5)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_Tmp5;})(_tag_fat(_Tmp4,sizeof(void*),2));});};}
# 2757
int Cyc_Tcutil_is_noalias_qual(void*aq,int must_be_unique){
void*_Tmp0=Cyc_Absyn_compress(aq);void*_Tmp1;enum Cyc_Absyn_AliasQualVal _Tmp2;if(*((int*)_Tmp0)==0)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1)){case 16: _Tmp2=((struct Cyc_Absyn_AqualConstCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1)->f1;{enum Cyc_Absyn_AliasQualVal aqv=_Tmp2;
# 2760
switch((int)aqv){case Cyc_Absyn_Unique_qual:
 return 1;case Cyc_Absyn_Refcnt_qual:
 goto _LLF;case Cyc_Absyn_Restricted_qual: _LLF:
 return !must_be_unique;default:
 return 0;};}case 17: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f2!=0)switch(*((int*)((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f2)->hd)){case 2: _Tmp1=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f2->tl;{struct Cyc_List_List*bnd=_Tmp1;
# 2768
_Tmp1=bnd;goto _LL6;}case 0: _Tmp1=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f2->tl;_LL6: {struct Cyc_List_List*bnd=_Tmp1;
# 2770
void*_Tmp3=(void*)_check_null(bnd)->hd;if(*((int*)_Tmp3)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp3)->f1)==16)switch((int)((struct Cyc_Absyn_AqualConstCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp3)->f1)->f1){case Cyc_Absyn_Unique_qual:
 return 1;case Cyc_Absyn_Refcnt_qual:
 goto _LL18;case Cyc_Absyn_Restricted_qual: _LL18:
 return !must_be_unique;default: goto _LL19;}else{goto _LL19;}}else{_LL19:
 return 0;};}default: goto _LL7;}else{goto _LL7;}default: goto _LL7;}else{_LL7:
# 2778
 return 0;};}
# 2783
int Cyc_Tcutil_is_noalias_pointer(struct Cyc_List_List*aqb,void*t,int must_be_unique){
void*_Tmp0=Cyc_Absyn_compress(t);void*_Tmp1;switch(*((int*)_Tmp0)){case 4: _Tmp1=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.aqual;{void*aq=_Tmp1;
# 2786
return Cyc_Tcutil_is_noalias_qual(aq,must_be_unique);}case 2: _Tmp1=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Tvar*tv=_Tmp1;
# 2788
void*_Tmp2=Cyc_Tcutil_lookup_aquals(aqb,t);return Cyc_Tcutil_is_noalias_qual(_Tmp2,must_be_unique);}default:
 return 0;};}
# 2792
int Cyc_Tcutil_is_noalias_pointer_or_aggr(struct Cyc_List_List*aqb,void*t){
t=Cyc_Absyn_compress(t);
if(Cyc_Tcutil_is_noalias_pointer(aqb,t,0))
return 1;{
union Cyc_Absyn_DatatypeFieldInfo _Tmp0;void*_Tmp1;void*_Tmp2;switch(*((int*)t)){case 0: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)t)->f1)){case 24: if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)t)->f1)->f1.KnownAggr.tag==2){_Tmp2=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)t)->f1)->f1.KnownAggr.val;_Tmp1=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)t)->f2;{struct Cyc_Absyn_Aggrdecl**adp=_Tmp2;struct Cyc_List_List*ts=_Tmp1;
# 2798
if((*adp)->impl==0)
return 0;{
struct Cyc_List_List*inst=Cyc_List_zip((*adp)->tvs,ts);
{struct Cyc_List_List*x=_check_null((*adp)->impl)->fields;for(0;x!=0;x=x->tl){
if(({struct Cyc_List_List*_Tmp3=aqb;Cyc_Tcutil_is_noalias_pointer_or_aggr(_Tmp3,Cyc_Tcutil_substitute(inst,((struct Cyc_Absyn_Aggrfield*)x->hd)->type));}))
return 1;}}
return 0;}}}else{
# 2810
goto _LL8;}case 22: _LL8:
 return 0;case 23: _Tmp0=((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)t)->f1)->f1;_Tmp2=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)t)->f2;{union Cyc_Absyn_DatatypeFieldInfo tinfo=_Tmp0;struct Cyc_List_List*ts=_Tmp2;
# 2814
void*_Tmp3;void*_Tmp4;if(tinfo.UnknownDatatypefield.tag==1)
return 0;else{_Tmp4=tinfo.KnownDatatypefield.val.f0;_Tmp3=tinfo.KnownDatatypefield.val.f1;{struct Cyc_Absyn_Datatypedecl*td=_Tmp4;struct Cyc_Absyn_Datatypefield*fld=_Tmp3;
# 2817
struct Cyc_List_List*inst=Cyc_List_zip(td->tvs,ts);
{struct Cyc_List_List*typs=fld->typs;for(0;typs!=0;typs=typs->tl){
t=inst==0?(*((struct _tuple14*)typs->hd)).f1: Cyc_Tcutil_substitute(inst,(*((struct _tuple14*)typs->hd)).f1);
if(Cyc_Tcutil_is_noalias_pointer_or_aggr(aqb,t))
return 1;}}
# 2823
return 0;}};}default: goto _LLB;}case 7: _Tmp2=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)t)->f3;{struct Cyc_List_List*x=_Tmp2;
# 2806
for(1;x!=0;x=x->tl){
if(Cyc_Tcutil_is_noalias_pointer_or_aggr(aqb,((struct Cyc_Absyn_Aggrfield*)x->hd)->type))
return 1;}
return 0;}default: _LLB:
# 2825
 return 0;};}}
# 2832
int Cyc_Tcutil_is_noalias_path(struct Cyc_List_List*aqb,struct Cyc_Absyn_Exp*e){
void*_Tmp0=e->r;void*_Tmp1;void*_Tmp2;switch(*((int*)_Tmp0)){case 1: if(*((int*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_Tmp0)->f1)==1)
return 0;else{goto _LL13;}case 22: _Tmp2=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e1=_Tmp2;
_Tmp2=e1;goto _LL6;}case 20: _Tmp2=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_LL6: {struct Cyc_Absyn_Exp*e1=_Tmp2;
# 2837
return Cyc_Tcutil_is_noalias_pointer(aqb,_check_null(e1->topt),1)&& Cyc_Tcutil_is_noalias_path(aqb,e1);}case 21: _Tmp2=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp1=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_Absyn_Exp*e1=_Tmp2;struct _fat_ptr*f=_Tmp1;
return Cyc_Tcutil_is_noalias_path(aqb,e1);}case 23: _Tmp2=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp1=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_Absyn_Exp*e1=_Tmp2;struct Cyc_Absyn_Exp*e2=_Tmp1;
return 0;}case 6: _Tmp2=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_Tmp1=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;{struct Cyc_Absyn_Exp*e1=_Tmp2;struct Cyc_Absyn_Exp*e2=_Tmp1;
# 2841
return Cyc_Tcutil_is_noalias_path(aqb,e1)&& Cyc_Tcutil_is_noalias_path(aqb,e2);}case 9: _Tmp2=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_Absyn_Exp*e2=_Tmp2;
_Tmp2=e2;goto _LL10;}case 14: _Tmp2=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_LL10: {struct Cyc_Absyn_Exp*e2=_Tmp2;
return Cyc_Tcutil_is_noalias_path(aqb,e2);}case 36: _Tmp2=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Stmt*s=_Tmp2;
# 2845
while(1){
{void*_Tmp3=s->r;void*_Tmp4;void*_Tmp5;switch(*((int*)_Tmp3)){case 2: _Tmp5=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_Tmp3)->f1;_Tmp4=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_Tmp3)->f2;{struct Cyc_Absyn_Stmt*s1=_Tmp5;struct Cyc_Absyn_Stmt*s2=_Tmp4;
s=s2;goto _LL15;}case 12: _Tmp5=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_Tmp3)->f1;_Tmp4=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_Tmp3)->f2;{struct Cyc_Absyn_Decl*d=_Tmp5;struct Cyc_Absyn_Stmt*s1=_Tmp4;
s=s1;goto _LL15;}case 1: _Tmp5=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_Tmp3)->f1;{struct Cyc_Absyn_Exp*e=_Tmp5;
return Cyc_Tcutil_is_noalias_path(aqb,e);}default:
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp6=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp7;_Tmp7.tag=0,_Tmp7.f1=_tag_fat("is_noalias_stmt_exp: ill-formed StmtExp",sizeof(char),40U);_Tmp7;});void*_Tmp7[1];_Tmp7[0]=& _Tmp6;({int(*_Tmp8)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_Tmp8;})(_tag_fat(_Tmp7,sizeof(void*),1));});}_LL15:;}
# 2846
1U;}}default: _LL13:
# 2852
 return 1;};}
# 2869 "tcutil.cyc"
struct _tuple13 Cyc_Tcutil_addressof_props(struct Cyc_Absyn_Exp*e){
# 2871
struct _tuple13 bogus_ans=({struct _tuple13 _Tmp0;_Tmp0.f0=0,_Tmp0.f1=Cyc_Absyn_heap_rgn_type;_Tmp0;});
void*_Tmp0=e->r;int _Tmp1;void*_Tmp2;void*_Tmp3;switch(*((int*)_Tmp0)){case 1: _Tmp3=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{void*x=_Tmp3;
# 2875
void*_Tmp4;switch(*((int*)x)){case 0:
 goto _LL11;case 2: _LL11:
 return bogus_ans;case 1: _Tmp4=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)x)->f1;{struct Cyc_Absyn_Vardecl*vd=_Tmp4;
# 2879
void*_Tmp5=Cyc_Absyn_compress(_check_null(e->topt));if(*((int*)_Tmp5)==5){
# 2881
struct _tuple13 _Tmp6;_Tmp6.f0=1,_Tmp6.f1=Cyc_Absyn_heap_rgn_type;return _Tmp6;}else{
struct _tuple13 _Tmp6;_Tmp6.f0=vd->tq.real_const,_Tmp6.f1=Cyc_Absyn_heap_rgn_type;return _Tmp6;};}case 4: _Tmp4=((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)x)->f1;{struct Cyc_Absyn_Vardecl*vd=_Tmp4;
# 2885
void*_Tmp5=Cyc_Absyn_compress(_check_null(e->topt));if(*((int*)_Tmp5)==5){
struct _tuple13 _Tmp6;_Tmp6.f0=1,_Tmp6.f1=_check_null(vd->rgn);return _Tmp6;}else{
# 2888
vd->escapes=1;{
struct _tuple13 _Tmp6;_Tmp6.f0=vd->tq.real_const,_Tmp6.f1=_check_null(vd->rgn);return _Tmp6;}};}case 5: _Tmp4=((struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*)x)->f1;{struct Cyc_Absyn_Vardecl*vd=_Tmp4;
# 2891
_Tmp4=vd;goto _LL19;}default: _Tmp4=((struct Cyc_Absyn_Param_b_Absyn_Binding_struct*)x)->f1;_LL19: {struct Cyc_Absyn_Vardecl*vd=_Tmp4;
# 2893
vd->escapes=1;{
struct _tuple13 _Tmp5;_Tmp5.f0=vd->tq.real_const,_Tmp5.f1=_check_null(vd->rgn);return _Tmp5;}}};}case 21: _Tmp3=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp2=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_Tmp1=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;{struct Cyc_Absyn_Exp*e1=_Tmp3;struct _fat_ptr*f=_Tmp2;int is_tagged=_Tmp1;
# 2898
if(is_tagged)return bogus_ans;{
# 2901
void*_Tmp4=Cyc_Absyn_compress(_check_null(e1->topt));void*_Tmp5;switch(*((int*)_Tmp4)){case 7: _Tmp5=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_Tmp4)->f3;{struct Cyc_List_List*fs=_Tmp5;
# 2903
struct Cyc_Absyn_Aggrfield*finfo=Cyc_Absyn_lookup_field(fs,f);
if(finfo!=0 && finfo->width==0){
struct _tuple13 _Tmp6=Cyc_Tcutil_addressof_props(e1);void*_Tmp7;int _Tmp8;_Tmp8=_Tmp6.f0;_Tmp7=_Tmp6.f1;{int c=_Tmp8;void*t=_Tmp7;
struct _tuple13 _Tmp9;_Tmp9.f0=finfo->tq.real_const || c,_Tmp9.f1=t;return _Tmp9;}}
# 2908
return bogus_ans;}case 0: if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp4)->f1)==24){if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp4)->f1)->f1.KnownAggr.tag==2){_Tmp5=*((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp4)->f1)->f1.KnownAggr.val;{struct Cyc_Absyn_Aggrdecl*ad=_Tmp5;
# 2910
struct Cyc_Absyn_Aggrfield*finfo=Cyc_Absyn_lookup_decl_field(ad,f);
if(finfo!=0 && finfo->width==0){
struct _tuple13 _Tmp6=Cyc_Tcutil_addressof_props(e1);void*_Tmp7;int _Tmp8;_Tmp8=_Tmp6.f0;_Tmp7=_Tmp6.f1;{int c=_Tmp8;void*t=_Tmp7;
struct _tuple13 _Tmp9;_Tmp9.f0=finfo->tq.real_const || c,_Tmp9.f1=t;return _Tmp9;}}
# 2915
return bogus_ans;}}else{goto _LL29;}}else{goto _LL29;}default: _LL29:
 return bogus_ans;};}}case 22: _Tmp3=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp2=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_Tmp1=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;{struct Cyc_Absyn_Exp*e1=_Tmp3;struct _fat_ptr*f=_Tmp2;int is_tagged=_Tmp1;
# 2920
if(is_tagged)return bogus_ans;{
# 2923
void*_Tmp4=Cyc_Absyn_compress(_check_null(e1->topt));void*_Tmp5;void*_Tmp6;if(*((int*)_Tmp4)==4){_Tmp6=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp4)->f1.elt_type;_Tmp5=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp4)->f1.ptr_atts.eff;{void*t1=_Tmp6;void*r=_Tmp5;
# 2925
struct Cyc_Absyn_Aggrfield*finfo;
{void*_Tmp7=Cyc_Absyn_compress(t1);void*_Tmp8;switch(*((int*)_Tmp7)){case 7: _Tmp8=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_Tmp7)->f3;{struct Cyc_List_List*fs=_Tmp8;
# 2928
finfo=Cyc_Absyn_lookup_field(fs,f);goto _LL36;}case 0: if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp7)->f1)==24){if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp7)->f1)->f1.KnownAggr.tag==2){_Tmp8=*((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp7)->f1)->f1.KnownAggr.val;{struct Cyc_Absyn_Aggrdecl*ad=_Tmp8;
# 2930
finfo=Cyc_Absyn_lookup_decl_field(ad,f);goto _LL36;}}else{goto _LL3B;}}else{goto _LL3B;}default: _LL3B:
 return bogus_ans;}_LL36:;}
# 2933
if(finfo!=0 && finfo->width==0){
struct _tuple13 _Tmp7;_Tmp7.f0=finfo->tq.real_const,_Tmp7.f1=r;return _Tmp7;}
return bogus_ans;}}else{
return bogus_ans;};}}case 20: _Tmp3=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e1=_Tmp3;
# 2940
void*_Tmp4=Cyc_Absyn_compress(_check_null(e1->topt));void*_Tmp5;struct Cyc_Absyn_Tqual _Tmp6;if(*((int*)_Tmp4)==4){_Tmp6=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp4)->f1.elt_tq;_Tmp5=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp4)->f1.ptr_atts.eff;{struct Cyc_Absyn_Tqual tq=_Tmp6;void*r=_Tmp5;
# 2942
struct _tuple13 _Tmp7;_Tmp7.f0=tq.real_const,_Tmp7.f1=r;return _Tmp7;}}else{
return bogus_ans;};}case 23: _Tmp3=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp2=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_Absyn_Exp*e1=_Tmp3;struct Cyc_Absyn_Exp*e2=_Tmp2;
# 2948
void*t=Cyc_Absyn_compress(_check_null(e1->topt));
void*_Tmp4;struct Cyc_Absyn_Tqual _Tmp5;switch(*((int*)t)){case 4: _Tmp5=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)t)->f1.elt_tq;_Tmp4=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)t)->f1.ptr_atts.eff;{struct Cyc_Absyn_Tqual tq=_Tmp5;void*r=_Tmp4;
# 2951
struct _tuple13 _Tmp6;_Tmp6.f0=tq.real_const,_Tmp6.f1=r;return _Tmp6;}case 5: _Tmp5=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)t)->f1.tq;{struct Cyc_Absyn_Tqual tq=_Tmp5;
# 2957
struct _tuple13 _Tmp6;_Tmp6.f0=tq.real_const,({void*_Tmp7=Cyc_Tcutil_addressof_props(e1).f1;_Tmp6.f1=_Tmp7;});return _Tmp6;}default:
 return bogus_ans;};}default:
# 2961
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp4=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp5;_Tmp5.tag=0,_Tmp5.f1=_tag_fat("unary & applied to non-lvalue",sizeof(char),30U);_Tmp5;});void*_Tmp5[1];_Tmp5[0]=& _Tmp4;Cyc_Warn_err2(e->loc,_tag_fat(_Tmp5,sizeof(void*),1));});
return bogus_ans;};}
# 2968
void Cyc_Tcutil_check_bound(unsigned loc,unsigned i,void*b,int do_warn){
if(Cyc_Tcutil_is_cvar_type(b)){
if(i > 1U){
if(!({unsigned _Tmp0=loc;Cyc_BansheeIf_add_constraint(_Tmp0,({void*_Tmp1=_check_null(Cyc_Tcutil_ptrbnd_cvar_equivalent(b));Cyc_BansheeIf_equality_constraint(_Tmp1,
Cyc_Absyn_fatconst());}));}))
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp0=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp1;_Tmp1.tag=0,_Tmp1.f1=_tag_fat("cannot infer an appropriate bound",sizeof(char),34U);_Tmp1;});void*_Tmp1[1];_Tmp1[0]=& _Tmp0;((do_warn?Cyc_Warn_warn2: Cyc_Warn_err2))(loc,_tag_fat(_Tmp1,sizeof(void*),1));});}
# 2976
return;}{
# 2978
struct Cyc_Absyn_Exp*eopt=({void*_Tmp0=Cyc_Absyn_bounds_one();Cyc_Tcutil_get_bounds_exp(_Tmp0,b);});
if(eopt==0)return;{
struct _tuple12 _Tmp0=Cyc_Evexp_eval_const_uint_exp(eopt);int _Tmp1;unsigned _Tmp2;_Tmp2=_Tmp0.f0;_Tmp1=_Tmp0.f1;{unsigned j=_Tmp2;int known=_Tmp1;
if(known && j <= i)
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp3=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp4;_Tmp4.tag=0,_Tmp4.f1=_tag_fat("a dereference is out of bounds: ",sizeof(char),33U);_Tmp4;});struct Cyc_Warn_Int_Warn_Warg_struct _Tmp4=({struct Cyc_Warn_Int_Warn_Warg_struct _Tmp5;_Tmp5.tag=12,_Tmp5.f1=(int)j;_Tmp5;});struct Cyc_Warn_String_Warn_Warg_struct _Tmp5=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp6;_Tmp6.tag=0,_Tmp6.f1=
_tag_fat(" <= ",sizeof(char),5U);_Tmp6;});struct Cyc_Warn_Int_Warn_Warg_struct _Tmp6=({struct Cyc_Warn_Int_Warn_Warg_struct _Tmp7;_Tmp7.tag=12,_Tmp7.f1=(int)i;_Tmp7;});void*_Tmp7[4];_Tmp7[0]=& _Tmp3,_Tmp7[1]=& _Tmp4,_Tmp7[2]=& _Tmp5,_Tmp7[3]=& _Tmp6;((
# 2982
do_warn?Cyc_Warn_warn2: Cyc_Warn_err2))(loc,_tag_fat(_Tmp7,sizeof(void*),4));});
# 2984
return;}}}}
# 2987
void Cyc_Tcutil_check_nonzero_bound(unsigned loc,void*b){
Cyc_Tcutil_check_bound(loc,0U,b,0);}
# 2995
static int Cyc_Tcutil_cnst_exp(int var_okay,int valueof_okay,struct Cyc_Absyn_Exp*e){
int(*rec)(int,int,struct Cyc_Absyn_Exp*)=Cyc_Tcutil_cnst_exp;
void*_Tmp0=e->r;void*_Tmp1;void*_Tmp2;void*_Tmp3;switch(*((int*)_Tmp0)){case 0:
 goto _LL4;case 2: _LL4:
 goto _LL6;case 17: _LL6:
 goto _LL8;case 18: _LL8:
 goto _LLA;case 19: _LLA:
 goto _LLC;case 31: _LLC:
 goto _LLE;case 32: _LLE:
 return 1;case 1: _Tmp3=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{void*b=_Tmp3;
# 3008
void*_Tmp4;switch(*((int*)b)){case 2:
 return 1;case 1: _Tmp4=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)b)->f1;{struct Cyc_Absyn_Vardecl*vd=_Tmp4;
# 3011
void*_Tmp5=Cyc_Absyn_compress(vd->type);switch(*((int*)_Tmp5)){case 5:
 goto _LL42;case 6: _LL42:
 return 1;default:
 return var_okay;};}case 4: _Tmp4=((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)b)->f1;{struct Cyc_Absyn_Vardecl*vd=_Tmp4;
# 3017
if((int)vd->sc!=0)
return var_okay;{
void*_Tmp5=Cyc_Absyn_compress(vd->type);switch(*((int*)_Tmp5)){case 5:
 goto _LL49;case 6: _LL49:
 return 1;default:
 return var_okay;};}}case 0:
# 3024
 return 0;default:
 return var_okay;};}case 6: _Tmp3=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp2=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_Tmp1=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;{struct Cyc_Absyn_Exp*e1=_Tmp3;struct Cyc_Absyn_Exp*e2=_Tmp2;struct Cyc_Absyn_Exp*e3=_Tmp1;
# 3028
return(rec(0,valueof_okay,e1)&& rec(0,valueof_okay,e2))&&
 rec(0,valueof_okay,e3);}case 9: _Tmp3=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_Tmp2=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_Absyn_Exp*e1=_Tmp3;struct Cyc_Absyn_Exp*e2=_Tmp2;
# 3031
return rec(0,valueof_okay,e1)&& rec(0,valueof_okay,e2);}case 40: _Tmp3=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e2=_Tmp3;
_Tmp3=e2;goto _LL18;}case 12: _Tmp3=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_LL18: {struct Cyc_Absyn_Exp*e2=_Tmp3;
_Tmp3=e2;goto _LL1A;}case 13: _Tmp3=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_LL1A: {struct Cyc_Absyn_Exp*e2=_Tmp3;
_Tmp3=e2;goto _LL1C;}case 14: _Tmp3=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_LL1C: {struct Cyc_Absyn_Exp*e2=_Tmp3;
# 3037
return Cyc_Tcutil_cnst_exp(var_okay,valueof_okay,e2);}case 15: _Tmp3=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e2=_Tmp3;
return rec(1,valueof_okay,e2);}case 26: _Tmp3=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;_Tmp2=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;{struct Cyc_Absyn_Exp*e1=_Tmp3;struct Cyc_Absyn_Exp*e2=_Tmp2;
# 3040
return rec(0,valueof_okay,e1)&& rec(0,valueof_okay,e2);}case 27: _Tmp3=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Exp*e=_Tmp3;
return rec(0,valueof_okay,e);}case 25: _Tmp3=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_List_List*des=_Tmp3;
_Tmp3=des;goto _LL26;}case 29: _Tmp3=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;_LL26: {struct Cyc_List_List*des=_Tmp3;
_Tmp3=des;goto _LL28;}case 28: _Tmp3=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_Tmp0)->f3;_LL28: {struct Cyc_List_List*des=_Tmp3;
# 3045
for(1;des!=0;des=des->tl){
if(!Cyc_Tcutil_cnst_exp(0,valueof_okay,(*((struct _tuple16*)des->hd)).f1))
return 0;}
return 1;}case 3: _Tmp3=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_Tmp0)->f2;{struct Cyc_List_List*es=_Tmp3;
_Tmp3=es;goto _LL2C;}case 30: _Tmp3=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;_LL2C: {struct Cyc_List_List*es=_Tmp3;
# 3051
for(1;es!=0;es=es->tl){
if(!Cyc_Tcutil_cnst_exp(0,valueof_okay,(struct Cyc_Absyn_Exp*)es->hd))
return 0;}
return 1;}case 38:
 return valueof_okay;case 36: _Tmp3=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_Tmp0)->f1;{struct Cyc_Absyn_Stmt*s=_Tmp3;
# 3057
void*_Tmp4=s->r;void*_Tmp5;if(*((int*)_Tmp4)==1){_Tmp5=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_Tmp4)->f1;{struct Cyc_Absyn_Exp*e=_Tmp5;
return Cyc_Tcutil_cnst_exp(var_okay,valueof_okay,e);}}else{
return 0;};}default:
# 3061
 return 0;};}
# 3064
int Cyc_Tcutil_is_const_exp(struct Cyc_Absyn_Exp*e){
return Cyc_Tcutil_cnst_exp(0,0,e);}
# 3067
int Cyc_Tcutil_is_pure_exp(struct Cyc_Absyn_Exp*e){
return Cyc_Tcutil_cnst_exp(0,1,e);}
# 3071
static int Cyc_Tcutil_fields_zeroable(struct Cyc_List_List*,struct Cyc_List_List*,struct Cyc_List_List*);
int Cyc_Tcutil_zeroable_type(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);void*_Tmp1;void*_Tmp2;switch(*((int*)_Tmp0)){case 0: _Tmp2=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1;_Tmp1=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f2;{void*c=_Tmp2;struct Cyc_List_List*ts=_Tmp1;
# 3075
union Cyc_Absyn_AggrInfo _Tmp3;void*_Tmp4;switch(*((int*)c)){case 0:
 goto _LLF;case 1: _LLF:
 goto _LL11;case 3: _LL11:
 goto _LL13;case 2: _LL13:
 return 1;case 19: _Tmp4=((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)c)->f2;{struct Cyc_Absyn_Enumdecl*edo=_Tmp4;
# 3081
if(edo==0 || edo->fields==0)
return 0;
_Tmp4=(struct Cyc_List_List*)edo->fields->v;goto _LL17;}case 20: _Tmp4=((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)c)->f1;_LL17: {struct Cyc_List_List*fs=_Tmp4;
# 3085
{struct Cyc_List_List*fs2=fs;for(0;fs2!=0;fs2=fs2->tl){
if(((struct Cyc_Absyn_Enumfield*)fs2->hd)->tag==0)
return fs2==fs;{
struct _tuple12 _Tmp5=Cyc_Evexp_eval_const_uint_exp(((struct Cyc_Absyn_Enumfield*)fs2->hd)->tag);int _Tmp6;unsigned _Tmp7;_Tmp7=_Tmp5.f0;_Tmp6=_Tmp5.f1;{unsigned i=_Tmp7;int known=_Tmp6;
if(known && i==0U)
return 1;}}}}
# 3092
return 0;}case 24: _Tmp3=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)c)->f1;{union Cyc_Absyn_AggrInfo info=_Tmp3;
# 3095
struct Cyc_Absyn_Aggrdecl*ad=Cyc_Absyn_get_known_aggrdecl(info);
if(ad->impl==0)return 0;
if(ad->impl->exist_vars!=0)return 0;
if((int)ad->kind==1 && ad->impl->tagged)return 0;
return Cyc_Tcutil_fields_zeroable(ad->tvs,ts,ad->impl->fields);}default:
 return 0;};}case 4: _Tmp2=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1.ptr_atts.nullable;{void*n=_Tmp2;
# 3103
return Cyc_Tcutil_force_type2bool(1,n);}case 5: _Tmp2=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.elt_type;{void*t=_Tmp2;
return Cyc_Tcutil_zeroable_type(t);}case 7: _Tmp2=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_Tmp0)->f3;{struct Cyc_List_List*fs=_Tmp2;
return Cyc_Tcutil_fields_zeroable(0,0,fs);}default:
 return 0;};}
# 3109
static int Cyc_Tcutil_fields_zeroable(struct Cyc_List_List*tvs,struct Cyc_List_List*ts,struct Cyc_List_List*fs){
# 3111
struct _RegionHandle _Tmp0=_new_region(0U,"rgn");struct _RegionHandle*rgn=& _Tmp0;_push_region(rgn);
{struct Cyc_List_List*inst=Cyc_List_rzip(rgn,rgn,tvs,ts);
for(1;fs!=0;fs=fs->tl){
void*t=((struct Cyc_Absyn_Aggrfield*)fs->hd)->type;
if(Cyc_Tcutil_zeroable_type(t))continue;
t=Cyc_Tcutil_rsubstitute(rgn,inst,((struct Cyc_Absyn_Aggrfield*)fs->hd)->type);
if(!Cyc_Tcutil_zeroable_type(t)){int _Tmp1=0;_npop_handler(0);return _Tmp1;}}{
# 3119
int _Tmp1=1;_npop_handler(0);return _Tmp1;}}
# 3112
;_pop_region();}
# 3123
void Cyc_Tcutil_check_no_qual(unsigned loc,void*t){
void*_Tmp0;if(*((int*)t)==8){_Tmp0=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)t)->f3;{struct Cyc_Absyn_Typedefdecl*tdopt=_Tmp0;
# 3126
if(tdopt==0)
return;{
struct Cyc_Absyn_Tqual tq=tdopt->tq;
if(((tq.print_const || tq.q_volatile)|| tq.q_restrict)|| tq.real_const)
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp1=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp2;_Tmp2.tag=0,_Tmp2.f1=_tag_fat("qualifier within typedef type ",sizeof(char),31U);_Tmp2;});struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp2=({struct Cyc_Warn_Typ_Warn_Warg_struct _Tmp3;_Tmp3.tag=2,_Tmp3.f1=(void*)t;_Tmp3;});struct Cyc_Warn_String_Warn_Warg_struct _Tmp3=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp4;_Tmp4.tag=0,_Tmp4.f1=_tag_fat(" is ignored",sizeof(char),12U);_Tmp4;});void*_Tmp4[3];_Tmp4[0]=& _Tmp1,_Tmp4[1]=& _Tmp2,_Tmp4[2]=& _Tmp3;Cyc_Warn_warn2(loc,_tag_fat(_Tmp4,sizeof(void*),3));});
return;}}}else{
return;};}
# 3139
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_type_bound(void*t){
void*_Tmp0=Cyc_Absyn_compress(t);void*_Tmp1;struct Cyc_Absyn_PtrInfo _Tmp2;switch(*((int*)_Tmp0)){case 4: _Tmp2=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_Tmp0)->f1;{struct Cyc_Absyn_PtrInfo pi=_Tmp2;
void*_Tmp3=Cyc_Absyn_bounds_one();return Cyc_Tcutil_get_bounds_exp_constrain(_Tmp3,pi.ptr_atts.bounds,1);}case 5: _Tmp1=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.num_elts;{struct Cyc_Absyn_Exp*e=_Tmp1;
return e;}default:
 return 0;};}
# 3149
struct Cyc_Absyn_Vardecl*Cyc_Tcutil_nonesc_vardecl(void*b){
void*_Tmp0;switch(*((int*)b)){case 5: _Tmp0=((struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*)b)->f1;{struct Cyc_Absyn_Vardecl*x=_Tmp0;
_Tmp0=x;goto _LL4;}case 4: _Tmp0=((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)b)->f1;_LL4: {struct Cyc_Absyn_Vardecl*x=_Tmp0;
_Tmp0=x;goto _LL6;}case 3: _Tmp0=((struct Cyc_Absyn_Param_b_Absyn_Binding_struct*)b)->f1;_LL6: {struct Cyc_Absyn_Vardecl*x=_Tmp0;
_Tmp0=x;goto _LL8;}case 1: _Tmp0=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)b)->f1;_LL8: {struct Cyc_Absyn_Vardecl*x=_Tmp0;
if(x->escapes)return 0;else{return x;}}default:
 return 0;};}
# 3160
struct Cyc_List_List*Cyc_Tcutil_filter_nulls(struct Cyc_List_List*l){
struct Cyc_List_List*res=0;
for(1;l!=0;l=l->tl){
if((void**)l->hd!=0)res=({struct Cyc_List_List*_Tmp0=_cycalloc(sizeof(struct Cyc_List_List));_Tmp0->hd=*((void**)l->hd),_Tmp0->tl=res;_Tmp0;});}
return res;}
# 3167
void*Cyc_Tcutil_promote_array(void*t,void*rgn,void*aqual,int convert_tag){
void*_Tmp0=Cyc_Absyn_compress(t);void*_Tmp1;void*_Tmp2;struct Cyc_Absyn_Tqual _Tmp3;void*_Tmp4;if(*((int*)_Tmp0)==5){_Tmp4=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.elt_type;_Tmp3=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.tq;_Tmp2=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.num_elts;_Tmp1=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_Tmp0)->f1.zero_term;{void*et=_Tmp4;struct Cyc_Absyn_Tqual tq=_Tmp3;struct Cyc_Absyn_Exp*eopt=_Tmp2;void*zt=_Tmp1;
# 3170
void*b;
if(eopt==0)
b=Cyc_Absyn_fat_bound_type;else{
if(convert_tag){
if(eopt->topt==0)
({struct Cyc_Warn_String_Warn_Warg_struct _Tmp5=({struct Cyc_Warn_String_Warn_Warg_struct _Tmp6;_Tmp6.tag=0,_Tmp6.f1=_tag_fat("cannot convert tag without type!",sizeof(char),33U);_Tmp6;});void*_Tmp6[1];_Tmp6[0]=& _Tmp5;({int(*_Tmp7)(struct _fat_ptr)=(int(*)(struct _fat_ptr))Cyc_Warn_impos2;_Tmp7;})(_tag_fat(_Tmp6,sizeof(void*),1));});{
void*_Tmp5=Cyc_Absyn_compress(eopt->topt);void*_Tmp6;if(*((int*)_Tmp5)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp5)->f1)==5){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp5)->f2!=0){_Tmp6=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp5)->f2->hd;{void*t=_Tmp6;
# 3179
b=Cyc_Absyn_thin_bounds_exp(Cyc_Absyn_valueof_exp(t,0U));
goto _LL5;}}else{goto _LL8;}}else{goto _LL8;}}else{_LL8:
 b=Cyc_Tcutil_is_const_exp(eopt)?Cyc_Absyn_thin_bounds_exp(eopt): Cyc_Absyn_fat_bound_type;}_LL5:;}}else{
# 3184
b=Cyc_Absyn_thin_bounds_exp(eopt);}}
return Cyc_Absyn_atb_type(et,rgn,aqual,tq,b,zt,Cyc_Absyn_false_type);}}else{
return t;};}
# 3190
static int Cyc_Tcutil_aquals_bnds_pred(struct _tuple15*bnd){
void*_Tmp0=(*bnd).f0;if(*((int*)_Tmp0)==0){if(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_Tmp0)->f1)==15)
# 3193
return 1;else{goto _LL3;}}else{_LL3:
# 3195
 return 0;};}
# 3199
struct Cyc_List_List*Cyc_Tcutil_get_aquals_bounds(struct Cyc_Absyn_Fndecl*fd){
# 3201
return fd->i.qual_bnd;}
# 3205
void*Cyc_Tcutil_eval_aquals_of(struct Cyc_List_List*aqb,void*t){
void*_Tmp0;switch(*((int*)t)){case 4: _Tmp0=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)t)->f1.ptr_atts.aqual;{void*aq=_Tmp0;
# 3208
return aq;}case 2:
# 3210
 return Cyc_Tcutil_lookup_aquals(aqb,t);default:
# 3212
 return 0;};}struct _tuple28{struct Cyc_Absyn_Tvar*f0;int f1;};
# 3216
struct _tuple28*Cyc_Tcutil_tvar_bool_pair(int b,struct Cyc_Absyn_Tvar*t){
struct _tuple28*_Tmp0=_cycalloc(sizeof(struct _tuple28));_Tmp0->f0=t,_Tmp0->f1=b;return _Tmp0;}
# 3223
static void Cyc_Tcutil_visit_for_cvars(void*t,void(*fn)(void**,void*),void*env){
void*_Tmp0;void*_Tmp1;switch(*((int*)t)){case 4: _Tmp1=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)t)->f1.elt_type;_Tmp0=(void**)&((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)t)->f1.ptr_atts.bounds;{void*ta=_Tmp1;void**b=(void**)_Tmp0;
# 3226
fn(b,env);
Cyc_Tcutil_visit_for_cvars(ta,fn,env);
return;}case 6: _Tmp1=(struct Cyc_Absyn_FnInfo*)&((struct Cyc_Absyn_FnType_Absyn_Type_struct*)t)->f1;{struct Cyc_Absyn_FnInfo*i=_Tmp1;
# 3230
Cyc_Tcutil_visit_for_cvars(i->ret_type,fn,env);
{struct Cyc_List_List*args=i->args;for(0;(unsigned)args;args=args->tl){
Cyc_Tcutil_visit_for_cvars(((struct _tuple9*)args->hd)->f2,fn,env);}}
# 3234
return;}case 0: _Tmp1=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)t)->f2;{struct Cyc_List_List*ts=_Tmp1;
_Tmp1=ts;goto _LL8;}case 8: _Tmp1=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)t)->f2;_LL8: {struct Cyc_List_List*ts=_Tmp1;
# 3237
for(1;(unsigned)ts;ts=ts->tl){Cyc_Tcutil_visit_for_cvars((void*)ts->hd,fn,env);}
return;}case 7: if(((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)t)->f2==1){_Tmp1=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)t)->f3;{struct Cyc_List_List*fields=_Tmp1;
# 3240
for(1;(unsigned)fields;fields=fields->tl){
Cyc_Tcutil_visit_for_cvars(((struct Cyc_Absyn_Aggrfield*)fields->hd)->type,fn,env);}
# 3243
return;}}else{goto _LLB;}default: _LLB:
# 3245
 return;};}
# 3249
static void Cyc_Tcutil_introduce_cvar_fn(void**b,void*env){
if(*_check_null(b)==env)
({void*_Tmp0=Cyc_Absyn_cvar_type(& Cyc_Kinds_ptrbko);*b=_Tmp0;});}
# 3255
static void Cyc_Tcutil_replace_cvar_fn(void**b,void*env){
if(Cyc_Tcutil_is_cvar_type(*_check_null(b)))
*b=env;}
# 3261
void Cyc_Tcutil_introduce_cvars(void*t){
({void*_Tmp0=t;Cyc_Tcutil_visit_for_cvars(_Tmp0,Cyc_Tcutil_introduce_cvar_fn,Cyc_Absyn_bounds_one());});}
# 3265
void Cyc_Tcutil_replace_cvars(void*t,void*with){
Cyc_Tcutil_visit_for_cvars(t,Cyc_Tcutil_replace_cvar_fn,with);}
# 3270
int Cyc_Tcutil_is_main(struct _tuple1*n){
void*_Tmp0;union Cyc_Absyn_Nmspace _Tmp1;_Tmp1=n->f0;_Tmp0=n->f1;{union Cyc_Absyn_Nmspace nms=_Tmp1;struct _fat_ptr*v=_Tmp0;
if(nms.Abs_n.tag==2){if(nms.Abs_n.val==0)
return Cyc_strcmp(*v,_tag_fat("main",sizeof(char),5U))==0;else{goto _LL6;}}else{_LL6:
 return 0;};}}
# 3278
static void Cyc_Tcutil_deconstruct_tycon(void*tc){
switch(*((int*)tc)){case 3:
 Cyc_fprintf(Cyc_stderr,_tag_fat("ComplexCon",sizeof(char),11U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 0:
 Cyc_fprintf(Cyc_stderr,_tag_fat("VoidCon",sizeof(char),8U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 1:
 Cyc_fprintf(Cyc_stderr,_tag_fat("IntCon",sizeof(char),7U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 2:
 Cyc_fprintf(Cyc_stderr,_tag_fat("FloatCon",sizeof(char),9U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 4:
 Cyc_fprintf(Cyc_stderr,_tag_fat("RgnHandleCon",sizeof(char),13U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 18:
 Cyc_fprintf(Cyc_stderr,_tag_fat("AqualHandleCon",sizeof(char),15U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 5:
 Cyc_fprintf(Cyc_stderr,_tag_fat("TagCon",sizeof(char),7U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 6:
 Cyc_fprintf(Cyc_stderr,_tag_fat("HeapCon",sizeof(char),8U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 7:
 Cyc_fprintf(Cyc_stderr,_tag_fat("UniqueCon",sizeof(char),10U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 8:
 Cyc_fprintf(Cyc_stderr,_tag_fat("RefCntCon",sizeof(char),10U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 9:
 Cyc_fprintf(Cyc_stderr,_tag_fat("JoinCon",sizeof(char),8U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 10:
 Cyc_fprintf(Cyc_stderr,_tag_fat("RgnsCon",sizeof(char),8U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 11:
 Cyc_fprintf(Cyc_stderr,_tag_fat("TrueCon",sizeof(char),8U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 12:
 Cyc_fprintf(Cyc_stderr,_tag_fat("FalseCon",sizeof(char),9U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 13:
 Cyc_fprintf(Cyc_stderr,_tag_fat("ThinCon",sizeof(char),8U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 14:
 Cyc_fprintf(Cyc_stderr,_tag_fat("FatCon",sizeof(char),7U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 19:
 Cyc_fprintf(Cyc_stderr,_tag_fat("EnumCon",sizeof(char),8U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 20:
 Cyc_fprintf(Cyc_stderr,_tag_fat("AnonEnumCon",sizeof(char),12U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 21:
 Cyc_fprintf(Cyc_stderr,_tag_fat("BuiltinCon",sizeof(char),11U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 22:
 Cyc_fprintf(Cyc_stderr,_tag_fat("DatatypeCon",sizeof(char),12U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 23:
 Cyc_fprintf(Cyc_stderr,_tag_fat("DatatypeFieldCon",sizeof(char),17U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 24:
 Cyc_fprintf(Cyc_stderr,_tag_fat("AggrCon",sizeof(char),8U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 16:
 Cyc_fprintf(Cyc_stderr,_tag_fat("AqualsConstCon",sizeof(char),15U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 15:
 Cyc_fprintf(Cyc_stderr,_tag_fat("AqualsCon",sizeof(char),10U),_tag_fat(0U,sizeof(void*),0));goto _LL0;default:
 Cyc_fprintf(Cyc_stderr,_tag_fat("AqualVarCon",sizeof(char),12U),_tag_fat(0U,sizeof(void*),0));goto _LL0;}_LL0:;}
# 3308
void Cyc_Tcutil_deconstruct_type(void*t){
({struct Cyc_Int_pa_PrintArg_struct _Tmp0=({struct Cyc_Int_pa_PrintArg_struct _Tmp1;_Tmp1.tag=1,_Tmp1.f1=(unsigned)t;_Tmp1;});struct Cyc_String_pa_PrintArg_struct _Tmp1=({struct Cyc_String_pa_PrintArg_struct _Tmp2;_Tmp2.tag=0,({struct _fat_ptr _Tmp3=Cyc_Absynpp_typ2string(t);_Tmp2.f1=_Tmp3;});_Tmp2;});void*_Tmp2[2];_Tmp2[0]=& _Tmp0,_Tmp2[1]=& _Tmp1;Cyc_fprintf(Cyc_stderr,_tag_fat("Type 0x%x %s is ...",sizeof(char),20U),_tag_fat(_Tmp2,sizeof(void*),2));});{
void*_Tmp0;void*_Tmp1;const char*_Tmp2;const char*_Tmp3;int _Tmp4;void*_Tmp5;void*_Tmp6;switch(*((int*)t)){case 0: _Tmp6=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)t)->f1;_Tmp5=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)t)->f2;{void*tc=_Tmp6;struct Cyc_List_List*ts=_Tmp5;
# 3312
Cyc_fprintf(Cyc_stderr,_tag_fat(" AppType with constructor ",sizeof(char),27U),_tag_fat(0U,sizeof(void*),0));Cyc_Tcutil_deconstruct_tycon(tc);
Cyc_fprintf(Cyc_stderr,_tag_fat("and args <",sizeof(char),11U),_tag_fat(0U,sizeof(void*),0));
while((unsigned)ts){
Cyc_Tcutil_deconstruct_type((void*)ts->hd);
ts=ts->tl;
# 3315
1U;}
# 3318
Cyc_fprintf(Cyc_stderr,_tag_fat(">\n",sizeof(char),3U),_tag_fat(0U,sizeof(void*),0));
goto _LL0;}case 3: _Tmp6=(void*)((struct Cyc_Absyn_Cvar_Absyn_Type_struct*)t)->f2;_Tmp4=((struct Cyc_Absyn_Cvar_Absyn_Type_struct*)t)->f3;_Tmp3=((struct Cyc_Absyn_Cvar_Absyn_Type_struct*)t)->f5;_Tmp2=((struct Cyc_Absyn_Cvar_Absyn_Type_struct*)t)->f6;{void*r=_Tmp6;int id=_Tmp4;const char*name=_Tmp3;const char*pos=_Tmp2;
# 3321
({struct Cyc_Int_pa_PrintArg_struct _Tmp7=({struct Cyc_Int_pa_PrintArg_struct _Tmp8;_Tmp8.tag=1,_Tmp8.f1=(unsigned long)id;_Tmp8;});struct Cyc_String_pa_PrintArg_struct _Tmp8=({struct Cyc_String_pa_PrintArg_struct _Tmp9;_Tmp9.tag=0,_Tmp9.f1=({const char*_TmpA=name;_tag_fat((void*)_TmpA,sizeof(char),_get_zero_arr_size_char((void*)_TmpA,1U));});_Tmp9;});struct Cyc_String_pa_PrintArg_struct _Tmp9=({struct Cyc_String_pa_PrintArg_struct _TmpA;_TmpA.tag=0,_TmpA.f1=({const char*_TmpB=pos;_tag_fat((void*)_TmpB,sizeof(char),_get_zero_arr_size_char((void*)_TmpB,1U));});_TmpA;});void*_TmpA[3];_TmpA[0]=& _Tmp7,_TmpA[1]=& _Tmp8,_TmpA[2]=& _Tmp9;Cyc_fprintf(Cyc_stderr,_tag_fat("Got Cvar id=%d, name=%s pos=%s",sizeof(char),31U),_tag_fat(_TmpA,sizeof(void*),3));});
if((unsigned)r){
Cyc_fprintf(Cyc_stderr,_tag_fat(" resolved to = ",sizeof(char),16U),_tag_fat(0U,sizeof(void*),0));
Cyc_Tcutil_deconstruct_type(r);}
# 3326
Cyc_fprintf(Cyc_stderr,_tag_fat("\n",sizeof(char),2U),_tag_fat(0U,sizeof(void*),0));
goto _LL0;}case 1: _Tmp6=(void*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)t)->f2;{void*r=_Tmp6;
# 3329
Cyc_fprintf(Cyc_stderr,_tag_fat("Got Evar",sizeof(char),9U),_tag_fat(0U,sizeof(void*),0));
if((unsigned)r){
Cyc_fprintf(Cyc_stderr,_tag_fat(" with compression = ",sizeof(char),21U),_tag_fat(0U,sizeof(void*),0));
Cyc_Tcutil_deconstruct_type(r);}
# 3334
Cyc_fprintf(Cyc_stderr,_tag_fat("\n",sizeof(char),2U),_tag_fat(0U,sizeof(void*),0));
goto _LL0;}case 2: _Tmp6=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)t)->f1;{struct Cyc_Absyn_Tvar*tv=_Tmp6;
# 3337
({struct Cyc_String_pa_PrintArg_struct _Tmp7=({struct Cyc_String_pa_PrintArg_struct _Tmp8;_Tmp8.tag=0,({struct _fat_ptr _Tmp9=Cyc_Absynpp_kindbound2string(tv->kind);_Tmp8.f1=_Tmp9;});_Tmp8;});struct Cyc_Int_pa_PrintArg_struct _Tmp8=({struct Cyc_Int_pa_PrintArg_struct _Tmp9;_Tmp9.tag=1,_Tmp9.f1=(unsigned long)tv->identity;_Tmp9;});void*_Tmp9[2];_Tmp9[0]=& _Tmp7,_Tmp9[1]=& _Tmp8;Cyc_fprintf(Cyc_stderr,_tag_fat("Got VarType -- kindbnd is %s, id is %d",sizeof(char),39U),_tag_fat(_Tmp9,sizeof(void*),2));});
if((unsigned)tv->aquals_bound)
({struct Cyc_String_pa_PrintArg_struct _Tmp7=({struct Cyc_String_pa_PrintArg_struct _Tmp8;_Tmp8.tag=0,({struct _fat_ptr _Tmp9=Cyc_Absynpp_typ2string(tv->aquals_bound);_Tmp8.f1=_Tmp9;});_Tmp8;});void*_Tmp8[1];_Tmp8[0]=& _Tmp7;Cyc_fprintf(Cyc_stderr,_tag_fat(" AQbnd = %s",sizeof(char),12U),_tag_fat(_Tmp8,sizeof(void*),1));});
Cyc_fprintf(Cyc_stderr,_tag_fat("\n",sizeof(char),2U),_tag_fat(0U,sizeof(void*),0));
goto _LL0;}case 4: _Tmp6=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)t)->f1.elt_type;_Tmp5=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)t)->f1.ptr_atts.eff;_Tmp1=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)t)->f1.ptr_atts.nullable;_Tmp0=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)t)->f1.ptr_atts.aqual;{void*t1=_Tmp6;void*rgn=_Tmp5;void*nlbl=_Tmp1;void*aq=_Tmp0;
# 3343
({struct Cyc_String_pa_PrintArg_struct _Tmp7=({struct Cyc_String_pa_PrintArg_struct _Tmp8;_Tmp8.tag=0,({struct _fat_ptr _Tmp9=Cyc_Absynpp_typ2string(t);_Tmp8.f1=_Tmp9;});_Tmp8;});void*_Tmp8[1];_Tmp8[0]=& _Tmp7;Cyc_fprintf(Cyc_stderr,_tag_fat("Got PointerType(%s) ... alias qualifier is \n",sizeof(char),45U),_tag_fat(_Tmp8,sizeof(void*),1));});
Cyc_Tcutil_deconstruct_type(aq);
({struct Cyc_String_pa_PrintArg_struct _Tmp7=({struct Cyc_String_pa_PrintArg_struct _Tmp8;_Tmp8.tag=0,({struct _fat_ptr _Tmp9=Cyc_Absynpp_typ2string(t);_Tmp8.f1=_Tmp9;});_Tmp8;});void*_Tmp8[1];_Tmp8[0]=& _Tmp7;Cyc_fprintf(Cyc_stderr,_tag_fat("Got PointerType(%s) ... region is \n",sizeof(char),36U),_tag_fat(_Tmp8,sizeof(void*),1));});
Cyc_Tcutil_deconstruct_type(rgn);
Cyc_fprintf(Cyc_stderr,_tag_fat("*******************************Element Type***************************\n",sizeof(char),72U),_tag_fat(0U,sizeof(void*),0));
Cyc_Tcutil_deconstruct_type(t1);
goto _LL0;}case 5:
# 3351
 Cyc_fprintf(Cyc_stderr,_tag_fat("Got ArrayType\n",sizeof(char),15U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 6: _Tmp6=(struct Cyc_Absyn_FnInfo*)&((struct Cyc_Absyn_FnType_Absyn_Type_struct*)t)->f1;{struct Cyc_Absyn_FnInfo*i=(struct Cyc_Absyn_FnInfo*)_Tmp6;
# 3353
Cyc_fprintf(Cyc_stderr,_tag_fat("Got FnType\n",sizeof(char),12U),_tag_fat(0U,sizeof(void*),0));
Cyc_fprintf(Cyc_stderr,_tag_fat("ReturnType:",sizeof(char),12U),_tag_fat(0U,sizeof(void*),0));
Cyc_Tcutil_deconstruct_type(i->ret_type);
{struct Cyc_List_List*it=i->args;for(0;it!=0;it=it->tl){
struct _tuple9*_Tmp7=(struct _tuple9*)it->hd;void*_Tmp8;_Tmp8=_Tmp7->f2;{void*at=_Tmp8;
Cyc_fprintf(Cyc_stderr,_tag_fat("Argument:",sizeof(char),10U),_tag_fat(0U,sizeof(void*),0));
Cyc_Tcutil_deconstruct_type(at);}}}
# 3361
{struct Cyc_List_List*qt=i->qual_bnd;for(0;qt!=0;qt=qt->tl){
struct _tuple15*_Tmp7=(struct _tuple15*)qt->hd;void*_Tmp8;void*_Tmp9;_Tmp9=_Tmp7->f0;_Tmp8=_Tmp7->f1;{void*t1=_Tmp9;void*t2=_Tmp8;
Cyc_fprintf(Cyc_stderr,_tag_fat("Qualbnd:",sizeof(char),9U),_tag_fat(0U,sizeof(void*),0));
Cyc_Tcutil_deconstruct_type(t1);
Cyc_fprintf(Cyc_stderr,_tag_fat(" <= ",sizeof(char),5U),_tag_fat(0U,sizeof(void*),0));
Cyc_Tcutil_deconstruct_type(t2);}}}
# 3368
Cyc_fprintf(Cyc_stderr,_tag_fat("Effect: ",sizeof(char),9U),_tag_fat(0U,sizeof(void*),0));
if((unsigned)i->effect)
Cyc_Tcutil_deconstruct_type(i->effect);else{
# 3372
Cyc_fprintf(Cyc_stderr,_tag_fat("NULL",sizeof(char),5U),_tag_fat(0U,sizeof(void*),0));}
goto _LL0;}case 7:
# 3375
 Cyc_fprintf(Cyc_stderr,_tag_fat("Got AnonAggrType\n",sizeof(char),18U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 8:
# 3377
 Cyc_fprintf(Cyc_stderr,_tag_fat("Got TypedefType\n",sizeof(char),17U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 9:
# 3379
 Cyc_fprintf(Cyc_stderr,_tag_fat("Got ValueofType\n",sizeof(char),17U),_tag_fat(0U,sizeof(void*),0));goto _LL0;case 10:
# 3381
 Cyc_fprintf(Cyc_stderr,_tag_fat("Got TypeDeclType\n",sizeof(char),18U),_tag_fat(0U,sizeof(void*),0));goto _LL0;default:
# 3383
 Cyc_fprintf(Cyc_stderr,_tag_fat("Got TypeofType\n",sizeof(char),16U),_tag_fat(0U,sizeof(void*),0));goto _LL0;}_LL0:;}}
