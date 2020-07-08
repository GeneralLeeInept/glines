#include "vgfw.h"

#include <algorithm>
#include <array>
#include <functional>
#include <vector>

#include "bits.h"
#include "gamepak.h"
#include "gli2a03.h"
#include "gli2c02.h"
#include "ntsc_palette.h"
#include "vga9.h"

class GliNes : public Vgfw
{
public:
    static constexpr int DisplayWidth = 256;
    static constexpr int DisplayHeight = 240;
    static constexpr int DisplayScale = 4;

    static constexpr int InspectorWidth = 80 * 9;
    static constexpr int InspectorHeight = 50 * 16;

    static constexpr int WindowWidth = 16 + (DisplayWidth * DisplayScale) + 16 + InspectorWidth + 16;
    static constexpr int WindowHeight = 16 + std::max(DisplayHeight * DisplayScale, InspectorHeight) + 16;


    bool on_create() override
    {
        set_palette(ntsc_palette, sizeof(ntsc_palette));
        _cpu.connect(std::bind(&GliNes::read, this, std::placeholders::_1), std::bind(&GliNes::write, this, std::placeholders::_1, std::placeholders::_2));
        reset(true);
        return true;
    }


    void on_destroy() override
    {

    }


    gli2A03 _cpu;
    gli2C02 _ppu;
    std::shared_ptr<GamePak> _game_pak;
    uint8_t _ram[2 * 1024];


    // Bus
    enum CpuMemoryMap
    {
        RAM_BASE = 0x0000,
        RAM_TOP = 0x1FFF,
        PPU_REG_BASE = 0x2000,
        PPU_REG_TOP = 0x3FFF,
        APU_IO_BASE = 0x4000,
        OAMDMA = 0x4014,
        JOY1 = 0x4016,
        JOY2 = 0x4017,
        APU_IO_TOP = 0x401F,
        CART_BASE = 0x4020,
    };

    struct ControllerState
    {
        uint8_t latch;

        union
        {
            uint8_t buttons;

            struct
            {
                uint8_t right : 1;
                uint8_t left : 1;
                uint8_t down : 1;
                uint8_t up : 1;
                uint8_t start : 1;
                uint8_t select : 1;
                uint8_t b : 1;
                uint8_t a : 1;
            };
        };
    };

    ControllerState joy1{};
    ControllerState joy2{};

    uint8_t read(uint16_t address)
    {
        uint8_t value = 0; // TODO: open bus behavior (make this static)

        if (address <= CpuMemoryMap::RAM_TOP)
        {
            address = CpuMemoryMap::RAM_BASE + (address & 0x7FF);
            value = _ram[address];
        }
        else if (address <= CpuMemoryMap::PPU_REG_TOP)
        {
            value = _ppu.cpu_read(address);
        }
        else if (address <= CpuMemoryMap::APU_IO_TOP)
        {
            if (address == JOY1)
            {
                set_bit(value, 0, get_bit(joy1.latch, 7));
                joy1.latch <<= 1;
            }
            else if (address == JOY2)
            {
                set_bit(value, 0, get_bit(joy2.latch, 7));
                joy2.latch <<= 1;
            }
        }
        else if (_game_pak)
        {
            value = _game_pak->cpu_read(address);
        }

        return value;
    }


    void write(uint16_t address, uint8_t value)
    {
        if (address <= CpuMemoryMap::RAM_TOP)
        {
            address = CpuMemoryMap::RAM_BASE + (address & 0x7FF);
            _ram[address] = value;
        }
        else if (address <= CpuMemoryMap::PPU_REG_TOP)
        {
            _ppu.cpu_write(address, value);
        }
        else if (address <= CpuMemoryMap::APU_IO_TOP)
        {
            if (address == OAMDMA)
            {
                _cpu.dma(value);
            }
            else if (address == JOY1)
            {
                if ((value & 1)== 0)
                {
                    // latch controller values
                    joy1.latch = joy1.buttons;
                    joy2.latch = joy2.buttons;
                }
            }
        }
        else if (_game_pak)
        {
            _game_pak->cpu_write(address, value);
        }
    }


    // System
    uint32_t _system_clock = 0;


    void reset(bool coldstart)
    {
        _cpu.reset(coldstart);
        _ppu.reset(coldstart);
        _system_clock = 0;
        run_emulation = false;
        joy1.latch = 0;
        joy2.latch = 0;
    }


    void clock()
    {
        _system_clock++;

        if ((_system_clock % 3) == 0)
        {
            _cpu.clock();
        }

        _ppu.clock();

        if (_ppu._nmi)
        {
            _cpu.nmi();
            _ppu._nmi = 0;
        }
    }


