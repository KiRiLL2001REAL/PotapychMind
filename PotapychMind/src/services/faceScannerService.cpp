#include "faceScannerService.h"

#include <exception>

//#include <opencv2/imgproc.hpp>
#include "../utility.h"

#include "facedetectcnn.h"
//define the buffer size. Do not change the size!
//0x9000 = 1024 * (16 * 2 + 4), detect 1024 face at most
#ifndef DETECT_BUFFER_SIZE
#define DETECT_BUFFER_SIZE 0x9000
#endif

void FaceScannerService::getFrame(cv::Mat& dst)
{
    std::lock_guard<std::mutex> lk(mutFrame_);
    mFrame.copyTo(dst);
    mFrameHandled = true;
}

void FaceScannerService::storeDetectionResult(cv::Mat& src, std::vector<Face>& faces)
{
    std::unique_lock<std::shared_mutex> lk(mutFaces_);
    mCachedDetectionResult = faces;
    src.copyTo(mCachedFrame);
}

void FaceScannerService::runner()
{
    pTrace->Register_Thread(TM("FaceScannerServiceRunner"), 0);
    pTrace->P7_INFO(hModule, TM("Face-scanner thread started"));

    auto spin = []()
    {
        using namespace std::chrono_literals;
        std::this_thread::yield();
        std::this_thread::sleep_for(1ms);
    };

    int* pResults = NULL;
    //pBuffer is used in the detection functions.
    //If you call functions in multiple threads, please create one buffer for each thread!
    unsigned char* pBuffer = (unsigned char*)malloc(DETECT_BUFFER_SIZE);
    if (!pBuffer)
    {
        pTrace->P7_CRITICAL(hModule, TM("Can not allocate buffer"));
        throw std::runtime_error("Can not allocate buffer");
    }

    cv::Mat frame;
    std::vector<Face> detectionResult;
    while (mActiveFlag)
    {
        if (mFrameHandled)
        {
            spin();
            continue;
        }

        getFrame(frame);
        
        //!!! The input image must be a BGR one (three-channel) instead of RGB
        //!!! DO NOT RELEASE pResults !!!
        pResults = facedetect_cnn(pBuffer,
            (unsigned char*)(frame.ptr(0)),
            frame.cols, frame.rows, (int)frame.step);

        detectionResult.clear();
        for (int i = 0; i < (pResults ? *pResults : 0); i++)
        {
            short* p = ((short*)(pResults + 1)) + 16 * i;
            int confidence = p[0];
            if (confidence < 80)
                continue;
            int x = p[1];
            int y = p[2];
            int w = p[3];
            int h = p[4];
            if (x < 0) x = 0;
            if (y < 0) y = 0;
            if (x + w >= frame.cols)
                w = frame.cols - x - 1;
            if (y + h >= frame.rows)
                h = frame.rows - y - 1;

            Face face;
            face.roi = cv::Rect(x, y, w, h);

            face.peopleClass = PeopleClass::Stranger;
            if (faceComparator.isKnownFace(frame(face.roi)))
            {
                face.peopleClass = PeopleClass::Familiar;
            }

            if (!mFaceInterviewee.empty() && faceComparator
                .equality(frame(face.roi), mFaceInterviewee) >= 0.8)
            {
                face.peopleClass = PeopleClass::Interviewee;
            }

            detectionResult.emplace_back(face);
        }

        // если сетка обнаружила несколько интервьюируемых, нужно оставить одного.
        int largestIntervieweeIdx = -1;
        int largestArea = 0;
        for (int i = 0; i < detectionResult.size(); i++)
        {
            const auto& dr = detectionResult[i];
            if (dr.peopleClass == PeopleClass::Interviewee)
            {
                int area = dr.roi.area();
                if (area > largestArea)
                {
                    largestArea = area;
                    largestIntervieweeIdx = i;
                }
            }
        }
        for (int i = 0; i < detectionResult.size(); i++)
        {
            auto& dr = detectionResult[i];
            if (dr.peopleClass == PeopleClass::Interviewee && largestIntervieweeIdx != i)
                dr.peopleClass = PeopleClass::Familiar;
        }

        storeDetectionResult(frame, detectionResult);

        spin();
    }

    if (pBuffer)
        free(pBuffer);

    mCanDestroyThread = true;

    pTrace->P7_INFO(hModule, TM("Face-scanner thread finished"));
    pTrace->Unregister_Thread(0);
}

FaceScannerService::FaceScannerService():
    BaseService("FaceScannerService"),
    mCachedDetectionResult(),
    mFrameHandled(true),
    mFaceInterviewee(),
    faceComparator(Utility::getWorkingDirectory() + "/resources/face_comparator")
{
}

FaceScannerService::~FaceScannerService()
{
    {
        std::unique_lock<std::shared_mutex> lk(mutFaces_);
        mCachedFrame.release();
        mCachedDetectionResult.clear();
    }
    {
        std::lock_guard<std::mutex> lk(mutFrame_);
        mFrame.release();
        std::lock_guard<std::mutex> lk2(_mutInterviewee);
        mFaceInterviewee.release();
    }
}

void FaceScannerService::pushFrame(cv::Mat& src)
{
    std::lock_guard<std::mutex> lk(mutFrame_);
    src.copyTo(mFrame);
    mFrameHandled = false;
}

void FaceScannerService::getDetectionResult(cv::Mat& dstMat, std::vector<Face>& dstVec)
{
    std::shared_lock<std::shared_mutex> lk(mutFaces_);
    mCachedFrame.copyTo(dstMat);
    dstVec = mCachedDetectionResult;
}

void FaceScannerService::getDetectionResult(std::vector<Face>& dstVec)
{
    std::shared_lock<std::shared_mutex> lk(mutFaces_);
    dstVec = mCachedDetectionResult;
}

void FaceScannerService::exprungeDetectionResult()
{
    std::unique_lock<std::shared_mutex> lk(mutFaces_);
    mCachedFrame.release();
    mCachedDetectionResult.clear();
}

void FaceScannerService::intervieweeSet(cv::Mat face)
{
    std::lock_guard<std::mutex> lk(_mutInterviewee);
    if (!mFaceInterviewee.empty())
        mFaceInterviewee.release();
    face.copyTo(mFaceInterviewee);
    //cv::resize(face, mFaceInterviewee, cv::Size(92, 112));
}

void FaceScannerService::intervieweeReset()
{
    std::lock_guard<std::mutex> lk(_mutInterviewee);
    if (!mFaceInterviewee.empty())
        mFaceInterviewee.release();
}

void FaceScannerService::intervieweeRememberAndReset()
{
    std::lock_guard<std::mutex> lk(_mutInterviewee);
    if (!mFaceInterviewee.empty())
    {
        faceComparator.rememberFace(mFaceInterviewee);
        mFaceInterviewee.release();
    }
}

bool FaceScannerService::launch()
{
    pTrace->P7_INFO(hModule, TM("Launching service"));
    mActiveFlag = true;
    mCanDestroyThread = false;
    mThreadIsAlive = true;
    mpRunnerThr = new std::thread(&FaceScannerService::runner, this);
    pTrace->P7_INFO(hModule, TM("Service launched"));
    return true;
}

void FaceScannerService::stop()
{
    BaseService::stop();
}