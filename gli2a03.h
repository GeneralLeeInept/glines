#pragma once

#include <cstdint>
#include <functional>
#include <string>

class gli2A03
{
public:
    typedef std::function<uint8_t(uint16_t addr)> ReadCallback;
    typedef std::function<void(uint16_t addr, uint8_t data)> WriteCallback;

    gli2A03() = default;
    ~gli2A03() = default;

    void connect(ReadCallback read_callback, WriteCallback write_callback);

    void reset(bool coldstart);
    void clock();

    void nmi() { _nmi = 1; }

    std::string disassemble(uint16_t addr);

    uint16_t    _pc;        // program counter
    uint8_t     _p;         // status register
    uint8_t     _a;         // accumulator
    uint8_t     _x;         // x register
    uint8_t     _y;         // y register
    uint8_t     _s;         // stack pointer
    bool        _stopped;   // CPU halted

private:
    ReadCallback read;
    WriteCallback write;

    uint8_t _ir;
    uint8_t _instruction_cycles_remaining;
    uint64_t _cycle_counter;
    uint8_t _nmi = 0;

    uint16_t read_word(uint16_t addr);
    void push(uint8_t value);
    uint8_t pop();

    void exec();
};