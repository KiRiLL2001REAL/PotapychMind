

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

#include <iostream>
#include "utility.h"
#include "devices/deviceEnumerator.h"
#include <vector>
#include <string>
#include <fstream>

#include <P7_Client.h>
#include <P7_Trace.h>

//    std::cout << "INFO: Video devices list:\n";
//    pTrace->P7_INFO(hModule, TM("Video devices list:"));
//    auto videoDevices = devices::DeviceEnumerator::getVideoDevicesMap();
//    for (const auto& [id, dev] : videoDevices)
//    {
//        std::cout << "\tid: " << dev.id << "\tname: " << dev.deviceName << "\n";
//        pTrace->P7_INFO(hModule, TM("\tid: %d\tname: %s"), dev.id, Utility::to_wstring(dev.deviceName).c_str());
//    }
//
//
//    /*std::vector<std::pair<size_t, size_t>> possible_resolutions;
//    {
//        auto filename = workingDirectory + "/resources/possible_camera_resolution.txt";
//        std::ifstream inStream(filename);
//        if (!inStream.is_open())
//        {
//            std::cerr << "ERR : Can't read file '" << filename << "'\n";
//            pTrace->P7_ERROR(hModule, TM("Can't read file '%s'"), Utility::to_wstring(filename).c_str());
//            loggerDeInitialize();
//            return -1;
//        }
//
//        std::string line;
//        size_t width, height;
//        size_t lineCounter = 0;
//        while (std::getline(inStream, line))
//        {
//            size_t separatorPos = line.find('*');
//            if (separatorPos == line.npos) {
//                std::cout << "WARN: Skip unknown resolution pattern at line " << lineCounter << ".\n";
//                pTrace->P7_WARNING(hModule, TM("Skip unknown resolution pattern at line %d"), lineCounter);
//            }
//            width = atoi(line.substr(0, separatorPos).c_str());
//            height = atoi(line.substr(separatorPos + 1).c_str());
//            possible_resolutions.push_back(std::make_pair(width, height));
//
//            lineCounter++;
//        }
//
//        inStream.close();
//    }*/
//
//
//    /*std::cout << "INFO: Analyzing possible resolutions... This operation may take some time.\n";
//    pTrace->P7_INFO(hModule, TM("Analyzing possible resolutions... This operation may take some time"));
//
//    std::vector<std::pair<size_t, size_t>> confirmed_resolutions;
//    {
//        for (const auto& res : possible_resolutions)
//        {
//            cap.set(cv::CAP_PROP_FRAME_WIDTH, (double)res.first);
//            cap.set(cv::CAP_PROP_FRAME_HEIGHT, (double)res.second);
//            if (res.first == (int)cap.get(cv::CAP_PROP_FRAME_WIDTH)
//                && res.second == (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT))
//                confirmed_resolutions.push_back(res);
//        }
//    }
//
//    std::cout << "INFO: Here are list of confirmed resolutions:\n";
//    pTrace->P7_INFO(hModule, TM("Here are list of confirmed resolutions:"));
//    for (const auto& res : confirmed_resolutions)
//    {
//        std::cout << "\t" << res.first << "x" << res.second << "\n";
//        pTrace->P7_INFO(hModule, TM("\t%dx%d"), res.first, res.second);
//    }*/


#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/GL.h>
#include <tchar.h>

#include "services/cameraService.h"
#include "imageCv2GlAdapter.h"
#include "gui/cameraWindow.h"

// Data stored per platform window
struct WGL_WindowData { HDC hDC; };

// Data
static HGLRC            g_hRC;
static WGL_WindowData   g_MainWindow;
static int              g_Width;
static int              g_Height;

// Forward declarations of helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data);
void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data);
void ResetDeviceWGL();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Support function for multi-viewports
// Unlike most other backend combination, we need specific hooks to combine Win32+OpenGL.
// We could in theory decide to support Win32-specific code in OpenGL backend via e.g. an hypothetical ImGui_ImplOpenGL3_InitForRawWin32().
static void Hook_Renderer_CreateWindow(ImGuiViewport* viewport)
{
    assert(viewport->RendererUserData == NULL);

    WGL_WindowData* data = IM_NEW(WGL_WindowData);
    CreateDeviceWGL((HWND)viewport->PlatformHandle, data);
    viewport->RendererUserData = data;
}

static void Hook_Renderer_DestroyWindow(ImGuiViewport* viewport)
{
    if (viewport->RendererUserData != NULL)
    {
        WGL_WindowData* data = (WGL_WindowData*)viewport->RendererUserData;
        CleanupDeviceWGL((HWND)viewport->PlatformHandle, data);
        IM_DELETE(data);
        viewport->RendererUserData = NULL;
    }
}

static void Hook_Platform_RenderWindow(ImGuiViewport* viewport, void*)
{
    // Activate the platform window DC in the OpenGL rendering context
    if (WGL_WindowData* data = (WGL_WindowData*)viewport->RendererUserData)
        wglMakeCurrent(data->hDC, g_hRC);
}

static void Hook_Renderer_SwapBuffers(ImGuiViewport* viewport, void*)
{
    if (WGL_WindowData* data = (WGL_WindowData*)viewport->RendererUserData)
        ::SwapBuffers(data->hDC);
}

