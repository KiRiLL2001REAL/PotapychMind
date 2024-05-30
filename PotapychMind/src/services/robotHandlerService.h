#pragma once

#include "baseService.h"

#include "../devices/serial/serialPortWrapper.h"
#include <mutex>
#include <queue>
#include <string>

class RobotHandlerService : public BaseService
{
protected:
	std::mutex mutSerial_;
	SerialPortWrapper mSerial;

	std::mutex mutCmd_;
	std::queue<std::pair<int, int>> mCmdQ;

	void popQ(std::pair<int, int>& dst);
	virtual void runner();

public:
	RobotHandlerService();
	~RobotHandlerService();

	const std::wstring& getConnectedComPortName();

	/*
	first second explain
	-1    n      wait for n millis
	k>=0  x      translate k'th motor to position x
	*/
	void putCmd(const std::pair<int, int>& cmd);

	bool launch(const std::wstring& comName);
	virtual void stop();
};

