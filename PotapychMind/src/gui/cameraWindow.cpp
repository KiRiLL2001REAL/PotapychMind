#include "cameraWindow.h"

#include "imgui.h"

void GUI::showCameraWindow(bool* p_open, ImageCv2GlAdapter& img)
{
    static ImVec2 cachedRegionSize;
    static cv::Size cachedCameraFrameSize;
    static ImVec2 cachedCameraFrameScaledSize;
    static bool anyFrameReceived = false;

    char buffered_window_name[128];
    sprintf_s(buffered_window_name, "Camera %dx%d###CameraViewport",
        cachedCameraFrameSize.width, cachedCameraFrameSize.height);
    
    if (anyFrameReceived)
        ImGui::SetNextWindowSize(cachedCameraFrameScaledSize, ImGuiCond_Once);
    if (!ImGui::Begin(buffered_window_name, p_open))
    {
        ImGui::End();
        return;
    }

    anyFrameReceived = anyFrameReceived || !img.size().empty();

    // Подстройка размера кадра по размеру региона окна
    auto regionSize = ImGui::GetContentRegionAvail();
    auto frameSize = img.size();

    if (cachedRegionSize.x != regionSize.x && cachedRegionSize.y != regionSize.y
        || cachedCameraFrameSize != frameSize)
    {
        float scale = min(regionSize.x / frameSize.width, regionSize.y / frameSize.height);

        cachedRegionSize = regionSize;
        cachedCameraFrameSize = frameSize;
        cachedCameraFrameScaledSize = ImVec2(
            cachedCameraFrameSize.width * scale,
            cachedCameraFrameSize.height * scale);
    }

    img.imGuiDraw(cachedCameraFrameScaledSize);

    ImGui::End();
}