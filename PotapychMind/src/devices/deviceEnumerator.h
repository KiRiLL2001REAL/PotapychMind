#pragma once

// based on https://github.com/studiosi/OpenCVDeviceEnumerator solution
// refactored getDevicesMap function

#include <Windows.h>
#pragma comment(lib, "strmiids")
#include <vector>
#include <string>

namespace devices {

    struct Device final
    {
        int id;
        std::wstring devicePath;
        std::wstring deviceName;
    };

    class DeviceEnumerator final
    {
    public:
        static std::vector<Device> getDevices(const GUID deviceClass);
        static std::vector<Device> getVideoDevices();
        static std::vector<Device> getComDevices();
    };

}