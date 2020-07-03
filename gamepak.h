#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

class GamePak
{
public:
    GamePak() = default;
    ~GamePak() = default;

    bool load(const std::string& path);

    uint8_t cpu_read(uint16_t addr);
    void cpu_write(uint16_t addr, uint8_t val);

protected:
    friend class Mapper;

    std::array<char, 16> _header_mem{};
    std::vector<uint8_t> _prg_rom;
    std::vector<uint8_t> _chr_rom;
    std::shared_ptr<class Mapper> _mapper;
};