    // Cart
    bool load_game_pak(const std::string& path)
    {
        _game_pak = std::make_shared<GamePak>();

        if (!_game_pak->load(path))
        {
            _game_pak = nullptr;
        }

        _ppu.connect_game_pak(_game_pak);

        return !!_game_pak;
    }


    uint16_t mem_offs = 0x0;
    bool run_emulation = false;
    uint8_t palette = 0;
    float accumulated_time = 0.0f;

    bool on_update(float delta) override
    {
        if ((m_keys[VK_LCONTROL].down || m_keys[VK_RCONTROL].down) && m_keys['Q'].pressed)
        {
            return false;
        }

        if (m_keys[VK_NEXT].pressed)
        {
            mem_offs += 32 * 16;
        }
        else if (m_keys[VK_PRIOR].pressed)
        {
            mem_offs -= 32 * 16;
        }

        if (m_keys['P'].pressed)
        {
            palette = (palette + 1) & 0x7;
        }

        if (m_keys[VK_OEM_3].pressed)
        {
            reset(m_keys[VK_LCONTROL].down);
        }
        else
        {
            if (m_keys[VK_F5].pressed)
            {
                run_emulation = !run_emulation;
            }

            if (run_emulation)
            {
                joy1.right = m_keys[VK_RIGHT].down;
                joy1.left = m_keys[VK_LEFT].down;
                joy1.up = m_keys[VK_UP].down;
                joy1.down = m_keys[VK_DOWN].down;
                joy1.start = m_keys['V'].down;
                joy1.select = m_keys['B'].down;
                joy1.a = m_keys['Z'].down;
                joy1.b = m_keys['X'].down;

                accumulated_time += delta;

                if (accumulated_time > 1.0f / 60.0f)
                {
                    accumulated_time -= 1.0f / 60.0f;

                    uint32_t f = _ppu.frame_number();

                    do
                    {
                        clock();
                    } while (_ppu.frame_number() == f);
                }
            }
            else if (m_keys[VK_F11].pressed)
            {
                uint16_t pc = _cpu._pc;

                do
                {
                    clock();
                } while (!_cpu._stopped && (_cpu._pc == pc));
            }
            else if (m_keys[VK_F10].pressed)
            {
                uint32_t f = _ppu.frame_number();

                do
                {
                    clock();
                } while (_ppu.frame_number() == f);
            }
        }

        clear_screen(0);

        // TV display
        copy_rect_scaled(16, 16, DisplayWidth * DisplayScale, DisplayHeight * DisplayScale, _ppu._screen.data(), 256, DisplayScale);

        // Dump RAM
        int ram_dump_x = 16 + (DisplayWidth * DisplayScale) + 16;
        int ram_dump_y = 16;
        int ram_dump_w = InspectorWidth;
        int ram_dump_h = 8 + 32 * vga9_glyph_height + 8;

        fill_rect(ram_dump_x, ram_dump_y, ram_dump_w, ram_dump_h, 2, 2, 0x20);

        int mem_y = ram_dump_y + 8;
        int mem_x = ram_dump_x + 8;
        uint16_t mem_ptr = mem_offs;

        for (int i = 0; i < 32; ++i)
        {
            uint8_t memp[16];
            char memc[16];

            for (int p = 0; p < 16; ++p)
            {
                // Don't read registers like this...
                if ((mem_ptr + p) >= 0x2000 && (mem_ptr + p) <= 0x3FFF)
                    memp[p] = 0;
                else
                    memp[p] = read(mem_ptr + p);

                memc[p] = (memp[p] >= 0x20 && memp[p] <= 0X7F) ? (char)memp[p] : '.';
            }

            format_string(mem_x, mem_y, (const int*)vga9_glyphs, vga9_glyph_width, vga9_glyph_height, 0x20, 2,
                "%04X   %02X %02X %02X %02X %02X %02X %02X %02X  %02X %02X %02X %02X %02X %02X %02X %02X   %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", mem_ptr,
                memp[0], memp[1], memp[2], memp[3], memp[4], memp[5], memp[6], memp[7], memp[8], memp[9], memp[10], memp[11], memp[12], memp[13], memp[14], memp[15],
                memc[0], memc[1], memc[2], memc[3], memc[4], memc[5], memc[6], memc[7], memc[8], memc[9], memc[10], memc[11], memc[12], memc[13], memc[14], memc[15]);

            mem_ptr += 16;
            mem_y += vga9_glyph_height;
        }

        // CPU State
        int cpu_state_x = ram_dump_x;
        int cpu_state_y = ram_dump_y + ram_dump_h + 16;
        int cpu_state_w = InspectorWidth;
        int cpu_state_h = 8 + 4 * vga9_glyph_height + 8;

        fill_rect(cpu_state_x, cpu_state_y, cpu_state_w, cpu_state_h, 2, 2, 0x20);

        int cpu_x = cpu_state_x + 8;
        int cpu_y = cpu_state_y + 8;

        std::string dissassembly = _cpu.disassemble(_cpu._pc);
        format_string(cpu_x, cpu_y, (const int*)vga9_glyphs, vga9_glyph_width, vga9_glyph_height, 0x20, 2, "    PC: %04X  %s", _cpu._pc, dissassembly.c_str());
        cpu_y += vga9_glyph_height;

        format_string(cpu_x, cpu_y, (const int*)vga9_glyphs, vga9_glyph_width, vga9_glyph_height, 0x20, 2, "     A: %02X  X: %02X  Y: %02X  SP: %02X", _cpu._a, _cpu._x, _cpu._y, _cpu._s);
        cpu_y += vga9_glyph_height;

        auto bit = [](uint8_t byte, int bit) -> char { return ((byte >> bit) & 1) ? '1' : '-'; };
        format_string(cpu_x, cpu_y, (const int*)vga9_glyphs, vga9_glyph_width, vga9_glyph_height, 0x20, 2, "        N V   B D I Z C");
        cpu_y += vga9_glyph_height;
        format_string(cpu_x, cpu_y, (const int*)vga9_glyphs, vga9_glyph_width, vga9_glyph_height, 0x20, 2, "Status: %c %c   %c %c %c %c %c %s",
            bit(_cpu._p, 7), bit(_cpu._p, 6), bit(_cpu._p, 4), bit(_cpu._p, 3), bit(_cpu._p, 2), bit(_cpu._p, 1), bit(_cpu._p, 0), _cpu._stopped ? "** STOPPED **" : "");
        cpu_y += vga9_glyph_height;

        // Pattern table 1
        int pattern_table_x = ram_dump_x;
        int pattern_table_y = cpu_y + 16;

        std::array<uint8_t, 0x4000> pattern_table;
        _ppu.get_pattern_table(0, palette, pattern_table);
        copy_rect_scaled(pattern_table_x, pattern_table_y, 256, 256, pattern_table.data(), 128, 2);

        // Pattern table 2
        _ppu.get_pattern_table(1, palette, pattern_table);
        copy_rect_scaled(pattern_table_x + 256 + 16, pattern_table_y, 256, 256, pattern_table.data(), 128, 2);

        // Palettes
        int palette_x = ram_dump_x;
        int palette_y = pattern_table_y + 256 + 16;

        for (uint8_t column = 0; column < 4; ++column)
        {
            int palette_x = ram_dump_x + (144 * column);

            for (uint8_t row = 0; row < 2; ++row)
            {
                int palette_y = pattern_table_y + 256 + 16 + row * 32;
                std::array<uint8_t, 4> palette;
                _ppu.get_palette(column + row * 4, palette);

                for (uint8_t c = 0; c < 4; ++c)
                {
                    fill_rect(palette_x + (c * 20), palette_y, 16, 16, 1, palette[c], palette[c]);
                }
            }
        }

        return true;
    }
};


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
    GliNes nes;

    if (!nes.initialize(L"GLI NES", GliNes::WindowWidth, GliNes::WindowHeight, 1))
    {
        return 1;
    }

    nes.load_game_pak(R"(D:\EMU\nes\nestest.nes)");
    //nes.load_game_pak(R"(D:\EMU\nes\instr_test-v5\official_only.nes)");
    //nes.load_game_pak(R"(D:\EMU\nes\instr_test-v5\all_instrs.nes)");
    //nes.load_game_pak(R"(D:\EMU\nes\branch_timing_tests\1.Branch_Basics.nes)");
    //nes.load_game_pak(R"(D:\EMU\nes\branch_timing_tests\2.Backward_Branch.nes)");
    //nes.load_game_pak(R"(D:\EMU\nes\branch_timing_tests\3.Forward_Branch.nes)");
    //nes.load_game_pak(R"(D:\EMU\nes\instr_timing\instr_timing.nes)");
    //nes.load_game_pak(R"(D:\EMU\nes\ppu_vbl_nmi\ppu_vbl_nmi.nes)");
    nes.run();

    return 0;
}
