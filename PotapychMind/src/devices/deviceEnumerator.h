#pragma once

// based on https://github.com/studiosi/OpenCVDeviceEnumerator solution
// refactored getDevicesMap function

#include <Windows.h>
#pragma comment(lib, "strmiids")
#include <map>
#include <string>

namespace devices {

	struct Device final
	{
		int id;
		std::string devicePath;
		std::string deviceName;
	};


	class DeviceEnumerator final
	{
	private:
		static std::string ConvertBSTRToMBS(BSTR bstr);
		static std::string ConvertWCSToMBS(const wchar_t* pstr, long wslen);
	public:
		static std::map<int, Device> getDevicesMap(const GUID deviceClass);
		static std::map<int, Device> getVideoDevicesMap();
		static std::map<int, Device> getAudioDevicesMap();
	};

}