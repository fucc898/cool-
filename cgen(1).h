#ifndef CGEN_H
#define CGEN_H

#include <assert.h>
#include <stdio.h>
#include "cool-tree.h"
#include "emit.h"
#include "symtab.h"
#include "glist.h"

// 在 cgen.h 的 #include 之后添加
enum Basicness { Basic, NotBasic };

// 声明全局调试开关
extern int cgen_debug;

// 声明 COOL 基础类符号 (这些通常在 stringtab 中定义，但这里需要显式声明)
extern Symbol Int, Bool, Str, Object, No_class;

class CgenClassTable;
typedef CgenClassTable *CgenClassTableP;

class CgenNode;
typedef CgenNode *CgenNodeP;

// Environment 类用于在表达式求值时追踪变量偏移
class Environment {
private:
    CgenNodeP m_class_node;
    std::vector<Symbol> m_var_idx_tab;   // Let 变量 (栈)
    std::vector<Symbol> m_param_idx_tab; // 方法参数

public:
    Environment(CgenNodeP cn) : m_class_node(cn) {}
    
    CgenNodeP GetClassNode() { return m_class_node; }
    
    // 变量查找逻辑
    int LookUpVar(Symbol sym);
    int LookUpParam(Symbol sym);
    int LookUpAttrib(Symbol sym);
    
    // 作用域管理
    void AddVar(Symbol sym) { m_var_idx_tab.push_back(sym); }
    void RemoveVar() { m_var_idx_tab.pop_back(); }
    void SetParams(std::vector<Symbol> params) { m_param_idx_tab = params; }
    int GetNumParams() { return m_param_idx_tab.size(); }
};

class CgenClassTable : public SymbolTable<Symbol, CgenNode> {
private:
    std::vector<CgenNodeP> m_class_nodes;
    ostream& str;
    int tag_counter;

    // 内部辅助流程
    void setup_basic_classes();
    void install_basic_classes();
    void install_class(CgenNodeP nd);
    void install_classes(Classes cs);
    void build_inheritance_tree();
    void set_relations(CgenNodeP nd);

public:
    CgenClassTable(Classes, ostream& str);
    
    // 代码生成核心步骤
    void code();
    void code_global_data();
    void code_constants();
    void code_class_nameTab();
    void code_class_objTab();
    void code_dispatchTabs();
    void code_protObjs();
    void code_global_text();
    void code_class_inits();
    void code_class_methods();

    // 工具函数
    CgenNodeP GetNode(Symbol name);
    CgenNodeP root();
};

class CgenNode : public Class__class {
private: 
    CgenNodeP parentnd;            // 父节点指针
    std::vector<CgenNodeP> children; // 子节点
    Basicness basic_status;        // 是否为基础类
    int tag;                       // 类的唯一编号
    int max_child_tag;             // 子树中最大的 tag（用于 Case 匹配）
    
    std::vector<attr_class*> m_full_attribs;   // 包含继承的所有属性
    std::vector<method_class*> m_full_methods; // 包含继承的所有方法

public:
    CgenNode(Class_ c, Basicness bstatus, CgenClassTableP class_table);

    // 必须实现 cool-tree.h 中的虚接口
    Symbol get_name() override { return name; }
    Symbol get_parent() override { return parent; }
    Features get_features() override { return features; }
    Symbol get_filename() override { return filename; }
    
    void dump(ostream& s, int n) override { }
    void dump_with_types(ostream& s, int n) override { }

    // 继承树构建
    void add_child(CgenNodeP child) { children.push_back(child); }
    std::vector<CgenNodeP> get_children() { return children; }
    void set_parentnd(CgenNodeP p) { parentnd = p; }
    CgenNodeP get_parentnd() { return parentnd; }

    // 布局逻辑 (核心)
    void LayoutFeatures();
    int GetTag() { return tag; }
    int GetMaxChildTag() { return max_child_tag; }
    void SetTag(int t) { tag = t; }
    void SetMaxChildTag(int t) { max_child_tag = t; }

    // 偏移量查找
    int GetAttrOffset(Symbol name);
    int GetMethodOffset(Symbol name);
    
    std::vector<attr_class*>& GetFullAttribs() { return m_full_attribs; }
    std::vector<method_class*>& GetFullMethods() { return m_full_methods; }
};

// 全局单例引用
extern CgenClassTableP codegen_classtable;

#endif
