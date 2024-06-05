#include "iniReader.h"

#include <Windows.h>
#include <fstream>

Ini::IniReader::IniReader():
    m_exist(false)
{
    memset(m_filename, 0, sizeof(charType) * FILENAME_PATH_MAX_LEN);
}

Ini::IniReader::IniReader(const charType* filename):
    IniReader()
{
    open(filename);
}

Ini::IniReader::~IniReader()
{
    close();
}

bool Ini::IniReader::open(const charType* filename)
{
    size_t length
#ifdef UNICODE
        = wcslen(filename);
#else
        = strlen(filename);
#endif
    memcpy(m_filename, filename, length * sizeof(*filename));
    m_filename[FILENAME_PATH_MAX_LEN - 1] = 0;
    
    std::ifstream file(m_filename);
    m_exist = file.is_open();
    file.close();

    return m_exist;
}

void Ini::IniReader::close()
{
    if (!m_exist)
        return;
    memset(m_filename, 0, sizeof(charType) * FILENAME_PATH_MAX_LEN);
    m_exist = false;
}

int Ini::IniReader::readInt(const charType* section, const charType* key, const int defaultValue)
{
    if (!m_exist)
        return defaultValue;
    
    int iResult = GetPrivateProfileInt(section, key, defaultValue, m_filename);
    return iResult;
}

float Ini::IniReader::readFloat(const charType* section, const charType* key, const float defaultValue)
{
    if (!m_exist)
        return defaultValue;

    charType _result[STRING_MAX_LEN], _default[STRING_MAX_LEN];
#ifdef UNICODE
    swprintf_s(_default, L"%f", defaultValue);
#else
    sprintf_s(_default, "%f", defaultValue);
#endif

    GetPrivateProfileString(section, key, _default, _result, STRING_MAX_LEN, m_filename);

    float fResult
#ifdef UNICODE
        = (float)_wtof(_result);
#else
        = (float)atof(cResult);
#endif

    return fResult;
}

bool Ini::IniReader::readBool(const charType* section, const charType* key, const bool defaultValue)
{
    if (!m_exist)
        return defaultValue;

    charType _result[STRING_MAX_LEN], _default[STRING_MAX_LEN];
#ifdef UNICODE
    swprintf_s(_default, L"%s", defaultValue ? L"True" : L"False");
#else
    sprintf_s(_default, "%s", defaultValue ? "True" : "False");
#endif

    GetPrivateProfileString(section, key, _default, _result, STRING_MAX_LEN, m_filename);

    bool bResult
#ifdef UNICODE
        = wcscmp(_result, L"True") == 0 || wcscmp(_result, L"true") == 0;
#else
        = strcmp(_result, "True") == 0 || strcmp(_result, "true") == 0;
#endif

    return bResult;
}

Ini::charType* Ini::IniReader::readStr(const charType* section, const charType* key, const charType* defaultValue)
{
    charType* sResult = new charType[STRING_MAX_LEN];
    memset(sResult, 0, sizeof(charType) * STRING_MAX_LEN);

    if (!m_exist)
    {
        memcpy(sResult, defaultValue, sizeof(defaultValue));
        return sResult;
    }

    GetPrivateProfileString(section, key, defaultValue, sResult, STRING_MAX_LEN, m_filename);
    return sResult;
}
