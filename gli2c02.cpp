#include "gli2c02.h"

#include "bits.h"
#include "gamepak.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>


enum PpuRegisters : uint16_t
{
    PPUCTRL     = 0x2000,
    PPUMASK     = 0x2001,
    PPUSTATUS   = 0x2002,
    OAMADDR     = 0x2003,
    OAMDATA     = 0x2004,
    PPUSCROLL   = 0x2005,
    PPUADDR     = 0x2006,
    PPUDATA     = 0x2007,

    REG_BASE    = 0x2000,
    REG_MASK    = 0x200F
};


enum class PpuCtrlBits : uint8_t
{
    I = 2,
    S = 3,
    B = 4,
    H = 5,
    P = 6,
    V = 7
};


enum class PpuStatusBits : uint8_t
{
    O = 5,
    S = 6,
    V = 7
};


enum PpuMemoryMap
{
    PATTERN_TABLE_BASE      = 0x0000,
    PATTERN_TABLE0_BASE     = 0x0000,
    PATTERN_TABLE0_TOP      = 0x0FFF,
    PATTERN_TABLE1_BASE     = 0x1000,
    PATTERN_TABLE1_TOP      = 0x1FFF,
    PATTERN_TABLE_TOP       = 0x1FFF,

    NAMETABLE_BASE          = 0x2000,
    NAMETABLE0_BASE         = 0x2000,
    NAMETABLE0_TOP          = 0x23FF,
    NAMETABLE1_BASE         = 0x2400,
    NAMETABLE1_TOP          = 0x27FF,
    NAMETABLE2_BASE         = 0x2800,
    NAMETABLE2_TOP          = 0x2BFF,
    NAMETABLE3_BASE         = 0x2C00,
    NAMETABLE3_TOP          = 0x2FFF,
    NAMETABLE_TOP           = 0x2FFF,
    NAMETABLE_MASK          = 0x2FFF,

    NAMETABLE_MIRROR_BASE   = 0x3000,
    NAMETABLE_MIRROR_TOP    = 0x3EFF,

    PALETTE_BASE            = 0x3F00,
    PALETTE_TOP             = 0x3FFF,
    PALETTE_MASK            = 0x3F1F,
};


void gli2C02::reset(bool coldstart)
{
    // https://wiki.nesdev.com/w/index.php/PPU_power_up_state
    //
    // Initial Register Values
    //  Register                            At Power        After Reset
    //  PPUCTRL($2000)                      0000 0000       0000 0000
    //  PPUMASK($2001)                      0000 0000       0000 0000
    //  PPUSTATUS($2002)                    +0+x xxxx       U??x xxxx
    //  OAMADDR($2003)                      $00             unchanged (^1)
    //  $2005 / $2006 latch                 cleared         cleared
    //  PPUSCROLL($2005)                    $0000           $0000
    //  PPUADDR($2006)                      $0000           unchanged
    //  PPUDATA($2007) read buffer          $00             $00
    //  odd frame                           no              no
    //  OAM                                 unspecified     unspecified
    //  Palette                             unspecified     unchanged
    //  NT RAM(external, in Control Deck)   unspecified     unchanged
    //  CHR RAM(external, in Game Pak)      unspecified     unchanged
    //
    //  ? = unknown, x = irrelevant, + = often set, U = unchanged
    //  (^1): Although OAMADDR is unchanged by reset, it is changed during rendering and cleared at the end of normal rendering, so you should assume
    //        its contents will be random.

    _frame = 0;
    _scanline = 0;
    _cycle = 0;

    _ppuctrl = 0;
    _ppumask = 0;
    _address_latch = 0;
    _ppudatabuffer = 0;

    if (coldstart)
    {
        _ppustatus = (_ppustatus & 0x1F) | static_cast<uint8_t>(PpuStatusBits::V) | static_cast<uint8_t>(PpuStatusBits::O);
        _oamaddr = 0;
        _ppuscroll = 0;
        _ppuaddr = 0;
        _reset = 0;
    }
}


void gli2C02::clock()
{
    if (_scanline < 240 && _cycle < 256)
    {
        // Produce pixel
        uint8_t p = 0x3f;
        _screen[_scanline * 256 + _cycle] = p;
    }
    else if (_scanline == 241 && _cycle == 1)
    {
        set_bit(_ppustatus, static_cast<uint8_t>(PpuStatusBits::V), 1);
    }
    else if (_scanline == 261 && _cycle == 1)
    {
        set_bit(_ppustatus, static_cast<uint8_t>(PpuStatusBits::V), 0);
        set_bit(_ppustatus, static_cast<uint8_t>(PpuStatusBits::S), 0);
        set_bit(_ppustatus, static_cast<uint8_t>(PpuStatusBits::O), 0);
        _reset = 0;
    }

    ++_cycle;

    if (_scanline == 261 && _cycle == 340 && (_frame & 1))
    {
        // Skip the last cycle on the prerender scanline of odd frames
        ++_cycle;
    }

    if (_cycle == 341)
    {
        _cycle = 0;
        ++_scanline;

        if (_scanline == 262)
        {
            _scanline = 0;
        }
    }

    if (_scanline == 0 && _cycle == 0)
    {
        _frame++;
    }
}


