extern	struct	_iobuf {
int	_cnt;
unsigned char *_ptr;
unsigned char *_base;
int	_bufsiz;
short	_flag;
char	_file;	} _iob[];
typedef struct _iobuf FILE;
extern struct _iobuf	*fopen(const char *, const char *);
extern struct _iobuf	*fdopen(int, const char *);
extern struct _iobuf	*freopen(const char *, const char *, FILE *);
extern struct _iobuf	*popen(const char *, const char *);
extern struct _iobuf	*tmpfile(void);
extern long	ftell(FILE *);
extern char	*fgets(char *, int, FILE *);
extern char	*gets(char *);
extern char	*sprintf(char *, const char *, ...);
extern char	*ctermid(char *);
extern char	*cuserid(char *);
extern char	*tempnam(const char *, const char *);
extern char	*tmpnam(char *);
extern	char	_ctype_[];
typedef	int jmp_buf[9];
typedef	int sigjmp_buf[9+1];
int	setjmp(jmp_buf);
int	_setjmp(jmp_buf);
int	sigsetjmp(sigjmp_buf, int);
void	longjmp(jmp_buf, int);
void	_longjmp(jmp_buf, int);
void	siglongjmp(sigjmp_buf, int);
typedef struct node {
char n_type;	char n_flags;	union {	struct xsym {	struct node *xsy_plist;	struct node *xsy_value;	} n_xsym;
struct xsubr {	struct node *(*xsu_subr)();	} n_xsubr;
struct xlist {	struct node *xl_car;	struct node *xl_cdr;	} n_xlist;
struct xint {	long xi_int;	} n_xint;
struct xfloat {	float xf_float;	} n_xfloat;
struct xstr {	int xst_type;	char *xst_str;	} n_xstr;
struct xfptr {	struct _iobuf *xf_fp;	int xf_savech;	} n_xfptr;
struct xvect {	int xv_size;	struct node **xv_data;	} n_xvect;
} n_info;
} NODE;
typedef struct context {
int c_flags;	struct node *c_expr;	jmp_buf c_jmpbuf;	struct context *c_xlcontext;	struct node ***c_xlstack;	struct node *c_xlenv;	int c_xltrace;	} CONTEXT;
struct fdef {
char *f_name;	int f_type;	struct node *(*f_fcn)();	};
struct segment {
int sg_size;
struct segment *sg_next;
struct node sg_nodes[1];
};
extern struct node ***xlsave(NODE ** arg1, ...);	extern struct node *xleval();	extern struct node *xlapply();	extern struct node *xlevlist();	extern struct node *xlarg();	extern struct node *xlevarg();	extern struct node *xlmatch();	extern struct node *xlevmatch();	extern struct node *xlgetfile();	extern struct node *xlsend();	extern struct node *xlenter();	extern struct node *xlsenter();	extern struct node *xlmakesym();	extern struct node *xlframe();	extern struct node *xlgetvalue();	extern struct node *xlxgetvalue();	extern struct node *xlygetvalue();	extern struct node *cons();	extern struct node *consa();	extern struct node *consd();	extern struct node *cvsymbol();	extern struct node *cvcsymbol();	extern struct node *cvstring();	extern struct node *cvcstring();	extern struct node *cvfile();	extern struct node *cvsubr();	extern struct node *cvfixnum();	extern struct node *cvflonum();	extern struct node *newstring();	extern struct node *newvector();	extern struct node *newobject();	extern struct node *xlgetprop();	extern char *xlsymname();	extern void xlsetvalue();
extern void xlprint();
extern void xltest();
NODE *true = (NODE *)0, *s_dot = (NODE *)0;
NODE *s_quote = (NODE *)0, *s_function = (NODE *)0;
NODE *s_bquote = (NODE *)0, *s_comma = (NODE *)0, *s_comat = (NODE *)0;
NODE *s_evalhook = (NODE *)0, *s_applyhook = (NODE *)0;
NODE *s_lambda = (NODE *)0, *s_macro = (NODE *)0;
NODE *s_stdin = (NODE *)0, *s_stdout = (NODE *)0, *s_rtable = (NODE *)0;
NODE *s_tracenable = (NODE *)0, *s_tlimit = (NODE *)0, *s_breakenable = (NODE *)0;
NODE *s_car = (NODE *)0, *s_cdr = (NODE *)0, *s_nth = (NODE *)0;
NODE *s_get = (NODE *)0, *s_svalue = (NODE *)0, *s_splist = (NODE *)0, *s_aref = (NODE *)0;
NODE *s_eql = (NODE *)0, *k_test = (NODE *)0, *k_tnot = (NODE *)0;
NODE *k_wspace = (NODE *)0, *k_const = (NODE *)0, *k_nmacro = (NODE *)0, *k_tmacro = (NODE *)0;
NODE *k_optional = (NODE *)0, *k_rest = (NODE *)0, *k_aux = (NODE *)0;
NODE *a_subr = (NODE *)0, *a_fsubr = (NODE *)0;
NODE *a_list = (NODE *)0, *a_sym = (NODE *)0, *a_int = (NODE *)0, *a_float = (NODE *)0;
NODE *a_str = (NODE *)0, *a_obj = (NODE *)0, *a_fptr = (NODE *)0, *a_vect;
NODE *obarray = (NODE *)0, *s_unbound = (NODE *)0;
NODE ***xlstack = 0, ***xlstkbase = 0, ***xlstktop = 0;
NODE *xlenv = (NODE *)0;
CONTEXT *xlcontext = 0;	NODE *xlvalue = (NODE *)0;	int xldebug = 0;	int xltrace = -1;	NODE **trace_stack = 0;	int xlsample = 0;	char gsprefix[100+1] = { 'G',0 };	int gsnumber = 1;	int prompt = 1; int xlplevel = 0;	int xlfsize = 0;	long total = 0L;	int anodes = 0;	int nnodes = 0;	int nsegs = 0;	int nfree = 0;	int gccalls = 0;	struct segment *segs = 0;	NODE *fnodes = (NODE *)0;	NODE *self = (NODE *)0, *class = (NODE *)0, *object = (NODE *)0;
NODE *new = (NODE *)0, *isnew = (NODE *)0, *msgcls = (NODE *)0, *msgclass = (NODE *)0;
char buf[100+1] = { 0 };