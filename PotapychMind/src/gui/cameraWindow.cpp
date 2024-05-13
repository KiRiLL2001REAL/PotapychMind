#include "cameraWindow.h"

#include "imgui.h"

void GUI::showCameraWindow(bool* p_open, ImageCv2GlAdapter& img)
{
	static ImVec2 cachedRegionSize;
	static cv::Size cachedCameraFrameSize;
	static ImVec2 cachedCameraFrameScaledSize;

	if (!ImGui::Begin("Camera", p_open))
	{
		ImGui::End();
		return;
	}

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

	ImGui::Text("%dx%d", cachedCameraFrameSize.width, cachedCameraFrameSize.height);
	img.imGuiDraw(cachedCameraFrameScaledSize);

	ImGui::End();
}