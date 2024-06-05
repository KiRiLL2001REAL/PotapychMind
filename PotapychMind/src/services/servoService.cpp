#include "servoService.h"

#include "../data/defaultConfig.h"
#include "../data/servoState.h"

#include <chrono>

size_t ServoService::_mServiceId = 0;
std::mutex ServoService::_mConfigCheckMutex;
bool ServoService::_mConfigChecked = false;
bool ServoService::_mConfigInitialized = false;

void ServoService::runner()
{
    {
        wchar_t buf[80]; memset(buf, 0, sizeof(wchar_t) * 80);
        swprintf_s(buf, L"ServoService%dRunner", mServoId);
        pTrace->Register_Thread(buf, 0);
    }
    pTrace->P7_INFO(hModule, TM("Servo%d thread started"), mServoId);

    const auto& config = *DefaultConfig::getInstance();

    const auto servo_lower_bound = config.getServoBounds(mServoId).first;
    const auto servo_upper_bound = config.getServoBounds(mServoId).second;

    auto& servoState = *ServoState::getInstance();

    while (mActiveFlag)
    {
        if (isQueueEmpty())
        {
            using namespace std::chrono_literals;
            std::this_thread::yield();
            std::this_thread::sleep_for(10ms);
            continue;
        }

        std::pair<unsigned int, int> cmd;
        {
            std::lock_guard<std::mutex> lk(mutCmd_);
            cmd = mCmdQ.front();
            mCmdQ.pop();
        }
        unsigned int position = cmd.first;
        int delay = max(5, cmd.second);

        auto bounded_position = max(servo_lower_bound, min(servo_upper_bound, position));
        mServoOutBuffer[5] = bounded_position;
        mpSerial->write(mServoOutBuffer, sizeof(unsigned char) * 6);
        // обновляем актуальные данные положения сервы
        servoState.setPosition(mServoId, bounded_position);

        using namespace std::chrono_literals;
        std::this_thread::yield();
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }

    mCanDestroyThread = true;

    pTrace->P7_INFO(hModule, TM("Servo%d thread finished"));
    pTrace->Unregister_Thread(0);
}

ServoService::ServoService() :
    BaseService("ServoService" + std::to_string(_mServiceId++)),
    mServoId(-1),
    mCmdQ(),
    mpSerial(nullptr)
{
    memset(mServoOutBuffer, 0, sizeof(unsigned char) * 6);
    mServoOutBuffer[0] = 1;
    mServoOutBuffer[1] = 0;
    mServoOutBuffer[2] = 0;
    mServoOutBuffer[3] = 1;
}

ServoService::~ServoService()
{
    mServoId = -1;
    {
        std::lock_guard<std::mutex> lk(mutCmd_);
        std::queue<std::pair<unsigned int, int>>().swap(mCmdQ);
    }
    memset(mServoOutBuffer, 0, sizeof(unsigned char) * 6);
    mpSerial = nullptr;
}

void ServoService::enqueueTarget(unsigned int position, int delay_ms)
{
    std::lock_guard<std::mutex> lk(mutCmd_);
    mCmdQ.push(std::pair(position, delay_ms));
}

bool ServoService::isQueueEmpty()
{
    std::lock_guard<std::mutex> lk(mutCmd_);
    return mCmdQ.empty();
}

bool ServoService::launch(const std::wstring& servo_name, SerialPortWrapper& serial)
{
    mpSerial = &serial;
    const auto& config = *DefaultConfig::getInstance();

    { // Проверяем, загружен ли конфиг (при первом создании подобного сервиса)
        std::lock_guard<std::mutex> lk(_mConfigCheckMutex);
        if (!_mConfigChecked)
        {
            _mConfigChecked = true;
            pTrace->P7_INFO(hModule, TM("Check if config is initialized"));
            _mConfigInitialized = config.isInitialized();
            if (!_mConfigInitialized)
            { // Если конфиг не загружен - сразу выходим (не прочитать Id сервы)
                pTrace->P7_ERROR(hModule, TM("Failure. Config is not initialized"));
                return false;
            }
            pTrace->P7_INFO(hModule, TM("Check passed"));
        }
    }
    
    { // Проверяем, загружен ли конфиг
        std::lock_guard<std::mutex> lk(_mConfigCheckMutex);
        if (!_mConfigInitialized)
        {
            pTrace->P7_ERROR(hModule, TM("Can't launch. Config is not initialized"));
            return false;
        }
    }

    // Читаем Id сервы
    pTrace->P7_INFO(hModule, TM("Reading servoId from config"));
    mServoId = config.getServoId(servo_name);
    if (mServoId == -1)
    {
        pTrace->P7_ERROR(hModule, TM("Failure. Servo name '%s' was not declared in configuration file"), servo_name.c_str());
        return false;
    }
    pTrace->P7_INFO(hModule, TM("Success. Servo with name '%s' has id '%d'"), servo_name.c_str(), mServoId);

    // Дописываем данные для выгрузки на последовательный порт
    // и помещаем команду занятия начального положения в очередь
    mServoOutBuffer[4] = mServoId;
    auto position = (unsigned int)config.getServoInitPos(mServoId);
    enqueueTarget(position, 0);

    pTrace->P7_INFO(hModule, TM("Launching service"));
    mActiveFlag = true;
    mCanDestroyThread = false;
    mThreadIsAlive = true;
    mpRunnerThr = new std::thread(&ServoService::runner, this);
    pTrace->P7_INFO(hModule, TM("Service launched"));

    return true;
}
