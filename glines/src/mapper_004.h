#pragma once

#include "mapper.h"

class Mapper_004 : public Mapper
{
public:
    Mapper_004(GamePak& game_pak);

    void reset(bool coldstart) override;

    uint8_t cpu_read(uint16_t address) override;
    void cpu_write(uint16_t address, uint8_t value) override;

    bool ppu_read(uint16_t address, uint8_t& value) override;
    bool ppu_write(uint16_t address, uint8_t value) override;
    bool ppu_remap_address(uint16_t& address) override;

private:
    std::array<uint8_t, 0x2000> _prg_ram{};
    std::array<uint8_t, 8> _bank_select_registers{};


    uint64_t _last_ppu_clock_count;
    uint16_t _last_ppu_address;
    uint8_t _bank_select;
    uint8_t _irq_latch;
    uint8_t _irq_reload;
    uint8_t _irq_counter;
    uint8_t _irq_enabled;
    uint8_t _mirroring;


    void clock_irq();
}; 
