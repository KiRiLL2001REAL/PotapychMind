#include "iniWriter.h"

#include <Windows.h>
#include <fstream>

Ini::IniWriter::IniWriter():
    m_exist(false)
{
    memset(m_filename, 0, sizeof(charType) * FILENAME_PATH_MAX_LEN);
}

Ini::IniWriter::IniWriter(const charType* filename):
    IniWriter()
{
    open(filename);
}

Ini::IniWriter::~IniWriter()
{
    close();
}

bool Ini::IniWriter::open(const charType* filename)
{
    size_t length
#ifdef UNICODE
        = wcslen(filename);
#else
        = strlen(filename);
#endif
    memcpy(m_filename, filename, length * sizeof(*filename));
    m_filename[FILENAME_PATH_MAX_LEN - 1] = 0;

    std::fstream file(m_filename, std::ios::app);
    m_exist = file.is_open();
    file.close();

    return m_exist;
}

void Ini::IniWriter::close()
{
    if (!m_exist)
        return;
    memset(m_filename, 0, sizeof(charType) * FILENAME_PATH_MAX_LEN);
    m_exist = false;
}

void Ini::IniWriter::writeInt(const charType* section, const charType* key, const int value)
{
    if (!m_exist)
        return;

    charType sValue[STRING_MAX_LEN];
#ifdef UNICODE
    swprintf_s(sValue, L"%d", value);
#else
    sprintf_s(sValue, "%d", value);
#endif

    WritePrivateProfileString(section, key, sValue, m_filename);
}

void Ini::IniWriter::writeFloat(const charType* section, const charType* key, const float value)
{
    if (!m_exist)
        return;

    charType sValue[STRING_MAX_LEN];
#ifdef UNICODE
    swprintf_s(sValue, L"%f", value);
#else
    sprintf_s(sValue, "%f", value);
#endif

    WritePrivateProfileString(section, key, sValue, m_filename);
}

void Ini::IniWriter::writeBool(const charType* section, const charType* key, const bool value)
{
    if (!m_exist)
        return;

    charType sValue[STRING_MAX_LEN];
#ifdef UNICODE
    swprintf_s(sValue, L"%s", value ? L"True" : L"False");
#else
    sprintf_s(sValue, "%s", value ? "True" : "False");
#endif

    WritePrivateProfileString(section, key, sValue, m_filename);
}

void Ini::IniWriter::writeStr(const charType* section, const charType* key, const charType* value)
{
    if (!m_exist)
        return;

    WritePrivateProfileString(section, key, value, m_filename);
}
