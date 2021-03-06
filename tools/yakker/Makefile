include ../../Makefile.inc

# Yakker is bootstrapped: compiling it requires files that it
# produces.  It is stored in the repository with copies of those files
# in the bootfiles directory, so that yakker can be built in the first
# place.  Yakker developers can develop new versions of those files
# through the sequence "make all boot", resulting in a new yakker
# executable; to make this permanent for the repository do "make
# update".

# How to deal with nullable symbols:
# Plan A: short circuit them and they are lost during reconstruction.
# Use -DNULL_PLAN_A
# Plan B: don't short circuit them. Instead, at beginning of DFA and at calls,
# check whether call target is a final state and do an immediate return,
# if so. RETURN closure is modified to allow returns on epsilon.
# Use -DNULL_PLAN_B
# Plan C: Short circuit nullable symbols by marking their start state as final
# *and*, at beginning of DFA and at calls,
# check whether call target is a final state and, if so, do an immediate return.
# Use -DNULL_PLAN_C
CYCFLAGS=--novcgen -DNULL_PLAN_C
#CYCFLAGS=-g -save-c -pp
YAKFLAGS=

CURDIR=$(shell pwd)

PROG=yakker$(EXE)

LIB_YAKKER=libyakker.a
LIB_YAKKER_SSL=libyakker-ssl.a
# compile SSL version only if SSL has been installed on the machine
ifneq ($LIBSSL,"")
  MAYBE_SSL=$(LIB_YAKKER_SSL)
endif

# Set this to yakker to make all targets generated with yakker depend
# on yakker.  Useful to turn off when simultenously modifying yakker
# and working on a program based on generated code.
PROG_DEP=$(PROG)

GENDIR=gen
BNFDIR=$(GENDIR)
BINDIR=$(CYCDIR)/bin
# examples directory. Use full path to avoid ambiguity.
EXDIR=$(CURDIR)/examples
CYC_LIB_PATH = $(BINDIR)/lib
CYCLONE=$(BINDIR)/cyclone -I../../include -B$(BINDIR)/lib/cyc-lib -L$(CYC_LIB_PATH)
CYCBISON=$(BINDIR)/cycbison
CYCLEX=$(BINDIR)/cyclex
STRINGIFY=$(BINDIR)/stringify

BOOT_FILES = instantiate ykbuf

YAKKER_FILES = analyze axarray cs bnf extract \
  semiring fa dfa-core nfa_lazy nfa_eager \
  flatten termgrammar lex pads \
  parse_tab parsergen crawlergen pr prose \
  strings_yakker corerules \
  $(BOOT_FILES) \
  util namespaces earley-common earley \
  yakker

DEPENDS += $(foreach file,$(YAKKER_FILES),build/$(file).d)

OBJS=$(foreach yfile,$(YAKKER_FILES),$(yfile).o)

# OBJS compiled for allocation profiling.
OBJS_PA=$(foreach yfile,$(YAKKER_FILES),$(yfile)_a.o)
PROG_PA=$(PROG)_a

#######################################
## Earley-related variables
#######################################
# Earley-backend files.
EBE_FILES = axarray semiring dfa-core earley-common util \
	    earley-backend

#######################################
## libyakker-related variables
#######################################

LIB_YAKKER_FILES = ykbuf funtable $(EBE_FILES)

LIB_YAKKER_OBJS = $(foreach file,$(LIB_YAKKER_FILES),$(file).o)

LY_ONLY_FILES = $(filter-out $(YAKKER_FILES), $(LIB_YAKKER_FILES))
DEPENDS += $(foreach file,$(LY_ONLY_FILES),build/$(file).d)

define ARCHIVE
	-$(RM) $@
	$(AR) rc $@ $^
	$(RANLIB) $@
endef

###############################################################################
# Targets
###############################################################################

all: $(PROG) $(LIB_YAKKER) $(MAYBE_SSL)

cyclone-install: $(BINDIR)/$(PROG)

boot:
	make $(PROG) BOOTSTRAP=X

update: $(PROG) $(foreach file,$(BOOT_FILES),$(file).cyc)
	cp $(foreach file,$(BOOT_FILES),$(file).cyc) bootfiles

ifdef BOOTSTRAP
instantiate.cyc: instantiate.yk
	./$(PROG) -gen sequence -no-main $< > $@
