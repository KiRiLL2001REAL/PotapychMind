#include "cameraService.h"

#include "../utility.h"
#include <opencv2/highgui.hpp>

void CameraService::storeFrame(cv::Mat& frame)
{
	std::unique_lock<std::shared_mutex> lk(mut_);
	frame.copyTo(mCachedFrame);
	using namespace std::chrono;
	mCachedTimestamp = steady_clock::now().time_since_epoch().count();
}

void CameraService::runner()
{
	cv::Mat frame;
	while (mActiveFlag)
	{
		if (cap.read(frame))
			storeFrame(frame);
		cv::waitKey(1);
		std::this_thread::yield();
	}
	mCanDestroyThread = true;
}

CameraService::CameraService() :
	mCachedTimestamp(0),
	mCachedFrame(),
	mConnectedDeviceId(-1),
	mActiveFlag(false),
	mpRunnerThr(nullptr),
	mCanDestroyThread(true)
{
}

CameraService::~CameraService()
{
	stop();
	if (mpRunnerThr)
	{
		mpRunnerThr->join();
		delete mpRunnerThr;
	}
	mCachedFrame.release();
}

std::vector<devices::Device> CameraService::getCameraList()
{
	auto videoDevices = devices::DeviceEnumerator::getVideoDevicesMap();
	std::vector<devices::Device> result;
	Utility::mapToVec(videoDevices, result);
	return result;
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
	if (cap.open(deviceId, cv::CAP_ANY))
	{
		cap.set(cv::CAP_PROP_FRAME_WIDTH, reqWidth);
		cap.set(cv::CAP_PROP_FRAME_HEIGHT, reqHeight);
		if (cap.get(cv::CAP_PROP_FRAME_WIDTH) == reqWidth
			&& cap.get(cv::CAP_PROP_FRAME_HEIGHT) == reqHeight)
		{
			mConnectedDeviceId = deviceId;
		}
	}
	mutDeviceId_.unlock();

	if (getConnectedDeviceId() == -1)
		return false;

	mActiveFlag = true;
	mCanDestroyThread = false;
	mpRunnerThr = new std::thread(&CameraService::runner, this);

	return true;
}

void CameraService::stop()
{
	std::lock_guard<std::mutex> lk(mutDeviceId_);
	mActiveFlag = false;
	while (!mCanDestroyThread)
	{
		using namespace std::chrono_literals;
		std::this_thread::yield();
		std::this_thread::sleep_for(1ms);
	}
	if (cap.isOpened())
	{
		cap.release();
		mConnectedDeviceId = -1;
	}
}
