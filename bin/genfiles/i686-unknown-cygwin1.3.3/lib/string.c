// This is a C header file to be used by the output of the Cyclone
// to C translator.  The corresponding definitions are in file lib/runtime_cyc.c
#ifndef _CYC_INCLUDE_H_
#define _CYC_INCLUDE_H_

#include <setjmp.h>

#ifdef NO_CYC_PREFIX
#define ADD_PREFIX(x) x
#else
#define ADD_PREFIX(x) Cyc_##x
#endif

#ifndef offsetof
// should be size_t, but int is fine.
#define offsetof(t,n) ((int)(&(((t *)0)->n)))
#endif

//// Tagged arrays
struct _tagged_arr { 
  unsigned char *curr; 
  unsigned char *base; 
  unsigned char *last_plus_one; 
};

//// Discriminated Unions
struct _xtunion_struct { char *tag; };

// Need one of these per thread (we don't have threads)
// The runtime maintains a stack that contains either _handler_cons
// structs or _RegionHandle structs.  The tag is 0 for a handler_cons
// and 1 for a region handle.  
struct _RuntimeStack {
  int tag; // 0 for an exception handler, 1 for a region handle
  struct _RuntimeStack *next;
};

//// Regions
struct _RegionPage {
#ifdef CYC_REGION_PROFILE
  unsigned int total_bytes;
  unsigned int free_bytes;
#endif
  struct _RegionPage *next;
  char data[0];
};

struct _RegionHandle {
  struct _RuntimeStack s;
  struct _RegionPage *curr;
  char               *offset;
  char               *last_plus_one;
#ifdef CYC_REGION_PROFILE
  const char         *name;
#endif
};

extern struct _RegionHandle _new_region(const char *);
extern void * _region_malloc(struct _RegionHandle *, unsigned);
extern void * _region_calloc(struct _RegionHandle *, unsigned t, unsigned n);
extern void   _free_region(struct _RegionHandle *);

//// Exceptions 
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
extern int _throw_null();
extern int _throw_arraybounds();
extern int _throw_badalloc();
extern int _throw(void * e);
#endif

extern struct _xtunion_struct *_exn_thrown;

//// Built-in Exceptions
extern struct _xtunion_struct ADD_PREFIX(Null_Exception_struct);
extern struct _xtunion_struct * ADD_PREFIX(Null_Exception);
extern struct _xtunion_struct ADD_PREFIX(Array_bounds_struct);
extern struct _xtunion_struct * ADD_PREFIX(Array_bounds);
extern struct _xtunion_struct ADD_PREFIX(Match_Exception_struct);
extern struct _xtunion_struct * ADD_PREFIX(Match_Exception);
extern struct _xtunion_struct ADD_PREFIX(Bad_alloc_struct);
extern struct _xtunion_struct * ADD_PREFIX(Bad_alloc);

//// Built-in Run-time Checks and company
static inline 
void * _check_null(void * ptr) {
#ifndef NO_CYC_NULL_CHECKS
  if(!ptr)
    _throw_null();
#endif
  return ptr;
}
static inline 
char * _check_known_subscript_null(void * ptr, unsigned bound, 
				   unsigned elt_sz, unsigned index) {
#ifndef NO_CYC_NULL_CHECKS
  if(!ptr)
    _throw_null();
#endif
#ifndef NO_CYC_BOUNDS_CHECKS
  if(index >= bound)
    _throw_arraybounds();
#endif
  return ((char *)ptr) + elt_sz*index;
}
static inline 
unsigned _check_known_subscript_notnull(unsigned bound, unsigned index) {
#ifndef NO_CYC_BOUNDS_CHECKS
  if(index >= bound)
    _throw_arraybounds();
#endif
  return index;
}
static inline 
char * _check_unknown_subscript(struct _tagged_arr arr,
				unsigned elt_sz, unsigned index) {
  // caller casts first argument and result
  // multiplication looks inefficient, but C compiler has to insert it otherwise
  // by inlining, it should be able to avoid actual multiplication
  unsigned char * ans = arr.curr + elt_sz * index;
  // might be faster not to distinguish these two cases. definitely would be
  // smaller.
#ifndef NO_CYC_NULL_CHECKS
  if(!arr.base) 
    _throw_null();
#endif
#ifndef NO_CYC_BOUNDS_CHECKS
  if(ans < arr.base || ans >= arr.last_plus_one)
    _throw_arraybounds();
#endif
  return ans;
}
static inline 
struct _tagged_arr _tag_arr(const void * curr, 
			    unsigned elt_sz, unsigned num_elts) {
  // beware the gcc bug, can happen with *temp = _tag_arr(...) in weird places!
  struct _tagged_arr ans;
  ans.base = (void *)curr;
  ans.curr = (void *)curr;
  ans.last_plus_one = ((char *)curr) + elt_sz * num_elts;
  return ans;
}
static inline
struct _tagged_arr * _init_tag_arr(struct _tagged_arr * arr_ptr, void * arr, 
				   unsigned elt_sz, unsigned num_elts) {
  // we use this to (hopefully) avoid the gcc bug
  arr_ptr->base = arr_ptr->curr = arr;
  arr_ptr->last_plus_one = ((char *)arr) + elt_sz * num_elts;
  return arr_ptr;
}

static inline
char * _untag_arr(struct _tagged_arr arr, unsigned elt_sz, unsigned num_elts) {
  // Note: if arr is "null" base and curr should both be null, so this
  //       is correct (caller checks for null if untagging to @ type)
  // base may not be null if you use t ? pointer subtraction to get 0 -- oh well
#ifndef NO_CYC_BOUNDS_CHECKS
  if(arr.curr < arr.base || arr.curr + elt_sz * num_elts > arr.last_plus_one)
    _throw_arraybounds();
#endif
  return arr.curr;
}
static inline 
unsigned _get_arr_size(struct _tagged_arr arr, unsigned elt_sz) {
  return (arr.curr<arr.base || arr.curr>=arr.last_plus_one) ? 0 : ((arr.last_plus_one - arr.curr) / elt_sz);
}
static inline
struct _tagged_arr _tagged_arr_plus(struct _tagged_arr arr, unsigned elt_sz,
				    int change) {
  struct _tagged_arr ans = arr;
  ans.curr += ((int)elt_sz)*change;
  return ans;
}
static inline
struct _tagged_arr _tagged_arr_inplace_plus(struct _tagged_arr * arr_ptr,
					    unsigned elt_sz, int change) {
  arr_ptr->curr += ((int)elt_sz)*change;
  return *arr_ptr;
}
static inline
struct _tagged_arr _tagged_arr_inplace_plus_post(struct _tagged_arr * arr_ptr,
						 unsigned elt_sz, int change) {
  struct _tagged_arr ans = *arr_ptr;
  arr_ptr->curr += ((int)elt_sz)*change;
  return ans;
}
				  
//// Allocation
extern void * GC_malloc(int);
extern void * GC_malloc_atomic(int);
extern void * GC_calloc(unsigned int,unsigned int);
extern void * GC_calloc_atomic(unsigned int,unsigned int);

static inline void * _cycalloc(int n) {
  void * ans = (void *)GC_malloc(n);
  if(!ans)
    _throw_badalloc();
  return ans;
}
static inline void * _cycalloc_atomic(int n) {
  void * ans = (void *)GC_malloc_atomic(n);
  if(!ans)
    _throw_badalloc();
  return ans;
}
static inline void * _cyccalloc(unsigned int n, unsigned int s) {
  void * ans = (void *)GC_calloc(n,s);
  if (!ans)
    _throw_badalloc();
  return ans;
}
static inline void * _cyccalloc_atomic(unsigned int n, unsigned int s) {
  void * ans = (void *)GC_calloc_atomic(n,s);
  if (!ans)
    _throw_badalloc();
  return ans;
}
#define MAX_MALLOC_SIZE (1 << 28)
static inline unsigned int _check_times(unsigned int x, unsigned int y) {
  unsigned long long whole_ans = 
    ((unsigned long long)x)*((unsigned long long)y);
  unsigned int word_ans = (unsigned int)whole_ans;
  if(word_ans < whole_ans || word_ans > MAX_MALLOC_SIZE)
    _throw_badalloc();
  return word_ans;
}

