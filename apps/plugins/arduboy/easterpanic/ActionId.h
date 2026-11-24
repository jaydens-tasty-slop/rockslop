#pragma once

#include <stdint.h>

enum class ActionId : uint8_t
{
	None = 0,
	MoveForward = 1,
	TurnLeft = 2,
	TurnRight = 3,
	ForStart = 4,
	ForEnd = 5,
};

constexpr const ActionId ActionIdFirst = ActionId::None;
constexpr const ActionId ActionIdLast = ActionId::ForEnd;
constexpr const ActionId ActionIdCount = static_cast<ActionId>(static_cast<uint8_t>(ActionIdLast) + 1);

constexpr inline ActionId nextActionIdWrapped(ActionId actionId)
{
	return (actionId != ActionIdLast) ? static_cast<ActionId>(static_cast<uint8_t>(actionId) + 1) : ActionIdFirst;
}

constexpr inline ActionId previousActionIdWrapped(ActionId actionId)
{
	return (actionId != ActionIdFirst) ? static_cast<ActionId>(static_cast<uint8_t>(actionId) - 1) : ActionIdLast;
}

constexpr inline ActionId nextActionIdClamped(ActionId actionId)
{
	return (actionId != ActionIdLast) ? static_cast<ActionId>(static_cast<uint8_t>(actionId) + 1) : actionId;
}

constexpr inline ActionId previousActionIdClamped(ActionId actionId)
{
	return (actionId != ActionIdFirst) ? static_cast<ActionId>(static_cast<uint8_t>(actionId) - 1) : actionId;
}

/*inline ActionId & operator++(ActionId & actionId)
{
	actionId = nextActionIdWrapped(actionId);
	return actionId;
}

inline ActionId & operator--(ActionId & actionId)
{
	actionId = previousActionIdWrapped(actionId);
	return actionId;
}*/