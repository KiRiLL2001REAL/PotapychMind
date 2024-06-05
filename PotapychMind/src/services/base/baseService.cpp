#include "baseService.h"

#include <exception>

BaseService::BaseService(const std::string& name):
    Loggable(name),
    mActiveFlag(false),
    mpRunnerThr(nullptr),
    mCanDestroyThread(true),
    mThreadIsAlive(false)
{
    pTrace = P7_Create_Trace(pClient, TM("Trace Service"));
    if (NULL == pTrace)
    {
        printf("ERR : Can't create P7 trace channel in %s.\n", name.c_str());
        pClient->Release();
        pClient = NULL;
        char msg[128];
        sprintf_s(msg, "Can not create P7 trace channel (%s)", name.c_str());
        throw std::runtime_error(msg);
    }
    else
    {
        auto decorated_w_name = _mName + L"Instance";
        pTrace->Register_Thread(decorated_w_name.c_str(), 0);
        pTrace->Register_Module(_mName.c_str(), &hModule);
    }
    pTrace->P7_INFO(hModule, TM("%s instance is created"), _mName.c_str());
}

BaseService::~BaseService()
{
    if (mThreadIsAlive)
        stop();
    if (mpRunnerThr)
    {
        mpRunnerThr->join();
        delete mpRunnerThr;
    }
    pTrace->P7_INFO(hModule, TM("%s instance is disposed"), _mName.c_str());
}

void BaseService::stop()
{
    pTrace->P7_INFO(hModule, TM("Stopping service"));
    mActiveFlag = false;
    while (!mCanDestroyThread)
    {
        using namespace std::chrono_literals;
        std::this_thread::yield();
        std::this_thread::sleep_for(1ms);
    }
    mThreadIsAlive = false;
    pTrace->P7_INFO(hModule, TM("Service stopped"));
}
