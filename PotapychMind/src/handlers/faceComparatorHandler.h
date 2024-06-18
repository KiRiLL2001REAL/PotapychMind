#pragma once

#include <opencv2/core.hpp>

#include <cppflow/model.h>

#include <string>
#include <mutex>

class FaceComparatorHandler
{
protected:
    std::mutex _mutModel;
    cppflow::model* mModel;

public:
    explicit FaceComparatorHandler(const std::string& folder_name);
    ~FaceComparatorHandler();

    float equality(cv::Mat m1, cv::Mat m2);
};