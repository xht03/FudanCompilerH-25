#define DEBUG
#undef DEBUG

#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include "namemaps.hh"
#include "semant.hh"

using namespace std;
using namespace fdmj;


AST_Semant_Map* semant_analyze(Program* node) {
    // std::cerr << "Start Semantic Analysis" << std::endl;
    if (node == nullptr) {
        return nullptr;
    }
    Name_Maps* name_maps = makeNameMaps(node);
    AST_Semant_Visitor semant_visitor(name_maps);
    semant_visitor.visit(node);
    std::cerr << "Semantic Analysis Done" << std::endl;
    return semant_visitor.getSemantMap();
}


/**
 * PROG -> MAINMETHOD CLASSDECLLIST
 */
void AST_Semant_Visitor::visit(Program* node) {
#ifdef DEBUG
    std::cout << "Visiting Program" << std::endl;
#endif

    if (node == nullptr) return;

    // 访问主方法
    if (node->main != nullptr) {
        node->main->accept(*this);
    }

    // 访问所有类声明
    if (node->cdl != nullptr) {
        for (auto cl : *(node->cdl)) {
            cl->accept(*this);
        }
    }
}


/**
 * MAINMETHOD -> PUBLIC INT MAIN '(' ')' '{' VARDECLLIST STMLIST '}'
 */
void AST_Semant_Visitor::visit(MainMethod* node) {
#ifdef DEBUG
    std::cout << "Visiting MainMethod" << std::endl;
#endif

    if (node == nullptr) return;

    current_class = "";
    current_method = "main";

    // 局部变量声明
    if (node->vdl != nullptr) {
        for (auto vd : *(node->vdl)) {
            vd->accept(*this);
        }
    }

    // 所有 Stm
    if (node->sl != nullptr) {
        for (auto s : *(node->sl)) {
            s->accept(*this);
        }
    }

    current_method = "";
}


/**
 *  CLASSDECL -> PUBLIC CLASS ID '{' VARDECLLIST METHODDECLLIST '}'
 *             | PUBLIC CLASS ID EXTENDS ID '{' VARDECLLIST METHODDECLLIST '}'
 */
void AST_Semant_Visitor::visit(ClassDecl* node) {
#ifdef DEBUG
    std::cout << "Visiting ClassDecl" << std::endl;
#endif

    if (node == nullptr) return;

    current_class = node->id->id;

    // 类内变量声明
    if (node->vdl != nullptr) {
        for (auto vd : *(node->vdl)) {
            vd->accept(*this);
        }
    }

    // 类内方法
    if (node->mdl != nullptr) {
        for (auto md : *(node->mdl)) {
            md->accept(*this);
        }
    }

    current_class = "";
}


/**
 * TYPE -> CLASS ID | INT | INT '[' ']'
 */
void AST_Semant_Visitor::visit(Type* node) {
#ifdef DEBUG
    std::cout << "Visiting Type" << std::endl;
#endif
    // 无需语义信息
    return;
}


/** 
 * VARDECL -> CLASS ID ID ';'                                                             // id <=> [a-z_A-Z][a-z_A-Z0-9]*
 *          | INT ID ';' | INT ID '=' CONST ';'
 *          | INT '[' ']' ID ';' | INT '[' ']' ID '=' '{' CONSTLIST '}' ';'               // 数组长度只由[]内的数字决定，若为空则是0
 *          | INT '[' NUM ']' ID ';' | INT '[' NUM ']' ID '=' '{' CONSTLIST '}' ';'       // NUM <=> [1-9][0-9]*|0
 */
void AST_Semant_Visitor::visit(VarDecl* node) {
#ifdef DEBUG
    std::cout << "Visiting VarDecl" << std::endl;
#endif
    // 无需语义信息
    return;
}


/**
 * METHODDECL -> PUBLIC TYPE ID '(' FORMALLIST ')' '{' VARDECLLIST STMLIST '}'
 */
void AST_Semant_Visitor::visit(MethodDecl* node) {
#ifdef DEBUG
    std::cout << "Visiting MethodDecl" << std::endl;
#endif
    
    if (node == nullptr) return;

    current_method = node->id->id;

    // 形参列表
    if (node->fl != nullptr) {
        for (auto f : *(node->fl)) {
            f->accept(*this);
        }
    }

    // 局部变量声明
    if (node->vdl != nullptr) {
        for (auto vd : *(node->vdl)) {
            vd->accept(*this);
        }
    }

    // 所有 Stm
    if (node->sl != nullptr) {
        for (auto s : *(node->sl)) {
            s->accept(*this);
        }
    }

    current_method = "";
}


/**
 * FORMAL -> TYPE ID
 */
void AST_Semant_Visitor::visit(Formal* node) {
#ifdef DEBUG
    std::cout << "Visiting Formal" << std::endl;
#endif

    if (node == nullptr) return;

    // !!!
}


/**
 * NESTED -> '{' STMLIST '}'
 */
void AST_Semant_Visitor::visit(Nested* node) {
#ifdef DEBUG
    std::cout << "Visiting Nested" << std::endl;
#endif

    if (node == nullptr) return;

    // 所有 Stm
    if (node->sl != nullptr) {
        for (auto s : *(node->sl)) {
            s->accept(*this);
        }
    }
}