else
%.cyc: bootfiles/%.cyc
	cp $< $@
endif

$(BINDIR)/$(PROG): $(PROG)
	cp $< $@

$(PROG): $(OBJS)
	$(CYCLONE) $(CYCFLAGS) $^ -o $@ -lm

$(PROG_PA): $(OBJS_PA)
	$(CYCLONE) $(CYCFLAGS) -pa $^ -o $@ -lm

testindexer: indexer 1rfc_index
	./indexer < 1rfc_index

indexer: indexer.o ykbuf.o bnf.o extract.o pr.o parse_tab.o util.o lex.o cs.o
	$(CYCLONE) $^ -o $@

# --novcgen is temporary until a bug is fixed
indexer.o: indexer.cyc
	$(CYCLONE) -c $< -o $@ --novcgen

indexer.cyc: examples/indexer.yk $(PROG_DEP)
	./yakker $(YAKFLAGS) -gen entry-list $< > $@

#########################
## Earley-backend targets

$(LIB_YAKKER) : $(LIB_YAKKER_OBJS)
	$(ARCHIVE)

ifneq ($LIBSSL,"")
$(LIB_YAKKER_SSL) : ssl.o
	$(ARCHIVE)
endif

include examples/abnf-echo.mk
include examples/calc.mk
include examples/xpath.mk

# Broken: need to be fixed:
#include examples/indexer.mk

#####################################

wf: wf.o ykbuf.o
	$(CYCLONE) $^ -o $@

wf.cyc: examples/wf.yk $(PROG_DEP)
	./yakker $(YAKFLAGS) -gen file $< > $@

imapcl: imapcl.o ykbuf.o ssl.o imap.o
	$(CYCLONE) $^ -o $@ -lssl -lcrypto

imapcl.o: examples/imapcl.cyc
	$(CYCLONE) -c $^ -o $@ -I.

# --novcgen is temporary until a bug is fixed
imap.o: imap.cyc
	$(CYCLONE) -I. -c $< -o $@ --novcgen

imap.cyc: gen/imap_revised_fixed.bnf $(PROG_DEP)
	./yakker $(YAKFLAGS) -gen command -no-main -no-globals $< > $@

imapp.key:
	openssl genrsa > $@

imapp.cert: imapp.key
	openssl req -new -key $< -x509 -subj /C=aa/ST=b/L=c/O=d/OU=e/CN=shoebox.research.att.com > $@

imapp: imapp.o ykbuf.o ssl.o imap.o
	$(CYCLONE) $^ -o $@ -lssl -lcrypto

imapp.o: examples/imapp.cyc
	$(CYCLONE) -c $^ -o $@ -I.

# Echo server

echop.o: examples/echop.cyc
	$(CYCLONE) -c $^ -o $@ -I.

echop: echop.o ykbuf.o ssl.o
	$(CYCLONE) $^ -o $@ -lssl -lcrypto

# If LIBSQLITE isn't set in the environment, set it here.
# Useful to set in env. if running on OS X, in which
# case you need to link directly to the library.
ifndef LIBSQLITE
LIBSQLITE=-lsqlite3
endif

imapserver.o: $(EXDIR)/imapserver.cyc
	$(CYCLONE) -c $^ -o $@ -I.

# Earley-based IMAP server
imapserver: imapserver.o imap-scanf.o cs.o imap-tg-dfa.o imap-grm-dfa.o $(LIB_YAKKER) $(LIB_YAKKER_SSL)
	$(CYCLONE) -o $@ $^ $(LIBSQLITE) -lpthread -lssl -lcrypto -lm

include examples/imap.mk

###############################################################################
# Rules for full imap grammar with pattern-matching support.
###############################################################################
# imap_genpm.cyc: imap_genpm.bnf $(PROG_DEP)_a
#	./yakker_a $(YAKFLAGS) -all-start -no-minus-elim -gen command -no-main $< > $@

# Use -no-minus-elim as minus elimination was already done, and redoing it causes problems (in particular,
# the charset optimization loses necessary semantic actions, and flattening can be undone).
imap_genpm.cyc: $(GENDIR)/imap_genpm.bnf $(PROG_DEP)
	./yakker $(YAKFLAGS) -gen-crawl command \
			     -cyc-namespace ImapFormat \
			     -all-start \
			     -no-minus-elim \
			     -no-main \
			     -no-globals \
			     $< > $@

