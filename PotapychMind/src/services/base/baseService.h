#pragma once

#include "../../internal/loggable/loggable.h"
#include <shared_mutex>
#include <thread>

//TODO разделить на BaseService и Loggable
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
    // блокирующая функция, смотри ::stop()
    ~BaseService();

    // bool launch(params) необходимо реализовать самим

    // блокирующая функция, ожидает завершения потока
    virtual void stop();

};

