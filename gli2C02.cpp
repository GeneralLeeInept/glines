#include "gli2C02.h"

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
    AHX,    // aka AXA
    ALR,
    ANC,
    ARR,
    AXS,
    DCP,    // aka DCM
    ISC,    // aka INS
    LAS,
    LAX,
    RLA,
    RRA,
    SAX,
    SHX,    // aka XAS
    SHY,    // aka SAY
    SLO,    // aka ASO
    SRE,    // aka LSE
    STP,    // aka HLT
    TAS,
    XAA
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
    { Opcode::ORA,  "ORA", AddressingMode::Absolute_Y,    0 },    // 19
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
    { Opcode::EOR,  "EOR", AddressingMode::Absolute_X,    0 },    // 5D
    { Opcode::LSR,  "LSR", AddressingMode::Absolute_X,    0 },    // 5E
    { Opcode::SRE,  "SRE", AddressingMode::Absolute_X,    0 },    // 5F

    { Opcode::RTS,  "RTS", AddressingMode::Implied,       0 },    // 60
    { Opcode::ADC,  "ADC", AddressingMode::Indirect_X,    0 },    // 61
    { Opcode::STP,  "STP", AddressingMode::Implied,       0 },    // 62
    { Opcode::RRA,  "RRA", AddressingMode::Indirect_X,    0 },    // 63
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
    { Opcode::STX,  "STX", AddressingMode::ZeroPage_Y,    0 },    // 96
    { Opcode::SAX,  "SAX", AddressingMode::ZeroPage_Y,    0 },    // 97
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
    { Opcode::LDX,  "LDX", AddressingMode::ZeroPage_Y,    0 },    // B6
    { Opcode::LAX,  "LAX", AddressingMode::ZeroPage_Y,    0 },    // B7
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
    C = 0,
    Z = 1,
    I = 2,
    D = 3,
    B = 4,  // Never held in process status register
    X = 5,  // Never held in process status register
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


void gli2C02::connect(ReadCallback read_callback, WriteCallback write_callback)
{
    read = read_callback;
    write = write_callback;
}


void  gli2C02::reset(bool coldstart)
{
    _pc = read_word(0xfffc);
    _stopped = false;

    if (coldstart)
    {
        // https://wiki.nesdev.com/w/index.php/CPU_ALL#At_power-up
        _p = 0x24;
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


void gli2C02::clock()
{

}


void gli2C02::step()
{
    if (_stopped)
    {
        return;
    }

    //std::string dissassembly = disassemble(_pc);
    //char log[1024];
    //std::snprintf(log, 1024, "%04X %-31s A:%02X X:%02X Y:%02X P:%02X SP:%02X\n", _pc, dissassembly.c_str(), _a, _x, _y, _p, _s);
    //OutputDebugStringA(log);

    uint8_t opcode = read(_pc++);

    if (opcode > sizeof(InstructionTable) / sizeof(InstructionTable[0]))
    {
        _stopped = true;
    }
    else
    {
        exec(InstructionTable[opcode]);
    }
}


uint16_t gli2C02::read_word(uint16_t addr)
{
    return make_word(read(addr), read(addr + 1));
}


void gli2C02::push(uint8_t value)
{
    write(0x100 + _s, value);
    _s -= 1;
}


uint8_t gli2C02::pop()
{
    _s += 1;
    return read(0x100 + _s);
}


void gli2C02::exec(const Instruction& instruction)
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
            address = read_word(_pc) + _x;
            _pc += 2;
            break;
        }
        case Absolute_Y:
        {
            address = read_word(_pc) + _y;
            _pc += 2;
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
                set_bit(_p, StatusBits::C, get_bit(_a, 7));
                value = _a << 1;
                load_register(_a);
            }
            else
            {
                value = read(address);
                set_bit(_p, StatusBits::C, get_bit(value, 7));
                value <<= 1;
                set_bit(_p, StatusBits::Z, _a == 0);
                set_bit(_p, StatusBits::N, get_bit(value, 7));
                write(address, value);
            }
            break;
        }
        case Opcode::BCC:
        {
            if (!get_bit(_p, StatusBits::C))
            {
                _pc = address;
            }
            break;
        }
        case Opcode::BCS:
        {
            if (get_bit(_p, StatusBits::C))
            {
                _pc = address;
            }
            break;
        }
        case Opcode::BEQ:
        {
            if (get_bit(_p, StatusBits::Z))
            {
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
                _pc = address;
            }
            break;
        }
        case Opcode::BNE:
        {
            if (!get_bit(_p, StatusBits::Z))
            {
                _pc = address;
            }
            break;
        }
        case Opcode::BPL:
        {
            if (!get_bit(_p, StatusBits::N))
            {
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
                _pc = address;
            }
            break;
        }
        case Opcode::BVS:
        {
            if (get_bit(_p, StatusBits::V))
            {
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
            // Interrupts are level triggered so if /IRQ is low an interrupt will occur as soon as CLI executes
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
            set_bit(_p, StatusBits::B, 1);
            set_bit(_p, StatusBits::X, 1);
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


std::string gli2C02::disassemble(uint16_t addr)
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
