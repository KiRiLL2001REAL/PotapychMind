#include "faceComparatorHandler.h"

#include <opencv2/imgproc.hpp>
#include <vector>

#include <cppflow/ops.h>

float FaceComparatorHandler::equality(const cppflow::tensor& t1, const cppflow::tensor& t2)
{
    std::lock_guard<std::mutex> lk(_mutModel);

    cppflow::tensor output;

    try
    {
        output = (*mModel)(
            { {"serving_default_input_1:0", t1}, {"serving_default_input_2:0", t2} },
            { "StatefulPartitionedCall:0" })[0];
    }
    catch (std::runtime_error& e)
    {
        std::cout << e.what() << std::endl;
        return 0;
    }

    return output.get_data<float>()[0];
}

cppflow::tensor FaceComparatorHandler::makeTensorFromMat(cv::Mat img)
{
    cv::Mat res;
    cv::resize(img, res, cv::Size(92, 112));
    cv::cvtColor(res, res, cv::COLOR_BGR2RGB);

    const std::vector<int> sz = { res.rows, res.cols, res.channels() };
    const std::vector<int64_t> sz_ = { res.rows, res.cols, res.channels() };
    const int size = sz[0] * sz[1] * sz[2];

    auto tensor = cppflow::tensor(std::vector<uint8_t>(res.data, res.data + size), sz_);
    res.release();

    tensor = cppflow::cast(tensor, TF_UINT8, TF_FLOAT);
    tensor = tensor / 255.f;
    tensor = cppflow::expand_dims(tensor, 0);

    return tensor;
}

FaceComparatorHandler::FaceComparatorHandler(
    const std::string& folder_name
) {
    mModel = new cppflow::model(folder_name);
}

FaceComparatorHandler::~FaceComparatorHandler()
{
    std::lock_guard<std::mutex> lk(_mutModel);
    std::unique_lock<std::shared_mutex> lk2(_mutFaces);
    delete mModel;
    mKnownFaceTensors.clear();
}

float FaceComparatorHandler::equality(cv::Mat face1, cv::Mat face2)
{
    auto tensor1 = makeTensorFromMat(face1);
    auto tensor2 = makeTensorFromMat(face2);

    return equality(tensor1, tensor2);
}

void FaceComparatorHandler::rememberFace(cv::Mat face)
{
    std::unique_lock<std::shared_mutex> lk(_mutFaces);
    mKnownFaceTensors.emplace_back(makeTensorFromMat(face));
}

bool FaceComparatorHandler::isKnownFace(cv::Mat face)
{
    std::shared_lock<std::shared_mutex> lk(_mutFaces);
    auto original = makeTensorFromMat(face);
    for (auto& t : mKnownFaceTensors)
        if (equality(original, t) >= 0.8)
            return true;
    return false;
}
