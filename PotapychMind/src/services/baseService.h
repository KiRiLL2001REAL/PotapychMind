#pragma once

#include <shared_mutex>
#include <thread>

#include <string>

#include <P7_Client.h>
#include <P7_Trace.h>

class BaseService
{
private:
    std::wstring m_name;

protected:
    // Логгинг
    IP7_Client* pClient;
    IP7_Trace* pTrace;
    IP7_Trace::hModule hModule;

    // Потоки
    std::atomic<bool> mActiveFlag;
    std::thread* mpRunnerThr;
    std::atomic<bool> mCanDestroyThread;
    bool mThreadIsAlive;

    virtual void runner() = 0;

public:
    BaseService(const std::string& name);
    // блокирующая функция, смотри ::stop()
    ~BaseService();

    // bool launch(params) необходимо реализовать самим

    // блокирующая функция, ожидает завершения потока
    virtual void stop();

};

