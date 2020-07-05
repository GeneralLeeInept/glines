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

    _ppuctrl.reg = 0;
    _ppumask.reg = 0;
    _address_latch = 0;
    _ppudatabuffer = 0;
    _address_latch = 0;

    if (coldstart)
    {
        _ppustatus.O = 1;
        _ppustatus.V = 1;
        _oamaddr = 0;
        _ppuscroll = 0;
        _ppuaddr = 0;
        _temp_vram_address = 0;
        _fine_x_scroll = 0;
        _reset = 0;
    }
}


void gli2C02::clock()
{
    if (_reset)
    {
        if (_scanline >= 0 && _scanline < 240 && _cycle < 256)
        {
            uint8_t p = read(PpuMemoryMap::PALETTE_BASE);
            _screen[_scanline * 256 + _cycle] = p;
        }
    }
    else
    {
        if (_scanline < 240)
        {
            uint8_t bg_pixel = 0;
            uint8_t fg_pixel = 0;

            if (_ppumask.b || _ppumask.s)
            {
                // Update registers
                if (_cycle <= 256)
                {
                    uint8_t phase = (_cycle & 0x7);

                    if (_cycle == 0 && _scanline == 0)
                    {
                        // Cycle zero is idle except on odd frames it does the second tick of the last dummy nametable fetch.
                        phase = (_frame & 1) ? 2 : 0xFF;
                    }

                    // Fetch & latch
                    switch (phase)
                    {
                        case 1:
                        {
                            // Reload shift registers
                            _bl_shift = (_bl_shift & 0xFF00) | _bl_latch;
                            _bh_shift = (_bh_shift & 0xFF00) | _bh_latch;
                            _al_shift = (_al_shift & 0xFF00) | _al_latch;
                            _ah_shift = (_ah_shift & 0xFF00) | _ah_latch;
                            break;
                        }
                        case 2:
                        {
                            // Latch name table
                            uint16_t tile_address = PpuMemoryMap::NAMETABLE_BASE | (_ppuaddr & 0x0FFF);
                            _nt_latch = read(tile_address);
                            break;
                        }
                        case 4:
                        {
                            // Latch attribute byte
                            uint16_t attribute_address = 0x23C0 | (_ppuaddr & 0x0C00) | ((_ppuaddr & 0x0380) >> 4) | ((_ppuaddr & 0x001C) >> 2);
                            uint8_t attribute_byte = read(attribute_address);

                            // Select crumb for tile
                            uint8_t c = ((_ppuaddr & 0x3) < 2) ? 0 : 2;
                            uint8_t r = (((_ppuaddr >> 5) & 0x3) < 2) ? 0 : 4;
                            uint8_t s = (c + r);
                            _al_latch = (attribute_byte & (1 << s)) ? 0xFF : 0;
                            _ah_latch = (attribute_byte & (2 << s)) ? 0xFF : 0;
                            break;
                        }
                        case 6:
                        {
                            // Latch low BG tile byte
                            uint16_t address;
                            address = (_ppuctrl.B << 0xC) | (_nt_latch << 4) | (0 << 3) | ((_ppuaddr >> 12) & 0x7);
                            _bl_latch = read(address);
                            break;
                        }
                        case 0:
                        {
                            // Latch high BG tile byte
                            uint16_t address;
                            address = (_ppuctrl.B << 0xC) | (_nt_latch << 4) | (1 << 3) | ((_ppuaddr >> 12) & 0x7);
                            _bh_latch = read(address);
                            break;
                        }
                    }

                    if (phase == 0)
                    {
                        // Inc hori(v)
                        uint8_t coarse_x = _ppuaddr & 0x1F;
                        coarse_x++;
                        if (coarse_x == 0x20)
                        {
                            coarse_x = 0;
                            _ppuaddr ^= 0x0400; // Toggle bit 10 (low bit of nametable select) when coarse x wraps around
                        }
                        _ppuaddr = (_ppuaddr & ~0x1F) | (coarse_x & 0x1F);
                    }
                }
                else if (_cycle == 257)
                {
                    // Inc vert(v)
                    uint16_t fine_y = _ppuaddr >> 12;
                    fine_y++;
                    _ppuaddr = (_ppuaddr & ~0x7000) | (fine_y & 0x07) << 12;

                    if (fine_y & 0x08)
                    {
                        uint8_t coarse_y = (_ppuaddr & 0x03E0) >> 5;
                        coarse_y++;

                        /*
                            Row 29 is the last row of tiles in a nametable. To wrap to the next nametable when incrementing coarse Y from 29, the
                            vertical nametable is switched by toggling bit 11, and coarse Y wraps to row 0.

                            Coarse Y can be set out of bounds (> 29), which will cause the PPU to read the attribute data stored there as tile data.
                            If coarse Y is incremented from 31, it will wrap to 0, but the nametable will not switch.
                        */
                        if (coarse_y == 30)
                        {
                            coarse_y = 0;
                            _ppuaddr ^= 0x0800;
                        }

                        _ppuaddr = (_ppuaddr & ~0x03E0) | (coarse_y & 0x1F) << 5;
                    }

                    // hori(v) = hori(t)
                    _ppuaddr = (_ppuaddr & ~0x1F) | (_temp_vram_address & 0x1F);
                }
                else if (_cycle >= 280 && _cycle < 305)
                {
                    if (_scanline == -1)
                    {
                        // vert(v) = vert(t)
                        _ppuaddr = (_ppuaddr & ~0x73E0) | (_temp_vram_address & 0x73E0);
                    }
                }
                else if (_cycle > 320 && _cycle <= 337)
                {
                    if (_cycle == 322 || _cycle == 330)
                    {
                        // Latch name table
                        uint16_t tile_address = PpuMemoryMap::NAMETABLE_BASE | (_ppuaddr & 0x0FFF);
                        _nt_latch = read(tile_address);
                    }
                    else if (_cycle == 324 || _cycle == 332)
                    {
                        // Latch attribute byte
                        uint16_t attribute_address = 0x23C0 | (_ppuaddr & 0x0C00) | ((_ppuaddr & 0x0380) >> 4) | ((_ppuaddr & 0x001C) >> 2);
                        _al_latch = read(attribute_address);
                    }
                    else if (_cycle == 326 || _cycle == 334)
                    {
                        // Latch low BG tile byte
                        uint16_t address;
                        address = (_ppuctrl.B << 0xC) | (_nt_latch << 4) | (0 << 3) | ((_ppuaddr >> 12) & 0x7);
                        _bl_latch = read(address);
                    }
                    else if (_cycle == 328 || _cycle == 336)
                    {
                        // Latch high BG tile byte
                        uint16_t address;
                        address = (_ppuctrl.B << 0xC) | (_nt_latch << 4) | (1 << 3) | ((_ppuaddr >> 12) & 0x7);
                        _bh_latch = read(address);
                    }
                    else if (_cycle == 329 || _cycle == 337)
                    {
                        // Reload shift registers
                        _bl_shift = (_bl_shift & 0xFF00) | _bl_latch;
                        _bh_shift = (_bh_shift & 0xFF00) | _bh_latch;
                        _al_shift = _al_latch;
                        _ah_shift = _ah_latch;
                    }
                }

                if (_scanline >= 0 && _cycle < 256)
                {
                    // Produce background pixel
                    uint8_t al = _al_shift >> (15 - _fine_x_scroll);
                    uint8_t ah = _ah_shift >> (15 - _fine_x_scroll);
                    uint8_t bl = _bl_shift >> (15 - _fine_x_scroll);
                    uint8_t bh = _bh_shift >> (15 - _fine_x_scroll);
                    bg_pixel = (ah << 3) | (al << 2) | (bh << 1) | bl;
                }

                if ((_cycle >= 2 && _cycle <= 257) || (_cycle >= 322 && _cycle <= 337))
                {
                    // Shift background shift registers
                    _bl_shift <<= 1;
                    _bh_shift <<= 1;
                    _al_shift <<= 1;
                    _ah_shift <<= 1;
                }
            } // if _ppumask.b

            if (_scanline >= 0 && _cycle < 256)
            {
                // mux background and sprite (TODO - for now just the background)
                _screen[_scanline * 256 + _cycle] = read(PpuMemoryMap::PALETTE_BASE + bg_pixel);
            }
        } // if _scanline < 240
    }

    if (_scanline == 241 && _cycle == 1)
    {
        _ppustatus.V = 1;

        if (_ppuctrl.V)
            _nmi = 1;
    }
    else if (_scanline == -1 && _cycle == 1)
    {
        _ppustatus.reg = 0;
        _reset = 0;
    }

    ++_cycle;

    if (_scanline == -1 && _cycle == 340 && (_frame & 1))
    {
        // Skip the last cycle on the prerender scanline of odd frames
        ++_cycle;
    }

    if (_cycle == 341)
    {
        _cycle = 0;
        ++_scanline;

        if (_scanline == 261)
        {
            _scanline = -1;
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
        (Clearing PPUSCROLL and PPUADDR corresponds to clearing the VRAM address latch (W) and the fine X scroll. Note that the VRAM address itself (V)
        is not cleared.) This reset signal is set on reset and cleared at the end of VBlank, by the same signal that clears the VBlank, sprite 0, and
        overflow flags. Attempting to write to a register while it is being cleared has no effect, which explains why writes are "ignored" after reset.

        See https://wiki.nesdev.com/w/index.php/PPU_scrolling for information on how the internal registers are written by writes to $2005 & $2006.
    */

    switch (address)
    {
        case PpuRegisters::PPUCTRL:
        {
            if (!_reset)
            {
                _ppuctrl.reg = value;
                _temp_vram_address = (_temp_vram_address & ~0x0C00) | (((uint16_t)value << 10) & 0x0C00);
            }

            break;
        }
        case PpuRegisters::PPUMASK:
        {
            if (!_reset)
                _ppumask.reg = value;

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

            if (_scanline > 239)
                _oam[_oamaddr++] = value;

            break;
        }
        case PpuRegisters::PPUSCROLL:
        {
            if (!_reset)
            {
                if (_address_latch == 0)
                {
                    _ppuscroll = (_ppuscroll & 0xFF00) | (value << 8);

                    /*
                        $2005 first write (w is 0)

                        t: ....... ...HGFED = d: HGFED...
                        x:              CBA = d: .....CBA
                        w:                  = 1
                    */
                    _temp_vram_address = (_temp_vram_address & ~0x001F) | ((value >> 3) & 0x1F);
                    _fine_x_scroll = value & 0x1F;
                    _address_latch = 1;
                }
                else
                {
                    _ppuscroll = (_ppuscroll & 0x00FF) | value;

                    /*
                        $2005 second write (w is 1)

                        t: CBA..HG FED..... = d: HGFEDCBA
                        w:                  = 0
                    */
                    _temp_vram_address = (_temp_vram_address & ~0x39F0) | (((uint16_t)value << 12) & 0x3000) | (((uint16_t)value << 2) & 0x09F0);
                    _address_latch = 0;
                }

            }

            break;
        }
        case PpuRegisters::PPUADDR:
        {
            if (!_reset)
            {
                if (_address_latch == 0)
                {
                    /*
                        $2006 first write (w is 0)

                        t: .FEDCBA ........ = d: ..FEDCBA
                        t: X...... ........ = 0
                        w:                  = 1
                    */
                    _temp_vram_address = (_temp_vram_address & ~0x3F00) | (((uint16_t)value << 8) & 0x3F00);
                    _temp_vram_address = _temp_vram_address & 0x7FFF;
                    _address_latch = 1;
                }
                else
                {
                    /*
                        $2006 second write (w is 1)

                        t: .......HGFEDCBA = d : HGFEDCBA
                        v = t
                        w : = 0
                    */
                    _temp_vram_address = (_temp_vram_address & ~0x00FF) | value;
                    _ppuaddr = _temp_vram_address;
                    _address_latch = 0;
                }
            }

            break;
        }
        case PpuRegisters::PPUDATA:
        {
            // VRAM read/write data register. After access, the video memory address will increment by an amount determined by bit 2 of $2000.
            write(_ppuaddr, value);
            _ppuaddr += (_ppuctrl.I == 0) ? 1 : 32;
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
            value = (_ppustatus.reg & 0xE0) | (_ppudatabuffer & 0x1F);

            /* Reading the status register clears vblank bit and the address latch used by PPUSCROLL and PPUADDR. */
            _ppustatus.V = 0;
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
            _ppuaddr += (_ppuctrl.I == 0) ? 1 : 32;
            break;
        }
    }

    return value;
}


void gli2C02::get_pattern_table(uint8_t index, uint8_t palette, std::array<uint8_t, 0x4000>& table)
{
    for (uint16_t t = 0; t < 256; ++t)
    {
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
