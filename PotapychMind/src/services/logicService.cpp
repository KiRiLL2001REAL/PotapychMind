#include "logicService.h"

#include "../data/defaultConfig.h"
#include "../data/servoState.h"

#include <chrono>

void LogicService::runner()
{
    pTrace->P7_INFO(hModule, TM("Logic thread started"));

    while (mActiveFlag)
    {
        using namespace std::chrono_literals;
        std::this_thread::yield();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    mCanDestroyThread = true;

    pTrace->P7_INFO(hModule, TM("Servo%d thread finished"));
    pTrace->Unregister_Thread(0);
}

LogicService::LogicService() :
    BaseService("LogicService")
{

}

LogicService::~LogicService()
{
    
}

bool LogicService::launch()
{
    pTrace->P7_INFO(hModule, TM("Launching service"));
    mActiveFlag = true;
    mCanDestroyThread = false;
    mThreadIsAlive = true;
    mpRunnerThr = new std::thread(&LogicService::runner, this);
    pTrace->P7_INFO(hModule, TM("Service launched"));

    return true;
}