void gli2C02::connect_game_pak(std::shared_ptr<GamePak>& game_pak)
{
    _game_pak = game_pak;
}


void gli2C02::cpu_write(uint16_t address, uint8_t value)
{
    address = address & REG_MASK;

    /*
        There is an internal reset signal that clears PPUCTRL, PPUMASK, PPUSCROLL, PPUADDR, the PPUSCROLL/PPUADDR latch, and the PPUDATA read buffer.
        (Clearing PPUSCROLL and PPUADDR corresponds to clearing the VRAM address latch (T) and the fine X scroll. Note that the VRAM address itself (V)
        is not cleared.) This reset signal is set on reset and cleared at the end of VBlank, by the same signal that clears the VBlank, sprite 0, and
        overflow flags. Attempting to write to a register while it is being cleared has no effect, which explains why writes are "ignored" after reset.
    */

    switch (address)
    {
        case PpuRegisters::PPUCTRL:
        {
            if (!_reset)
                _ppuctrl = value;

            break;
        }
        case PpuRegisters::PPUMASK:
        {
            if (!_reset)
                _ppumask = value;

            break;
        }
        case PpuRegisters::OAMADDR:
        {
            _oamaddr = value;
            break;
        }
        case PpuRegisters::OAMDATA:
        {
            /*
                Writes to OAMDATA during rendering (on the pre-render line and the visible lines 0-239, provided either sprite or background
                rendering is enabled) do not modify values in OAM, but do perform a glitchy increment of OAMADDR, bumping only the high 6 bits
                (i.e., it bumps the [n] value in PPU sprite evaluation - it's plausible that it could bump the low bits instead depending on the
                current status of sprite evaluation). This extends to DMA transfers via OAMDMA, since that uses writes to $2004. For emulation
                purposes, it is probably best to completely ignore writes during rendering.
            */

            if (_scanline > 240 && _scanline < 261)
                _oam[_oamaddr++] = value;

            break;
        }
        case PpuRegisters::PPUSCROLL:
        {
            if (!_reset)
            {
                // TODO: https://wiki.nesdev.com/w/index.php/PPU_scrolling
                _address_latch = 1 - _address_latch;
            }

            break;
        }
        case PpuRegisters::PPUADDR:
        {
            if (!_reset)
            {
                // TODO: https://wiki.nesdev.com/w/index.php/PPU_scrolling
                // Not doing this properly yet but need to let the program write the address so it can read/write PPUDATA correctly
                if (_address_latch)
                {
                    _ppuaddr = (_ppuaddr & 0xFF00) | value;
                }
                else
                {
                    _ppuaddr = (_ppuaddr & 0x00FF) | (value << 8);
                }

                _address_latch = 1 - _address_latch;
            }

            break;
        }
        case PpuRegisters::PPUDATA:
        {
            // VRAM read/write data register. After access, the video memory address will increment by an amount determined by bit 2 of $2000.
            write(_ppuaddr, value);
            _ppuaddr += (get_bit(_ppuctrl, static_cast<uint8_t>(PpuCtrlBits::I)) == 0) ? 1 : 32;
            break;
        }
    }
}


