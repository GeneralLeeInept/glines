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

    virtual uint8_t cpu_read(uint16_t addr) = 0;
    virtual void cpu_write(uint16_t addr, uint8_t val) = 0;

protected:
    GamePak& _game_pak;

    std::vector<uint8_t>& prg_rom() { return _game_pak._prg_rom;  }
};


class Mapper_000 : public Mapper
{
public:
    Mapper_000(GamePak& cartridge)
        : Mapper(cartridge) {}

    uint8_t cpu_read(uint16_t addr) override
    {
        if (addr >= 0x8000)
        {
            addr &= (prg_rom().size() - 1);
            return prg_rom()[addr];
        }

        return 0;
    }

    void cpu_write(uint16_t addr, uint8_t val) override {}
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
        if (_header_mem[6] & 4)
            ifs.seekg(512, std::ios_base::cur);

        if (!ifs)
            return false;

        // Read PRG ROM
        _prg_rom.resize(_header_mem[4] * size_t(0x4000));
        ifs.read((char*)(_prg_rom.data()), _prg_rom.size());

        // Read CHR ROM
        _chr_rom.resize(_header_mem[5] * size_t(0x2000));
        ifs.read((char*)(_chr_rom.data()), _chr_rom.size());

        // Setup mapper
        uint8_t mapper_num{};
        mapper_num = (_header_mem[7] & 0xf0) | (_header_mem[6] >> 4);

        switch (mapper_num)
        {
            case 0:
            {
                _mapper = std::make_shared<Mapper_000>(*this);
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


uint8_t GamePak::cpu_read(uint16_t addr)
{
    return _mapper ? _mapper->cpu_read(addr) : 0;
}


void GamePak::cpu_write(uint16_t addr, uint8_t val)
{
    _mapper ? _mapper->cpu_write(addr, val) : ((void)0);
}
