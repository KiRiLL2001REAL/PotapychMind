#pragma once

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

#include "baseService.h"

class CameraService: public BaseService
{
protected:
	// Данные
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
	// блокирующая функция, смотри ::stop()
	~CameraService();

	// возвращает id подключённой камеры (-1 если не подключились)
	int getConnectedDeviceId();
	// копирует данные в указанную матрицу, вовзращает временную метку
	void getFrame(cv::Mat& dst, long long& timestamp);

	// в случае неудачного захвата камеры возвращает false, иначе - запускает поток захватчика кадров
	bool launch(int deviceId, int width, int height);
	// блокирующая функция, ожидает завершения потока
	virtual void stop();

};

