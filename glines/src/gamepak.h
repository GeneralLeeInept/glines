#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

class gli2A03;
class gli2C02;

class GamePak
{
public:
    GamePak() = default;
    ~GamePak() = default;

    bool load(const std::string& path);
    void connect(gli2A03* cpu, gli2C02* ppu);
    void reset(bool coldstart);

    uint8_t cpu_read(uint16_t address);
    void cpu_write(uint16_t address, uint8_t value);

    bool ppu_read(uint16_t address, uint8_t& value);
    bool ppu_write(uint16_t address, uint8_t value);
    uint16_t ppu_remap_address(uint16_t address);

protected:
    friend class Mapper;

    std::array<char, 16> _header_mem{};
    std::vector<uint8_t> _prg_rom;
    std::vector<uint8_t> _chr_rom;
    std::shared_ptr<class Mapper> _mapper;
    gli2A03* _cpu;
    gli2C02* _ppu;
};
