#pragma once

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

#include "baseService.h"

class CameraService: public BaseService
{
protected:
	// ������
	mutable std::shared_mutex mut_;
	long long mCachedTimestamp;
	cv::Mat mCachedFrame;

	std::mutex mutDeviceId_;
	cv::VideoCapture mCap;
	int mConnectedDeviceId;

	void storeFrame(cv::Mat& frame);
	virtual void runner();

public:
	CameraService();
	// ����������� �������, ������ ::stop()
	~CameraService();

	// ���������� id ������������ ������ (-1 ���� �� ������������)
	int getConnectedDeviceId();
	// �������� ������ � ��������� �������, ���������� ��������� �����
	void getFrame(cv::Mat& dst, long long& timestamp);

	// � ������ ���������� ������� ������ ���������� false, ����� - ��������� ����� ���������� ������
	bool launch(int deviceId, int width, int height);
	// ����������� �������, ������� ���������� ������
	virtual void stop();

};

