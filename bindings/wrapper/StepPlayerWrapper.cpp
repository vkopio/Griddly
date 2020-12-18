#pragma once
#include <pybind11/pybind11.h>
#include <spdlog/spdlog.h>

#include <memory>

#include "../../src/Griddly/Core/GDY/Objects/Object.hpp"
#include "../../src/Griddly/Core/Players/Player.hpp"

namespace py = pybind11;

namespace griddly {
class Py_StepPlayerWrapper {
 public:
  Py_StepPlayerWrapper(int playerId, std::string playerName, std::shared_ptr<Observer> observer, std::shared_ptr<GDYFactory> gdyFactory, std::shared_ptr<GameProcess> gameProcess)
      : player_(std::shared_ptr<Player>(new Player(playerId, playerName, observer))), gdyFactory_(gdyFactory), gameProcess_(gameProcess) {
  }

  std::shared_ptr<Player> unwrapped() {
    return player_;
  }

  std::array<uint32_t, 2> getTileSize() const {
    auto tileSize = player_->getObserver()->getTileSize();
    return {(uint32_t)tileSize[0], (uint32_t)tileSize[1]};
  }

  std::shared_ptr<NumpyWrapper<uint8_t>> observe() {
    auto observer = player_->getObserver();
    if (observer == nullptr) {
      throw std::invalid_argument("No player observer configured");
    }

    return std::shared_ptr<NumpyWrapper<uint8_t>>(new NumpyWrapper<uint8_t>(observer->getShape(), observer->getStrides(), player_->observe()));
  }

  py::tuple stepBatch(std::vector<std::vector<int32_t>> actionArrayList, bool updateTicks) {
    auto externalActionNames = gdyFactory_->getExternalActionNames();
    auto gameProcess = player_->getGameProcess();

    if (gameProcess != nullptr && !gameProcess->isInitialized()) {
      throw std::invalid_argument("Cannot send player commands when game has not been initialized.");
    }

    std::vector<std::shared_ptr<Action>> actionBatch;
    for (auto actionArray : actionArrayList) {
      auto actionName = externalActionNames[actionArray[0]];
      actionArray.erase(actionArray.begin());

      auto action = buildAction(actionName, actionArray);
      if (action != nullptr) {
        actionBatch.push_back(action);
      }
    }

    return performActions(actionBatch, updateTicks);
  }

  py::tuple stepSingle(std::string actionName, std::vector<int32_t> actionArray, bool updateTicks) {
    auto gameProcess = player_->getGameProcess();

    if (gameProcess != nullptr && !gameProcess->isInitialized()) {
      throw std::invalid_argument("Cannot send player commands when game has not been initialized.");
    }

    auto action = buildAction(actionName, actionArray);

    if (action != nullptr) {
      return performActions({action}, updateTicks);
    } else {
      return performActions({}, updateTicks);
    }
  }

 private:
  const std::shared_ptr<Player> player_;
  const std::shared_ptr<GDYFactory> gdyFactory_;
  const std::shared_ptr<GameProcess> gameProcess_;

  py::tuple performActions(std::vector<std::shared_ptr<Action>> actions, bool updateTicks) {
    ActionResult actionResult;

    actionResult = player_->performActions(actions, updateTicks);

    int totalRewards = 0;
    for (auto &r : actionResult.rewards) {
      totalRewards += r;
    }

    auto info = buildInfo(actionResult);

    return py::make_tuple(totalRewards, actionResult.terminated, info);
  }

  py::dict buildInfo(ActionResult actionResult) {
    py::dict py_info;

    if (actionResult.terminated) {
      py::dict py_playerResults;

      for (auto playerRes : actionResult.playerStates) {
        std::string playerStatusString;
        switch (playerRes.second) {
          case TerminationState::WIN:
            playerStatusString = "Win";
            break;
          case TerminationState::LOSE:
            playerStatusString = "Lose";
            break;
          case TerminationState::NONE:
            playerStatusString = "";
            break;
        }

        if (playerStatusString.size() > 0) {
          py_playerResults[std::to_string(playerRes.first).c_str()] = playerStatusString;
        }
      }
      py_info["PlayerResults"] = py_playerResults;
    }

    auto history = gameProcess_->getGrid()->getHistory();

    if (history.size() > 0) {
      std::vector<py::dict> py_events;
      for (auto historyEvent : history) {
        py::dict py_event;

        py_event["PlayerId"] = historyEvent.playerId;
        py_event["ActionName"] = historyEvent.actionName;
        py_event["Tick"] = historyEvent.tick;
        py_event["Reward"] = historyEvent.reward;
        py_event["Delay"] = historyEvent.delay;

        py_event["SourceObjectName"] = historyEvent.sourceObjectName;
        py_event["DestinationObjectName"] = historyEvent.destObjectName;

        py_event["SourceObjectPlayerId"] = historyEvent.sourceObjectPlayerId;
        py_event["DestinationObjectPlayerId"] = historyEvent.destinationObjectPlayerId;

        py_event["SourceLocation"] = std::array{historyEvent.sourceLocation.x, historyEvent.sourceLocation.y};
        py_event["DestinationLocation"] = std::array{historyEvent.destLocation.x, historyEvent.destLocation.y};

        py_events.push_back(py_event);
      }
      py_info["History"] = py_events;

      gameProcess_->getGrid()->purgeHistory();
    }

    return py_info;
  }

  std::shared_ptr<Action> buildAction(std::string actionName, std::vector<int32_t> actionArray) {
    auto actionInputsDefinition = gdyFactory_->findActionInputsDefinition(actionName);
    auto playerAvatar = player_->getAvatar();

    auto inputMappings = actionInputsDefinition.inputMappings;

    if (playerAvatar != nullptr) {
      auto actionId = actionArray[0];

      if (inputMappings.find(actionId) == inputMappings.end()) {
        return nullptr;
      }

      auto mapping = inputMappings[actionId];
      auto vectorToDest = mapping.vectorToDest;
      auto orientationVector = mapping.orientationVector;
      auto action = std::shared_ptr<Action>(new Action(gameProcess_->getGrid(), actionName, 0));
      action->init(playerAvatar, vectorToDest, orientationVector, actionInputsDefinition.relative);

      return action;
    } else {
      glm::ivec2 sourceLocation = {actionArray[0], actionArray[1]};

      auto actionId = actionArray[2];

      if (inputMappings.find(actionId) == inputMappings.end()) {
        return nullptr;
      }

      auto mapping = inputMappings[actionId];
      auto vector = mapping.vectorToDest;
      auto orientationVector = mapping.orientationVector;

      glm::ivec2 destinationLocation = sourceLocation + vector;

      auto action = std::shared_ptr<Action>(new Action(gameProcess_->getGrid(), actionName, 0));
      action->init(sourceLocation, destinationLocation);

      return action;
    }
  }
};

}  // namespace griddly