/**        
 * STM -> IF '(' EXP ')' STM ELSE STM       // 检查 EXP 为 int
 *      | IF '(' EXP ')' STM
 */
void AST_Semant_Visitor::visit(If* node) {
#ifdef DEBUG
    std::cout << "Visiting If" << std::endl;
#endif

    if (node == nullptr) return;

    // 条件 EXP
    if (node->exp != nullptr) {
        node->exp->accept(*this);
        
        // 检查条件是否为整数类型
        AST_Semant* exp_semant = semant_map->getSemant(node->exp);
        if (exp_semant == nullptr || exp_semant->get_type() != TypeKind::INT) {
            cerr << "Error: If condition must be of integer type" << endl;
        }
    } else {
        cerr << "Error: If statement missing condition" << endl;
    }

    // then 语句
    if (node->stm1 != nullptr) {
        node->stm1->accept(*this);
    }

    // else 语句 (如果存在)
    if (node->stm2 != nullptr) {
        node->stm2->accept(*this);
    }

}


/**
 * STM -> WHILE '(' EXP ')' STM     // 检查 EXP 为 int
 *      | WHILE '(' EXP ')' ';'
 */
void AST_Semant_Visitor::visit(While* node) {
#ifdef DEBUG
    std::cout << "Visiting While" << std::endl;
#endif

    if (node == nullptr) return;

    // 循环条件 EXP
    if (node->exp != nullptr) {
        node->exp->accept(*this);
        
        // 检查条件是否为整数类型
        AST_Semant* exp_semant = semant_map->getSemant(node->exp);
        if (exp_semant == nullptr || exp_semant->get_type() != TypeKind::INT) {
            cerr << "Error: While condition must be of integer type" << endl;
        }
    } else {
        cerr << "Error: While statement missing condition" << endl;
    }

    // 访问循环体语句
    if (node->stm != nullptr) {
        is_in_while = true;
        node->stm->accept(*this);
        is_in_while = false;
    }
}


/**
 * STM -> EXP '=' EXP ';'   // 检查类型是否兼容，允许 upcast，允许 int 和 bool 之间的隐式转换
 */
void AST_Semant_Visitor::visit(Assign* node) {
#ifdef DEBUG
    std::cout << "Visiting Assign" << std::endl;
#endif

    if (node == nullptr) return;

    // 左侧 EXP
    if (node->left != nullptr) {
        node->left->accept(*this);
    } else {
        cerr << "Error: Assignment missing left-hand side" << endl;
        return;
    }

    // 右侧 EXP
    if (node->exp != nullptr) {
        node->exp->accept(*this);
    } else {
        cerr << "Error: Assignment missing right-hand side" << endl;
        return;
    }

    // 获取左右表达式的语义信息
    AST_Semant* lhs_semant = semant_map->getSemant(node->left);
    AST_Semant* rhs_semant = semant_map->getSemant(node->exp);

    // 检查左侧是否为左值
    if (lhs_semant == nullptr || !lhs_semant->is_lvalue()) {
        cerr << "Error: Left side of assignment is not an lvalue" << endl;
        return;
    }

    // 如果任一 EXP 语义信息缺失，报错并返回
    if (lhs_semant == nullptr || rhs_semant == nullptr) {
        cerr << "Error: Invalid expression in assignment" << endl;
        return;
    }

    // 类型兼容性检查
    TypeKind lhs_type = lhs_semant->get_type();
    TypeKind rhs_type = rhs_semant->get_type();

    // 类型完全相同
    if (lhs_type == rhs_type) {
        // 如果是类类型，检查是否允许向上转型 (upcast)
        if (lhs_type == TypeKind::CLASS) {
            string lhs_class = get<string>(lhs_semant->get_type_par());
            string rhs_class = get<string>(rhs_semant->get_type_par());
            
            // 检查类型兼容性
            if (lhs_class != rhs_class) {
                // 检查是否为有效向上转型：rhs_class 是否是 lhs_class 的子类
                set<string> ancestors = name_maps->get_ancestors(rhs_class);
                if (ancestors.find(lhs_class) == ancestors.end()) {
                    cerr << "Error: Cannot assign " << rhs_class << " to " << lhs_class << endl;
                    return;
                }
            }
        }

        // array 和 int 类型无需进一步检查
        return;
    } 

    // 其他类型不兼容
    cerr << "Error: Incompatible types in assignment" << endl;
}


/**
 * STM -> EXP '.' ID '(' EXPLIST ')' ';'    // 检查 EXP 为 class，EXP 有 id 方法，EXPLIST 匹配方法参数
 */
