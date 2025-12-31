#include "cgen.h"
#include "cgen_gc.h"
#include <algorithm> // 必须包含这个才能用 std::sort
#include "cgen_gc.h" // 如果有垃圾回收相关的定义

// 补全缺失的汇编辅助宏
#define emit_label_ref(l, s) { s << "label" << l; }
#define emit_label_def(l, s) { s << "label" << l << ": "; }
#define emit_return(s)       { s << RET << endl; }

// 修正之前的 emit_branch 等宏，确保调用了 emit_label_ref
#undef emit_branch
#define emit_branch(label, s) { s << "\tb\t"; emit_label_ref(label, s); s << endl; }

static int label_count = 0;
CgenClassTable* codegen_classtable = nullptr; // 解决 typcase 报错

// 补全 emit 工具宏
#define emit_branch(label, s) { s << "\tb\t"; emit_label_ref(label, s); s << endl; }
#define emit_blti(reg, imm, label, s) { s << "\tblt\t" << reg << " " << imm << " "; emit_label_ref(label, s); s << endl; }
#define emit_bgti(reg, imm, label, s) { s << "\tbgt\t" << reg << " " << imm << " "; emit_label_ref(label, s); s << endl; }

// 解决 emit_add 报错（有些环境叫 emit_addu）
inline void emit_add(char *dest, char *src1, char *src2, ostream& s) {
    s << "\tadd\t" << dest << " " << src1 << " " << src2 << endl;
}

extern int curr_lineno;

// 显式声明 emit 函数，强行解决 "not declared in this scope" 错误
// 这些函数定义在原生的 emit.h/cgen.cc 工具库中
extern "C" {
    void emit_load(char *dest_reg, int offset, char *source_reg, ostream& s);
    void emit_store(char *source_reg, int offset, char *dest_reg, ostream& s);
    void emit_addiu(char *dest_reg, char *src_reg, int imm, ostream& s);
    void emit_jal(char *label, ostream& s);
    void emit_move(char *dest_reg, char *src_reg, ostream& s);
    void emit_push(char *reg, ostream& s);
    void emit_label_def(int label_num, ostream& s);
    void emit_bne(char *reg1, char *reg2, int label_num, ostream& s);
    void emit_beq(char *reg1, char *reg2, int label_num, ostream& s);
    void emit_load_address(char *dest_reg, char *address, ostream& s);
    void emit_load_imm(char *dest_reg, int imm, ostream& s);
    void emit_jalr(char *reg, ostream& s);
    void emit_addu(char *dest, char *src1, char *src2, ostream& s);
    void emit_sll(char *dest, char *src1, int num, ostream& s);
}

//**************************************************************
// Environment 成员函数实现
//**************************************************************

int Environment::LookUpVar(Symbol sym) {
    // 从后往前搜，确保覆盖（Let 作用域）
    for (int i = m_var_idx_tab.size() - 1; i >= 0; i--) {
        if (m_var_idx_tab[i] == sym) return i;
    }
    return -1;
}

int Environment::LookUpParam(Symbol sym) {
    for (int i = 0; i < (int)m_param_idx_tab.size(); i++) {
        if (m_param_idx_tab[i] == sym) return i;
    }
    return -1;
}

int Environment::LookUpAttrib(Symbol sym) {
    auto& attribs = m_class_node->GetFullAttribs();
    for (int i = 0; i < (int)attribs.size(); i++) {
        if (attribs[i]->get_name() == sym) return i;
    }
    return -1;
}

//**************************************************************
// CgenNode 成员函数实现
//**************************************************************

CgenNode::CgenNode(Class_ c, Basicness bstatus, CgenClassTableP class_table) :
   class__class((class__class &) *c),
   parentnd(NULL),
   children(NULL),
   basic_status(bstatus)
{ 
    // 这里的 filename 等成员由 class__class 继承而来
}

// 递归布局属性和方法（处理继承的关键）
void CgenNode::LayoutFeatures() {
    if (parentnd && parentnd->get_name() != idtable.add_string("No_class")) {
        // 继承父类的属性和方法布局
        m_full_attribs = parentnd->GetFullAttribs();
        // 实际开发中还需要处理方法的覆盖逻辑，此处简化
    }

    Features features = get_features();
    for (int i = features->first(); features->more(i); i = features->next(i)) {
        Feature f = features->nth(i);
        if (f->IsMethod()) {
            // 方法处理逻辑
        } else {
            // 属性处理逻辑
            m_full_attribs.push_back((attr_class*)f);
        }
    }
}

//**************************************************************
// CgenClassTable 核心逻辑
//**************************************************************

