#include "gamepak.h"

#include "bits.h"

#include <fstream>


struct INesHeader
{
    uint8_t magic[4];
    uint8_t prg_rom_size;
    uint8_t chr_rom_size;
    uint8_t mirroring : 1;
    uint8_t battery : 1;
    uint8_t trainer : 1;
    uint8_t mirroring_ignore : 1;
    uint8_t mapper_lo : 4;
    uint8_t vs_system : 1;
    uint8_t playchoice10 : 1;
    uint8_t nes2 : 2;
    uint8_t mapper_hi : 4;
    uint8_t prg_ram_size;
    uint8_t pal : 1;
    uint8_t reserved : 7;
    uint8_t tv_system : 2;
    uint8_t unused : 2;
    uint8_t prg_ram : 1;
    uint8_t bus_conflicts : 1;
    uint8_t unused2 : 2;
};


class Mapper
{
public:
    Mapper(GamePak& game_pak)
        : _game_pak{ game_pak } {};

    virtual uint8_t cpu_read(uint16_t address) = 0;
    virtual void cpu_write(uint16_t address, uint8_t value) = 0;

    virtual bool ppu_read(uint16_t address, uint8_t& value) = 0;
    virtual bool ppu_write(uint16_t address, uint8_t value) = 0;

    virtual bool ppu_remap_address(uint16_t& address)
    {
        return false;
    }

protected:
    GamePak& _game_pak;

    std::array<char, 16>& header() { return _game_pak._header_mem; }
    std::vector<uint8_t>& prg_rom() { return _game_pak._prg_rom;  }
    std::vector<uint8_t>& chr_rom() { return _game_pak._chr_rom;  }
};


class Mapper_000 : public Mapper
{
public:
    Mapper_000(GamePak& cartridge)
        : Mapper(cartridge) {}

    uint8_t cpu_read(uint16_t address) override
    {
        if (address >= 0x8000)
        {
            address &= (prg_rom().size() - 1);
            return prg_rom()[address];
        }

        return 0;
    }


    void cpu_write(uint16_t address, uint8_t value) override {}


    bool ppu_read(uint16_t address, uint8_t& value) override
    {
        if (address < 0x2000)
        {
            address &= (chr_rom().size() - 1);
            value = chr_rom()[address];
            return true;
        }

        return false;
    }


    bool ppu_write(uint16_t address, uint8_t value) override
    {
        return false;
    }
};


class Mapper_001 : public Mapper
{
public:
    Mapper_001(GamePak& cartridge)
        : Mapper(cartridge)
    {
        _prg_bank = 0;
        _chr_bank_0 = 0;
        reset();
    }


    void reset()    // TODO: Virtualize and expose to system reset
    {
        _load = 0x10;
        _control = _control | 0x0C;

        update_prg_rom_mapping();
        update_chr_rom_mapping();
    }


    uint8_t cpu_read(uint16_t address) override
    {
        uint8_t value = 0;

        if (address >= 0x6000 && address < 0x8000)
        {
            // TODO: Test PRG RAM chip enable? Per docs it is ignored on MMC1A
            //if (get_bit(_prg_bank, 4) == 0)
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


    void cpu_write(uint16_t address, uint8_t value) override
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
                reset();
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


    bool ppu_read(uint16_t address, uint8_t& value) override
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
            value = prg_rom()[_x1000 + offset];
            return true;
        }
        else if (address >= 0x2000 && address < 0x2FFF)
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
                if ((address & 0xFF00) == 0x2000 || (address & 0xFF00) == 0x2800)
                {
                    address = 0x2000 | (address & 0x3FF);
                }
                else if ((address & 0xFF00) == 0x2400 || (address & 0xFF00) == 0x2C00)
                {
                    address = 0x2400 | (address & 0x3FF);
                }
            }
            else
            {
                // 3: horizontal
                if ((address & 0xFF00) == 0x2000 || (address & 0xFF00) == 0x2400)
                {
                    address = 0x2000 | (address & 0x3FF);
                }
                else if ((address & 0xFF00) == 0x2800 || (address & 0xFF00) == 0x2C00)
                {
                    address = 0x2400 | (address & 0x3FF);
                }
            }
        }
        return false;
    }


    bool ppu_write(uint16_t address, uint8_t value) override
    {
        if (address >= 0x0000 && address < 0x1000)
        {
            uint16_t offset = address & 0x0FFF;
            chr_rom()[_x0000 + offset] = value;
            return true;
        }
        return false;
    }


    void update_prg_rom_mapping()
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
            _xC000 = (_prg_bank & 0xE) * 0x4000;
        }
        else if (prg_mode == 3)
        {
            // Fix last bank at $C000 and switch 16 KB bank at $8000)
            _x8000 = (_prg_bank & 0xE) * 0x4000;
            _xC000 = (header()[4] - 1) * 0x4000;
        }
    }


    void update_chr_rom_mapping()
    {
        if (get_bit(_control, 4) == 0)
        {
            // Switch 8KB at a time
            _x0000 = (_chr_bank_0 & 0xE) * 0x1000;
            _x1000 = _x0000 + 0x1000;
        }
        else
        {
            // Switch two separate 4KB banks
            _x0000 = _chr_bank_0 * 0x1000;
            _x1000 = _chr_bank_1 * 0x1000;
        }
    }


    // PRG RAM
    std::array<uint8_t, 0x2000> _prg_ram;


    // Registers
    uint8_t _load;
    uint8_t _control;
    uint8_t _chr_bank_0;
    uint8_t _chr_bank_1;
    uint8_t _prg_bank;


    // PRG ROM mapping
    uint16_t _x8000;
    uint16_t _xC000;


    // CHR ROM mapping
    uint16_t _x0000;
    uint16_t _x1000;
};


