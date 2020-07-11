#include "mapper.h"

#include "gamepak.h"

std::array<char, 16>& Mapper::header()
{
    return _game_pak._header_mem;
}


std::vector<uint8_t>& Mapper::prg_rom()
{
    return _game_pak._prg_rom;
}


std::vector<uint8_t>& Mapper::chr_rom()
{
    return _game_pak._chr_rom;
}


uint8_t* Mapper::prg_rom(uint8_t bank)
{
    uintptr_t baseaddr = ((uintptr_t)bank) << 0xE;
    return _game_pak._prg_rom.data() + baseaddr;
}


gli2A03* Mapper::cpu()
{
    return _game_pak._cpu;
}


gli2C02* Mapper::ppu()
{
    return _game_pak._ppu;
}
