#pragma once

#include <array>
#include <memory>

class GamePak;

class gli2C02
{
public:
    gli2C02() = default;
    ~gli2C02() = default;

    void reset(bool coldstart);
    void clock();

    void connect_game_pak(std::shared_ptr<GamePak>& game_pak);

    void cpu_write(uint16_t address, uint8_t value);
    uint8_t cpu_read(uint16_t address);

    uint32_t frame_number() { return _frame; }
    void get_pattern_table(uint8_t index, uint8_t palette, std::array<uint8_t, 0x4000>& table);

    std::array<uint8_t, 256 * 240> _screen;

private:
    std::shared_ptr<GamePak> _game_pak;
    std::array<uint8_t, 0x1000> _ram;
    std::array<uint8_t, 0x100> _oam;
    std::array<uint8_t, 0x20> _palette;

    uint8_t _ppuctrl;
    uint8_t _ppumask;
    uint8_t _ppustatus;
    uint8_t _oamaddr;
    uint8_t _address_latch;
    uint16_t _ppuscroll;
    uint16_t _ppuaddr;
//    uint8_t _t;   // TODO: temporary register
    uint8_t _ppudatabuffer;
    uint32_t _frame;
    uint16_t _scanline;
    uint16_t _cycle;
    uint8_t _reset;

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);
};
