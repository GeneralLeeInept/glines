#include "gli2A03.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

// Addressing mode - high bit set means instructions which load across a page boundary using the addressing mode will incur a 1-cycle penalty.
// Instruction length (in bytes) is determined by addressing mode
enum AddressingMode : uint8_t
{
    Implied     = 0x00, // 1 byte
    Immediate   = 0x01, // 2 bytes
    ZeroPage    = 0x02, // 2 bytes
    ZeroPage_X  = 0x03, // 2 bytes
    ZeroPage_Y  = 0x04, // 2 bytes
    Relative    = 0x05, // 2 bytes
    Indirect_X  = 0x06, // (Indirect,X) 2 bytes
    Indirect_Y  = 0x87, // (Indirect),Y 2 bytes
    Indirect    = 0x08, // (Indirect) 3 bytes
    Absolute_X  = 0x89, // 3 bytes
    Absolute_Y  = 0x8A, // 3 bytes
    Absolute    = 0x0B, // 3 bytes
};


// Opcodes - undocumented opcodes commented with a '*'
enum Opcode : uint8_t
{
    ADC,
    ALR,    // *
    ANC,    // *
    AND,
    AHX,    // * aka AXA
    ARR,    // *
    ASL,
    AXS,    // *
    BCC,
    BCS,
    BEQ,
    BIT,
    BMI,
    BNE,
    BPL,
    BRK,
    BVC,
    BVS,
    CLC,
    CLD,
    CLI,
    CLV,
    CMP,
    CPX,
    CPY,
    DCP,    // * aka DCM
    DEC,
    DEX,
    DEY,
    EOR,
    INC,
    INX,
    INY,
    ISC,    // * aka INS
    JMP,
    JSR,
    LAS,    // *
    LAX,    // *
    LDA,
    LDX,
    LDY,
    LSR,
    NOP,
    ORA,
    PHA,
    PHP,
    PLA,
    PLP,
    RLA,    // *
    ROL,
    ROR,
    RRA,    // *
    RTI,
    RTS,
    SAX,    // *
    SBC,
    SEC,
    SED,
    SEI,
    SHX,    // * aka XAS
    SHY,    // * aka SAY
    SLO,    // * aka ASO
    SRE,    // * aka LSE
    STA,
    STP,    // * aka HLT
    STX,
    STY,
    TAS,    // *
    TAX,
    TAY,
    TSX,
    TXA,
    TXS,
    TYA,
    XAA,    // *
};


// Instruction description - high byte of cycles is set for instructions which will incur a penalty when using an indexed addressing mode and
// crossing a page boundary when cacluating the address.
struct Instruction
{
    const char* mnemonic;
    Opcode opcode;
    AddressingMode addressing_mode;
    int cycles;
};