void AST_Semant_Visitor::visit(CallStm* node) {
#ifdef DEBUG
    std::cout << "Visiting CallStm" << std::endl;
#endif

    if (node == nullptr) return;

    // 检查 EXP
    if (node->obj == nullptr) {
        cerr << "Error: Call statement missing object expression" << endl;
        return;
    }
    
    // 分析 EXP
    node->obj->accept(*this);
    
    // 获取 EXP 的语义信息
    AST_Semant* obj_semant = semant_map->getSemant(node->obj);
    if (obj_semant == nullptr) {
        cerr << "Error: Invalid object in method call" << endl;
        return;
    }
    
    // EXP 是否为类类型
    if (obj_semant->get_type() != TypeKind::CLASS) {
        cerr << "Error: Method call on non-class type" << endl;
        return;
    }
    
    // 获取类名
    string class_name = get<string>(obj_semant->get_type_par());
    
    // 方法 ID 是否存在
    if (node->name == nullptr) {
        cerr << "Error: Missing method name in call" << endl;
        return;
    }
    
    string method_name = node->name->id;
    if (!name_maps->is_method(class_name, method_name)) {
        cerr << "Error: Method " << method_name << " does not exist in class " << class_name << endl;
        return;
    }
    
    // 访问实参列表
    vector<AST_Semant*> arg_types;
    if (node->par != nullptr) {
        for (auto arg : *(node->par)) {
            if (arg != nullptr) {
                arg->accept(*this);
                AST_Semant* arg_semant = semant_map->getSemant(arg);
                if (arg_semant != nullptr) {
                    arg_types.push_back(arg_semant);
                } else {
                    cerr << "Error: Invalid argument in method call" << endl;
                    return;
                }
            }
        }
    }
    
    // 获取方法形参列表
    vector<Formal*>* formal_list = name_maps->get_method_formal_list(class_name, method_name);
    if (formal_list == nullptr) {
        cerr << "Error: Cannot get formal list for method " << method_name << endl;
        return;
    }
    
    // 检查参数数量是否匹配
    if (node->par == nullptr && !formal_list->empty()) {
        cerr << "Error: Method " << method_name << " expects " << formal_list->size() 
             << " arguments, but none provided" << endl;
        return;
    }
    
    if (node->par != nullptr && formal_list->size() != node->par->size()) {
        cerr << "Error: Method " << method_name << " expects " << formal_list->size() 
             << " arguments, but " << node->par->size() << " provided" << endl;
        return;
    }
    
    // 检查参数类型是否匹配
    for (size_t i = 0; i < arg_types.size(); i++) {
        Formal* formal = (*formal_list)[i];
        AST_Semant* arg_semant = arg_types[i];
        
        // 类型不匹配
        if (formal->type->typeKind != arg_semant->get_type()) {
            cerr << "Error: Type mismatch for argument " << i+1 << " in call to " << method_name << endl;
            return;
        }
        
        // 如果是类类型，还需检查类的兼容性
        if (formal->type->typeKind == TypeKind::CLASS) {
            string formal_class = formal->type->cid->id;
            string arg_class = get<string>(arg_semant->get_type_par());
            
            if (formal_class != arg_class) {
                // 检查是否可以向上转型
                set<string> ancestors = name_maps->get_ancestors(arg_class);
                if (ancestors.find(formal_class) == ancestors.end()) {
                    cerr << "Error: Cannot convert from " << arg_class << " to " << formal_class 
                         << " for argument " << i+1 << endl;
                    return;
                }
            }
        }
    }

    // 至此，检查完毕，方法调用有效
}


/**
 * STM -> CONTINUE ';'  // 检查是否在循环内
 */
void AST_Semant_Visitor::visit(Continue* node) {
#ifdef DEBUG
    std::cout << "Visiting Continue" << std::endl;
#endif

    if (node == nullptr) return;

    // 检查是否在循环内
    if (!is_in_while) {
        cerr << "Error: 'continue' statement not within a loop" << endl;
        return;
    }

    // 检查完毕，且无需语义信息
}


/**
 * STM -> BREAK ';'     // 检查是否在循环内
 */
void AST_Semant_Visitor::visit(Break* node) {
#ifdef DEBUG
    std::cout << "Visiting Break" << std::endl;
#endif

    if (node == nullptr) return;

    // 检查是否在循环内
    if (!is_in_while) {
        cerr << "Error: 'break' statement not within a loop" << endl;
        return;
    }

    // 检查完毕，且无需语义信息 
}


/**
 * STM -> RETURN EXP ';'    // 检查 EXP 为函数返回值
 */
void AST_Semant_Visitor::visit(Return* node) {
#ifdef DEBUG
    std::cout << "Visiting Return" << std::endl;
#endif
    if (node == nullptr) return;

    // RETURN 语句必须在方法内
    if (current_method == "") {
        cerr << "Error: Return statement outside of method context" << endl;
        return;
    }

    // 必须有返回值
    if (node->exp == nullptr) {
        cerr << "Error: Return statement must have an expression to return" << endl;
        return;
    }

    // 访问 EXP
    node->exp->accept(*this);
    AST_Semant* exp_semant = semant_map->getSemant(node->exp);
    if (exp_semant == nullptr) {
        cerr << "Error: Invalid return expression" << endl;
        return;
    }

    // 获取当前方法的返回类型
    Type* method_type = name_maps->get_method(current_class, current_method);
    
    // 检查返回值类型与方法返回类型是否兼容
    TypeKind method_typeKind = method_type->typeKind;
    TypeKind exp_typeKind = exp_semant->get_type();
    
    if (method_typeKind != exp_typeKind) {
        cerr << "Error: Return type mismatch, expected " 
             << (method_typeKind == TypeKind::INT ? "int" : 
                (method_typeKind == TypeKind::CLASS ? "class" : "array"))
             << " but got " 
             << (exp_typeKind == TypeKind::INT ? "int" : 
                (exp_typeKind == TypeKind::CLASS ? "class" : "array"))
             << endl;
        return;
    }
    
    // 如果是类类型，还需检查类的兼容性
    if (method_typeKind == TypeKind::CLASS) {
        string method_class = method_type->cid->id;
        string exp_class = get<string>(exp_semant->get_type_par());
        
        if (method_class != exp_class) {
            // 检查是否可以向上转型（子类->父类）
            set<string> ancestors = name_maps->get_ancestors(exp_class);
            if (ancestors.find(method_class) == ancestors.end()) {
                cerr << "Error: Cannot convert from " << exp_class << " to " << method_class 
                     << " in return statement" << endl;
                return;
            }
        }
    }
    
    // 返回语句类型检查通过
}


