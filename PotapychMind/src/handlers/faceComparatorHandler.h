#pragma once

#include <opencv2/core.hpp>

#include <cppflow/model.h>

#include <string>
#include <mutex>
#include <shared_mutex>

class FaceComparatorHandler
{
protected:
    std::mutex _mutModel;
    cppflow::model* mModel;

    mutable std::shared_mutex _mutFaces;
    std::vector<cppflow::tensor> mKnownFaceTensors;
    
    float equality(const cppflow::tensor& t1, const cppflow::tensor& t2);

    cppflow::tensor makeTensorFromMat(cv::Mat img);

public:
    explicit FaceComparatorHandler(const std::string& folder_name);
    ~FaceComparatorHandler();

    float equality(cv::Mat face1, cv::Mat face2);
    void rememberFace(cv::Mat face);
    bool isKnownFace(cv::Mat face);
};