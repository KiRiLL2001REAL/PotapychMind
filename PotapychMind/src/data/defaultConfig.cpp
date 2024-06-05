#include "defaultConfig.h"

#include "../internal/ini/iniReader.h"
#include <exception>

DefaultConfig* DefaultConfig::mpInstance = nullptr;
std::mutex DefaultConfig::mutInst_;

DefaultConfig::DefaultConfig():
    mInitialized(false),
    mCameraId(-1),
    mCameraSize(-1, -1),
    mFaceEps(-1, -1),
    mServoCnt(-1),
    mServoId(),
    mServoInitPos(),
    mServoBounds()
{
    memset(mComPort, 0, sizeof(wchar_t) * 256);
}

DefaultConfig::~DefaultConfig()
{
    std::unique_lock<std::shared_mutex> lk(mutData_);
    mInitialized = false;
    mCameraId = mServoCnt = -1;
    mCameraSize = mFaceEps = { -1, -1 };
    memset(mComPort, 0, sizeof(wchar_t) * 256);
    std::map<std::wstring, int>().swap(mServoId);
    mServoInitPos.clear();
    mServoBounds.clear();
}

DefaultConfig* DefaultConfig::getInstance()
{
    std::lock_guard<std::mutex> lk(mutInst_);
    if (!mpInstance)
        mpInstance = new DefaultConfig();
    return mpInstance;
}

bool DefaultConfig::initialize(const std::wstring& cfg_filename)
{
    std::unique_lock<std::shared_mutex> lk(mutData_);
    if (mInitialized) // грузим данные только 1 раз
        return true;
    Ini::IniReader rd;
    mInitialized = rd.open(cfg_filename.c_str());
    if (!mInitialized) // если файл не найден, значит инициализация не возможна
        return false;
    // камера
    mCameraId = rd.readInt(L"CAMERA", L"ID");
    {
        int width = rd.readInt(L"CAMERA", L"WIDTH");
        int height = rd.readInt(L"CAMERA", L"HEIGHT");
        mCameraSize = { width, height };
    }
    // последовательный порт
    {
        wchar_t* ret = rd.readStr(L"DEFAULT_COM", L"ADDR");
        memcpy(mComPort, ret, sizeof(wchar_t) * wcslen(ret));
        delete[] ret;
    }
    // трекинг лица
    {
        int eps_x = rd.readInt(L"FACE_TRACKING", L"EPS_X");
        int eps_y = rd.readInt(L"FACE_TRACKING", L"EPS_Y");
        mFaceEps = { eps_x, eps_y };
    }
    // сервоприводы
    mServoCnt = rd.readInt(L"SERVO", L"COUNT");
    for (int i = 0; i < mServoCnt; i++)
    {
        static const std::wstring name_prefix        = L"NAME_";
        static const std::wstring init_pos_prefix    = L"INIT_POS_";
        static const std::wstring lower_bound_prefix = L"BOUND_LOWER_";
        static const std::wstring upper_bound_prefix = L"BOUND_UPPER_";
        wchar_t buf[8]; memset(buf, 0, sizeof(wchar_t) * 8);
        _itow_s(i, buf, 10);
        const std::wstring id(buf);

        std::wstring name;
        {
            wchar_t* buf = rd.readStr(L"SERVO", (name_prefix + id).c_str());
            name = std::wstring(buf);
            delete[] buf;
        }
        int init_pos = rd.readInt(L"SERVO", (init_pos_prefix + id).c_str());
        int lower_bound = rd.readInt(L"SERVO", (lower_bound_prefix + id).c_str());
        int upper_bound = rd.readInt(L"SERVO", (upper_bound_prefix + id).c_str());
        mServoId[name] = i;
        mServoInitPos.push_back(init_pos);
        mServoBounds.push_back({ lower_bound , upper_bound });
    }
    rd.close();

    return true;
}

bool DefaultConfig::isInitialized() const
{
    std::shared_lock<std::shared_mutex> lk(mutData_);
    return mInitialized;
}

int DefaultConfig::getCameraId() const
{
    std::shared_lock<std::shared_mutex> lk(mutData_);
    return mCameraId;
}

std::pair<int, int> DefaultConfig::getCameraSize() const
{
    std::shared_lock<std::shared_mutex> lk(mutData_);
    return mCameraSize;
}

void DefaultConfig::getComPort(wchar_t* buf, size_t buf_length) const
{
    std::shared_lock<std::shared_mutex> lk(mutData_);
    size_t length = buf_length < 256 ? buf_length : 256;
    memcpy(buf, mComPort, sizeof(wchar_t) * buf_length);
}

std::pair<int, int> DefaultConfig::getFaceEps() const
{
    std::shared_lock<std::shared_mutex> lk(mutData_);
    return mFaceEps;
}

int DefaultConfig::getServoCnt() const
{
    std::shared_lock<std::shared_mutex> lk(mutData_);
    return mServoCnt;
}

int DefaultConfig::getServoId(const std::wstring& servo_name) const
{
    std::shared_lock<std::shared_mutex> lk(mutData_);
    auto it = mServoId.find(servo_name);
    if (it != mServoId.end())
        return it->second;
    return -1;
}

const std::wstring& DefaultConfig::getServoName(int id) const
{
    std::shared_lock<std::shared_mutex> lk(mutData_);
    auto elem = mServoId.end();
    for (auto it = mServoId.begin(); it != mServoId.end(); it++)
        if (it->second == id)
        {
            elem = it;
            break;
        }
    if (elem != mServoId.end())
        return elem->first;
    return L"";
}

unsigned int DefaultConfig::getServoInitPos(int id) const
{
    std::shared_lock<std::shared_mutex> lk(mutData_);
    if (id < 0 || id >= mServoInitPos.size())
        throw std::exception("servo_id out of bounds");
    return mServoInitPos[id];
}

std::pair<unsigned int, unsigned int> DefaultConfig::getServoBounds(int id) const
{
    std::shared_lock<std::shared_mutex> lk(mutData_);
    if (id < 0 || id >= mServoInitPos.size())
        throw std::exception("servo_id out of bounds");
    return mServoBounds[id];
}

