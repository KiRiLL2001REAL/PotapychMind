#pragma once

#include <string>
#include <vector>
#include <variant>

enum class ScenarioError {
    NoError,
    FileNotFound,
    CanNotParse,
    SemanticError,
    SyntaxError
};

class ScenarioHandler
{
public:
    enum class CmdType {
        ERR,
        servo_action,
        internal
    };
    enum class InternalAction {
        wait_servos,
        change_state
    };
    enum class ServoAction {
        ERR,
        to_center,
        flap
    };

    struct CommandNode
    {
        struct _servo_action {
            std::wstring servo;
            ServoAction action;
        };

        struct _internal {
            InternalAction what;
            std::string next_state;
        };

        CmdType type;
        std::variant<
            _servo_action,
            _internal> data;
    };

protected:
    std::vector<std::string> states;
    std::vector<std::pair<std::string, std::vector<CommandNode>>> scenario;

    void invalidateData();
    bool isStateExists(const std::string& state);
    CmdType parseCmdType(const std::string& cmdType);
    ServoAction parseServoAction(const std::string& action);

public:
    ScenarioHandler();
    ~ScenarioHandler();

    ScenarioError loadScenario(const std::string& path);

};

