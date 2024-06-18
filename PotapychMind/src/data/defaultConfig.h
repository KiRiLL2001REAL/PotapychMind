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
    // ������������� ���
    std::vector<int> mFaceSize;
    // ������������
    int mServoCnt;
    std::map<std::wstring, int> mServoId;
    std::vector<unsigned int> mServoInitPos;
    std::vector<std::pair<unsigned int, unsigned int>> mServoBounds;

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
    const std::wstring& getServoName(int id) const; // ��������������� ������� �� ��� �� ������, ��� � ::getServoId
    unsigned int getServoInitPos(int id) const;
    std::pair<unsigned int, unsigned int> getServoBounds(int id) const;

};

