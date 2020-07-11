#pragma once

#include "mapper.h"

class Mapper_002 : public Mapper
{
public:
    Mapper_002(GamePak& game_pak);

    uint8_t cpu_read(uint16_t address) override;
    void cpu_write(uint16_t address, uint8_t value) override;
    bool ppu_read(uint16_t address, uint8_t& value) override;
    bool ppu_write(uint16_t address, uint8_t value) override;
    
public:
    uint8_t _prg_banks[2];
}; 


