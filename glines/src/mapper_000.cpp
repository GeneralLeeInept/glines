#include "mapper_000.h"

Mapper_000::Mapper_000(GamePak& game_pak)
    : Mapper(game_pak)
{
}


uint8_t Mapper_000::cpu_read(uint16_t address)
{
    if (address >= 0x8000)
    {
        address &= (prg_rom().size() - 1);
        return prg_rom()[address];
    }

    return 0;
}


void Mapper_000::cpu_write(uint16_t address, uint8_t value)
{
}


bool Mapper_000::ppu_read(uint16_t address, uint8_t& value)
{
    if (address < 0x2000)
    {
        address &= (chr_rom().size() - 1);
        value = chr_rom()[address];
        return true;
    }

    return false;
}


bool Mapper_000::ppu_write(uint16_t address, uint8_t value)
{
    return false;
}