imap_genpm.h: $(GENDIR)/imap_genpm.bnf $(PROG_DEP)
	./yakker $(YAKFLAGS) -cyc-namespace ImapFormat -gen-crawl-header $< > $@

# $(GENDIR)/imap_genpm_lf.bnf: $(BNFDIR)/imap_genpm.bnf $(PROG_DEP)
#	./yakker -left-factor $< > $@

$(GENDIR)/imap_genpm.bnf: $(BNFDIR)/imap_revised_fixed.bnf $(PROG_DEP)
	./yakker -escape "\\%()" -flatten-full -bindgrammar -termgrammar_bnf $< > $@

$(GENDIR)/imap_bind.bnf: $(BNFDIR)/imap.bnf $(PROG_DEP)
	./yakker -escape "\\%()" -flatten -bindgrammar -no-anon-bind $< > $@

$(GENDIR)/imap_flat.bnf: $(BNFDIR)/imap.bnf $(PROG_DEP)
	./yakker -escape "\\%()" -flatten $< > $@

###############################################################################
# Pattern matching BNF examples.
###############################################################################

pm_bnf.o : pm_bnf.cyc pm_bnf.h pads.h bnf.h
	$(CYCLONE) -c $< -o $@ -I.

###############################################################################

boxextract: boxextract.o ykbuf.o
	$(CYCLONE) $^ -o $@

boxextract.cyc: boxextract.yk $(PROG_DEP)
	./yakker $(YAKFLAGS) -gen lines $< > $@

TEMPLATES = dfa_engine1.cyc dfa_engine2_1.str crawl_main.cyc\
            prose_elim_grm.bnf corerules_grm.bnf
strings_yakker.cyc: $(foreach file,$(TEMPLATES),templates/$(file))
	$(STRINGIFY) $^ > $@

###############################################################################
# Automatic dependency generation
###############################################################################

build/%.d: %.cyc
	$(CYCLONE) -M -MG $< > $@

.PHONY: depend

depend: $(DEPEND)

###############################################################################

.SECONDARY: parse_tab.cyc

uri.p: $(PROG_DEP) uri.bnf
	$(RM) $@.tmp
	./yakker -pads uri.bnf > $@.tmp
	cat $@.tmp | ./fix_hostname.pl | ./add_precord.pl URI_reference_t | cat > $@
	$(RM) $@.tmp

uri_one_string.p: $(PROG_DEP) uri_orig.bnf
	$(RM) $@.tmp
	./yakker -pads -re uri_orig.bnf > $@.tmp
	cat $@.tmp | ./fix_hostname.pl | ./add_precord.pl URI_reference_t | cat > $@
	$(RM) $@.tmp

gen/sip.bnf: $(PROG_DEP) gen/rfc3261.bnf
	./yakker gen/rfc3261.bnf -omit generic-message start-line Request-Line Status-Line header response opaque URI s userinfo reg-name domainlabel > $@
	echo 'header = hname "=" hvalue.' >> $@
	echo 'userinfo = user [":" password] "@".' >> $@
	echo 'opaque = "opaque" EQUAL quoted-string.' >> $@
	echo 'URI = absoluteURI|abs-path.' >> $@
	echo 'Request-Line = Method SP Request-URI SP SIP-Version CRLF.' >> $@
	echo 'Status-Line = SIP-Version SP Status-Code SP Reason-Phrase CRLF.' >> $@
	echo 'EPS1 = "".' >> $@
	echo 'EPS2 = "".' >> $@
	echo 'reg-name = 1*((unreserved|escaped|"$$"|","|";"|":"|"@"|"&"|"="|"+")EPS1).' >> $@
	echo 'domainlabel = alphanum|(alphanum *((alphanum|"-")EPS2) alphanum).' >> $@

gen/http.bnf: $(PROG_DEP) gen/rfc2616.bnf gen/rfc2617.bnf gen/rfc2396.bnf gen/rfc2822.bnf gen/rfc3501.bnf
	./yakker gen/rfc2616.bnf \
	  -externals gen/rfc2617.bnf gen/rfc2396.bnf gen/rfc2822.bnf gen/rfc3501.bnf \
	   >$@

