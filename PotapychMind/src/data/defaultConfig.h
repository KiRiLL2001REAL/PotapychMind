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
    // загружены ли значения
    bool mInitialized;
    // Мьютекс на данные
    mutable std::shared_mutex mutData_;

    // дефолтные значения камеры
    int mCameraId;
    std::pair<int, int> mCameraSize;
    // дефолтный последовательный порт
    wchar_t mComPort[256];
    // отслеживание лица
    std::pair<int, int> mFaceEps;
    // распознавание лиц
    std::vector<int> mFaceSize;
    // сервоприводы
    int mServoCnt;
    std::map<std::wstring, int> mServoId;
    std::vector<unsigned int> mServoInitPos;
    std::vector<std::pair<unsigned int, unsigned int>> mServoBounds;

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
    bool isInitialized() const;

    // camera
    int getCameraId() const;
    std::pair<int, int> getCameraSize() const;
    // com
    void getComPort(wchar_t* buf, size_t buf_length) const;
    // face tracking
    std::pair<int, int> getFaceEps() const;
    // servos
    int getServoCnt() const;
    int getServoId(const std::wstring& servo_name) const;
    const std::wstring& getServoName(int id) const; // вспомогательная функция на тех же данных, что и ::getServoId
    unsigned int getServoInitPos(int id) const;
    std::pair<unsigned int, unsigned int> getServoBounds(int id) const;

};

