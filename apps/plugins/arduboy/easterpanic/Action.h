#pragma once

#include "ActionId.h"

class Action
{
private:
	ActionId id;
	uint8_t argument;

public:
	Action(void) = default;
	Action(ActionId id) : id(id), argument(1) {}
	Action(ActionId id, uint8_t argument) : id(id), argument(argument) {}
	
	ActionId & getId(void) { return this->id; }
	const ActionId & getId(void) const { return this->id; }
	
	uint8_t & getArgument(void) { return this->argument; }
	const uint8_t & getArgument(void) const { return this->argument; }
};