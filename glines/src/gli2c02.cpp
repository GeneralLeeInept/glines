#include "gli2c02.h"

#include "bits.h"
#include "gamepak.h"


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
    REG_MASK    = 0x2007
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
    PALETTE_MASK            = 0x1F,
};


// Shift and mask constants for VRAM address registers (v & t registers)
static constexpr uint16_t PpuAddrCoarseXShift = 0x0;
static constexpr uint16_t PpuAddrCoarseXMask = (0x1F << PpuAddrCoarseXShift);
static constexpr uint16_t PpuAddrCoarseYShift = 0x5;
static constexpr uint16_t PpuAddrCoarseYMask = (0x1F << PpuAddrCoarseYShift);
static constexpr uint16_t PpuAddrNametableXShift = 0xA;
static constexpr uint16_t PpuAddrNametableXMask = (0x1 << PpuAddrNametableXShift);
static constexpr uint16_t PpuAddrNametableYShift = 0xB;
static constexpr uint16_t PpuAddrNametableYMask = (0x1 << PpuAddrNametableYShift);
static constexpr uint16_t PpuAddrFineYShift = 0xC;
static constexpr uint16_t PpuAddrFineYMask = (0x7 << PpuAddrFineYShift);


// Emulation state flags
enum StateFlags
{
    Reset       = 0x01, // PPU internal reset signal is set
    OamReadMask = 0x02, // Reads from OAMDATA return 0xFF
};

void gli2C02::reset(bool coldstart)
{
    /*
        https://wiki.nesdev.com/w/index.php/PPU_power_up_state

        Initial Register Values

        Register                             |   At Power     |   After Reset
        -------------------------------------+----------------+--------------
        PPUCTRL($2000)                       |   0000 0000    |   0000 0000
        PPUMASK($2001)                       |   0000 0000    |   0000 0000
        PPUSTATUS($2002)                     |   +0+x xxxx    |   U??x xxxx
        OAMADDR($2003)                       |   $00          |   unchanged (1)
        $2005 / $2006 latch                  |   cleared      |   cleared
        PPUSCROLL($2005)                     |   $0000        |   $0000
        PPUADDR($2006)                       |   $0000        |   unchanged
        PPUDATA($2007) read buffer           |   $00          |   $00
        odd frame                            |   no           |   no
        OAM                                  |   unspecified  |   unspecified
        Palette                              |   unspecified  |   unchanged
        NT RAM(external, in Control Deck)    |   unspecified  |   unchanged
        CHR RAM(external, in Game Pak)       |   unspecified  |   unchanged
        
        ? = unknown, x = irrelevant, + = often set, U = unchanged
        (1): Although OAMADDR is unchanged by reset, it is changed during rendering and cleared at the end of normal rendering, so you should assume
            its contents will be random.
    */

    _ppuctrl.reg = 0;
    _ppumask.reg = 0;
    _address_latch = 0;
    _ppudatabuffer = 0;
    _address_latch = 0;

    _active_sprites = 0;
    _sprite_zero_visible = 0;

    _clocks = 0;
    _frame = 0;
    _scanline = 0;
    _cycle = 0;
    _state_flags = StateFlags::Reset;
    _nmi = 0;

    if (coldstart)
    {
        _ppustatus.reg = 0;
        _oamaddr = 0;
        _ppuaddr = 0;
        _temp_vram_address = 0;
        _fine_x_scroll = 0;
    }
}