// Instruction lookup table
static const Instruction InstructionTable[] =
{
    { "BRK", Opcode::BRK, AddressingMode::Implied,       0x07 },    // 00
    { "ORA", Opcode::ORA, AddressingMode::Indirect_X,    0x06 },    // 01
    { "STP", Opcode::STP, AddressingMode::Implied,       0x00 },    // 02
    { "SLO", Opcode::SLO, AddressingMode::Indirect_X,    0x08 },    // 03
    { "NOP", Opcode::NOP, AddressingMode::ZeroPage,      0x03 },    // 04
    { "ORA", Opcode::ORA, AddressingMode::ZeroPage,      0x03 },    // 05
    { "ASL", Opcode::ASL, AddressingMode::ZeroPage,      0x05 },    // 06
    { "SLO", Opcode::SLO, AddressingMode::ZeroPage,      0x05 },    // 07
    { "PHP", Opcode::PHP, AddressingMode::Implied,       0x03 },    // 08
    { "ORA", Opcode::ORA, AddressingMode::Immediate,     0x02 },    // 09
    { "ASL", Opcode::ASL, AddressingMode::Implied,       0x02 },    // 0A
    { "ANC", Opcode::ANC, AddressingMode::Immediate,     0x02 },    // 0B
    { "NOP", Opcode::NOP, AddressingMode::Absolute,      0x04 },    // 0C
    { "ORA", Opcode::ORA, AddressingMode::Absolute,      0x04 },    // 0D
    { "ASL", Opcode::ASL, AddressingMode::Absolute,      0x06 },    // 0E
    { "SLO", Opcode::SLO, AddressingMode::Absolute,      0x06 },    // 0F
    { "BPL", Opcode::BPL, AddressingMode::Relative,      0x02 },    // 10
    { "ORA", Opcode::ORA, AddressingMode::Indirect_Y,    0x85 },    // 11
    { "STP", Opcode::STP, AddressingMode::Implied,       0x00 },    // 12
    { "SLO", Opcode::SLO, AddressingMode::Indirect_Y,    0x08 },    // 13
    { "NOP", Opcode::NOP, AddressingMode::ZeroPage_X,    0x04 },    // 14
    { "ORA", Opcode::ORA, AddressingMode::ZeroPage_X,    0x04 },    // 15
    { "ASL", Opcode::ASL, AddressingMode::ZeroPage_X,    0x06 },    // 16
    { "SLO", Opcode::SLO, AddressingMode::ZeroPage_X,    0x06 },    // 17
    { "CLC", Opcode::CLC, AddressingMode::Implied,       0x02 },    // 18
    { "ORA", Opcode::ORA, AddressingMode::Absolute_Y,    0x84 },    // 19
    { "NOP", Opcode::NOP, AddressingMode::Implied,       0x02 },    // 1A
    { "SLO", Opcode::SLO, AddressingMode::Absolute_Y,    0x07 },    // 1B
    { "NOP", Opcode::NOP, AddressingMode::Absolute_X,    0x84 },    // 1C
    { "ORA", Opcode::ORA, AddressingMode::Absolute_X,    0x84 },    // 1D
    { "ASL", Opcode::ASL, AddressingMode::Absolute_X,    0x07 },    // 1E
    { "SLO", Opcode::SLO, AddressingMode::Absolute_X,    0x07 },    // 1F

    { "JSR", Opcode::JSR, AddressingMode::Absolute,      0x06 },    // 20
    { "AND", Opcode::AND, AddressingMode::Indirect_X,    0x06 },    // 21
    { "STP", Opcode::STP, AddressingMode::Implied,       0x00 },    // 22
    { "RLA", Opcode::RLA, AddressingMode::Indirect_X,    0x08 },    // 23
    { "BIT", Opcode::BIT, AddressingMode::ZeroPage,      0x03 },    // 24
    { "AND", Opcode::AND, AddressingMode::ZeroPage,      0x03 },    // 25
    { "ROL", Opcode::ROL, AddressingMode::ZeroPage,      0x05 },    // 26
    { "RLA", Opcode::RLA, AddressingMode::ZeroPage,      0x05 },    // 27
    { "PLP", Opcode::PLP, AddressingMode::Implied,       0x04 },    // 28
    { "AND", Opcode::AND, AddressingMode::Immediate,     0x02 },    // 29
    { "ROL", Opcode::ROL, AddressingMode::Implied,       0x02 },    // 2A
    { "ANC", Opcode::ANC, AddressingMode::Immediate,     0x02 },    // 2B
    { "BIT", Opcode::BIT, AddressingMode::Absolute,      0x04 },    // 2C
    { "AND", Opcode::AND, AddressingMode::Absolute,      0x04 },    // 2D
    { "ROL", Opcode::ROL, AddressingMode::Absolute,      0x06 },    // 2E
    { "RLA", Opcode::RLA, AddressingMode::Absolute,      0x06 },    // 2F
    { "BMI", Opcode::BMI, AddressingMode::Relative,      0x02 },    // 30
    { "AND", Opcode::AND, AddressingMode::Indirect_Y,    0x85 },    // 31
    { "STP", Opcode::STP, AddressingMode::Implied,       0x00 },    // 32
    { "RLA", Opcode::RLA, AddressingMode::Indirect_Y,    0x08 },    // 33
    { "NOP", Opcode::NOP, AddressingMode::ZeroPage_X,    0x04 },    // 34
    { "AND", Opcode::AND, AddressingMode::ZeroPage_X,    0x04 },    // 35
    { "ROL", Opcode::ROL, AddressingMode::ZeroPage_X,    0x06 },    // 36
    { "RLA", Opcode::RLA, AddressingMode::ZeroPage_X,    0x06 },    // 37
    { "SEC", Opcode::SEC, AddressingMode::Implied,       0x02 },    // 38
    { "AND", Opcode::AND, AddressingMode::Absolute_Y,    0x84 },    // 39
    { "NOP", Opcode::NOP, AddressingMode::Implied,       0x02 },    // 3A
    { "RLA", Opcode::RLA, AddressingMode::Absolute_Y,    0x07 },    // 3B
    { "NOP", Opcode::NOP, AddressingMode::Absolute_X,    0x84 },    // 3C
    { "AND", Opcode::AND, AddressingMode::Absolute_X,    0x84 },    // 3D
    { "ROL", Opcode::ROL, AddressingMode::Absolute_X,    0x07 },    // 3E
    { "RLA", Opcode::RLA, AddressingMode::Absolute_X,    0x07 },    // 3F

    { "RTI", Opcode::RTI, AddressingMode::Implied,       0x06 },    // 40
    { "EOR", Opcode::EOR, AddressingMode::Indirect_X,    0x06 },    // 41
    { "STP", Opcode::STP, AddressingMode::Implied,       0x00 },    // 42
    { "SRE", Opcode::SRE, AddressingMode::Indirect_X,    0x08 },    // 43
    { "NOP", Opcode::NOP, AddressingMode::ZeroPage,      0x03 },    // 44
    { "EOR", Opcode::EOR, AddressingMode::ZeroPage,      0x03 },    // 45
    { "LSR", Opcode::LSR, AddressingMode::ZeroPage,      0x05 },    // 46
    { "SRE", Opcode::SRE, AddressingMode::ZeroPage,      0x05 },    // 47
    { "PHA", Opcode::PHA, AddressingMode::Implied,       0x03 },    // 48
    { "EOR", Opcode::EOR, AddressingMode::Immediate,     0x02 },    // 49
    { "LSR", Opcode::LSR, AddressingMode::Implied,       0x02 },    // 4A
    { "ALR", Opcode::ALR, AddressingMode::Immediate,     0x02 },    // 4B
    { "JMP", Opcode::JMP, AddressingMode::Absolute,      0x03 },    // 4C
    { "EOR", Opcode::EOR, AddressingMode::Absolute,      0x04 },    // 4D
    { "LSR", Opcode::LSR, AddressingMode::Absolute,      0x06 },    // 4E
    { "SRE", Opcode::SRE, AddressingMode::Absolute,      0x06 },    // 4F
    { "BVC", Opcode::BVC, AddressingMode::Relative,      0x02 },    // 50
    { "EOR", Opcode::EOR, AddressingMode::Indirect_Y,    0x85 },    // 51
    { "STP", Opcode::STP, AddressingMode::Implied,       0x00 },    // 52
    { "SRE", Opcode::SRE, AddressingMode::Indirect_Y,    0x08 },    // 53
    { "NOP", Opcode::NOP, AddressingMode::ZeroPage_X,    0x04 },    // 54
    { "EOR", Opcode::EOR, AddressingMode::ZeroPage_X,    0x04 },    // 55
    { "LSR", Opcode::LSR, AddressingMode::ZeroPage_X,    0x06 },    // 56
    { "SRE", Opcode::SRE, AddressingMode::ZeroPage_X,    0x06 },    // 57
    { "CLI", Opcode::CLI, AddressingMode::Implied,       0x02 },    // 58
    { "EOR", Opcode::EOR, AddressingMode::Absolute_Y,    0x84 },    // 59
    { "NOP", Opcode::NOP, AddressingMode::Implied,       0x02 },    // 5A
    { "SRE", Opcode::SRE, AddressingMode::Absolute_Y,    0x07 },    // 5B
    { "NOP", Opcode::NOP, AddressingMode::Absolute_X,    0x84 },    // 5C
    { "EOR", Opcode::EOR, AddressingMode::Absolute_X,    0x84 },    // 5D
    { "LSR", Opcode::LSR, AddressingMode::Absolute_X,    0x07 },    // 5E
    { "SRE", Opcode::SRE, AddressingMode::Absolute_X,    0x07 },    // 5F

    { "RTS", Opcode::RTS, AddressingMode::Implied,       0x06 },    // 60
    { "ADC", Opcode::ADC, AddressingMode::Indirect_X,    0x06 },    // 61
    { "STP", Opcode::STP, AddressingMode::Implied,       0x00 },    // 62
    { "RRA", Opcode::RRA, AddressingMode::Indirect_X,    0x08 },    // 63
    { "NOP", Opcode::NOP, AddressingMode::ZeroPage,      0x03 },    // 64
    { "ADC", Opcode::ADC, AddressingMode::ZeroPage,      0x03 },    // 65
    { "ROR", Opcode::ROR, AddressingMode::ZeroPage,      0x05 },    // 66
    { "RRA", Opcode::RRA, AddressingMode::ZeroPage,      0x05 },    // 67
    { "PLA", Opcode::PLA, AddressingMode::Implied,       0x04 },    // 68
    { "ADC", Opcode::ADC, AddressingMode::Immediate,     0x02 },    // 69
    { "ROR", Opcode::ROR, AddressingMode::Implied,       0x02 },    // 6A
    { "ARR", Opcode::ARR, AddressingMode::Immediate,     0x02 },    // 6B
    { "JMP", Opcode::JMP, AddressingMode::Indirect,      0x05 },    // 6C
    { "ADC", Opcode::ADC, AddressingMode::Absolute,      0x04 },    // 6D
    { "ROR", Opcode::ROR, AddressingMode::Absolute,      0x06 },    // 6E
    { "RRA", Opcode::RRA, AddressingMode::Absolute,      0x06 },    // 6F
    { "BVS", Opcode::BVS, AddressingMode::Relative,      0x02 },    // 70
    { "ADC", Opcode::ADC, AddressingMode::Indirect_Y,    0x85 },    // 71
    { "STP", Opcode::STP, AddressingMode::Implied,       0x00 },    // 72
    { "RRA", Opcode::RRA, AddressingMode::Indirect_Y,    0x08 },    // 73
    { "NOP", Opcode::NOP, AddressingMode::ZeroPage_X,    0x04 },    // 74
    { "ADC", Opcode::ADC, AddressingMode::ZeroPage_X,    0x04 },    // 75
    { "ROR", Opcode::ROR, AddressingMode::ZeroPage_X,    0x06 },    // 76
    { "RRA", Opcode::RRA, AddressingMode::ZeroPage_X,    0x06 },    // 77
    { "SEI", Opcode::SEI, AddressingMode::Implied,       0x02 },    // 78
    { "ADC", Opcode::ADC, AddressingMode::Absolute_Y,    0x84 },    // 79
    { "NOP", Opcode::NOP, AddressingMode::Implied,       0x02 },    // 7A
    { "RRA", Opcode::RRA, AddressingMode::Absolute_Y,    0x07 },    // 7B
    { "NOP", Opcode::NOP, AddressingMode::Absolute_X,    0x84 },    // 7C
    { "ADC", Opcode::ADC, AddressingMode::Absolute_X,    0x84 },    // 7D
    { "ROR", Opcode::ROR, AddressingMode::Absolute_X,    0x07 },    // 7E
    { "RRA", Opcode::RRA, AddressingMode::Absolute_X,    0x07 },    // 7F

    { "NOP", Opcode::NOP, AddressingMode::Immediate,     0x02 },    // 80
    { "STA", Opcode::STA, AddressingMode::Indirect_X,    0x06 },    // 81
    { "NOP", Opcode::NOP, AddressingMode::Immediate,     0x02 },    // 82
    { "SAX", Opcode::SAX, AddressingMode::Indirect_X,    0x06 },    // 83
    { "STY", Opcode::STY, AddressingMode::ZeroPage,      0x03 },    // 84
    { "STA", Opcode::STA, AddressingMode::ZeroPage,      0x03 },    // 85
    { "STX", Opcode::STX, AddressingMode::ZeroPage,      0x03 },    // 86
    { "SAX", Opcode::SAX, AddressingMode::ZeroPage,      0x03 },    // 87
    { "DEY", Opcode::DEY, AddressingMode::Implied,       0x02 },    // 88
    { "NOP", Opcode::NOP, AddressingMode::Immediate,     0x02 },    // 89
    { "TXA", Opcode::TXA, AddressingMode::Implied,       0x02 },    // 8A
    { "XAA", Opcode::XAA, AddressingMode::Immediate,     0x02 },    // 8B
    { "STY", Opcode::STY, AddressingMode::Absolute,      0x04 },    // 8C
    { "STA", Opcode::STA, AddressingMode::Absolute,      0x04 },    // 8D
    { "STX", Opcode::STX, AddressingMode::Absolute,      0x04 },    // 8E
    { "SAX", Opcode::SAX, AddressingMode::Absolute,      0x04 },    // 8F
    { "BCC", Opcode::BCC, AddressingMode::Relative,      0x02 },    // 90
    { "STA", Opcode::STA, AddressingMode::Indirect_Y,    0x06 },    // 91
    { "STP", Opcode::STP, AddressingMode::Implied,       0x00 },    // 92
    { "AHX", Opcode::AHX, AddressingMode::Indirect_Y,    0x06 },    // 93
    { "STY", Opcode::STY, AddressingMode::ZeroPage_X,    0x04 },    // 94
    { "STA", Opcode::STA, AddressingMode::ZeroPage_X,    0x04 },    // 95
    { "STX", Opcode::STX, AddressingMode::ZeroPage_Y,    0x04 },    // 96
    { "SAX", Opcode::SAX, AddressingMode::ZeroPage_Y,    0x04 },    // 97
    { "TYA", Opcode::TYA, AddressingMode::Implied,       0x02 },    // 98
    { "STA", Opcode::STA, AddressingMode::Absolute_Y,    0x05 },    // 99
    { "TXS", Opcode::TXS, AddressingMode::Implied,       0x02 },    // 9A
    { "TAS", Opcode::TAS, AddressingMode::Absolute_Y,    0x05 },    // 9B
    { "SHY", Opcode::SHY, AddressingMode::Absolute_X,    0x05 },    // 9C
    { "STA", Opcode::STA, AddressingMode::Absolute_X,    0x05 },    // 9D
    { "SHX", Opcode::SHX, AddressingMode::Absolute_Y,    0x05 },    // 9E
    { "AHX", Opcode::AHX, AddressingMode::Absolute_Y,    0x05 },    // 9F

    { "LDY", Opcode::LDY, AddressingMode::Immediate,     0x02 },    // A0
    { "LDA", Opcode::LDA, AddressingMode::Indirect_X,    0x06 },    // A1
    { "LDX", Opcode::LDX, AddressingMode::Immediate,     0x02 },    // A2
    { "LAX", Opcode::LAX, AddressingMode::Indirect_X,    0x06 },    // A3
    { "LDY", Opcode::LDY, AddressingMode::ZeroPage,      0x03 },    // A4
    { "LDA", Opcode::LDA, AddressingMode::ZeroPage,      0x03 },    // A5
    { "LDX", Opcode::LDX, AddressingMode::ZeroPage,      0x03 },    // A6
    { "LAX", Opcode::LAX, AddressingMode::ZeroPage,      0x03 },    // A7
    { "TAY", Opcode::TAY, AddressingMode::Implied,       0x02 },    // A8
    { "LDA", Opcode::LDA, AddressingMode::Immediate,     0x02 },    // A9
    { "TAX", Opcode::TAX, AddressingMode::Implied,       0x02 },    // AA
    { "LAX", Opcode::LAX, AddressingMode::Immediate,     0x02 },    // AB
    { "LDY", Opcode::LDY, AddressingMode::Absolute,      0x04 },    // AC
    { "LDA", Opcode::LDA, AddressingMode::Absolute,      0x04 },    // AD
    { "LDX", Opcode::LDX, AddressingMode::Absolute,      0x04 },    // AE
    { "LAX", Opcode::LAX, AddressingMode::Absolute,      0x04 },    // AF
    { "BCS", Opcode::BCS, AddressingMode::Relative,      0x02 },    // B0
    { "LDA", Opcode::LDA, AddressingMode::Indirect_Y,    0x85 },    // B1
    { "STP", Opcode::STP, AddressingMode::Implied,       0x00 },    // B2
    { "LAX", Opcode::LAX, AddressingMode::Indirect_Y,    0x85 },    // B3
    { "LDY", Opcode::LDY, AddressingMode::ZeroPage_X,    0x04 },    // B4
    { "LDA", Opcode::LDA, AddressingMode::ZeroPage_X,    0x04 },    // B5
    { "LDX", Opcode::LDX, AddressingMode::ZeroPage_Y,    0x04 },    // B6
    { "LAX", Opcode::LAX, AddressingMode::ZeroPage_Y,    0x04 },    // B7
    { "CLV", Opcode::CLV, AddressingMode::Implied,       0x02 },    // B8
    { "LDA", Opcode::LDA, AddressingMode::Absolute_Y,    0x84 },    // B9
    { "TSX", Opcode::TSX, AddressingMode::Implied,       0x02 },    // BA
    { "LAS", Opcode::LAS, AddressingMode::Absolute_Y,    0x84 },    // BB
    { "LDY", Opcode::LDY, AddressingMode::Absolute_X,    0x84 },    // BC
    { "LDA", Opcode::LDA, AddressingMode::Absolute_X,    0x84 },    // BD
    { "LDX", Opcode::LDX, AddressingMode::Absolute_Y,    0x84 },    // BE
    { "LAX", Opcode::LAX, AddressingMode::Absolute_Y,    0x84 },    // BF

    { "CPY", Opcode::CPY, AddressingMode::Immediate,     0x02 },    // C0
    { "CMP", Opcode::CMP, AddressingMode::Indirect_X,    0x06 },    // C1
    { "NOP", Opcode::NOP, AddressingMode::Immediate,     0x02 },    // C2
    { "DCP", Opcode::DCP, AddressingMode::Indirect_X,    0x08 },    // C3
    { "CPY", Opcode::CPY, AddressingMode::ZeroPage,      0x03 },    // C4
    { "CMP", Opcode::CMP, AddressingMode::ZeroPage,      0x03 },    // C5
    { "DEC", Opcode::DEC, AddressingMode::ZeroPage,      0x05 },    // C6
    { "DCP", Opcode::DCP, AddressingMode::ZeroPage,      0x05 },    // C7
    { "INY", Opcode::INY, AddressingMode::Implied,       0x02 },    // C8
    { "CMP", Opcode::CMP, AddressingMode::Immediate,     0x02 },    // C9
    { "DEX", Opcode::DEX, AddressingMode::Implied,       0x02 },    // CA
    { "AXS", Opcode::AXS, AddressingMode::Immediate,     0x02 },    // CB
    { "CPY", Opcode::CPY, AddressingMode::Absolute,      0x04 },    // CC
    { "CMP", Opcode::CMP, AddressingMode::Absolute,      0x04 },    // CD
    { "DEC", Opcode::DEC, AddressingMode::Absolute,      0x06 },    // CE
    { "DCP", Opcode::DCP, AddressingMode::Absolute,      0x06 },    // CF
    { "BNE", Opcode::BNE, AddressingMode::Relative,      0x02 },    // D0
    { "CMP", Opcode::CMP, AddressingMode::Indirect_Y,    0x85 },    // D1
    { "STP", Opcode::STP, AddressingMode::Implied,       0x00 },    // D2
    { "DCP", Opcode::DCP, AddressingMode::Indirect_Y,    0x08 },    // D3
    { "NOP", Opcode::NOP, AddressingMode::ZeroPage_X,    0x04 },    // D4
    { "CMP", Opcode::CMP, AddressingMode::ZeroPage_X,    0x04 },    // D5
    { "DEC", Opcode::DEC, AddressingMode::ZeroPage_X,    0x06 },    // D6
    { "DCP", Opcode::DCP, AddressingMode::ZeroPage_X,    0x06 },    // D7
    { "CLD", Opcode::CLD, AddressingMode::Implied,       0x02 },    // D8
    { "CMP", Opcode::CMP, AddressingMode::Absolute_Y,    0x84 },    // D9
    { "NOP", Opcode::NOP, AddressingMode::Implied,       0x02 },    // DA
    { "DCP", Opcode::DCP, AddressingMode::Absolute_Y,    0x07 },    // DB
    { "NOP", Opcode::NOP, AddressingMode::Absolute_X,    0x84 },    // DC
    { "CMP", Opcode::CMP, AddressingMode::Absolute_X,    0x84 },    // DD
    { "DEC", Opcode::DEC, AddressingMode::Absolute_X,    0x07 },    // DE
    { "DCP", Opcode::DCP, AddressingMode::Absolute_X,    0x07 },    // DF

    { "CPX", Opcode::CPX, AddressingMode::Immediate,     0x02 },    // E0
    { "SBC", Opcode::SBC, AddressingMode::Indirect_X,    0x06 },    // E1
    { "NOP", Opcode::NOP, AddressingMode::Immediate,     0x02 },    // E2
    { "ISC", Opcode::ISC, AddressingMode::Indirect_X,    0x08 },    // E3
    { "CPX", Opcode::CPX, AddressingMode::ZeroPage,      0x03 },    // E4
    { "SBC", Opcode::SBC, AddressingMode::ZeroPage,      0x03 },    // E5
    { "INC", Opcode::INC, AddressingMode::ZeroPage,      0x05 },    // E6
    { "ISC", Opcode::ISC, AddressingMode::ZeroPage,      0x05 },    // E7
    { "INX", Opcode::INX, AddressingMode::Implied,       0x02 },    // E8
    { "SBC", Opcode::SBC, AddressingMode::Immediate,     0x02 },    // E9
    { "NOP", Opcode::NOP, AddressingMode::Implied,       0x02 },    // EA
    { "SBC", Opcode::SBC, AddressingMode::Immediate,     0x02 },    // EB
    { "CPX", Opcode::CPX, AddressingMode::Absolute,      0x04 },    // EC
    { "SBC", Opcode::SBC, AddressingMode::Absolute,      0x04 },    // ED
    { "INC", Opcode::INC, AddressingMode::Absolute,      0x06 },    // EE
    { "ISC", Opcode::ISC, AddressingMode::Absolute,      0x06 },    // EF
    { "BEQ", Opcode::BEQ, AddressingMode::Relative,      0x02 },    // F0
    { "SBC", Opcode::SBC, AddressingMode::Indirect_Y,    0x85 },    // F1
    { "STP", Opcode::STP, AddressingMode::Implied,       0x00 },    // F2
    { "ISC", Opcode::ISC, AddressingMode::Indirect_Y,    0x08 },    // F3
    { "NOP", Opcode::NOP, AddressingMode::ZeroPage_X,    0x04 },    // F4
    { "SBC", Opcode::SBC, AddressingMode::ZeroPage_X,    0x04 },    // F5
    { "INC", Opcode::INC, AddressingMode::ZeroPage_X,    0x06 },    // F6
    { "ISC", Opcode::ISC, AddressingMode::ZeroPage_X,    0x06 },    // F7
    { "SED", Opcode::SED, AddressingMode::Implied,       0x02 },    // F8
    { "SBC", Opcode::SBC, AddressingMode::Absolute_Y,    0x84 },    // F9
    { "NOP", Opcode::NOP, AddressingMode::Implied,       0x02 },    // FA
    { "ISC", Opcode::ISC, AddressingMode::Absolute_Y,    0x07 },    // FB
    { "NOP", Opcode::NOP, AddressingMode::Absolute_X,    0x84 },    // FC
    { "SBC", Opcode::SBC, AddressingMode::Absolute_X,    0x84 },    // FD
    { "INC", Opcode::INC, AddressingMode::Absolute_X,    0x07 },    // FE
    { "ISC", Opcode::ISC, AddressingMode::Absolute_X,    0x07 },    // FF
};


