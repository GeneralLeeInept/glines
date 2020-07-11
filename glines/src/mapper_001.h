#pragma once

#include "mapper.h"

class Mapper_001 : public Mapper
{
public:
    Mapper_001(GamePak& game_pak);

    void reset(bool coldstart) override;

    uint8_t cpu_read(uint16_t address) override;
    void cpu_write(uint16_t address, uint8_t value) override;

    bool ppu_read(uint16_t address, uint8_t& value) override;
    bool ppu_write(uint16_t address, uint8_t value) override;
    bool ppu_remap_address(uint16_t& address) override;

protected:
    void update_prg_rom_mapping();
    void update_chr_rom_mapping();

    // PRG RAM
    std::array<uint8_t, 0x2000> _prg_ram;

    // Registers
    uint8_t _load;
    uint8_t _control;
    uint8_t _chr_bank_0;
    uint8_t _chr_bank_1;
    uint8_t _prg_bank;

    // PRG ROM mapping
    uint32_t _x8000;
    uint32_t _xC000;

    // CHR ROM mapping
    uint16_t _x0000;
    uint16_t _x1000;
};