// Main code
int main(int, char**)
{
    // ========== Инициализация логгера ==========

    IP7_Client* pClient = NULL;
    IP7_Trace* pTrace = NULL;
    IP7_Trace::hModule hModule = NULL;
    P7_Set_Crash_Handler();

    auto loggerDeInitialize = [&pClient, &pTrace]() {
        if (pTrace)
        {
            pTrace->Unregister_Thread(0); // может не сработать, если не дошли до создания потока, но здесь это не доставит проблем
            pTrace->Release();
            pTrace = NULL;
        }
        if (pClient)
        {
            pClient->Release();
            pClient = NULL;
        }
    };

    // Создаём объект P7 клиента
    pClient = P7_Create_Client(TM("/P7.Sink=FileTxt"));
    if (NULL == pClient)
    {
        std::cerr << "ERR : Can't create P7 client instance.\n";
        loggerDeInitialize();
        return -1;
    }
    pClient->Share(TM("AppClient"));

    // Создаём трассировочный канал 1
    pTrace = P7_Create_Trace(pClient, TM("Trace channel 1"));
    if (NULL == pTrace)
    {
        std::cerr << "ERR : Can't create P7 trace channel.\n";
        loggerDeInitialize();
        return -1;
    }
    pTrace->Register_Module(TM("Main"), &hModule);
    pTrace->Register_Thread(TM("Application"), 0);



    // ========== Рабочая директория ==========

    auto workingDirectory = Utility::getWorkingDirectory();
    if (workingDirectory.empty())
    {
        std::cerr << "ERR : Unable to get working directory\n";
        pTrace->P7_CRITICAL(hModule, TM("Unable to get working directory"));
        loggerDeInitialize();
        return -1;
    }
    pTrace->P7_INFO(hModule, TM("Current working directory is '%s'"), Utility::to_wstring(workingDirectory).c_str());


    // ========== Инициализация камеры ==========
     
    CameraService cameraService;
    int devId = 0;
    int reqWidth = 1280;
    int reqHeight = 720;
    if (!cameraService.launch(devId, reqWidth, reqHeight))
    {
        pTrace->P7_ERROR(hModule, TM("Can not start camera%d or resolution %dx%d is not supported."), devId, reqWidth, reqHeight);
    }

    cv::Mat frame;
    ImageCv2GlAdapter adaptFrame;



    // ========== Инициализация графики ==========

    // Create application window
    ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_OWNDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"PotapychMind GUI", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize OpenGL
    if (!CreateDeviceWGL(hwnd, &g_MainWindow))
    {
        CleanupDeviceWGL(hwnd, &g_MainWindow);
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        pTrace->P7_CRITICAL(hModule, TM("WGL device initialization failed"));
        loggerDeInitialize();
        return 1;
    }
    wglMakeCurrent(g_MainWindow.hDC, g_hRC);

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;       // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;     // Enable Multi-Viewport / Platform Windows

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_InitForOpenGL(hwnd);
    ImGui_ImplOpenGL3_Init();
    printf("ImGui uses OpenGL of version: %s\n", glGetString(GL_VERSION));

    // Win32+GL needs specific hooks for viewport, as there are specific things needed to tie Win32 and GL api.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
        IM_ASSERT(platform_io.Renderer_CreateWindow == NULL);
        IM_ASSERT(platform_io.Renderer_DestroyWindow == NULL);
        IM_ASSERT(platform_io.Renderer_SwapBuffers == NULL);
        IM_ASSERT(platform_io.Platform_RenderWindow == NULL);
        platform_io.Renderer_CreateWindow = Hook_Renderer_CreateWindow;
        platform_io.Renderer_DestroyWindow = Hook_Renderer_DestroyWindow;
        platform_io.Renderer_SwapBuffers = Hook_Renderer_SwapBuffers;
        platform_io.Platform_RenderWindow = Hook_Platform_RenderWindow;
    }

    // Our state
    bool cameraWindowOpened = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);



    // ========== Главный цикл GUI потока ==========

    pTrace->P7_INFO(hModule, TM("GUI thread started"));
    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();



        // Тут можем рисовать что-либо своё

        if (ImGui::Button("Show camera"))
            cameraWindowOpened = true;

        if (cameraWindowOpened)
        {
            long long timestamp;
            cameraService.getFrame(frame, timestamp);
            adaptFrame.updateImage(frame);

            GUI::showCameraWindow(&cameraWindowOpened, adaptFrame);
        }



        // Rendering
        ImGui::Render();
        glViewport(0, 0, g_Width, g_Height);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();

            // Restore the OpenGL rendering context to the main window DC, since platform windows might have changed it.
            wglMakeCurrent(g_MainWindow.hDC, g_hRC);
        }

        // Present
        ::SwapBuffers(g_MainWindow.hDC);
    }
    pTrace->P7_INFO(hModule, TM("GUI thread finished"));



    // ========== Освобождение ресурсов графики ==========

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceWGL(hwnd, &g_MainWindow);
    wglDeleteContext(g_hRC);
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);



    // ========== Освобождение прочих ресурсов ==========

    cameraService.stop();

    pTrace->P7_INFO(hModule, TM("Program finished"));
    loggerDeInitialize();

    return 0;
}

// Helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
    HDC hDc = ::GetDC(hWnd);
    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;

    const int pf = ::ChoosePixelFormat(hDc, &pfd);
    if (pf == 0)
        return false;
    if (::SetPixelFormat(hDc, pf, &pfd) == FALSE)
        return false;
    ::ReleaseDC(hWnd, hDc);

    data->hDC = ::GetDC(hWnd);
    if (!g_hRC)
        g_hRC = wglCreateContext(data->hDC);
    return true;
}

void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
    wglMakeCurrent(nullptr, nullptr);
    ::ReleaseDC(hWnd, data->hDC);
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            g_Width = LOWORD(lParam);
            g_Height = HIWORD(lParam);
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

