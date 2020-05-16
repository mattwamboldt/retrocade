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
    uint8 cycleCount;
    OpCode opCode;
    AddressingMode addressMode;
};

static Operation operations[] = {
    {0x00, 7, BRK, Implied},
    {0x01, 6, ORA, IndirectX},
    {0x02, 0, KILL, Implied},
    {0x03, 0, KILL, Implied},
    {0x04, 0, KILL, Implied},
    {0x05, 3, ORA, ZeroPage},
    {0x06, 5, ASL, ZeroPage},
    {0x07, 0, KILL, Implied},
    {0x08, 3, PHP, Implied},
    {0x09, 2, ORA, Immediate},
    {0x0A, 2, ASL, Accumulator},
    {0x0B, 0, KILL, Implied},
    {0x0C, 0, KILL, Implied},
    {0x0D, 4, ORA, Absolute},
    {0x0E, 6, ASL, Absolute},
    {0x0F, 0, KILL, Implied},
    {0x10, 2, BPL, Relative},
    {0x11, 5, ORA, IndirectY},
    {0x12, 0, KILL, Implied},
    {0x13, 0, KILL, Implied},
    {0x14, 0, KILL, Implied},
    {0x15, 4, ORA, ZeroPageX},
    {0x16, 6, ASL, ZeroPageX},
    {0x17, 0, KILL, Implied},
    {0x18, 2, CLC, Implied},
    {0x19, 4, ORA, AbsoluteY},
    {0x1A, 0, KILL, Implied},
    {0x1B, 0, KILL, Implied},
    {0x1C, 0, KILL, Implied},
    {0x1D, 4, ORA, AbsoluteX},
    {0x1E, 7, ASL, AbsoluteX},
    {0x1F, 0, KILL, Implied},
    {0x20, 6, JSR, Absolute},
    {0x21, 6, AND, IndirectX},
    {0x22, 0, KILL, Implied},
    {0x23, 0, KILL, Implied},
    {0x24, 3, BIT, ZeroPage},
    {0x25, 3, AND, ZeroPage},
    {0x26, 5, ROL, ZeroPage},
    {0x27, 0, KILL, Implied},
    {0x28, 4, PLP, Implied},
    {0x29, 2, AND, Immediate},
    {0x2A, 2, ROL, Accumulator},
    {0x2B, 0, KILL, Implied},
    {0x2C, 4, BIT, Absolute},
    {0x2D, 4, AND, Absolute},
    {0x2E, 6, ROL, Absolute},
    {0x2F, 0, KILL, Implied},
    {0x30, 2, BMI, Relative},
    {0x31, 5, AND, IndirectY},
    {0x32, 0, KILL, Implied},
    {0x33, 0, KILL, Implied},
    {0x34, 0, KILL, Implied},
    {0x35, 4, AND, ZeroPageX},
    {0x36, 6, ROL, ZeroPageX},
    {0x37, 0, KILL, Implied},
    {0x38, 2, SEC, Implied},
    {0x39, 4, AND, AbsoluteY},
    {0x3A, 0, KILL, Implied},
    {0x3B, 0, KILL, Implied},
    {0x3C, 0, KILL, Implied},
    {0x3D, 4, AND, AbsoluteX},
    {0x3E, 7, ROL, AbsoluteX},
    {0x3F, 0, KILL, Implied},
    {0x40, 6, RTI, Implied},
    {0x41, 6, EOR, IndirectX},
    {0x42, 0, KILL, Implied},
    {0x43, 0, KILL, Implied},
    {0x44, 0, KILL, Implied},
    {0x45, 3, EOR, ZeroPage},
    {0x46, 5, LSR, ZeroPage},
    {0x47, 0, KILL, Implied},
    {0x48, 3, PHA, Implied},
    {0x49, 2, EOR, Immediate},
    {0x4A, 2, LSR, Accumulator},
    {0x4B, 0, KILL, Implied},
    {0x4C, 3, JMP, Absolute},
    {0x4D, 4, EOR, Absolute},
    {0x4E, 6, LSR, Absolute},
    {0x4F, 0, KILL, Implied},
    {0x50, 2, BVC, Relative},
    {0x51, 5, EOR, IndirectY},
    {0x52, 0, KILL, Implied},
    {0x53, 0, KILL, Implied},
    {0x54, 0, KILL, Implied},
    {0x55, 4, EOR, ZeroPageX},
    {0x56, 6, LSR, ZeroPageX},
    {0x57, 0, KILL, Implied},
    {0x58, 2, CLI, Implied},
    {0x59, 4, EOR, AbsoluteY},
    {0x5A, 0, KILL, Implied},
    {0x5B, 0, KILL, Implied},
    {0x5C, 0, KILL, Implied},
    {0x5D, 4, EOR, AbsoluteX},
    {0x5E, 7, LSR, AbsoluteX},
    {0x5F, 0, KILL, Implied},
    {0x60, 6, RTS, Implied},
    {0x61, 6, ADC, IndirectX},
    {0x62, 0, KILL, Implied},
    {0x63, 0, KILL, Implied},
    {0x64, 0, KILL, Implied},
    {0x65, 3, ADC, ZeroPage},
    {0x66, 5, ROR, ZeroPage},
    {0x67, 0, KILL, Implied},
    {0x68, 4, PLA, Implied},
    {0x69, 2, ADC, Immediate},
    {0x6A, 2, ROR, Accumulator},
    {0x6B, 0, KILL, Implied},
    {0x6C, 5, JMP, Indirect},
    {0x6D, 4, ADC, Absolute},
    {0x6E, 6, ROR, Absolute},
    {0x6F, 0, KILL, Implied},
    {0x70, 2, BVS, Relative},
    {0x71, 5, ADC, IndirectY},
    {0x72, 0, KILL, Implied},
    {0x73, 0, KILL, Implied},
    {0x74, 0, KILL, Implied},
    {0x75, 4, ADC, ZeroPageX},
    {0x76, 6, ROR, ZeroPageX},
    {0x77, 0, KILL, Implied},
    {0x78, 2, SEI, Implied},
    {0x79, 4, ADC, AbsoluteY},
    {0x7A, 0, KILL, Implied},
    {0x7B, 0, KILL, Implied},
    {0x7C, 0, KILL, Implied},
    {0x7D, 4, ADC, AbsoluteX},
    {0x7E, 7, ROR, AbsoluteX},
    {0x7F, 0, KILL, Implied},
    {0x80, 0, KILL, Implied},
    {0x81, 6, STA, IndirectX},
    {0x82, 0, KILL, Implied},
    {0x83, 0, KILL, Implied},
    {0x84, 3, STY, ZeroPage},
    {0x85, 3, STA, ZeroPage},
    {0x86, 3, STX, ZeroPage},
    {0x87, 0, KILL, Implied},
    {0x88, 2, DEY, Implied},
    {0x89, 0, KILL, Implied},
    {0x8A, 2, TXA, Implied},
    {0x8B, 0, KILL, Implied},
    {0x8C, 4, STY, Absolute},
    {0x8D, 4, STA, Absolute},
    {0x8E, 4, STX, Absolute},
    {0x8F, 0, KILL, Implied},
    {0x90, 2, BCC, Relative},
    {0x91, 6, STA, IndirectY},
    {0x92, 0, KILL, Implied},
    {0x93, 0, KILL, Implied},
    {0x94, 4, STY, ZeroPageX},
    {0x95, 4, STA, ZeroPageX},
    {0x96, 4, STX, ZeroPageY},
    {0x97, 0, KILL, Implied},
    {0x98, 2, TYA, Implied},
    {0x99, 5, STA, AbsoluteY},
    {0x9A, 2, TXS, Implied},
    {0x9B, 0, KILL, Implied},
    {0x9C, 0, KILL, Implied},
    {0x9D, 5, STA, AbsoluteX},
    {0x9E, 0, KILL, Implied},
    {0x9F, 0, KILL, Implied},
    {0xA0, 2, LDY, Immediate},
    {0xA1, 6, LDA, IndirectX},
    {0xA2, 2, LDX, Immediate},
    {0xA3, 0, KILL, Implied},
    {0xA4, 3, LDY, ZeroPage},
    {0xA5, 3, LDA, ZeroPage},
    {0xA6, 3, LDX, ZeroPage},
    {0xA7, 0, KILL, Implied},
    {0xA8, 2, TAY, Implied},
    {0xA9, 2, LDA, Immediate},
    {0xAA, 2, TAX, Implied},
    {0xAB, 0, KILL, Implied},
    {0xAC, 4, LDY, Absolute},
    {0xAD, 4, LDA, Absolute},
    {0xAE, 4, LDX, Absolute},
    {0xAF, 0, KILL, Implied},
    {0xB0, 2, BCS, Relative},
    {0xB1, 5, LDA, IndirectY},
    {0xB2, 0, KILL, Implied},
    {0xB3, 0, KILL, Implied},
    {0xB4, 4, LDY, ZeroPageX},
    {0xB5, 4, LDA, ZeroPageX},
    {0xB6, 4, LDX, ZeroPageY},
    {0xB7, 0, KILL, Implied},
    {0xB8, 2, CLV, Implied},
    {0xB9, 4, LDA, AbsoluteY},
    {0xBA, 2, TSX, Implied},
    {0xBB, 0, KILL, Implied},
    {0xBC, 4, LDY, AbsoluteX},
    {0xBD, 4, LDA, AbsoluteX},
    {0xBE, 4, LDX, AbsoluteY},
    {0xBF, 0, KILL, Implied},
    {0xC0, 2, CPY, Immediate},
    {0xC1, 6, CMP, IndirectX},
    {0xC2, 0, KILL, Implied},
    {0xC3, 0, KILL, Implied},
    {0xC4, 3, CPY, ZeroPage},
    {0xC5, 3, CMP, ZeroPage},
    {0xC6, 5, DEC, ZeroPage},
    {0xC7, 0, KILL, Implied},
    {0xC8, 2, INY, Implied},
    {0xC9, 2, CMP, Immediate},
    {0xCA, 2, DEX, Implied},
    {0xCB, 0, KILL, Implied},
    {0xCC, 4, CPY, Absolute},
    {0xCD, 4, CMP, Absolute},
    {0xCE, 6, DEC, Absolute},
    {0xCF, 0, KILL, Implied},
    {0xD0, 2, BNE, Relative},
    {0xD1, 5, CMP, IndirectY},
    {0xD2, 0, KILL, Implied},
    {0xD3, 0, KILL, Implied},
    {0xD4, 0, KILL, Implied},
    {0xD5, 4, CMP, ZeroPageX},
    {0xD6, 6, DEC, ZeroPageX},
    {0xD7, 0, KILL, Implied},
    {0xD8, 2, CLD, Implied},
    {0xD9, 4, CMP, AbsoluteY},
    {0xDA, 0, KILL, Implied},
    {0xDB, 0, KILL, Implied},
    {0xDC, 0, KILL, Implied},
    {0xDD, 4, CMP, AbsoluteX},
    {0xDE, 7, DEC, AbsoluteX},
    {0xDF, 0, KILL, Implied},
    {0xE0, 2, CPX, Immediate},
    {0xE1, 6, SBC, IndirectX},
    {0xE2, 0, KILL, Implied},
    {0xE3, 0, KILL, Implied},
    {0xE4, 3, CPX, ZeroPage},
    {0xE5, 3, SBC, ZeroPage},
    {0xE6, 5, INC, ZeroPage},
    {0xE7, 0, KILL, Implied},
    {0xE8, 2, INX, Implied},
    {0xE9, 2, SBC, Immediate},
    {0xEA, 2, NOP, Implied},
    {0xEB, 0, KILL, Implied},
    {0xEC, 4, CPX, Absolute},
    {0xED, 4, SBC, Absolute},
    {0xEE, 6, INC, Absolute},
    {0xEF, 0, KILL, Implied},
    {0xF0, 2, BEQ, Relative},
    {0xF1, 5, SBC, IndirectY},
    {0xF2, 0, KILL, Implied},
    {0xF3, 0, KILL, Implied},
    {0xF4, 0, KILL, Implied},
    {0xF5, 4, SBC, ZeroPageX},
    {0xF6, 6, INC, ZeroPageX},
    {0xF7, 0, KILL, Implied},
    {0xF8, 2, SED, Implied},
    {0xF9, 4, SBC, AbsoluteY},
    {0xFA, 0, KILL, Implied},
    {0xFB, 0, KILL, Implied},
    {0xFC, 0, KILL, Implied},
    {0xFD, 4, SBC, AbsoluteX},
    {0xFE, 7, INC, AbsoluteX},
    {0xFF, 0, KILL, Implied},
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
    bool isExecuting() { return waitCycles > 0; }
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