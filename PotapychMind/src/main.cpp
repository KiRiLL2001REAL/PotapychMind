
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

#include <iostream>
#include "utility.h"
#include "devices/deviceEnumerator.h"
#include <vector>
#include <string>
#include <fstream>

#include <P7_Client.h>
#include <P7_Trace.h>

int main()
{

    IP7_Client* pClient = NULL;
    IP7_Trace* pTrace = NULL;
    IP7_Trace::hModule hModule = NULL;
    P7_Set_Crash_Handler();

    auto loggerDeInitialize = [&pClient, &pTrace]() {
        if (pTrace)
        {
            pTrace->Unregister_Thread(0); // может не сработать, если не дошли до создания потока, но здесь это не доставит проблем
            pTrace->Release();
            pTrace = NULL;
        }
        if (pClient)
        {
            pClient->Release();
            pClient = NULL;
        }
    };


    //create P7 client object
    pClient = P7_Create_Client(TM("/P7.Sink=FileTxt"));
    if (NULL == pClient)
    {
        std::cerr << "ERR : Can't create P7 client instance.\n";
        loggerDeInitialize();
        return -1;
    }

    //create P7 trace object 1
    pTrace = P7_Create_Trace(pClient, TM("Trace channel 1"));
    if (NULL == pTrace)
    {
        std::cerr << "ERR : Can't create P7 trace channel.\n";
        loggerDeInitialize();
        return -1;
    }
    pTrace->Register_Thread(TM("Application"), 0);
    pTrace->Register_Module(TM("Main"), &hModule);




    auto workingDirectory = Utility::getWorkingDirectory();
    if (workingDirectory.empty())
    {
        std::cerr << "ERR : Unable to get working directory\n";
        pTrace->P7_ERROR(hModule, TM("Unable to get working directory"));
        loggerDeInitialize();
        return -1;
    }
    std::cout << "INFO: Current working directory is '" << workingDirectory << "'\n";
    pTrace->P7_INFO(hModule, TM("Current working directory is '%s'"), Utility::to_wstring(workingDirectory).c_str());


    std::cout << "INFO: Video devices list:\n";
    pTrace->P7_INFO(hModule, TM("Video devices list:"));
    auto videoDevices = devices::DeviceEnumerator::getVideoDevicesMap();
    for (const auto& [id, dev] : videoDevices)
    {
        std::cout << "\tid: " << dev.id << "\tname: " << dev.deviceName << "\n";
        pTrace->P7_INFO(hModule, TM("\tid: %d\tname: %s"), dev.id, Utility::to_wstring(dev.deviceName).c_str());
    }


    /*std::vector<std::pair<size_t, size_t>> possible_resolutions;
    {
        auto filename = workingDirectory + "/resources/possible_camera_resolution.txt";
        std::ifstream inStream(filename);
        if (!inStream.is_open())
        {
            std::cerr << "ERR : Can't read file '" << filename << "'\n";
            pTrace->P7_ERROR(hModule, TM("Can't read file '%s'"), Utility::to_wstring(filename).c_str());
            loggerDeInitialize();
            return -1;
        }

        std::string line;
        size_t width, height;
        size_t lineCounter = 0;
        while (std::getline(inStream, line))
        {
            size_t separatorPos = line.find('*');
            if (separatorPos == line.npos) {
                std::cout << "WARN: Skip unknown resolution pattern at line " << lineCounter << ".\n";
                pTrace->P7_WARNING(hModule, TM("Skip unknown resolution pattern at line %d"), lineCounter);
            }
            width = atoi(line.substr(0, separatorPos).c_str());
            height = atoi(line.substr(separatorPos + 1).c_str());
            possible_resolutions.push_back(std::make_pair(width, height));

            lineCounter++;
        }

        inStream.close();
    }*/

    cv::VideoCapture cap;

    int deviceId = 0;
    int apiId = cv::CAP_ANY;

    cap.open(deviceId, apiId);
    if (!cap.isOpened())
    {
        std::cerr << "ERR : Unable to open camera!\n";
        pTrace->P7_ERROR(hModule, TM("Unable to open camera!"));
        loggerDeInitialize();
        return -1;
    }

    std::cout << "INFO: Opened camera " << deviceId << "\n";
    pTrace->P7_INFO(hModule, TM("Opened camera %d"), deviceId);
    std::cout << "INFO: Camera backend: " << cap.getBackendName() << "\n";
    pTrace->P7_INFO(hModule, TM("Camera backend: %s"), Utility::to_wstring(cap.getBackendName()).c_str());

    /*std::cout << "INFO: Analyzing possible resolutions... This operation may take some time.\n";
    pTrace->P7_INFO(hModule, TM("Analyzing possible resolutions... This operation may take some time"));

    std::vector<std::pair<size_t, size_t>> confirmed_resolutions;
    {
        for (const auto& res : possible_resolutions)
        {
            cap.set(cv::CAP_PROP_FRAME_WIDTH, (double)res.first);
            cap.set(cv::CAP_PROP_FRAME_HEIGHT, (double)res.second);
            if (res.first == (int)cap.get(cv::CAP_PROP_FRAME_WIDTH)
                && res.second == (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT))
                confirmed_resolutions.push_back(res);
        }
    }

    std::cout << "INFO: Here are list of confirmed resolutions:\n";
    pTrace->P7_INFO(hModule, TM("Here are list of confirmed resolutions:"));
    for (const auto& res : confirmed_resolutions)
    {
        std::cout << "\t" << res.first << "x" << res.second << "\n";
        pTrace->P7_INFO(hModule, TM("\t%dx%d"), res.first, res.second);
    }*/


    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);

    cv::Mat frame;

    while (true)
    {
        cap.read(frame);

        if (!frame.empty())
            cv::imshow("Camera", frame);
        else
        {
            std::cout << "WARN: Blank frame grabbed.\n";
            pTrace->P7_WARNING(hModule, TM("Blank frame grabbed"));
        }

        int keyCode = cv::waitKey(1);
        if (keyCode >= 0)
            break;
    }

    pTrace->P7_INFO(hModule, TM("Program finished"));

    loggerDeInitialize();
}