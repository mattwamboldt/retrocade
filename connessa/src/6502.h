#pragma once
#include "bus.h"

enum OpCode
{
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
    KILL, // Illegal opcodes
};

enum AddressingMode
{
    Absolute,
    AbsoluteX,
    AbsoluteY,
    Accumulator,
    Immediate,
    Implied,
    Indirect,
    IndirectX,
    IndirectY, // Very different mechanisms, but easier to keep straight this way
    Relative,
    ZeroPage,
    ZeroPageX,
    ZeroPageY,
};

static const char* opCodeNames[] = {
    "ADC",
    "AND",
    "ASL",
    "BCC",
    "BCS",
    "BEQ",
    "BIT",
    "BMI",
    "BNE",
    "BPL",
    "BRK",
    "BVC",
    "BVS",
    "CLC",
    "CLD",
    "CLI",
    "CLV",
    "CMP",
    "CPX",
    "CPY",
    "DEC",
    "DEX",
    "DEY",
    "EOR",
    "INC",
    "INX",
    "INY",
    "JMP",
    "JSR",
    "LDA",
    "LDX",
    "LDY",
    "LSR",
    "NOP",
    "ORA",
    "PHA",
    "PHP",
    "PLA",
    "PLP",
    "ROL",
    "ROR",
    "RTI",
    "RTS",
    "SBC",
    "SEC",
    "SED",
    "SEI",
    "STA",
    "STX",
    "STY",
    "TAX",
    "TAY",
    "TSX",
    "TXA",
    "TXS",
    "TYA",
    "KILL" // Illegal opcodes
};

struct Operation
{
    uint8 instruction;
    OpCode opCode;
    AddressingMode addressMode;
};

