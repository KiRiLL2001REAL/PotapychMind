#include "deviceEnumerator.h"

#include <dshow.h>

std::string devices::DeviceEnumerator::ConvertBSTRToMBS(BSTR bstr)
{
	int wslen = SysStringLen(bstr);
	return ConvertWCSToMBS((wchar_t*)bstr, wslen);
}

std::string devices::DeviceEnumerator::ConvertWCSToMBS(const wchar_t* pstr, long wslen)
{
	int len = ::WideCharToMultiByte(CP_ACP, 0, pstr, wslen, NULL, 0, NULL, NULL);

	std::string dblstr(len, '\0');
	len = ::WideCharToMultiByte(CP_ACP, 0 /* no flags */,
		pstr, wslen /* not necessary NULL-terminated */,
		&dblstr[0], len,
		NULL, NULL /* no default char */);

	return dblstr;
}

std::map<int, devices::Device> devices::DeviceEnumerator::getDevicesMap(const GUID deviceClass)
{
	std::map<int, Device> deviceMap;

	HRESULT hr = CoInitialize(nullptr);
	if (FAILED(hr))
		return deviceMap; // Empty deviceMap as an error

	// Create the System Device Enumerator
	ICreateDevEnum* pDevEnum;
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));

	if (FAILED(hr))
		return deviceMap; // Empty deviceMap as an error

	// If succeeded, create an enumerator for the category
	IEnumMoniker* pEnum = NULL;
	hr = pDevEnum->CreateClassEnumerator(deviceClass, &pEnum, 0);
	hr = (hr == S_FALSE ? VFW_E_NOT_FOUND : hr);
	pDevEnum->Release();

	if (FAILED(hr)) // Now we check if the enumerator creation succeeded
		return deviceMap; // Empty deviceMap as an error

	int deviceId = -1;
	// Fill the map with id and friendly device name
	IMoniker* pMoniker = NULL;
	while (pEnum->Next(1, &pMoniker, NULL) == S_OK)
	{
		IPropertyBag* pPropBag;
		hr = pMoniker->BindToStorage(0, NULL, IID_PPV_ARGS(&pPropBag));
		if (FAILED(hr))
		{
			pMoniker->Release();
			continue;
		}

		// Create variant to hold data
		VARIANT var;
		VariantInit(&var);

		std::string deviceName;
		std::string devicePath;

		// Read FriendlyName or Description
		hr = pPropBag->Read(L"Description", &var, 0); // Read description
		if (FAILED(hr)) // If description fails, try with the friendly name
			hr = pPropBag->Read(L"FriendlyName", &var, 0);
		
		if (FAILED(hr)) // If still fails, continue with next device
		{
			VariantClear(&var);
			continue;
		}
		else // Convert to string
			deviceName = ConvertBSTRToMBS(var.bstrVal);

		VariantClear(&var); // We clean the variable in order to read the next value

		// We try to read the DevicePath
		hr = pPropBag->Read(L"DevicePath", &var, 0);
		if (FAILED(hr)) // If it fails we continue with next device
		{
			VariantClear(&var);
			continue; 
		}
		else // Convert to string
			devicePath = ConvertBSTRToMBS(var.bstrVal);

		// We populate the map
		deviceId++;
		Device currentDevice;
		currentDevice.id = deviceId;
		currentDevice.deviceName = deviceName;
		currentDevice.devicePath = devicePath;
		deviceMap[deviceId] = currentDevice;

	}
	pEnum->Release();

	CoUninitialize();
	return deviceMap;
}

std::map<int, devices::Device> devices::DeviceEnumerator::getVideoDevicesMap()
{
	return getDevicesMap(CLSID_VideoInputDeviceCategory);
}

std::map<int, devices::Device> devices::DeviceEnumerator::getAudioDevicesMap()
{
	return getDevicesMap(CLSID_AudioInputDeviceCategory);
}
