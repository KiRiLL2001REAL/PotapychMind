#include <string>

namespace SerialPortUtility
{
    bool connect(const std::wstring comPortName = L"COM13");
    void disconnect();
    void write(const void *pData, size_t iLen);
}