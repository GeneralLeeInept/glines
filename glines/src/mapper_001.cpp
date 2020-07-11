#include "mapper_001.h"

#include "bits.h"


Mapper_001::Mapper_001(GamePak& game_pak)
    : Mapper(game_pak)
{
    _prg_bank = 0;
    _chr_bank_0 = 0;
    reset(true);
}


void Mapper_001::reset(bool coldstart)
{
    _load = 0x10;
    _control = _control | 0x0C;

    update_prg_rom_mapping();
    update_chr_rom_mapping();
}


uint8_t Mapper_001::cpu_read(uint16_t address)
{
    uint8_t value = 0;

    if (address >= 0x6000 && address < 0x8000)
    {
        uint16_t offset = address & 0x1FFF;
        value = _prg_ram[offset];
    }
    else if (address >= 0x8000 && address < 0xC000)
    {
        uint16_t offset = address & 0x3FFF;
        value = prg_rom()[_x8000 + offset];
    }
    else if (address >= 0xC000)
    {
        uint16_t offset = address & 0x3FFF;
        value = prg_rom()[_xC000 + offset];
    }

    return value;
}


void Mapper_001::cpu_write(uint16_t address, uint8_t value)
{
    if (address >= 0x6000 && address < 0x8000)
    {
        uint16_t offset = address & 0x1FFF;
        _prg_ram[offset] = value;
    }
    else if (address >= 0x8000)
    {
        if (value & 0x80)
        {
            reset(false);
        }
        else
        {
            uint8_t execute = get_bit(_load, 0);
            _load = (_load >> 1) | (get_bit(value, 0) << 4);

            if (execute)
            {
                uint8_t reg = (address >> 13) & 0x3;

                if (reg == 0)
                {
                    _control = _load;
                    update_prg_rom_mapping();
                    update_chr_rom_mapping();
                }
                else if (reg == 1)
                {
                    _chr_bank_0 = _load;
                    update_chr_rom_mapping();
                }
                else if (reg == 2)
                {
                    _chr_bank_1 = _load;
                    update_chr_rom_mapping();
                }
                else if (reg == 3)
                {
                    _prg_bank = _load;
                    update_prg_rom_mapping();
                }

                _load = 0x10;
            }
        }
    }
}


bool Mapper_001::ppu_read(uint16_t address, uint8_t& value)
{
    if (address >= 0x0000 && address < 0x1000)
    {
        uint16_t offset = address & 0x0FFF;
        value = chr_rom()[_x0000 + offset];
        return true;
    }
    else if (address >= 0x1000 && address < 0x2000)
    {
        uint16_t offset = address & 0x0FFF;
        value = chr_rom()[_x1000 + offset];
        return true;
    }

    return false;
}


bool Mapper_001::ppu_write(uint16_t address, uint8_t value)
{
    if (address >= 0x0000 && address < 0x1000)
    {
        uint16_t offset = address & 0x0FFF;
        chr_rom()[_x0000 + offset] = value;
        return true;
    }
    else if (address >= 0x1000 && address < 0x2000)
    {
        uint16_t offset = address & 0x0FFF;
        chr_rom()[_x1000 + offset] = value;
        return true;
    }

    return false;
}


bool Mapper_001::ppu_remap_address(uint16_t& address)
{
    if (address >= 0x2000 && address < 0x2FFF)
    {
        uint8_t mirroring = _control & 3;
        if (mirroring == 0)
        {
            // 0: one - screen, lower bank
            address = 0x2000 + (address & 0x3FF);
        }
        else if (mirroring == 1)
        {
            // 1: one - screen, upper bank
            address = 0x2400 + (address & 0x3FF);
        }
        else if (mirroring == 2)
        {
            // 2: vertical
            set_bit(address, 11, 0);
        }
        else
        {
            // 3: horizontal
            set_bit(address, 10, get_bit(address, 11));
            set_bit(address, 11, 0);
        }

        return true;
    }

    return false;
}


void Mapper_001::update_prg_rom_mapping()
{
    uint8_t prg_mode = (_control >> 2) & 3;

    if (prg_mode < 2)
    {
        // Switch 32 KB at $8000, ignoring low bit of bank number
        _x8000 = (_prg_bank & 0xE) * 0x8000;
        _xC000 = _x8000 + 0x4000;
    }
    else if (prg_mode == 2)
    {
        // Fix first bank at $8000 and switch 16 KB bank at $C000
        _x8000 = 0;
        _xC000 = _prg_bank * 0x4000;
    }
    else if (prg_mode == 3)
    {
        // Fix last bank at $C000 and switch 16 KB bank at $8000)
        _x8000 = _prg_bank * 0x4000;
        _xC000 = (header()[4] - 1) * 0x4000;
    }
}


void Mapper_001::update_chr_rom_mapping()
{
    if (get_bit(_control, 4) == 0)
    {
        // Switch 8KB at a time
        _x0000 = (_chr_bank_0 & 0xE) * 0x2000;
        _x1000 = _x0000 + 0x1000;
    }
    else
    {
        // Switch two separate 4KB banks
        _x0000 = _chr_bank_0 * 0x1000;
        _x1000 = _chr_bank_1 * 0x1000;
    }
}