static Operation operations[] = {
    {0x00, BRK, Implied},
    {0x01, ORA, IndirectX},
    {0x02, KILL, Implied},
    {0x03, KILL, Implied},
    {0x04, KILL, Implied},
    {0x05, ORA, ZeroPage},
    {0x06, ASL, ZeroPage},
    {0x07, KILL, Implied},
    {0x08, PHP, Implied},
    {0x09, ORA, Immediate},
    {0x0A, ASL, Accumulator},
    {0x0B, KILL, Implied},
    {0x0C, KILL, Implied},
    {0x0D, ORA, Absolute},
    {0x0E, ASL, Absolute},
    {0x0F, KILL, Implied},
    {0x10, BPL, Relative},
    {0x11, ORA, IndirectY},
    {0x12, KILL, Implied},
    {0x13, KILL, Implied},
    {0x14, KILL, Implied},
    {0x15, ORA, ZeroPageX},
    {0x16, ASL, ZeroPageX},
    {0x17, KILL, Implied},
    {0x18, CLC, Implied},
    {0x19, ORA, AbsoluteY},
    {0x1A, KILL, Implied},
    {0x1B, KILL, Implied},
    {0x1C, KILL, Implied},
    {0x1D, ORA, AbsoluteX},
    {0x1E, ASL, AbsoluteX},
    {0x1F, KILL, Implied},
    {0x20, JSR, Absolute},
    {0x21, AND, IndirectX},
    {0x22, KILL, Implied},
    {0x23, KILL, Implied},
    {0x24, BIT, ZeroPage},
    {0x25, AND, ZeroPage},
    {0x26, ROL, ZeroPage},
    {0x27, KILL, Implied},
    {0x28, PLP, Implied},
    {0x29, AND, Immediate},
    {0x2A, ROL, Accumulator},
    {0x2B, KILL, Implied},
    {0x2C, BIT, Absolute},
    {0x2D, AND, Absolute},
    {0x2E, ROL, Absolute},
    {0x2F, KILL, Implied},
    {0x30, BMI, Relative},
    {0x31, AND, IndirectY},
    {0x32, KILL, Implied},
    {0x33, KILL, Implied},
    {0x34, KILL, Implied},
    {0x35, AND, ZeroPageX},
    {0x36, ROL, ZeroPageX},
    {0x37, KILL, Implied},
    {0x38, SEC, Implied},
    {0x39, AND, AbsoluteY},
    {0x3A, KILL, Implied},
    {0x3B, KILL, Implied},
    {0x3C, KILL, Implied},
    {0x3D, AND, AbsoluteX},
    {0x3E, ROL, AbsoluteX},
    {0x3F, KILL, Implied},
    {0x40, RTI, Implied},
    {0x41, EOR, IndirectX},
    {0x42, KILL, Implied},
    {0x43, KILL, Implied},
    {0x44, KILL, Implied},
    {0x45, EOR, ZeroPage},
    {0x46, LSR, ZeroPage},
    {0x47, KILL, Implied},
    {0x48, PHA, Implied},
    {0x49, EOR, Immediate},
    {0x4A, LSR, Accumulator},
    {0x4B, KILL, Implied},
    {0x4C, JMP, Absolute},
    {0x4D, EOR, Absolute},
    {0x4E, LSR, Absolute},
    {0x4F, KILL, Implied},
    {0x50, BVC, Relative},
    {0x51, EOR, IndirectY},
    {0x52, KILL, Implied},
    {0x53, KILL, Implied},
    {0x54, KILL, Implied},
    {0x55, EOR, ZeroPageX},
    {0x56, LSR, ZeroPageX},
    {0x57, KILL, Implied},
    {0x58, CLI, Implied},
    {0x59, EOR, AbsoluteY},
    {0x5A, KILL, Implied},
    {0x5B, KILL, Implied},
    {0x5C, KILL, Implied},
    {0x5D, EOR, AbsoluteX},
    {0x5E, LSR, AbsoluteX},
    {0x5F, KILL, Implied},
    {0x60, RTS, Implied},
    {0x61, ADC, IndirectX},
    {0x62, KILL, Implied},
    {0x63, KILL, Implied},
    {0x64, KILL, Implied},
    {0x65, ADC, ZeroPage},
    {0x66, ROR, ZeroPage},
    {0x67, KILL, Implied},
    {0x68, PLA, Implied},
    {0x69, ADC, Immediate},
    {0x6A, ROR, Accumulator},
    {0x6B, KILL, Implied},
    {0x6C, JMP, Indirect},
    {0x6D, ADC, Absolute},
    {0x6E, ROR, Absolute},
    {0x6F, KILL, Implied},
    {0x70, BVS, Relative},
    {0x71, ADC, IndirectY},
    {0x72, KILL, Implied},
    {0x73, KILL, Implied},
    {0x74, KILL, Implied},
    {0x75, ADC, ZeroPageX},
    {0x76, ROR, ZeroPageX},
    {0x77, KILL, Implied},
    {0x78, SEI, Implied},
    {0x79, ADC, AbsoluteY},
    {0x7A, KILL, Implied},
    {0x7B, KILL, Implied},
    {0x7C, KILL, Implied},
    {0x7D, ADC, AbsoluteX},
    {0x7E, ROR, AbsoluteX},
    {0x7F, KILL, Implied},
    {0x80, KILL, Implied},
    {0x81, STA, IndirectX},
    {0x82, KILL, Implied},
    {0x83, KILL, Implied},
    {0x84, STY, ZeroPage},
    {0x85, STA, ZeroPage},
    {0x86, STX, ZeroPage},
    {0x87, KILL, Implied},
    {0x88, DEY, Implied},
    {0x89, KILL, Implied},
    {0x8A, TXA, Implied},
    {0x8B, KILL, Implied},
    {0x8C, STY, Absolute},
    {0x8D, STA, Absolute},
    {0x8E, STX, Absolute},
    {0x8F, KILL, Implied},
    {0x90, BCC, Relative},
    {0x91, STA, IndirectY},
    {0x92, KILL, Implied},
    {0x93, KILL, Implied},
    {0x94, STY, ZeroPageX},
    {0x95, STA, ZeroPageX},
    {0x96, STX, ZeroPageY},
    {0x97, KILL, Implied},
    {0x98, TYA, Implied},
    {0x99, STA, AbsoluteY},
    {0x9A, TXS, Implied},
    {0x9B, KILL, Implied},
    {0x9C, KILL, Implied},
    {0x9D, STA, AbsoluteX},
    {0x9E, KILL, Implied},
    {0x9F, KILL, Implied},
    {0xA0, LDY, Immediate},
    {0xA1, LDA, IndirectX},
    {0xA2, LDX, Immediate},
    {0xA3, KILL, Implied},
    {0xA4, LDY, ZeroPage},
    {0xA5, LDA, ZeroPage},
    {0xA6, LDX, ZeroPage},
    {0xA7, KILL, Implied},
    {0xA8, TAY, Implied},
    {0xA9, LDA, Immediate},
    {0xAA, TAX, Implied},
    {0xAB, KILL, Implied},
    {0xAC, LDY, Absolute},
    {0xAD, LDA, Absolute},
    {0xAE, LDX, Absolute},
    {0xAF, KILL, Implied},
    {0xB0, BCS, Relative},
    {0xB1, LDA, IndirectY},
    {0xB2, KILL, Implied},
    {0xB3, KILL, Implied},
    {0xB4, LDY, ZeroPageX},
    {0xB5, LDA, ZeroPageX},
    {0xB6, LDX, ZeroPageY},
    {0xB7, KILL, Implied},
    {0xB8, CLV, Implied},
    {0xB9, LDA, AbsoluteY},
    {0xBA, TSX, Implied},
    {0xBB, KILL, Implied},
    {0xBC, LDY, AbsoluteX},
    {0xBD, LDA, AbsoluteX},
    {0xBE, LDX, AbsoluteY},
    {0xBF, KILL, Implied},
    {0xC0, CPY, Immediate},
    {0xC1, CMP, IndirectX},
    {0xC2, KILL, Implied},
    {0xC3, KILL, Implied},
    {0xC4, CPY, ZeroPage},
    {0xC5, CMP, ZeroPage},
    {0xC6, DEC, ZeroPage},
    {0xC7, KILL, Implied},
    {0xC8, INY, Implied},
    {0xC9, CMP, Immediate},
    {0xCA, DEX, Implied},
    {0xCB, KILL, Implied},
    {0xCC, CPY, Absolute},
    {0xCD, CMP, Absolute},
    {0xCE, DEC, Absolute},
    {0xCF, KILL, Implied},
    {0xD0, BNE, Relative},
    {0xD1, CMP, IndirectY},
    {0xD2, KILL, Implied},
    {0xD3, KILL, Implied},
    {0xD4, KILL, Implied},
    {0xD5, CMP, ZeroPageX},
    {0xD6, DEC, ZeroPageX},
    {0xD7, KILL, Implied},
    {0xD8, CLD, Implied},
    {0xD9, CMP, AbsoluteY},
    {0xDA, KILL, Implied},
    {0xDB, KILL, Implied},
    {0xDC, KILL, Implied},
    {0xDD, CMP, AbsoluteX},
    {0xDE, DEC, AbsoluteX},
    {0xDF, KILL, Implied},
    {0xE0, CPX, Immediate},
    {0xE1, SBC, IndirectX},
    {0xE2, KILL, Implied},
    {0xE3, KILL, Implied},
    {0xE4, CPX, ZeroPage},
    {0xE5, SBC, ZeroPage},
    {0xE6, INC, ZeroPage},
    {0xE7, KILL, Implied},
    {0xE8, INX, Implied},
    {0xE9, SBC, Immediate},
    {0xEA, NOP, Implied},
    {0xEB, KILL, Implied},
    {0xEC, CPX, Absolute},
    {0xED, SBC, Absolute},
    {0xEE, INC, Absolute},
    {0xEF, KILL, Implied},
    {0xF0, BEQ, Relative},
    {0xF1, SBC, IndirectY},
    {0xF2, KILL, Implied},
    {0xF3, KILL, Implied},
    {0xF4, KILL, Implied},
    {0xF5, SBC, ZeroPageX},
    {0xF6, INC, ZeroPageX},
    {0xF7, KILL, Implied},
    {0xF8, SED, Implied},
    {0xF9, SBC, AbsoluteY},
    {0xFA, KILL, Implied},
    {0xFB, KILL, Implied},
    {0xFC, KILL, Implied},
    {0xFD, SBC, AbsoluteX},
    {0xFE, INC, AbsoluteX},
    {0xFF, KILL, Implied},
};