enum StatusBits : uint8_t
{
    C = 0,
    Z = 1,
    I = 2,
    D = 3,
    B = 4,
    X = 5,
    V = 6,
    N = 7
};


template <typename T>
void set_bit(T& flags, uint8_t bit, int set)
{
    T mask = T(1) << bit;
    flags |= set ? mask : T(0);
    flags &= set ? ~T(0) : ~mask;
}


template <typename T>
int get_bit(T flags, uint8_t bit)
{
    return (flags >> bit) & 1;
}


inline uint8_t lo(uint16_t w)
{
    return (uint8_t)(w & 0xff);
}


inline uint8_t hi(uint16_t w)
{
    return (uint8_t)((w & 0xff00) >> 8);
}


inline uint16_t make_word(uint8_t lo, uint8_t hi)
{
    return (((uint16_t)hi) << 8) | lo;
}


void gli2A03::connect(ReadCallback read_callback, WriteCallback write_callback)
{
    read = read_callback;
    write = write_callback;
}


void gli2A03::reset(bool coldstart)
{
    _pc = read_word(0xfffc);
    _stopped = false;

    if (coldstart)
    {
        // https://wiki.nesdev.com/w/index.php/CPU_ALL#At_power-up
        _p = 0x34;
        _a = _x = _y = 0;
        _s = 0xfd;
    }
    else
    {
        // https://wiki.nesdev.com/w/index.php/CPU_ALL#After_reset
        _s -= 3;
        _p |= 0x4;
    }

    _cycle_counter = 0;
    _instruction_cycles_remaining = 6;
}


