#pragma once

template< typename True, typename False, bool Condition > struct Conditional;
template< typename True, typename False > struct Conditional<True, False, true> { using Type = True; };
template< typename True, typename False > struct Conditional<True, False, false> { using Type = False; };

template< typename True, typename False, bool Condition > using ConditionalType = typename Conditional<True, False, Condition>::Type;

template< typename True, typename False > using LargerType = ConditionalType<True, False, (sizeof(True) > sizeof(False))>;
template< typename True, typename False > using SmallerType = ConditionalType<True, False, (sizeof(True) < sizeof(False))>;