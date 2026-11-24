#pragma once

#include <stdint.h>

template< typename Type, uint8_t Capacity >
class Array
{
public:
	static_assert(Capacity > 0, "Attempt to create Array with Capacity less than 1");
	static_assert(Capacity < 128, "Attempt to create Array with Capacity greater than 127");

	using ItemType = Type;
	using IndexType = uint8_t;
	using IndexOfType = int8_t;

	using ValueType = Type;

	constexpr static const IndexType FirstIndex = 0;
	constexpr static const IndexType LastIndex = Capacity - 1;

private:
	ItemType items[Capacity];

public:
	Array(void) = default;

	constexpr IndexType getCount(void) const; // O(1)
	constexpr IndexType getCapacity(void) const; // O(1)
	constexpr IndexType getFirstIndex(void) const; // O(1)
	constexpr IndexType getLastIndex(void) const; // O(1)
	
	void clear(void); // O(n)
	bool contains(const ItemType & item) const; // O(n)

	// Returns -1 if item not found
	IndexOfType indexOf(const ItemType & item) const; // O(n)

	ItemType & operator [] (const IndexType & index); // O(1)
	const ItemType & operator [] (const IndexType & index) const; // O(1)
};


//
// Definition
//

/*template< typename ItemType, uint8_t Capacity >
Array< ItemType, Capacity >::Array(void)
	: items()
{
}*/

template< typename ItemType, uint8_t Capacity >
constexpr typename Array< ItemType, Capacity >::IndexType Array< ItemType, Capacity >::getCount(void) const // O(1)
{
	return static_cast<IndexType>(Capacity);
}

template< typename ItemType, uint8_t Capacity >
constexpr typename Array< ItemType, Capacity >::IndexType Array< ItemType, Capacity >::getCapacity(void) const // O(1)
{
	return static_cast<IndexType>(Capacity);
}

template< typename ItemType, uint8_t Capacity >
constexpr typename Array< ItemType, Capacity >::IndexType Array< ItemType, Capacity >::getFirstIndex(void) const // O(1)
{
	return static_cast<IndexType>(FirstIndex);
}

template< typename ItemType, uint8_t Capacity >
constexpr typename Array< ItemType, Capacity >::IndexType Array< ItemType, Capacity >::getLastIndex(void) const // O(1)
{
	return static_cast<IndexType>(LastIndex);
}

template< typename ItemType, uint8_t Capacity >
void Array< ItemType, Capacity >::clear(void) // O(n)
{
	for (IndexType i = this->getFirstIndex(); i < this->getCount(); ++i)
		(&this->items[i])->~ItemType();
}

template< typename ItemType, uint8_t Capacity >
bool Array< ItemType, Capacity >::contains(const ItemType & item) const // O(n)
{
	return this->indexOf(item) != -1;
}

template< typename ItemType, uint8_t Capacity >
typename Array< ItemType, Capacity >::IndexOfType Array< ItemType, Capacity >::indexOf(const ItemType & item) const // O(n)
{
	for (IndexType i = this->getFirstIndex(); i < this->getCount(); ++i)
		if (this->items[i] == item)
			return i;
	return -1;
}

template< typename ItemType, uint8_t Capacity >
ItemType & Array< ItemType, Capacity >::operator [] (const typename Array< ItemType, Capacity >::IndexType & index) // O(1)
{
	//DEBUG_ASSERT(index < this->getCapacity());
	//DEBUG_ASSERT(index < this->next);

	return this->items[index];
}

template< typename ItemType, uint8_t Capacity >
const ItemType & Array< ItemType, Capacity >::operator [] (const typename Array< ItemType, Capacity >::IndexType & index) const // O(1)
{
	//DEBUG_ASSERT(index < this->getCapacity());
	//DEBUG_ASSERT(index < this->next);

	return this->items[index];
}