void gli2A03::clock()
{
    if (!_stopped)
    {
        ++_cycle_counter;

        if (!_instruction_cycles_remaining)
        {
            // Fetch, decode & execute the next instruction

            if (0)
            {
                char log[128];
                std::string dissassembly = disassemble(_pc);
                snprintf(log, 128, "%04X  %-32s A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%lld\n", _pc, dissassembly.c_str(), _a, _x, _y, _p, _s, _cycle_counter);
                OutputDebugStringA(log);
            }

            _ir = read(_pc++);
            exec();
        }

        --_instruction_cycles_remaining;
    }
}

void gli2A03::step()
{
    if (_stopped)
    {
        return;
    }

    if (!_instruction_cycles_remaining)
    {
        clock();
    }

    while (_instruction_cycles_remaining)
    {
        clock();
    }
}


uint16_t gli2A03::read_word(uint16_t addr)
{
    return make_word(read(addr), read(addr + 1));
}


void gli2A03::push(uint8_t value)
{
    write(0x100 + _s, value);
    _s -= 1;
}


uint8_t gli2A03::pop()
{
    _s += 1;
    return read(0x100 + _s);
}


void gli2A03::exec()
{
    const Instruction& instruction = InstructionTable[_ir];
    uint16_t address = 0xd1ed;
    bool page_crossing_penalty = !!get_bit(instruction.cycles & instruction.addressing_mode, 7);

    _instruction_cycles_remaining = instruction.cycles & 0x7F;  // Mask off high bit

    switch (instruction.addressing_mode)
    {
        case Implied:
        {
            break;
        }
        case Immediate:
        {
            address = _pc++;
            break;
        }
        case ZeroPage:
        {
            address = read(_pc++);
            break;
        }
        case ZeroPage_X:
        {
            address = lo(read(_pc++) + _x);
            break;
        }
        case ZeroPage_Y:
        {
            address = lo(read(_pc++) + _y);
            break;
        }
        case Relative:
        {
            address = (int8_t)read(_pc++) + _pc;
            break;
        }
        case Absolute:
        {
            address = read_word(_pc);
            _pc += 2;
            break;
        }
        case Absolute_X:
        {
            uint16_t absolute_address = read_word(_pc);
            _pc += 2;
            address = absolute_address + _x;

            if (page_crossing_penalty && hi(address) != hi(absolute_address))
            {
                ++_instruction_cycles_remaining;
            }

            break;
        }
        case Absolute_Y:
        {
            uint16_t absolute_address = read_word(_pc);
            _pc += 2;
            address = absolute_address + _y;

            if (page_crossing_penalty && hi(address) != hi(absolute_address))
            {
                ++_instruction_cycles_remaining;
            }

            break;
        }
        case Indirect:
        {
            uint16_t indirect_address = read_word(_pc);
            _pc += 2;
            uint8_t address_lo = read(indirect_address);
            uint8_t address_hi = read(make_word(lo(indirect_address + 1), hi(indirect_address)));
            address = make_word(address_lo, address_hi);
            break;
        }
        case Indirect_X:
        {
            uint16_t indirect_address = lo(read(_pc++) + _x);
            uint8_t address_lo = read(indirect_address);
            uint8_t address_hi = read(make_word(lo(indirect_address + 1), hi(indirect_address)));
            address = make_word(address_lo, address_hi);
            break;
        }
        case Indirect_Y:
        {
            uint16_t indirect_address = read(_pc++);
            uint8_t address_lo = read(indirect_address);
            uint8_t address_hi = read(make_word(lo(indirect_address + 1), hi(indirect_address)));
            address = make_word(address_lo, address_hi) + _y;

            if (page_crossing_penalty && hi(address) != address_hi)
            {
                ++_instruction_cycles_remaining;
            }

            break;
        }
    }

    uint8_t value = 0xcd;

    auto load_register = [this, &value](uint8_t& reg)
    {
        reg = lo(value);
        set_bit(_p, StatusBits::Z, reg == 0);
        set_bit(_p, StatusBits::N, get_bit(reg, 7));
    };

    auto decrement = [this](uint8_t& value)
    {
        value = value - 1;
        set_bit(_p, StatusBits::Z, value == 0);
        set_bit(_p, StatusBits::N, get_bit(value, 7));
    };

    auto increment = [this](uint8_t& value)
    {
        value = value + 1;
        set_bit(_p, StatusBits::Z, value == 0);
        set_bit(_p, StatusBits::N, get_bit(value, 7));
    };

    auto adc = [this, &value]()
    {
        uint16_t sum = _a + value + get_bit(_p, StatusBits::C);
        set_bit(_p, StatusBits::C, get_bit(sum, 8));
        set_bit(_p, StatusBits::V, (get_bit((_a ^ sum) & (value ^ sum), 7)));
        _a = lo(sum);
        set_bit(_p, StatusBits::Z, _a == 0);
        set_bit(_p, StatusBits::N, get_bit(_a, 7));
    };

    auto cmp = [this, &value, &address](uint8_t reg)
    {
        value = read(address);
        set_bit(_p, StatusBits::C, reg >= value);
        set_bit(_p, StatusBits::Z, reg == value);
        set_bit(_p, StatusBits::N, get_bit(reg - value, 7));
    };

    switch (instruction.opcode)
    {
        case Opcode::ADC:
        {
            value = read(address);
            adc();
            break;
        }
        case Opcode::AND:
        {
            value = _a & read(address);
            load_register(_a);
            break;
        }
        case Opcode::ASL:
        {
            if (instruction.addressing_mode == AddressingMode::Implied)
            {
                value = _a;
            }
            else
            {
                value = read(address);
            }

            set_bit(_p, StatusBits::C, get_bit(value, 7));
            value <<= 1;

            if (instruction.addressing_mode == AddressingMode::Implied)
            {
                load_register(_a);
            }
            else
            {
                set_bit(_p, StatusBits::Z, value == 0);
                set_bit(_p, StatusBits::N, get_bit(value, 7));
                write(address, value);
            }
            break;
        }
        case Opcode::BCC:
        {
            if (!get_bit(_p, StatusBits::C))
            {
                _instruction_cycles_remaining += (hi(address) == hi(_pc)) ? 1 : 2;
                _pc = address;
            }
            break;
        }
        case Opcode::BCS:
        {
            if (get_bit(_p, StatusBits::C))
            {
                _instruction_cycles_remaining += (hi(address) == hi(_pc)) ? 1 : 2;
                _pc = address;
            }
            break;
        }
        case Opcode::BEQ:
        {
            if (get_bit(_p, StatusBits::Z))
            {
                _instruction_cycles_remaining += (hi(address) == hi(_pc)) ? 1 : 2;
                _pc = address;
            }
            break;
        }
        case Opcode::BIT:
        {
            value = read(address);
            set_bit(_p, StatusBits::Z, (_a & value) == 0);
            set_bit(_p, StatusBits::V, get_bit(value, 6));
            set_bit(_p, StatusBits::N, get_bit(value, 7));
            break;
        }
        case Opcode::BMI:
        {
            if (get_bit(_p, StatusBits::N))
            {
                _instruction_cycles_remaining += (hi(address) == hi(_pc)) ? 1 : 2;
                _pc = address;
            }
            break;
        }
        case Opcode::BNE:
        {
            if (!get_bit(_p, StatusBits::Z))
            {
                _instruction_cycles_remaining += (hi(address) == hi(_pc)) ? 1 : 2;
                _pc = address;
            }
            break;
        }
        case Opcode::BPL:
        {
            if (!get_bit(_p, StatusBits::N))
            {
                _instruction_cycles_remaining += (hi(address) == hi(_pc)) ? 1 : 2;
                _pc = address;
            }
            break;
        }
        case Opcode::BRK:
        {
            read(_pc++);
            push(hi(_pc));
            push(lo(_pc));
            value = _p;
            set_bit(value, StatusBits::B, 1);
            set_bit(value, StatusBits::X, 1);
            push(_p);
            _pc = read_word(0xFFFE);
            _stopped = true;
            break;
        }
        case Opcode::BVC:
        {
            if (!get_bit(_p, StatusBits::V))
            {
                _instruction_cycles_remaining += (hi(address) == hi(_pc)) ? 1 : 2;
                _pc = address;
            }
            break;
        }
        case Opcode::BVS:
        {
            if (get_bit(_p, StatusBits::V))
            {
                _instruction_cycles_remaining += (hi(address) == hi(_pc)) ? 1 : 2;
                _pc = address;
            }
            break;
        }
        case Opcode::CLC:
        {
            set_bit(_p, StatusBits::C, 0);
            break;
        }
        case Opcode::CLD:
        {
            set_bit(_p, StatusBits::D, 0);
            break;
        }
        case Opcode::CLI:
        {
            set_bit(_p, StatusBits::I, 0);
            break;
        }
        case Opcode::CLV:
        {
            set_bit(_p, StatusBits::V, 0);
            break;
        }
        case Opcode::CMP:
        {
            cmp(_a);
            break;
        }
        case Opcode::CPX:
        {
            cmp(_x);
            break;
        }
        case Opcode::CPY:
        {
            cmp(_y);
            break;
        }
        case Opcode::DEC:
        {
            value = read(address);
            decrement(value);
            write(address, value);
            break;
        }
        case Opcode::DEX:
        {
            decrement(_x);
            break;
        }
        case Opcode::DEY:
        {
            decrement(_y);
            break;
        }
        case Opcode::EOR:
        {
            value = read(address);
            value = _a ^ value;
            load_register(_a);
            break;
        }
        case Opcode::INC:
        {
            value = read(address);
            increment(value);
            write(address, value);
            break;
        }
        case Opcode::INX:
        {
            increment(_x);
            break;
        }
        case Opcode::INY:
        {
            increment(_y);
            break;
        }
        case Opcode::JMP:
        {
            _pc = address;
            break;
        }
        case Opcode::JSR:
        {
            push(hi(_pc - 1));
            push(lo(_pc - 1));
            _pc = address;
            break;
        }
        case Opcode::LDA:
        {
            value = read(address);
            load_register(_a);
            break;
        }
        case Opcode::LDX:
        {
            value = read(address);
            load_register(_x);
            break;
        }
        case Opcode::LDY:
        {
            value = read(address);
            load_register(_y);
            break;
        }
        case Opcode::LSR:
        {
            if (instruction.addressing_mode == AddressingMode::Implied)
            {
                value = _a;
            }
            else
            {
                value = read(address);
            }

            set_bit(_p, StatusBits::C, get_bit(value, 0));
            value >>= 1;
            set_bit(_p, StatusBits::Z, value == 0);
            set_bit(_p, StatusBits::N, get_bit(value, 7));

            if (instruction.addressing_mode == AddressingMode::Implied)
            {
                _a = value;
            }
            else
            {
                write(address, value);
            }

            break;
        }
        case Opcode::NOP:
        {
            break;
        }
        case Opcode::ORA:
        {
            value = read(address);
            value = _a | value;
            load_register(_a);
            break;
        }
        case Opcode::PHA:
        {
            push(_a);
            break;
        }
        case Opcode::PHP:
        {
            value = _p;
            set_bit(value, StatusBits::B, 1);
            set_bit(value, StatusBits::X, 1);
            push(value);
            break;
        }
        case Opcode::PLA:
        {
            value = pop();
            load_register(_a);
            break;
        }
        case Opcode::PLP:
        {
            value = pop();
            set_bit(value, StatusBits::B, 0);
            set_bit(value, StatusBits::X, 0);
            _p = value;
            break;
        }
        case Opcode::ROL:
        {
            if (instruction.addressing_mode == AddressingMode::Implied)
            {
                value = _a;
            }
            else
            {
                value = read(address);
            }

            int c = get_bit(value, 7);
            value <<= 1;
            set_bit(value, 0, get_bit(_p, StatusBits::C));
            set_bit(_p, StatusBits::C, c);
            set_bit(_p, StatusBits::Z, value == 0);
            set_bit(_p, StatusBits::N, get_bit(value, 7));

            if (instruction.addressing_mode == AddressingMode::Implied)
            {
                _a = value;
            }
            else
            {
                write(address, value);
            }

            break;
        }
        case Opcode::ROR:
        {
            if (instruction.addressing_mode == AddressingMode::Implied)
            {
                value = _a;
            }
            else
            {
                value = read(address);
            }

            int c = get_bit(value, 0);
            value >>= 1;
            set_bit(value, 7, get_bit(_p, StatusBits::C));
            set_bit(_p, StatusBits::C, c);
            set_bit(_p, StatusBits::Z, value == 0);
            set_bit(_p, StatusBits::N, get_bit(value, 7));

            if (instruction.addressing_mode == AddressingMode::Implied)
            {
                _a = value;
            }
            else
            {
                write(address, value);
            }

            break;
        }
        case Opcode::RTI:
        {
            value = pop();
            set_bit(value, StatusBits::B, 0);
            set_bit(value, StatusBits::X, 0);
            _p = value;
            uint8_t lo = pop();
            uint8_t hi = pop();
            _pc = make_word(lo, hi);
            break;
        }
        case Opcode::RTS:
        {
            uint8_t lo = pop();
            uint8_t hi = pop();
            _pc = make_word(lo, hi) + 1;
            break;
        }
        case Opcode::SBC:
        {
            value = read(address) ^ 0xFF;
            adc();
            break;
        }
        case Opcode::SEC:
        {
            set_bit(_p, StatusBits::C, 1);
            break;
        }
        case Opcode::SED:
        {
            set_bit(_p, StatusBits::D, 1);
            break;
        }
        case Opcode::SEI:
        {
            set_bit(_p, StatusBits::I, 1);
            break;
        }
        case Opcode::STA:
        {
            write(address, _a);
            break;
        }
        case Opcode::STX:
        {
            write(address, _x);
            break;
        }
        case Opcode::STY:
        {
            write(address, _y);
            break;
        }
        case Opcode::TAX:
        {
            value = _a;
            load_register(_x);
            break;
        }
        case Opcode::TAY:
        {
            value = _a;
            load_register(_y);
            break;
        }
        case Opcode::TSX:
        {
            value = _s;
            load_register(_x);
            break;
        }
        case Opcode::TXA:
        {
            value = _x;
            load_register(_a);
            break;
        }
        case Opcode::TXS:
        {
            _s = _x;
            break;
        }
        case Opcode::TYA:
        {
            value = _y;
            load_register(_a);
            break;
        }

        // Unofficial opcodes
        case Opcode::DCP:
        {
            value = read(address);
            decrement(value);
            write(address, value);
            set_bit(_p, StatusBits::C, _a >= value);
            set_bit(_p, StatusBits::Z, _a == value);
            set_bit(_p, StatusBits::N, get_bit(_a - value, 7));
            break;
        }
        case Opcode::ISC:
        {
            value = read(address);
            increment(value);
            write(address, value);
            value ^= 0xFF;
            adc();
            break;
        }
        case Opcode::LAX:
        {
            value = read(address);
            load_register(_a);
            load_register(_x);
            break;
        }
        case Opcode::RLA:
        {
            value = read(address);
            int c = get_bit(value, 7);
            value <<= 1;
            set_bit(value, 0, get_bit(_p, StatusBits::C));
            set_bit(_p, StatusBits::C, c);
            set_bit(_p, StatusBits::Z, value == 0);
            set_bit(_p, StatusBits::N, get_bit(value, 7));
            write(address, value);
            value = _a & value;
            load_register(_a);
            break;
        }
        case Opcode::RRA:
        {
            value = read(address);
            int c = get_bit(value, 0);
            value >>= 1;
            set_bit(value, 7, get_bit(_p, StatusBits::C));
            set_bit(_p, StatusBits::C, c);
            set_bit(_p, StatusBits::Z, value == 0);
            set_bit(_p, StatusBits::N, get_bit(value, 7));
            write(address, value);
            adc();
            break;
        }
        case Opcode::SAX:
        {
            value = _a & _x;
            write(address, value);
            break;
        }
        case Opcode::SLO:
        {
            value = read(address);
            set_bit(_p, StatusBits::C, get_bit(value, 7));
            value <<= 1;
            set_bit(_p, StatusBits::Z, _a == 0);
            set_bit(_p, StatusBits::N, get_bit(value, 7));
            write(address, value);
            value = _a | value;
            load_register(_a);
            break;
        }
        case Opcode::SRE:
        {
            value = read(address);
            set_bit(_p, StatusBits::C, get_bit(value, 0));
            value >>= 1;
            set_bit(_p, StatusBits::Z, value == 0);
            set_bit(_p, StatusBits::N, get_bit(value, 7));
            write(address, value);
            value = _a ^ value;
            load_register(_a);
            break;
        }
        default:
        {
            // illegal instruction, treat as nop
            //_stopped = true;
            break;
        }
    }
}