#if defined(CYC_REGION_PROFILE) 
extern void * _profile_GC_malloc(int,char *file,int lineno);
extern void * _profile_GC_malloc_atomic(int,char *file,int lineno);
extern void * _profile_region_malloc(struct _RegionHandle *, unsigned int,
                                     char *file,int lineno);
extern struct _RegionHandle _profile_new_region(const char *rgn_name,
						char *file,int lineno);
extern void _profile_free_region(struct _RegionHandle *,
				 char *file,int lineno);
#  if !defined(RUNTIME_CYC)
#define _new_region(n) _profile_new_region(n,__FILE__ ":" __FUNCTION__,__LINE__)
#define _free_region(r) _profile_free_region(r,__FILE__ ":" __FUNCTION__,__LINE__)
#define _region_malloc(rh,n) _profile_region_malloc(rh,n,__FILE__ ":" __FUNCTION__,__LINE__)
#  endif
#define _cycalloc(n) _profile_GC_malloc(n,__FILE__ ":" __FUNCTION__,__LINE__)
#define _cycalloc_atomic(n) _profile_GC_malloc_atomic(n,__FILE__ ":" __FUNCTION__,__LINE__)
#endif
#endif
 struct Cyc_Std__types_fd_set{int fds_bits[2];};struct Cyc_Core_Opt{void*v;};extern
