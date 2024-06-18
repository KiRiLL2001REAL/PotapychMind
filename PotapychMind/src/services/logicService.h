#pragma once

#include "base/baseService.h"

#include <mutex>
#include <queue>

#include "../devices/serial/serialPortWrapper.h"

class LogicService : public BaseService
{
private:

protected:
    virtual void runner();

public:
    LogicService();
    // ����������� �������, ������ ::stop()
    ~LogicService();

    bool launch();
};

