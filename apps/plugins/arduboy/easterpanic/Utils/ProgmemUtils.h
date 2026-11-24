#pragma once

// Might be needed for memcpy_P
#include <avr/pgmspace.h>

//
// FlashString
//

inline const __FlashStringHelper * FlashString(const char * flashString)
{
	return reinterpret_cast<const __FlashStringHelper*>(flashString);
}

namespace Details
{
	// Pay no attention to the man behind the curtain

	template< typename T, size_t Size = sizeof(T) >
	struct ProgmemReadHelper;

	template< typename T, size_t Size >
	struct ProgmemReadHelper
	{
		inline static T ProgmemRead(T const * const progmem)
		{
			T t;
			memcpy_P(&t, progmem, sizeof(T));
			return t;
		}
	};

	template< typename T >
	struct ProgmemReadHelper<T, 1>
	{
		constexpr inline static T ProgmemRead(T const * const progmem)
		{
			return static_cast<T>(pgm_read_byte(progmem));
		}
	};

	
	// Fix later
	/*template< typename T >
	struct ProgmemReadHelper<T, 2>
	{
		constexpr inline static T ProgmemRead(T const * const progmem)
		{
			return static_cast<T>(pgm_read_word(progmem));
		}
	};*/

	/*template<>
	struct ProgmemReadHelper<uint16_t, 2>
	{
		constexpr inline static uint16_t ProgmemRead(uint16_t const * const progmem)
		{
			return static_cast<uint16_t>(pgm_read_word(progmem));
		}
	};

	template<>
	struct ProgmemReadHelper<int16_t, 2>
	{
		constexpr inline static int16_t ProgmemRead(int16_t const * const progmem)
		{
			return static_cast<int16_t>(pgm_read_word(progmem));
		}
	};*/

	template< typename T >
	struct ProgmemReadHelper<T*, 2>
	{
		constexpr inline static T * ProgmemRead(T const * const * const progmem)
		{
			return reinterpret_cast<T *>(pgm_read_word(progmem));
		}
	};
}

//
// ProgmemRead
//

// As if by magic
template< typename T >
inline T ProgmemRead(const T * const progmem)
{
	return Details::ProgmemReadHelper<T>::ProgmemRead(progmem);
}

//
// ProgmemMemoryCopy overloads
//

template< typename T >
inline T * ProgmemMemoryCopy(T * destination, const T & source)
{
	return reinterpret_cast<T *>(memcpy_P(destination, &source, sizeof(T)));
}

template< typename T >
inline T * ProgmemMemoryCopy(T & destination, const T * source)
{
	return reinterpret_cast<T *>(memcpy_P(&destination, source, sizeof(T)));
}

template< typename T >
inline T * ProgmemMemoryCopy(T * destination, const T * source)
{
	return reinterpret_cast<T *>(memcpy_P(destination, source, sizeof(T)));
}

template< typename T >
inline T * ProgmemMemoryCopy(T & destination, const T & source, size_t n)
{
	return reinterpret_cast<T *>(memcpy_P(&destination, &source, sizeof(T) * n));
}

template< typename T >
inline T * ProgmemMemoryCopy(T * destination, const T & source, size_t n)
{
	return reinterpret_cast<T *>(memcpy_P(destination, &source, sizeof(T) * n));
}

template< typename T >
inline T * ProgmemMemoryCopy(T & destination, const T * source, size_t n)
{
	return reinterpret_cast<T *>(memcpy_P(&destination, source, sizeof(T) * n));
}

template< typename T >
inline T * ProgmemMemoryCopy(T * destination, const T * source, size_t n)
{
	return reinterpret_cast<T *>(memcpy_P(destination, source, sizeof(T) * n));
}

template< typename T >
inline T * ProgmemMemoryCopy(T & destination, const T & source)
{
	return reinterpret_cast<T *>(memcpy_P(&destination, &source, sizeof(T)));
}