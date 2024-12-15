#pragma once
#include "NoCopy.h"
#include "string"
#include "iostream"
#include "fstream"
#include "type_traits"

class DebugBase
{
public:
    DebugBase() = default;
    virtual ~DebugBase() {}
    virtual DebugBase& operator << (std::string) = 0;
};


class ConsoleDebug : public DebugBase
{
public:
    bool autoEndl;
    bool outTime;
    ConsoleDebug& operator<< (std::string s)
    {
        if (outTime)
        {
            cout << time(0) << ':';
        }
        std::cout << s;
        if (autoEndl)
            std::cout << std::endl;
        return *this;
    }
};

class FileDebug : public DebugBase
{
    std::ofstream fout;

    FileDebug(std::string filePath) : fout(filePath.c_str())
    {}

    FileDebug& operator<< (std::string s)
    {
        fout << s;
        return *this;
    }

};
