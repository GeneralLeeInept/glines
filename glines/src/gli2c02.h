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
    void get_pattern_table(uint8_t table_index, uint8_t palette_index, std::array<uint8_t, 0x4000>& pattern_table);
    void get_palette(uint8_t palette_index, std::array<uint8_t, 4>& palette);

    void clear_nmi() { _nmi = 0; }
    uint8_t nmi() { return _nmi; }
    uint64_t clock_count() { return _clocks; }

    std::array<uint8_t, 256 * 240> _screen;

private:
    union PpuCtrlRegister
    {
        uint8_t reg;

        struct
        {
            uint8_t N : 2;  // Base nametable address
            uint8_t I : 1;  // VRAM address increment (0: add 1; 1: add 32)
            uint8_t S : 1;  // Sprite pattern table address for 8x8 sprites (0: $0000; 1: $1000; ignored in 8x16 mode)
            uint8_t B : 1;  // Background pattern table address (0: $0000; 1: $1000)
            uint8_t H : 1;  // Sprite size (0: 8x8 pixels; 1: 8x16 pixels)
            uint8_t P : 1;  // PPU master/slave select (0: read backdrop from EXT pins; 1: output color on EXT pins) #NOTIMPLEMENTED
            uint8_t V : 1;  // Generate an NMI at the start of the vertical blanking interval(0: off; 1: on)
        };
    };


    union PpuMaskRegister
    {
        uint8_t reg;

        struct
        {
            uint8_t g : 1;  // Greyscale (0: normal color, 1: produce a greyscale display)
            uint8_t m : 1;  // 1: Show background in leftmost 8 pixels of screen, 0: Hide
            uint8_t M : 1;  // 1: Show sprites in leftmost 8 pixels of screen, 0: Hide
            uint8_t b : 1;  // 1: Show background
            uint8_t s : 1;  // 1: Show sprites
            uint8_t R : 1;  // Emphasize red
            uint8_t G : 1;  // Emphasize green
            uint8_t B : 1;  // Emphasize blue
        };
    };


    union PpuStatusRegister
    {
        uint8_t reg;

        struct
        {
            uint8_t u : 5;  // Unused
            uint8_t O : 1;  // Sprite overflow
            uint8_t S : 1;  // Sprite zero hit
            uint8_t V : 1;  // Vertical blank
        };
    };


    struct SpriteOutputUnit
    {
        uint8_t pattern_lo;
        uint8_t pattern_hi;
        uint8_t attributes;
        uint8_t x_position;
    };


    std::shared_ptr<GamePak> _game_pak;
    std::array<uint8_t, 0x800> _ram;
    std::array<uint8_t, 0x100> _oam;
    std::array<uint8_t, 0x20> _secondary_oam;
    std::array<uint8_t, 0x20> _palette;


    // Registers
    PpuCtrlRegister _ppuctrl;
    PpuMaskRegister _ppumask;
    PpuStatusRegister _ppustatus;
    uint8_t _oamaddr;
    uint16_t _ppuaddr;  // 'v' register


    // Internal registers
    uint8_t _ppudatabuffer;
    uint16_t _temp_vram_address; // 't' register
    uint8_t _fine_x_scroll; // 'x' register
    uint8_t _address_latch; // 'w' register

    /*
        These contain the pattern table data for two tiles. Every 8 cycles, the pattern data for the next tile is loaded into the upper 8 bits of the
        shift register. Meanwhile, the pixel to render is fetched from one of the lower 8 bits.
    */
    uint16_t _bl_shift;
    uint16_t _bh_shift;

    /*
        These contain the palette attributes for the lower 8 pixels of the 16-bit shift register. These registers are fed by a latch which contains
        the palette attribute for the next tile. Every 8 cycles, the latch is loaded with the palette attribute for the next tile.
    */
    uint16_t _al_shift;
    uint16_t _ah_shift;

    /*
        These latches are used to store bytes fetched from memory before they are loaded into the shift registers
    */
    uint8_t _nt_latch;
    uint8_t _bl_latch;
    uint8_t _bh_latch;
    uint8_t _attribute_latch;

    std::array<SpriteOutputUnit, 8> _sprite_output_units;
    uint8_t _active_sprites; // // 0x AA BB -- A: active sprites next scanline, B: active sprites current scanline
    uint8_t _sprite_zero_visible; // 0b XXXX XX A B -- A: sprite zero visible next scanline, B: sprite zero visible current scanline


    // Emulation state
    uint64_t _clocks;
    uint32_t _frame;
    int16_t _scanline;
    uint16_t _cycle;
    uint8_t _state_flags;
    uint8_t _nmi;


    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);
};
