#pragma once

//
// ArrayLength
//

template< typename T >
constexpr size_t ArrayLength(T(&)[0])
{
	return 0;
}

template< typename T, size_t Size >
constexpr size_t ArrayLength(T (&)[Size])
{
	return Size;
}

//
// ArrayFirstIndex
//

template< typename T >
constexpr size_t ArrayFirstIndex(T(&)[0])
{
	return 0;
}

template< typename T, size_t Size >
constexpr size_t ArrayFirstIndex(T (&)[Size])
{
	return 0;
}

//
// ArrayLastIndex
//

template< typename T >
constexpr size_t ArrayLastIndex(T(&)[0])
{
	return 0;
}

template< typename T, size_t Size >
constexpr size_t ArrayLastIndex(T (&)[Size])
{
	return Size - 1;
}

//
// StringLength
//

template< typename T >
constexpr size_t StringLength(T (&)[0])
{
	return 0;
}

template< typename T, size_t Size >
constexpr size_t StringLength(T (&)[Size])
{
	return Size - 1;
}