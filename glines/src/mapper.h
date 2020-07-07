#pragma once

#include <array>
#include <vector>

class GamePak;

class Mapper
{
public:
    Mapper(GamePak& game_pak)
        : _game_pak{ game_pak } {}

    virtual uint8_t cpu_read(uint16_t address) = 0;
    virtual void cpu_write(uint16_t address, uint8_t value) = 0;

    virtual bool ppu_read(uint16_t address, uint8_t& value) = 0;
    virtual bool ppu_write(uint16_t address, uint8_t value) = 0;

    virtual bool ppu_remap_address(uint16_t& address) { return false;  }

protected:
    GamePak& _game_pak;

    std::array<char, 16>& header();
    std::vector<uint8_t>& prg_rom();
    std::vector<uint8_t>& chr_rom();
    uint8_t* prg_rom(uint8_t bank);
};
