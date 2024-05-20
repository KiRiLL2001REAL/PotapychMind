#pragma once

#include <opencv2/core.hpp>
#include <vector>

#include "baseService.h"

enum class PeopleClass
{
    Stranger,    // � ����� ��� �� ��������
    Interviewee, // � ���� ��������
    Familiar     // ����� ��� ������
};

struct Face
{
    cv::Rect roi;
    PeopleClass peopleClass;
};

class FaceScannerService : public BaseService
{
protected:
    // ������
    mutable std::shared_mutex mutFaces_;
    std::vector<Face> mCachedDetectionResult;
    cv::Mat mCachedFrame;

    std::mutex mutFrame_;
    cv::Mat mFrame;
    std::atomic<bool> mFrameHandled;

    void getFrame(cv::Mat& dst);
    void storeDetectionResult(cv::Mat& src, std::vector<Face>& faces);
    virtual void runner();

public:
    FaceScannerService();
    ~FaceScannerService();

    // ����������� ������ ���� � ������� BGR
    void pushFrame(cv::Mat& src);

    void getDetectionResult(cv::Mat& dstMat, std::vector<Face>& dstVec);
    void getDetectionResult(std::vector<Face>& dstVec);

    bool launch();
    virtual void stop();
};