#include "mapper_002.h"

Mapper_002::Mapper_002(GamePak& game_pak)
    : Mapper(game_pak)
{
    _prg_banks[0] = 0;
    _prg_banks[1] = header()[4] - 1;
}

uint8_t Mapper_002::cpu_read(uint16_t address)
{
    if (address >= 0x8000)
    {
        uint8_t bank = _prg_banks[(address & 0x4000) >> 14];
        uint8_t* bank_mem = prg_rom(bank);
        return bank_mem[address & 0x3FFF] ;
    }
    return 0;
}

void Mapper_002::cpu_write(uint16_t address, uint8_t value)
{
    if (address > 0x8000)
    {
        _prg_banks[0] = value % (header()[4] - 1);
    }
}

bool Mapper_002::ppu_read(uint16_t address, uint8_t& value)
{
    if (address < 0x2000)
    {
        uint16_t offset = address & 0x1FFF;
        value = chr_rom()[offset];
        return true;
    }

    return false;
}

bool Mapper_002::ppu_write(uint16_t address, uint8_t value)
{
    if (address < 0x2000)
    {
        uint16_t offset = address & 0x1FFF;
        chr_rom()[offset] = value;
        return true;
    }

    return false;
}
