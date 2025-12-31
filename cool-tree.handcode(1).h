#ifndef COOL_TREE_HANDCODE_H
#define COOL_TREE_HANDCODE_H

#include <iostream>
#include "tree.h"
#include "cool.h"
#include "stringtab.h"

#define yylineno curr_lineno;
extern int yylineno;

class Environment;

#define Program_EXTRAS                          \
virtual void cgen(ostream&) = 0;                \
virtual void dump_with_types(ostream&,int) = 0; 

#define program_EXTRAS                          \
void cgen(ostream&);     			            \
void dump_with_types(ostream&,int);            

#define Class__EXTRAS                           \
virtual Symbol get_name() = 0;      	        \
virtual Symbol get_parent() = 0;    	        \
virtual Symbol get_filename() = 0;              \
virtual Features get_features() = 0;            \
virtual void dump_with_types(ostream&,int) = 0; 

#define class__EXTRAS                           \
Symbol get_name()   { return name; }		    \
Symbol get_parent() { return parent; }     	    \
Symbol get_filename() { return filename; }      \
Features get_features() { return features; }    \
void dump_with_types(ostream&,int);                    

// 注意：这里不再定义 IsMethod，交给子类实现
#define Feature_EXTRAS                          \
virtual void dump_with_types(ostream&,int) = 0; \
virtual bool IsMethod() = 0;

#define Feature_SHARED_EXTRAS                   \
void dump_with_types(ostream&,int);    

#define method_EXTRAS                           \
    Symbol get_name() { return name; }          \
    Formals get_formals() { return formals; }   \
    Symbol get_return_type() { return return_type; } \
    Expression get_expr() { return expr; }      \
    bool IsMethod() { return true; }

#define attr_EXTRAS                             \
    Symbol get_name() { return name; }          \
    Symbol get_type_decl() { return type_decl; }\
    Expression get_init() { return init; }      \
    bool IsMethod() { return false; }

#define Formal_EXTRAS                           \
virtual Symbol get_name() = 0;                  \
virtual void dump_with_types(ostream&,int) = 0;

#define formal_EXTRAS                           \
Symbol get_name() { return name; }              \
void dump_with_types(ostream&,int);

#define Case_EXTRAS                             \
virtual Symbol get_type_decl() = 0;             \
virtual void dump_with_types(ostream& ,int) = 0;

#define branch_EXTRAS                           \
Symbol get_name() { return name; }              \
Symbol get_type_decl() { return type_decl; }    \
Expression get_expr() { return expr; }          \
void dump_with_types(ostream& ,int);

#define Expression_EXTRAS                    \
Symbol type;                                 \
Symbol get_type() { return type; }           \
Expression set_type(Symbol s) { type = s; return this; } \
virtual void code(ostream&, Environment&) = 0; \
virtual void dump_with_types(ostream&,int) = 0;  \
void dump_type(ostream&, int);               \
Expression_class() { type = (Symbol) NULL; }

#define Expression_SHARED_EXTRAS           \
void code(ostream&, Environment&);         \
void dump_with_types(ostream&,int); 

#endif