void gli2C02::clock()
{
    ++_clocks;

    if (_state_flags & StateFlags::Reset)
    {
        if (_scanline >= 0 && _scanline < 240 && _cycle < 256)
        {
            uint8_t p = read(PpuMemoryMap::PALETTE_BASE);
            _screen[size_t(_scanline) * 256 + _cycle] = p;
        }
    }
    else
    {
        if (_scanline < 240)
        {
            // Cycles 338 & 339 aren't really idle but they do their own thing
            bool idle = (_cycle < 1 ||
                        (_cycle > 256 && _cycle < 321) ||
                        _cycle > 338);

            // Process background
            if (_ppumask.b)
            {
                if (_cycle == 0)
                {
                    // Cycle zero is idle except on the first rendering scanline of odd frames it does the second tick of the last dummy nametable fetch.
                    if (_frame & 1)
                    {
                        // Fetch nametable byte
                        uint16_t tile_address = PpuMemoryMap::NAMETABLE_BASE | (_ppuaddr & 0x0FFF);
                        _nt_latch = read(tile_address);
                    }
                }
                else if (_cycle <= 256 || (_cycle > 320 && _cycle <= 337))
                {
                    if (_cycle > 1)
                    {
                        // Shift background shift registers
                        _bl_shift <<= 1;
                        _bh_shift <<= 1;
                        _al_shift <<= 1;
                        _ah_shift <<= 1;
                    }

                    uint8_t phase;
                    phase = ((_cycle - 1) & 7);

                    switch (phase)
                    {
                        case 0:
                        {
                            // Reload shift registers
                            _bl_shift = (_bl_shift & 0xFF00) | _bl_latch;
                            _bh_shift = (_bh_shift & 0xFF00) | _bh_latch;
                            _al_shift = (_al_shift & 0xFF00) | ((_attribute_latch & 1) ? 0xFF : 0);
                            _ah_shift = (_ah_shift & 0xFF00) | ((_attribute_latch & 2) ? 0xFF : 0);
                            break;
                        }
                        case 1:
                        {
                            // Fetch nametable byte
                            uint16_t tile_address = PpuMemoryMap::NAMETABLE_BASE | (_ppuaddr & 0x0FFF);
                            _nt_latch = read(tile_address);
                            break;
                        }
                        case 3:
                        {
                            // Fetch attribute byte
                            uint16_t coarse_y = (_ppuaddr & PpuAddrCoarseYMask) >> PpuAddrCoarseYShift;
                            uint16_t coarse_x = (_ppuaddr & PpuAddrCoarseXMask) >> PpuAddrCoarseXShift;
                            uint16_t attribute_address = 0x23C0 | (_ppuaddr & 0x0C00) | ((coarse_y >> 2) << 3) | (coarse_x >> 2);
                            _attribute_latch = read(attribute_address);
                            if (coarse_y & 0x02) _attribute_latch >>= 4;
                            if (coarse_x & 0x02) _attribute_latch >>= 2;
                            break;
                        }
                        case 5:
                        {
                            // Fetch low BG tile byte
                            uint16_t address;
                            address = (_ppuctrl.B << 0xC) | ((uint16_t)_nt_latch << 4) | (0 << 3) | (_ppuaddr & PpuAddrFineYMask) >> PpuAddrFineYShift;
                            _bl_latch = read(address);
                            break;
                        }
                        case 7:
                        {
                            // Fetch high BG tile byte
                            uint16_t address;
                            address = (_ppuctrl.B << 0xC) | ((uint16_t)_nt_latch << 4) | (1 << 3) | (_ppuaddr & PpuAddrFineYMask) >> PpuAddrFineYShift;
                            _bh_latch = read(address);
                            break;
                        }
                    }
                }
            } // background


            // Sprite evaluation
            if (_scanline > -1)
            {
                if (_cycle == 1)
                {
                    _state_flags |= StateFlags::OamReadMask;
                    _sprite_zero_visible >>= 1;
                    _active_sprites >>= 4;
                }
                else if (_cycle == 65)
                {
                    _state_flags &= ~StateFlags::OamReadMask;
                }

                if (_cycle > 0 && _cycle <= 64)
                {
                    // Clear secondary OAM - don't bother with the (masked) reads from primary in the odd cycles, just write 0xFF every even cycle
                    if ((_cycle & 1) == 0)
                    {
                        _secondary_oam[(_cycle - 1) >> 1] = 0xFF;
                    }
                }
                else if (_cycle <= 256)
                {
                    /*
                        Cycles 65-256: Sprite evaluation:

                        * On odd cycles, data is read from (primary) OAM

                        * On even cycles, data is written to secondary OAM (unless secondary OAM is full, in which case it will read the value in
                            secondary OAM instead)

                        1. Starting at n = 0, read a sprite's Y-coordinate (OAM[n][0], copying it to the next open slot in secondary OAM (unless 8
                            sprites have been found, in which case the write is ignored).
                                1a. If Y-coordinate is in range, copy remaining bytes of sprite data (OAM[n][1] thru OAM[n][3]) into secondary OAM.

                        2. Increment n
                            2a. If n has overflowed back to zero (all 64 sprites evaluated), go to 4.
                            2b. If less than 8 sprites have been found, go to 1.
                            2c. If exactly 8 sprites have been found, disable writes to secondary OAM because it is full. This causes sprites in back
                                to drop out.

                        3. Starting at m = 0, evaluate OAM[n][m] as a Y-coordinate.
                            3a. If the value is in range, set the sprite overflow flag in $2002 and read the next 3 entries of OAM (incrementing 'm'
                                after each byte and incrementing 'n' when 'm' overflows); if m = 3, increment n.
                            3b. If the value is not in range, increment n and m (without carry). If n overflows to 0, go to 4; otherwise go to 3.
                                    The m increment is a hardware bug - if only n was incremented, the overflow flag would be set whenever more than
                                    8 sprites were present on the same scanline, as expected.

                        4. Attempt (and fail) to copy OAM[n][0] into the next free slot in secondary OAM, and increment n (repeat until HBLANK is
                            reached).
                    */
                    // TODO: Cycle emulation - for now just execute the whole thing on cycle 256
                    if (_cycle == 256)
                    {
                        uint8_t sprite_size = _ppuctrl.H ? 16 : 8;
                        uint8_t active_sprites = 0;

                        for (uint8_t sprite = 0; sprite < 64; ++sprite)
                        {
                            uint8_t oam_read_ptr = (_oamaddr + (sprite << 2)) & 0xFF;
                            uint8_t sprite_y = _oam[oam_read_ptr];
                            uint8_t oam_write_ptr = active_sprites << 2;
                            int16_t diff = (int16_t)_scanline - (int16_t)sprite_y;

                            if (diff >= 0 && diff < sprite_size)
                            {
                                _sprite_zero_visible |= (sprite == 0) ? 2 : 0;

                                if (active_sprites < 8)
                                {
                                    for (uint8_t i = 0; i < 4; ++i)
                                        _secondary_oam[oam_write_ptr++] = _oam[oam_read_ptr++];

                                    active_sprites++;
                                }
                                else
                                {
                                    _ppustatus.O = 1;
                                    break;
                                }
                            }
                        }

                        _active_sprites = (active_sprites << 4) | (_active_sprites & 0x0F);
                    }
                } // scanline > -1

                if (_cycle > 256 && _cycle <= 320)
                {
                    /*
                        Cycles 257-320: Sprite fetches (8 sprites total, 8 cycles per sprite)
                            1-4: Read the Y-coordinate, tile number, attributes, and X-coordinate of the selected sprite from secondary OAM
                            5-8: Read the X-coordinate of the selected sprite from secondary OAM 4 times (while the PPU fetches the sprite tile data)
                    */
                    // TODO: Cycle emulation - for now do it all on cycle 261 when the first sprite tile fetch begins
                    if (_cycle == 261 && _ppumask.s)
                    {
                        memset(_sprite_output_units.data(), 0, sizeof(SpriteOutputUnit) * 8);
                        uint8_t sprite_size = _ppuctrl.H ? 16 : 8;

                        for (uint8_t sprite = 0; sprite < 8; ++sprite)
                        {
                            uint8_t oam_read_ptr = sprite << 2;
                            uint8_t sprite_y = _scanline - _secondary_oam[oam_read_ptr + 0];
                            uint8_t tile_index = _secondary_oam[oam_read_ptr + 1];
                            _sprite_output_units[sprite].attributes = _secondary_oam[oam_read_ptr + 2];
                            _sprite_output_units[sprite].x_position = _secondary_oam[oam_read_ptr + 3];

                            uint8_t pattern_table;
                            uint16_t lsb_address;

                            if (_sprite_output_units[sprite].attributes & 0x80)
                            {
                                sprite_y = sprite_size - sprite_y - 1;
                            }

                            if (_ppuctrl.H == 0)
                            {
                                pattern_table = _ppuctrl.S;
                            }
                            else
                            {
                                pattern_table = tile_index & 1;
                                tile_index = (tile_index & 0xFE) | (sprite_y >> 3);
                            }

                            lsb_address = (pattern_table << 0xC) | (tile_index << 4) | (sprite_y & 0x7);
                            uint8_t pattern_mask = (sprite < (_active_sprites >> 4)) ? 0xFF : 0x00;
                            _sprite_output_units[sprite].pattern_lo = read(lsb_address) & pattern_mask;
                            _sprite_output_units[sprite].pattern_hi = read(lsb_address + 8) & pattern_mask;

                            if (_sprite_output_units[sprite].attributes & (1 << 6))
                            {
                                _sprite_output_units[sprite].pattern_lo = reverse(_sprite_output_units[sprite].pattern_lo);
                                _sprite_output_units[sprite].pattern_hi = reverse(_sprite_output_units[sprite].pattern_hi);
                            }
                        }
                    }

                    /* OAMADDR is set to 0 during each of ticks 257 - 320 (the sprite tile loading interval) of the pre - render and visible scanlines. */
                    _oamaddr = 0;
                }
            }

            // Update sprite output units
            if (_cycle > 1 && _cycle <= 256)
            {
                for (uint8_t sprite = 0; sprite < (_active_sprites & 0xF); ++sprite)
                {
                    if (_sprite_output_units[sprite].x_position > 0)
                    {
                        _sprite_output_units[sprite].x_position--;
                    }
                    else
                    {
                        _sprite_output_units[sprite].pattern_lo <<= 1;
                        _sprite_output_units[sprite].pattern_hi <<= 1;
                    }
                }
            }

            if (!idle && ((_cycle - 1) & 0x7) == 7)
            {
                if (_ppumask.b || _ppumask.s)
                {
                    // Inc hori(v)
                    uint16_t coarse_x = (_ppuaddr & PpuAddrCoarseXMask) >> PpuAddrCoarseXShift;

                    if (coarse_x < 0x1F)
                    {
                        _ppuaddr++;
                    }
                    else
                    {
                        _ppuaddr = _ppuaddr & ~PpuAddrCoarseXMask;
                        _ppuaddr ^= PpuAddrNametableXMask;
                    }
                }
            }

            if (_cycle == 256)
            {
                if (_ppumask.b || _ppumask.s)
                {
                    // Inc vert(v)
                    uint16_t fine_y = (_ppuaddr & PpuAddrFineYMask) >> PpuAddrFineYShift;

                    if (fine_y < 0x7)
                    {
                        _ppuaddr += 1 << PpuAddrFineYShift;
                    }
                    else
                    {
                        _ppuaddr &= ~PpuAddrFineYMask;

                        uint16_t coarse_y = (_ppuaddr & PpuAddrCoarseYMask) >> PpuAddrCoarseYShift;

                        if (coarse_y == 29)
                        {
                            /*
                                Row 29 is the last row of tiles in a nametable. To wrap to the next nametable when incrementing coarse Y from 29, the
                                vertical nametable is switched by toggling bit 11, and coarse Y wraps to row 0.
                            */

                            _ppuaddr = _ppuaddr & ~PpuAddrCoarseYMask;
                            _ppuaddr ^= PpuAddrNametableYMask;
                        }
                        else if (coarse_y == 31)
                        {
                            /*
                                Coarse Y can be set out of bounds (> 29), which will cause the PPU to read the attribute data stored there as tile
                                data. If coarse Y is incremented from 31, it will wrap to 0, but the nametable will not switch.
                            */
                            _ppuaddr = _ppuaddr & ~PpuAddrCoarseYMask;
                        }
                        else
                        {
                            _ppuaddr += 1 << PpuAddrCoarseYShift;
                        }
                    }
                }
            }

            if (_cycle == 257)
            {
                if (_ppumask.b || _ppumask.s)
                {
                    // hori(v) = hori(t)
                    _ppuaddr = (_ppuaddr & ~0x41F) | (_temp_vram_address & 0x41F);
                }
            }

            if (_cycle >= 280 && _cycle < 305)
            {
                if (_scanline == -1)
                {
                    if (_ppumask.b || _ppumask.s)
                    {
                        // vert(v) = vert(t)
                        _ppuaddr = (_ppuaddr & ~0x7BE0) | (_temp_vram_address & 0x7BE0);
                    }
                }
            }

            if (_cycle == 338 || _cycle == 340)
            {
                uint16_t tile_address = PpuMemoryMap::NAMETABLE_BASE | (_ppuaddr & 0x0FFF);
                _nt_latch = read(tile_address);
            }

            if (_scanline >= 0 && _cycle > 0 && _cycle <= 256)
            {
                uint8_t bg_pixel = 0;
                uint8_t bg_palette = 0;
                uint8_t fg_pixel = 0;
                uint8_t fg_palette = 0;
                uint8_t fg_priority = 0;

                if (_ppumask.b && _cycle > (_ppumask.m ? 0 : 8))
                {
                    // Produce background pixel
                    uint8_t bit_select = 0xF - _fine_x_scroll;
                    uint8_t al = (_al_shift >> bit_select) & 1;
                    uint8_t ah = (_ah_shift >> bit_select) & 1;
                    uint8_t bl = (_bl_shift >> bit_select) & 1;
                    uint8_t bh = (_bh_shift >> bit_select) & 1;
                    bg_pixel = (bh << 1) | bl;
                    bg_palette = (ah << 1) | al;
                }

                if (_scanline > 0)
                {
                    bool sprite_zero_drawn = false;

                    if (_ppumask.s && _cycle > (_ppumask.M ? 0 : 8))
                    {
                        // Produce foreground (sprite) pixel
                        for (uint8_t sprite = 0; sprite < (_active_sprites & 0xF); ++sprite)
                        {
                            if (_sprite_output_units[sprite].x_position == 0)
                            {
                                uint8_t pl = (_sprite_output_units[sprite].pattern_lo >> 7) & 1;
                                uint8_t ph = (_sprite_output_units[sprite].pattern_hi >> 7) & 1;
                                fg_pixel = (ph << 1 | pl);
                                fg_palette = (_sprite_output_units[sprite].attributes & 0x3) + 4;
                                fg_priority = (_sprite_output_units[sprite].attributes & (1 << 5)) == 0;

                                if (fg_pixel)
                                {
                                    sprite_zero_drawn = (sprite == 0);
                                    break;
                                }
                            }
                        }
                    }

                    // Set sprite zero hit flag
                    if (bg_pixel && sprite_zero_drawn)
                        _ppustatus.S |= _sprite_zero_visible & 1;
                }

                // Mux fg & bg pixels
                uint8_t pixel = 0x00;
                uint8_t palette = 0x00;

                if (bg_pixel && fg_pixel)
                {
                    pixel = fg_priority ? fg_pixel : bg_pixel;
                    palette = fg_priority ? fg_palette : bg_palette;
                }
                else if (bg_pixel)
                {
                    pixel = bg_pixel;
                    palette = bg_palette;
                }
                else if (fg_pixel)
                {
                    pixel = fg_pixel;
                    palette = fg_palette;
                }

                _screen[_scanline * 256 + (_cycle - 1)] = read(PpuMemoryMap::PALETTE_BASE | (palette << 2) | pixel);
            }
        } // if _scanline < 240
    }

    if (_scanline == 241 && _cycle == 1)
    {
        _ppustatus.V = 1;

        if (_ppuctrl.V)
            _nmi = 1;
    }

    if (_scanline == -1 && _cycle == 1)
    {
        _ppustatus.reg = 0;
        _state_flags &= ~StateFlags::Reset;
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
        _sprite_zero_visible = 0;
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
            if ((_state_flags & StateFlags::Reset) == 0)
            {
                if ((value & 0x80) && !(_ppuctrl.V) && _ppustatus.V)
                {
                    _nmi = true;
                }

                _ppuctrl.reg = value;
                _temp_vram_address &= ~(PpuAddrNametableYMask | PpuAddrNametableXMask);
                _temp_vram_address |= ((uint16_t)_ppuctrl.N << PpuAddrNametableXShift) & (PpuAddrNametableYMask | PpuAddrNametableXMask);
            }

            break;
        }
        case PpuRegisters::PPUMASK:
        {
            if ((_state_flags & StateFlags::Reset) == 0)
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

            if (_scanline > 239 || (_ppumask.b == 0 && _ppumask.s == 0))
                _oam[_oamaddr++] = value;

            break;
        }
        case PpuRegisters::PPUSCROLL:
        {
            if ((_state_flags & StateFlags::Reset) == 0)
            {
                if (_address_latch == 0)
                {
                    /*
                        $2005 first write (w is 0)

                        t: ....... ...HGFED = d: HGFED...
                        x:              CBA = d: .....CBA
                        w:                  = 1
                    */
                    _temp_vram_address &= ~PpuAddrCoarseXMask;
                    _temp_vram_address |= ((value >> 3) & PpuAddrCoarseXMask);
                    _fine_x_scroll = value & 0x7;
                    _address_latch = 1;
                }
                else
                {
                    /*
                        $2005 second write (w is 1)

                        t: CBA..HG FED..... = d: HGFEDCBA
                        w:                  = 0
                    */
                    _temp_vram_address &= ~(PpuAddrFineYMask | PpuAddrCoarseYMask);
                    _temp_vram_address |= ((uint16_t)value << PpuAddrFineYShift) & PpuAddrFineYMask;
                    _temp_vram_address |= ((uint16_t)(value >> 3) << PpuAddrCoarseYShift) & PpuAddrCoarseYMask;
                    _address_latch = 0;
                }
            }

            break;
        }
        case PpuRegisters::PPUADDR:
        {
            if ((_state_flags & StateFlags::Reset) == 0)
            {
                if (_address_latch == 0)
                {
                    /*
                        $2006 first write (w is 0)

                        t: .FEDCBA ........ = d: ..FEDCBA
                        t: X...... ........ = 0
                        w:                  = 1
                    */
                    _temp_vram_address &= 0x00FF;
                    _temp_vram_address |= ((uint16_t)value & 0x3F) << 8;
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
                    _temp_vram_address &= 0xFF00;
                    _temp_vram_address |= value;
                    _ppuaddr = _temp_vram_address;
                    _address_latch = 0;

                    // Seems that writing PPUADDR is supposed to be visible to the mapper (MMC3), presumably it sets the PPU address bus. Do a dummy
                    // read from _ppuaddr here so MMC3 IRQ clocking works...
                    if (_ppuaddr < 0x2000)
                    {
                        read(_ppuaddr);
                    }
                }
            }

            break;
        }
        case PpuRegisters::PPUDATA:
        {
            // VRAM read/write data register. After access, the video memory address will increment by an amount determined by bit 2 of $2000.
            write(_ppuaddr, value);
            _ppuaddr += (_ppuctrl.I ? 32 : 1);
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
            if (_state_flags & StateFlags::OamReadMask)
                value = 0xFF;
            else
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

            value = _ppudatabuffer;
            _ppudatabuffer = read(_ppuaddr);

            if (_ppuaddr >= PpuMemoryMap::PALETTE_BASE)
                value = _ppudatabuffer;

            _ppuaddr += (_ppuctrl.I ? 32 : 1);
            break;
        }
    }

    return value;
}