/**
 * STM -> PUTINT '(' EXP ')' ';'    // 检查 EXP 为 int
 */
void AST_Semant_Visitor::visit(PutInt* node) {
#ifdef DEBUG
    std::cout << "Visiting PutInt" << std::endl;
#endif

    if (node == nullptr) return;
    
    // EXP 不能为空
    if (node->exp == nullptr) {
        cerr << "Error: putint requires an expression to print" << endl;
        return;
    }
    
    // 访问 EXP
    node->exp->accept(*this);
    
    // 检查 EXP 类型是否为 int
    AST_Semant* exp_semant = semant_map->getSemant(node->exp);
    if (exp_semant == nullptr || exp_semant->get_type() != TypeKind::INT) {
        cerr << "Error: putint requires an integer expression" << endl;
        return;
    }
    
    // 检查完毕，无需语义信息
}


/**
 * STM -> PUTCH '(' EXP ')' ';'    // 检查 EXP 为 int (ASCII)
 */
void AST_Semant_Visitor::visit(PutCh* node) {
#ifdef DEBUG
    std::cout << "Visiting PutCh" << std::endl;
#endif
    
    if (node == nullptr) return;
    
    // EXP 不能为空
    if (node->exp == nullptr) {
        cerr << "Error: putch requires an expression to print" << endl;
        return;
    }
    
    // 访问 EXP
    node->exp->accept(*this);
    
    // 检查 EXP 类型是否为 int
    AST_Semant* exp_semant = semant_map->getSemant(node->exp);
    if (exp_semant == nullptr || exp_semant->get_type() != TypeKind::INT) {
        cerr << "Error: putch requires an integer expression" << endl;
        return;
    }
    
    // 检查完毕，无需语义信息
}


/**
 * STM -> PUTARRAY '(' EXP ',' EXP ')' ';'    // 检查第一个 EXP 为 int，第二个 EXP 为 array
 */
void AST_Semant_Visitor::visit(PutArray* node) {
#ifdef DEBUG
    std::cout << "Visiting PutArray" << std::endl;
#endif

    if (node == nullptr) return;
    
    // 检查第一个参数 (n)
    if (node->n == nullptr) {
        cerr << "Error: putarray requires a size expression as first argument" << endl;
        return;
    }
    
    // 访问并检查第一个 EXP (n)
    node->n->accept(*this);
    AST_Semant* n_semant = semant_map->getSemant(node->n);
    if (n_semant == nullptr || n_semant->get_type() != TypeKind::INT) {
        cerr << "Error: putarray requires an integer as first argument (size)" << endl;
        return;
    }
    
    // 检查第二个参数 (arr)
    if (node->arr == nullptr) {
        cerr << "Error: putarray requires an array expression as second argument" << endl;
        return;
    }
    
    // 访问并检查第二个 EXP (arr)
    node->arr->accept(*this);
    AST_Semant* arr_semant = semant_map->getSemant(node->arr);
    if (arr_semant == nullptr || arr_semant->get_type() != TypeKind::ARRAY) {
        cerr << "Error: putarray requires an array as second argument" << endl;
        return;
    }
    
    // 检查完毕，无需语义信息
}


/**
 * STM -> STARTTIME '(' ')' ';'
 */
void AST_Semant_Visitor::visit(Starttime* node) {
#ifdef DEBUG
    std::cout << "Visiting Starttime" << std::endl;
#endif
    
    // 无需语义信息
    return;
}


/**
 * STM -> STOPTIME '(' ')' ';'
 */
void AST_Semant_Visitor::visit(Stoptime* node) {
#ifdef DEBUG
    std::cout << "Visiting Stoptime" << std::endl;
#endif
    // 无需语义信息
    return;
}


/**
 * EXP -> EXP OP EXP    // 检查 EXP 是 int 或 array ，返回 EXP 类型。不用检查两个数组的长度是否相同，因为有些数组长度是runtime才能确定的（比如getarray()）
 */
