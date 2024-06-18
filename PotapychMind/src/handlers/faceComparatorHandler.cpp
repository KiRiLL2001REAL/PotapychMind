#include "faceComparatorHandler.h"

#include <opencv2/imgproc.hpp>
#include <vector>

#include <cppflow/ops.h>

FaceComparatorHandler::FaceComparatorHandler(
    const std::string& folder_name
) {
    mModel = new cppflow::model(folder_name);
}

FaceComparatorHandler::~FaceComparatorHandler()
{
    std::lock_guard<std::mutex> lk(_mutModel);
    delete mModel;
}

float FaceComparatorHandler::equality(cv::Mat m1, cv::Mat m2)
{
    cv::Mat res1, res2;
    cv::resize(m1, res1, cv::Size(92, 112));
    cv::resize(m2, res2, cv::Size(92, 112));
    cv::cvtColor(res1, res1, cv::COLOR_BGR2RGB);
    cv::cvtColor(res2, res2, cv::COLOR_BGR2RGB);

    const std::vector<int> sz = { res1.rows, res1.cols, res1.channels() };
    const std::vector<int64_t> sz_ = { res1.rows, res1.cols, res1.channels() };
    const int size = sz[0] * sz[1] * sz[2];

    auto tensor1 = cppflow::tensor(std::vector<uint8_t>(res1.data, res1.data + size), sz_);
    auto tensor2 = cppflow::tensor(std::vector<uint8_t>(res2.data, res2.data + size), sz_);
    res1.release();
    res2.release();
    tensor1 = cppflow::cast(tensor1, TF_UINT8, TF_FLOAT);
    tensor2 = cppflow::cast(tensor2, TF_UINT8, TF_FLOAT);
    tensor1 = tensor1 / 255.f;
    tensor2 = tensor2 / 255.f;
    tensor1 = cppflow::expand_dims(tensor1, 0);
    tensor2 = cppflow::expand_dims(tensor2, 0);

    cppflow::tensor output;

    try
    {
        output = (*mModel)(
            { {"serving_default_input_1:0", tensor1}, {"serving_default_input_2:0", tensor2} },
            { "StatefulPartitionedCall:0" })[0];
    }
    catch (std::runtime_error& e)
    {
        std::cout << e.what() << std::endl;
        return 0;
    }
    
    return output.get_data<float>()[0];
}
