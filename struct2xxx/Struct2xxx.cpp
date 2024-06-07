#include "Struct2xxx.h"

#include <functional>

#include "strfunc.h"

#include <clang-c/Index.h>
#include <iostream>
#include <print>

#include "filefunc.h"

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
        if (l.contains("CXXMethodDecl")
            || l.contains("CXXConstructorDecl")
            || l.contains("CXXDestructorDecl")
            || l.contains("FunctionDecl"))
        {
            if (l.contains("parent"))
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
                    if (p.find("<") == 0 && p.contains(":"))
                    {
                        line0 = std::stoi(p.substr(p.find(":") + 1));
                    }
                    else if (p.find("line:") == 0 && p.contains(">"))
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

enum class NodeKind    //simplied from CXCursorKind
{
    Unknown,
    Namespace,
    Struct,
    Member,
    Function,
};

struct NodeInfo
{
    NodeKind kind = NodeKind::Unknown;
    CXCursorKind cursorKind = CXCursorKind::CXCursor_UnexposedDecl;
    std::string name;
    CX_CXXAccessSpecifier accessed = CX_CXXInvalidAccessSpecifier;
    std::map<int, NodeInfo> children;
    std::string filename;
    int64_t id = 0;
    size_t pos0 = 0, pos1 = 0;
};

struct UserData
{
    NodeInfo root;
    std::map<int64_t, NodeInfo*> id2node;
};

CXChildVisitResult visit(CXCursor cursor, CXCursor parent, CXClientData data)
{
    auto& userData = *static_cast<UserData*>(data);
    auto& root = userData.root;
    auto& id2node = userData.id2node;

    CXCursorKind kind = clang_getCursorKind(cursor);
    NodeInfo node;
    node.cursorKind = kind;
    auto cursorName = clang_getCursorDisplayName(cursor);
    auto cursorNameString = std::string(clang_getCString(cursorName));
    node.name = cursorNameString;

    node.accessed = clang_getCXXAccessSpecifier(cursor);

    CXSourceRange range = clang_getCursorExtent(cursor);
    // Grab the start of the range.
    CXSourceLocation location0 = clang_getRangeStart(range);
    CXFile file;
    unsigned int line;
    unsigned int column;
    clang_getFileLocation(location0, &file, &line, &column, nullptr);
    node.pos0 = line;
    //end of the range
    CXSourceLocation location1 = clang_getRangeEnd(range);
    clang_getFileLocation(location1, &file, &line, &column, nullptr);
    node.pos1 = line;
    // Get the name of the file.
    auto fileName = clang_getFileName(file);

    if (clang_getCString(fileName))
    {
        node.filename = std::string(clang_getCString(fileName));
    }

    auto id = (int64_t)cursor.data[0];
    node.id = id;

    if (kind == CXCursorKind::CXCursor_Namespace)
    {
        node.kind = NodeKind::Namespace;
    }
    else if (kind == CXCursorKind::CXCursor_ClassDecl
        || kind == CXCursorKind::CXCursor_StructDecl)
    {
        node.kind = NodeKind::Struct;
    }
    else if (kind == CXCursorKind::CXCursor_FunctionDecl
        || kind == CXCursorKind::CXCursor_CXXMethod
        || kind == CXCursorKind::CXCursor_Constructor
        || kind == CXCursorKind::CXCursor_Destructor)
    {
        node.kind = NodeKind::Function;
    }
    else if (kind == CXCursorKind::CXCursor_FieldDecl)
    {
        node.kind = NodeKind::Member;
    }

    auto c = clang_getCursorSemanticParent(cursor);
    parent = c;

    CXCursorKind pkind = clang_getCursorKind(c);

    clang_disposeString(fileName);
    clang_disposeString(cursorName);

    {
        auto id_parent = (int64_t)parent.data[0];
        NodeInfo* nodePtr = nullptr;
        if (id2node.contains(id_parent))
        {
            auto& p = *id2node[id_parent];
            p.children[p.children.size()] = node;
            nodePtr = &p.children[p.children.size() - 1];
        }
        else
        {
            root.children[root.children.size()] = node;
            nodePtr = &root.children[root.children.size() - 1];
        }
        id2node[id] = nodePtr;
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

    const char* argv[] = { "clang", "-Xclang", "-ast-dump", filename_cpp.c_str() };
    int argc = 4;
    CXTranslationUnit TU = clang_parseTranslationUnit(
        Index,
        filename_cpp.c_str(),
        argv + 1,
        argc - 2,
        nullptr,
        0,
        0);

    UserData data;
    CXCursor cursor = clang_getTranslationUnitCursor(TU);
    clang_visitChildren(cursor, visit, &data);

    // RAII?
    clang_disposeTranslationUnit(TU);
    clang_disposeIndex(Index);

    std::vector<FuncInfo> funcInfos;
    std::vector<FuncBody> funcBodies;
    std::map<std::string, int> funcInfoIndex;

    std::vector<std::string> current_record;

    auto make_name = [&]()
    {
        std::string name;
        for (auto& c : current_record)
        {
            if (c != "")
            {
                if (c.contains(" "))
                {
                    name += c;
                }
                else
                {
                    name += "::" + c;
                }
            }
        }
        return name;
    };
    std::function<void(NodeInfo&)> check_node = [&](NodeInfo& node)
    {
        if (node.kind == NodeKind::Function)
        {
            auto name = make_name();
            name += "::" + node.name;
            if (node.filename == filename_cpp)
            {
                FuncBody fb;
                fb.name = name;
                fb.line0 = node.pos0;
                fb.line1 = node.pos1;
                fb.index = funcInfoIndex[fb.name];
                funcBodies.push_back(fb);
            }
            else
            {
                FuncInfo fi;
                fi.name = name;
                funcInfos.push_back(fi);
                funcInfoIndex[fi.name] = node.pos0;
            }
            //std::print("::{} : {} [{},{}] {}\n",
            //    node.name, filefunc::getFileExt(node.filename), node.pos0, node.pos1, node.children.size());
        }
        if (node.kind == NodeKind::Member)
        {
            auto name = make_name();
            name += "::" + node.name;
            //std::print("{} : {} [{},{}] {}\n",
            //    name, filefunc::getFileExt(node.filename), node.pos0, node.pos1, node.children.size());
        }

        current_record.push_back(node.name);
        for (auto& [key, value] : node.children)
        {
            check_node(value);
        }
        current_record.pop_back();
    };

    check_node(data.root);

    return { funcInfos, funcBodies };
}
