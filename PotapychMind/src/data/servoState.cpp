#include "servoState.h"

ServoState* ServoState::mpInstance = nullptr;
std::mutex ServoState::mutInst_;

ServoState::ServoState():
    mIsChangable(),
    mPosition()
{
}

ServoState::~ServoState()
{
    std::unique_lock<std::shared_mutex> lk(mutData_);
    mIsChangable.clear();
    mPosition.clear();
}

ServoState* ServoState::getInstance()
{
    std::lock_guard<std::mutex> lk(mutInst_);
    if (!mpInstance)
        mpInstance = new ServoState();
    return mpInstance;
}

void ServoState::init(int count)
{
    std::unique_lock<std::shared_mutex> lk(mutData_);
    mIsChangable.clear();
    mPosition.clear();
    for (int i = 0; i < count; i++)
    {
        mIsChangable.push_back(true);
        mPosition.push_back(0);
    }
}

void ServoState::setChangable(int id, bool can_change)
{
    std::unique_lock<std::shared_mutex> lk(mutData_);
    if (id < 0 || id >= mIsChangable.size())
        throw std::exception("id out of bounds");
    mIsChangable[id] = can_change;
}

void ServoState::setPosition(int id, unsigned int pos)
{
    std::unique_lock<std::shared_mutex> lk(mutData_);
    if (id < 0 || id >= mIsChangable.size())
        throw std::exception("id out of bounds");
    mPosition[id] = pos;
}

bool ServoState::getIsChangable(int id) const
{
    std::shared_lock<std::shared_mutex> lk(mutData_);
    if (id < 0 || id >= mIsChangable.size())
        throw std::exception("id out of bounds");
    return mIsChangable[id];
}

unsigned int ServoState::getPosition(int id) const
{
    std::shared_lock<std::shared_mutex> lk(mutData_);
    if (id < 0 || id >= mPosition.size())
        throw std::exception("id out of bounds");
    return mPosition[id];
}
