#include "loggable.h"

#include "../../utility.h"

Loggable::Loggable(const std::string& name):
	_mName(Utility::to_wstring(name)),
	pClient(NULL),
	pTrace(NULL),
	hModule(NULL)
{
    pClient = P7_Get_Shared(TM("AppClient"));
    if (pClient == NULL)
    {
        printf("ERR : Can't get P7 shared client instance.\n");
        char msg[128];
        sprintf_s(msg, "Can not get shared P7 client (%s)", name.c_str());
        throw std::runtime_error(msg);
    }
}

Loggable::~Loggable()
{
    if (pTrace)
    {
        pTrace->Unregister_Thread(0);
        pTrace->Release();
        pTrace = NULL;
    }
    if (pClient)
    {
        pClient->Release();
        pClient = NULL;
    }
}