void gli2C02::get_pattern_table(uint8_t table_index, uint8_t palette_index, std::array<uint8_t, 0x4000>& pattern_table)
{
    for (uint16_t t = 0; t < 256; ++t)
    {
        uint8_t tx = (t & 0xF) << 3;
        uint8_t ty = (t >> 4) << 3;

        for (uint8_t y = 0; y < 8; ++y)
        {
            uint16_t addr = static_cast<uint16_t>(table_index) << 12 | t << 4 | y;
            uint8_t tile_lsb = read(addr);
            uint8_t tile_msb = read(addr + 8);

            for (uint8_t x = 0; x < 8; ++x)
            {
                uint8_t bit = 7 - x;
                uint8_t lsb = get_bit(tile_lsb, bit);
                uint8_t msb = get_bit(tile_msb, bit);
                uint8_t pixel = (msb << 1) | lsb;
                uint16_t offset = ((ty + y) << 7) + (tx + x);
                pattern_table[offset] = read(pixel ? (PALETTE_BASE | (palette_index << 2) | pixel) : PALETTE_BASE);
            }
        }
    }
}


void gli2C02::get_palette(uint8_t palette_index, std::array<uint8_t, 4>& palette)
{
    for (uint8_t i = 0; i < 4; ++i)
    {
        palette[i] = read(PpuMemoryMap::PALETTE_BASE | (palette_index << 2) | i);
    }
}


