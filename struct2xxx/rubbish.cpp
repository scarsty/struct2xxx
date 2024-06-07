#include "Struct2xxx.h"
#include "nlohmann/json.hpp"
#include <print>

using namespace Struct2xxx;

void ast_json_111()
{
    nlohmann::json j;
    //j = nlohmann::json::parse(strfunc::readStringFromFile("temp.ast"));
    std::vector<ClassInfo> current_record;
    std::function<void(nlohmann::json&)> get_funcs = [&get_funcs, &current_record](nlohmann::json& j)
    {
        bool has = false;
        if (j["kind"].is_string())
        {
            auto kind = j["kind"].get<std::string>();
            if (kind == "CXXMethodDecl"
                || kind == "CXXConstructorDecl"
                || kind == "CXXDestructorDecl"
                || kind == "FunctionDecl")
            {
                std::string prefix = "";
                for (auto& c : current_record)
                {
                    prefix += "::" + c.name;
                }
                prefix += "::";
                std::print("{}{} {}\n", prefix, j["name"].get<std::string>(), j["type"]["qualType"].get<std::string>());
            }
            if (kind == "NamespaceDecl")
            {
                //std::print("{}\n", j["name"].get<std::string>());
                current_record.push_back({ j["name"].get<std::string>(), 0, 0, 0 });
                has = true;
            }
            if (kind == "CXXRecordDecl")
            {
                //std::print("{}\n", j["name"].get<std::string>());
                current_record.push_back({ j["name"].get<std::string>(), 0, 0, 1 });
                has = true;
            }
        }
        if (j["inner"].is_array())
        {
            for (auto& j1 : j["inner"])
            {
                get_funcs(j1);
            }
        }
        if (has && current_record.size() != 0)
        {
            current_record.pop_back();
        }
    };
    get_funcs(j);
}