void AST_Semant_Visitor::visit(BinaryOp* node) {
#ifdef DEBUG
    std::cout << "Visiting BinaryOp" << std::endl;
#endif

    if (node == nullptr) return;
    
    // 访问左右两个 EXP
    if (node->left != nullptr) {
        node->left->accept(*this);
    } else {
        cerr << "Error: Binary operation missing left operand" << endl;
        return;
    }
    
    if (node->right != nullptr) {
        node->right->accept(*this);
    } else {
        cerr << "Error: Binary operation missing right operand" << endl;
        return;
    }
    
    // 获取左右 EXP 的语义信息
    AST_Semant* left_semant = semant_map->getSemant(node->left);
    AST_Semant* right_semant = semant_map->getSemant(node->right);
    
    if (left_semant == nullptr || right_semant == nullptr) {
        cerr << "Error: Invalid operands in binary operation" << endl;
        return;
    }
    
    // 获取运算符
    string op = "";
    if (node->op != nullptr) {
        op = node->op->op;
    } else {
        cerr << "Error: Binary operation missing operator" << endl;
        return;
    }
    
    // 检查左右 EXP 类型是否相同
    TypeKind left_type = left_semant->get_type();
    TypeKind right_type = right_semant->get_type();
    
    // 两边必须是相同类型
    if (left_type != right_type) {
        cerr << "Error: Incompatible types in binary operation: " << op << endl;
        return;
    }

    // 判断结果类型
    TypeKind result_type;
    
    if (left_type == TypeKind::INT) {
        // 对于整数，允许所有算术和比较操作
        result_type = TypeKind::INT; // 所有整数操作都返回整数
    } 
    else if (left_type == TypeKind::ARRAY) {
        // 对于数组，只允许 + - * / == !=
        if (op == "+" || op == "-" || op == "*" || op == "/") {
            result_type = TypeKind::ARRAY;
        }
        else if (op == "==" && op == "!=") {
            result_type = TypeKind::INT;
        }
        else {
            cerr << "Error: Operator " << op << " not supported for array types" << endl;
            return;
        }
    }
    
    // 创建并保存语义信息
    AST_Semant* semant = new AST_Semant(
        AST_Semant::Kind::Value,
        result_type,
        monostate(),  // 无需额外类型信息
        false         // 二元操作结果不是左值
    );
    
    semant_map->setSemant(node, semant);
}


/**
 * EXP -> '!' EXP   // 检查 EXP 是 int 或 array ，返回 EXP 类型。
 *      | '-' EXP
 */
void AST_Semant_Visitor::visit(UnaryOp* node) {
#ifdef DEBUG
    std::cout << "Visiting UnaryOp" << std::endl;
#endif

    if (node == nullptr) return;
    
    // EXP 不能为空
    if (node->exp == nullptr) {
        cerr << "Error: Unary operation missing operand" << endl;
        return;
    }
    
    // 访问 EXP
    node->exp->accept(*this);
    AST_Semant* exp_semant = semant_map->getSemant(node->exp);
    if (exp_semant == nullptr) {
        cerr << "Error: Invalid operand in unary operation" << endl;
        return;
    }
    
    // 检查 EXP 类型
    TypeKind exp_type = exp_semant->get_type();
    if (exp_type != TypeKind::INT && exp_type != TypeKind::ARRAY) {
        cerr << "Error: Unary operation requires int or array type" << endl;
        return;
    }
    
    // 创建并保存语义信息 - 保持操作数的类型
    AST_Semant* semant = new AST_Semant(
        AST_Semant::Kind::Value,
        exp_type,
        exp_semant->get_type_par(),  // 保持原类型的参数
        false                        // 一元操作结果不是左值
    );
    
    semant_map->setSemant(node, semant);
}


/**
 * EXP -> EXP '[' EXP ']'    // 检查第一个 EXP 是 array，第二个 EXP 为 int 
 */
void AST_Semant_Visitor::visit(ArrayExp* node) {
#ifdef DEBUG
    std::cout << "Visiting ArrayExp" << std::endl;
#endif

    if (node == nullptr) return;
    
    // 检查数组表达式
    if (node->arr == nullptr) {
        cerr << "Error: Array access missing array expression" << endl;
        return;
    }
    
    // 分析数组表达式
    node->arr->accept(*this);
    AST_Semant* arr_semant = semant_map->getSemant(node->arr);
    if (arr_semant == nullptr) {
        cerr << "Error: Invalid array expression" << endl;
        return;
    }
    
    // 验证第一个 EXP 是数组类型
    if (arr_semant->get_type() != TypeKind::ARRAY) {
        cerr << "Error: Array access requires an array type" << endl;
        return;
    }
    
    // 检查索引表达式
    if (node->index == nullptr) {
        cerr << "Error: Array access missing index expression" << endl;
        return;
    }
    
    // 分析索引表达式
    node->index->accept(*this);
    AST_Semant* index_semant = semant_map->getSemant(node->index);
    if (index_semant == nullptr) {
        cerr << "Error: Invalid index expression" << endl;
        return;
    }
    
    // 验证索引表达式是整数类型
    if (index_semant->get_type() != TypeKind::INT) {
        cerr << "Error: Array index must be of integer type" << endl;
        return;
    }
    
    // 数组访问表达式返回整数类型
    AST_Semant* semant = new AST_Semant(
        AST_Semant::Kind::Value,
        TypeKind::INT,
        monostate(),  // 无需额外类型信息
        true          // 数组元素是左值
    );
    
    // 保存语义信息
    semant_map->setSemant(node, semant);
}


/**
 * EXP -> EXP '.' ID '(' EXPLIST ')'    // 检查 EXP 为 class，EXP 有 id 方法，EXPLIST 匹配方法参数，返回方法类型
 */
