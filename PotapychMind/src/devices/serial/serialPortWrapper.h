#include <string>

#define STRICT
#include <tchar.h>
#include <windows.h>
#include "serial.h"

#include <mutex>

struct SerialPortWrapper
{
protected:
    CSerial serialPort;
    std::mutex servoMutex;

public:
    bool connect(const std::wstring comPortName = L"COM13");
    void disconnect();
    void write(const void* pData, size_t iLen);

};