uint8_t gli2C02::read(uint16_t address)
{
    address &= 0x3FFF;
    uint8_t value = 0;

    if (address <= PpuMemoryMap::PATTERN_TABLE_TOP)
    {
        if (_game_pak)
            _game_pak->ppu_read(address, value);
    }
    else if (address <= NAMETABLE_MIRROR_TOP)
    {
        address &= PpuMemoryMap::NAMETABLE_TOP;

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
        address &= PpuMemoryMap::PALETTE_MASK;
        if (address == 0x0010) address = 0x0000;
        if (address == 0x0014) address = 0x0004;
        if (address == 0x0018) address = 0x0008;
        if (address == 0x001C) address = 0x000C;
        value = _palette[address];
    }

    return value;
}


void gli2C02::write(uint16_t address, uint8_t value)
{
    if (address <= PpuMemoryMap::PATTERN_TABLE_TOP)
    {
        if (_game_pak)
            _game_pak->ppu_write(address, value);
    }
    else if (address <= NAMETABLE_MIRROR_TOP)
    {
        address &= PpuMemoryMap::NAMETABLE_TOP;

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
        address &= PpuMemoryMap::PALETTE_MASK;
        if (address == 0x0010) address = 0x0000;
        if (address == 0x0014) address = 0x0004;
        if (address == 0x0018) address = 0x0008;
        if (address == 0x001C) address = 0x000C;
        _palette[address] = value;
    }
}