void AST_Semant_Visitor::visit(CallExp* node) {
#ifdef DEBUG
    std::cout << "Visiting CallExp" << std::endl;
#endif

    if (node == nullptr) return;

    // 检查 EXP
    if (node->obj == nullptr) {
        cerr << "Error: Call expression missing object expression" << endl;
        return;
    }

    // 分析 EXP
    node->obj->accept(*this);

    // 获取 EXP 的语义信息
    AST_Semant* obj_semant = semant_map->getSemant(node->obj);
    if (obj_semant == nullptr) {
        cerr << "Error: Invalid object in method call expression" << endl;
        return;
    }

    // EXP 是否为类类型
    if (obj_semant->get_type() != TypeKind::CLASS) {
        cerr << "Error: Method call on non-class type in expression" << endl;
        return;
    }

    // 获取类名
    string class_name = get<string>(obj_semant->get_type_par());

    // 方法 ID 是否存在
    if (node->name == nullptr) {
        cerr << "Error: Missing method name in call expression" << endl;
        return;
    }

    string method_name = node->name->id;
    if (!name_maps->is_method(class_name, method_name)) {
        cerr << "Error: Method " << method_name << " does not exist in class " << class_name << endl;
        return;
    }

    // 访问实参列表
    vector<AST_Semant*> arg_types;
    if (node->par != nullptr) {
        for (auto arg : *(node->par)) {
            if (arg != nullptr) {
                arg->accept(*this);
                AST_Semant* arg_semant = semant_map->getSemant(arg);
                if (arg_semant != nullptr) {
                    arg_types.push_back(arg_semant);
                } else {
                    cerr << "Error: Invalid argument in method call expression" << endl;
                    return;
                }
            }
        }
    }

    // 获取方法形参列表
    vector<Formal*>* formal_list = name_maps->get_method_formal_list(class_name, method_name);
    if (formal_list == nullptr) {
        cerr << "Error: Cannot get formal list for method " << method_name << endl;
        return;
    }

    // 检查参数数量是否匹配
    if (node->par == nullptr && !formal_list->empty()) {
        cerr << "Error: Method " << method_name << " expects " << formal_list->size() 
             << " arguments, but none provided" << endl;
        return;
    }

    if (node->par != nullptr && formal_list->size() != node->par->size()) {
        cerr << "Error: Method " << method_name << " expects " << formal_list->size() 
             << " arguments, but " << node->par->size() << " provided" << endl;
        return;
    }

    // 检查参数类型是否匹配
    for (size_t i = 0; i < arg_types.size(); i++) {
        Formal* formal = (*formal_list)[i];
        AST_Semant* arg_semant = arg_types[i];
        
        // 类型不匹配
        if (formal->type->typeKind != arg_semant->get_type()) {
            cerr << "Error: Type mismatch for argument " << i+1 << " in call to " << method_name << endl;
            return;
        }
        
        // 如果是类类型，还需检查类的兼容性
        if (formal->type->typeKind == TypeKind::CLASS) {
            string formal_class = formal->type->cid->id;
            string arg_class = get<string>(arg_semant->get_type_par());
            
            if (formal_class != arg_class) {
                // 检查是否可以向上转型
                set<string> ancestors = name_maps->get_ancestors(arg_class);
                if (ancestors.find(formal_class) == ancestors.end()) {
                    cerr << "Error: Cannot convert from " << arg_class << " to " << formal_class 
                         << " for argument " << i+1 << endl;
                    return;
                }
            }
        }
    }

    // 获取方法返回类型并设置语义信息
    Type* method_type = name_maps->get_method(class_name, method_name);
    if (method_type == nullptr) {
        cerr << "Error: Cannot get return type for method " << method_name << endl;
        return;
    }

    // 创建表达式的语义信息，基于方法的返回类型
    variant<monostate, string, int> type_par;
    if (method_type->typeKind == TypeKind::CLASS) {
        type_par = method_type->cid->id;
    } else if (method_type->typeKind == TypeKind::ARRAY) {
        if (method_type->arity != nullptr) {
            type_par = method_type->arity->val;
        } else {
            type_par = 0; // 默认数组大小为0
        }
    } else {
        type_par = monostate{}; // INT类型无需额外信息
    }

    AST_Semant* semant = new AST_Semant(
        AST_Semant::Kind::Value,
        method_type->typeKind,
        type_par,
        false // 方法调用结果不是左值
    );

    semant_map->setSemant(node, semant);

}


/**
 * EXP -> EXP '.' ID    // 检查 EXP 为 class，EXP 有 id 变量，返回变量类型
 */
