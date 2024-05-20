#include "baseService.h"

#include <exception>
#include "../utility.h"

BaseService::BaseService(const std::string& name):
    m_name(Utility::to_wstring(name)),
    pClient(NULL),
    pTrace(NULL),
    hModule(NULL), mActiveFlag(false),
    mpRunnerThr(nullptr),
    mCanDestroyThread(true),
    mThreadIsAlive(false)
{
    pClient = P7_Get_Shared(TM("AppClient"));
    if (pClient == NULL)
    {
        printf("ERR : Can't get P7 shared client instance.\n");
        char msg[128];
        sprintf_s(msg, "Can not get shared P7 client (%s)", name.c_str());
        throw std::runtime_error(msg);
    }
    else
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
            auto decorated_w_name = m_name + L"Instance";
            pTrace->Register_Thread(decorated_w_name.c_str(), 0);
            pTrace->Register_Module(m_name.c_str(), &hModule);
        }
    }
    pTrace->P7_INFO(hModule, TM("%s instance is created"), m_name.c_str());
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
    pTrace->P7_INFO(hModule, TM("%s instance is disposed"), m_name.c_str());
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
