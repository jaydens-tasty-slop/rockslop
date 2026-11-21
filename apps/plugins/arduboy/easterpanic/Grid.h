#pragma once

template< typename Type, uint8_t WidthValue, uint8_t HeightValue >
class Grid
{
public:
	using ValueType = Type;

public:
	constexpr static const uint8_t Width = WidthValue;
	constexpr static const uint8_t Height = HeightValue;
	constexpr static const uint16_t Count = Width * Height;

private:
	ValueType items[Count];

	inline uint8_t flattenIndex(const uint8_t & x, const uint8_t & y) const
	{
		return (Width * y) + x;
	}
	
public:	
	constexpr uint16_t getCount(void) const;
	constexpr uint8_t getWidth(void) const;
	constexpr uint8_t getHeight(void) const;

	ValueType & getItem(const uint8_t & x, const uint8_t & y);
	const ValueType & getItem(const uint8_t & x, const uint8_t & y) const;
	
	void fill(const ValueType & value); // O(n)
	void clear(void); // O(n)
};

template< typename Type, uint8_t Width, uint8_t Height >
constexpr uint16_t Grid<Type, Width, Height>::getCount(void) const
{
	return Grid<Type, Width, Height>::Count;
}
	
template< typename Type, uint8_t Width, uint8_t Height >
constexpr uint8_t Grid<Type, Width, Height>::getWidth(void) const
{
	return Grid<Type, Width, Height>::Width;
}
	
template< typename Type, uint8_t Width, uint8_t Height >
constexpr uint8_t Grid<Type, Width, Height>::getHeight(void) const
{
	return Grid<Type, Width, Height>::Height;
}

template< typename Type, uint8_t Width, uint8_t Height >
typename Grid<Type, Width, Height>::ValueType & Grid<Type, Width, Height>::getItem(const uint8_t & x, const uint8_t & y)
{
	return this->items[flattenIndex(x, y)];
}

template< typename Type, uint8_t Width, uint8_t Height >
const typename Grid<Type, Width, Height>::ValueType & Grid<Type, Width, Height>::getItem(const uint8_t & x, const uint8_t & y) const
{
	return this->items[flattenIndex(x, y)];
}

template< typename Type, uint8_t Width, uint8_t Height >
void Grid<Type, Width, Height>::fill(const ValueType & value)
{
	for(uint16_t i = 0; i < Count; ++i)
		items[i] = value;
}

template< typename Type, uint8_t Width, uint8_t Height >
void Grid<Type, Width, Height>::clear(void) // O(n)
{
	for (uint16_t i = 0; i < this->next; ++i)
		(&this->items[i])->~ValueType();
	this->next = 0;
}