class Mapper_003 : public Mapper
{
public:
    Mapper_003(GamePak& cartridge)
        : Mapper(cartridge) {}

    uint8_t cpu_read(uint16_t address) override
    {
        if (address >= 0x8000)
        {
            address &= (prg_rom().size() - 1);
            return prg_rom()[address];
        }

        return 0;
    }


    void cpu_write(uint16_t address, uint8_t value) override
    {
        if (address > 0x8000)
        {
            INesHeader* ih = (INesHeader*)header().data();
            _chr_bank = value & (ih->chr_rom_size - 1);
        }
    }


    bool ppu_read(uint16_t address, uint8_t& value) override
    {
        if (address < 0x2000)
        {
            uint32_t base = _chr_bank * 0x2000;
            uint32_t offset = address & 0x1FFF;
            uint32_t read_address = base + offset;
            value = chr_rom()[read_address];
            return true;
        }

        return false;
    }


    bool ppu_write(uint16_t address, uint8_t value) override
    {
        return false;
    }

    uint8_t _chr_bank = 0;
};


class Mapper_004 : public Mapper
{
public:
    Mapper_004(GamePak& cartridge)
        : Mapper(cartridge) {}

    uint8_t cpu_read(uint16_t address) override
    {
        if (address >= 0x8000)
        {
            address &= (prg_rom().size() - 1);
            return prg_rom()[address];
        }

        return 0;
    }


    void cpu_write(uint16_t address, uint8_t value) override {}


    bool ppu_read(uint16_t address, uint8_t& value) override
    {
        if (address < 0x2000)
        {
            address &= (chr_rom().size() - 1);
            value = chr_rom()[address];
            return true;
        }

        return false;
    }


    bool ppu_write(uint16_t address, uint8_t value) override
    {
        return false;
    }
}; 


bool GamePak::load(const std::string& path)
{
    std::ifstream ifs(path, std::ios::binary);

    if (!ifs)
    {
        return false;
    }

    ifs.read(_header_mem.data(), 16);

    if (!ifs)
    {
        return false;
    }

    bool ines = (_header_mem[0] == 'N' && _header_mem[1] == 'E' && _header_mem[2] == 'S' && _header_mem[3] == 0x1A);

    if (!ines)
    {
        return false;
    }

    if ((_header_mem[7] & 0x0C) == 0x08)
    {
        // NES2.0
    }
    else
    {
        // iNES
        INesHeader* header = (INesHeader*)_header_mem.data();

        // Skip 512 byte trainer
        if (header->trainer)
            ifs.seekg(512, std::ios_base::cur);

        if (!ifs)
            return false;

        // Read PRG ROM
        _prg_rom.resize(header->prg_rom_size * size_t(0x4000));
        ifs.read((char*)(_prg_rom.data()), _prg_rom.size());

        // Read CHR ROM
        if (header->chr_rom_size == 0)
        {
            // FIXME: Must have CHR RAM?
            header->chr_rom_size = 0x10;
        }

        _chr_rom.resize(header->chr_rom_size * size_t(0x2000));
        ifs.read((char*)(_chr_rom.data()), _chr_rom.size());

        // Setup mapper
        uint8_t mapper_num = (header->mapper_hi << 4) | header->mapper_lo;

        switch (mapper_num)
        {
            case 0:
            {
                _mapper = std::make_shared<Mapper_000>(*this);
                break;
            }
            case 1:
            {
                _mapper = std::make_shared<Mapper_001>(*this);
                break;
            }
            case 3:
            {
                _mapper = std::make_shared<Mapper_003>(*this);
                break;
            }
            case 4:
            {
                _mapper = std::make_shared<Mapper_004>(*this);
                break;
            }
            default:
            {
                return false;
            }
        }
    }

    return true;
}


uint8_t GamePak::cpu_read(uint16_t address)
{
    return _mapper ? _mapper->cpu_read(address) : 0;
}


void GamePak::cpu_write(uint16_t address, uint8_t value)
{
    _mapper ? _mapper->cpu_write(address, value) : ((void)0);
}


bool GamePak::ppu_read(uint16_t address, uint8_t& value)
{
    return _mapper ? _mapper->ppu_read(address, value) : false;
}


bool GamePak::ppu_write(uint16_t address, uint8_t value)
{
    return _mapper ? _mapper->ppu_write(address, value) : false;
}


uint16_t GamePak::ppu_remap_address(uint16_t address)
{
    if (!_mapper || !_mapper->ppu_remap_address(address))
    {
        if (address >= 0x2000 && address < 0x3000)
        {
            // Mirroring
            if ((_header_mem[6] & 1) == 0)
            {
                // Horizontal
                if ((address & 0xFF00) == 0x2000 || (address & 0xFF00) == 0x2400)
                {
                    address = 0x2000 | (address & 0x3FF);
                }
                else if ((address & 0xFF00) == 0x2800 || (address & 0xFF00) == 0x2C00)
                {
                    address = 0x2400 | (address & 0x3FF);
                }
            }
            else
            {
                // Vertical
                if ((address & 0xFF00) == 0x2000 || (address & 0xFF00) == 0x2800)
                {
                    address = 0x2000 | (address & 0x3FF);
                }
                else if ((address & 0xFF00) == 0x2400 || (address & 0xFF00) == 0x2C00)
                {
                    address = 0x2400 | (address & 0x3FF);
                }
            }
        }
    }

    return address;
}
