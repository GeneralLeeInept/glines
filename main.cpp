#include "vgfw.h"

#include <algorithm>

#include "vga9.h"

enum AddressingMode : uint8_t
{
    Implied,        // 1 byte
    Immediate,      // 2 bytes
    ZeroPage,       // 2 bytes
    ZeroPage_X,     // 2 bytes
    ZeroPage_Y,     // 2 bytes
    Relative,       // 2 bytes
    Absolute,       // 3 bytes
    Absolute_X,     // 3 bytes
    Absolute_Y,     // 3 bytes
    Indirect,       // (Indirect) 3 bytes
    Indirect_X,     // (Indirect,X) 2 bytes
    Indirect_Y      // (Indirect),Y 2 bytes
};

enum Opcode : uint8_t
{
    // Official
    ADC,
    AND,
    ASL,
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
    DEC,
    DEX,
    DEY,
    EOR,
    INC,
    INX,
    INY,
    JMP,
    JSR,
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
    ROL,
    ROR,
    RTI,
    RTS,
    SBC,
    SEC,
    SED,
    SEI,
    STA,
    STX,
    STY,
    TAX,
    TAY,
    TSX,
    TXA,
    TXS,
    TYA,

    // Unofficial
    STP,    // aka HLT
    SLO,    // aka ASO
    RLA,
    SRE,    // aka LSE
    RRA,
    AXS,
    LAX,
    DCP,    // aka DCM
    ISC,    // aka INS
    ALR,
    ARR,
    XAA,
    SAX,
    TAS,
    SHY,    // aka SAY
    SHX,    // aka XAS
    AHX,    // aka AXA
    ANC,
    LAS
};

struct Instruction
{
    Opcode opcode;
    const char* mnemonic;
    AddressingMode addressing_mode;
    int cycles;
};

