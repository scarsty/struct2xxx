#include "Struct2xxx.h"
#include "filefunc.h"

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

using namespace Struct2xxx;

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

    std::print("Processing {}\n", filename_cpp);

    auto [funcInfos, funcBodies] = findFunctions3(filename_cpp,
        { "--std=c++23",
            "-I../../mlcc",
            "-I../include",
            R"(-IC:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.5\include)",
            "-I../cccc-cuda" });

    std::print("Found function bodies:\n");
    for (auto& fb : funcBodies)
    {
        std::print("{} : {} [{}, {}]\n", fb.name, fb.index, fb.line0, fb.line1);
    }

    //处理换行符
    std::string line_break = "\n";
    if (cpp_content.contains("\r\n"))
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

void rest(std::vector<int>)
{
}