#pragma once

#include <opencv2/core.hpp>
#include <shared_mutex>
#include <thread>
#include <vector>
#include <string>

#include <P7_Client.h>
#include <P7_Trace.h>

#include "cameraService.h"

enum class PeopleClass
{
    Stranger,    // С таким еще не виделись
    Interviewee, // С этим общаемся
    Familiar     // Этого уже видели
};

struct Face
{
    cv::Rect roi;
    PeopleClass peopleClass;
};

class FaceScannerService final
{
protected:
    // Логгинг
    IP7_Client* pClient;
    IP7_Trace* pTrace;
    IP7_Trace::hModule hModule;

    // Данные
    mutable std::shared_mutex mutFaces_;
    std::vector<Face> mCachedDetectionResult;
    cv::Mat mCachedFrame;

    std::mutex mutFrame_;
    cv::Mat mFrame;
    std::atomic<bool> mFrameHandled;

    // Потоки
    std::atomic<bool> mActiveFlag;
    std::thread* mpRunnerThr;
    std::atomic<bool> mCanDestroyThread;
    bool mThreadIsAlive;

    void getFrame(cv::Mat& dst);
    void storeDetectionResult(cv::Mat& src, std::vector<Face>& faces);
    void runner();

public:
    FaceScannerService();
    ~FaceScannerService();

    // Изображение должно быть в формате BGR
    void pushFrame(cv::Mat& src);

    void getDetectionResult(cv::Mat& dstMat, std::vector<Face>& dstVec);
    void getDetectionResult(std::vector<Face>& dstVec);

    bool launch();
    void stop();
};

