#pragma once

namespace Ini
{
#ifdef UNICODE
    typedef wchar_t charType;
#else
    typedef char charType;
#endif

    const int DefaultIntegerValue = 0;
    const float DefaultFloatValue = 0.0f;
    const bool DefaultBooleanValue = false;

    const charType* const DefaultStringValue // символ ';' пропущен намеренно
#ifdef UNICODE
        = L"DEFAULT_STRING"; // <-
#else
        = "DEFAULT_STRING"; // <-
#endif

#ifndef FILENAME_PATH_MAX_LEN
#define FILENAME_PATH_MAX_LEN 1024
#endif

#ifndef STRING_MAX_LEN
#define STRING_MAX_LEN 256
#endif
}