enum StatusFlags
{
    STATUS_CARRY       = 0b00000001,
    STATUS_ZERO        = 0b00000010,
    STATUS_INT_DISABLE = 0b00000100,
    STATUS_DECIMAL     = 0b00001000,
    STATUS_B           = 0b00110000,
    STATUS_OVERFLOW    = 0b01000000,
    STATUS_NEGATIVE    = 0b10000000
};

class MOS6502
{
public:
    // Actual Registers on the system
    uint16 pc;
    uint8 stack;
    uint8 status;
    uint8 accumulator;
    uint8 x;
    uint8 y;

    // Values to abstract the internal magic of the 6502
    uint8 inst; // Currently executing instruction
    uint16 instAddr; // Address of current instruction

    uint8 tempData; // temp storage

    void connect(IBus* bus) { this->bus = bus; }

    void reset();
    bool tick(); // true on start of instruction
    bool hasHalted() { return isHalted; }
    bool isFlagSet(uint8 flags) { return status & flags; }
    uint16 calcAddress(AddressingMode addressMode, uint16 address, uint8 p1, uint8 p2);
    bool requireRead(uint8 opCode);

    // Operations (individual functions w/ data param helps debugging)
    void addWithCarry(uint8 data);
    void subtractWithCarry(uint8 data);

