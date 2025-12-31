#ifndef COOL_TREE_H
#define COOL_TREE_H

#include "tree.h"
#include "cool-tree.handcode.h"

// 前置声明
class Program_class;
typedef Program_class *Program;
class Class__class;
typedef Class__class *Class_;
class Feature_class;
typedef Feature_class *Feature;
class Formal_class;
typedef Formal_class *Formal;
class Expression_class;
typedef Expression_class *Expression;
class Case_class;
typedef Case_class *Case;

typedef list_node<Class_> Classes_node;
typedef Classes_node *Classes;
typedef list_node<Feature> Features_node;
typedef Features_node *Features;
typedef list_node<Formal> Formals_node;
typedef Formals_node *Formals;
typedef list_node<Expression> Expressions_node;
typedef Expressions_node *Expressions;
typedef list_node<Case> Cases_node;
typedef Cases_node *Cases;

// ============================================================
// 基类定义
// ============================================================

class Program_class : public tree_node {
public:
    program_EXTRAS
    virtual Program copy_Program() = 0;
    tree_node *copy() override { return copy_Program(); }
};

class Class__class : public tree_node {
protected:
    Symbol name, parent, filename;
    Features features;
public:
    Class__class(Symbol a1, Symbol a2, Features a3, Symbol a4) : 
        name(a1), parent(a2), features(a3), filename(a4) {}
    
    class__EXTRAS // 这里已经包含了 get_name, get_parent, get_features 等
    
    virtual Class_ copy_Class_() = 0;
    tree_node *copy() override { return copy_Class_(); }
};

class Feature_class : public tree_node {
public:
    Feature_EXTRAS
    virtual Feature copy_Feature() = 0;
    tree_node *copy() override { return copy_Feature(); }
};

class Formal_class : public tree_node {
public:
    Formal_EXTRAS
    virtual Formal copy_Formal() = 0;
    tree_node *copy() override { return copy_Formal(); }
};

class Expression_class : public tree_node {
public:
    Expression_EXTRAS
    virtual Expression copy_Expression() = 0;
    tree_node *copy() override { return copy_Expression(); }
};

class Case_class : public tree_node {
public:
    Case_EXTRAS
    virtual Case copy_Case() = 0;
    tree_node *copy() override { return copy_Case(); }
};

// ============================================================
// 具体实现类
// ============================================================

class program_class : public Program_class {
protected:
    Classes classes;
public:
    program_class(Classes a1) : classes(a1) {}
    Program copy_Program() override { return new program_class(classes->copy_list()); }
    
    // 实现 tree_node 的纯虚函数 dump
    void dump(ostream& stream, int n) override { stream << pad(n) << "program\n"; classes->dump(stream, n+2); }
    void dump_with_types(ostream&, int) override;
};

class class__class : public Class__class {
public:
    class__class(Symbol a1, Symbol a2, Features a3, Symbol a4) : 
        Class__class(a1, a2, a3, a4) {}
    Class_ copy_Class_() override { 
        return new class__class(name, parent, features->copy_list(), filename); 
    }
    void dump(ostream& stream, int n) override { /* 实现省略 */ }
    void dump_with_types(ostream&, int) override;
};

class method_class : public Feature_class {
protected:
    Symbol name;
    Formals formals;
    Symbol return_type;
    Expression expr;
public:
    method_class(Symbol a1, Formals a2, Symbol a3, Expression a4) :
        name(a1), formals(a2), return_type(a3), expr(a4) {}
    
    method_EXTRAS // 宏内已含 get_name, get_formals 等，不要在外部重写
    
    Feature copy_Feature() override { 
        return new method_class(name, formals->copy_list(), return_type, expr->copy_Expression()); 
    }
    void dump(ostream& stream, int n) override { /* 实现省略 */ }
    void dump_with_types(ostream&, int) override;
};

class attr_class : public Feature_class {
protected:
    Symbol name;
    Symbol type_decl;
    Expression init;
public:
    attr_class(Symbol a1, Symbol a2, Expression a3) :
        name(a1), type_decl(a2), init(a3) {}
    
    attr_EXTRAS
    
    Feature copy_Feature() override {
        return new attr_class(name, type_decl, init->copy_Expression());
    }
    void dump(ostream& stream, int n) override { /* 实现省略 */ }
    void dump_with_types(ostream&, int) override;
};

class formal_class : public Formal_class {
protected:
    Symbol name;
    Symbol type_decl;
public:
    formal_class(Symbol a1, Symbol a2) : name(a1), type_decl(a2) {}
    
    formal_EXTRAS
    
    Formal copy_Formal() override { return new formal_class(name, type_decl); }
    void dump(ostream& stream, int n) override { /* 实现省略 */ }
    void dump_with_types(ostream&, int) override;
};

class branch_class : public Case_class {
protected:
    Symbol name;
    Symbol type_decl;
    Expression expr;
public:
    branch_class(Symbol a1, Symbol a2, Expression a3) :
        name(a1), type_decl(a2), expr(a3) {}
    
