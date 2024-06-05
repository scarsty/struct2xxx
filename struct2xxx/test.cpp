
#include <algorithm>

#include "Struct2xxx.h"
#include "filefunc.h"
#include "fmt1.h"
#include "strfunc.h"
#include <cstdio>
#include <Windows.h>

struct FuncInfo
{
    std::string name;
    std::string type;
    std::string args;
    std::string ret;
    std::string body;
    //bool incpp = false;
    int index = 0;
};

struct FuncBody
{
    std::string name;
    std::string body;
    //bool incpp = false;
};

std::vector<FuncInfo> funcInfos;
std::vector<FuncBody> funcBodies;
std::map<std::string, int> funcInfoIndex;

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }    

    std::string filename = argv[1];
    filename = filefunc::changeFileExt(filename, "");
    std::string filename_h = filename + ".h";
    std::string filename_cpp = filename + ".cpp";
    std::string h_content = strfunc::readStringFromFile(filename_h);
    std::string cpp_content = strfunc::readStringFromFile(filename_cpp);
    CopyFileA(filename_cpp.c_str(), (filename + std::to_string(time(nullptr)) + ".cpp").c_str(), FALSE);
    //strfunc::writeStringToFile(cpp_content, filename+std::to_string(time(nullptr)) + ".cpp");
    MessageBoxA(NULL, filename.c_str(), "cccc", MB_ICONERROR);
    
    //找出所有定义的函数名
    auto h_lines = strfunc::splitString(h_content, "\n", false);
    int h_space = -1;
    for (auto l : h_lines)
    {
        if (l.find("(") > l.find("=")
            || l.find("(") > l.find("#"))
        {
            continue;
        }
        auto parts = strfunc::splitString(l, " ");

        for (auto str : parts)
        {
            if (str.find("(") != std::string::npos)
            {
                if (h_space == -1)
                {
                    h_space = l.find_first_not_of(" ");
                }
                else
                {
                    if (l.find_first_not_of(" ") != h_space)
                    {
                        break;
                    }
                }
                FuncInfo fi;
                fi.name = str.substr(0, str.find("("));
                //fi.args = str.substr(str.find("(") + 1, str.find(")") - str.find("(") - 1);
                //fi.type = parts[parts.size() - 2];
                funcInfos.push_back(fi);
            }
        }
    }
    //fmt1::print("Found functions:\n");
    int index = 0;
    for (auto& fi : funcInfos)
    {
        //fmt1::print("  {}\n", fi.name);
        fi.index = index++;
        funcInfoIndex[fi.name] = fi.index;
    }

    //处理换行符
    std::string line_break = "\n";
    if (cpp_content.find("\r\n") != std::string::npos)
    {
        line_break = "\r\n";
    }
    strfunc::replaceAllSubStringRef(cpp_content, "\r", "");

    //找出函数体
    std::string func_end = "\n}\n";

    int pos0 = 0, pos1 = 0;

    pos1 = cpp_content.find(func_end);

    while (pos1 != std::string::npos)
    {
        FuncBody fb;
        fb.body = cpp_content.substr(pos0, pos1 - pos0 + 3);
        pos0 = pos1 + 3;
        pos1 = cpp_content.find(func_end, pos0);
        funcBodies.push_back(fb);
    }
    std::string tail = cpp_content.substr(pos0);

    for (auto& fb : funcBodies)
    {
        auto pos = fb.body.size();
        for (auto& fi : funcInfos)
        {
            auto posnew = fb.body.find(" " + fi.name + "(");
            if (posnew == std::string::npos)
            {
                posnew = fb.body.find("::" + fi.name + "(");
            }
            if (posnew < pos)
            {
                pos = posnew;
                fb.name = fi.name;
            }
        }
    }

    //排序
    std::sort(funcBodies.begin() + 1, funcBodies.end(), [&](const FuncBody& a, const FuncBody& b)
        {
            return funcInfoIndex[a.name] < funcInfoIndex[b.name];
        });

    //生成新的cpp文件
    std::string new_cpp_content;
    for (auto& fb : funcBodies)
    {
        new_cpp_content += fb.body;
    }
    new_cpp_content += tail;
    strfunc::replaceAllSubStringRef(new_cpp_content, "\n", line_break);
    strfunc::writeStringToFile(new_cpp_content, filename_cpp);
    //fmt1::print("{}", new_cpp_content); 

    return 0;
}