struct _tagged_arr Cyc_Core_new_string(unsigned int);extern struct _tagged_arr Cyc_Core_rnew_string(
struct _RegionHandle*,unsigned int);extern unsigned char Cyc_Core_Invalid_argument[
21];struct Cyc_Core_Invalid_argument_struct{unsigned char*tag;struct _tagged_arr f1;
};extern unsigned char Cyc_Core_Failure[12];struct Cyc_Core_Failure_struct{
unsigned char*tag;struct _tagged_arr f1;};extern unsigned char Cyc_Core_Impossible[
15];struct Cyc_Core_Impossible_struct{unsigned char*tag;struct _tagged_arr f1;};
extern unsigned char Cyc_Core_Not_found[14];extern unsigned char Cyc_Core_Unreachable[
16];struct Cyc_Core_Unreachable_struct{unsigned char*tag;struct _tagged_arr f1;};
extern struct _RegionHandle*Cyc_Core_heap_region;extern struct _tagged_arr
wrap_Cstring_as_string(unsigned char*,unsigned int);struct Cyc_List_List{void*hd;
struct Cyc_List_List*tl;};extern int Cyc_List_length(struct Cyc_List_List*x);extern
unsigned char Cyc_List_List_mismatch[18];extern unsigned char Cyc_List_Nth[8];
extern int toupper(int);extern struct _tagged_arr Cyc_Std_strerror(int);extern
unsigned int Cyc_Std_strlen(struct _tagged_arr s);extern int Cyc_Std_strcmp(struct
_tagged_arr s1,struct _tagged_arr s2);extern int Cyc_Std_strptrcmp(struct _tagged_arr*
s1,struct _tagged_arr*s2);extern int Cyc_Std_strncmp(struct _tagged_arr s1,struct
_tagged_arr s2,unsigned int len);extern int Cyc_Std_zstrcmp(struct _tagged_arr,struct
_tagged_arr);extern int Cyc_Std_zstrncmp(struct _tagged_arr s1,struct _tagged_arr s2,
unsigned int n);extern int Cyc_Std_zstrptrcmp(struct _tagged_arr*,struct _tagged_arr*);
extern int Cyc_Std_strcasecmp(struct _tagged_arr,struct _tagged_arr);extern int Cyc_Std_strncasecmp(
struct _tagged_arr s1,struct _tagged_arr s2,unsigned int len);extern struct _tagged_arr
Cyc_Std_strcat(struct _tagged_arr dest,struct _tagged_arr src);extern struct
_tagged_arr Cyc_Std_strconcat(struct _tagged_arr,struct _tagged_arr);extern struct
_tagged_arr Cyc_Std_rstrconcat(struct _RegionHandle*,struct _tagged_arr,struct
_tagged_arr);extern struct _tagged_arr Cyc_Std_strconcat_l(struct Cyc_List_List*);
extern struct _tagged_arr Cyc_Std_rstrconcat_l(struct _RegionHandle*,struct Cyc_List_List*);
extern struct _tagged_arr Cyc_Std_str_sepstr(struct Cyc_List_List*,struct _tagged_arr);
extern struct _tagged_arr Cyc_Std_rstr_sepstr(struct _RegionHandle*,struct Cyc_List_List*,
struct _tagged_arr);extern struct _tagged_arr Cyc_Std_strcpy(struct _tagged_arr dest,
struct _tagged_arr src);extern struct _tagged_arr Cyc_Std_strncpy(struct _tagged_arr,
struct _tagged_arr,unsigned int);extern struct _tagged_arr Cyc_Std_zstrncpy(struct
_tagged_arr,struct _tagged_arr,unsigned int);extern struct _tagged_arr Cyc_Std_realloc(
struct _tagged_arr,unsigned int);extern struct _tagged_arr Cyc_Std__memcpy(struct
_tagged_arr d,struct _tagged_arr s,unsigned int,unsigned int);extern struct
_tagged_arr Cyc_Std__memmove(struct _tagged_arr d,struct _tagged_arr s,unsigned int,
unsigned int);extern int Cyc_Std_memcmp(struct _tagged_arr s1,struct _tagged_arr s2,
unsigned int n);extern struct _tagged_arr Cyc_Std_memchr(struct _tagged_arr s,
unsigned char c,unsigned int n);extern struct _tagged_arr Cyc_Std_mmemchr(struct
_tagged_arr s,unsigned char c,unsigned int n);extern struct _tagged_arr Cyc_Std_memset(
struct _tagged_arr s,unsigned char c,unsigned int n);extern void Cyc_Std_bzero(struct
_tagged_arr s,unsigned int n);extern void Cyc_Std__bcopy(struct _tagged_arr src,struct
_tagged_arr dst,unsigned int n,unsigned int sz);extern struct _tagged_arr Cyc_Std_expand(
struct _tagged_arr s,unsigned int sz);extern struct _tagged_arr Cyc_Std_rexpand(struct
_RegionHandle*,struct _tagged_arr s,unsigned int sz);extern struct _tagged_arr Cyc_Std_realloc_str(
struct _tagged_arr str,unsigned int sz);extern struct _tagged_arr Cyc_Std_rrealloc_str(
struct _RegionHandle*r,struct _tagged_arr str,unsigned int sz);extern struct
_tagged_arr Cyc_Std_strdup(struct _tagged_arr src);extern struct _tagged_arr Cyc_Std_rstrdup(
struct _RegionHandle*,struct _tagged_arr src);extern struct _tagged_arr Cyc_Std_substring(
struct _tagged_arr,int ofs,unsigned int n);extern struct _tagged_arr Cyc_Std_rsubstring(
struct _RegionHandle*,struct _tagged_arr,int ofs,unsigned int n);extern struct
_tagged_arr Cyc_Std_replace_suffix(struct _tagged_arr,struct _tagged_arr,struct
_tagged_arr);extern struct _tagged_arr Cyc_Std_rreplace_suffix(struct _RegionHandle*
r,struct _tagged_arr src,struct _tagged_arr curr_suffix,struct _tagged_arr new_suffix);
extern struct _tagged_arr Cyc_Std_strchr(struct _tagged_arr s,unsigned char c);extern
struct _tagged_arr Cyc_Std_mstrchr(struct _tagged_arr s,unsigned char c);extern struct
_tagged_arr Cyc_Std_mstrrchr(struct _tagged_arr s,unsigned char c);extern struct
_tagged_arr Cyc_Std_strrchr(struct _tagged_arr s,unsigned char c);extern struct
_tagged_arr Cyc_Std_mstrstr(struct _tagged_arr haystack,struct _tagged_arr needle);
extern struct _tagged_arr Cyc_Std_strstr(struct _tagged_arr haystack,struct
_tagged_arr needle);extern struct _tagged_arr Cyc_Std_strpbrk(struct _tagged_arr s,
struct _tagged_arr accept);extern struct _tagged_arr Cyc_Std_mstrpbrk(struct
_tagged_arr s,struct _tagged_arr accept);extern unsigned int Cyc_Std_strspn(struct
_tagged_arr s,struct _tagged_arr accept);extern unsigned int Cyc_Std_strcspn(struct
_tagged_arr s,struct _tagged_arr accept);extern struct _tagged_arr Cyc_Std_strtok(
struct _tagged_arr s,struct _tagged_arr delim);extern struct Cyc_List_List*Cyc_Std_explode(
struct _tagged_arr s);extern struct Cyc_List_List*Cyc_Std_rexplode(struct
_RegionHandle*,struct _tagged_arr s);extern struct _tagged_arr Cyc_Std_implode(struct
Cyc_List_List*c);extern void*Cyc_Std___assert_fail(struct _tagged_arr assertion,
struct _tagged_arr file,unsigned int line);extern unsigned char*strerror(int errnum);
struct _tagged_arr Cyc_Std_strerror(int errnum){return(struct _tagged_arr)
wrap_Cstring_as_string(strerror(errnum),- 1);}unsigned int Cyc_Std_strlen(struct
_tagged_arr s){unsigned int i;unsigned int sz=_get_arr_size(s,sizeof(unsigned char));
for(i=0;i < sz;i ++){if(((const unsigned char*)s.curr)[(int)i]== '\000'){return i;}}
return i;}inline static unsigned int Cyc_Std_int_strleno(struct _tagged_arr s,struct
_tagged_arr error){int i;unsigned int sz=_get_arr_size(s,sizeof(unsigned char));
for(i=0;i < sz;i ++){if(((const unsigned char*)s.curr)[i]== '\000'){break;}}return(
unsigned int)i;}int Cyc_Std_strcmp(struct _tagged_arr s1,struct _tagged_arr s2){if(s1.curr
== s2.curr){return 0;}{int i=0;unsigned int sz1=_get_arr_size(s1,sizeof(
unsigned char));unsigned int sz2=_get_arr_size(s2,sizeof(unsigned char));
unsigned int minsz=sz1 < sz2? sz1: sz2;(minsz <= _get_arr_size(s1,sizeof(
unsigned char))? minsz <= _get_arr_size(s2,sizeof(unsigned char)): 0)? 0:((int(*)(
struct _tagged_arr assertion,struct _tagged_arr file,unsigned int line))Cyc_Std___assert_fail)(
_tag_arr("minsz <= s1.size && minsz <= s2.size",sizeof(unsigned char),37),
_tag_arr("string.cyc",sizeof(unsigned char),11),84);while(i < minsz){
unsigned char c1=((const unsigned char*)s1.curr)[i];unsigned char c2=((const
unsigned char*)s2.curr)[i];if(c1 == '\000'){if(c2 == '\000'){return 0;}else{return -
1;}}else{if(c2 == '\000'){return 1;}else{int diff=c1 - c2;if(diff != 0){return diff;}}}
++ i;}if(sz1 == sz2){return 0;}if(sz1 == minsz){if(((const unsigned char*)s2.curr)[i]
== '\000'){return 0;}else{return - 1;}}else{if(((const unsigned char*)s1.curr)[i]== '\000'){
return 0;}else{return 1;}}}}int Cyc_Std_strptrcmp(struct _tagged_arr*s1,struct
_tagged_arr*s2){return Cyc_Std_strcmp(*s1,*s2);}inline static int Cyc_Std_ncmp(
struct _tagged_arr s1,unsigned int len1,struct _tagged_arr s2,unsigned int len2,
unsigned int n){if(n <= 0){return 0;}{unsigned int min_len=len1 > len2? len2: len1;
unsigned int bound=min_len > n? n: min_len;(bound <= _get_arr_size(s1,sizeof(
unsigned char))? bound <= _get_arr_size(s2,sizeof(unsigned char)): 0)? 0:((int(*)(
struct _tagged_arr assertion,struct _tagged_arr file,unsigned int line))Cyc_Std___assert_fail)(
_tag_arr("bound <= s1.size && bound <= s2.size",sizeof(unsigned char),37),
_tag_arr("string.cyc",sizeof(unsigned char),11),120);{int i=0;for(0;i < bound;i ++){
int retc;if((retc=((const unsigned char*)s1.curr)[i]- ((const unsigned char*)s2.curr)[
i])!= 0){return retc;}}}if(len1 < n? 1: len2 < n){return(int)len1 - (int)len2;}return 0;}}
int Cyc_Std_strncmp(struct _tagged_arr s1,struct _tagged_arr s2,unsigned int n){
unsigned int len1=Cyc_Std_int_strleno(s1,_tag_arr("Std::strncmp",sizeof(
unsigned char),13));unsigned int len2=Cyc_Std_int_strleno(s2,_tag_arr("Std::strncmp",
sizeof(unsigned char),13));return Cyc_Std_ncmp(s1,len1,s2,len2,n);}int Cyc_Std_zstrcmp(
struct _tagged_arr a,struct _tagged_arr b){if(a.curr == b.curr){return 0;}{
unsigned int as=_get_arr_size(a,sizeof(unsigned char));unsigned int bs=
_get_arr_size(b,sizeof(unsigned char));unsigned int min_length=as < bs? as: bs;int i=
- 1;(min_length <= _get_arr_size(a,sizeof(unsigned char))? min_length <= 
_get_arr_size(b,sizeof(unsigned char)): 0)? 0:((int(*)(struct _tagged_arr assertion,
struct _tagged_arr file,unsigned int line))Cyc_Std___assert_fail)(_tag_arr("min_length <= a.size && min_length <= b.size",
sizeof(unsigned char),45),_tag_arr("string.cyc",sizeof(unsigned char),11),150);
while((++ i,i < min_length)){int diff=(int)((const unsigned char*)a.curr)[i]- (int)((
const unsigned char*)b.curr)[i];if(diff != 0){return diff;}}return(int)as - (int)bs;}}
int Cyc_Std_zstrncmp(struct _tagged_arr s1,struct _tagged_arr s2,unsigned int n){if(n
<= 0){return 0;}{unsigned int s1size=_get_arr_size(s1,sizeof(unsigned char));
unsigned int s2size=_get_arr_size(s2,sizeof(unsigned char));unsigned int min_size=
s1size > s2size? s2size: s1size;unsigned int bound=min_size > n? n: min_size;(bound <= 
_get_arr_size(s1,sizeof(unsigned char))? bound <= _get_arr_size(s2,sizeof(
unsigned char)): 0)? 0:((int(*)(struct _tagged_arr assertion,struct _tagged_arr file,
unsigned int line))Cyc_Std___assert_fail)(_tag_arr("bound <= s1.size && bound <= s2.size",
sizeof(unsigned char),37),_tag_arr("string.cyc",sizeof(unsigned char),11),168);{
int i=0;for(0;i < bound;i ++){if(((const unsigned char*)s1.curr)[i]< ((const
unsigned char*)s2.curr)[i]){return - 1;}else{if(((const unsigned char*)s2.curr)[i]< ((
const unsigned char*)s1.curr)[i]){return 1;}}}}if(min_size <= bound){return 0;}if(
s1size < s2size){return - 1;}else{return 1;}}}int Cyc_Std_zstrptrcmp(struct
_tagged_arr*a,struct _tagged_arr*b){return Cyc_Std_zstrcmp(*a,*b);}inline static
struct _tagged_arr Cyc_Std_int_strcato(struct _tagged_arr dest,struct _tagged_arr src,
struct _tagged_arr error){int i;unsigned int dsize;unsigned int slen;unsigned int dlen;
dsize=_get_arr_size(dest,sizeof(unsigned char));dlen=Cyc_Std_strlen((struct
_tagged_arr)dest);slen=Cyc_Std_int_strleno(src,error);if(slen + dlen <= dsize){
slen <= _get_arr_size(src,sizeof(unsigned char))? 0:((int(*)(struct _tagged_arr
assertion,struct _tagged_arr file,unsigned int line))Cyc_Std___assert_fail)(
_tag_arr("slen <= src.size",sizeof(unsigned char),17),_tag_arr("string.cyc",
sizeof(unsigned char),11),203);for(i=0;i < slen;i ++){*((unsigned char*)
_check_unknown_subscript(dest,sizeof(unsigned char),(int)(i + dlen)))=((const
unsigned char*)src.curr)[i];}if(i != dsize){*((unsigned char*)
_check_unknown_subscript(dest,sizeof(unsigned char),(int)(i + dlen)))='\000';}}
else{(int)_throw((void*)({struct Cyc_Core_Invalid_argument_struct*_tmp0=_cycalloc(
sizeof(struct Cyc_Core_Invalid_argument_struct));_tmp0[0]=({struct Cyc_Core_Invalid_argument_struct
_tmp1;_tmp1.tag=Cyc_Core_Invalid_argument;_tmp1.f1=error;_tmp1;});_tmp0;}));}
return dest;}struct _tagged_arr Cyc_Std_strcat(struct _tagged_arr dest,struct
_tagged_arr src){return Cyc_Std_int_strcato(dest,src,_tag_arr("Std::strcat",
sizeof(unsigned char),12));}struct _tagged_arr Cyc_Std_rstrconcat(struct
_RegionHandle*r,struct _tagged_arr a,struct _tagged_arr b){unsigned int _tmp2=Cyc_Std_strlen(
a);unsigned int _tmp3=Cyc_Std_strlen(b);struct _tagged_arr ans=Cyc_Core_rnew_string(
r,(_tmp2 + _tmp3)+ 1);int i;int j;(_tmp2 <= _get_arr_size(ans,sizeof(unsigned char))?
_tmp2 <= _get_arr_size(a,sizeof(unsigned char)): 0)? 0:((int(*)(struct _tagged_arr
assertion,struct _tagged_arr file,unsigned int line))Cyc_Std___assert_fail)(
_tag_arr("alen <= ans.size && alen <= a.size",sizeof(unsigned char),35),_tag_arr("string.cyc",
sizeof(unsigned char),11),227);for(i=0;i < _tmp2;++ i){((unsigned char*)ans.curr)[
i]=((const unsigned char*)a.curr)[i];}_tmp3 <= _get_arr_size(b,sizeof(
unsigned char))? 0:((int(*)(struct _tagged_arr assertion,struct _tagged_arr file,
unsigned int line))Cyc_Std___assert_fail)(_tag_arr("blen <= b.size",sizeof(
unsigned char),15),_tag_arr("string.cyc",sizeof(unsigned char),11),229);for(j=0;
j < _tmp3;++ j){*((unsigned char*)_check_unknown_subscript(ans,sizeof(
unsigned char),i + j))=((const unsigned char*)b.curr)[j];}return ans;}struct
_tagged_arr Cyc_Std_strconcat(struct _tagged_arr a,struct _tagged_arr b){return Cyc_Std_rstrconcat(
Cyc_Core_heap_region,a,b);}struct _tagged_arr Cyc_Std_rstrconcat_l(struct
_RegionHandle*r,struct Cyc_List_List*strs){unsigned int len;unsigned int total_len=
0;struct _tagged_arr ans;{struct _RegionHandle _tmp4=_new_region("temp");struct
_RegionHandle*temp=& _tmp4;_push_region(temp);{struct Cyc_List_List*lens=({struct
Cyc_List_List*_tmp7=_region_malloc(temp,sizeof(struct Cyc_List_List));_tmp7->hd=(
void*)((unsigned int)0);_tmp7->tl=0;_tmp7;});struct Cyc_List_List*end=lens;{
struct Cyc_List_List*p=strs;for(0;p != 0;p=p->tl){len=Cyc_Std_strlen(*((struct
_tagged_arr*)p->hd));total_len +=len;((struct Cyc_List_List*)_check_null(end))->tl=({
struct Cyc_List_List*_tmp5=_region_malloc(temp,sizeof(struct Cyc_List_List));_tmp5->hd=(
void*)len;_tmp5->tl=0;_tmp5;});end=((struct Cyc_List_List*)_check_null(end))->tl;}}
lens=lens->tl;ans=Cyc_Core_rnew_string(r,total_len + 1);{unsigned int i=0;while(
strs != 0){struct _tagged_arr _tmp6=*((struct _tagged_arr*)strs->hd);len=(
unsigned int)((struct Cyc_List_List*)_check_null(lens))->hd;Cyc_Std_strncpy(
_tagged_arr_plus(ans,sizeof(unsigned char),(int)i),_tmp6,len);i +=len;strs=strs->tl;
lens=lens->tl;}}};_pop_region(temp);}return ans;}struct _tagged_arr Cyc_Std_strconcat_l(
struct Cyc_List_List*strs){return Cyc_Std_rstrconcat_l(Cyc_Core_heap_region,strs);}
struct _tagged_arr Cyc_Std_rstr_sepstr(struct _RegionHandle*r,struct Cyc_List_List*
strs,struct _tagged_arr separator){if(strs == 0){return Cyc_Core_rnew_string(r,0);}
if(strs->tl == 0){return Cyc_Std_rstrdup(r,*((struct _tagged_arr*)strs->hd));}{
struct Cyc_List_List*_tmp8=strs;struct _RegionHandle _tmp9=_new_region("temp");
struct _RegionHandle*temp=& _tmp9;_push_region(temp);{struct Cyc_List_List*lens=({
struct Cyc_List_List*_tmpD=_region_malloc(temp,sizeof(struct Cyc_List_List));_tmpD->hd=(
void*)((unsigned int)0);_tmpD->tl=0;_tmpD;});struct Cyc_List_List*end=lens;
unsigned int len=0;unsigned int total_len=0;unsigned int list_len=0;for(0;_tmp8 != 0;
_tmp8=_tmp8->tl){len=Cyc_Std_strlen(*((struct _tagged_arr*)_tmp8->hd));total_len +=
len;((struct Cyc_List_List*)_check_null(end))->tl=({struct Cyc_List_List*_tmpA=
_region_malloc(temp,sizeof(struct Cyc_List_List));_tmpA->hd=(void*)len;_tmpA->tl=
0;_tmpA;});end=((struct Cyc_List_List*)_check_null(end))->tl;++ list_len;}lens=
lens->tl;{unsigned int seplen=Cyc_Std_strlen(separator);total_len +=(list_len - 1)* 
seplen;{struct _tagged_arr ans=Cyc_Core_rnew_string(r,total_len);unsigned int i=0;
while(strs->tl != 0){struct _tagged_arr _tmpB=*((struct _tagged_arr*)strs->hd);len=(
unsigned int)((struct Cyc_List_List*)_check_null(lens))->hd;Cyc_Std_strncpy(
_tagged_arr_plus(ans,sizeof(unsigned char),(int)i),_tmpB,len);i +=len;Cyc_Std_strncpy(
_tagged_arr_plus(ans,sizeof(unsigned char),(int)i),separator,seplen);i +=seplen;
strs=strs->tl;lens=lens->tl;}Cyc_Std_strncpy(_tagged_arr_plus(ans,sizeof(
unsigned char),(int)i),*((struct _tagged_arr*)strs->hd),(unsigned int)((struct Cyc_List_List*)
_check_null(lens))->hd);{struct _tagged_arr _tmpC=ans;_npop_handler(0);return _tmpC;}}}};
_pop_region(temp);}}struct _tagged_arr Cyc_Std_str_sepstr(struct Cyc_List_List*strs,
struct _tagged_arr separator){return Cyc_Std_rstr_sepstr(Cyc_Core_heap_region,strs,
separator);}struct _tagged_arr Cyc_Std_strncpy(struct _tagged_arr dest,struct
_tagged_arr src,unsigned int n){int i;(n <= _get_arr_size(src,sizeof(unsigned char))?
n <= _get_arr_size(dest,sizeof(unsigned char)): 0)? 0:((int(*)(struct _tagged_arr
assertion,struct _tagged_arr file,unsigned int line))Cyc_Std___assert_fail)(
_tag_arr("n <= src.size && n <= dest.size",sizeof(unsigned char),32),_tag_arr("string.cyc",
sizeof(unsigned char),11),327);for(i=0;i < n;i ++){unsigned char _tmpE=((const
unsigned char*)src.curr)[i];if(_tmpE == '\000'){break;}((unsigned char*)dest.curr)[
i]=_tmpE;}for(0;i < n;i ++){((unsigned char*)dest.curr)[i]='\000';}return dest;}
struct _tagged_arr Cyc_Std_zstrncpy(struct _tagged_arr dest,struct _tagged_arr src,
unsigned int n){(n <= _get_arr_size(dest,sizeof(unsigned char))? n <= _get_arr_size(
src,sizeof(unsigned char)): 0)? 0:((int(*)(struct _tagged_arr assertion,struct
_tagged_arr file,unsigned int line))Cyc_Std___assert_fail)(_tag_arr("n <= dest.size && n <= src.size",
sizeof(unsigned char),32),_tag_arr("string.cyc",sizeof(unsigned char),11),365);{
int i;for(i=0;i < n;i ++){((unsigned char*)dest.curr)[i]=((const unsigned char*)src.curr)[
i];}return dest;}}struct _tagged_arr Cyc_Std_strcpy(struct _tagged_arr dest,struct
_tagged_arr src){unsigned int len=Cyc_Std_strlen(src);Cyc_Std_strncpy(dest,src,len);
if(len < _get_arr_size(dest,sizeof(unsigned char))){((unsigned char*)dest.curr)[(
int)len]='\000';}return dest;}struct _tagged_arr Cyc_Std_rstrdup(struct
_RegionHandle*r,struct _tagged_arr src){unsigned int len;struct _tagged_arr temp;len=
Cyc_Std_strlen(src);temp=Cyc_Core_rnew_string(r,len + 1);Cyc_Std_strncpy(temp,src,
len);return temp;}struct _tagged_arr Cyc_Std_strdup(struct _tagged_arr src){return Cyc_Std_rstrdup(
Cyc_Core_heap_region,src);}struct _tagged_arr Cyc_Std_rexpand(struct _RegionHandle*
r,struct _tagged_arr s,unsigned int sz){struct _tagged_arr temp;unsigned int slen;slen=
Cyc_Std_strlen(s);sz=sz > slen? sz: slen;temp=Cyc_Core_rnew_string(r,sz);Cyc_Std_strncpy(
temp,s,slen);if(slen != _get_arr_size(s,sizeof(unsigned char))){*((unsigned char*)
_check_unknown_subscript(temp,sizeof(unsigned char),(int)slen))='\000';}return
temp;}struct _tagged_arr Cyc_Std_expand(struct _tagged_arr s,unsigned int sz){return
Cyc_Std_rexpand(Cyc_Core_heap_region,s,sz);}struct _tagged_arr Cyc_Std_rrealloc_str(
struct _RegionHandle*r,struct _tagged_arr str,unsigned int sz){unsigned int maxsizeP=
_get_arr_size(str,sizeof(unsigned char));if(maxsizeP == 0){maxsizeP=30 > sz? 30: sz;
str=Cyc_Core_rnew_string(r,maxsizeP);*((unsigned char*)_check_unknown_subscript(
str,sizeof(unsigned char),0))='\000';}else{if(sz > maxsizeP){maxsizeP=maxsizeP * 2
> (sz * 5)/ 4? maxsizeP * 2:(sz * 5)/ 4;str=Cyc_Std_rexpand(r,(struct _tagged_arr)str,
maxsizeP);}}return str;}struct _tagged_arr Cyc_Std_realloc_str(struct _tagged_arr str,
unsigned int sz){return Cyc_Std_rrealloc_str(Cyc_Core_heap_region,str,sz);}struct
_tagged_arr Cyc_Std_rsubstring(struct _RegionHandle*r,struct _tagged_arr s,int start,
unsigned int amt){if((start < 0? 1: amt < 0)? 1:(start + (int)amt)- 1 > (int)
_get_arr_size(s,sizeof(unsigned char))){(int)_throw((void*)({struct Cyc_Core_Invalid_argument_struct*
_tmpF=_cycalloc(sizeof(struct Cyc_Core_Invalid_argument_struct));_tmpF[0]=({
struct Cyc_Core_Invalid_argument_struct _tmp10;_tmp10.tag=Cyc_Core_Invalid_argument;
_tmp10.f1=_tag_arr("Std::substring",sizeof(unsigned char),15);_tmp10;});_tmpF;}));}{
struct _tagged_arr ans=Cyc_Core_rnew_string(r,amt + 1);amt < _get_arr_size(ans,
sizeof(unsigned char))? 0:((int(*)(struct _tagged_arr assertion,struct _tagged_arr
file,unsigned int line))Cyc_Std___assert_fail)(_tag_arr("amt < ans.size",sizeof(
unsigned char),15),_tag_arr("string.cyc",sizeof(unsigned char),11),448);{
unsigned int i=0;for(0;i < amt;++ i){((unsigned char*)ans.curr)[(int)i]=*((const
unsigned char*)_check_unknown_subscript(s,sizeof(unsigned char),(int)(start + i)));}}((
unsigned char*)ans.curr)[(int)amt]='\000';return ans;}}struct _tagged_arr Cyc_Std_substring(
struct _tagged_arr s,int start,unsigned int amt){return Cyc_Std_rsubstring(Cyc_Core_heap_region,
s,start,amt);}struct _tagged_arr Cyc_Std_rreplace_suffix(struct _RegionHandle*r,
struct _tagged_arr src,struct _tagged_arr curr_suffix,struct _tagged_arr new_suffix){
unsigned int m=_get_arr_size(src,sizeof(unsigned char));unsigned int n=
_get_arr_size(curr_suffix,sizeof(unsigned char));struct _tagged_arr err=_tag_arr("Std::replace_suffix",
sizeof(unsigned char),20);if(m < n){(int)_throw((void*)({struct Cyc_Core_Invalid_argument_struct*
_tmp11=_cycalloc(sizeof(struct Cyc_Core_Invalid_argument_struct));_tmp11[0]=({
struct Cyc_Core_Invalid_argument_struct _tmp12;_tmp12.tag=Cyc_Core_Invalid_argument;
_tmp12.f1=err;_tmp12;});_tmp11;}));}{unsigned int i=1;for(0;i <= n;++ i){if(*((
const unsigned char*)_check_unknown_subscript(src,sizeof(unsigned char),(int)(m - 
i)))!= *((const unsigned char*)_check_unknown_subscript(curr_suffix,sizeof(
unsigned char),(int)(n - i)))){(int)_throw((void*)({struct Cyc_Core_Invalid_argument_struct*
_tmp13=_cycalloc(sizeof(struct Cyc_Core_Invalid_argument_struct));_tmp13[0]=({
struct Cyc_Core_Invalid_argument_struct _tmp14;_tmp14.tag=Cyc_Core_Invalid_argument;
_tmp14.f1=err;_tmp14;});_tmp13;}));}}}{struct _tagged_arr ans=Cyc_Core_rnew_string(
r,(m - n)+ _get_arr_size(new_suffix,sizeof(unsigned char)));Cyc_Std_strncpy(ans,
src,m - n);Cyc_Std_strncpy(_tagged_arr_plus(_tagged_arr_plus(ans,sizeof(
unsigned char),(int)m),sizeof(unsigned char),-(int)n),new_suffix,_get_arr_size(
new_suffix,sizeof(unsigned char)));return ans;}}struct _tagged_arr Cyc_Std_replace_suffix(
struct _tagged_arr src,struct _tagged_arr curr_suffix,struct _tagged_arr new_suffix){
return Cyc_Std_rreplace_suffix(Cyc_Core_heap_region,src,curr_suffix,new_suffix);}
struct _tagged_arr Cyc_Std_strpbrk(struct _tagged_arr s,struct _tagged_arr accept){int
len=(int)Cyc_Std_int_strleno(s,_tag_arr("Std::strpbrk",sizeof(unsigned char),13));
unsigned char c='\000';unsigned int asize=_get_arr_size(accept,sizeof(
unsigned char));for(0;len > 0?(c=*((const unsigned char*)_check_unknown_subscript(
s,sizeof(unsigned char),0)))!= 0: 0;(len --,_tagged_arr_inplace_plus_post(& s,
sizeof(unsigned char),1))){int j=0;for(0;j < asize;j ++){if(c == ((const
unsigned char*)accept.curr)[j]){return s;}}}return(struct _tagged_arr)_tag_arr(0,0,
0);}struct _tagged_arr Cyc_Std_mstrpbrk(struct _tagged_arr s,struct _tagged_arr accept){
int len=(int)Cyc_Std_int_strleno((struct _tagged_arr)s,_tag_arr("Std::strpbrk",
sizeof(unsigned char),13));unsigned char c='\000';unsigned int asize=_get_arr_size(
accept,sizeof(unsigned char));for(0;len > 0?(c=*((unsigned char*)
_check_unknown_subscript(s,sizeof(unsigned char),0)))!= 0: 0;(len --,
_tagged_arr_inplace_plus_post(& s,sizeof(unsigned char),1))){int j=0;for(0;j < 
asize;j ++){if(c == ((const unsigned char*)accept.curr)[j]){return s;}}}return
_tag_arr(0,0,0);}struct _tagged_arr Cyc_Std_mstrchr(struct _tagged_arr s,
unsigned char c){int len=(int)Cyc_Std_int_strleno((struct _tagged_arr)s,_tag_arr("Std::strchr",
sizeof(unsigned char),12));unsigned char c2='\000';for(0;len > 0?(c2=*((
unsigned char*)_check_unknown_subscript(s,sizeof(unsigned char),0)))!= 0: 0;(len
--,_tagged_arr_inplace_plus_post(& s,sizeof(unsigned char),1))){if(c2 == c){return
s;}}return _tag_arr(0,0,0);}struct _tagged_arr Cyc_Std_strchr(struct _tagged_arr s,
unsigned char c){int len=(int)Cyc_Std_int_strleno(s,_tag_arr("Std::strchr",sizeof(
unsigned char),12));unsigned char c2='\000';for(0;len > 0?(c2=*((const
unsigned char*)_check_unknown_subscript(s,sizeof(unsigned char),0)))!= 0: 0;(len
--,_tagged_arr_inplace_plus_post(& s,sizeof(unsigned char),1))){if(c2 == c){return
s;}}return(struct _tagged_arr)_tag_arr(0,0,0);}struct _tagged_arr Cyc_Std_strrchr(
struct _tagged_arr s,unsigned char c){int len=(int)Cyc_Std_int_strleno(s,_tag_arr("Std::strrchr",
sizeof(unsigned char),13));int i=len - 1;_tagged_arr_inplace_plus(& s,sizeof(
unsigned char),i);for(0;i >= 0;(i --,_tagged_arr_inplace_plus_post(& s,sizeof(
unsigned char),-1))){if(*((const unsigned char*)_check_unknown_subscript(s,
sizeof(unsigned char),0))== c){return s;}}return(struct _tagged_arr)_tag_arr(0,0,0);}
struct _tagged_arr Cyc_Std_mstrrchr(struct _tagged_arr s,unsigned char c){int len=(int)
Cyc_Std_int_strleno((struct _tagged_arr)s,_tag_arr("Std::mstrrchr",sizeof(
unsigned char),14));int i=len - 1;_tagged_arr_inplace_plus(& s,sizeof(unsigned char),
i);for(0;i >= 0;(i --,_tagged_arr_inplace_plus_post(& s,sizeof(unsigned char),-1))){
if(*((unsigned char*)_check_unknown_subscript(s,sizeof(unsigned char),0))== c){
return s;}}return _tag_arr(0,0,0);}struct _tagged_arr Cyc_Std_strstr(struct
_tagged_arr haystack,struct _tagged_arr needle){if(!((unsigned int)haystack.curr)? 1:
!((unsigned int)needle.curr)){(int)_throw((void*)({struct Cyc_Core_Invalid_argument_struct*
_tmp15=_cycalloc(sizeof(struct Cyc_Core_Invalid_argument_struct));_tmp15[0]=({
struct Cyc_Core_Invalid_argument_struct _tmp16;_tmp16.tag=Cyc_Core_Invalid_argument;
_tmp16.f1=_tag_arr("Std::strstr",sizeof(unsigned char),12);_tmp16;});_tmp15;}));}
if(*((const unsigned char*)_check_unknown_subscript(needle,sizeof(unsigned char),
0))== '\000'){return haystack;}{int len=(int)Cyc_Std_int_strleno(needle,_tag_arr("Std::strstr",
sizeof(unsigned char),12));{struct _tagged_arr start=haystack;for(0;(start=Cyc_Std_strchr(
start,*((const unsigned char*)_check_unknown_subscript(needle,sizeof(
unsigned char),0)))).curr != ((struct _tagged_arr)_tag_arr(0,0,0)).curr;start=Cyc_Std_strchr(
_tagged_arr_plus(start,sizeof(unsigned char),1),*((const unsigned char*)
_check_unknown_subscript(needle,sizeof(unsigned char),0)))){if(Cyc_Std_strncmp(
start,needle,(unsigned int)len)== 0){return start;}}}return(struct _tagged_arr)
_tag_arr(0,0,0);}}struct _tagged_arr Cyc_Std_mstrstr(struct _tagged_arr haystack,
struct _tagged_arr needle){if(!((unsigned int)haystack.curr)? 1: !((unsigned int)
needle.curr)){(int)_throw((void*)({struct Cyc_Core_Invalid_argument_struct*_tmp17=
_cycalloc(sizeof(struct Cyc_Core_Invalid_argument_struct));_tmp17[0]=({struct Cyc_Core_Invalid_argument_struct
_tmp18;_tmp18.tag=Cyc_Core_Invalid_argument;_tmp18.f1=_tag_arr("Std::mstrstr",
sizeof(unsigned char),13);_tmp18;});_tmp17;}));}if(*((const unsigned char*)
_check_unknown_subscript(needle,sizeof(unsigned char),0))== '\000'){return
haystack;}{int len=(int)Cyc_Std_int_strleno(needle,_tag_arr("Std::mstrstr",
sizeof(unsigned char),13));{struct _tagged_arr start=haystack;for(0;(start=Cyc_Std_mstrchr(
start,*((const unsigned char*)_check_unknown_subscript(needle,sizeof(
unsigned char),0)))).curr != (_tag_arr(0,0,0)).curr;start=Cyc_Std_mstrchr(
_tagged_arr_plus(start,sizeof(unsigned char),1),*((const unsigned char*)
_check_unknown_subscript(needle,sizeof(unsigned char),0)))){if(Cyc_Std_strncmp((
struct _tagged_arr)start,needle,(unsigned int)len)== 0){return start;}}}return
_tag_arr(0,0,0);}}unsigned int Cyc_Std_strspn(struct _tagged_arr s,struct
_tagged_arr accept){unsigned int len=Cyc_Std_int_strleno(s,_tag_arr("Std::strspn",
sizeof(unsigned char),12));unsigned int asize=_get_arr_size(accept,sizeof(
unsigned char));len <= _get_arr_size(s,sizeof(unsigned char))? 0:((int(*)(struct
_tagged_arr assertion,struct _tagged_arr file,unsigned int line))Cyc_Std___assert_fail)(
_tag_arr("len <= s.size",sizeof(unsigned char),14),_tag_arr("string.cyc",sizeof(
unsigned char),11),596);{unsigned int i=0;for(0;i < len;i ++){int j;for(j=0;j < asize;
j ++){if(((const unsigned char*)s.curr)[(int)i]== ((const unsigned char*)accept.curr)[
j]){break;}}if(j == asize){return i;}}}return len;}unsigned int Cyc_Std_strcspn(
struct _tagged_arr s,struct _tagged_arr accept){unsigned int len=Cyc_Std_int_strleno(
s,_tag_arr("Std::strspn",sizeof(unsigned char),12));unsigned int asize=
_get_arr_size(accept,sizeof(unsigned char));len <= _get_arr_size(s,sizeof(
unsigned char))? 0:((int(*)(struct _tagged_arr assertion,struct _tagged_arr file,
unsigned int line))Cyc_Std___assert_fail)(_tag_arr("len <= s.size",sizeof(
unsigned char),14),_tag_arr("string.cyc",sizeof(unsigned char),11),616);{
unsigned int i=0;for(0;i < len;i ++){int j;for(j=0;j < asize;j ++){if(((const
unsigned char*)s.curr)[(int)i]!= ((const unsigned char*)accept.curr)[j]){break;}}
if(j == asize){return i;}}}return len;}struct _tagged_arr Cyc_Std_strtok(struct
_tagged_arr s,struct _tagged_arr delim){static struct _tagged_arr olds={(void*)0,(void*)
0,(void*)(0 + 0)};struct _tagged_arr token;if(s.curr == (_tag_arr(0,0,0)).curr){if(
olds.curr == (_tag_arr(0,0,0)).curr){return _tag_arr(0,0,0);}s=olds;}{unsigned int
inc=Cyc_Std_strspn((struct _tagged_arr)s,delim);if(inc >= _get_arr_size(s,sizeof(
unsigned char))? 1:*((unsigned char*)_check_unknown_subscript(_tagged_arr_plus(s,
sizeof(unsigned char),(int)inc),sizeof(unsigned char),0))== '\000'){olds=
_tag_arr(0,0,0);return _tag_arr(0,0,0);}else{_tagged_arr_inplace_plus(& s,sizeof(
unsigned char),(int)inc);}token=s;s=Cyc_Std_mstrpbrk(token,delim);if(s.curr == (
_tag_arr(0,0,0)).curr){olds=_tag_arr(0,0,0);}else{*((unsigned char*)
_check_unknown_subscript(s,sizeof(unsigned char),0))='\000';olds=
_tagged_arr_plus(s,sizeof(unsigned char),1);}return token;}}struct Cyc_List_List*
Cyc_Std_rexplode(struct _RegionHandle*r,struct _tagged_arr s){struct Cyc_List_List*
result=0;{int i=(int)(Cyc_Std_strlen(s)- 1);for(0;i >= 0;i --){result=({struct Cyc_List_List*
_tmp19=_region_malloc(r,sizeof(struct Cyc_List_List));_tmp19->hd=(void*)((int)((
const unsigned char*)s.curr)[i]);_tmp19->tl=result;_tmp19;});}}return result;}
struct Cyc_List_List*Cyc_Std_explode(struct _tagged_arr s){return Cyc_Std_rexplode(
Cyc_Core_heap_region,s);}struct _tagged_arr Cyc_Std_implode(struct Cyc_List_List*
chars){struct _tagged_arr s=Cyc_Core_new_string((unsigned int)((int(*)(struct Cyc_List_List*
x))Cyc_List_length)(chars));unsigned int i=0;while(chars != 0){*((unsigned char*)
_check_unknown_subscript(s,sizeof(unsigned char),(int)i ++))=(unsigned char)((int)
chars->hd);chars=chars->tl;}return s;}inline static int Cyc_Std_casecmp(struct
_tagged_arr s1,unsigned int len1,struct _tagged_arr s2,unsigned int len2){
unsigned int min_length=len1 < len2? len1: len2;(min_length <= _get_arr_size(s1,
sizeof(unsigned char))? min_length <= _get_arr_size(s2,sizeof(unsigned char)): 0)? 0:((
int(*)(struct _tagged_arr assertion,struct _tagged_arr file,unsigned int line))Cyc_Std___assert_fail)(
_tag_arr("min_length <= s1.size && min_length <= s2.size",sizeof(unsigned char),
47),_tag_arr("string.cyc",sizeof(unsigned char),11),697);{int i=- 1;while((++ i,i < 
min_length)){int diff=toupper((int)((const unsigned char*)s1.curr)[i])- toupper((
int)((const unsigned char*)s2.curr)[i]);if(diff != 0){return diff;}}return(int)len1
- (int)len2;}}int Cyc_Std_strcasecmp(struct _tagged_arr s1,struct _tagged_arr s2){if(
s1.curr == s2.curr){return 0;}{unsigned int len1=Cyc_Std_int_strleno(s1,_tag_arr("Std::strcasecmp",
sizeof(unsigned char),16));unsigned int len2=Cyc_Std_int_strleno(s2,_tag_arr("Std::strcasecmp",
sizeof(unsigned char),16));return Cyc_Std_casecmp(s1,len1,s2,len2);}}inline static
int Cyc_Std_caseless_ncmp(struct _tagged_arr s1,unsigned int len1,struct _tagged_arr
s2,unsigned int len2,unsigned int n){if(n <= 0){return 0;}{unsigned int min_len=len1 > 
len2? len2: len1;unsigned int bound=min_len > n? n: min_len;(bound <= _get_arr_size(s1,
sizeof(unsigned char))? bound <= _get_arr_size(s2,sizeof(unsigned char)): 0)? 0:((
int(*)(struct _tagged_arr assertion,struct _tagged_arr file,unsigned int line))Cyc_Std___assert_fail)(
_tag_arr("bound <= s1.size && bound <= s2.size",sizeof(unsigned char),37),
_tag_arr("string.cyc",sizeof(unsigned char),11),724);{int i=0;for(0;i < bound;i ++){
int retc;if((retc=toupper((int)((const unsigned char*)s1.curr)[i])- toupper((int)((
const unsigned char*)s2.curr)[i]))!= 0){return retc;}}}if(len1 < n? 1: len2 < n){
return(int)len1 - (int)len2;}return 0;}}int Cyc_Std_strncasecmp(struct _tagged_arr s1,
struct _tagged_arr s2,unsigned int n){unsigned int len1=Cyc_Std_int_strleno(s1,
_tag_arr("Std::strncasecmp",sizeof(unsigned char),17));unsigned int len2=Cyc_Std_int_strleno(
s2,_tag_arr("Std::strncasecmp",sizeof(unsigned char),17));return Cyc_Std_caseless_ncmp(
s1,len1,s2,len2,n);}extern void*memcpy(void*,const void*,unsigned int n);extern void*
memmove(void*,const void*,unsigned int n);extern int memcmp(const void*,const void*,
unsigned int n);extern unsigned char*memchr(const unsigned char*,unsigned char c,
unsigned int n);extern void*memset(void*,int c,unsigned int n);extern void bcopy(const
void*src,void*dest,unsigned int n);extern void bzero(void*s,unsigned int n);extern
unsigned char*GC_realloc(unsigned char*,unsigned int n);struct _tagged_arr Cyc_Std_realloc(
struct _tagged_arr s,unsigned int n){unsigned char*_tmp1A=GC_realloc((unsigned char*)
_check_null(_untag_arr(s,sizeof(unsigned char),1)),n);return
wrap_Cstring_as_string(_tmp1A,n);}struct _tagged_arr Cyc_Std__memcpy(struct
_tagged_arr d,struct _tagged_arr s,unsigned int n,unsigned int sz){if(((d.curr == (
_tag_arr(0,0,0)).curr? 1: _get_arr_size(d,sizeof(void))< n)? 1: s.curr == ((struct
_tagged_arr)_tag_arr(0,0,0)).curr)? 1: _get_arr_size(s,sizeof(void))< n){(int)
_throw((void*)({struct Cyc_Core_Invalid_argument_struct*_tmp1B=_cycalloc(sizeof(
struct Cyc_Core_Invalid_argument_struct));_tmp1B[0]=({struct Cyc_Core_Invalid_argument_struct
_tmp1C;_tmp1C.tag=Cyc_Core_Invalid_argument;_tmp1C.f1=_tag_arr("Std::memcpy",
sizeof(unsigned char),12);_tmp1C;});_tmp1B;}));}memcpy((void*)_check_null(
_untag_arr(d,sizeof(void),1)),(const void*)_check_null(_untag_arr(s,sizeof(void),
1)),n * sz);return d;}struct _tagged_arr Cyc_Std__memmove(struct _tagged_arr d,struct
_tagged_arr s,unsigned int n,unsigned int sz){if(((d.curr == (_tag_arr(0,0,0)).curr?
1: _get_arr_size(d,sizeof(void))< n)? 1: s.curr == ((struct _tagged_arr)_tag_arr(0,0,
0)).curr)? 1: _get_arr_size(s,sizeof(void))< n){(int)_throw((void*)({struct Cyc_Core_Invalid_argument_struct*
_tmp1D=_cycalloc(sizeof(struct Cyc_Core_Invalid_argument_struct));_tmp1D[0]=({
struct Cyc_Core_Invalid_argument_struct _tmp1E;_tmp1E.tag=Cyc_Core_Invalid_argument;
_tmp1E.f1=_tag_arr("Std::memove",sizeof(unsigned char),12);_tmp1E;});_tmp1D;}));}
memmove((void*)_check_null(_untag_arr(d,sizeof(void),1)),(const void*)_check_null(
_untag_arr(s,sizeof(void),1)),n * sz);return d;}int Cyc_Std_memcmp(struct
_tagged_arr s1,struct _tagged_arr s2,unsigned int n){if(((s1.curr == ((struct
_tagged_arr)_tag_arr(0,0,0)).curr? 1: s2.curr == ((struct _tagged_arr)_tag_arr(0,0,0)).curr)?
1: _get_arr_size(s1,sizeof(unsigned char))>= n)? 1: _get_arr_size(s2,sizeof(
unsigned char))>= n){(int)_throw((void*)({struct Cyc_Core_Invalid_argument_struct*
_tmp1F=_cycalloc(sizeof(struct Cyc_Core_Invalid_argument_struct));_tmp1F[0]=({
struct Cyc_Core_Invalid_argument_struct _tmp20;_tmp20.tag=Cyc_Core_Invalid_argument;
_tmp20.f1=_tag_arr("Std::memcmp",sizeof(unsigned char),12);_tmp20;});_tmp1F;}));}
return memcmp((const void*)_check_null(_untag_arr(s1,sizeof(unsigned char),1)),(
const void*)_check_null(_untag_arr(s2,sizeof(unsigned char),1)),n);}struct
_tagged_arr Cyc_Std_memchr(struct _tagged_arr s,unsigned char c,unsigned int n){
unsigned int sz=_get_arr_size(s,sizeof(unsigned char));if(s.curr == ((struct
_tagged_arr)_tag_arr(0,0,0)).curr? 1: n > sz){(int)_throw((void*)({struct Cyc_Core_Invalid_argument_struct*
_tmp21=_cycalloc(sizeof(struct Cyc_Core_Invalid_argument_struct));_tmp21[0]=({
struct Cyc_Core_Invalid_argument_struct _tmp22;_tmp22.tag=Cyc_Core_Invalid_argument;
_tmp22.f1=_tag_arr("Std::memchr",sizeof(unsigned char),12);_tmp22;});_tmp21;}));}{
unsigned char*_tmp23=memchr((const unsigned char*)_check_null(_untag_arr(s,
sizeof(unsigned char),1)),c,n);if(_tmp23 == 0){return(struct _tagged_arr)_tag_arr(
0,0,0);}{unsigned int _tmp24=(unsigned int)s.curr;unsigned int _tmp25=(
unsigned int)_tmp23;unsigned int _tmp26=_tmp25 - _tmp24;return _tagged_arr_plus(s,
sizeof(unsigned char),(int)_tmp26);}}}struct _tagged_arr Cyc_Std_mmemchr(struct
_tagged_arr s,unsigned char c,unsigned int n){unsigned int sz=_get_arr_size(s,
sizeof(unsigned char));if(s.curr == (_tag_arr(0,0,0)).curr? 1: n > sz){(int)_throw((
void*)({struct Cyc_Core_Invalid_argument_struct*_tmp27=_cycalloc(sizeof(struct Cyc_Core_Invalid_argument_struct));
_tmp27[0]=({struct Cyc_Core_Invalid_argument_struct _tmp28;_tmp28.tag=Cyc_Core_Invalid_argument;
_tmp28.f1=_tag_arr("Std::mmemchr",sizeof(unsigned char),13);_tmp28;});_tmp27;}));}{
unsigned char*_tmp29=memchr((const unsigned char*)_check_null(_untag_arr(s,
sizeof(unsigned char),1)),c,n);if(_tmp29 == 0){return _tag_arr(0,0,0);}{
unsigned int _tmp2A=(unsigned int)s.curr;unsigned int _tmp2B=(unsigned int)_tmp29;
unsigned int _tmp2C=_tmp2B - _tmp2A;return _tagged_arr_plus(s,sizeof(unsigned char),(
int)_tmp2C);}}}struct _tagged_arr Cyc_Std_memset(struct _tagged_arr s,unsigned char c,
unsigned int n){if(s.curr == (_tag_arr(0,0,0)).curr? 1: n > _get_arr_size(s,sizeof(
unsigned char))){(int)_throw((void*)({struct Cyc_Core_Invalid_argument_struct*
_tmp2D=_cycalloc(sizeof(struct Cyc_Core_Invalid_argument_struct));_tmp2D[0]=({
struct Cyc_Core_Invalid_argument_struct _tmp2E;_tmp2E.tag=Cyc_Core_Invalid_argument;
_tmp2E.f1=_tag_arr("Std::memset",sizeof(unsigned char),12);_tmp2E;});_tmp2D;}));}
memset((void*)((unsigned char*)_check_null(_untag_arr(s,sizeof(unsigned char),1))),(
int)c,n);return s;}void Cyc_Std_bzero(struct _tagged_arr s,unsigned int n){if(s.curr
== (_tag_arr(0,0,0)).curr? 1: _get_arr_size(s,sizeof(unsigned char))< n){(int)
_throw((void*)({struct Cyc_Core_Invalid_argument_struct*_tmp2F=_cycalloc(sizeof(
struct Cyc_Core_Invalid_argument_struct));_tmp2F[0]=({struct Cyc_Core_Invalid_argument_struct
_tmp30;_tmp30.tag=Cyc_Core_Invalid_argument;_tmp30.f1=_tag_arr("Std::bzero",
sizeof(unsigned char),11);_tmp30;});_tmp2F;}));}((void(*)(unsigned char*s,
unsigned int n))bzero)((unsigned char*)_check_null(_untag_arr(s,sizeof(
unsigned char),1)),n);}void Cyc_Std__bcopy(struct _tagged_arr src,struct _tagged_arr
dst,unsigned int n,unsigned int sz){if(((src.curr == ((struct _tagged_arr)_tag_arr(0,
0,0)).curr? 1: _get_arr_size(src,sizeof(void))< n)? 1: dst.curr == (_tag_arr(0,0,0)).curr)?
1: _get_arr_size(dst,sizeof(void))< n){(int)_throw((void*)({struct Cyc_Core_Invalid_argument_struct*
_tmp31=_cycalloc(sizeof(struct Cyc_Core_Invalid_argument_struct));_tmp31[0]=({
struct Cyc_Core_Invalid_argument_struct _tmp32;_tmp32.tag=Cyc_Core_Invalid_argument;
_tmp32.f1=_tag_arr("Std::bcopy",sizeof(unsigned char),11);_tmp32;});_tmp31;}));}
bcopy((const void*)_check_null(_untag_arr(src,sizeof(void),1)),(void*)_check_null(
_untag_arr(dst,sizeof(void),1)),n * sz);}