void AST_Semant_Visitor::visit(ClassVar* node) {
#ifdef DEBUG
    std::cout << "Visiting ClassVar" << std::endl;
#endif

    if (node == nullptr) return;

    // 检查对象表达式
    if (node->obj == nullptr) {
        cerr << "Error: Missing object in class variable access" << endl;
        return;
    }

    // 分析对象表达式
    node->obj->accept(*this);
    AST_Semant* obj_semant = semant_map->getSemant(node->obj);
    if (obj_semant == nullptr) {
        cerr << "Error: Invalid object in class variable access" << endl;
        return;
    }

    // 验证对象是类类型
    if (obj_semant->get_type() != TypeKind::CLASS) {
        cerr << "Error: Cannot access member of non-class type" << endl;
        return;
    }

    // 获取类名
    string class_name = get<string>(obj_semant->get_type_par());

    // 检查变量名
    if (node->id == nullptr) {
        cerr << "Error: Missing identifier in class variable access" << endl;
        return;
    }
    
    string var_name = node->id->id;
    
    // 检查该类是否有此变量
    if (!name_maps->is_class_var(class_name, var_name)) {
        cerr << "Error: Class " << class_name << " has no variable named " << var_name << endl;
        return;
    }
    
    // 获取变量声明
    VarDecl* var_decl = name_maps->get_class_var(class_name, var_name);
    if (var_decl == nullptr) {
        cerr << "Error: Failed to retrieve declaration for " << var_name << endl;
        return;
    }
    
    // 根据变量类型创建语义信息
    Type* type = var_decl->type;
    
    if (type->typeKind == TypeKind::CLASS) {
        // 类变量
        AST_Semant* semant = new AST_Semant(
            AST_Semant::Kind::Value,
            TypeKind::CLASS,
            type->cid->id,  // 类名作为类型信息
            true            // 变量是左值
        );
        semant_map->setSemant(node, semant);
    } else if (type->typeKind == TypeKind::INT) {
        // 整型变量
        AST_Semant* semant = new AST_Semant(
            AST_Semant::Kind::Value,
            TypeKind::INT,
            monostate(),    // 整型无需额外类型信息
            true            // 变量是左值
        );
        semant_map->setSemant(node, semant);
    } else if (type->typeKind == TypeKind::ARRAY) {
        // 数组变量
        int array_size = 0;
        if (type->arity != nullptr) {
            array_size = type->arity->val;
        }
        
        AST_Semant* semant = new AST_Semant(
            AST_Semant::Kind::Value,
            TypeKind::ARRAY,
            array_size,     // 数组尺寸作为类型信息
            true            // 变量是左值
        );
        semant_map->setSemant(node, semant);
    }

}


/**
 * EXP -> TRUE | FALSE      // 返回 int
 */
void AST_Semant_Visitor::visit(BoolExp* node) {
#ifdef DEBUG
    std::cout << "Visiting BoolExp" << std::endl;
#endif

    if (node == nullptr) return;
    
    // 创建布尔值的语义信息
    AST_Semant* semant = new AST_Semant(
        AST_Semant::Kind::Value,
        TypeKind::INT,     // 布尔值实际上是整数
        monostate(),       // 无需额外类型信息
        false              // 布尔值不是左值
    );
    
    semant_map->setSemant(node, semant);
}


/**
 * EXP -> THIS      // 返回 class
 */
void AST_Semant_Visitor::visit(This* node) {
#ifdef DEBUG
    std::cout << "Visiting This" << std::endl;
#endif

    if (node == nullptr) return;
    
    // THIS 必须在类方法中使用
    if (current_class.empty()) {
        cerr << "Error: 'this' cannot be used outside of a class method" << endl;
        return;
    }
    
    // 创建 THIS 的语义信息
    AST_Semant* semant = new AST_Semant(
        AST_Semant::Kind::Value,
        TypeKind::CLASS,
        current_class,     // 当前类名作为额外类型信息
        false              // THIS 不是左值
    );
    
    semant_map->setSemant(node, semant);
}


/**
 * EXP -> LENGTH '(' EXP ')'    // 检查 EXP 为 array，返回 int
 */
void AST_Semant_Visitor::visit(Length* node) {
#ifdef DEBUG
    std::cout << "Visiting Length" << std::endl;
#endif

    if (node == nullptr) return;
    
    // 检查传入的数组表达式
    if (node->exp == nullptr) {
        cerr << "Error: length() requires an array expression" << endl;
        return;
    }
    
    // 分析数组表达式
    node->exp->accept(*this);
    
    // 获取表达式的语义信息
    AST_Semant* exp_semant = semant_map->getSemant(node->exp);
    if (exp_semant == nullptr) {
        cerr << "Error: Invalid expression in length()" << endl;
        return;
    }
    
    // 验证表达式是数组类型
    if (exp_semant->get_type() != TypeKind::ARRAY) {
        cerr << "Error: length() requires an array argument" << endl;
        return;
    }
    
    // length() 返回整数值，表示数组长度
    AST_Semant* semant = new AST_Semant(
        AST_Semant::Kind::Value,
        TypeKind::INT,
        monostate(),  // 无需额外类型信息
        false         // length() 结果不是左值
    );
    
    // 保存语义信息
    semant_map->setSemant(node, semant);

}


/**
 * EXP -> '(' '{' STMLIST '}' EXP ')'   // 返回 EXP 类型
 */
void AST_Semant_Visitor::visit(Esc* node) {
#ifdef DEBUG
    std::cout << "Visiting Esc" << std::endl;
#endif

    if (node == nullptr) return;
    
    // 检查 Stm 列表
    if (node->sl != nullptr) {
        for (auto s : *(node->sl)) {
            if (s != nullptr) {
                s->accept(*this);
            }
        }
    }
    
    // 检查 EXP
    if (node->exp == nullptr) {
        cerr << "Error: Esc expression missing final expression" << endl;
        return;
    }
    
    // 访问 EXP
    node->exp->accept(*this);
    
    // 获取 EXP 的语义信息
    AST_Semant* exp_semant = semant_map->getSemant(node->exp);
    if (exp_semant == nullptr) {
        cerr << "Error: Invalid final expression in Esc" << endl;
        return;
    }
    
    // Esc 返回 EXP 的类型
    AST_Semant* semant = new AST_Semant(
        AST_Semant::Kind::Value,
        exp_semant->get_type(),
        exp_semant->get_type_par(), // 保持原类型的参数
        false                       // Esc 结果不是左值
    );
    
    semant_map->setSemant(node, semant);
}


