#pragma once

#include <mutex>
#include <shared_mutex>
#include <vector>
#include <map>
#include <string>

// readonly ������
class DefaultConfig
{
private:
    // ��������� �� ��������
    bool mInitialized;
    // ������� �� ������
    mutable std::shared_mutex mutData_;
    
    // ��������� �������� ������
    int mCameraId;
    std::pair<int, int> mCameraSize;
    // ��������� ���������������� ����
    wchar_t mComPort[256];
    // ������������ ����
    std::pair<int, int> mFaceEps;
    // ������������
    int mServoCnt;
    std::map<std::wstring, int> mServoId;
    std::vector<int> mServoInitPos;
    std::vector<std::pair<int, int>> mServoBounds;

    // ��������� �� ��������
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

