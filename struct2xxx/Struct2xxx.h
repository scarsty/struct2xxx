#pragma once
#include <cstdint>
#include <string>
#include <tuple>
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

std::tuple<std::vector<FuncInfo>, std::vector<FuncBody>> find_functions(const std::string& filename_cpp);
std::tuple<std::vector<FuncInfo>, std::vector<FuncBody>> find_functions2(const std::string& filename_cpp, const std::vector<std::string>& args);
std::tuple<std::vector<FuncInfo>, std::vector<FuncBody>> find_functions3(const std::string& filename_cpp, const std::vector<std::string>& args);

//修正后的类名，模板参数，成员变量列表
std::tuple<std::string, std::string, std::vector<MemberInfo>> find_members(const std::string& filename_cpp, const std::string& class_name, const std::vector<std::string>& args);

}    //namespace Struct2xxx