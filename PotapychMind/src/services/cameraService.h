#pragma once

#include "../devices/deviceEnumerator.h"
#include <opencv2/core.hpp>
#include <shared_mutex>
#include <thread>

#include <vector>
#include "../devices/deviceEnumerator.h"

#include <opencv2/videoio.hpp>

class CameraService final
{
protected:
	mutable std::shared_mutex mut_;
	long long mCachedTimestamp;
	cv::Mat mCachedFrame;

	std::mutex mutDeviceId_;
	cv::VideoCapture cap;
	int mConnectedDeviceId;

	std::atomic<bool> mActiveFlag;
	std::thread* mpRunnerThr;
	std::atomic<bool> mCanDestroyThread;

	void storeFrame(cv::Mat& frame);
	void runner();

public:
	CameraService();
	// ����������� �������, ������ ::stop()
	~CameraService();

	// �������� ������ ���� ����� � �������
	std::vector<devices::Device> getCameraList();

	// ���������� id ������������ ������ (-1 ���� �� ������������)
	int getConnectedDeviceId();
	// �������� ������ � ��������� �������, ���������� ��������� �����
	void getFrame(cv::Mat& dst, long long& timestamp);

	// � ������ ���������� ������� ������ ���������� false, ����� - ��������� ����� ���������� ������
	bool launch(int deviceId, int width, int height);
	// ����������� �������, ������� ���������� ������
	void stop();

};