uint8_t gli2C02::cpu_read(uint16_t address)
{
    address = address & REG_MASK;

    /*
        Reading any readable port (PPUSTATUS, OAMDATA, or PPUDATA) also fills the latch with the bits read. Reading a nominally "write-only" register
        returns the latch's current value, as do the unused bits of PPUSTATUS.
    */

    uint8_t value = _ppudatabuffer;

    switch (address)
    {
        case PpuRegisters::PPUSTATUS:
        {
            value = (_ppustatus & 0xE0) | (_ppudatabuffer & 0x1F);

            /* Reading the status register clears bit 7 and also the address latch used by PPUSCROLL and PPUADDR. */
            set_bit(_ppustatus, static_cast<uint8_t>(PpuStatusBits::V), 0);
            _address_latch = 0;
            break;
        }
        case PpuRegisters::OAMDATA:
        {
            value = _oam[_oamaddr];
            break;
        }
        case PpuRegisters::PPUDATA:
        {
            /*
                The PPUDATA read buffer(post - fetch)
                When reading while the VRAM address is in the range 0 - $3EFF(i.e., before the palettes), the read will return the contents of an
                internal read buffer.This internal buffer is updated only when reading PPUDATA, and so is preserved across frames.After the CPU reads
                and gets the contents of the internal buffer, the PPU will immediately update the internal buffer with the byte at the current VRAM
                address.Thus, after setting the VRAM address, one should first read this registerand discard the result.

                Reading palette data from $3F00 - $3FFF works differently.The palette data is placed immediately on the data bus, and hence no dummy
                read is required.Reading the palettes still updates the internal buffer though, but the data placed in it is the mirrored nametable
                data that would appear "underneath" the palette.
            */

            if (_ppuaddr >= PpuMemoryMap::PALETTE_BASE)
            {
                value = read(_ppuaddr & PpuMemoryMap::NAMETABLE_MASK);
            }
            else
            {
                value = _ppudatabuffer;
            }

            _ppudatabuffer = read(_ppuaddr & PpuMemoryMap::NAMETABLE_MASK);
            _ppuaddr += (get_bit(_ppuctrl, static_cast<uint8_t>(PpuCtrlBits::I)) == 0) ? 1 : 32;
            break;
        }
    }

    return value;
}


void gli2C02::get_pattern_table(uint8_t index, uint8_t palette, std::array<uint8_t, 0x4000>& table)
{
    for (uint16_t t = 0; t < 256; ++t)
    {
        std::array<uint8_t, 16> tile_bytes;

        for (uint8_t i = 0; i < 16; ++i)
        {
            uint16_t addr = static_cast<uint16_t>(index) << 12 | t << 4 | i;
            tile_bytes[i] = read(addr);
        }

        uint8_t tx = (t & 0xF) << 3;
        uint8_t ty = (t >> 4) << 3;

        for (uint8_t y = 0; y < 8; ++y)
        {
            uint16_t addr = static_cast<uint16_t>(index) << 12 | t << 4 | y;
            uint8_t tile_lsb = read(addr);
            uint8_t tile_msb = read(addr + 8);

            for (uint8_t x = 0; x < 8; ++x)
            {
                uint8_t bit = 7 - x;
                uint8_t lsb = get_bit(tile_lsb, bit);
                uint8_t msb = get_bit(tile_msb, bit);
                uint8_t palette_index = (msb << 1) | lsb;
                uint16_t table_index = ((ty + y) << 7) + (tx + x);
                table[table_index] = read(PALETTE_BASE + palette_index + (palette << 2));
            }
        }
    }
}


uint8_t gli2C02::read(uint16_t address)
{
    if (address >= PpuMemoryMap::NAMETABLE_MIRROR_BASE && address <= PpuMemoryMap::NAMETABLE_MIRROR_TOP)
        address -= PpuMemoryMap::NAMETABLE_MIRROR_BASE - PpuMemoryMap::NAMETABLE_BASE;

    uint8_t value = 0;

    if (address <= PpuMemoryMap::PATTERN_TABLE_TOP)
    {
        if (_game_pak)
            _game_pak->ppu_read(address, value);
    }
    else if (address <= NAMETABLE_TOP)
    {
        if (_game_pak)
            address = _game_pak->ppu_remap_address(address);


        if (!_game_pak || !_game_pak->ppu_read(address, value))
        {
            address = address & 0x7FF;
            value = _ram[address];
        }
    }
    else if (address <= PpuMemoryMap::PALETTE_TOP)
    {
        address = (address & PpuMemoryMap::PALETTE_MASK) - PpuMemoryMap::PALETTE_BASE;
        value = _palette[address];
    }

    return value;
}


void gli2C02::write(uint16_t address, uint8_t value)
{
    if (address >= PpuMemoryMap::NAMETABLE_MIRROR_BASE && address <= PpuMemoryMap::NAMETABLE_MIRROR_TOP)
        address -= PpuMemoryMap::NAMETABLE_MIRROR_BASE - PpuMemoryMap::NAMETABLE_BASE;

    if (address <= PpuMemoryMap::PATTERN_TABLE_TOP)
    {
        if (_game_pak)
            _game_pak->ppu_write(address, value);
    }
    else if (address <= NAMETABLE_TOP)
    {
        if (_game_pak)
            address = _game_pak->ppu_remap_address(address);

        if (!_game_pak || !_game_pak->ppu_write(address, value))
        {
            address = address & 0x7FF;
            _ram[address] = value;
        }
    }
    else if (address <= PpuMemoryMap::PALETTE_TOP)
    {
        address = (address & PpuMemoryMap::PALETTE_MASK) - PpuMemoryMap::PALETTE_BASE;
        _palette[address] = value;
    }
}
