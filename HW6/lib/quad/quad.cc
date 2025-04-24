#define DEBUG
#undef DEBUG

#include <iostream>
#include <string>
#include <set>
#include "quad.hh"
#include "temp.hh"

using namespace std;
using namespace quad;

std::string QuadTerm::print() {
#ifdef DEBUG
    cout << "In QuadTerm::print" << endl;
#endif
    TempExp *t_tempexp;
    switch (kind) {
        case QuadTermKind::TEMP:
            t_tempexp = get<TempExp*>(term);
            return "t" + to_string(t_tempexp->temp->num) + ":" + (t_tempexp->type == Type::INT?"INT":"PTR"); 
        case QuadTermKind::CONST:
            return "Const:"+to_string(this->get_const());
        case QuadTermKind::MAME:
            return "Name:"+get<std::string>(term);
    }
    return "";
}
TempExp* QuadTerm::get_temp() {
    if (kind == QuadTermKind::TEMP) {
        return get<TempExp*>(term);
    }
    return nullptr;
}

int QuadTerm::get_const() {
    if (kind == QuadTermKind::CONST) {
        return get<int>(term);
    }
    return 0;
}

string QuadTerm::get_name() {
    if (kind == QuadTermKind::MAME) {
        return get<std::string>(term);
    }
    return nullptr;
}

static string print_indent(int indent) {
#ifdef DEBUG
    cout << "In print_indent" << endl;
#endif
    std::string output_str; output_str.reserve(20);
    for (int i = 0; i < indent; i++) {
        output_str += " ";
    }
    return output_str;
}

string print_def_use(set<Temp*> *def, set<Temp*> *use) {
#ifdef DEBUG
    cout << "In print_def_use" << endl;
#endif
    std::string output_str; output_str.reserve(200);
    output_str += "def: ";
    if (def != nullptr) {
        for (auto t : *def) {
            output_str += to_string(t->name());
            output_str += " ";
        }
    }
    output_str += "use: ";
    if (use != nullptr) {
        for (auto t : *use) {
            output_str += to_string(t->name());
            output_str += " ";
        }
    }
    return output_str;
}

static string print_temp(TempExp *t) {
#ifdef DEBUG
    cout << "In print_temp" << endl;
#endif
    std::string output_str; output_str.reserve(20);
    if (t != nullptr) {
        output_str += "t";
        output_str += to_string(t->temp->num);
        output_str += ":";
        output_str += (t->type == Type::INT?"INT":"PTR");
    }
    return output_str;
}

static std::string print_label(Label *l) {
#ifdef DEBUG
    cout << "In print_label" << endl;
#endif
    std::string str; str.reserve(20);
    if (l != nullptr) {
        str = "L" + to_string(l->num);
    }
    return str;
}

static std::string print_call(QuadCall *call) {
#ifdef DEBUG
    cout << "In print_call" << endl;
#endif
    std::string use_str; use_str.reserve(100);
    std::string id = call->name;
    QuadTerm *obj_term = call->obj_term;
    vector<QuadTerm*> *args = call->args;
    use_str += call->name;
    use_str += "[";
    use_str +=  obj_term->print();
    use_str += "] (";
    bool first = true;
    for (auto arg : *args) {
        use_str += (first?"":", ");
        use_str += arg->print();
        first = false;
    }
    use_str += "); " ;
    return use_str;
}

string print_extcall(QuadExtCall *call) {
#ifdef DEBUG
    cout << "In print_extcall" << endl;
#endif
    string use_str; use_str.reserve(100);
    vector<QuadTerm*> *args = call->args;
    use_str += call->extfun;
    use_str += "(";
    bool first = true;
    for (auto arg : *args) {
        use_str += (first?"":", ");
        use_str += arg->print();
        first = false;
    }
    use_str += "); " ;
    return use_str;
}

void QuadProgram::print(string &use_str, int indent, bool to_print_def_use) {
#ifdef DEBUG
    cout << "In QuadProgram::print: " <<  (this->kind == QuadKind::PROGRAM?"Program!":"Oh?") << endl;
#endif
    if (quadFuncDeclList == nullptr) {
        return ;
    }

    for (auto func : *quadFuncDeclList) {
        if (func == nullptr) {
            cerr << "Error: QuadFuncDecl is null!" << endl;
            continue;
        }
        func->print(use_str, indent, to_print_def_use);
    }
    return ;
}

