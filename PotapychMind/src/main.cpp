
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

#include <iostream>
#include "utility.h"
#include "devices/deviceEnumerator.h"
#include <vector>
#include <string>
#include <fstream>

int main()
{
    auto workingDirectory = Utility::getWorkingDirectory();
    if (workingDirectory.empty())
    {
        std::cerr << "ERR : Unable to get working directory\n";
        return -1;
    }
    std::cout << "INFO: Current working directory is '" << workingDirectory << "'\n";


    std::cout << "INFO: Video devices list:\n";
    auto videoDevices = devices::DeviceEnumerator::getVideoDevicesMap();
    for (const auto& [id, dev] : videoDevices)
        std::cout << "\tid: " << dev.id << "\tname: " << dev.deviceName << "\n";


    /*std::vector<std::pair<size_t, size_t>> possible_resolutions;
    {
        auto filename = workingDirectory + "/resources/possible_camera_resolution.txt";
        std::ifstream inStream(filename);
        if (!inStream.is_open())
        {
            std::cerr << "ERR : Can't read file '" << filename << "'\n";
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
        return -1;
    }

    std::cout << "INFO: Opened camera " << deviceId << "\n";
    std::cout << "INFO: Camera backend: " << cap.getBackendName() << "\n";

    /*std::cout << "INFO: Analyzing possible resolutions... This operation may take some time.\n";

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
    for (const auto& res : confirmed_resolutions)
        std::cout << "\t" << res.first << "x" << res.second << "\n";*/


    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);

    cv::Mat frame;

    while (true)
    {
        cap.read(frame);

        if (!frame.empty())
            cv::imshow("Camera", frame);
        else
            std::cout << "WARN: Blank frame grabbed.\n";

        int keyCode = cv::waitKey(1);
        if (keyCode >= 0)
            break;
    }
}