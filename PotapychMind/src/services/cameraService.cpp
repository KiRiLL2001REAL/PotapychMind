#include "cameraService.h"

#include "../utility.h"
#include <opencv2/highgui.hpp>

#include <exception>

void CameraService::storeFrame(cv::Mat& frame)
{
	std::unique_lock<std::shared_mutex> lk(mut_);
	frame.copyTo(mCachedFrame);
	using namespace std::chrono;
	mCachedTimestamp = steady_clock::now().time_since_epoch().count();
}

void CameraService::runner()
{
	pTrace->Register_Thread(TM("CameraServiceRunner"), 0);
	pTrace->P7_INFO(hModule, TM("Camera-frame-grabber thread started"));

	cv::Mat frame;
	while (mActiveFlag)
	{
		if (mCap.read(frame))
			storeFrame(frame);
		{
			using namespace std::chrono_literals;
			std::this_thread::yield();
			std::this_thread::sleep_for(1ms);
		}
	}
	mCanDestroyThread = true;

	pTrace->P7_INFO(hModule, TM("Camera-frame-grabber thread finished"));
	pTrace->Unregister_Thread(0);
}

CameraService::CameraService() :
	pClient(NULL),
	pTrace(NULL),
	hModule(NULL),
	mCachedTimestamp(0),
	mCachedFrame(),
	mConnectedDeviceId(-1),
	mActiveFlag(false),
	mpRunnerThr(nullptr),
	mCanDestroyThread(true),
	mThreadIsAlive(false)
{
	pClient = P7_Get_Shared(TM("AppClient"));
	if (pClient == NULL)
	{
		printf("ERR : Can't get P7 shared client instance.\n");
		throw std::runtime_error("Can not get shared P7 client (CameraService)");
	}
	else
	{
		pTrace = P7_Create_Trace(pClient, TM("Trace Service"));
		if (NULL == pTrace)
		{
			printf("ERR : Can't create P7 trace channel in CameraService.\n");
			pClient->Release();
			pClient = NULL;
			throw std::runtime_error("Can not create P7 trace channel (CameraService)");
		}
		else
		{
			pTrace->Register_Thread(TM("CameraServiceInstance"), 0);
			pTrace->Register_Module(TM("CameraService"), &hModule);
		}
	}
	pTrace->P7_INFO(hModule, TM("CameraService instance is created"));
}

CameraService::~CameraService()
{
	if (mThreadIsAlive)
		stop();
	if (mpRunnerThr)
	{
		mpRunnerThr->join();
		delete mpRunnerThr;
	}
	mCachedFrame.release();
	pTrace->P7_INFO(hModule, TM("CameraService instance is disposed"));
	if (pTrace)
	{
		pTrace->Unregister_Thread(0);
		pTrace->Release();
		pTrace = NULL;
	}
	if (pClient)
	{
		pClient->Release();
		pClient = NULL;
	}
}

int CameraService::getConnectedDeviceId()
{
	std::lock_guard<std::mutex> lk(mutDeviceId_);
	return mConnectedDeviceId;
}

void CameraService::getFrame(cv::Mat& dst, long long& timestamp)
{
	std::shared_lock<std::shared_mutex> lk(mut_);
	mCachedFrame.copyTo(dst);
	timestamp = mCachedTimestamp;
}

bool CameraService::launch(int deviceId, int reqWidth, int reqHeight)
{
	mutDeviceId_.lock();
	pTrace->P7_INFO(hModule, TM("Trying to open device%d"), deviceId);
	if (mCap.open(deviceId, cv::CAP_ANY))
	{
		pTrace->P7_INFO(hModule, TM("Connected device%d"), deviceId);
		pTrace->P7_INFO(hModule, TM("Check if device supports resolution %dx%d..."), reqWidth, reqHeight);
		mCap.set(cv::CAP_PROP_FRAME_WIDTH, reqWidth);
		mCap.set(cv::CAP_PROP_FRAME_HEIGHT, reqHeight);
		if (mCap.get(cv::CAP_PROP_FRAME_WIDTH) == reqWidth
			&& mCap.get(cv::CAP_PROP_FRAME_HEIGHT) == reqHeight)
		{
			mConnectedDeviceId = deviceId;
			pTrace->P7_INFO(hModule, TM("Success"));
		}
		else
		{
			mCap.release();
			pTrace->P7_ERROR(hModule, TM("Failure. Closed device%d"), deviceId);
		}
	}
	else
		pTrace->P7_ERROR(hModule, TM("Device%d is busy, or unreachable"), deviceId);
	mutDeviceId_.unlock();

	if (getConnectedDeviceId() == -1)
		return false;

	pTrace->P7_INFO(hModule, TM("Launching service"));
	mActiveFlag = true;
	mCanDestroyThread = false;
	mThreadIsAlive = true;
	mpRunnerThr = new std::thread(&CameraService::runner, this);
	pTrace->P7_INFO(hModule, TM("Service launched"));

	return true;
}

void CameraService::stop()
{
	pTrace->P7_INFO(hModule, TM("Stopping service"));
	mActiveFlag = false;
	while (!mCanDestroyThread)
	{
		using namespace std::chrono_literals;
		std::this_thread::yield();
		std::this_thread::sleep_for(1ms);
	}
	mThreadIsAlive = false;
	if (mCap.isOpened())
	{
		std::lock_guard<std::mutex> lk(mutDeviceId_);
		mCap.release();
		pTrace->P7_INFO(hModule, TM("Closed device%d"), mConnectedDeviceId);
		mConnectedDeviceId = -1;
	}

	pTrace->P7_INFO(hModule, TM("Service stopped"));
}