void QuadFuncDecl::print(string &use_str, int indent, bool to_print_def_use) {
#ifdef DEBUG
    cout << "In QuadFuncDecl::print" << endl;
#endif
    if (this->quadblocklist == nullptr) {
        return;
    }
    use_str += "Function "; use_str += this->funcname;
    if (params != nullptr) {
        use_str += "(";
        bool first = true;
        for (auto param : *this->params) {
            if (!first) {
                use_str += ", ";
            }
            use_str += "t"+to_string(param->num);
            first = false;
        }
        use_str += ")";
    }
    use_str += " last_label=";
    use_str += to_string(this->last_label_num);
    use_str += " last_temp=";
    use_str += to_string(this->last_temp_num);
    use_str += ":\n";
    for (auto block : *this->quadblocklist) {
        block->print(use_str, indent+2, to_print_def_use);
    }
    return ;
}

void QuadBlock::print(string &use_str, int indent, bool to_print_def_use) {
#ifdef DEBUG
    cout << "In QuadBlock::print" << endl;
#endif
    use_str += print_indent(indent);
    use_str += "Block: Entry Label: ";
    use_str += print_label(this->entry_label);
    use_str +=+ "\n";
    if (this->exit_labels != nullptr) {
        use_str += print_indent(indent+2);
        use_str += "Exit labels: ";
        for (auto label : *this->exit_labels) {
            use_str += print_label(label);
            use_str += " ";
        }
        use_str += "\n";
    }
    for (auto quad : *this->quadlist) {
        quad->print(use_str, indent+2, to_print_def_use);
    }
    return ;
}

void QuadMove::print(string &use_str, int indent, bool to_print_def_use) {
#ifdef DEBUG
    cout << "In QuadMove::print" << endl;
#endif
    TempExp *dst_temp = this->dst;
    QuadTerm *src_term = this->src;
    use_str += print_indent(indent);
    use_str += "MOVE ";
    use_str += print_temp(dst_temp);
    use_str += " <- ";
    use_str += src_term->print();
    use_str += "; ";
    use_str += (to_print_def_use? print_def_use(this->def, this->use) : "");
    use_str += "\n";
    return ;
}

void QuadLoad::print(string &use_str, int indent, bool to_print_def_use) {
#ifdef DEBUG
    cout << "In QuadLoad::print" << endl;
#endif
    TempExp *dst_temp = this->dst;
    QuadTerm *src_term = this->src;
    use_str += print_indent(indent);
    use_str += "LOAD ";
    use_str +=  print_temp(dst_temp);
    use_str += " <- Mem(";
    use_str += src_term->print() + "); ";
    use_str += (to_print_def_use? print_def_use(this->def, this->use) : "");
    use_str += "\n";
    return;
}

//store is term->mem(term)
void QuadStore::print(string &use_str, int indent, bool to_print_def_use) {
#ifdef DEBUG
    cout << "In QuadStore::print" << endl;
#endif
    QuadTerm *src_term = this->src;
    QuadTerm *dst_term = this->dst;
    use_str += print_indent(indent);
    use_str += "STORE ";
    use_str += src_term->print();
    use_str += " -> ";
    use_str += "Mem(";
    use_str += dst_term->print();
    use_str += "); ";
    use_str += (to_print_def_use? print_def_use(this->def, this->use) : "");
    use_str += "\n";
    return;
}
        
void QuadMoveBinop::print(string &use_str, int indent, bool to_print_def_use) {
#ifdef DEBUG
    cout << "In QuadMoveBinop::print" << endl;
#endif
    TempExp *dst_temp = this->dst;
    string binop = this->binop;
    QuadTerm *left_term = this->left;
    QuadTerm *right_term = this->right;
    use_str += print_indent(indent);
    use_str += "MOVE_BINOP ";
    use_str += print_temp(dst_temp);
    use_str += " <- ";
    use_str += "(";
    use_str += binop;
    use_str += ", ";
    use_str += left_term->print();
    use_str += ", ";
    use_str += right_term->print();
    use_str += "); ";
    use_str += (to_print_def_use? print_def_use(this->def, this->use) : "");
    use_str += "\n";
    return ;
}

void QuadCall::print(string &use_str, int indent, bool to_print_def_use) {
#ifdef DEBUG
    cout << "In QuadCall::print" << endl;
#endif
        string call_string = print_call(this);
        use_str += print_indent(indent);
        use_str += "CALL ";
        use_str += call_string;
        use_str += (to_print_def_use? print_def_use(this->def, this->use) : "");
        use_str += "\n";
        return;
}

