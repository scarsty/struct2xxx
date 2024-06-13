#include "filefunc.h"
#include "strfunc.h"

#include "clang-c/Index.h"

#include "clang/Tooling/Tooling.h"

#include "clang/AST/AST.h"

#include <cstdint>

export module Struct2xxx;

import std;

export namespace Struct2xxx
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

struct MemberInfo
{
    std::string name;
    std::string full_name;
    std::string type;
    int64_t id;
    int is_member = 0;
    int is_public = 1;
    int is_const = 0;
};

std::tuple<std::vector<FuncInfo>, std::vector<FuncBody>> find_functions(const std::string& filename_cpp)
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

std::tuple<std::vector<FuncInfo>, std::vector<FuncBody>> find_functions2(const std::string& filename_cpp, const std::vector<std::string>& args)
{
    CXIndex Index = clang_createIndex(1, 1);

    // Expected arguments:
    // 1) The index to add the translation unit to,
    // 2) The name of the file to parse,
    // 3) A pointer to strings of further command line arguments to compile,
    // 4) The number of further arguments to compile,
    // 5) A pointer to a an array of CXUnsavedFiles structs,
    // 6) The number of CXUnsavedFiles (buffers of unsaved files) in the array,
    // 7) A bitmask of options.

    auto argv = new const char*[args.size() + 3];
    argv[0] = "clang";
    argv[1] = "-cc1";
    argv[2] = "-ast-dump";
    for (int i = 0; i < args.size(); i++)
    {
        argv[i + 3] = args[i].c_str();
    }
    int argc = args.size() + 3;
    CXTranslationUnit TU = clang_parseTranslationUnit(
        Index,
        filename_cpp.c_str(),
        argv + 1,
        argc - 1,
        nullptr,
        0,
        0);

    UserData data;
    CXCursor cursor = clang_getTranslationUnitCursor(TU);
    clang_visitChildren(cursor, visit, &data);

    // RAII?
    clang_disposeTranslationUnit(TU);
    clang_disposeIndex(Index);

    std::vector<FuncInfo> func_infos;
    std::vector<FuncBody> func_bodies;
    std::map<std::string, int> func_info_index;

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
                    name += "::" + c.substr(c.find(" ") + 1);
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
                //fb.index = funcInfoIndex[fb.name];
                func_bodies.push_back(fb);
            }
            else
            {
                FuncInfo fi;
                fi.name = name;
                fi.index = func_infos.size();
                func_info_index[fi.name] = func_infos.size();
                func_infos.push_back(fi);
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
    //修正索引
    for (auto& fb : func_bodies)
    {
        fb.index = func_info_index[fb.name];
    }

    return { func_infos, func_bodies };
}

//得到类型的字符串表示
std::string deal_type(const clang::QualType& t)
{
    //return t.getAsString();
    std::string type_string;
    auto tc = t->getTypeClass();
    if (tc == clang::Type::TypeClass::Pointer)
    {
        auto t2 = t->getPointeeType();
        type_string = deal_type(t2);
        if (type_string.find_last_not_of("*&") == type_string.size() - 1) { type_string += ' '; }
        type_string += "*";
    }
    else if (tc == clang::Type::TypeClass::LValueReference)
    {
        auto t2 = t->getPointeeType();
        type_string = deal_type(t2);
        if (type_string.find_last_not_of("*&") == type_string.size() - 1) { type_string += ' '; }
        type_string += "&";
    }
    else if (tc == clang::Type::TypeClass::RValueReference)
    {
        auto t2 = t->getPointeeType();
        type_string = deal_type(t2);
        if (type_string.find_last_not_of("*&") == type_string.size() - 1) { type_string += ' '; }
        type_string += "&&";
    }
    else if (t.isConstQualified())
    {
        auto t2 = t.getUnqualifiedType();
        type_string = "const " + deal_type(t2);
    }
    else if (tc == clang::Type::TypeClass::Elaborated)
    {
        if (auto t2 = t->getAs<clang::RecordType>())
        {
            type_string = t2->desugar().getAsString();
        }
        else if (auto t2 = t->getAs<clang::TypedefType>())
        {
            type_string = t2->desugar().getAsString();
        }
        else if (auto t2 = t->getAs<clang::TemplateSpecializationType>())
        {
            type_string = t2->desugar().getAsString();
        }
        else if (auto t2 = t->getAs<clang::EnumType>())
        {
            type_string = t2->desugar().getAsString();
        }
        else
        {
            type_string = t.getAsString();
        }
    }
    else if (tc == clang::Type::TypeClass::Builtin)
    {
        type_string = t.getAsString();
    }
    else
    {
        type_string = t.getAsString();
    }
    return type_string;
}

