#pragma once

#include <mutex>
#include <shared_mutex>
#include <vector>

class ServoState
{
private:
    mutable std::shared_mutex mutData_;
    std::vector<bool> mIsChangable;
    std::vector<unsigned int> mPosition;

    ServoState();
    ~ServoState();

    // указатель на синглтон
    static std::mutex mutInst_;
    static ServoState* mpInstance;

    ServoState(const ServoState&) = delete;
    void operator=(const ServoState&) = delete;

public:
    static ServoState* getInstance();

    void init(int count);

    void setChangable(int id, bool can_change);
    void setPosition(int id, unsigned int pos);

    bool getIsChangable(int id) const;
    unsigned int getPosition(int id) const;
};
