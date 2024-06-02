#pragma once

#include <mutex>
#include <shared_mutex>
#include <vector>
#include <map>
#include <string>

// readonly данные
class DefaultConfig
{
private:
    // загружены ли значени€
    bool mInitialized;
    // ћьютекс на данные
    mutable std::shared_mutex mutData_;
    
    // дефолтные значени€ камеры
    int mCameraId;
    std::pair<int, int> mCameraSize;
    // дефолтный последовательный порт
    wchar_t mComPort[256];
    // отслеживание лица
    std::pair<int, int> mFaceEps;
    // сервоприводы
    int mServoCnt;
    std::map<std::wstring, int> mServoId;
    std::vector<int> mServoInitPos;
    std::vector<std::pair<int, int>> mServoBounds;

    // указатель на синглтон
    static std::mutex mutInst_;
    static DefaultConfig* mpInstance;

    DefaultConfig();
    ~DefaultConfig();

    DefaultConfig(const DefaultConfig&) = delete;
    void operator=(const DefaultConfig&) = delete;

public:
    static DefaultConfig* getInstance();

    bool initialize(const std::wstring& cfg_filename);

    bool isInitialized();

    int getCameraId();
    std::pair<int, int> getCameraSize();
    void getComPort(wchar_t* buf, size_t buf_length);
    std::pair<int, int> getFaceEps();
    int getServoCnt();
    int getServoId(const std::wstring& servo_name);
    int getServoInitPos(int id);
    std::pair<int, int> getServoBounds(int id);

};

