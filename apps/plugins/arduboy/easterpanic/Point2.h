#pragma once

template< typename Type >
class Point2
{
public:
	using ValueType = Type;

public:
	ValueType x = ValueType(0);
	ValueType y = ValueType(0);

public:
	constexpr Point2(void) = default;
	
	constexpr Point2(const ValueType & x, const ValueType & y) : x(x), y(y) {}
	
	template< typename OtherType >
	explicit constexpr Point2(const OtherType & x, const OtherType & y) : x(x), y(y) {}
	
	template< typename OtherType >
	explicit constexpr Point2(const Point2<OtherType> & other) : x(other.x), y(other.y) {}	
};

using PointU8 = Point2<uint8_t>;
using PointS8 = Point2<int8_t>;
using PointU16 = Point2<uint8_t>;
using PointS16 = Point2<int16_t>;