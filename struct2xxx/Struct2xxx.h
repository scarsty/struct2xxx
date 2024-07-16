#pragma once
#include <map>
#include <string>
#include <vector>

namespace Struct2xxx
{

struct FuncInfo
{
    std::string name;
    int64_t index = 0;
};

struct FuncBody
{
    std::string name;
    std::string body;
    int line0, line1;
    int64_t index = 0;
};

struct ClassInfo
{
    std::string name;
    size_t pos = 0;
    int64_t id;
    int is_class = 0;
};

struct MemberInfo
{
    std::string name;
    std::string full_name;
    std::string type;
    int64_t id;
    int is_member = 0;
    int is_public = 1;
    int is_const = 0;
};

//使用命令行工具clang来解析cpp文件，找到其中的函数
std::tuple<std::vector<FuncInfo>, std::vector<FuncBody>> find_functions(const std::string& filename_cpp);
//使用clang库的C接口来解析cpp文件，找到其中的函数
std::tuple<std::vector<FuncInfo>, std::vector<FuncBody>> find_functions2(const std::string& filename_cpp, const std::vector<std::string>& args);
//使用clang库的C++接口来解析cpp文件，找到其中的函数
std::tuple<std::vector<FuncInfo>, std::vector<FuncBody>> find_functions3(const std::string& filename_cpp, const std::vector<std::string>& args);
//查找一个类中的成员，生成相关的转换代码
std::tuple<std::string, std::string, std::vector<MemberInfo>> find_members(const std::string& filename_cpp, const std::string& class_name = "", const std::vector<std::string>& args = {});
//生成一个枚举类型的值和字符串的映射
std::map<int, std::string> make_enum_map(const std::string& filename_cpp, const std::string& enum_name, const std::vector<std::string>& args = {});

}    //namespace Struct2xxx