#pragma once
#include <map>
#include <queue>
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

std::tuple<std::vector<FuncInfo>, std::vector<FuncBody>> findFunctions(const std::string& filename_cpp);
std::tuple<std::vector<FuncInfo>, std::vector<FuncBody>> findFunctions2(const std::string& filename_cpp);
std::tuple<std::vector<FuncInfo>, std::vector<FuncBody>> findFunctions3(const std::string& filename_cpp);
}    //namespace Struct2xxx