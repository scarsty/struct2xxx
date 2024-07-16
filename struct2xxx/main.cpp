#include "Struct2xxx.h"
#include "filefunc.h"
#include "strfunc.h"
#include <algorithm>
#include <print>

using namespace Struct2xxx;

void rearrange_cpp(const std::string& filename_cpp)
{
    //读入的文件认为是cpp文件
    auto filename = filefunc::getFileMainName(filename_cpp);
    std::string filename_h = filename + ".h";
    std::string h_content = filefunc::readFileToString(filename_h);
    std::string cpp_content = filefunc::readFileToString(filename_cpp);
    filefunc::changePath(filefunc::getFilePath(filename_cpp));

    std::print("Processing {}\n", filename_cpp);

    auto [func_infos, func_bodies] = find_functions2(filename_cpp,
        { "--std=c++23",
            "--language=c++",
            "-I../../mlcc",
            "-I../include",
            R"(-IC:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.5\include)",
            "-I../cccc-cuda" });

    std::print("Found function bodies:\n");
    for (auto& fb : func_bodies)
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
    for (auto& fb : func_bodies)
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
    if (func_bodies.size() > 1)
    {
        std::sort(func_bodies.begin(), func_bodies.end(), [&](const FuncBody& a, const FuncBody& b)
            {
                return a.index < b.index;
            });
    }
    //生成新的cpp文件
    new_cpp_content.clear();
    new_cpp_content += fb_head.body;
    for (auto& fb : func_bodies)
    {
        new_cpp_content += fb.body;
    }
    new_cpp_content += fb_tail.body;
    strfunc::replaceAllSubStringRef(new_cpp_content, "\n", line_break);
    std::print("Found {} function declares\n", func_infos.size());
    std::print("Found {} function bodies\n", func_bodies.size());
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
}

void make_xxx(const std::string& filename_cpp, const std::string& class_name = "", const std::string xxx = "XXX", const std::string& out_filename = "")
{
    auto filename = filefunc::getFileMainName(filename_cpp);
    std::string filename_h = filename + ".h";
    std::string h_content = filefunc::readFileToString(filename_h);
    std::string cpp_content = filefunc::readFileToString(filename_cpp);
    filefunc::changePath(filefunc::getFilePath(filename_cpp));

    std::string c_name = class_name;
    if (c_name.empty())
    {
        c_name = filefunc::getFileMainNameWithoutPath(filename_cpp);
    }

    std::print("Processing {}\n", filename_cpp);

    auto [c_name1, template_str, member_infos] = find_members(filename_cpp, c_name,
        { "--std=c++23",
            "--language=c++",
            "-I../../local/include1" });

    std::print("Found {} members:\n", member_infos.size());
    for (auto& mi : member_infos)
    {
        std::print("{} : {} (public: {}, const: {})\n", mi.full_name, mi.type, mi.is_public, mi.is_const);
    }
    std::print("\n");
    std::print("Make codes like this:\n\n");

    std::string str;
    if (template_str != "") { template_str += "\n"; }
    str += std::format("{}void {}2{}(const {}& v, {}& x)\n{{\n", template_str, c_name, xxx, c_name1, xxx);
    for (auto& mi : member_infos)
    {
        str += std::format("    x[\"{}\"] = v.{};\n", mi.name, mi.name);
    }
    str += std::format("}}\n\n");

    str += std::format("{}void {}2{}(const {}& x, {}& v)\n{{\n", template_str, xxx, c_name, xxx, c_name1);
    for (auto& mi : member_infos)
    {
        if (mi.is_public && !mi.is_const)
        {
            str += std::format("    v.{} = x[\"{}\"];\n", mi.name, mi.name);
        }
    }
    str += std::format("}}\n\n");
    std::print("{}\n", str);

    //若没有变动，则不保存
    if (out_filename != "")
    {
        auto str0 = filefunc::readFileToString(out_filename);
        if (str0 != str)
        {
            filefunc::writeStringToFile(str, out_filename);
            std::print("Change have saved in {}\n", out_filename);
        }
        else
        {
            std::print("No change\n");
        }
    }
}

void make_enum_xxx(const std::string& filename_cpp, const std::string& enum_name = "", const std::string& out_filename = "")
{
    auto filename = filefunc::getFileMainName(filename_cpp);
    std::string filename_h = filename + ".h";
    std::string h_content = filefunc::readFileToString(filename_h);
    std::string cpp_content = filefunc::readFileToString(filename_cpp);
    filefunc::changePath(filefunc::getFilePath(filename_cpp));

    std::print("Processing {}\n", filename_cpp);

    auto m = make_enum_map(filename_cpp, enum_name,
        { "--std=c++23",
            "--language=c++",
            "-I../../local/include1" });

    std::print("Found enum:\n");
    for (auto& [k, v] : m)
    {
        std::print("{} : {}\n", k, v);
    }
    std::print("\n");
    std::print("Make codes like this:\n\n");

    std::string str;
    str += std::format("std::string {}2string({} value)\n{{\n", enum_name, enum_name);
    str += "    switch (value)\n    {\n";
    for (auto& [k, v] : m)
    {
        str += std::format("    case {}:\n        return \"{}\";\n", v, v);
    }
    str += "    default:\n        return \"\";\n    }\n}\n\n";
    std::print("{}\n", str);

    //若没有变动，则不保存
    if (out_filename != "")
    {
        auto str0 = filefunc::readFileToString(out_filename);
        if (str0 != str)
        {
            filefunc::writeStringToFile(str, out_filename);
            std::print("Change have saved in {}\n", out_filename);
        }
        else
        {
            std::print("No change\n");
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::print("Usage: {} <filename>\n", argv[0]);
        return 1;
    }
    make_enum_xxx(argv[1], "LayerConnectionType");
    return 0;
}
