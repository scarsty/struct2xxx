#include "Struct2xxx.h"
#include "filefunc.h"
#include "nlohmann/json.hpp"
#include "strfunc.h"
#include <Windows.h>
#include <algorithm>
#include <iostream>
#include <map>
#include <print>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::print("Usage: {} <filename>\n", argv[0]);
        return 1;
    }

    std::string filename = argv[1];
    filename = filefunc::changeFileExt(filename, "");
    std::string filename_h = filename + ".h";
    std::string filename_cpp = filename + ".cpp";
    std::string h_content = strfunc::readStringFromFile(filename_h);
    std::string cpp_content = strfunc::readStringFromFile(filename_cpp);
    filefunc::changePath(filefunc::getFilePath(filename_cpp));

    std::print("Processing {} and {}\n", filename_h, filename_cpp);

    //auto [funcInfos, funcBodies] = findFunctions(filename_cpp);
    auto [funcInfos, funcBodies] = findFunctions2(filename_cpp);
    std::print("Found function bodies:\n");
    for (auto& fb : funcBodies)
    {
        std::print("  {} : {}\n", fb.name, fb.index);
    }

    //处理换行符
    std::string line_break = "\n";
    if (cpp_content.find("\r\n") != std::string::npos)
    {
        line_break = "\r\n";
    }
    auto new_cpp_content = strfunc::replaceAllSubString(cpp_content, "\r", "");
    auto lines_cpp = strfunc::splitString(new_cpp_content, "\n", false);
    std::vector<FuncBody*> lines_below(lines_cpp.size());
    for (auto& fb : funcBodies)
    {
        for (int i = fb.line0 - 1; i <= fb.line1 - 1; i++)
        {
            fb.body += lines_cpp[i] + "\n";
            lines_below[i] = &fb;
        }
    }
    std::string head;
    FuncBody fb_head, fb_tail;
    //前面的行认为是头
    for (int i = 0; i < lines_cpp.size(); i++)
    {
        if (lines_below[i] == nullptr)
        {
            lines_below[i] = &fb_head;
            fb_head.body += lines_cpp[i] + "\n";
        }
        else
        {
            break;
        }
    }
    //中间的行归属其后的，后面归为尾
    FuncBody* last = &fb_tail;
    for (int i = lines_cpp.size() - 1; i != 0; i--)
    {
        if (lines_below[i] == nullptr)
        {
            lines_below[i] = last;
            if (last)
            {
                last->body = lines_cpp[i] + "\n" + last->body;
            }
        }
        else
        {
            last = lines_below[i];
        }
    }
    if (!fb_tail.body.empty() && fb_tail.body.back() == '\n')
    {
        fb_tail.body.pop_back();
    }
    //排序
    if (funcBodies.size() > 1)
    {
        std::sort(funcBodies.begin(), funcBodies.end(), [&](const FuncBody& a, const FuncBody& b)
            {
                return a.index < b.index;
            });
    }
    //生成新的cpp文件
    new_cpp_content.clear();
    new_cpp_content += fb_head.body;
    for (auto& fb : funcBodies)
    {
        new_cpp_content += fb.body;
    }
    new_cpp_content += fb_tail.body;
    strfunc::replaceAllSubStringRef(new_cpp_content, "\n", line_break);
    std::print("Found {} function declares\n", funcInfos.size());
    std::print("Found {} function bodies\n", funcBodies.size());
    if (new_cpp_content != cpp_content)
    {
        //CopyFileA(filename_cpp.c_str(), (filename + std::to_string(time(nullptr)) + ".cpp").c_str(), FALSE);
        //strfunc::writeStringToFile(new_cpp_content, filename_cpp);
        std::print("Change have saved\n");
    }
    else
    {
        std::print("No change\n");
    }
    std::print("End\n");
    return 0;
}

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