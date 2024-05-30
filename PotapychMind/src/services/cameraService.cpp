#include "cameraService.h"

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
	BaseService("CameraService"),
	mCachedTimestamp(0),
	mCachedFrame(),
	mConnectedDeviceId(-1)
{
}

CameraService::~CameraService()
{
	mCachedFrame.release();
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
	BaseService::stop();
	if (mCap.isOpened())
	{
		std::lock_guard<std::mutex> lk(mutDeviceId_);
		mCap.release();
		pTrace->P7_INFO(hModule, TM("Closed device%d"), mConnectedDeviceId);
		mConnectedDeviceId = -1;
	}
}
