#pragma once

#include <string>

#include <P7_Client.h>
#include <P7_Trace.h>

class Loggable
{
protected:
    std::wstring _mName;

    IP7_Client* pClient;
    IP7_Trace* pTrace;
    IP7_Trace::hModule hModule;

public:
    Loggable(const std::string& name);
    ~Loggable();
};

