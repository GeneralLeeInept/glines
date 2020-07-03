#include "gamepak.h"

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

    virtual uint16_t ppu_remap_address(uint16_t address)
    {
        // TODO: Mirroring
        return address;
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
            case 3:
            {
                _mapper = std::make_shared<Mapper_003>(*this);
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
    return _mapper ? _mapper->ppu_remap_address(address) : address;
}