std::string make_para_string(const llvm::MutableArrayRef<clang::ParmVarDecl*>& params)
{
    std::string para_string = "(";
    for (auto& p : params)
    {
        para_string += deal_type(p->getType()) + ",";
    }
    if (params.size() > 0)
    {
        para_string.pop_back();
    }
    para_string += ")";
    return para_string;
}

std::tuple<std::vector<FuncInfo>, std::vector<FuncBody>> find_functions3(const std::string& filename_cpp, const std::vector<std::string>& args)
{
    auto str = filefunc::readFileToString(filename_cpp);
    auto ast = clang::tooling::buildASTFromCodeWithArgs(str, args, filename_cpp);
    auto DC = ast->getASTContext().getTranslationUnitDecl();

    int depth = 0;
    std::vector<FuncInfo> func_infos;
    std::vector<FuncBody> func_bodies;
    std::map<std::string, int> func_info_index;
    std::function<void(clang::Decl*)> check_decl = [&](clang::Decl* decl)
    {
        depth++;
        std::string name1;
        bool is_func = false;
        if (decl->getKind() == clang::Decl::Kind::Namespace)
        {
            auto d1 = static_cast<clang::NamespaceDecl*>(decl);
            name1 = d1->getQualifiedNameAsString();
            for (auto& d : d1->decls())
            {
                check_decl(d);
            }
        }
        else if (decl->getKind() == clang::Decl::Kind::CXXRecord)
        {
            auto d1 = static_cast<clang::CXXRecordDecl*>(decl);
            name1 = d1->getQualifiedNameAsString();
            for (auto& d : d1->decls())
            {
                check_decl(d);
            }
        }
        else if (decl->getKind() == clang::Decl::Kind::CXXMethod)
        {
            auto d1 = static_cast<clang::CXXMethodDecl*>(decl);
            name1 = d1->getQualifiedNameAsString() + make_para_string(d1->parameters());
            is_func = true;
        }
        else if (decl->getKind() == clang::Decl::Kind::CXXConstructor)
        {
            auto d1 = static_cast<clang::CXXConstructorDecl*>(decl);
            name1 = d1->getQualifiedNameAsString() + make_para_string(d1->parameters());
            is_func = true;
        }
        else if (decl->getKind() == clang::Decl::Kind::CXXDestructor)
        {
            auto d1 = static_cast<clang::CXXDestructorDecl*>(decl);
            name1 = d1->getQualifiedNameAsString() + make_para_string(d1->parameters());
            is_func = true;
        }
        else if (decl->getKind() == clang::Decl::Kind::Function)
        {
            auto d1 = static_cast<clang::FunctionDecl*>(decl);
            name1 = d1->getQualifiedNameAsString() + make_para_string(d1->parameters());
            is_func = true;
            /*if (name1.contains("test("))
            {
                for (auto& p : d1->parameters())
                {
                    auto t = p->getType();
                    t->dump();
                }
            }*/
        }
        if (is_func)
        {
            auto loc0 = decl->getBeginLoc();
            auto loc1 = decl->getEndLoc();
            auto& srcmgr = ast->getSourceManager();
            auto filename = srcmgr.getFilename(loc0).str();

            if (filename != filename_cpp)
            {
                FuncInfo fi;
                fi.name = name1;
                fi.index = func_infos.size();
                func_info_index[fi.name] = func_infos.size();
                func_infos.push_back(fi);
            }
            else
            {
                FuncBody fb;
                fb.name = name1;
                fb.line0 = srcmgr.getSpellingLineNumber(loc0);
                fb.line1 = srcmgr.getSpellingLineNumber(loc1);
                //fb.index = funcInfoIndex[fb.name];
                func_bodies.push_back(fb);
            }
        }

        depth--;
    };

    for (auto& d : DC->decls())
    {
        check_decl(d);
    }
    //修正索引
    for (auto& fb : func_bodies)
    {
        fb.index = func_info_index[fb.name];
    }

    return { func_infos, func_bodies };
}

