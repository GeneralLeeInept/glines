#include "mapper_004.h"

#include "bits.h"
#include "gli2a03.h"
#include "gli2c02.h"


Mapper_004::Mapper_004(GamePak& game_pak)
    : Mapper(game_pak)
{
    reset(true);
}


void Mapper_004::reset(bool coldstart)
{
    _last_ppu_clock_count = 0;
    _last_ppu_address = 0xFFFF;
    _bank_select = 0;
    _irq_latch = 0;
    _irq_reload = 0;
    _irq_counter = 0;
    _irq_enabled = 0;
    _mirroring = 0;
}


uint8_t Mapper_004::cpu_read(uint16_t address)
{
    uint8_t value = 0;

    if (address >= 0x6000 && address < 0x8000)
    {
        value = _prg_ram[address & 0x1FFF];
    }
    else if (address >= 0x8000)
    {
        uint8_t mode = get_bit(_bank_select, 6);
        uint8_t bank = 0;

        /*
            PRG map mode        $8000.D6 = 0    $8000.D6 = 1
            CPU Bank            Value of MMC3 register
            $8000-$9FFF         R6              (-2)
            $A000-$BFFF         R7              R7
            $C000-$DFFF         (-2)            R6
            $E000-$FFFF         (-1)            (-1)
        */

        if (mode == 0)
        {
            if  (address < 0xA000)
                bank = (_bank_select_registers[6] & 0x3F);
            else if (address < 0xC000)
                bank = (_bank_select_registers[7] & 0x3F);
            else if (address < 0xE000)
                bank = (header()[4] << 1) - 2; // TODO: Sort out header
            else
                bank = (header()[4] << 1) - 1;
        }
        else
        {
            if  (address < 0xA000)
                bank = (header()[4] << 1) - 2;
            else if (address < 0xC000)
                bank = (_bank_select_registers[7] & 0x3F);
            else if (address < 0xE000)
                bank = (_bank_select_registers[6] & 0x3F);
            else
                bank = (header()[4] << 1) - 1;
        }

        size_t rom_address = ((size_t)bank * 0x2000) + (address & 0x1FFF);
        value = prg_rom()[rom_address];
    }

    return value;
}


void Mapper_004::cpu_write(uint16_t address, uint8_t value)
{
    if (address >= 0x6000 && address < 0x8000)
    {
        _prg_ram[address & 0x1FFF] = value;
    }
    else if (address >= 0x8000 && address < 0xA000)
    {
        if ((address & 1) == 0)
        {
            // Bank select
            _bank_select = value;
        }
        else
        {
            // Bank data
            _bank_select_registers[_bank_select & 0x7] = value;
        }
    }
    else if (address >= 0xA000 && address < 0xC000)
    {
        if ((address & 1) == 0)
        {
            // Mirroring
            _mirroring = get_bit(value, 0);
        }
        else
        {
            // PRG RAM protect - #NOTIMPLEMENTED
        }
    }
    else if (address >= 0xC000 && address < 0xE000)
    {
        if ((address & 1) == 0)
        {
            // IRQ latch
            _irq_latch = value;
        }
        else
        {
            // IRQ reload
            _irq_reload = 1;
        }
    }
    else if (address >= 0xE000)
    {
        _irq_enabled = get_bit(address, 0);
    }
}


bool Mapper_004::ppu_read(uint16_t address, uint8_t& value)
{
    // Test for rising edge in A12 when reading pattern memory (filtered to ignore HF oscillations)
    if (address < 0x2000)
    {
        if ((ppu()->clock_count() - _last_ppu_clock_count) > 3)
        {
            bool was_low = (_last_ppu_address & 0x1000) == 0;
            bool is_high = address & 0x1000;

            _last_ppu_clock_count = ppu()->clock_count();
            _last_ppu_address = address;

            if (was_low && is_high)
                clock_irq();
        }
    }

    /*
        When $8000 & $80    is $00      is $80
        PPU Bank            Value of MMC3 register
        $0000-$03FF         R0          R2
        $0400-$07FF         R0          R3
        $0800-$0BFF         R1          R4
        $0C00-$0FFF         R1          R5
        $1000-$13FF         R2          R0
        $1400-$17FF         R3          R0
        $1800-$1BFF         R4          R1
        $1C00-$1FFF         R5          R1
    */

    if (address < 0x2000)
    {
        uint8_t mode = get_bit(_bank_select, 7);
        uint8_t register_index = 0;

        if (address >= 0x0000 && address < 0x0400)
            register_index = mode ? 2 : 0;
        else if (address >= 0x0400 && address < 0x0800)
            register_index = mode ? 3 : 0;
        else if (address >= 0x0800 && address < 0x0C00)
            register_index = mode ? 4 : 1;
        else if (address >= 0x0C00 && address < 0x1000)
            register_index = mode ? 5 : 1;
        else if (address >= 0x1000 && address < 0x1400)
            register_index = mode ? 0 : 2;
        else if (address >= 0x1400 && address < 0x1800)
            register_index = mode ? 0 : 3;
        else if (address >= 0x1800 && address < 0x1C00)
            register_index = mode ? 1 : 4;
        else if (address >= 0x1C00 && address < 0x2000)
            register_index = mode ? 1 : 5;

        uint16_t bank_size = (register_index < 2) ? 0x800 : 0x400;
        uint8_t bank = _bank_select_registers[register_index];
        bank = (register_index < 2) ? (bank & 0xFE) : bank;
        size_t rom_address = ((size_t)bank * 0x400) + (address & (bank_size - 1));
        value = chr_rom()[rom_address];

        return true;
    }

    return false;
}


bool Mapper_004::ppu_write(uint16_t address, uint8_t value)
{
    return false;
}


bool Mapper_004::ppu_remap_address(uint16_t& address)
{
    if (address >= 0x2000 && address < 0x3000)
    {
        // Mirroring
        if ((header()[6] & 4) == 0)
        {
            if (_mirroring == 0)
            {
                // Vertical
                set_bit(address, 11, 0);
            }
            else
            {
                // Horizontal
                set_bit(address, 10, get_bit(address, 11));
                set_bit(address, 11, 0);
            }

            return true;
        }
    }

    return false;
}


void Mapper_004::clock_irq()
{
    if (_irq_counter == 0 || _irq_reload)
    {
        _irq_counter = _irq_latch;
        _irq_reload = 0;
    }
    else
        _irq_counter--;

    if (_irq_counter == 0 && _irq_enabled)
        cpu()->irq();
}
