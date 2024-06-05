#pragma once

#include "../../internal/loggable/loggable.h"
#include <shared_mutex>
#include <thread>

//TODO ��������� �� BaseService � Loggable
class BaseService abstract : public Loggable
{
protected:
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

