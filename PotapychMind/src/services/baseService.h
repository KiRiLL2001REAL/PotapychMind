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
    // �������
    IP7_Client* pClient;
    IP7_Trace* pTrace;
    IP7_Trace::hModule hModule;

    // ������
    std::atomic<bool> mActiveFlag;
    std::thread* mpRunnerThr;
    std::atomic<bool> mCanDestroyThread;
    bool mThreadIsAlive;

    virtual void runner() = 0;

public:
    BaseService(const std::string& name);
    // ����������� �������, ������ ::stop()
    ~BaseService();

    // bool launch(params) ���������� ����������� �����

    // ����������� �������, ������� ���������� ������
    virtual void stop();

};

