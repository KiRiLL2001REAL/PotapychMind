#pragma once

#include "../devices/deviceEnumerator.h"
#include <opencv2/core.hpp>
#include <shared_mutex>
#include <thread>

#include <vector>
#include "../devices/deviceEnumerator.h"

#include <opencv2/videoio.hpp>

#include <P7_Client.h>
#include <P7_Trace.h>

class CameraService final
{
protected:
	IP7_Client* pClient;
	IP7_Trace* pTrace;
	IP7_Trace::hModule hModule;

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
	// блокирующая функция, смотри ::stop()
	~CameraService();

	// получить список всех камер в системе
	std::vector<devices::Device> getCameraList();

	// возвращает id подключённой камеры (-1 если не подключились)
	int getConnectedDeviceId();
	// копирует данные в указанную матрицу, вовзращает временную метку
	void getFrame(cv::Mat& dst, long long& timestamp);

	// в случае неудачного захвата камеры возвращает false, иначе - запускает поток захватчика кадров
	bool launch(int deviceId, int width, int height);
	// блокирующая функция, ожидает завершения потока
	void stop();

};