/**
 * EXP -> GETINT '(' ')'    // 返回 int
 */
void AST_Semant_Visitor::visit(GetInt* node) {
#ifdef DEBUG
    std::cout << "Visiting GetInt" << std::endl;
#endif

    if (node == nullptr) return;
    
    // getint() 返回整数值
    AST_Semant* semant = new AST_Semant(
        AST_Semant::Kind::Value,
        TypeKind::INT,
        monostate(),
        false         // getint() 结果不是左值
    );
    
    semant_map->setSemant(node, semant);
}


/**
 * EXP -> GETCH '(' ')'    // 返回 int
 */
void AST_Semant_Visitor::visit(GetCh* node) {
#ifdef DEBUG
    std::cout << "Visiting GetCh" << std::endl;
#endif

    if (node == nullptr) return;
        
    // getch() 返回整数值 (ASCII)
    AST_Semant* semant = new AST_Semant(
        AST_Semant::Kind::Value,
        TypeKind::INT,
        monostate(), 
        false         // getch() 结果不是左值
    );

    semant_map->setSemant(node, semant);
}


/**
 * EXP -> GETARRAY '(' EXP ')'    // 检查 EXP 为 array，返回 int
 */
void AST_Semant_Visitor::visit(GetArray* node) {
#ifdef DEBUG
    std::cout << "Visiting GetArray" << std::endl;
#endif
    if (node == nullptr) return;
    
    // 首先分析传递给 getarray 的 EXP
    if (node->exp != nullptr) {
        node->exp->accept(*this);
    } 
    else {
        cerr << "Error: getarray expects an array argument" << endl;
        return;
    }

    // 检查 EXP 是否为 array
    AST_Semant* exp_semant = semant_map->getSemant(node->exp);
    if (exp_semant == nullptr || exp_semant->get_type() != TypeKind::ARRAY) {
        cerr << "Error: GetArray expects an array argument" << endl;
        return;
    }
    
    // 返回整数值
    AST_Semant* semant = new AST_Semant(
        AST_Semant::Kind::Value,
        TypeKind::INT,
        monostate(),  // 无需额外类型信息
        false         // getarry 结果不是左值
    );
    
    // 保存语义信息
    semant_map->setSemant(node, semant);
}


/**
 * EXP -> ID    // 返回变量类型
 */
void AST_Semant_Visitor::visit(IdExp* node) {
#ifdef DEBUG
    std::cout << "Visiting IdExp" << std::endl;
#endif

    if (node == nullptr) return;
    
    string id_name = node->id;
    
    // 检查变量是否在当前作用域中存在
    VarDecl* var_decl = nullptr;
    Formal* formal = nullptr;
    Type* var_type = nullptr;
    bool is_lvalue = true;      // 标识符通常是左值
    
    // 1. 首先查找局部变量或形参 (如果当前在方法内)
    if (!current_method.empty()) {
        // 检查是否是局部变量
        if (name_maps->is_method_var(current_class, current_method, id_name)) {
            var_decl = name_maps->get_method_var(current_class, current_method, id_name);
            var_type = var_decl->type;
        }
        // 检查是否是形参
        else if (name_maps->is_method_formal(current_class, current_method, id_name)) {
            formal = name_maps->get_method_formal(current_class, current_method, id_name);
            var_type = formal->type;
        }
    }
    
    // 2. 如果不是局部变量或形参，检查是否是类成员变量 (如果在类内)
    if (var_type == nullptr && !current_class.empty()) {
        if (name_maps->is_class_var(current_class, id_name)) {
            var_decl = name_maps->get_class_var(current_class, id_name);
            var_type = var_decl->type;
        }
    }
    
    // 3. 如果找不到变量，报错
    if (var_type == nullptr) {
        cerr << "Error: Undefined identifier '" << id_name << "'" << endl;
        return;
    }
    
    // 创建并保存语义信息
    variant<monostate, string, int> type_par;
    if (var_type->typeKind == TypeKind::CLASS) {
        type_par = var_type->cid->id;
    } else if (var_type->typeKind == TypeKind::ARRAY) {
        if (var_type->arity != nullptr) {
            type_par = var_type->arity->val;
        } else {
            type_par = 0;  // 默认数组大小为0
        }
    } else {
        type_par = monostate{};  // INT类型无需额外信息
    }
    
    AST_Semant* semant = new AST_Semant(
        AST_Semant::Kind::Value,
        var_type->typeKind,
        type_par,
        is_lvalue
    );
    
    semant_map->setSemant(node, semant);
}

// 无需语义信息
void AST_Semant_Visitor::visit(OpExp* node) {
#ifdef DEBUG
    std::cout << "Visiting OpExp" << std::endl;
#endif
    return;
}


/**
 * EXP -> NUM    // 返回 int
 */
void AST_Semant_Visitor::visit(IntExp* node) {
#ifdef DEBUG
    std::cout << "Visiting IntExp" << std::endl;
#endif

    if (node == nullptr) return;
    
    AST_Semant* semant = new AST_Semant(
        AST_Semant::Kind::Value,
        TypeKind::INT,
        monostate(),  // 无需额外类型信息
        false         // 整数常量不是左值
    );
    
    // 保存语义信息
    semant_map->setSemant(node, semant);
}