static const Instruction InstructionTable[] =
{
    { Opcode::BRK,  "BRK", AddressingMode::Implied,       7 },    // 00
    { Opcode::ORA,  "ORA", AddressingMode::Indirect_X,    4 },    // 01
    { Opcode::STP,  "STP", AddressingMode::Implied,       0 },    // 02
    { Opcode::SLO,  "SLO", AddressingMode::Indirect_X,    0 },    // 03
    { Opcode::NOP,  "NOP", AddressingMode::ZeroPage,      0 },    // 04
    { Opcode::ORA,  "ORA", AddressingMode::ZeroPage,      0 },    // 05
    { Opcode::ASL,  "ASL", AddressingMode::ZeroPage,      0 },    // 06
    { Opcode::SLO,  "SLO", AddressingMode::ZeroPage,      0 },    // 07
    { Opcode::PHP,  "PHP", AddressingMode::Implied,       0 },    // 08
    { Opcode::ORA,  "ORA", AddressingMode::Immediate,     0 },    // 09
    { Opcode::ASL,  "ASL", AddressingMode::Implied,       0 },    // 0A
    { Opcode::ANC,  "ANC", AddressingMode::Immediate,     0 },    // 0B
    { Opcode::NOP,  "NOP", AddressingMode::Absolute,      0 },    // 0C
    { Opcode::ORA,  "ORA", AddressingMode::Absolute,      0 },    // 0D
    { Opcode::ASL,  "ASL", AddressingMode::Absolute,      0 },    // 0E
    { Opcode::SLO,  "SLO", AddressingMode::Absolute,      0 },    // 0F
    { Opcode::BPL,  "BPL", AddressingMode::Relative,      0 },    // 10
    { Opcode::ORA,  "ORA", AddressingMode::Indirect_Y,    0 },    // 11
    { Opcode::STP,  "STP", AddressingMode::Implied,       0 },    // 12
    { Opcode::SLO,  "SLO", AddressingMode::Indirect_Y,    0 },    // 13
    { Opcode::NOP,  "NOP", AddressingMode::ZeroPage_X,    0 },    // 14
    { Opcode::ORA,  "ORA", AddressingMode::ZeroPage_X,    0 },    // 15
    { Opcode::ASL,  "ASL", AddressingMode::ZeroPage_X,    0 },    // 16
    { Opcode::SLO,  "SLO", AddressingMode::ZeroPage_X,    0 },    // 17
    { Opcode::CLC,  "CLC", AddressingMode::Implied,       0 },    // 18
    { Opcode::ORA,  "ORA", AddressingMode::ZeroPage_Y,    0 },    // 19
    { Opcode::NOP,  "NOP", AddressingMode::Implied,       0 },    // 1A
    { Opcode::SLO,  "SLO", AddressingMode::Absolute_Y,    0 },    // 1B
    { Opcode::NOP,  "NOP", AddressingMode::Absolute_X,    0 },    // 1C
    { Opcode::ORA,  "ORA", AddressingMode::Absolute_X,    0 },    // 1D
    { Opcode::ASL,  "ASL", AddressingMode::Absolute_X,    0 },    // 1E
    { Opcode::SLO,  "SLO", AddressingMode::Absolute_X,    0 },    // 1F

    { Opcode::JSR,  "JSR", AddressingMode::Absolute,      0 },    // 20
    { Opcode::AND,  "AND", AddressingMode::Indirect_X,    0 },    // 21
    { Opcode::STP,  "STP", AddressingMode::Implied,       0 },    // 22
    { Opcode::RLA,  "RLA", AddressingMode::Indirect_X,    0 },    // 23
    { Opcode::BIT,  "BIT", AddressingMode::ZeroPage,      0 },    // 24
    { Opcode::AND,  "AND", AddressingMode::ZeroPage,      0 },    // 25
    { Opcode::ROL,  "ROL", AddressingMode::ZeroPage,      0 },    // 26
    { Opcode::RLA,  "RLA", AddressingMode::ZeroPage,      0 },    // 27
    { Opcode::PLP,  "PLP", AddressingMode::Implied,       0 },    // 28
    { Opcode::AND,  "AND", AddressingMode::Immediate,     0 },    // 29
    { Opcode::ROL,  "ROL", AddressingMode::Implied,       0 },    // 2A
    { Opcode::ANC,  "ANC", AddressingMode::Immediate,     0 },    // 2B
    { Opcode::BIT,  "BIT", AddressingMode::Absolute,      0 },    // 2C
    { Opcode::AND,  "AND", AddressingMode::Absolute,      0 },    // 2D
    { Opcode::ROL,  "ROL", AddressingMode::Absolute,      0 },    // 2E
    { Opcode::RLA,  "RLA", AddressingMode::Absolute,      0 },    // 2F
    { Opcode::BMI,  "BMI", AddressingMode::Relative,      0 },    // 30
    { Opcode::AND,  "AND", AddressingMode::Indirect_Y,    0 },    // 31
    { Opcode::STP,  "STP", AddressingMode::Implied,       0 },    // 32
    { Opcode::RLA,  "RLA", AddressingMode::Indirect_Y,    0 },    // 33
    { Opcode::NOP,  "NOP", AddressingMode::ZeroPage_X,    0 },    // 34
    { Opcode::AND,  "AND", AddressingMode::ZeroPage_X,    0 },    // 35
    { Opcode::ROL,  "ROL", AddressingMode::ZeroPage_X,    0 },    // 36
    { Opcode::RLA,  "RLA", AddressingMode::ZeroPage_X,    0 },    // 37
    { Opcode::SEC,  "SEC", AddressingMode::Implied,       0 },    // 38
    { Opcode::AND,  "AND", AddressingMode::Absolute_Y,    0 },    // 39
    { Opcode::NOP,  "NOP", AddressingMode::Implied,       0 },    // 3A
    { Opcode::RLA,  "RLA", AddressingMode::Absolute_Y,    0 },    // 3B
    { Opcode::NOP,  "NOP", AddressingMode::Absolute_X,    0 },    // 3C
    { Opcode::AND,  "AND", AddressingMode::Absolute_X,    0 },    // 3D
    { Opcode::ROL,  "ROL", AddressingMode::Absolute_X,    0 },    // 3E
    { Opcode::RLA,  "RLA", AddressingMode::Absolute_X,    0 },    // 3F

    { Opcode::RTI,  "RTI", AddressingMode::Implied,       0 },    // 40
    { Opcode::EOR,  "EOR", AddressingMode::Indirect_X,    0 },    // 41
    { Opcode::STP,  "STP", AddressingMode::Implied,       0 },    // 42
    { Opcode::SRE,  "SRE", AddressingMode::Indirect_X,    0 },    // 43
    { Opcode::NOP,  "NOP", AddressingMode::ZeroPage,      0 },    // 44
    { Opcode::EOR,  "EOR", AddressingMode::ZeroPage,      0 },    // 45
    { Opcode::LSR,  "LSR", AddressingMode::ZeroPage,      0 },    // 46
    { Opcode::SRE,  "SRE", AddressingMode::ZeroPage,      0 },    // 47
    { Opcode::PHA,  "PHA", AddressingMode::Implied,       0 },    // 48
    { Opcode::EOR,  "EOR", AddressingMode::Immediate,     0 },    // 49
    { Opcode::LSR,  "LSR", AddressingMode::Implied,       0 },    // 4A
    { Opcode::ALR,  "ALR", AddressingMode::Immediate,     0 },    // 4B
    { Opcode::JMP,  "JMP", AddressingMode::Absolute,      0 },    // 4C
    { Opcode::EOR,  "EOR", AddressingMode::Absolute,      0 },    // 4D
    { Opcode::LSR,  "LSR", AddressingMode::Absolute,      0 },    // 4E
    { Opcode::SRE,  "SRE", AddressingMode::Absolute,      0 },    // 4F
    { Opcode::BVC,  "BVC", AddressingMode::Relative,      0 },    // 50
    { Opcode::EOR,  "EOR", AddressingMode::Indirect_Y,    0 },    // 51
    { Opcode::STP,  "STP", AddressingMode::Implied,       0 },    // 52
    { Opcode::SRE,  "SRE", AddressingMode::Indirect_Y,    0 },    // 53
    { Opcode::NOP,  "NOP", AddressingMode::ZeroPage_X,    0 },    // 54
    { Opcode::EOR,  "EOR", AddressingMode::ZeroPage_X,    0 },    // 55
    { Opcode::LSR,  "LSR", AddressingMode::ZeroPage_X,    0 },    // 56
    { Opcode::SRE,  "SRE", AddressingMode::ZeroPage_X,    0 },    // 57
    { Opcode::CLI,  "CLI", AddressingMode::Implied,       0 },    // 58
    { Opcode::EOR,  "EOR", AddressingMode::Absolute_Y,    0 },    // 59
    { Opcode::NOP,  "NOP", AddressingMode::Implied,       0 },    // 5A
    { Opcode::SRE,  "SRE", AddressingMode::Absolute_Y,    0 },    // 5B
    { Opcode::NOP,  "NOP", AddressingMode::Absolute_X,    0 },    // 5C
    { Opcode::AND,  "AND", AddressingMode::Absolute_X,    0 },    // 5D
    { Opcode::ROL,  "ROL", AddressingMode::Absolute_X,    0 },    // 5E
    { Opcode::RLA,  "RLA", AddressingMode::Absolute_X,    0 },    // 5F

    { Opcode::RTS,  "RTS", AddressingMode::Implied,       0 },    // 60
    { Opcode::ADC,  "ADC", AddressingMode::Indirect_X,    0 },    // 61
    { Opcode::STP,  "STP", AddressingMode::Implied,       0 },    // 62
    { Opcode::RRA,  "SRE", AddressingMode::Indirect_X,    0 },    // 63
    { Opcode::NOP,  "NOP", AddressingMode::ZeroPage,      0 },    // 64
    { Opcode::ADC,  "ADC", AddressingMode::ZeroPage,      0 },    // 65
    { Opcode::ROR,  "ROR", AddressingMode::ZeroPage,      0 },    // 66
    { Opcode::RRA,  "RRA", AddressingMode::ZeroPage,      0 },    // 67
    { Opcode::PLA,  "PLA", AddressingMode::Implied,       0 },    // 68
    { Opcode::ADC,  "ADC", AddressingMode::Immediate,     0 },    // 69
    { Opcode::ROR,  "ROR", AddressingMode::Implied,       0 },    // 6A
    { Opcode::ARR,  "ARR", AddressingMode::Immediate,     0 },    // 6B
    { Opcode::JMP,  "JMP", AddressingMode::Indirect,      0 },    // 6C
    { Opcode::ADC,  "ADC", AddressingMode::Absolute,      0 },    // 6D
    { Opcode::ROR,  "ROR", AddressingMode::Absolute,      0 },    // 6E
    { Opcode::RRA,  "RRA", AddressingMode::Absolute,      0 },    // 6F
    { Opcode::BVS,  "BVS", AddressingMode::Relative,      0 },    // 70
    { Opcode::ADC,  "ADC", AddressingMode::Indirect_Y,    0 },    // 71
    { Opcode::STP,  "STP", AddressingMode::Implied,       0 },    // 72
    { Opcode::RRA,  "RRA", AddressingMode::Indirect_Y,    0 },    // 73
    { Opcode::NOP,  "NOP", AddressingMode::ZeroPage_X,    0 },    // 74
    { Opcode::ADC,  "ADC", AddressingMode::ZeroPage_X,    0 },    // 75
    { Opcode::ROR,  "ROR", AddressingMode::ZeroPage_X,    0 },    // 76
    { Opcode::RRA,  "RRA", AddressingMode::ZeroPage_X,    0 },    // 77
    { Opcode::SEI,  "SEI", AddressingMode::Implied,       0 },    // 78
    { Opcode::ADC,  "ADC", AddressingMode::Absolute_Y,    0 },    // 79
    { Opcode::NOP,  "NOP", AddressingMode::Implied,       0 },    // 7A
    { Opcode::RRA,  "RRA", AddressingMode::Absolute_Y,    0 },    // 7B
    { Opcode::NOP,  "NOP", AddressingMode::Absolute_X,    0 },    // 7C
    { Opcode::ADC,  "AND", AddressingMode::Absolute_X,    0 },    // 7D
    { Opcode::ROR,  "ROL", AddressingMode::Absolute_X,    0 },    // 7E
    { Opcode::RRA,  "RRA", AddressingMode::Absolute_X,    0 },    // 7F

    { Opcode::NOP,  "NOP", AddressingMode::Immediate,     0 },    // 80
    { Opcode::STA,  "STA", AddressingMode::Indirect_X,    0 },    // 81
    { Opcode::NOP,  "NOP", AddressingMode::Immediate,     0 },    // 82
    { Opcode::SAX,  "SAX", AddressingMode::Indirect_X,    0 },    // 83
    { Opcode::STY,  "STY", AddressingMode::ZeroPage,      0 },    // 84
    { Opcode::STA,  "STA", AddressingMode::ZeroPage,      0 },    // 85
    { Opcode::STX,  "STX", AddressingMode::ZeroPage,      0 },    // 86
    { Opcode::SAX,  "SAX", AddressingMode::ZeroPage,      0 },    // 87
    { Opcode::DEY,  "DEY", AddressingMode::Implied,       0 },    // 88
    { Opcode::NOP,  "NOP", AddressingMode::Immediate,     0 },    // 89
    { Opcode::TXA,  "TXA", AddressingMode::Implied,       0 },    // 8A
    { Opcode::XAA,  "XAA", AddressingMode::Immediate,     0 },    // 8B
    { Opcode::STY,  "STY", AddressingMode::Absolute,      0 },    // 8C
    { Opcode::STA,  "STA", AddressingMode::Absolute,      0 },    // 8D
    { Opcode::STX,  "STX", AddressingMode::Absolute,      0 },    // 8E
    { Opcode::SAX,  "SAX", AddressingMode::Absolute,      0 },    // 8F
    { Opcode::BCC,  "BCC", AddressingMode::Relative,      0 },    // 90
    { Opcode::STA,  "STA", AddressingMode::Indirect_Y,    0 },    // 91
    { Opcode::STP,  "STP", AddressingMode::Implied,       0 },    // 92
    { Opcode::AHX,  "AHX", AddressingMode::Indirect_Y,    0 },    // 93
    { Opcode::STY,  "STY", AddressingMode::ZeroPage_X,    0 },    // 94
    { Opcode::STA,  "STA", AddressingMode::ZeroPage_X,    0 },    // 95
    { Opcode::STX,  "STX", AddressingMode::ZeroPage_X,    0 },    // 96
    { Opcode::SAX,  "SAX", AddressingMode::ZeroPage_X,    0 },    // 97
    { Opcode::TYA,  "TYA", AddressingMode::Implied,       0 },    // 98
    { Opcode::STA,  "STA", AddressingMode::Absolute_Y,    0 },    // 99
    { Opcode::TXS,  "TXS", AddressingMode::Implied,       0 },    // 9A
    { Opcode::TAS,  "TAS", AddressingMode::Absolute_Y,    0 },    // 9B
    { Opcode::SHY,  "SHY", AddressingMode::Absolute_X,    0 },    // 9C
    { Opcode::STA,  "STA", AddressingMode::Absolute_X,    0 },    // 9D
    { Opcode::SHX,  "SHX", AddressingMode::Absolute_Y,    0 },    // 9E
    { Opcode::AHX,  "AHX", AddressingMode::Absolute_Y,    0 },    // 9F

    { Opcode::LDY,  "LDY", AddressingMode::Immediate,     0 },    // A0
    { Opcode::LDA,  "LDA", AddressingMode::Indirect_X,    0 },    // A1
    { Opcode::LDX,  "LDX", AddressingMode::Immediate,     0 },    // A2
    { Opcode::LAX,  "LAX", AddressingMode::Indirect_X,    0 },    // A3
    { Opcode::LDY,  "LDY", AddressingMode::ZeroPage,      0 },    // A4
    { Opcode::LDA,  "LDA", AddressingMode::ZeroPage,      0 },    // A5
    { Opcode::LDX,  "LDX", AddressingMode::ZeroPage,      0 },    // A6
    { Opcode::LAX,  "LAX", AddressingMode::ZeroPage,      0 },    // A7
    { Opcode::TAY,  "TAY", AddressingMode::Implied,       0 },    // A8
    { Opcode::LDA,  "LDA", AddressingMode::Immediate,     0 },    // A9
    { Opcode::TAX,  "TAX", AddressingMode::Implied,       0 },    // AA
    { Opcode::LAX,  "LAX", AddressingMode::Immediate,     0 },    // AB
    { Opcode::LDY,  "LDY", AddressingMode::Absolute,      0 },    // AC
    { Opcode::LDA,  "LDA", AddressingMode::Absolute,      0 },    // AD
    { Opcode::LDX,  "LDX", AddressingMode::Absolute,      0 },    // AE
    { Opcode::LAX,  "LAX", AddressingMode::Absolute,      0 },    // AF
    { Opcode::BCS,  "BCS", AddressingMode::Relative,      0 },    // B0
    { Opcode::LDA,  "LDA", AddressingMode::Indirect_Y,    0 },    // B1
    { Opcode::STP,  "STP", AddressingMode::Implied,       0 },    // B2
    { Opcode::LAX,  "LAX", AddressingMode::Indirect_Y,    0 },    // B3
    { Opcode::LDY,  "LDY", AddressingMode::ZeroPage_X,    0 },    // B4
    { Opcode::LDA,  "LDA", AddressingMode::ZeroPage_X,    0 },    // B5
    { Opcode::LDX,  "LDX", AddressingMode::ZeroPage_X,    0 },    // B6
    { Opcode::LAX,  "LAX", AddressingMode::ZeroPage_X,    0 },    // B7
    { Opcode::CLV,  "CLV", AddressingMode::Implied,       0 },    // B8
    { Opcode::LDA,  "LDA", AddressingMode::Absolute_Y,    0 },    // B9
    { Opcode::TSX,  "TSX", AddressingMode::Implied,       0 },    // BA
    { Opcode::LAS,  "LAS", AddressingMode::Absolute_Y,    0 },    // BB
    { Opcode::LDY,  "LDY", AddressingMode::Absolute_X,    0 },    // BC
    { Opcode::LDA,  "LDA", AddressingMode::Absolute_X,    0 },    // BD
    { Opcode::LDX,  "LDX", AddressingMode::Absolute_Y,    0 },    // BE
    { Opcode::LAX,  "LAX", AddressingMode::Absolute_Y,    0 },    // BF

    { Opcode::CPY,  "CPY", AddressingMode::Immediate,     0 },    // C0
    { Opcode::CMP,  "CMP", AddressingMode::Indirect_X,    0 },    // C1
    { Opcode::NOP,  "NOP", AddressingMode::Immediate,     0 },    // C2
    { Opcode::DCP,  "DCP", AddressingMode::Indirect_X,    0 },    // C3
    { Opcode::CPY,  "CPY", AddressingMode::ZeroPage,      0 },    // C4
    { Opcode::CMP,  "CMP", AddressingMode::ZeroPage,      0 },    // C5
    { Opcode::DEC,  "DEC", AddressingMode::ZeroPage,      0 },    // C6
    { Opcode::DCP,  "DCP", AddressingMode::ZeroPage,      0 },    // C7
    { Opcode::INY,  "INY", AddressingMode::Implied,       0 },    // C8
    { Opcode::CMP,  "CMP", AddressingMode::Immediate,     0 },    // C9
    { Opcode::DEX,  "DEX", AddressingMode::Implied,       0 },    // CA
    { Opcode::AXS,  "AXS", AddressingMode::Immediate,     0 },    // CB
    { Opcode::CPY,  "CPY", AddressingMode::Absolute,      0 },    // CC
    { Opcode::CMP,  "CMP", AddressingMode::Absolute,      0 },    // CD
    { Opcode::DEC,  "DEC", AddressingMode::Absolute,      0 },    // CE
    { Opcode::DCP,  "DCP", AddressingMode::Absolute,      0 },    // CF
    { Opcode::BNE,  "BNE", AddressingMode::Relative,      0 },    // D0
    { Opcode::CMP,  "CMP", AddressingMode::Indirect_Y,    0 },    // D1
    { Opcode::STP,  "STP", AddressingMode::Implied,       0 },    // D2
    { Opcode::DCP,  "DCP", AddressingMode::Indirect_Y,    0 },    // D3
    { Opcode::NOP,  "NOP", AddressingMode::ZeroPage_X,    0 },    // D4
    { Opcode::CMP,  "CMP", AddressingMode::ZeroPage_X,    0 },    // D5
    { Opcode::DEC,  "DEC", AddressingMode::ZeroPage_X,    0 },    // D6
    { Opcode::DCP,  "DCP", AddressingMode::ZeroPage_X,    0 },    // D7
    { Opcode::CLD,  "CLD", AddressingMode::Implied,       0 },    // D8
    { Opcode::CMP,  "CMP", AddressingMode::Absolute_Y,    0 },    // D9
    { Opcode::NOP,  "NOP", AddressingMode::Implied,       0 },    // DA
    { Opcode::DCP,  "DCP", AddressingMode::Absolute_Y,    0 },    // DB
    { Opcode::NOP,  "NOP", AddressingMode::Absolute_X,    0 },    // DC
    { Opcode::CMP,  "CMP", AddressingMode::Absolute_X,    0 },    // DD
    { Opcode::DEC,  "DEC", AddressingMode::Absolute_X,    0 },    // DE
    { Opcode::DCP,  "DCP", AddressingMode::Absolute_X,    0 },    // DF

    { Opcode::CPX,  "CPX", AddressingMode::Immediate,     0 },    // E0
    { Opcode::SBC,  "SBC", AddressingMode::Indirect_X,    0 },    // E1
    { Opcode::NOP,  "NOP", AddressingMode::Immediate,     0 },    // E2
    { Opcode::ISC,  "ISC", AddressingMode::Indirect_X,    0 },    // E3
    { Opcode::CPX,  "CPX", AddressingMode::ZeroPage,      0 },    // E4
    { Opcode::SBC,  "SBC", AddressingMode::ZeroPage,      0 },    // E5
    { Opcode::INC,  "INC", AddressingMode::ZeroPage,      0 },    // E6
    { Opcode::ISC,  "ISC", AddressingMode::ZeroPage,      0 },    // E7
    { Opcode::INX,  "INX", AddressingMode::Implied,       0 },    // E8
    { Opcode::SBC,  "SBC", AddressingMode::Immediate,     0 },    // E9
    { Opcode::NOP,  "NOP", AddressingMode::Implied,       0 },    // EA
    { Opcode::SBC,  "SBC", AddressingMode::Immediate,     0 },    // EB
    { Opcode::CPX,  "CPX", AddressingMode::Absolute,      0 },    // EC
    { Opcode::SBC,  "SBC", AddressingMode::Absolute,      0 },    // ED
    { Opcode::INC,  "INC", AddressingMode::Absolute,      0 },    // EE
    { Opcode::ISC,  "ISC", AddressingMode::Absolute,      0 },    // EF
    { Opcode::BEQ,  "BEQ", AddressingMode::Relative,      0 },    // F0
    { Opcode::SBC,  "SBC", AddressingMode::Indirect_Y,    0 },    // F1
    { Opcode::STP,  "STP", AddressingMode::Implied,       0 },    // F2
    { Opcode::ISC,  "ISC", AddressingMode::Indirect_Y,    0 },    // F3
    { Opcode::NOP,  "NOP", AddressingMode::ZeroPage_X,    0 },    // F4
    { Opcode::SBC,  "SBC", AddressingMode::ZeroPage_X,    0 },    // F5
    { Opcode::INC,  "INC", AddressingMode::ZeroPage_X,    0 },    // F6
    { Opcode::ISC,  "ISC", AddressingMode::ZeroPage_X,    0 },    // F7
    { Opcode::SED,  "SED", AddressingMode::Implied,       0 },    // F8
    { Opcode::SBC,  "SBC", AddressingMode::Absolute_Y,    0 },    // F9
    { Opcode::NOP,  "NOP", AddressingMode::Implied,       0 },    // FA
    { Opcode::ISC,  "ISC", AddressingMode::Absolute_Y,    0 },    // FB
    { Opcode::NOP,  "NOP", AddressingMode::Absolute_X,    0 },    // FC
    { Opcode::SBC,  "SBC", AddressingMode::Absolute_X,    0 },    // FD
    { Opcode::INC,  "INC", AddressingMode::Absolute_X,    0 },    // FE
    { Opcode::ISC,  "ISC", AddressingMode::Absolute_X,    0 },    // FF
};

