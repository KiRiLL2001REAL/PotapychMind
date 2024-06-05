#pragma once

#include "baseService.h"

#include <mutex>
#include <queue>

#include "../devices/serial/serialPortWrapper.h"

class ServoService : public BaseService
{
private:
    static size_t _mServiceId;
    // ������������ ��� ������
    static std::mutex _mConfigCheckMutex;
    static bool _mConfigChecked;
    static bool _mConfigInitialized;

protected:
    // ������
    int mServoId;

    std::mutex mutCmd_;
    // target position, delay after action (ms)
    std::queue<std::pair<unsigned, int>> mCmdQ;

    // ��� ������������� � ��������� �������������, ������ ����� �������� �������� 1,0,0,1
    unsigned char mServoOutBuffer[6];
    SerialPortWrapper* mpSerial;

    virtual void runner();

public:
    ServoService();
    // ����������� �������, ������ ::stop()
    ~ServoService();

    void enqueueTarget(unsigned int position, int delay_ms);
    bool isQueueEmpty();

    bool launch(const std::wstring& servo_name, SerialPortWrapper& serial);
};

