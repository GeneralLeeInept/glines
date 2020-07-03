#pragma once

#include <cstdint>


template <typename T>
void set_bit(T& flags, uint8_t bit, int set)
{
    T mask = T(1) << bit;
    flags |= set ? mask : T(0);
    flags &= set ? ~T(0) : ~mask;
}


template <typename T>
int get_bit(T flags, uint8_t bit)
{
    return (flags >> bit) & 1;
}


inline uint8_t lo(uint16_t w)
{
    return (uint8_t)(w & 0xff);
}


inline uint8_t hi(uint16_t w)
{
    return (uint8_t)((w & 0xff00) >> 8);
}


inline uint16_t word(uint8_t lo, uint8_t hi)
{
    return (((uint16_t)hi) << 8) | lo;
}
