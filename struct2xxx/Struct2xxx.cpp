#include "Struct2xxx.h"
#include "strfunc.h"

#include <clang-c/Index.h>
#include <iostream>


std::tuple<std::vector<FuncInfo>, std::vector<FuncBody>> findFunctions(const std::string& filename_cpp)
{
    std::vector<FuncInfo> funcInfos;
    std::vector<FuncBody> funcBodies;
    std::map<std::string, int> funcInfoIndex;
    std::map<int64_t, ClassInfo> ClassInfoIndex;

    std::string cmd = "clang -cc1 -ast-dump " + filename_cpp;
    std::string ast_content = strfunc::get_cmd_output(cmd);
    auto lines_ast = strfunc::splitString(ast_content, "\n", false);
    int index = 0;
    std::vector<ClassInfo> current_record;
    int i_line = 0;
    for (auto l : lines_ast)
    {
        i_line++;
        auto ps = strfunc::splitString(l, " ");
        if (current_record.size() != 0)
        {
            if (l.rfind("|-") <= current_record.back().pos)
            {
                current_record.resize(l.rfind("|-") / 2);
            }
        }
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
                        break;
                    }
                }
                current_record.push_back({ name, pos_class0, id, 1 });
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
                    break;
                }
            }
            current_record.push_back({ name, pos_class0, id, 0 });
            ClassInfoIndex[id] = current_record.back();
        }
        if (l.find("CXXMethodDecl") != std::string::npos
            || l.find("CXXConstructorDecl") != std::string::npos
            || l.find("CXXDestructorDecl") != std::string::npos
            || l.find("FunctionDecl") != std::string::npos)
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
            std::string prefix = "";
            for (auto& c : current_record)
            {
                prefix += "::" + c.name;
            }
            prefix += "::";
            if (class_name != "")    //类名非首次出现，是一个实现
            {
                name = prefix + class_name + "::" + name;
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
                fb.index = funcInfoIndex[fb.name];
                funcBodies.push_back(fb);
            }
            else
            {
                name = prefix + name;
                if (!funcInfoIndex.contains(name))    //全名首次出现，认为是一个声明
                {
                    FuncInfo fi;
                    fi.name = name;
                    funcInfos.push_back(fi);
                    funcInfoIndex[fi.name] = index++;
                }
            }
        }
    }

    return { funcInfos, funcBodies };
}



CXChildVisitResult visit(CXCursor cursor, CXCursor parent, CXClientData data)
{
    CXCursorKind kind = clang_getCursorKind(cursor);

    if (kind == CXCursorKind::CXCursor_FunctionDecl || kind == CXCursorKind::CXCursor_CXXMethod)
    {
        // The display name is sometimes more descriptive than the spelling name
        // (which is just the source code).
        auto cursorName = clang_getCursorDisplayName(cursor);

        auto cursorNameString = std::string(clang_getCString(cursorName));
        //if (cursorNameString.find("foo") != std::string::npos)
        {
            // Grab the source range, i.e. (start, end) SourceLocation pair.
            CXSourceRange range = clang_getCursorExtent(cursor);

            // Grab the start of the range.
            CXSourceLocation location = clang_getRangeStart(range);

            // Decompose the SourceLocation into a location in a file.
            CXFile file;
            unsigned int line;
            unsigned int column;
            clang_getFileLocation(location, &file, &line, &column, nullptr);

            // Get the name of the file.
            auto fileName = clang_getFileName(file);

            std::cout << "Found function " << cursorNameString
                      << " in " << clang_getCString(fileName)
                      << " at " << line
                      << ":" << column
                      << std::endl;

            // Manual cleanup!
            clang_disposeString(fileName);
        }

        // Manual cleanup!
        clang_disposeString(cursorName);
    }

    return CXChildVisit_Recurse;
}

std::tuple<std::vector<FuncInfo>, std::vector<FuncBody>> findFunctions2(const std::string& filename_cpp)
{



     // excludeDeclsFromPCH = 1, displayDiagnostics = 1
    CXIndex Index = clang_createIndex(1, 1);

    // Expected arguments:
    // 1) The index to add the translation unit to,
    // 2) The name of the file to parse,
    // 3) A pointer to strings of further command line arguments to compile,
    // 4) The number of further arguments to compile,
    // 5) A pointer to a an array of CXUnsavedFiles structs,
    // 6) The number of CXUnsavedFiles (buffers of unsaved files) in the array,
    // 7) A bitmask of options.


    const char* argv[] = {"clang", "-Xclang", "-ast-dump", filename_cpp.c_str() };
    int argc = 4;
    CXTranslationUnit TU = clang_parseTranslationUnit(
        Index,
        filename_cpp.c_str(),
        argv+1,
        argc-2,
        nullptr,
        0,
        CXTranslationUnit_SkipFunctionBodies);


    CXCursor cursor = clang_getTranslationUnitCursor(TU);
    clang_visitChildren(cursor, visit, nullptr);

    // RAII?
    clang_disposeTranslationUnit(TU);
    clang_disposeIndex(Index);


    std::vector<FuncInfo> funcInfos;
    std::vector<FuncBody> funcBodies;




        return { funcInfos, funcBodies };
}