gen/message-header.bnf: http.bnf $(PROG_DEP)
	./yakker $< -subset message-header > $@

$(BNFDIR)/imap.bnf: $(BNFDIR)/rfc3501.bnf $(PROG_DEP)
	./yakker $(BNFDIR)/rfc3501.bnf -tsort -unnamespace > $@
	echo 'my-greeting = ("" greeting)$$x {greeting_hook(x);}.' >> $@
	echo 'my-response = ("" response)$$x {response_hook(x);}.' >> $@
	echo 'my-command = ("" command)$$x {command_hook(x);}.' >> $@
	echo '{ extern void greeting_hook(const char ?); }' >> $@
	echo '{ extern void response_hook(const char ?); }' >> $@
	echo '{ extern void command_hook(const char ?); }' >> $@

$(BNFDIR)/imap-ext%.bnf: $(BNFDIR)/rfc%.bnf $(PROG_DEP)
	./yakker $< -tsort -unnamespace > $@

# $(BNFDIR)/imap-ext2342.bnf is causing problems. Not extracted correctly.
#	mv $@.tmp $@
#	./yakker $@ -omit `./yakker -list-defs $(BNFDIR)/imap-ext2342.bnf` > $@.tmp
#	cat $(BNFDIR)/imap-ext2342.bnf >> $@.tmp
$(BNFDIR)/imap_revised.bnf: $(BNFDIR)/rfc3501.bnf \
  $(BNFDIR)/imap-ext2088.bnf  \
  $(BNFDIR)/imap-ext3502.bnf $(BNFDIR)/imap-ext3516.bnf \
  $(BNFDIR)/imap-ext4466.bnf
	./yakker $(BNFDIR)/rfc3501.bnf -tsort -unnamespace > $@
	./yakker $@ -omit `./yakker -list-defs $(BNFDIR)/imap-ext2088.bnf` > $@.tmp
	cat $(BNFDIR)/imap-ext2088.bnf >> $@.tmp
	mv $@.tmp $@
	./yakker $@ -omit `./yakker -list-defs $(BNFDIR)/imap-ext3502.bnf` > $@.tmp
	cat $(BNFDIR)/imap-ext3502.bnf >> $@.tmp
	mv $@.tmp $@
	./yakker $@ -omit `./yakker -list-defs $(BNFDIR)/imap-ext3516.bnf` > $@.tmp
	cat $(BNFDIR)/imap-ext3516.bnf >> $@.tmp
	mv $@.tmp $@
	./yakker $@ -omit `./yakker -list-defs $(BNFDIR)/imap-ext4466.bnf` > $@.tmp
	cat $(BNFDIR)/imap-ext4466.bnf >> $@.tmp
	./yakker $@.tmp -tsort > $@
	rm $@.tmp

# FIX: not sure of the real definition of set. Looks to be identical to sequence-set.
# session is added for conveinience.
# FIX: flattened ["+"] by hand b/c haven't written code yet to properly flatten dep. sequences.
$(BNFDIR)/imap_revised_fixed.bnf: $(BNFDIR)/imap_revised.bnf
	./yakker $< -rename set sequence-set -omit literal literal8 mailbox-data > $@.tmp
	echo 'mailbox-data = ("FLAGS" SP flag-list)|("LIST" SP mailbox-list)|("LSUB" SP mailbox-list)|("SEARCH" *(SP nz-number))|("STATUS" SP mailbox SP "(" [status-att-list] ")")|(number SP "EXISTS")|(number SP "RECENT")|Namespace-Response|esearch-response.' >> $@.tmp
	echo 'PLUS = "+".' >> $@.tmp
	echo 'optional-plus = [PLUS].' >> $@.tmp
	echo 'literal = "{" number$$number optional-plus "}" CRLF @repeat(number)CHAR8.' >> $@.tmp
	echo 'literal8 = "~{" number$$number optional-plus "}" CRLF @repeat(number)OCTET.' >> $@.tmp
	./yakker -tsort $@.tmp > $@
	rm $@.tmp
	echo 'my-greeting = ("" greeting)$$x {greeting_hook(x);}.' >> $@
	echo 'my-response = ("" response)$$x {response_hook(x);}.' >> $@
	echo 'my-command = ("" command)$$x {command_hook(x);}.' >> $@
	echo "my-client-session = 1*command." >> $@
	echo "my-server-session = greeting *response." >> $@
	echo '{ extern void greeting_hook(const char ?); }' >> $@
	echo '{ extern void response_hook(const char ?); }' >> $@
	echo '{ extern void command_hook(const char ?); }' >> $@