enum StatusBits : uint8_t
{
    Carry = 0,
    Zero = 1,
    InterruptDisable = 2,
    Decimal = 3,
    Overflow = 6,
    Negative = 7
};

template <typename T>
void SetBit(T& flags, uint8_t bit, bool set)
{
    T mask = T(1) << bit;
    flags |= set ? mask : T(0);
    flags &= set ? ~T(0) : ~mask;
}

template <typename T>
bool TestBit(T& flags, uint8_t bit)
{
    T mask = T(1) << bit;
    return (flags & mask) == mask;
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

class GliNes : public Vgfw
{
public:
    static constexpr int DisplayWidth = 256;
    static constexpr int DisplayHeight = 240;

    static constexpr int InspectorWidth = 80 * 9;
    static constexpr int InspectorHeight = 50 * 16;

    static constexpr int WindowWidth = 16 + (DisplayWidth * 2) + 16 + InspectorWidth + 16;
    static constexpr int WindowHeight = 16 + std::max(DisplayHeight * 2, InspectorHeight) + 16;

    bool on_create() override
    {
        return true;
    }

    void on_destroy() override
    {

    }

    // RAM
    uint8_t _ram[64 * 1024];

    // Bus
    uint8_t read(uint16_t addr)
    {
        return _ram[addr];
    }

    uint16_t read_word(uint16_t addr)
    {
        return make_word(_ram[addr], _ram[addr + 1]);
    }

    void write(uint16_t addr, uint8_t val)
    {
        _ram[addr] = val;
    }

    void write_word(uint16_t addr, uint16_t val)
    {
        _ram[addr] = lo(val);
        _ram[addr + 1] = hi(val);
    }

    // CPU
    uint16_t _pc;
    uint8_t _p, _a, _x, _y, _s;
    bool _stopped;

    void reset(bool coldstart)
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
    }

    void push(uint8_t value)
    {
        write(_s, value);
        _s -= 1;
    }

    uint8_t pop()
    {
        _s += 1;
        return read(_s);
    }

    void exec(const Instruction& instruction)
    {
        uint16_t address = 0xd1ed;

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
                address = read_word(_pc + 1) + _x;
                _pc += 2;
                break;
            }
            case Absolute_Y:
            {
                address = read_word(_pc + 1) + _y;
                _pc += 2;
                break;
            }
            case Indirect:
            {
                address = read_word(_pc + 1);
                _pc += 2;
                break;
            }
            case Indirect_X:
            {
                uint16_t address = read(_pc++) + _x;
                address = read_word(address);
                break;
            }
            case Indirect_Y:
            {
                uint16_t address = read(_pc++);
                address = read_word(address) + _a;
                break;
            }
        }

        uint8_t value = 0xcd;

        auto load_register = [this, &value](uint8_t& reg)
        {
            reg = lo(value);
            SetBit(_p, StatusBits::Zero, reg == 0);
            SetBit(_p, StatusBits::Negative, TestBit(reg, 7));
        };

        switch (instruction.opcode)
        {
            case Opcode::AND:
            {
                value = _a & read(address);
                load_register(_a);
                break;
            }
            case Opcode::BCC:
            {
                if (!TestBit(_p, StatusBits::Carry))
                {
                    _pc = address;
                }
                break;
            }
            case Opcode::BCS:
            {
                if (TestBit(_p, StatusBits::Carry))
                {
                    _pc = address;
                }
                break;
            }
            case Opcode::BEQ:
            {
                if (TestBit(_p, StatusBits::Zero))
                {
                    _pc = address;
                }
                break;
            }
            case Opcode::BIT:
            {
                value = read(address);
                SetBit(_p, StatusBits::Zero, (_a & value) == 0);
                SetBit(_p, StatusBits::Overflow, TestBit(value, 6));
                SetBit(_p, StatusBits::Negative, TestBit(value, 7));
                break;
            }
            case Opcode::BMI:
            {
                if (TestBit(_p, StatusBits::Negative))
                {
                    _pc = address;
                }
                break;
            }
            case Opcode::BNE:
            {
                if (!TestBit(_p, StatusBits::Zero))
                {
                    _pc = address;
                }
                break;
            }
            case Opcode::BPL:
            {
                if (!TestBit(_p, StatusBits::Negative))
                {
                    _pc = address;
                }
                break;
            }
            case Opcode::BVC:
            {
                if (!TestBit(_p, StatusBits::Overflow))
                {
                    _pc = address;
                }
                break;
            }
            case Opcode::BVS:
            {
                if (TestBit(_p, StatusBits::Overflow))
                {
                    _pc = address;
                }
                break;
            }
            case Opcode::CLC:
            {
                SetBit(_p, StatusBits::Carry, false);
                break;
            }
            case Opcode::CLD:
            {
                SetBit(_p, StatusBits::Decimal, false);
                break;
            }
            case Opcode::CMP:
            {
                value = _a - read(address);
                SetBit(_p, StatusBits::Carry, value >= 0);
                SetBit(_p, StatusBits::Zero, value == 0);
                SetBit(_p, StatusBits::Negative, TestBit(value, 7));
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
            case Opcode::NOP:
            {
                break;
            }
            case Opcode::PHA:
            {
                push(_a);
                break;
            }
            case Opcode::PHP:
            {
                push(_p);
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
                _p = pop();
                break;
            }
            case Opcode::RTS:
            {
                uint8_t lo = pop();
                uint8_t hi = pop();
                _pc = make_word(lo, hi) + 1;
                break;
            }
            case Opcode::SEC:
            {
                SetBit(_p, StatusBits::Carry, true);
                break;
            }
            case Opcode::SED:
            {
                SetBit(_p, StatusBits::Decimal, true);
                break;
            }
            case Opcode::SEI:
            {
                SetBit(_p, StatusBits::InterruptDisable, true);
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
            default:
            {
                // illegal instruction, panic!
                _stopped = true;
                break;
            }
        }
    }

    void step()
    {
        uint8_t opcode = read(_pc++);

        if (_stopped)
        {
            return;
        }

        if (opcode > sizeof(InstructionTable) / sizeof(InstructionTable[0]))
        {
            _stopped = true;
        }
        else
        {
            exec(InstructionTable[opcode]);
        }
    }

    std::string disassemble(uint16_t addr)
    {
        uint8_t opcode = read(addr);

        if (opcode > sizeof(InstructionTable) / sizeof(InstructionTable[0]))
        {
            return "???";
        }

        const Instruction& instruction = InstructionTable[opcode];

        const char* format = "";
        // "xx yy zz   LDA   ($abcd),X
        uint16_t operand = 0;
        int opbytes = 0;

        switch (instruction.addressing_mode)
        {
            case Implied:       // 1 byte
            {
                format = "%02X         %s";
                break;
            }
            case Immediate:     // 2 bytes
            {
                format = "%02X %02X      %s   #$%02X";
                operand = read(addr + 1);
                opbytes = 1;
                break;
            }
            case ZeroPage:      // 2 bytes
            {
                format = "%02X %02X      %s   $%02X";
                operand = read(addr + 1);
                opbytes = 1;
                break;
            }
            case ZeroPage_X:    // 2 bytes
            {
                format = "%02X %02X      %s   $%02x,X";
                operand = read(addr + 1);
                opbytes = 1;
                break;
            }
            case ZeroPage_Y:    // 2 bytes
            {
                format = "%02X %02X      %s   $%02X,Y";
                operand = read(addr + 1);
                opbytes = 1;
                break;
            }
            case Relative:      // 2 bytes
            {
                format = "%02X %02X      %s   *+$%02X";
                operand = read(addr + 1);
                opbytes = 1;
                break;
            }
            case Absolute:      // 3 bytes
            {
                format = "%02X %02X %02X   %s   $%04X";
                operand = read_word(addr + 1);
                opbytes = 2;
                break;
            }
            case Absolute_X:      // 3 bytes
            {
                format = "%02X %02X %02X   %s   $%04X,X";
                operand = read_word(addr + 1);
                opbytes = 2;
                break;
            }
            case Absolute_Y:      // 3 bytes
            {
                format = "%02X %02X %02X   %s   $%04X,Y";
                operand = read_word(addr + 1);
                opbytes = 2;
                break;
            }
            case Indirect:      // (Indirect) 3 bytes
            {
                format = "%02X %02X %02X   %s   ($%04X)";
                operand = read_word(addr + 1);
                opbytes = 2;
                break;
            }
            case Indirect_X:      // (Indirect,X) 2 bytes
            {
                format = "%02X %02X      %s   ($%02X,X)";
                operand = read(addr + 1);
                opbytes = 1;
                break;
            }
            case Indirect_Y:      // (Indirect),Y 2 bytes
            {
                format = "%02X %02X      %s   ($%02X),Y";
                operand = read(addr + 1);
                opbytes = 1;
                break;
            }
        }

        char buffer[256];

        if (opbytes == 0)
        {
            std::snprintf(buffer, 256, format, opcode, instruction.mnemonic);
        }
        else if (opbytes == 1)
        {
            std::snprintf(buffer, 256, format, opcode, lo(operand), instruction.mnemonic, lo(operand));
        }
        else if (opbytes == 2)
        {
            std::snprintf(buffer, 256, format, opcode, lo(operand), hi(operand), instruction.mnemonic, operand);
        }

        return std::string(buffer);
    }

    // Cart
    bool load_cartridge(const std::string& path)
    {
        std::ifstream ifs(path, std::ios::binary);

        if (!ifs)
        {
            return false;
        }

        char header[16];

        ifs.read(header, 16);

        if (!ifs)
        {
            return false;
        }

        bool ines = (header[0] == 'N' && header[1] == 'E' && header[2] == 'S' && header[3] == 0x1A);

        if (!ines)
        {
            return false;
        }

        if ((header[7] & 0x0C) == 0x08)
        {
            // NES2.0
        }
        else
        {
            // iNES
            if (header[6] & 4)
            {
                // Skip 512 byte trainer
                ifs.seekg(512, std::ios_base::cur);
            }

            if (!ifs)
            {
                return false;
            }

            // Read PRG ROM into RAM
            ifs.read((char*)(&_ram[0x8000]), header[4] * size_t(0x4000));

            if (header[4] == 1)
            {
                memcpy(&_ram[0xC000], &_ram[0x8000], 0x4000);
            }
        }
       
        return true;
    }

    uint16_t mem_offs = 0x0;

    bool on_update(float delta) override
    {
        if (m_keys[VK_NEXT].pressed)
        {
            mem_offs += 24 * 16;
        }
        else if (m_keys[VK_PRIOR].pressed)
        {
            mem_offs -= 24 * 16;
        }

        if (m_keys[VK_F5].pressed)
        {
            reset(false);
        }
        else if (m_keys[VK_F10].pressed)
        {
            step();
        }

        clear_screen(0x3f);

        // TV display
        for (int i = -1; i <= 1; ++i)
        {
            draw_rect(16 + i, 16 + i, (DisplayWidth - i) * 2, (DisplayHeight - i) * 2, 2);
        }

        // Palette
        int pal_y = 16 + (DisplayHeight * 2) + 16;

        for (int i = 0; i < 4; ++i)
        {
            int pal_x = 16;
            uint8_t c = i * 16;

            for (int j = 0; j < 16; ++j)
            {
                fill_rect(pal_x, pal_y, 32, 32, 0, c++, 0);
                pal_x += 33;
            }

            pal_y += 33;
        }

        // Dump RAM
        int ram_dump_x = 16 + (DisplayWidth * 2) + 16;
        int ram_dump_y = 16;
        int ram_dump_w = InspectorWidth;
        int ram_dump_h = 8 + 24 * vga9_glyph_height + 8;

        fill_rect(ram_dump_x, ram_dump_y, ram_dump_w, ram_dump_h, 2, 2, 0x20);

        int mem_y = ram_dump_y + 8;
        int mem_x = ram_dump_x + 8;
        uint16_t mem_ptr = mem_offs;

        for (int i = 0; i < 24; ++i)
        {
            uint8_t memp[16];
            char memc[16];

            for (int p = 0; p < 16; ++p)
            {
                memp[p] = read(mem_ptr + p);
                memc[p] = (memp[p] >= 0x20 && memp[p] <= 0x7f) ? (char)memp[p] : '.';
            }

            format_string(mem_x, mem_y, (const int*)vga9_glyphs, vga9_glyph_width, vga9_glyph_height, 0x20, 2,
                "%04X   %02X %02X %02X %02X %02X %02X %02X %02X  %02X %02X %02X %02X %02X %02X %02X %02X   %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", mem_ptr,
                memp[0], memp[1], memp[2], memp[3], memp[4], memp[5], memp[6], memp[7], memp[8], memp[9], memp[10], memp[11], memp[12], memp[13], memp[14], memp[15],
                memc[0], memc[1], memc[2], memc[3], memc[4], memc[5], memc[6], memc[7], memc[8], memc[9], memc[10], memc[11], memc[12], memc[13], memc[14], memc[15]);

            mem_ptr += 16;
            mem_y += vga9_glyph_height;
        }

        // CPU State
        int cpu_state_x = ram_dump_x;
        int cpu_state_y = ram_dump_y + ram_dump_h + 16;
        int cpu_state_w = InspectorWidth;
        int cpu_state_h = 8 + 4 * vga9_glyph_height + 8;

        fill_rect(cpu_state_x, cpu_state_y, cpu_state_w, cpu_state_h, 2, 2, 0x20);

        int cpu_x = cpu_state_x + 8;
        int cpu_y = cpu_state_y + 8;

        if (_stopped)
        {
            format_string(cpu_x, cpu_y, (const int*)vga9_glyphs, vga9_glyph_width, vga9_glyph_height, 0x20, 2, "    PC: ** STOPPED **");
            cpu_y += vga9_glyph_height;
        }
        else
        {
            std::string dissassembly = disassemble(_pc);
            format_string(cpu_x, cpu_y, (const int*)vga9_glyphs, vga9_glyph_width, vga9_glyph_height, 0x20, 2, "    PC: %04X  %s", _pc, dissassembly.c_str());
            cpu_y += vga9_glyph_height;
        }

        format_string(cpu_x, cpu_y, (const int*)vga9_glyphs, vga9_glyph_width, vga9_glyph_height, 0x20, 2, "     A: %02X  X: %02X  Y: %02X  SP: %02X", _a, _x, _y, _s);
        cpu_y += vga9_glyph_height;

        auto bit = [](uint8_t byte, int bit) -> char { return ((byte >> bit) & 1) ? '1' : '-'; };
        format_string(cpu_x, cpu_y, (const int*)vga9_glyphs, vga9_glyph_width, vga9_glyph_height, 0x20, 2, "        N V   B D I Z C");
        cpu_y += vga9_glyph_height;
        format_string(cpu_x, cpu_y, (const int*)vga9_glyphs, vga9_glyph_width, vga9_glyph_height, 0x20, 2, "Status: %c %c   %c %c %c %c %c",
            bit(_p, 7), bit(_p, 6), bit(_p, 4), bit(_p, 3), bit(_p, 2), bit(_p, 1), bit(_p, 0));
        cpu_y += vga9_glyph_height;

        return true;
    }
};

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
    GliNes nes;

    if (!nes.initialize(L"GLI NES", GliNes::WindowWidth, GliNes::WindowHeight, 1))
    {
        return 1;
    }

    nes.load_palette("ntscpalette.pal");
    nes.load_cartridge(R"(D:\EMU\nes\nestest.nes)");
    nes.reset(true);
    nes._pc = 0xc000;   // nestest ROM doesn't set the boot vector properly?
    nes.run();

    return 0;
}