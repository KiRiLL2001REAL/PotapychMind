#include "deviceEnumerator.h"

#include <dshow.h>
#include <winspool.h>
#include <tchar.h>

#include <iostream> // temporary

std::vector<devices::Device> devices::DeviceEnumerator::getDevices(const GUID deviceClass)
{
    std::vector<Device> deviceVec;

    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr))
        return deviceVec; // Empty deviceMap as an error

    // Create the System Device Enumerator
    ICreateDevEnum* pDevEnum;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));

    if (FAILED(hr))
        return deviceVec; // Empty deviceMap as an error

    // If succeeded, create an enumerator for the category
    IEnumMoniker* pEnum = NULL;
    hr = pDevEnum->CreateClassEnumerator(deviceClass, &pEnum, 0);
    hr = (hr == S_FALSE ? VFW_E_NOT_FOUND : hr);
    pDevEnum->Release();

    if (FAILED(hr)) // Now we check if the enumerator creation succeeded
        return deviceVec; // Empty deviceMap as an error

    int deviceId = -1;
    // Fill the map with id and friendly device name
    IMoniker* pMoniker = NULL;
    while (pEnum->Next(1, &pMoniker, NULL) == S_OK)
    {
        IPropertyBag* pPropBag;
#pragma warning ( push )
#pragma warning ( disable : 6387 )
        hr = pMoniker->BindToStorage(0, NULL, IID_PPV_ARGS(&pPropBag));
#pragma warning ( pop )
        if (FAILED(hr))
        {
            pMoniker->Release();
            continue;
        }

        // Create variant to hold data
        VARIANT var;
        VariantInit(&var);

        std::wstring deviceName;
        std::wstring devicePath;

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
        {
            size_t len = _tcslen(var.bstrVal);
            deviceName = std::wstring(var.bstrVal, var.bstrVal + len);
        }

        VariantClear(&var); // We clean the variable in order to read the next value

        // We try to read the DevicePath
        hr = pPropBag->Read(L"DevicePath", &var, 0);
        if (FAILED(hr)) // If it fails we continue with next device
        {
            VariantClear(&var);
            continue; 
        }
        else // Convert to string
        {
            size_t len = _tcslen(var.bstrVal);
            devicePath = std::wstring(var.bstrVal, var.bstrVal + len);
        }

        // We populate the map
        deviceId++;
        Device currentDevice;
        currentDevice.id = deviceId;
        currentDevice.deviceName = deviceName;
        currentDevice.devicePath = devicePath;
        deviceVec.emplace_back(currentDevice);

    }
    pEnum->Release();

    CoUninitialize();
    return deviceVec;
}

std::vector<devices::Device> devices::DeviceEnumerator::getVideoDevices()
{
    return getDevices(CLSID_VideoInputDeviceCategory);
}

std::vector<devices::Device> devices::DeviceEnumerator::getComDevices()
{
    auto isNumeric = [](LPCWSTR pszString, bool bIgnoreColon)
    {
        const size_t nLen = wcslen(pszString);
        if (nLen == 0)
            return false;
        bool bNumeric = true;
        for (size_t i = 0; i < nLen && bNumeric; i++)
        {
            if ((pszString[i] == L':') && bIgnoreColon)
            {
                bNumeric = true;
                continue;
            }
            bNumeric = (iswdigit(pszString[i]) != 0);
        }
        return bNumeric;
    };

    std::vector<Device> deviceVec;

    DWORD bufferSize = 0;
    DWORD portsAvailable = 0;
    
    if (!EnumPorts(nullptr, 2, nullptr, 0, &bufferSize, &portsAvailable))
    {
        const DWORD error = GetLastError();
        if (error != ERROR_INSUFFICIENT_BUFFER)
            return deviceVec; // что-то пошло не так, функци€ не работает
    }

    BYTE* pBufferData = new BYTE[bufferSize];
    bool success = EnumPorts(nullptr, 2, pBufferData, bufferSize, &bufferSize, &portsAvailable);
    if (success)
    {
        auto pPortInfo = reinterpret_cast<const PORT_INFO_2*>(pBufferData);
        for (DWORD i = 0; i < portsAvailable; i++)
        {
            // »щем шаблон COMX, где X - число (минимум 4 символа)
#pragma warning ( push )
#pragma warning ( disable : 6385 )
            size_t nameLen = _tcslen(pPortInfo[i].pPortName);
#pragma warning ( pop )
            if (nameLen <= 3)
                continue;
            if (_tcsnicmp(pPortInfo[i].pPortName, _T("COM"), 3) != 0)
                continue;
            if (!isNumeric(&pPortInfo[i].pPortName[3], true))
                continue;

            const int nPort = _ttoi(&pPortInfo[i].pPortName[3]);
            size_t len;
            len = _tcslen(pPortInfo[i].pPortName);
            const auto wPath = L"\\\\.\\" + std::wstring(pPortInfo[i].pPortName, pPortInfo[i].pPortName + len - 1);
            len = _tcslen(pPortInfo[i].pDescription);
            const auto wDesc = std::wstring(pPortInfo[i].pDescription, pPortInfo[i].pDescription + len);

            Device currentDevice;
            currentDevice.id = nPort;
            currentDevice.devicePath = wPath;
            currentDevice.deviceName = wDesc;
            deviceVec.emplace_back(currentDevice);
        }
    }
    delete[] pBufferData;

    return deviceVec;
}
