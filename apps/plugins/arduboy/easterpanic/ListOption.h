#pragma once

template< typename StateId >
class ListOption
{
public:
	StateId state;
	const char * text;
	
public:
	ListOption(void) = default;
	constexpr ListOption(const StateId & state, const char * text)
		: state(state), text(text)
	{
	}
};