CgenClassTable::CgenClassTable(Classes cs, ostream& s) : nds(NULL) , str(s) {
    // 1. 安装基础类
    install_basic_classes();
    // 2. 安装用户类
    install_classes(cs);
    // 3. 建立继承树
    build_inheritance_tree();
    // 4. 生成代码
    code();
}

void CgenClassTable::install_basic_classes() {
    // 必须确保这些类在 tag 0-4
    // 逻辑省略，通常由框架预置
}

void CgenClassTable::install_classes(Classes cs) {
    for (int i = cs->first(); cs->more(i); i = cs->next(i)) {
        install_class(new CgenNode(cs->nth(i), NotBasic, this));
    }
}

void CgenClassTable::code() {
    if (cgen_debug) cout << "Coding global data" << endl;
    code_global_data();

    if (cgen_debug) cout << "Coding constants" << endl;
    code_constants();

    if (cgen_debug) cout << "Coding class tables" << endl;
    code_class_nameTab();
    code_class_objTab();
    code_dispatchTabs();
    code_protObjs();

    if (cgen_debug) cout << "Coding global text" << endl;
    code_global_text();
    code_class_inits();
    code_class_methods();
}

//**************************************************************
// 表达式代码生成 (Expression::code)
//**************************************************************

void assign_class::code(ostream &s, Environment &env) {
    expr->code(s, env);
    int idx;
    
    // 优先级：局部变量 > 参数 > 属性
    if ((idx = env.LookUpVar(name)) != -1) {
        emit_store(ACC, idx + 1, SP, s);
    } 
    else if ((idx = env.LookUpParam(name)) != -1) {
        int num_params = env.GetNumParams();
        emit_store(ACC, (num_params - idx) + 2, FP, s);
    } 
    else if ((idx = env.LookUpAttrib(name)) != -1) {
        emit_store(ACC, idx + 3, SELF, s);
    }
}

void static_dispatch_class::code(ostream &s, Environment &env) {
    // 参数入栈
    for (int i = actual->first(); actual->more(i); i = actual->next(i)) {
        actual->nth(i)->code(s, env);
        emit_push(ACC, s);
    }
    
    expr->code(s, env);
    
    int non_void_label = 100; // 临时硬编码
    emit_bne(ACC, ZERO, non_void_label, s);
    
    // Abort on void
    emit_load_address(ACC, "str_const0", s);
    emit_load_imm(T1, get_line_number(), s);
    emit_jal("_dispatch_abort", s);
    
    emit_label_def(non_void_label, s);
    
    // 静态绑定：直接通过类名构造分发表地址
    std::string target = std::string(type_name->get_string()) + "_dispatch_tab";
    emit_load_address(T1, (char*)target.c_str(), s);
    
    // 此处需要通过静态类型找到方法偏移，暂定 0
    emit_load(T1, 0, T1, s);
    emit_jalr(T1, s);
}

//**************************************************************
// 表达式代码生成 (续)
//**************************************************************

void dispatch_class::code(ostream &s, Environment &env) {
    // 1. 将参数按顺序求值并压栈
    for (int i = actual->first(); actual->more(i); i = actual->next(i)) {
        actual->nth(i)->code(s, env);
        emit_push(ACC, s);
    }

    // 2. 求值对象表达式（dispatch 目标）
    expr->code(s, env);

    // 3. 检查对象是否为 Void
    int non_void_label = ++label_count;
    emit_bne(ACC, ZERO, non_void_label, s);

    // 如果为 Void，调用 runtime 错误处理
    // 注意：filename 成员来自 Class__class 接口
    Symbol filename = env.GetClassNode()->get_filename();
    std::string file_str = filename->get_string();
    emit_load_address(ACC, (char*)file_str.c_str(), s);
    emit_load_imm(T1, get_line_number(), s);
    emit_jal("_dispatch_abort", s);

    emit_label_def(non_void_label, s);

    // 4. 从对象的 dispatch table 中查找方法偏移
    // 对象的 dispatch table 指针在 header 的偏移 2 位置
    emit_load(T1, 2, ACC, s);

    // 通过方法名查找在分发表中的偏移 (具体逻辑需在 CgenNode 中预存)
    int method_offset = env.GetClassNode()->GetMethodOffset(name);
    emit_load(T1, method_offset, T1, s);
    emit_jalr(T1, s);
}

