#pragma once
#include <string>
#include <vector>

class Struct2xxx
{
public:
    enum class Type
    {
        Public,
        Private,
        Protected,
    };

    struct Record
    {
        std::string name;
        std::string type;
        Type available;
    };
    std::vector<Record> records;

    void Parse(const std::string& content);
};
