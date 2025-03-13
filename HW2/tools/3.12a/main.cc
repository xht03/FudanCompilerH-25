#include <iostream>
#include <string>
#include <stack>
#include <map>

using namespace std;

string action;                      // 解析动作 (shift, reduce, accept)
pair<string, string> prod;          // 当前产生式
int state, curState, newState;      // 解析状态，栈顶状态，规约后新状态

// 产生式
const map<int, pair<string, string>> prods = {
    { 1, { "S", "E" } },
    { 2, { "E", "i" } },
    { 3, { "E", "i(E)" } },
    { 4, { "E", "E+i" } },
};

// 分析表
const map<pair<int, char>, string> table = {

    // 状态0
    // S -> ·E
    // E -> ·i
    // E -> ·i(E)
    // E -> ·E+i
    { { 0, 'i' }, "s3" },
    { { 0, 'E' }, "1" },
    { { 0, 'S' }, "2" },

    // 状态1
    // S -> E·
    // E -> E·+i
    { { 1, '+' }, "s4" },

    { { 1, '$' }, "r1" },
    { { 1, 'i' }, "r1" },
    { { 1, '(' }, "r1" },
    { { 1, ')' }, "r1" },

    // 状态2
    // S' -> S·
    { { 2, '$' }, "$" },

    // 状态3
    // E -> i·
    // E -> i·(E)
    { { 3, '(' }, "s5" },

    { { 3, ')' }, "r2" },
    { { 3, '+' }, "r2" },
    { { 3, 'i' }, "r2" },
    { { 3, '$' }, "r2" },

    // 状态4
    // E -> E+·i
    { { 4, 'i' }, "s6" },

    // 状态5
    // E -> i(·E)
    // E -> ·i
    // E -> ·i(E)
    // E -> ·E+i
    { { 5, 'i' }, "s3" },
    { { 5, 'E' }, "7" },

    // 状态6
    // E -> E+i·
    { { 6, '(' }, "r4" },
    { { 6, ')' }, "r4" },
    { { 6, '+' }, "r4" },
    { { 6, 'i' }, "r4" },
    { { 6, '$' }, "r4" },

    // 状态7
    // E -> i(E·)
    // E -> E·+i
    { { 7, ')' }, "s8" },
    { { 7, '+' }, "s4" },

    // 状态8
    // E -> i(E)·
    { { 8, '(' }, "r3" },
    { { 8, ')' }, "r3" },
    { { 8, 'i' }, "r3" },
    { { 8, '+' }, "r3" },
    { { 8, '$' }, "r3" },
};

int main()
{
    string str;
    cin >> str;
    // 将字符串中的id替换成i
    for (int i = 0; i < str.length(); i++)
        if (str[i] == 'i' && str[i + 1] == 'd') {
            str[i] = 'i';
            str.erase(i + 1, 1);
        }
    str += "$";

    // <字符, 状态号> 解析栈
    stack<pair<char, int>> Stack;
    Stack.push({ '*', 0 });

    try {
        for (char c : str) {
        PARSE:
            state = Stack.top().second;         // 获取当前状态
            action = table.at({ state, c });    // 获取对应动作

            switch (action[0]) {
                case 's': // 移入字符与新状态
                    Stack.push({ c, stoi(action.substr(1)) });
                    continue;

                case 'r': // 进行规约
                    prod = prods.at(stoi(action.substr(1)));

                    // 弹出栈顶处的产生式右侧元素
                    for (int i = 0; i < prod.second.length(); i++)
                        Stack.pop();

                    // 压入规约后的新元素, 并更新状态
                    curState = Stack.top().second;
                    newState = stoi(table.at({ curState, prod.first[0] }));
                    Stack.push({ prod.first[0], newState });

                    // 重新解析
                    goto PARSE;

                case '$':
                    cout << "accept\n";
                    exit(0);
            }
        }
    } catch (...) {
        cout << "reject\n";
        exit(0);
    }
}