std::tuple<std::string, std::string, std::vector<MemberInfo>> find_members(const std::string& filename_cpp, const std::string& class_name, const std::vector<std::string>& args)
{
    auto str = filefunc::readFileToString(filename_cpp);
    auto ast = clang::tooling::buildASTFromCodeWithArgs(str, args, filename_cpp);
    auto DC = ast->getASTContext().getTranslationUnitDecl();

    int depth = 0;

    std::vector<MemberInfo> member_infos;
    std::map<std::string, std::string> template_full_name;
    std::string c_name = class_name, template_str;
    std::function<void(clang::Decl*)> check_decl = [&](clang::Decl* decl)
    {
        depth++;
        std::string name1;
        bool is_member = false;
        if (decl->getKind() == clang::Decl::Kind::Namespace)
        {
            auto d1 = static_cast<clang::NamespaceDecl*>(decl);
            name1 = d1->getQualifiedNameAsString();
            for (auto& d : d1->decls())
            {
                check_decl(d);
            }
        }
        else if (decl->getKind() == clang::Decl::Kind::CXXRecord)
        {
            auto d1 = static_cast<clang::CXXRecordDecl*>(decl);
            name1 = d1->getQualifiedNameAsString();
            for (auto& d : d1->decls())
            {
                check_decl(d);
            }
        }
        else if (decl->getKind() == clang::Decl::Kind::ClassTemplateSpecialization)
        {
            auto d1 = static_cast<clang::ClassTemplateSpecializationDecl*>(decl);
            name1 = d1->getQualifiedNameAsString();
            auto d2 = static_cast<clang::CXXRecordDecl*>(decl);
            name1 += d2->getQualifiedNameAsString();
            for (auto& d : d2->decls())
            {
                check_decl(d);
            }
        }
        else if (decl->getKind() == clang::Decl::Kind::ClassTemplate)
        {
            auto d1 = static_cast<clang::ClassTemplateDecl*>(decl);
            name1 = d1->getQualifiedNameAsString();
            auto ps = d1->getTemplateParameters()->asArray();
            std::string para_string = "<", para_string_t = "<";    //后者包含typename
            for (auto& p : ps)
            {
                para_string += p->getNameAsString() + ",";
                para_string_t += "typename " + p->getNameAsString() + ",";
            }
            if (ps.size() > 0)
            {
                para_string.pop_back();
                para_string_t.pop_back();
            }
            para_string += ">";
            para_string_t += ">";
            name1 += para_string;
            //模板类在此处直接得出结果了
            if (name1.find(class_name + "<") == 0 || name1.contains("::" + class_name + "<"))
            {
                c_name = name1;
                template_str = "template " + para_string_t;
            }
            template_full_name[d1->getNameAsString()] = name1;
            auto d2 = d1->getTemplatedDecl();
            for (auto& d : d2->decls())
            {
                check_decl(d);
            }
        }
        else if (decl->getKind() == clang::Decl::Kind::Field)
        {
            auto d1 = static_cast<clang::FieldDecl*>(decl);
            name1 = d1->getQualifiedNameAsString();
            if (name1.find(class_name + "::") == 0 || name1.contains("::" + class_name + "::") || name1.contains("::" + class_name + "<"))
            {
                MemberInfo mi;
                mi.name = d1->getNameAsString();
                mi.full_name = name1;
                mi.type = deal_type(d1->getType());
                mi.is_public = d1->getAccess() == clang::AccessSpecifier::AS_public;
                mi.is_const = d1->getType().isConstQualified();
                member_infos.push_back(mi);
            }
            is_member = true;
        }
        depth--;
    };
    for (auto& d : DC->decls())
    {
        check_decl(d);
    }
    return { c_name, template_str, member_infos };    //c_name1可能包含模板参数
}
}    //namespace Struct2xxx