    uint8 shiftLeft(uint8 data);
    uint8 shiftRight(uint8 data);

    void branchCarryClear(uint8 data);
    void branchCarrySet(uint8 data);
    void branchEqual(uint8 data);
    void branchNegative(uint8 data);
    void branchNotZero(uint8 data);
    void branchPositive(uint8 data);
    void branchOverflowSet(uint8 data);
    void branchOverflowClear(uint8 data);
    void bitTest(uint8 data);

    void clearCarry();
    void clearDecimal();
    void clearInteruptDisable();
    void clearOverflow();
    void setCarry();
    void setDecimal();
    void setInteruptDisable();

    void compareA(uint8 data);
    void compareX(uint8 data);
    void compareY(uint8 data);

    uint8 decrement(uint8 data);
    void decrementX();
    void decrementY();

    void andA(uint8 data);
    void exclusiveOrA(uint8 data);
    void orA(uint8 data);

    uint8 increment(uint8 data);
    void incrementX();
    void incrementY();

    void jump(uint16 address);
    void jumpSubroutine(uint16 address);

    void loadA(uint8 data);
    void loadX(uint8 data);
    void loadY(uint8 data);

    void pushA();
    void pushStatus();
    void pullA();
    void pullStatus();

    uint8 rotateLeft(uint8 data);
    uint8 rotateRight(uint8 data);

    void returnIntertupt();
    void returnSubroutine();

    void transferAtoX();
    void transferAtoY();
    void transferStoX();
    void transferXtoA();
    void transferXtoS();
    void transferYtoA();

    // Interupt functions
    void forceBreak();
    void nonMaskableInterrupt();
    void requestInterrupt();

private:
    bool isHalted;

    bool nmiRequested;
    bool interruptRequested;
    uint8 waitCycles;

    uint8 p1; // temp storage for operands
    uint8 p2;

    IBus* bus;

    // This model somewhat splits the operation in a way that could be
    // made cycle accurate in future, but for now is run all at once
    uint16 calcAddress(AddressingMode addressMode);
    void loadOperands();
    void executeInstruction();

    // Status Register helpers
    void setFlags(uint8 flags);
    void clearFlags(uint8 flags);
    void updateNZ(uint8 val);
    
    // Utility functions
    void compare(uint8 a, uint8 b);

    void jumpOnFlagSet(uint8 flag, uint8 offset);
    void jumpOnFlagClear(uint8 flag, uint8 offset);
    void jumpRelative(uint8 offset);

    // Stack Manipulation
    uint8 pull();
    uint16 pullWord();
    void push(uint8 v);
    void pushWord(uint16 v);

    uint16 readWord(uint16 base);
    uint8 readData(AddressingMode addressMode);
    void writeData(AddressingMode addressMode, uint8 val);
};