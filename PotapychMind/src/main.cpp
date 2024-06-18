// Dear ImGui: standalone example application for Win32 + OpenGL 3

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// This is provided for completeness, however it is strongly recommended you use OpenGL with SDL or GLFW.

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/GL.h>
#include <tchar.h>


#include "utility.h"
#include "devices/deviceEnumerator.h"
#include <P7_Client.h>
#include <P7_Trace.h>
#include "services/cameraService.h"
#include "imageCv2GlAdapter.h"
#include "gui/cameraWindow.h"
#include "data/defaultConfig.h"
#include <filesystem>


#include "handlers/robotHandler.h"
#include "data/servoState.h"


#include "services/faceScannerService.h"


#include "handlers/scenarioHandler.h"


#include <iostream>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "handlers/faceComparatorHandler.h"

#include <cppflow/cppflow.h>

void _printSlicedMat(cv::Mat mat, bool last, int typ = CV_8UC1)
{
    int dims = mat.dims;
    if (dims == 2)
    {
        std::cout << mat << (last ? "]" : "") << std::endl;
        return;
    }
    std::cout << "[";
    std::vector<int> sz;
    for (int i = 0; i < dims; i++)
        sz.push_back(mat.size[i]);
    for (int i = 0; i < sz[0]; i++)
    {
        cv::Mat M(dims - 1, std::vector<int>(sz.begin() + 1, sz.end()).data(), typ, mat.data + mat.step[0] * i);
        _printSlicedMat(M, i == sz[0] - 1, typ);
    }
}

void printSlicedMat(cv::Mat mat, int typ = CV_8UC1)
{
    _printSlicedMat(mat, false, typ);
}

void printOperations(cppflow::model& model)
{
    auto ops = model.get_operations();
    for (const auto& it : ops) {
        std::cout << it;
        if (it != "NoOp")
        {
            auto shape = model.get_operation_shape(it);
            std::cout << " (";
            if (!shape.empty())
            {
                std::cout << shape[0];
                for (int i = 1; i < shape.size(); i++)
                    std::cout << ", " << shape[i];
            }
            else
                std::cout << "None";
            std::cout << ")";
        }
        std::cout << std::endl;
    }
}

cv::Mat concat_images(cv::Mat img1, cv::Mat img2)
{
    int height = img1.size().height;
    int width = img2.size().width;
    cv::Mat face1holder = cv::Mat::zeros(height * 2, width * 2, img1.type());
    cv::Mat face2holder = cv::Mat::zeros(height * 2, width * 2, img2.type());
    cv::Mat roi1 = face1holder
        .rowRange(height / 2, 2 * height - height / 2)
        .colRange(width / 2, 2 * width - width / 2);
    cv::Mat roi2 = face2holder
        .rowRange(height / 2, 2 * height - height / 2)
        .colRange(width / 2, 2 * width - width / 2);
    img1.copyTo(roi1);
    img2.copyTo(roi2);

    cv::Mat concatenated;
    cv::hconcat(face1holder, face2holder, concatenated);
    face1holder.release();
    face2holder.release();

    return concatenated;
}



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

    //std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    //
    //robotHandler.headToCenter();
    //robotHandler.handsToCenter();
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



    // ========== Модуль сравнения лиц ==========

    FaceComparatorHandler faceComparator("resources/face_comparator");

    // ========== ... и его проверка ==========
    cv::Mat face1;
    cv::Mat face2;

    face1 = cv::imread("E:\\datasets\\faces\\cropped\\pins_Henry Cavil\\Henry Cavil98_1270.jpg");
    face2 = cv::imread("E:\\datasets\\faces\\cropped\\pins_Chris Pratt\\Chris Pratt103_728.jpg");

    { // preview
        auto concatenated = concat_images(face1, face2);
        
        float similarity = faceComparator.equality(face1, face2);
        char buf[32];
        sprintf_s(buf, "%f", similarity);
        cv::putText(concatenated,
            std::string("similarity: ") + std::string(buf),
            cv::Point(10, 20), cv::HersheyFonts::FONT_HERSHEY_PLAIN,
            1.2, cv::Scalar(255, 255, 255), 1);

        cv::imshow("faces", concatenated);
        //cv::waitKey(0);
        concatenated.release();
    }



    // ========== Модуль загрузки сценария ==========
    ScenarioHandler scenario;
    auto scenarioError = scenario.loadScenario(
        workingDirectory + "/resources/scenarios/default.json");
    if (scenarioError != ScenarioError::NoError)
    {
        if (scenarioError == ScenarioError::FileNotFound)
            pTrace->P7_ERROR(hModule, TM("Can not load scenario."));
        else if (scenarioError == ScenarioError::SemanticError)
            pTrace->P7_ERROR(hModule, TM("Scenario semantic error."));
        else if (scenarioError == ScenarioError::CanNotParse)
            pTrace->P7_ERROR(hModule, TM("Can not parse scenario."));
        else
            pTrace->P7_ERROR(hModule, TM("Scenario syntax error."));
    }



    // ========== Инициализация графики ==========

    // Create application window
    ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_OWNDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui Win32+OpenGL3 Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize OpenGL
    if (!CreateDeviceWGL(hwnd, &g_MainWindow))
    {
        CleanupDeviceWGL(hwnd, &g_MainWindow);
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
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
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls
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

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Our state
    bool cameraWindowOpened = true;
    int cameraWindowVisualization = 0;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

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

        ImGui::Checkbox("Show camera", &cameraWindowOpened);
        
        ImGui::BeginDisabled(!cameraWindowOpened);
        if (ImGui::CollapsingHeader("Algorythm settings"))
        {
            ImGui::RadioButton("Detected faces over camera preview", &cameraWindowVisualization, 0);
            ImGui::RadioButton("Detected faces over provided image", &cameraWindowVisualization, 1);
            ImGui::RadioButton("Raw camera preview", &cameraWindowVisualization, 2);
        }
        ImGui::EndDisabled();

        if (cameraWindowOpened)
        {
            // лямбда отрисовки прямоугольников поверх лиц
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

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceWGL(hwnd, &g_MainWindow);
    wglDeleteContext(g_hRC);
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);



    // ========== Освобождение прочих ресурсов ==========

    //faceScannerService.stop();
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