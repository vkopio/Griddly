#pragma once

#include "Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "gmock/gmock.h"

namespace griddly {

class MockObjectGenerator : public ObjectGenerator {
 public:
  MockObjectGenerator() : ObjectGenerator() {}

  MOCK_METHOD(void, defineNewObject, (const std::string& objectName, char mapCharacter, uint32_t zIdx, (const std::unordered_map<std::string, uint32_t>& parameterDefinitions)), ());
  MOCK_METHOD(void, defineActionBehaviour, (const std::string& objectName, const ActionBehaviourDefinition& behaviourDefinition), ());
  MOCK_METHOD(void, addInitialAction, (const std::string& objectName, const std::string& actionName, uint32_t actionId, uint32_t delay, bool randomize), ());

  MOCK_METHOD((const std::unordered_map<std::string, ActionInputsDefinition>&), getActionInputDefinitions, (), (const));

  MOCK_METHOD(std::shared_ptr<Object>, newInstance, (const std::string& objectName, uint32_t playerId, std::shared_ptr<Grid> grid), ());
  MOCK_METHOD(std::shared_ptr<Object>, cloneInstance, (std::shared_ptr<Object>, std::shared_ptr<Grid> grid), ());

  MOCK_METHOD(std::string&, getObjectNameFromMapChar, (char character), ());
  MOCK_METHOD((const std::map<std::string, std::shared_ptr<ObjectDefinition>>&), getObjectDefinitions, (), (const));

  MOCK_METHOD(void, setAvatarObject, (const std::string& objectName), ());
};
}  // namespace griddly
