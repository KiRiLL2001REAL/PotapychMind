#include "robotHandlerService.h"

void RobotHandlerService::popQ(std::pair<int, int>& dst)
{
	std::lock_guard<std::mutex> lk(mutCmd_);
	dst = mCmdQ.front();
	mCmdQ.pop();
}

void RobotHandlerService::runner()
{
	pTrace->Register_Thread(TM("RobotHandlerServiceRunner"), 0);
	pTrace->P7_INFO(hModule, TM("Robot-cmd-executor thread started"));
	
	int cmdProcessedInLoop = 0;
	std::pair<int, int> cmd;
	while (mActiveFlag)
	{
		cmdProcessedInLoop = 0;
		while (!mCmdQ.empty())
		{
			popQ(cmd);
			//TODO cmd handling
			cmdProcessedInLoop++;
		}

		if (cmdProcessedInLoop)
		{
			pTrace->P7_INFO(hModule, TM("Processed %d commands"), cmdProcessedInLoop);
		}

		using namespace std::chrono_literals;
		std::this_thread::yield();
		std::this_thread::sleep_for(1ms);
	}
	mCanDestroyThread = true;

	pTrace->P7_INFO(hModule, TM("Robot-cmd-executor thread finished"));
	pTrace->Unregister_Thread(0);
}

RobotHandlerService::RobotHandlerService():
	BaseService("RobotHandlerService"),
	mCmdQ()
{
}

RobotHandlerService::~RobotHandlerService()
{
}

const std::wstring& RobotHandlerService::getConnectedComPortName()
{
	std::lock_guard<std::mutex> lk(mutSerial_);
	return mSerial.getConnectedComName();
}

void RobotHandlerService::putCmd(const std::pair<int, int>& cmd)
{
	std::lock_guard<std::mutex> lk(mutCmd_);
	mCmdQ.push(cmd);
}

bool RobotHandlerService::launch(const std::wstring& comName)
{
	{
		std::lock_guard<std::mutex> lk(mutSerial_);
		const auto comNameChars = comName.c_str();
		pTrace->P7_INFO(hModule, TM("Trying to open port '%s'"), comNameChars);
		if (mSerial.connect(comName))
			pTrace->P7_INFO(hModule, TM("Connected port '%s'"), comNameChars);
		else
			pTrace->P7_ERROR(hModule, TM("Port '%s' is busy, or unreachable"), comNameChars);
	}
	if (mSerial.getConnectedComName() == L"")
		return false;

	pTrace->P7_INFO(hModule, TM("Launching service"));
	mActiveFlag = true;
	mCanDestroyThread = false;
	mThreadIsAlive = true;
	mpRunnerThr = new std::thread(&RobotHandlerService::runner, this);
	pTrace->P7_INFO(hModule, TM("Service launched"));

	return true;
}

void RobotHandlerService::stop()
{
	BaseService::stop();

	// Освобождение ресурсов
	std::lock_guard<std::mutex> lk(mutCmd_);
	if (mSerial.getConnectedComName() != L"")
	{
		std::queue<std::pair<int, int>>().swap(mCmdQ);
		mSerial.disconnect();
	}
}
