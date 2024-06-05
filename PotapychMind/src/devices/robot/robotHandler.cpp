#include "robotHandler.h"

#include "../../data/defaultConfig.h"

bool RobotHandler::checkServos() const
{
    bool empty = mServo.empty();
    if (empty)
        pTrace->P7_WARNING(hModule, TM("Servos aren't initialized yet"));
    return !empty;
}

RobotHandler::RobotHandler():
    Loggable("RobotHandler"),
    mServo()
{
    static const std::string name = "RobotHandler";
    pTrace = P7_Create_Trace(pClient, TM("Trace Handler"));
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

RobotHandler::~RobotHandler()
{
    int count = 0;
    for (int i = 0; i < mServo.size(); i++)
    {
        if (mServo[i])
        {
            delete mServo[i];
            count++;
        }
    }
    pTrace->P7_INFO(hModule, TM("Disposed %d servo services"), count);

    if (mSerial.getConnectedComName() != L"")
        mSerial.disconnect();
}

const std::wstring& RobotHandler::getConnectedComPortName()
{
    return mSerial.getConnectedComName();
}

void RobotHandler::headToCenter()
{
    static std::vector<std::wstring> servo_names = { L"HEAD_X", L"HEAD_Y" };

    if (!checkServos())
        return;

    const auto& config = *DefaultConfig::getInstance();

    for (const auto& servo_name : servo_names)
    {
        int servoId = config.getServoId(servo_name);
        if (servoId == -1)
        {
            pTrace->P7_WARNING(hModule, TM("Attempt to use unknown servo '%s'"), servo_name.c_str());
            continue;
        }
        if (!mServo[servoId])
            pTrace->P7_WARNING(hModule, TM("Attempt to use uninitialized servo %d"), servoId);
        else
        {
            auto pos = (unsigned int)config.getServoInitPos(servoId);
            mServo[servoId]->enqueueTarget(pos, 200);
        }
    }
}

void RobotHandler::headUp()
{
    static std::wstring servo_name = L"HEAD_Y";

    if (!checkServos())
        return;
    
    const auto& config = *DefaultConfig::getInstance();
    int servoId = config.getServoId(servo_name);
    if (servoId == -1)
    {
        pTrace->P7_WARNING(hModule, TM("Attempt to use unknown servo '%s'"), servo_name.c_str());
        return;
    }
    if (!mServo[servoId])
        pTrace->P7_WARNING(hModule, TM("Attempt to use uninitialized servo %d"), servoId);
    else
    {
        auto pos = (unsigned int)config.getServoBounds(servoId).first;
        mServo[servoId]->enqueueTarget(pos, 200);
    }
}

void RobotHandler::headDown()
{
    static std::wstring servo_name = L"HEAD_Y";

    if (!checkServos())
        return;

    const auto& config = *DefaultConfig::getInstance();
    int servoId = config.getServoId(servo_name);
    if (servoId == -1)
    {
        pTrace->P7_WARNING(hModule, TM("Attempt to use unknown servo '%s'"), servo_name.c_str());
        return;
    }
    if (!mServo[servoId])
        pTrace->P7_WARNING(hModule, TM("Attempt to use uninitialized servo %d"), servoId);
    else
    {
        auto pos = (unsigned int)config.getServoBounds(servoId).second;
        mServo[servoId]->enqueueTarget(pos, 200);
    }
}

void RobotHandler::headLeft()
{
    static std::wstring servo_name = L"HEAD_X";

    if (!checkServos())
        return;

    const auto& config = *DefaultConfig::getInstance();
    int servoId = config.getServoId(servo_name);
    if (servoId == -1)
    {
        pTrace->P7_WARNING(hModule, TM("Attempt to use unknown servo '%s'"), servo_name.c_str());
        return;
    }
    if (!mServo[servoId])
        pTrace->P7_WARNING(hModule, TM("Attempt to use uninitialized servo %d"), servoId);
    else
    {
        auto pos = (unsigned int)config.getServoBounds(servoId).first;
        mServo[servoId]->enqueueTarget(pos, 200);
    }
}

void RobotHandler::headRight()
{
    static std::wstring servo_name = L"HEAD_Y";

    if (!checkServos())
        return;

    const auto& config = *DefaultConfig::getInstance();
    int servoId = config.getServoId(servo_name);
    if (servoId == -1)
    {
        pTrace->P7_WARNING(hModule, TM("Attempt to use unknown servo '%s'"), servo_name.c_str());
        return;
    }
    if (!mServo[servoId])
        pTrace->P7_WARNING(hModule, TM("Attempt to use uninitialized servo %d"), servoId);
    else
    {
        auto pos = (unsigned int)config.getServoBounds(servoId).second;
        mServo[servoId]->enqueueTarget(pos, 200);
    }
}

void RobotHandler::handsToCenter()
{
    static std::vector<std::wstring> servo_names = { L"HAND_L", L"HAND_R" };

    if (!checkServos())
        return;

    const auto& config = *DefaultConfig::getInstance();

    for (const auto& servo_name : servo_names)
    {
        int servoId = config.getServoId(servo_name);
        if (servoId == -1)
        {
            pTrace->P7_WARNING(hModule, TM("Attempt to use unknown servo '%s'"), servo_name.c_str());
            continue;
        }
        if (!mServo[servoId])
            pTrace->P7_WARNING(hModule, TM("Attempt to use uninitialized servo %d"), servoId);
        else
        {
            auto pos = (unsigned int)config.getServoInitPos(servoId);
            mServo[servoId]->enqueueTarget(pos, 200);
        }
    }
}

void RobotHandler::leftHandUp()
{
    static std::wstring servo_name = L"HAND_L";

    if (!checkServos())
        return;

    const auto& config = *DefaultConfig::getInstance();
    int servoId = config.getServoId(servo_name);
    if (servoId == -1)
    {
        pTrace->P7_WARNING(hModule, TM("Attempt to use unknown servo '%s'"), servo_name.c_str());
        return;
    }
    if (!mServo[servoId])
        pTrace->P7_WARNING(hModule, TM("Attempt to use uninitialized servo %d"), servoId);
    else
    {
        auto pos = (unsigned int)config.getServoBounds(servoId).first;
        mServo[servoId]->enqueueTarget(pos, 200);
    }
}

void RobotHandler::leftHandDown()
{
    static std::wstring servo_name = L"HAND_L";

    if (!checkServos())
        return;

    const auto& config = *DefaultConfig::getInstance();
    int servoId = config.getServoId(servo_name);
    if (servoId == -1)
    {
        pTrace->P7_WARNING(hModule, TM("Attempt to use unknown servo '%s'"), servo_name.c_str());
        return;
    }
    if (!mServo[servoId])
        pTrace->P7_WARNING(hModule, TM("Attempt to use uninitialized servo %d"), servoId);
    else
    {
        auto pos = (unsigned int)config.getServoBounds(servoId).second;
        mServo[servoId]->enqueueTarget(pos, 200);
    }
}

void RobotHandler::rightHandUp()
{
    static std::wstring servo_name = L"HAND_R";

    if (!checkServos())
        return;

    const auto& config = *DefaultConfig::getInstance();
    int servoId = config.getServoId(servo_name);
    if (servoId == -1)
    {
        pTrace->P7_WARNING(hModule, TM("Attempt to use unknown servo '%s'"), servo_name.c_str());
        return;
    }
    if (!mServo[servoId])
        pTrace->P7_WARNING(hModule, TM("Attempt to use uninitialized servo %d"), servoId);
    else
    {
        auto pos = (unsigned int)config.getServoBounds(servoId).first;
        mServo[servoId]->enqueueTarget(pos, 200);
    }
}

void RobotHandler::rightHandDown()
{
    static std::wstring servo_name = L"HAND_R";

    if (!checkServos())
        return;

    const auto& config = *DefaultConfig::getInstance();
    int servoId = config.getServoId(servo_name);
    if (servoId == -1)
    {
        pTrace->P7_WARNING(hModule, TM("Attempt to use unknown servo '%s'"), servo_name.c_str());
        return;
    }
    if (!mServo[servoId])
        pTrace->P7_WARNING(hModule, TM("Attempt to use uninitialized servo %d"), servoId);
    else
    {
        auto pos = (unsigned int)config.getServoBounds(servoId).second;
        mServo[servoId]->enqueueTarget(pos, 200);
    }
}

void RobotHandler::flapHands()
{
    leftHandDown();
    rightHandDown();

    leftHandUp();
    rightHandUp();

    leftHandDown();
    rightHandDown();
}

bool RobotHandler::launchServos(const std::wstring& comName)
{
    auto& config = *DefaultConfig::getInstance();
    if (!config.isInitialized())
    {
        pTrace->P7_ERROR(hModule, TM("Can't launch. Config is not initialized"));
        return false;
    }

    { // подключение к последовательному порту
        const auto comNameChars = comName.c_str();
        pTrace->P7_INFO(hModule, TM("Trying to open port '%s'"), comNameChars);
        if (mSerial.connect(comName))
            pTrace->P7_INFO(hModule, TM("Connected port '%s'"), comNameChars);
        else
            pTrace->P7_ERROR(hModule, TM("Port '%s' is busy, or unreachable"), comNameChars);
    }
    if (mSerial.getConnectedComName() == L"")
        return false;
    
    int servos_up = 0;
    { // запуск потоков сервоприводов
        const int count = config.getServoCnt();
        servos_up = count;
        for (int i = 0; i < count; i++)
        {
            auto servoName = config.getServoName(i);
            ServoService* servoService = nullptr;
            if (servoName == L"")
            {
                pTrace->P7_WARNING(hModule, TM("Can't find name of servo%d"), i);
                servos_up--;
            }
            else
            {
                servoService = new ServoService();
                servoService->launch(servoName, mSerial);
            }
            
            mServo.push_back(servoService);
        }
    }
    pTrace->P7_INFO(hModule, TM("Launched services for %d/%d servos"), servos_up, config.getServoCnt());

    return true;
}