    branch_EXTRAS
    
    Case copy_Case() override {
        return new branch_class(name, type_decl, expr->copy_Expression());
    }
    void dump(ostream& stream, int n) override { /* 实现省略 */ }
    void dump_with_types(ostream&, int) override;
};

// ============================================================
// 5. Expression 子类具体实现
// ============================================================

class assign_class : public Expression_class {
protected:
    Symbol name;
    Expression expr;
public:
    assign_class(Symbol a1, Expression a2) : name(a1), expr(a2) {}
    Expression copy_Expression() override {
        return new assign_class(name, expr->copy_Expression());
    }
    void dump(ostream& stream, int n) override {
        stream << pad(n) << "assign\n";
        dump_Symbol(stream, n+2, name);
        expr->dump(stream, n+2);
    }
    void dump_with_types(ostream&, int) override;
};

class static_dispatch_class : public Expression_class {
protected:
    Expression expr;
    Symbol type_name;
    Symbol name;
    Expressions actual;
public:
    static_dispatch_class(Expression a1, Symbol a2, Symbol a3, Expressions a4) :
        expr(a1), type_name(a2), name(a3), actual(a4) {}
    Expression copy_Expression() override {
        return new static_dispatch_class(expr->copy_Expression(), type_name, name, actual->copy_list());
    }
    void dump(ostream& stream, int n) override {
        stream << pad(n) << "static_dispatch\n";
        expr->dump(stream, n+2);
        dump_Symbol(stream, n+2, type_name);
        dump_Symbol(stream, n+2, name);
        actual->dump(stream, n+2);
    }
    void dump_with_types(ostream&, int) override;
};

class dispatch_class : public Expression_class {
protected:
    Expression expr;
    Symbol name;
    Expressions actual;
public:
    dispatch_class(Expression a1, Symbol a2, Expressions a3) :
        expr(a1), name(a2), actual(a3) {}
    Expression copy_Expression() override {
        return new dispatch_class(expr->copy_Expression(), name, actual->copy_list());
    }
    void dump(ostream& stream, int n) override {
        stream << pad(n) << "dispatch\n";
        expr->dump(stream, n+2);
        dump_Symbol(stream, n+2, name);
        actual->dump(stream, n+2);
    }
    void dump_with_types(ostream&, int) override;
};

class cond_class : public Expression_class {
protected:
    Expression pred;
    Expression then_exp;
    Expression else_exp;
public:
    cond_class(Expression a1, Expression a2, Expression a3) :
        pred(a1), then_exp(a2), else_exp(a3) {}
    Expression copy_Expression() override {
        return new cond_class(pred->copy_Expression(), then_exp->copy_Expression(), else_exp->copy_Expression());
    }
    void dump(ostream& stream, int n) override {
        stream << pad(n) << "cond\n";
        pred->dump(stream, n+2);
        then_exp->dump(stream, n+2);
        else_exp->dump(stream, n+2);
    }
    void dump_with_types(ostream&, int) override;
};

class loop_class : public Expression_class {
protected:
    Expression pred;
    Expression body;
public:
    loop_class(Expression a1, Expression a2) : pred(a1), body(a2) {}
    Expression copy_Expression() override {
        return new loop_class(pred->copy_Expression(), body->copy_Expression());
    }
    void dump(ostream& stream, int n) override {
        stream << pad(n) << "loop\n";
        pred->dump(stream, n+2);
        body->dump(stream, n+2);
    }
    void dump_with_types(ostream&, int) override;
};

class typcase_class : public Expression_class {
protected:
    Expression expr;
    Cases cases;
public:
    typcase_class(Expression a1, Cases a2) : expr(a1), cases(a2) {}
    Expression copy_Expression() override {
        return new typcase_class(expr->copy_Expression(), cases->copy_list());
    }
    void dump(ostream& stream, int n) override {
        stream << pad(n) << "typcase\n";
        expr->dump(stream, n+2);
        cases->dump(stream, n+2);
    }
    void dump_with_types(ostream&, int) override;
};

class block_class : public Expression_class {
protected:
    Expressions body;
public:
    block_class(Expressions a1) : body(a1) {}
    Expression copy_Expression() override {
        return new block_class(body->copy_list());
    }
    void dump(ostream& stream, int n) override {
        stream << pad(n) << "block\n";
        body->dump(stream, n+2);
    }
    void dump_with_types(ostream&, int) override;
};

class let_class : public Expression_class {
protected:
    Symbol identifier;
    Symbol type_decl;
    Expression init;
    Expression body;
public:
    let_class(Symbol a1, Symbol a2, Expression a3, Expression a4) :
        identifier(a1), type_decl(a2), init(a3), body(a4) {}
    Expression copy_Expression() override {
        return new let_class(identifier, type_decl, init->copy_Expression(), body->copy_Expression());
    }
    void dump(ostream& stream, int n) override {
        stream << pad(n) << "let\n";
        dump_Symbol(stream, n+2, identifier);
        dump_Symbol(stream, n+2, type_decl);
        init->dump(stream, n+2);
        body->dump(stream, n+2);
    }
    void dump_with_types(ostream&, int) override;
};

