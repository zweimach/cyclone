/* Type checking for patterns.
   Copyright (C) 2001 Greg Morrisett
   This file is part of the Cyclone compiler.

   The Cyclone compiler is free software; you can redistribute it
   and/or modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   The Cyclone compiler is distributed in the hope that it will be
   useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the Cyclone compiler; see the file COPYING. If not,
   write to the Free Software Foundation, Inc., 59 Temple Place -
   Suite 330, Boston, MA 02111-1307, USA. */
#ifndef _TCPAT_H_
#define _TCPAT_H_

#include "tcenv.h"

namespace Tcpat {
using List;
using Absyn;
using Tcenv;

struct TcPatResult {
  $(list_t<$(tvar_t,bool)@>,list_t<effconstr_t>) * tvars_and_effconstr_opt;  
  //the effects result from unpacked existentials, or in the case of let alias
  //the newly introduced effect var is added to the capability
  //$(list_t<$(tvar_t,bool)@>,list_t<$(type_t,type_t)@>) * tvars_and_bounds_opt; //REMOVE THIS!
  // tvars_and_bounds_opt is a list of tvars and outlives constraints
  // to add to the environment of the statement in the scope of the
  // pattern.  These can arise either from an unpacked existential or
  // an alias pattern.  If the bool for a tvar is true, then this
  // is a newly-introduced region from an alias pattern.
  list_t<$(vardecl_t *,exp_opt_t)@> patvars;
  // patvars is a list of pattern variables and expressions, with
  // the following meaning: for each list element $(v,e):
  // 1) if v is not null, and e is not null, then e is the
  //    expression that corresponds to v in the matched expression
  // 2) if v is not null, and e is null, then v has no corresponding
  //    expression, and should be assumed initialized
  // 3) if v is null, then e is not null, and indicates an expression
  //    that should be "used:" (consumed) by the flow analysis.  These
  //    will correspond to the locations of datatypes in patterns, and
  //    thus effectively require that the datatype be fully initialized.
  list_t<$(type_t,type_t)@> aquals_bounds;
  //for existential unpacks, we need to add aqual(`a) <= ALIASABLE etc. to the env.
};

typedef struct TcPatResult tcpat_result_t;
  // You must call tcPat, then unify with the type of the value on which
  // you're switching, then call check_pat_regions.
  // If someone has a less clumsy proposal, I'd love to hear it.
tcpat_result_t tcPat(tenv_t, pat_t, type_t @ topt, exp_opt_t pat_var_exp);
void check_pat_regions(tenv_t,pat_t, tcpat_result_t@);


extern datatype PatTest {
  WhereTest(exp_opt_t);
  EqNull;
  NeqNull;
  EqEnum(enumdecl_t, enumfield_t);
  EqAnonEnum(type_t, enumfield_t);
  EqFloat(string_t,int);
  EqConst(unsigned int);
  EqDatatypeTag(int, datatypedecl_t, datatypefield_t);
  EqTaggedUnion(type_t,field_name_t,int);
  EqExtensibleDatatype(datatypedecl_t, datatypefield_t);
};
typedef datatype PatTest@ pat_test_t;
extern datatype Access {
  Dummy;  // used to deal with the dummy tuple we create for handling where clauses
  Deref(type_t res_type);
  DatatypeField(datatypedecl_t, datatypefield_t, unsigned, type_t res_type);
  AggrField(type_t aggrtype, bool tagged, stringptr_t, type_t res_type);
};
typedef datatype Access@ access_t;

@tagged union PatOrWhere {
  pat_t     pattern;
  exp_opt_t where_clause;
};
struct PathNode {
  union PatOrWhere orig_pat;
  access_t  access;
};
typedef struct PathNode@ path_node_t;
extern datatype Term_desc;
typedef datatype Term_desc @ term_desc_t;
typedef list_t<path_node_t> path_t;

struct Rhs {
  bool used;
  seg_t pat_loc;
  stmt_t rhs;
};
typedef struct Rhs @rhs_t;

typedef datatype Decision @decision_t;
extern datatype Decision {
  Failure(term_desc_t);
  Success(rhs_t);
  SwitchDec(path_t, list_t<$(pat_test_t, decision_t)@>, decision_t);
};

void check_switch_exhaustive(seg_t,tenv_t,list_t<switch_clause_t>,
			     decision_opt_t@);
bool check_let_pat_exhaustive(seg_t,tenv_t,pat_t,
			      decision_opt_t@); // true => exhaustive
void check_catch_overlap(seg_t,tenv_t,list_t<switch_clause_t>,decision_opt_t@);
void print_decision_tree(decision_t);

bool has_vars(Core::opt_t<list_t<$(vardecl_t*,exp_opt_t)@>> pat_vars);
}

#endif
