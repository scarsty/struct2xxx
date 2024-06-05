#include "Struct2xxx.h"
#include "Cppfa.h"
#include "filefunc.h"
#include "strfunc.h"
#include <Windows.h>
#include <algorithm>
#include <print>

struct FuncInfo
{
    std::string name;
    std::string type;
    std::string args;
    std::string ret;
    std::string body;
    int index = 0;
};

struct FuncBody
{
    std::string name;
    std::string body;
};

std::vector<FuncInfo> funcInfos;
std::vector<FuncBody> funcBodies;
std::map<std::string, int> funcInfoIndex;

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

    std::print("Processing {} and {}\n", filename_h, filename_cpp);
    //strfunc::writeStringToFile(cpp_content, filename+std::to_string(time(nullptr)) + ".cpp");
    //MessageBoxA(NULL, filename.c_str(), "cccc", MB_ICONERROR);

    cppfa::Cppfa cf;

    cf.analyze(h_content);




























    return 0;
    //找出所有定义的函数名
    strfunc::replaceAllSubStringRef(h_content, "\r", "");
    //strfunc::replaceAllSubStringRef(h_content, "\n", "");
    auto h_lines = strfunc::splitString(h_content, "\n", false);
    for (auto l : h_lines)
    {
        if (l.find("(") > l.find("=")
            || l.find("(") > l.find("#")
            || l.find("(") > l.find("//"))
        {
            continue;
        }
        auto parts = strfunc::splitString(l, " ");
        if (l.find("~") != std::string::npos)
        {
            int c = 1;
        }
        if (parts.size() > 0 && parts[0].find("~") == 0 && parts[0].find("(") != std::string::npos)
        {
            FuncInfo fi;
            fi.name = parts[0].substr(1, parts[0].find("(") - 1);
            if (!fi.name.empty())
            {
                funcInfos.push_back(fi);
                fi.name = "~" + fi.name;
                funcInfos.push_back(fi);
            }
        }

        for (int i = 1; i < parts.size(); i++)
        {
            auto str = parts[i];
            if (str.find("(") != std::string::npos
                && parts[i - 1].find_first_of("+=/.?") == std::string::npos
                && parts[i - 1] != "return")
            {
                FuncInfo fi;
                fi.name = str.substr(0, str.find("("));
                if (!fi.name.empty())
                {
                    //fi.args = str.substr(str.find("(") + 1, str.find(")") - str.find("(") - 1);
                    //fi.type = parts[parts.size() - 2];
                    funcInfos.push_back(fi);
                }
                break;
            }
        }
    }
    std::print("Found functions:\n");
    int index = 0;
    for (auto& fi : funcInfos)
    {
        std::print("  {}\n", fi.name);
        fi.index = index++;
        funcInfoIndex[fi.name] = fi.index;
    }

    //处理换行符
    std::string line_break = "\n";
    if (cpp_content.find("\r\n") != std::string::npos)
    {
        line_break = "\r\n";
    }
    auto new_cpp_content = strfunc::replaceAllSubString(cpp_content, "\r", "");

    //找出函数体
    std::string func_end = "\n}\n";

    int pos0 = 0, pos1 = 0;

    pos1 = new_cpp_content.find(func_end);

    while (pos1 != std::string::npos)
    {
        FuncBody fb;
        fb.body = new_cpp_content.substr(pos0, pos1 - pos0 + 3);
        pos0 = pos1 + 3;
        pos1 = new_cpp_content.find(func_end, pos0);
        funcBodies.push_back(fb);
    }
    std::string tail = new_cpp_content.substr(pos0);

    for (auto& fb : funcBodies)
    {
        auto pos = fb.body.size();
        for (auto& fi : funcInfos)
        {
            if (fi.name == "eval")
            {
                int d = 1;
            }
            auto posnew = min(fb.body.find(" " + fi.name + "("), fb.body.find("::" + fi.name + "("));
            if (posnew < pos)
            {
                pos = posnew;
                fb.name = fi.name;
            }
        }
    }

    for (int i = 1; i < funcBodies.size(); i++)
    {
        if (funcBodies[i].name.empty())
        {
            funcBodies[i].name = funcBodies[i - 1].name;
        }
    }

    //排序
    if (funcBodies.size() > 1)
    {
        std::sort(funcBodies.begin() + 1, funcBodies.end(), [&](const FuncBody& a, const FuncBody& b)
            {
                return funcInfoIndex[a.name] < funcInfoIndex[b.name];
            });
    }
    //生成新的cpp文件
    new_cpp_content.clear();
    for (auto& fb : funcBodies)
    {
        new_cpp_content += fb.body;
    }
    new_cpp_content += tail;
    strfunc::replaceAllSubStringRef(new_cpp_content, "\n", line_break);
    std::print("Found {} function declares\n", funcInfos.size());
    std::print("Found {} function bodies\n", funcBodies.size());
    if (new_cpp_content != cpp_content)
    {
        CopyFileA(filename_cpp.c_str(), (filename + std::to_string(time(nullptr)) + ".cpp").c_str(), FALSE);
        strfunc::writeStringToFile(new_cpp_content, filename_cpp);
        std::print("Change have saved\n");
    }
    else
    {
        std::print("No change\n");
    }

    return 0;
}