void cond_class::code(ostream &s, Environment &env) {
    int else_label = ++label_count;
    int finish_label = ++label_count;

    // 1. 计算谓词
    pred->code(s, env);
    
    // 2. 提取 Bool 对象中的原始布尔值 (偏移 3)
    emit_load(T1, 3, ACC, s);
    emit_beq(T1, ZERO, else_label, s);

    // 3. Then 分支
    then_exp->code(s, env);
    emit_branch(finish_label, s);

    // 4. Else 分支
    emit_label_def(else_label, s);
    else_exp->code(s, env);

    // 5. 结束
    emit_label_def(finish_label, s);
}

void loop_class::code(ostream &s, Environment &env) {
    int start_label = ++label_count;
    int finish_label = ++label_count;

    emit_label_def(start_label, s);

    // 谓词求值
    pred->code(s, env);
    emit_load(T1, 3, ACC, s);
    emit_beq(T1, ZERO, finish_label, s);

    // 循环体
    body->code(s, env);
    emit_branch(start_label, s);

    emit_label_def(finish_label, s);
    
    // Loop 总是返回 Void (即 $a0 置 0)
    emit_move(ACC, ZERO, s);
}

void plus_class::code(ostream &s, Environment &env) {
    e1->code(s, env);
    emit_push(ACC, s);
    e2->code(s, env);
    
    // 结果需要是一个新对象 (Object.copy)
    emit_jal("Object.copy", s);
    
    emit_load(T1, 1, SP, s);    // T1 = e1 对象地址
    emit_load(T2, 3, T1, s);    // T2 = e1 的原始整数值
    emit_load(T3, 3, ACC, s);   // T3 = e2 的原始整数值
    emit_add(T2, T2, T3, s);    // 这里的 emit_add 需要你确保在 extern "C" 里声明
    
    emit_store(T2, 3, ACC, s);  // 更新 ACC 对象的整数值位
    emit_addiu(SP, SP, 4, s);   // 弹出栈
}

void isvoid_class::code(ostream &s, Environment &env) {
    // 修正：手写的 cool-tree.h 中 isvoid 的成员通常是 e1
    e1->code(s, env);
    emit_move(T1, ACC, s);
    
    // 加载 Bool 常量
    // 这里依赖于你在 code_constants 中生成的全局 bool 标签
    emit_load_address(ACC, "bool_const1", s); // 假设 true 为 const1
    int true_label = ++label_count;
    emit_beq(T1, ZERO, true_label, s);
    emit_load_address(ACC, "bool_const0", s); // 否则加载 false
    
    emit_label_def(true_label, s);
}

void new__class::code(ostream &s, Environment &env) {
    if (type_name == idtable.add_string("SELF_TYPE")) {
        // 动态创建 SELF_TYPE
        emit_load(T1, 0, SELF, s);           // 取出 class tag
        emit_load_address(T2, "class_objTab", s);
        emit_sll(T1, T1, 3, s);              // tag * 8 (每个条目 2 个 word)
        emit_addu(T1, T1, T2, s);
        
        emit_push(T1, s);                    // 保存 objTab 条目地址
        emit_load(ACC, 0, T1, s);            // 加载 ProtObj
        emit_jal("Object.copy", s);
        
        emit_load(T1, 1, SP, s);
        emit_load(T1, 1, T1, s);             // 加载 Init 函数地址
        emit_addiu(SP, SP, 4, s);
        emit_jalr(T1, s);
    } else {
        // 静态创建已知类
        std::string prot_obj = std::string(type_name->get_string()) + "_protObj";
        std::string init_func = std::string(type_name->get_string()) + "_init";
        
        emit_load_address(ACC, (char*)prot_obj.c_str(), s);
        emit_jal("Object.copy", s);
        emit_jal((char*)init_func.c_str(), s);
    }
}

//**************************************************************
// Case 语句 (typcase) 的实现 - 核心是寻找最深子类匹配
//**************************************************************

