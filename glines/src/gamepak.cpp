#include "gamepak.h"

#include "bits.h"
#include "mapper.h"
#include "mapper_000.h"
#include "mapper_001.h"
#include "mapper_002.h"
#include "mapper_003.h"
#include "mapper_004.h"

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
            case 2:
            {
                _mapper = std::make_shared<Mapper_002>(*this);
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


void GamePak::connect(gli2A03* cpu, gli2C02* ppu)
{
    _cpu = cpu;
    _ppu = ppu;
}


void GamePak::reset(bool coldstart)
{
    _mapper ? _mapper->reset(coldstart) : ((void)0);
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
            if ((_header_mem[6] & 4) == 0)
            {
                if ((_header_mem[6] & 1) == 0)
                {
                    // Horizontal
                    set_bit(address, 10, get_bit(address, 11));
                    set_bit(address, 11, 0);
                }
                else
                {
                    // Vertical
                    set_bit(address, 11, 0);
                }
            }
        }
    }

    return address;
}
