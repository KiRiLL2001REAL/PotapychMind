#include "scenarioHandler.h"

#include <nlohmann/json.hpp>
#include <fstream>

#include <vector>
#include <exception>
#include <algorithm>

#include "../utility.h"

#include <iostream>

void ScenarioHandler::invalidateData()
{
    states.clear();
    scenario.clear();
}

bool ScenarioHandler::isStateExists(const std::string& state)
{
    for (const auto& it : states)
        if (state == it)
            return true;
    return false;
}

ScenarioHandler::CmdType ScenarioHandler::parseCmdType(const std::string& cmdType)
{
    if (cmdType == "servo_action")
        return CmdType::servo_action;
    else if (cmdType == "internal")
        return CmdType::internal;
    return CmdType::ERR;
}

ScenarioHandler::ServoAction ScenarioHandler::parseServoAction(const std::string& action)
{
    return ServoAction();
}

ScenarioHandler::ScenarioHandler()
{
}

ScenarioHandler::~ScenarioHandler()
{
    invalidateData();
}

ScenarioError ScenarioHandler::loadScenario(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open())
        return ScenarioError::FileNotFound;

    using json = nlohmann::json;

    json data;
    bool syntax_err = false;
    try
    {
        data = json::parse(f);
    }
    catch (std::exception& e)
    {
        syntax_err = true;
    }
    f.close();

    if (syntax_err)
    {
        std::cout << "Can not parse.\n";
        return ScenarioError::CanNotParse;
    }

    // подгрузка всех стэйтов
    auto& jstates = data["states"];
    if (jstates.empty())
    {
        std::cout << "No states in scenario\n";
        invalidateData();
        return ScenarioError::SemanticError;
    }
    for (auto& jstate : jstates)
        states.push_back(jstate.get<std::string>());

    // проходим по стейтам сценария, извлекаем команды
    auto& jscenario = data["scenario"];
    if (jscenario.empty())
    {
        std::cout << "No 'scenario' label\n";
        invalidateData();
        return ScenarioError::SyntaxError;
    }
    for (auto& jnode : jscenario)
    {
        auto& jstate = jnode["state"];
        if (jstate.empty())
        {
            std::cout << "No 'state' label\n";
            invalidateData();
            return ScenarioError::SyntaxError;
        }
        std::string state = jstate.get<std::string>();

        auto& jcommands = jnode["commands"];
        if (jcommands.empty()) {
            std::cout << "No 'commands' label\n";
            invalidateData();
            return ScenarioError::SyntaxError;
        }

        std::vector<CommandNode> commands;
        for (auto& jcommand : jcommands)
        {
            // тип команды
            CmdType type;
            {
                auto& jtype = jcommand["type"];
                if (jtype.empty()) {
                    std::cout << "Command without type\n";
                    invalidateData();
                    return ScenarioError::SemanticError;
                }
                auto typ = jtype.get<std::string>();
                type = parseCmdType(typ);
            }
            if (type == CmdType::ERR) {
                std::cout << "Incorrect command type\n";
                invalidateData();
                return ScenarioError::SyntaxError;
            }
            // парсим команду
            CommandNode commandNode;
            commandNode.type = type;
            switch (type)
            {
            case CmdType::ERR:
                std::cout << "Unknown command type\n";
                invalidateData();
                return ScenarioError::SyntaxError;
            case CmdType::servo_action:
                CommandNode::_servo_action action_data;
                ServoAction action;
                {
                    auto& jaction = jcommand["action"];
                    if (jaction.empty()) {
                        std::cout << "Arg 'action' is not found\n";
                        invalidateData();
                        return ScenarioError::SemanticError;
                    }
                    auto act = jaction.get<std::string>();
                    action = parseServoAction(act);
                }
                action_data.action = action;
                if (action == ServoAction::ERR) {
                    std::cout << "Unknown 'action'.\n";
                    invalidateData();
                    return ScenarioError::SyntaxError;
                }
                std::string servo;
                {
                    auto& jservo = jcommand["servo"];
                    if (jservo.empty()) {
                        std::cout << "Arg 'servo' is not found\n";
                        invalidateData();
                        return ScenarioError::SemanticError;
                    }
                    servo = jservo.get<std::string>();
                }
                action_data.servo = Utility::to_wstring(servo);
                commandNode.data = action_data;
                break;
            }

        }

        

        int g = 4;
    }

    auto str = data.dump(1);
    std::cout << str << std::endl;

    return ScenarioError::NoError;
}
