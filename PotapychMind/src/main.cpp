﻿
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

/*TODO
Поиск локального файла конфигурации.
Если он не найден, клонируем дефолтный.
Если какие-либо значения в локальном конфиге не найдены, записываем их из дефолтного.
*/

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
#include "services/faceScannerService.h"
#include "services/robotHandlerService.h"
#include "imageCv2GlAdapter.h"
#include "gui/cameraWindow.h"
#include "data/defaultConfig.h"
#include "data/servoState.h"

#include "devices/serial/serialPortWrapper.h"

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

//void servoTest()
//{
//    using namespace std::chrono_literals;
//
//    SerialPortWrapper serial;
//    if (!serial.connect(L"\\\\.\\COM5"))
//    {
//        std::cout << "connection error\n";
//        return;
//    }
//    std::cout << "connection estabilished\n";
//    std::this_thread::sleep_for(2000ms);
//
//    unsigned char buffer[6];
//    buffer[0] = 1;
//    buffer[1] = 0;
//    buffer[2] = 0;
//    buffer[3] = 1;
//
//    for (int i = 0; i < 300; i++)
//    {
//        static const int max_val = 170;
//
//        float a = float(abs(float(i % 100) / 100 * max_val));
//        unsigned char b;
//        if ((i / 100) % 2)
//            b = max_val - a;
//        else
//            b = a;
//        buffer[4] = i % 6; // servo addr
//
//        switch (buffer[4])
//        {
//        case 0:
//            break;
//        case 1:
//            b = max_val - b;
//            break;
//        case 2:
//            break;
//        case 3:
//            b = max_val - b;
//            break;
//        case 4:
//            break;
//        }
//
//        buffer[5] = b;     // servo pos
//        serial.write(buffer, 6);
//        std::this_thread::sleep_for(10ms);
//    }
//
//    serial.disconnect();
//    std::cout << "disconnected\n";
//}

// Main code
int main(int, char**)
{
    //servoTest();
    //return 0;

    // Добавляем в консоль поддержку символов кириллицы (иначе вывод может внезапно повиснуть)
    std::wcout.imbue(std::locale("rus_rus.866"));



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



    // ========== Загрузка конфигурации ==========

    const auto cfgFile = Utility::to_wstring(workingDirectory) + L"\\resources\\defaultCfg.ini";
    auto& config = *DefaultConfig::getInstance();
    if (!config.initialize(cfgFile))
    {
        std::cerr << "ERR : Unable to find default configuration file\n";
        pTrace->P7_CRITICAL(hModule, TM("Unable to find default configuration file '%s'"), cfgFile.c_str());
        loggerDeInitialize();
        return -1;
    }
    pTrace->P7_INFO(hModule, TM("Found and loaded default config from '%s'"), cfgFile.c_str());



    // ========== Инициализация данных ==========

    auto& servoState = *ServoState::getInstance();
    servoState.init(config.getServoCnt());
    servoState.setChangable(2, false);



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



    // ========== Инициализация сканера лиц ==========

    FaceScannerService faceScannerService;
    if (!faceScannerService.launch())
    {
        pTrace->P7_ERROR(hModule, TM("Can not start faceScanner."));
    }

    std::vector<Face> detectionResult = {};



    // ========== Инициализация модуля управления роботом ==========

    RobotHandler robotHandler;
    if (!robotHandler.launchServos(L"\\\\.\\COM5"))
    {
        pTrace->P7_ERROR(hModule, TM("Can not start robotHandler."));
    }

    //std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    //
    //robotHandler.headToCenter();
    //
    //robotHandler.headLeft();
    //robotHandler.headRight();
    //robotHandler.headUp();
    //robotHandler.headDown();
    //robotHandler.headToCenter();//
    //robotHandler.headLeft();
    //robotHandler.headUp();
    //robotHandler.headRight();
    //robotHandler.headDown();
    //robotHandler.headToCenter();//
    //
    //robotHandler.handsToCenter();
    //
    //robotHandler.leftHandUp();
    //robotHandler.leftHandDown();
    //robotHandler.rightHandUp();
    //robotHandler.rightHandDown();
    //robotHandler.handsToCenter();//
    //robotHandler.leftHandUp();
    //robotHandler.rightHandUp();
    //robotHandler.leftHandDown();
    //robotHandler.rightHandDown();
    //robotHandler.handsToCenter();//
    //robotHandler.flapHands();
    //robotHandler.handsToCenter();//



    // ========== TMP ==========
    auto devCom = devices::DeviceEnumerator::getComDevices();
    std::cout << "COM ports:\n";
    for (const auto& dev : devCom)
    {
        std::cout << "\tId: " << dev.id;
        std::wcout << L"\tName: " << dev.deviceName;
        std::wcout << L"\tPath: " << dev.devicePath << std::endl;
    }
    auto devCam = devices::DeviceEnumerator::getVideoDevices();
    std::cout << "Cameras:\n";
    for (const auto& dev : devCam)
    {
        std::cout << "\tId: " << dev.id;
        std::wcout << L"\tName: " << dev.deviceName;
        std::wcout << L"\tPath: " << dev.devicePath << std::endl;
    }



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
    int cameraWindowVisualization = 0;


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

        // Виджет управления окном камеры
        if (ImGui::Checkbox("Show camera", &cameraWindowOpened))
        {
        }
        ImGui::BeginDisabled(!cameraWindowOpened);
        if (ImGui::CollapsingHeader("Algorythm settings"))
        {
            ImGui::RadioButton("Detected faces over camera preview", &cameraWindowVisualization, 0);
            ImGui::RadioButton("Detected faces over provided image", &cameraWindowVisualization, 1);
            ImGui::RadioButton("Raw camera preview", &cameraWindowVisualization, 2);
        }
        ImGui::EndDisabled();
        //ImGui::BeginDisabled();
        //for (int i = 0; i < config.getServoCnt(); i++)
        //{
        //    const auto bounds = config.getServoBounds(i);
        //    unsigned int min_val = bounds.first;
        //    unsigned int max_val = bounds.second;
        //    int current = servoState.getPosition(i);
        //    if (ImGui::SliderInt("slider int", &current, min_val, max_val))
        //    {
        //        std::cout << "action\n";
        //    }
        //}
        //ImGui::EndDisabled();

        if (cameraWindowOpened)
        {
            // лямбда отрисовки прямоугольников на лицах
            auto drawRects = [detectionResult, frame]()
            {
                for (const auto& detRes : detectionResult)
                {
                    cv::Scalar color;
                    switch (detRes.peopleClass)
                    {
                        // blue
                    case PeopleClass::Familiar: color = cv::Scalar(255, 0, 0); break;
                        // green
                    case PeopleClass::Interviewee: color = cv::Scalar(0, 255, 0); break;
                        // red
                    case PeopleClass::Stranger:
                    default: color = cv::Scalar(0, 0, 255);
                    }
                    cv::rectangle(frame, detRes.roi, color, 2);
                }
            };

            long long timestamp;
            cameraService.getFrame(frame, timestamp);

            switch (cameraWindowVisualization)
            {
            case 0:
                faceScannerService.pushFrame(frame);
                faceScannerService.getDetectionResult(detectionResult);
                drawRects();
                break;
            case 1:
                faceScannerService.pushFrame(frame);
                faceScannerService.getDetectionResult(frame, detectionResult);
                drawRects();
                break;
            case 2:
                adaptFrame.updateImage(frame);
                break;
            }

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

    faceScannerService.stop();
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

