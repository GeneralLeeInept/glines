#include "mapper_003.h"

Mapper_003::Mapper_003(GamePak& game_pak)
    : Mapper(game_pak)
{
}


uint8_t Mapper_003::cpu_read(uint16_t address)
{
    if (address >= 0x8000)
    {
        address &= (prg_rom().size() - 1);
        return prg_rom()[address];
    }

    return 0;
}


void Mapper_003::cpu_write(uint16_t address, uint8_t value)
{
    if (address > 0x8000)
    {
        _chr_bank = value & (header()[5] - 1);
    }
}


bool Mapper_003::ppu_read(uint16_t address, uint8_t& value)
{
    if (address < 0x2000)
    {
        uint32_t base = _chr_bank * 0x2000;
        uint32_t offset = address & 0x1FFF;
        uint32_t read_address = base + offset;
        value = chr_rom()[read_address];
        return true;
    }

    return false;
}


bool Mapper_003::ppu_write(uint16_t address, uint8_t value)
{
    return false;
}