$(BNFDIR)/vcard.bnf: $(BNFDIR)/rfc2426.bnf $(BNFDIR)/rfc2425.bnf $(BNFDIR)/rfc1738.bnf \
		     $(BNFDIR)/rfc1766.bnf $(BNFDIR)/rfc3501.bnf $(PROG_DEP)
# 1766: for Language-Tag
# 1738: for genericurl
# 3501: for base64
	./yakker $(BNFDIR)/rfc2426.bnf -externals $(BNFDIR)/rfc2425.bnf \
		 $(BNFDIR)/rfc1738.bnf $(BNFDIR)/rfc1766.bnf $(BNFDIR)/rfc3501.bnf -tsort -unnamespace > $@

clean:
	$(RM) *.d *.o parse_tab.cyc parse_tab.h lex.cyc $(PROG)
	$(RM) in out errs parse.output
	$(RM) http.bnf http.p message-header.bnf message-header.p
	$(RM) imap.bnf imap.h imap.cyc rfc3501.bnf rfc2234.bnf rfc3501.txt rfc2234.txt
	$(RM) imap_genpm.cyc
	$(RM) imap.h imap.cyc imap_genpm.h imap_genpm.cyc
	$(RM) indexer.h indexer.cyc indexer_genpm.h indexer_genpm.cyc
	$(RM) strings_yakker.*
	$(RM) indexer indexer.cyc 1rfc_index 1rfc_index.txt
	$(RM) c_echop echop
	$(RM) wf wf.cyc
	$(RM) http.cyc
	$(RM) abnf-echo abnf-echo.cyc abnf-echo-dfa.cyc
	$(RM) abnf-cyc-cr-eb abnf-cyc-eb abnf-dfa-cr-eb abnf-dfa-eb abnf-dfa.cyc
	$(RM) abnf-dfa.txt abnf-echo-crawl.cyc abnf-echo-dfa.dot abnf-echo-earley-cyc abnf-echo-earley-dfa
	$(RM) abnf-flat-dfa.cyc abnf-flat-dfa.dot abnf-flat-dfa.txt abnf-flat.bnf
	$(RM) aecrawl-cyc.c aecrawl-cyc.cyc
	$(RM) calc-cyc-cr-eb calc-cyc-eb calc-dfa-cr-eb calc-dfa-eb calc-dfa.cyc calc-dfa.dot
	$(RM) calc-dfa.txt calc-flat-dfa.cyc calc-flat-dfa.dot calc-flat-dfa.txt calc.cyc calc.dot calc.txt
	$(RM) imapp imapcl imapp.key imapp.cert
	$(RM) markdown.cyc markdown
	$(RM) $(GENDIR)/*
	$(RM) $(BNFDIR)/*
	$(RM) build/*.d
	$(RM) parseviz.swf

%.p: %.bnf $(PROG_DEP)
	./yakker -pads $< > $@

# .o dependency forces the _a.o to depend on the same files as the .o.
# However, the .o is not really needed.
$(OBJS_PA): %_a.o: %.cyc %.o
	$(CYCLONE) $(CYCFLAGS) -pa -c $< -o $@

%.o: %.cyc
	$(CYCLONE) $(CYCFLAGS) -c $< -o $@

%_tab.cyc: %.y
	$(CYCBISON) -d $<
#	$(CYCBISON) -d --debug --verbose $<

%_tab.h: %.y
	$(CYCBISON) -d $<
#	$(CYCBISON) -d --debug --verbose $<

%.cyc: %.cyl
	$(CYCLEX) $< $@

rfc%.txt:
	curl -O http://www.ietf.org/rfc/$@

# Grab index from ietf
1rfc_index.txt:
	curl -O http://www.ietf.org/iesg/$@

# Strip off beginning of index, start at entry 0001 instead
# Add extra line feed, indexer assumes each entry is followed by two.
1rfc_index: 1rfc_index.txt
	sed -n -e '/0001/,$$ p' $< > $@
	echo >> $@

# Make sure the rfc%.txt files are not automatically deleted by make
.PRECIOUS: rfc%.txt

# literal needs to know about a prefixed length
# capability-data causes an unusual conflict, avoided for now
#	echo 'literal = "{" (number "")$$x "}" CRLF @repeat(atoi(x))CHAR8.' >> $@.tmp
#	echo 'literal = "{" (number$$x ("}" CRLF @repeat(atoi(x))CHAR8)).' >> $@.tmp
#	echo 'literal = "{" number$$x "}" CRLF CHAR8.' >> $@.tmp
$(BNFDIR)/rfc3501.bnf: rfc3501.txt $(PROG_DEP)
	./yakker -extract $< -omit literal capability-data > $@.tmp
	echo 'literal = "{" number$$x "}" CRLF @repeat(atoi(x))CHAR8.' >> $@.tmp
	echo 'capability-data = "CAPABILITY" *(SP capability).' >> $@.tmp
	./yakker $@.tmp -namespace rfc3501 > $@
	$(RM) $@.tmp

# IMAP extension: non-synchronizing literals
# First command line should be: ./yakker -extract $< > $@.tmp,
# but results in empty bnf because defined with ::= instead of =.
$(BNFDIR)/rfc2088.bnf: rfc2088.txt $(PROG_DEP)
	echo "literal = \"{\" number [\"+\"] \"}\" CRLF *CHAR8." > $@.tmp
	./yakker $@.tmp -namespace rfc2088 > $@
	$(RM) $@.tmp

# IMAP extension: Namespace
$(BNFDIR)/rfc2342.bnf: rfc2342.txt $(PROG_DEP)
	./yakker -extract $< > $@.tmp
	./yakker $@.tmp -namespace rfc2342 > $@
	$(RM) $@.tmp

# IMAP extension: MULTIAPPEND
$(BNFDIR)/rfc3502.bnf: rfc3502.txt $(PROG_DEP)
	./yakker -extract $< > $@.tmp
	./yakker $@.tmp -namespace rfc3502 > $@
	$(RM) $@.tmp

# IMAP extension: Binary content
$(BNFDIR)/rfc3516.bnf: rfc3516.txt $(PROG_DEP)
	./yakker -extract $< > $@.tmp
	./yakker $@.tmp -namespace rfc3516 > $@
	$(RM) $@.tmp

# Updated IMAP. Updates 2088, 2342, 3501, 3502, 3516
$(BNFDIR)/rfc4466.bnf: rfc4466.txt $(PROG_DEP)
	./yakker -extract $< > $@.tmp
	./yakker $@.tmp -namespace rfc4466 > $@
	$(RM) $@.tmp


# A1, A2, and passwd are extracted by mistake
# Request-URI is the proper spelling (defined in rfc2616)
# challenge is defined 3 times, the first is correct, we fix by hand
# credentials is defined more than once too
$(BNFDIR)/rfc2617.bnf: rfc2617.txt $(PROG_DEP)
	./yakker -extract $< -omit A1 A2 passwd challenge credentials \
	  -rename request-uri Request-URI >$@
	echo "challenge = auth-scheme 1*SP 1#auth-param." >> $@
	echo "credentials = auth-scheme #auth-param." >> $@

# freshness_lifetime is extracted by mistake
# entity-body gets multiple definitions, we correct by hand
$(BNFDIR)/rfc2616.bnf: rfc2616.txt $(PROG_DEP)
	./yakker -extract $< -omit freshness_lifetime \
	  -omit entity-body >$@
	echo "entity-body = *OCTET." >> $@

# CRLF has multiple (syntactically different) defs, we correct by hand
# rulename has a bunch of examples and the correct def
# Due to an extraneous newline in a comment the defn. of CHAR is screwed
#   Same for char-val and prose-val
# A bunch of things that look like definitions in the file but aren't:
#   b, d, x command, mumble, bar, foo, ruleset
$(BNFDIR)/rfc2234.bnf: rfc2234.txt $(PROG_DEP)
	./yakker -extract $< -omit CRLF rulename CHAR char-val prose-val b d x command mumble bar foo ruleset name >$@
	echo 'CRLF = CR LF.' >> $@
	echo 'rulename = ALPHA *(ALPHA / DIGIT / "-").' >> $@
	echo 'CHAR = %x01-7F.' >> $@
	echo 'char-val = DQUOTE *(%x20-21 / %x23-7E) DQUOTE.' >> $@
	echo 'prose-val = "<" *(%x20-3D / %x3F-7E) ">".' >> $@

# TEMPORARY: CFWS blows up lookahead construction
# TEMPORARY: FWS blows up lookahead construction
$(BNFDIR)/rfc2822.bnf: rfc2822.txt $(PROG_DEP)
	./yakker -extract $< -omit CFWS FWS >$@
	echo 'CFWS = SP.' >> $@
	echo 'FWS = SP.' >> $@
	echo 'dtext = NO-WS-CTL / %d33-90 / %d94-126.' >> $@
	echo 'qtext = NO-WS-CTL / %d33 / %d35-91 / %d93-126.' >> $@
	echo 'ctext = NO-WS-CTL / %d33-39 / %d42-91 / %d93-126.' >> $@
	echo 'item-value = 1*angle-addr / addr-spec / atom / domain / msg-id.' >> $@
	echo 'obs-zone = "UT" / "GMT" / "EST" / "EDT" / "CST" / "CDT" / "MST" / "MDT" / "PST" / "PDT" / %d65-73 / %d75-90 / %d97-105 / %d107-122.' >> $@

$(BNFDIR)/rfc1738.bnf: rfc1738.txt $(PROG_DEP)
	./yakker -extract $< -namespace rfc1738 > $@

$(BNFDIR)/rfc1766.bnf: rfc1766.txt $(PROG_DEP)
	./yakker -extract $< -namespace rfc1766 >$@

$(BNFDIR)/rfc2425.bnf: rfc2425.txt $(PROG_DEP)
	# time-numzome should be time-numzone
	# iana-token gets extracted twice, one defn is wrong
	# genericurl is defined in rfc1738
	# date-time is simply missing, note according to the text it will
	#  be different than the one in rfc3501
	#  We know date-time should be there because it is referred to
	#  by rfc2426, plus examples are given in the text of rfc2425
	./yakker -extract $< -omit time-numzome iana-token -rename genericurl rfc1738:genericurl > $@.tmp
	echo 'iana-token = 1*(ALPHA / DIGIT / "-").' >> $@.tmp
	echo 'time-numzone = sign time-hour [":"] time-minute.' >> $@.tmp
	echo 'date-time = date "T" time.' >> $@.tmp
	./yakker -namespace rfc2425 $@.tmp > $@
	$(RM) $@.tmp


$(BNFDIR)/rfc2426.bnf: rfc2426.txt $(PROG_DEP)
	# value and param have multiple definitions
	# the rename corrects a typo (but it is in a value which we omit...)
	./yakker -extract $< -rename snd-line-value snd-inline-value -omit value param > $@
	# these are the right definitions of value and param
	echo 'value = *VALUE-CHAR.' >> $@
	echo 'param = param-name "=" param-value *("," param-value).' >> $@
	# typos prevent extraction of these
	echo 'ESCAPED-CHAR = "\\" / "\;" / "\," / ("\" %d110) / ("\" %d78).' >> $@
	echo 'date-time-value = rfc2425:date-time.' >> $@
	echo 'snd-inline-param = ("VALUE" "=" "binary") / ("ENCODING" "=" "b") / ("TYPE" "=" *SAFE-CHAR).' >> $@
	# fill in some missing definitions
	echo 'uri = rfc1738:genericurl.' >> $@
	echo 'word = iana-token.' >> $@
	echo 'iana-type = iana-token.' >> $@

$(BNFDIR)/rfc%.bnf: rfc%.txt $(PROG_DEP)
	./yakker -extract $< >$@

parseviz0.swf: Parseviz0.mxml
	mxmlc -output $@ -use-network=false $<
parseviz.swf: Dfa.hx Earley.hx Parseviz.hx
	haxe -debug -swf-version 9 -swf $@ -lib flex $^

.PHONY: cyclone-install

#######################################
## miscellaneous dependencies
#######################################

DEPENDS += build/imap_scanf.d
lex.o: parse_tab.h

ifneq ($(MAKECMDGOALS),clean)
  -include $(DEPENDS)
endif