void typcase_class::code(ostream &s, Environment &env) {
    expr->code(s, env);
    int non_void_label = ++label_count;
    emit_bne(ACC, ZERO, non_void_label, s);

    // 错误处理：对 Void 进行 Case
    Symbol filename = env.GetClassNode()->get_filename();
    emit_load_address(ACC, (char*)filename->get_string(), s);
    emit_load_imm(T1, get_line_number(), s);
    emit_jal("_case_abort2", s);

    emit_label_def(non_void_label, s);
    emit_load(T1, 0, ACC, s); // T1 = 运行时对象的 class tag

    int finish_label = ++label_count;
    
    // 逻辑：Case 分支必须从最深（子类）到最浅（父类）排序
    // 这样第一个满足 tag 范围的分支就是最匹配的
    
    // 将 Case 分支转换为可排序的辅助结构
    struct BranchInfo {
        Case node;
        int tag;
        int max_tag;
    };
    std::vector<BranchInfo> sorted_branches;

    for (int i = cases->first(); cases->more(i); i = cases->next(i)) {
        branch_class* b = (branch_class*)cases->nth(i);
        CgenNodeP node = codegen_classtable->GetNode(b->get_type_decl());
        sorted_branches.push_back({b, node->get_tag(), node->get_max_child_tag()});
    }

    // 按 tag 降序排列 (子类 tag 通常大于父类)
    std::sort(sorted_branches.begin(), sorted_branches.end(), 
              [](const BranchInfo& a, const BranchInfo& b) { return a.tag > b.tag; });

    for (auto& b : sorted_branches) {
        int next_branch = ++label_count;
        // 检查 T1 是否在 [b.tag, b.max_tag] 范围内
        emit_blti(T1, b.tag, next_branch, s);
        emit_bgti(T1, b.max_tag, next_branch, s);

        // 匹配成功：进入该分支
        emit_push(ACC, s); // 绑定变量到栈
        env.AddVar(b.node->get_name());
        b.node->get_expr()->code(s, env);
        emit_addiu(SP, SP, 4, s); // 弹出变量
        emit_branch(finish_label, s);

        emit_label_def(next_branch, s);
    }

    // 若无分支匹配，运行时报错
    emit_jal("_case_abort", s);
    emit_label_def(finish_label, s);
}

//**************************************************************
// CgenClassTable 汇编输出实现
//**************************************************************

void CgenClassTable::code_dispatchTabs() {
    for (auto& node : m_class_nodes) {
        str << node->get_name() << DISPTAB_SUFFIX << LABEL;
        
        auto& methods = node->GetFullMethods();
        for (auto& m : methods) {
            str << WORD << m->get_container_name() << "." << m->get_name() << endl;
        }
    }
}

void CgenClassTable::code_protObjs() {
    for (auto& node : m_class_nodes) {
        // 1. 符号与标签
        str << WORD << "-1" << endl; // GC Tag
        str << node->get_name() << PROTOBJ_SUFFIX << LABEL;
        
        // 2. Class Tag
        str << WORD << node->get_tag() << endl;
        
        // 3. Size (Header 3 words + attributes)
        auto& attribs = node->GetFullAttribs();
        str << WORD << (DEFAULT_OBJFIELDS + attribs.size()) << endl;
        
        // 4. Dispatch Table Address
        str << WORD << node->get_name() << DISPTAB_SUFFIX << endl;
        
        // 5. Attributes Initial Values
        for (auto& attr : attribs) {
            Symbol type = attr->get_type_decl();
            if (type == Int) { inttable.lookup_string("0")->code_ref(str); }
            else if (type == Bool) { falsebool.code_ref(str); }
            else if (type == Str) { stringtable.lookup_string("")->code_ref(str); }
            else { str << WORD << "0" << endl; } // Void
            str << endl;
        }
    }
}

//**************************************************************
// 模块收尾：初始化方法
//**************************************************************

void CgenClassTable::code_class_inits() {
    for (auto& node : m_class_nodes) {
        str << node->get_name() << CLASSINIT_SUFFIX << LABEL;
        
        // 标准帧头
        emit_addiu(SP, SP, -12, str);
        emit_store(FP, 3, SP, str);
        emit_store(SELF, 2, SP, str);
        emit_store(RA, 1, SP, str);
        emit_addiu(FP, SP, 4, str);
        emit_move(SELF, ACC, str);

        // 调用父类 Init (Object 无父类)
        if (node->get_name() != Object) {
            std::string parent_init = std::string(node->get_parent()->get_string()) + CLASSINIT_SUFFIX;
            emit_jal((char*)parent_init.c_str(), str);
        }

        // 初始化当前类的属性
        auto& attribs = node->get_features(); // 只取当前类的特征
        Environment env(node);
        for (int i = attribs->first(); attribs->more(i); i = attribs->next(i)) {
            Feature f = attribs->nth(i);
            if (!f->IsMethod()) {
                attr_class* a = (attr_class*)f;
                if (a->get_init()->get_type() != NULL) { // 有初始化表达式
                    a->get_init()->code(str, env);
                    int offset = node->GetAttrOffset(a->get_name());
                    emit_store(ACC, offset, SELF, str);
                }
            }
        }

        // 还原现场
        emit_move(ACC, SELF, str);
        emit_load(FP, 3, SP, str);
        emit_load(SELF, 2, SP, str);
        emit_load(RA, 1, SP, str);
        emit_addiu(SP, SP, 12, str);
        emit_return(str);
    }
}
