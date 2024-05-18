#include "imageCv2GlAdapter.h"

#include <exception>

ImageCv2GlAdapter::ImageCv2GlAdapter():
    mMat(),
    mTextureId(0)
{
    glGenTextures(1, &mTextureId);
}

ImageCv2GlAdapter::ImageCv2GlAdapter(cv::Mat& mat):
    ImageCv2GlAdapter()
{
    updateImage(mat);
}

ImageCv2GlAdapter::~ImageCv2GlAdapter()
{
    std::unique_lock<std::shared_mutex> lk(mut_);
    mMat.release();

    if (mTextureId)
        glDeleteTextures(1, &mTextureId);
    mTextureId = 0;
}

void ImageCv2GlAdapter::updateImage(cv::Mat& mat)
{
    std::unique_lock<std::shared_mutex> lk(mut_);
    if (mat.empty() || mat.channels() == 2 || mat.channels() > 3)
    {
        //TODO logging
        return;
    }

    // Обновляем матрицу и текстуру
    bool needTextureRealloc =
        (mMat.type() != mat.type())                        // Отличается тип
        || (mMat.cols != mat.cols || mMat.rows != mat.rows // Отличается размерность
            || mMat.dims != mat.dims)
        || (mMat.data != mat.data);                        // Отличаются указатели на данные

    mat.copyTo(mMat);

    glBindTexture(GL_TEXTURE_2D, mTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    //TODO всё это долго (около 1.25мс для 1280х720), рассмотреть PBO https://www.songho.ca/opengl/gl_pbo.html
    if (mMat.channels() == 1)
    {
        if (needTextureRealloc)
            glTexImage2D(GL_TEXTURE_2D, 0,
                GL_LUMINANCE, mMat.cols, mMat.rows,
                0, GL_RED, GL_UNSIGNED_BYTE, mMat.data);
        else
            glTexSubImage2D(GL_TEXTURE_2D, 0,
                GL_LUMINANCE, mMat.cols, mMat.rows,
                0, GL_RED, GL_UNSIGNED_BYTE, mMat.data);
    }
    else
    {
        if (needTextureRealloc)
            glTexImage2D(GL_TEXTURE_2D, 0,
                GL_RGB, mMat.cols, mMat.rows,
                0, GL_BGR_EXT, GL_UNSIGNED_BYTE, mMat.data);
        else
            glTexSubImage2D(GL_TEXTURE_2D, 0,
                GL_RGB, mMat.cols, mMat.rows,
                0, GL_BGR_EXT, GL_UNSIGNED_BYTE, mMat.data);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ImageCv2GlAdapter::imGuiDraw(ImVec2 size)
{
    std::shared_lock<std::shared_mutex> lk(mut_);
    if (mMat.empty())
    {
        //TODO logging
        return;
    }
    glBindTexture(GL_TEXTURE_2D, mTextureId);
    if (0 == size.x and size.x == size.y)
        ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(mTextureId)), ImVec2(mMat.cols, mMat.rows));
    else
        ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(mTextureId)), size);
    glBindTexture(GL_TEXTURE_2D, 0);
}

const cv::Size& ImageCv2GlAdapter::size() const
{
    std::shared_lock<std::shared_mutex> lk(mut_);
    return mMat.size();
}
