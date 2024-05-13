#include "cameraWindow.h"

#include "imgui.h"

void GUI::showCameraWindow(bool* p_open, ImageCv2GlAdapter& img)
{
	if (!ImGui::Begin("Camera", p_open))
	{
		ImGui::End();
		return;
	}

	img.imGuiDraw();

	ImGui::End();
}