// ============================================================
// 算术与比较运算 (逻辑相同，仅展示典型实现)
// ============================================================

#define ARITH_CLASS_DEF(name_str, class_name) \
class class_name : public Expression_class { \
protected: \
    Expression e1, e2; \
public: \
    class_name(Expression a1, Expression a2) : e1(a1), e2(a2) {} \
    Expression copy_Expression() override { \
        return new class_name(e1->copy_Expression(), e2->copy_Expression()); \
    } \
    void dump(ostream& stream, int n) override { \
        stream << pad(n) << name_str << "\n"; \
        e1->dump(stream, n+2); e2->dump(stream, n+2); \
    } \
    void dump_with_types(ostream&, int) override; \
};

ARITH_CLASS_DEF("plus", plus_class)
ARITH_CLASS_DEF("sub", sub_class)
ARITH_CLASS_DEF("mul", mul_class)
ARITH_CLASS_DEF("divide", divide_class)
ARITH_CLASS_DEF("lt", lt_class)
ARITH_CLASS_DEF("eq", eq_class)
ARITH_CLASS_DEF("leq", leq_class)

// ============================================================
// 常量与原子表达式
// ============================================================

class int_const_class : public Expression_class {
protected:
    Symbol token;
public:
    int_const_class(Symbol a1) : token(a1) {}
    Expression copy_Expression() override { return new int_const_class(token); }
    void dump(ostream& stream, int n) override {
        stream << pad(n) << "int_const\n";
        dump_Symbol(stream, n+2, token);
    }
    void dump_with_types(ostream&, int) override;
};

class bool_const_class : public Expression_class {
protected:
    Boolean val;
public:
    bool_const_class(Boolean a1) : val(a1) {}
    Expression copy_Expression() override { return new bool_const_class(val); }
    void dump(ostream& stream, int n) override {
        stream << pad(n) << "bool_const\n";
        dump_Boolean(stream, n+2, val);
    }
    void dump_with_types(ostream&, int) override;
};

class string_const_class : public Expression_class {
protected:
    Symbol token;
public:
    string_const_class(Symbol a1) : token(a1) {}
    Expression copy_Expression() override { return new string_const_class(token); }
    void dump(ostream& stream, int n) override {
        stream << pad(n) << "string_const\n";
        dump_Symbol(stream, n+2, token);
    }
    void dump_with_types(ostream&, int) override;
};

class new__class : public Expression_class {
protected:
    Symbol type_name;
public:
    new__class(Symbol a1) : type_name(a1) {}
    Expression copy_Expression() override { return new new__class(type_name); }
    void dump(ostream& stream, int n) override {
        stream << pad(n) << "new\n";
        dump_Symbol(stream, n+2, type_name);
    }
    void dump_with_types(ostream&, int) override;
};

class isvoid_class : public Expression_class {
protected:
    Expression e1;
public:
    isvoid_class(Expression a1) : e1(a1) {}
    Expression copy_Expression() override { return new isvoid_class(e1->copy_Expression()); }
    void dump(ostream& stream, int n) override {
        stream << pad(n) << "isvoid\n";
        e1->dump(stream, n+2);
    }
    void dump_with_types(ostream&, int) override;
};

class no_expr_class : public Expression_class {
public:
    no_expr_class() {}
    Expression copy_Expression() override { return new no_expr_class(); }
    void dump(ostream& stream, int n) override { stream << pad(n) << "no_expr\n"; }
    void dump_with_types(ostream&, int) override;
};

class object_class : public Expression_class {
protected:
    Symbol name;
public:
    object_class(Symbol a1) : name(a1) {}
    Expression copy_Expression() override { return new object_class(name); }
    void dump(ostream& stream, int n) override {
        stream << pad(n) << "object\n";
        dump_Symbol(stream, n+2, name);
    }
    void dump_with_types(ostream&, int) override;
};

class neg_class : public Expression_class {
protected:
    Expression e1;
public:
    neg_class(Expression a1) : e1(a1) {}
    Expression copy_Expression() override { return new neg_class(e1->copy_Expression()); }
    void dump(ostream& stream, int n) override {
        stream << pad(n) << "neg\n";
        e1->dump(stream, n+2);
    }
    void dump_with_types(ostream&, int) override;
};

class comp_class : public Expression_class {
protected:
    Expression e1;
public:
    comp_class(Expression a1) : e1(a1) {}
    Expression copy_Expression() override { return new comp_class(e1->copy_Expression()); }
    void dump(ostream& stream, int n) override {
        stream << pad(n) << "comp\n";
        e1->dump(stream, n+2);
    }
    void dump_with_types(ostream&, int) override;
};

#endif
