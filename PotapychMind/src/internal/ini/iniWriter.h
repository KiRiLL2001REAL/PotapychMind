#pragma once

#include "iniDefaults.h"

// ref to https://www.codeproject.com/Articles/10809/A-Small-Class-to-Read-INI-File
namespace Ini
{
    class IniWriter
    {
    private:
        bool m_exist;
        charType m_filename[FILENAME_PATH_MAX_LEN];

        IniWriter(const IniWriter&) = delete;
        void operator=(const IniWriter&) = delete;

    public:
        IniWriter();
        IniWriter(const charType* filename);
        ~IniWriter();

        bool open(const charType* filename);
        void close();

        void writeInt(const charType* section, const charType* key, const int value);
        void writeFloat(const charType* section, const charType* key, const float value);
        void writeBool(const charType* section, const charType* key, const bool value);
        void writeStr(const charType* section, const charType* key, const charType* value);
    };
}
