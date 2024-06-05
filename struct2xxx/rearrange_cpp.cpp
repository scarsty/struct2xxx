#include "Struct2xxx.h"
#include "filefunc.h"
#include "strfunc.h"
#include <Windows.h>
#include <algorithm>
#include <iostream>
#include <map>
#include <print>
#include <sstream>
#include <string>
#include <vector>

struct FuncInfo
{
    std::string name;
    std::string type;
    std::string args;
    std::string ret;
    std::string body;
    int64_t index = 0;
};

struct FuncBody
{
    std::string name;
    std::string body;
    int line0, line1;
};

struct ClassInfo
{
    std::string name;
    size_t pos = 0;
    int64_t id;
};

std::vector<FuncInfo> funcInfos;
std::vector<FuncBody> funcBodies;
std::map<std::string, int> funcInfoIndex;
std::map<int64_t, ClassInfo> ClassInfoIndex;

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

    std::string cmd = "copy " + filename_cpp + " temp.cpp";
    system(cmd.c_str());
    cmd = "clang -cc1 -ast-dump temp.cpp > temp.ast";

    system(cmd.c_str());

    Sleep(1000);
    //std::string text = buffer.str();
    std::string ast_content = strfunc::readStringFromFile("temp.ast");
    cmd = "del temp.cpp temp.ast";
    //system(cmd.c_str());
    auto lines_ast = strfunc::splitString(ast_content, "\n", false);
    int index = 0;
    std::vector<ClassInfo> current_record;
    //current_record.push_back({});
    //pos_class = std::string::npos;
    for (auto l : lines_ast)
    {
        auto ps = strfunc::splitString(l, " ");
        auto pos_class0 = l.find("CXXRecordDecl");
        if (pos_class0 != std::string::npos)
        {
            auto pos_p = l.find("definition");
            if (pos_p != std::string::npos)
            {
                auto pos_c2 = l.rfind(" ", pos_p);
                auto pos_c1 = l.rfind(" ", pos_c2 - 1);
                auto name = l.substr(pos_c1 + 1, pos_c2 - pos_c1 - 1);
                int64_t id = 0;
                for (auto p : ps)
                {
                    if (p.find("0x") == 0)
                    {
                        id = std::stoll(p.substr(2), nullptr, 16);
                    }
                }
                current_record.push_back({ name, pos_class0, id });
                ClassInfoIndex[id] = current_record.back();
            }
        }
        pos_class0 = l.find("NamespaceDecl");
        if (pos_class0 != std::string::npos)
        {
            auto name = ps.back();
            int64_t id = 0;
            for (auto p : ps)
            {
                if (p.find("0x") == 0)
                {
                    id = std::stoll(p.substr(2), nullptr, 16);
                }
            }
            current_record.push_back({ name, pos_class0, id });
            ClassInfoIndex[id] = current_record.back();
        }

        if (current_record.size() != 0)
        {
            if (l.find("`", current_record.back().pos) == current_record.back().pos)
            {
                current_record.pop_back();
            }
        }

        if (l.find("CXXMethodDecl") != std::string::npos
            || l.find("CXXConstructorDecl") != std::string::npos
            || l.find("CXXDestructorDecl") != std::string::npos
            || l.find("FunctionDecl")!=std::string::npos)
        {
            if (l.find("parent") != std::string::npos)
            {
                //continue;
            }
            std::string class_name;
            for (int i = 0; i < ps.size(); i++)
            {
                if (ps[i] == "parent")
                {
                    auto id = std::stoll(ps[i + 1].substr(2), nullptr, 16);
                    class_name = ClassInfoIndex[id].name;
                }
            }
            //std::println("{}", l);
            auto pos_p = l.find("'");
            auto pos_c2 = l.rfind(" ", pos_p);
            auto pos_c1 = l.rfind(" ", pos_c2 - 1);
            auto pos_c3 = l.find("'", pos_p + 1);
            auto name = l.substr(pos_c1 + 1, pos_c3 - pos_c1);
            //std::println("{}", name);
            if (current_record.size() != 0)
            {
                name = current_record.back().name + "::" + name;
            }
            if (class_name != "")    //类名非首次出现，是一个实现
            {
                name = class_name + "::" + name;
                int line0 = 0, line1 = 0;
                for (auto p : ps)
                {
                    if (p.find("<") == 0 && p.find(":") != std::string::npos)
                    {
                        line0 = std::stoi(p.substr(p.find(":") + 1));
                    }
                    else if (p.find("line:") == 0 && p.find(">") != std::string::npos)
                    {
                        line1 = std::stoi(p.substr(5));
                    }
                }
                FuncBody fb;
                fb.name = name;
                fb.line0 = line0;
                fb.line1 = line1;
                funcBodies.push_back(fb);
            }
            if (!funcInfoIndex.count(name))    //全名首次出现，认为是一个声明
            {
                FuncInfo fi;
                fi.name = name;
                funcInfos.push_back(fi);
                funcInfoIndex[fi.name] = index++;
            }
        }
    }

    std::print("Found functions:\n");
    for (auto& fi : funcInfos)
    {
        std::print("  {}\n", fi.name);
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
                return funcInfoIndex[a.name] < funcInfoIndex[b.name];
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