std::string gli2A03::disassemble(uint16_t addr)
{
    uint8_t opcode = read(addr);

    if (opcode > sizeof(InstructionTable) / sizeof(InstructionTable[0]))
    {
        return "???";
    }

    const Instruction& instruction = InstructionTable[opcode];

    const char* format = "";
    uint8_t opbytes[2]{};
    uint8_t len;
    uint16_t operand;

    switch (instruction.addressing_mode)
    {
        case Implied:       // 1 byte
        {
            format = "%02X        %s";
            len = 1;
            break;
        }
        case Immediate:     // 2 bytes
        {
            format = "%02X %02X     %s #$%02X";
            opbytes[0] = read(addr + 1);
            operand = opbytes[0];
            len = 2;
            break;
        }
        case ZeroPage:      // 2 bytes
        {
            format = "%02X %02X     %s $%02X";
            opbytes[0] = read(addr + 1);
            operand = opbytes[0];
            len = 2;
            break;
        }
        case ZeroPage_X:    // 2 bytes
        {
            format = "%02X %02X     %s $%02x,X";
            opbytes[0] = read(addr + 1);
            operand = opbytes[0];
            len = 2;
            break;
        }
        case ZeroPage_Y:    // 2 bytes
        {
            format = "%02X %02X     %s $%02X,Y";
            opbytes[0] = read(addr + 1);
            operand = opbytes[0];
            len = 2;
            break;
        }
        case Relative:      // 2 bytes
        {
            format = "%02X %02X     %s $%04X";
            opbytes[0] = read(addr + 1);
            operand = opbytes[0] + (int16_t)(addr + 2);
            len = 2;
            break;
        }
        case Absolute:      // 3 bytes
        {
            format = "%02X %02X %02X  %s $%04X";
            opbytes[0] = read(addr + 1);
            opbytes[1] = read(addr + 2);
            operand = make_word(opbytes[0], opbytes[1]);
            len = 3;
            break;
        }
        case Absolute_X:      // 3 bytes
        {
            format = "%02X %02X %02X  %s $%04X,X";
            opbytes[0] = read(addr + 1);
            opbytes[1] = read(addr + 2);
            operand = make_word(opbytes[0], opbytes[1]);
            len = 3;
            break;
        }
        case Absolute_Y:      // 3 bytes
        {
            format = "%02X %02X %02X  %s $%04X,Y";
            opbytes[0] = read(addr + 1);
            opbytes[1] = read(addr + 2);
            operand = make_word(opbytes[0], opbytes[1]);
            len = 3;
            break;
        }
        case Indirect:      // (Indirect) 3 bytes
        {
            format = "%02X %02X %02X  %s ($%04X)";
            opbytes[0] = read(addr + 1);
            opbytes[1] = read(addr + 2);
            operand = make_word(opbytes[0], opbytes[1]);
            len = 3;
            break;
        }
        case Indirect_X:      // (Indirect,X) 2 bytes
        {
            format = "%02X %02X     %s ($%02X,X)";
            opbytes[0] = read(addr + 1);
            operand = opbytes[0];
            len = 2;
            break;
        }
        case Indirect_Y:      // (Indirect),Y 2 bytes
        {
            format = "%02X %02X     %s ($%02X),Y";
            opbytes[0] = read(addr + 1);
            operand = opbytes[0];
            len = 2;
            break;
        }
    }

    char buffer[256];

    if (len == 1)
    {
        std::snprintf(buffer, 256, format, opcode, instruction.mnemonic);
    }
    else if (len == 2)
    {
        std::snprintf(buffer, 256, format, opcode, opbytes[0], instruction.mnemonic, operand);
    }
    else if (len == 3)
    {
        std::snprintf(buffer, 256, format, opcode, opbytes[0], opbytes[1], instruction.mnemonic, operand);
    }

    return std::string(buffer);
}
