#include "serialPortWrapper.h"

#include <fstream>

bool SerialPortWrapper::connect(const std::wstring comPortName)
{
    LPCWSTR w_str = comPortName.c_str();

    std::lock_guard<std::mutex> lk(servoMutex);
    LONG result = serialPort.Open(w_str);
    if (ERROR_SUCCESS != result)
        return false;
    serialPort.Setup(CSerial::EBaud9600, CSerial::EData8, CSerial::EParNone, CSerial::EStop1);
    return true;
}

void SerialPortWrapper::disconnect()
{
    std::lock_guard<std::mutex> lk(servoMutex);
    serialPort.Close();
}

void SerialPortWrapper::write(const void* pData, size_t iLen)
{
    std::lock_guard<std::mutex> lk(servoMutex);
    serialPort.Write(pData, iLen);
}
