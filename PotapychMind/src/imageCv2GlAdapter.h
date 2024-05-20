#pragma once

#include <opencv2/core.hpp>

#include "imgui.h"
#include <windows.h>
#include <GL/GL.h>
#include <shared_mutex>

// ����� ��� ����������� cv::Mat � ��������� opengl
// ��������� ������ ����������� ���� CV_8UC1 � CV_8UC3.
class ImageCv2GlAdapter final
{
protected:
    mutable std::shared_mutex mut_;
    cv::Mat mMat;
    GLuint mTextureId;

    ImageCv2GlAdapter(const ImageCv2GlAdapter&) = delete;
    void operator=(const ImageCv2GlAdapter&) = delete;

public:
    ImageCv2GlAdapter();
    // �������� ������� � ����
    ImageCv2GlAdapter(cv::Mat& mat);
    ~ImageCv2GlAdapter();

    void updateImage(cv::Mat& mat);
    void imGuiDraw(ImVec2 size = {0, 0});

    cv::Size size() const;
};

