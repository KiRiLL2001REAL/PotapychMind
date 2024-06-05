#pragma once

#include "iniDefaults.h"

// ref to https://www.codeproject.com/Articles/10809/A-Small-Class-to-Read-INI-File
namespace Ini
{
    class IniReader
    {
    private:
        bool m_exist;
        charType m_filename[FILENAME_PATH_MAX_LEN];

        IniReader(const IniReader&) = delete;
        void operator=(const IniReader&) = delete;

    public:
        IniReader();
        IniReader(const charType* filename);
        ~IniReader();

        bool open(const charType* filename);
        void close();

        int readInt(const charType* section, const charType* key, const int defaultValue = DefaultIntegerValue);
        float readFloat(const charType* section, const charType* key, const float defaultValue = DefaultFloatValue);
        bool readBool(const charType* section, const charType* key, const bool defaultValue = DefaultBooleanValue);
        // выделяет память под считанное значение, после использования вызвать delete[]
        charType* readStr(const charType* section, const charType* key, const charType* defaultValue = DefaultStringValue);
    };
}