void QuadMoveCall::print(string &use_str, int indent, bool to_print_def_use) {
#ifdef DEBUG
    cout << "In QuadMoveCall::print" << endl;
#endif
    TempExp *dst_temp = this->dst;
    QuadCall *call = this->call;
    use_str += print_indent(indent);
    use_str += "MOVE_CALL ";
    use_str += print_temp(dst_temp);
    use_str += " <- ";
    use_str += print_call(call);
    use_str += (to_print_def_use? print_def_use(this->def, this->use) : "");
    use_str += "\n";
    return;
}

void QuadExtCall::print(string &use_str, int indent, bool to_print_def_use) {
#ifdef DEBUG
    cout << "In QuadExtCall::print" << endl;
#endif
    string extcall_str = print_extcall(this);
    use_str += print_indent(indent);
    use_str += "EXTCALL ";
    use_str += extcall_str;
    use_str += (to_print_def_use? print_def_use(this->def, this->use) : "");
    use_str += "\n";
    return;
}
        
void QuadMoveExtCall::print(string &use_str, int indent, bool to_print_def_use) {
#ifdef DEBUG
    cout << "In QuadMoveExtCall::print" << endl;
#endif
    TempExp *dst_temp = this->dst;
    QuadExtCall *extcall = this->extcall;
    use_str += print_indent(indent);
    use_str += "MOVE_EXTCALL ";
    use_str += print_temp(dst_temp);
    use_str += " <- ";
    use_str += print_extcall(extcall);
    use_str += (to_print_def_use? print_def_use(this->def, this->use) : "");
    use_str += "\n";
    return;
}

void QuadLabel::print(string &use_str, int indent, bool to_print_def_use) {
#ifdef DEBUG
    cout << "In QuadLabel::print" << endl;
#endif
    use_str += print_indent(indent);
    use_str += "LABEL ";
    use_str += print_label(this->label);
    use_str += "; ";
    use_str += (to_print_def_use? print_def_use(this->def, this->use) : "");
    use_str += "\n";
    return;
}

void QuadJump::print(string &use_str, int indent, bool to_print_def_use) {
#ifdef DEBUG
    cout << "In QuadJump::print" << endl;
#endif
    use_str += print_indent(indent);
    use_str += "JUMP ";
    use_str += print_label(this->label);
    use_str += "; ";
    use_str += (to_print_def_use? print_def_use(this->def, this->use) : "");
    use_str += "\n";
    return;
}

void QuadCJump::print(string &use_str, int indent, bool to_print_def_use) {
#ifdef DEBUG
    cout << "In QuadCJump::print" << endl;
#endif
    Label *true_label = this->t;
    Label *false_label = this->f;
    QuadTerm *left_term = this->left;
    QuadTerm *right_term = this->right;
    use_str += print_indent(indent);
    use_str += "CJUMP ";
    use_str += this->relop;
    use_str += " ";
    use_str += left_term->print();
    use_str += " ";
    use_str += right_term->print();
    use_str += "? ";
    use_str += print_label(true_label);
    use_str += " : ";
    use_str += print_label(false_label);
    use_str += "; ";
    use_str += (to_print_def_use? print_def_use(this->def, this->use) : "");
    use_str += "\n";
    return;
}

void QuadPhi::print(string &use_str, int indent, bool to_print_def_use) {
#ifdef DEBUG
    cout << "In QuadPhi::print" << endl;
#endif
    TempExp *phi_temp = this->temp;
    string phi_str = print_indent(indent) + "PHI " + print_temp(phi_temp) + " <- (";
    bool first = true;
    for (int i=0; i< this->args->size(); i++) {
        Temp *arg_temp = this->args->at(i).first;
        Label *arg_label = this->args->at(i).second;
        use_str += "t";
        use_str += to_string(arg_temp->num);
        use_str +=  ", ";
        use_str +=  print_label(arg_label);
        use_str += (first?"":"; ");
        first = false;
    }
    use_str += "); ";
    use_str += (to_print_def_use? print_def_use(this->def, this->use) : "");
    use_str += "\n";
    return ;
}

void QuadReturn::print(string &use_str, int indent, bool to_print_def_use) {
#ifdef DEBUG
    cout << "In QuadReturn::print" << endl;  
#endif
    QuadTerm *ret_term = this->value;
    use_str += print_indent(indent);
    use_str += "RETURN ";
    use_str += ret_term->print();
    use_str += "; ";
    use_str += (to_print_def_use? print_def_use(this->def, this->use) : "");
    use_str += "\n";
    return ;
}
