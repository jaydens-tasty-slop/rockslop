#pragma once

//
// StaticRange
//

template< typename Type, Type FirstValue, Type LastValue >
struct StaticRange
{
	//
	// Type aliases
	//
	using ValueType = Type;
	
	//
	// Constants
	//
	constexpr static const ValueType First = FirstValue;
	constexpr static const ValueType Last = LastValue;
	
	// Prevent creation of instance
	StaticRange(void) = delete;
	
	//
	// Static functions
	//
	
	constexpr static bool isInRange(const ValueType & value)
	{
		return (value >= First) && (value <= Last);
	}

	constexpr static ValueType wrappedNext(const ValueType & value)
	{
		return (value != Last) ? (value + 1) : First;
	}

	constexpr static ValueType wrappedPrevious(const ValueType & value)
	{
		return (value != First) ? (value - 1) : Last;
	}

	constexpr static ValueType clampedNext(const ValueType & value)
	{
		return (value != Last) ? (value + 1) : value;
	}

	constexpr static ValueType clampedPrevious(const ValueType & value)
	{
		return (value != First) ? (value - 1) : value;
	}
};