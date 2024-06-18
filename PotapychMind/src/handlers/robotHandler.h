#pragma once

#include "../internal/loggable/loggable.h"

#include "../devices/serial/serialPortWrapper.h"
#include <mutex>
#include <queue>
#include <string>
#include <vector>
#include "../services/servoService.h"

class RobotHandler : public Loggable
{
private:
    static const int mDefaultActionDelay;

protected:
    SerialPortWrapper mSerial;

    std::vector<ServoService*> mServo;

    bool checkServos() const;

public:
    RobotHandler();
    ~RobotHandler();

    const std::wstring& getConnectedComPortName();

    void headToCenter();
    void headUp();
    void headDown();
    void headLeft();
    void headRight();
    
    void handsToCenter();
    void leftHandUp();
    void leftHandDown();
    void rightHandUp();
    void rightHandDown();
    void flapHands();

    bool launchServos(const std::wstring& comName);
};

