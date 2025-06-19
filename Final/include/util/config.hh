#ifndef __CONFIG_HH__
#define __CONFIG_HH__
#include <map>
#include <stdexcept>

using namespace std;

// 直接使用, 无需实例化
// Compiler_Config::get("key")

static const map<string, int> compiler_config = {
    { "memory_alignment", 4 }, // 内存对齐
    { "address_length", 4 },   // 指针
    { "int_length", 4 },       // 整型
    { "float_length", 4 },     // 单精度浮点
    { "double_length", 8 },    // 双精度浮点
};

class Compiler_Config {
public:
    // Disable copy and assignment
    Compiler_Config() = delete;
    Compiler_Config& operator=(const Compiler_Config&) = delete;

    // 获取配置信息
    static int get(const std::string& key)
    {
        if (compiler_config.find(key) == compiler_config.end()) {
            throw std::runtime_error("Key '" + key + "' not found in configuration");
        } else
            return compiler_config.at(key);
    }
    static void print_config();
};

#endif
