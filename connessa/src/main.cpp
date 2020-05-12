#include <stdio.h>
#include <conio.h>
#include <windows.h>

#include "common.h"

struct CPU
{
    // Actual Registers on the system
    uint16 pc;
    uint8 stack;
    uint8 status;
    uint8 accumulator;
    uint8 x;
    uint8 y;

    uint8 add; //Internal register ussed to hold temp values (See Research)

    // Values to abstract the internal magic of the 6502
    // TODO: Use these to handle proper read and write timings
    uint8 t; // Cycle count since starting the operation
    uint8 inst; // Currently executing instruction
    uint16 instAddr; // Address of current instruction, used for debugging
    uint8 p1; // operands
    uint8 p2;
};

struct PPU
{
    uint8 control;
    uint8 mask;
    uint8 status;
    uint8 oamAddress;
    uint8 oamData;
    uint8 scroll;
    uint8 address;
    uint8 data;
    uint8 oamDma;
    uint8 oam[256];
};

// http://wiki.nesdev.com/w/index.php/2A03
struct APU
{
    uint8 square1Vol;
    uint8 square1Sweep;
    uint8 square1Lo;
    uint8 square1Hi;

    uint8 square2Vol;
    uint8 square2Sweep;
    uint8 square2Lo;
    uint8 square2Hi;

    uint8 triangleLinear;
    uint8 triangleLo;
    uint8 triangleHi;

    uint8 noiseVol;
    uint8 noiseLo;
    uint8 noiseHi;

    uint8 dmcFrequency;
    uint8 dmcRaw;
    uint8 dmcStart;
    uint8 dmcLength;

    uint8 channelStatus;
};

// PRG = cartridge side connected to the cpu
// CHR = cartridge side connected to the ppu

struct INESHeader
{
    uint32 formatMarker; // 0x4E45531A ("NES" followed by MS-DOS end-of-file)
    uint8 prgRomSize; // Size of PRG ROM in 16 KB units
    uint8 chrRomSize; // Size of CHR ROM in 8 KB units (Value 0 means the board uses CHR RAM)
    uint8 flags6;
    uint8 flags7;
    uint8 flags8;
    uint8 flags9;
    uint8 flags10;
    uint8 reserved[5];
};

bool running = true;

// globals are bad!!! change this later!!!
static CPU cpu = {};
static uint8 ram[2 * 1024] = {};
static uint8 cartRam[8 * 1024] = {};

static uint8* map000prgRomBank1;
static uint8* map000prgRomBank2;
static uint8* map000prgRam;

static PPU ppu = {};
static uint8 vram[2 * 1024] = {};
static uint8 paletteRam[32];
static uint8* chrRom;

static APU apu = {};
static uint8 joy1;
static uint8 joy2;

uint8 cpuRead(uint16 address)
{
    if (address < 0x2000)
    {
        // map to the internal 2kb ram
        return ram[address & 0x07FF];
    }
    
    if (address < 0x4000)
    {
        // map to the ppu register based on the last 3 bits
        // http://wiki.nesdev.com/w/index.php/PPU_registers
        // TODO: might be better to store the registers as an array or union
        // doing the dumb obvious thing for now
        uint16 masked = address & 0x0007;
        switch (masked)
        {
            case 0x00: return ppu.control;
            case 0x01: return ppu.mask;
            case 0x02: return ppu.status;
            case 0x03: return ppu.oamAddress;
            case 0x04: return ppu.oamData;
            case 0x05: return ppu.scroll;
            case 0x06: return ppu.address;
            case 0x07: return ppu.data;
            default: return 0;
        }
    }

    if (address < 0x4020)
    {
        // map to the apu/io registers
        // http://wiki.nesdev.com/w/index.php/2A03
        // TODO: docs mention these as readonly, remove if that matters
        switch (address)
        {
            case 0x4000: return apu.square1Vol;
            case 0x4001: return apu.square1Sweep;
            case 0x4002: return apu.square1Lo;
            case 0x4003: return apu.square1Hi;
            case 0x4004: return apu.square2Vol;
            case 0x4005: return apu.square2Sweep;
            case 0x4006: return apu.square2Lo;
            case 0x4007: return apu.square2Vol;
            case 0x4008: return apu.triangleLinear;
            case 0x400A: return apu.triangleLo;
            case 0x400B: return apu.triangleHi;
            case 0x400C: return apu.noiseVol;
            case 0x400E: return apu.noiseLo;
            case 0x400F: return apu.noiseHi;
            case 0x4010: return apu.dmcFrequency;
            case 0x4011: return apu.dmcRaw;
            case 0x4012: return apu.dmcStart;
            case 0x4013: return apu.dmcLength;
            case 0x4014: return ppu.oamDma;
            case 0x4015: return apu.channelStatus;
            case 0x4016: return joy1;
            case 0x4017: return joy2;
            default: return 0;
        }
    }

    // Cartridge space (logic depends on the mapper)
    // TODO: allow multiple mappers, for now using 000 since its the one in the test case

    if (address < 0x6000)
    {
        return 0;
    }

    if (address < 0x8000)
    {
        return cartRam[address - 0x6000];
    }

    if (address < 0xC000)
    {
        return map000prgRomBank1[address - 0x8000];
    }

    return map000prgRomBank2[address - 0xC000];
}

// TODO: we've officially reached the point of needing some refactor action
// The order of declarations is getting a bit hairy for my liking
static bool renderMode = false;
static uint8 debugMemPage;

void renderMemCell(uint16 address, uint8 val)
{
    if (renderMode)
    {
        return;
    }

    if (address >> 8 != debugMemPage && address >> 8 != debugMemPage + 1)
    {
        return;
    }

    uint8 x = (uint8)(address & 0x000F);
    uint8 y = (address & 0x01F0) >> 4;
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD cursorPos = { 7, 2 };
    cursorPos.X += x * 3;
    cursorPos.Y += y;
    SetConsoleCursorPosition(console, cursorPos);
    printf("%02X", val);
}

// TODO: Handle interrupts as a result of reads and writes
void cpuWrite(uint16 address, uint8 value)
{
    if (address < 0x2000)
    {
        // map to the internal 2kb ram
        ram[address & 0x07FF] = value;
        renderMemCell(address, value);
    }
    else if (address < 0x4000)
    {
        // map to the ppu register based on the last 3 bits
        // http://wiki.nesdev.com/w/index.php/PPU_registers
        uint16 masked = address & 0x0007;
        switch (masked)
        {
            case 0x00: ppu.control = value; break;
            case 0x01: ppu.mask = value; break;
            case 0x02: ppu.status = value; break;
            case 0x03: ppu.oamAddress = value; break;
            case 0x04: ppu.oamData = value; break;
            case 0x05: ppu.scroll = value; break;
            case 0x06: ppu.address = value; break;
            case 0x07: ppu.data = value; break;
        }

        renderMemCell(address, value);
    }
    else if (address < 0x4020)
    {
        // map to the apu/io registers
        // http://wiki.nesdev.com/w/index.php/2A03
        switch (address)
        {
            case 0x4000: apu.square1Vol = value; break;
            case 0x4001: apu.square1Sweep = value; break;
            case 0x4002: apu.square1Lo = value; break;
            case 0x4003: apu.square1Hi = value; break;
            case 0x4004: apu.square2Vol = value; break;
            case 0x4005: apu.square2Sweep = value; break;
            case 0x4006: apu.square2Lo = value; break;
            case 0x4007: apu.square2Vol = value; break;
            case 0x4008: apu.triangleLinear = value; break;
            case 0x400A: apu.triangleLo = value; break;
            case 0x400B: apu.triangleHi = value; break;
            case 0x400C: apu.noiseVol = value; break;
            case 0x400E: apu.noiseLo = value; break;
            case 0x400F: apu.noiseHi = value; break;
            case 0x4010: apu.dmcFrequency = value; break;
            case 0x4011: apu.dmcRaw = value; break;
            case 0x4012: apu.dmcStart = value; break;
            case 0x4013: apu.dmcLength = value; break;
            case 0x4014: ppu.oamDma = value; break;
            case 0x4015: apu.channelStatus = value; break;
            case 0x4016: joy1 = value; break;
            case 0x4017: joy2 = value; break;
            default: return;
        }

        renderMemCell(address, value);
    }
    // Cartridge space (logic depends on the mapper)
    // TODO: allow multiple mappers, for now using 000 since its the one in the test case
    else if (address > 0x6000 && address < 0x8000)
    {
        cartRam[address - 0x6000] = value;
        renderMemCell(address, value);
    }
}

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

struct Operation
{
    uint8 instruction;
    OpCode opCode;
    AddressingMode addressMode;
};

Operation operations[] = {
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

void setConsoleSize(int16 cols, int16 rows, int charWidth, int charHeight)
{
    // console buffer can never be smaller than the window and you cant set them in tandem
    // so we have to shrink the window to nothing, adjust the buffer then resize back to where we want
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    SMALL_RECT windowRect = {0, 0, 1, 1};
    BOOL result = SetConsoleWindowInfo(console, TRUE, &windowRect);

    COORD newSize = {cols, rows};
    result = SetConsoleScreenBufferSize(console, newSize);

    CONSOLE_FONT_INFOEX fontInfo;
    fontInfo.cbSize = sizeof(fontInfo);
    fontInfo.nFont = 0;
    fontInfo.dwFontSize.X = charWidth;
    fontInfo.dwFontSize.Y = charHeight;
    fontInfo.FontFamily = FF_DONTCARE;
    fontInfo.FontWeight = FW_NORMAL;
    wcscpy_s(fontInfo.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(console, false, &fontInfo);

    CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
    result = GetConsoleScreenBufferInfo(console, &bufferInfo);

    windowRect = {0, 0, bufferInfo.dwMaximumWindowSize.X - 1, bufferInfo.dwMaximumWindowSize.Y - 1 };
    result = SetConsoleWindowInfo(console, TRUE, &windowRect);
}

void hideCursor()
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.dwSize = 10;
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(console, &cursorInfo);
}

void showCursor()
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.dwSize = 10;
    cursorInfo.bVisible = true;
    SetConsoleCursorInfo(console, &cursorInfo);
}

void printInstruction(uint16 address, uint8 opcode, uint8 p1, uint8 p2)
{
    Operation op = operations[opcode];
    printf("0x%04X %s", address, opCodeNames[op.opCode]);
    
    switch (op.addressMode)
    {
    case Absolute:
        printf(" %02X%02X", p2, p1);
        break;
    case AbsoluteX:
        printf(" %02X%02X,X", p2, p1);
        break;
    case AbsoluteY:
        printf(" %02X%02X,Y", p2, p1);
        break;
    case Immediate:
        printf(" #%02X", p1);
        break;
    case Indirect:
        printf(" (%02X%02X)", p2, p1);
        break;
    case IndirectX:
        printf(" (%02X,X)", p1);
        break;
    case IndirectY:
        printf(" (%02X),Y", p1);
        break;
    case Relative:
        printf(" %d", (int8)p1);
        break;
    case ZeroPage:
        printf(" %02X", p1);
        break;
    case ZeroPageX:
        printf(" %02X,X", p1);
        break;
    case ZeroPageY:
        printf(" %02X,Y", p1);
        break;
    }

    printf("\n");
}

void printInstruction(uint16 address)
{
    uint8 opcode = cpuRead(address);
    uint8 p1 = cpuRead(address + 1);
    uint8 p2 = cpuRead(address + 2);
    printInstruction(address, opcode, p1, p2);
}

void printInstruction()
{
    printInstruction(cpu.instAddr, cpu.inst, cpu.p1, cpu.p2);
}

static bool memViewRendered = false;
static int64 frameElapsed = 1;
static int64 cpuFreq = 1;

void drawFps()
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD cursorPos = { 100, 3 };
    SetConsoleCursorPosition(console, cursorPos);

    int32 msPerFrame = (int32)((1000 * frameElapsed) / cpuFreq);
    if (msPerFrame)
    {
        int32 framesPerSecond = 1000 / msPerFrame;
        printf("MS Per Frame = %08d FPS: %d", msPerFrame, framesPerSecond);
    }
}

bool asciiMode = false;

void renderMemoryView()
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD cursorPos = {};
    SetConsoleCursorPosition(console, cursorPos);

    printf("CPU (M)emory\n");
    printf("      ");
    for (int i = 0; i < 16; ++i)
    {
        printf(" %02X", i);
    }

    cursorPos.Y = 2;
    SetConsoleCursorPosition(console, cursorPos);

    const int ROWS_TO_DISPLAY = 32;

    uint16 address = ((uint16)debugMemPage) << 8;
    while (cursorPos.Y < ROWS_TO_DISPLAY + 2)
    {
        //TODO: do proper console manipulation to put these where I want it
        printf("0x%04X", address);

        for (int i = 0; i < 16; ++i)
        {
            uint8 d = cpuRead(address + i);
            if (!asciiMode || !isprint(d))
            {
                printf(" %02X", d);
            }
            else
            {
                printf("  %c", (char)d);
            }
        }

        ++cursorPos.Y;
        address += 16;
        SetConsoleCursorPosition(console, cursorPos);
    }
}

void debugView()
{
    if (!memViewRendered)
    {
        renderMemoryView();
        memViewRendered = true;
    }

    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD cursorPos = { 56, 0 };
    SetConsoleCursorPosition(console, cursorPos);

    printf("CPU Registers");

    ++cursorPos.Y;
    SetConsoleCursorPosition(console, cursorPos);

    printf("A=%02X X=%02X Y=%02X", cpu.accumulator, cpu.x, cpu.y);

    ++cursorPos.Y;
    SetConsoleCursorPosition(console, cursorPos);
    printf("PC=%04hX S=0x01%02X P=%02X", cpu.pc, cpu.stack, cpu.status);

    ++cursorPos.Y;
    SetConsoleCursorPosition(console, cursorPos);
    
    uint8 carry = cpu.status & 0x01;
    uint8 zero = (cpu.status & 0x02) >> 1;
    uint8 interuptDisable = (cpu.status & 0x04) >> 2;
    uint8 overflow = (cpu.status & 0x40) >> 6;
    uint8 negative = (cpu.status & 0x80) >> 7;
    printf("C=%d Z=%d I=%d V=%d N=%d", carry, zero, interuptDisable, overflow, negative);

    cursorPos.Y += 3;
    SetConsoleCursorPosition(console, cursorPos);
    printf("                                  ");
    SetConsoleCursorPosition(console, cursorPos);
    printf("Current Inst: ");
    printInstruction(cpu.instAddr);

    drawFps();
}

void readValue()
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD cursorPos = {};
    cursorPos.X = 0;
    cursorPos.Y = 35;
    SetConsoleCursorPosition(console, cursorPos);
    printf("               ");
    SetConsoleCursorPosition(console, cursorPos);

    printf("Address: ");
    showCursor();
    uint16 address = 0;
    scanf("%hx", &address);
    hideCursor();

    SetConsoleCursorPosition(console, cursorPos);
    printf("               ");
    SetConsoleCursorPosition(console, cursorPos);
    printf("Result: %02X   ", cpuRead(address));
}

void updateNegative(uint8 val)
{
    cpu.status &= 0b01111111;
    cpu.status |= val & 0b10000000;
}

void updateZero(uint8 val)
{
    cpu.status &= 0b11111101;
    if (val == 0)
    {
        cpu.status |= 0b00000010;
    }
}

void loadOperands()
{
    uint8 opcode = cpuRead(cpu.instAddr);
    Operation op = operations[opcode];
    switch (op.addressMode)
    {
        case Absolute:
        case AbsoluteX:
        case AbsoluteY:
        case Indirect:
            cpu.p1 = cpuRead(cpu.pc++);
            cpu.p2 = cpuRead(cpu.pc++);
            break;

        case Immediate:
        case IndirectX:
        case IndirectY:
        case Relative:
        case ZeroPage:
        case ZeroPageX:
        case ZeroPageY:
            cpu.p1 = cpuRead(cpu.pc++);
            break;
    }
}

uint16 readWord(uint16 base)
{
    uint8 lo = cpuRead(base);
    uint8 hi = cpuRead(base + 1);
    return ((uint16)hi << 8) + lo;
}

uint16 calcAddress(AddressingMode addressMode)
{
    switch (addressMode)
    {
        case Absolute:
        {
            return (uint16)cpu.p2 << 8 | cpu.p1;
        }
        case AbsoluteX:
        {
            uint16 a = (uint16)cpu.p2 << 8 | cpu.p1;
            return a + cpu.x;
        }
        case AbsoluteY:
        {
            uint16 a = (uint16)cpu.p2 << 8 | cpu.p1;
            return a + cpu.y;
        }
        case Indirect:
        {
            uint16 base = (uint16)cpu.p2 << 8 | cpu.p1;
            return readWord(base);
        }
        case IndirectX:
        {
            uint8 base = cpu.p1 + cpu.x;
            return readWord(base);
        }
        case IndirectY:
        {
            uint16 lo = cpuRead(cpu.p1) + (uint16)cpu.y;
            uint16 hi = cpuRead(cpu.p1 + 1);
            return (hi << 8) + lo;
        }
        case Relative:
        {
            return (cpu.instAddr + 2) + (int8)cpu.p1;
        }
        case ZeroPage:
        {
            return cpu.p1;
        }
        case ZeroPageX:
        {
            return cpu.p1 + cpu.x;
        }
        case ZeroPageY:
        {
            return cpu.p1 + cpu.y;
        }
    }
}

uint8 readData(AddressingMode addressMode)
{
    if (addressMode == Immediate)
    {
        return cpu.p1;
    }

    if (addressMode == Accumulator)
    {
        return cpu.accumulator;
    }

    uint16 address = calcAddress(addressMode);
    return cpuRead(address);
}

void writeData(AddressingMode addressMode, uint8 val)
{
    if (addressMode == Accumulator)
    {
        cpu.accumulator = val;
        return;
    }

    uint16 address = calcAddress(addressMode);
    cpuWrite(address, val);
}

void push(uint8 val)
{
    cpuWrite(0x0100 | cpu.stack, val);
    cpu.stack--;
}

void pushWord(uint16 val)
{
    uint8 lo = val & 0x00FF;
    uint8 hi = (val & 0xFF00) >> 8;
    push(hi);
    push(lo);
}

uint8 pull()
{
    ++cpu.stack;
    return cpuRead(0x0100 | cpu.stack);
}

uint16 pullWord()
{
    uint16 lo = pull();
    uint16 hi = pull();
    return (hi << 8) + lo;
}

void executeInstruction()
{
    Operation op = operations[cpu.inst];
    switch (op.opCode)
    {
        case ADC:
        {
            uint8 data = readData(op.addressMode);
            uint8 carry = cpu.status & 0b00000001;
            uint8 sum = data + carry + cpu.accumulator;
            
            // Slightly complicated, but overflow only kicks in if signs are different
            uint8 overflow = (cpu.accumulator ^ sum) & (data ^ sum) & 0b10000000;
            cpu.status &= 0b10111110;
            cpu.status |= overflow >> 1;

            if ((uint16)data + carry + cpu.accumulator > 0xFF)
            {
                cpu.status |= 0b00000001;
            }

            cpu.accumulator = sum;
            updateNegative(cpu.accumulator);
            updateZero(cpu.accumulator);
        }
        break;
        case AND:
        {
            uint8 data = readData(op.addressMode);
            cpu.accumulator &= data;
            updateNegative(cpu.accumulator);
            updateZero(cpu.accumulator);
        }
        break;
        case ASL:
        {
            uint8 data = readData(op.addressMode);
            cpu.status &= ~0b00000001;
            cpu.status |= (data & 0b10000000) >> 7;
            data <<= 1;
            updateNegative(data);
            updateZero(data);
            writeData(op.addressMode, data);
        }
        break;
        case BCC:
        {
            if (!(cpu.status & 0b00000001))
            {
                cpu.pc = calcAddress(op.addressMode);
            }
        }
        break;
        case BCS:
        {
            if (cpu.status & 0b00000001)
            {
                cpu.pc = calcAddress(op.addressMode);
            }
        }
        break;
        case BEQ:
        {
            if (cpu.status & 0b00000010)
            {
                cpu.pc = calcAddress(op.addressMode);
            }
        }
        break;
        case BIT:
        {
            uint8 data = readData(op.addressMode);
            cpu.status &= 0b00111101;
            cpu.status |= (data & 0b11000000);
            
            if ((data & cpu.accumulator) == 0)
            {
                cpu.status |= (data & 0b00000010);
            }
        }
        break;
        case BMI:
        {
            if (cpu.status & 0b10000000)
            {
                cpu.pc = calcAddress(op.addressMode);
            }
        }
        break;
        case BNE:
        {
            if (!(cpu.status & 0b00000010))
            {
                cpu.pc = calcAddress(op.addressMode);
            }
        }
        break;
        case BPL:
        {
            if (!(cpu.status & 0b10000000))
            {
                cpu.pc = calcAddress(op.addressMode);
            }
        }
        break;
        case BRK:
        {
            pushWord(cpu.instAddr + 2);
            push(cpu.status | 0b00110000);
            cpu.pc = readWord(0xFFFE);
        }
        break;
        case BVC:
        {
            if (!(cpu.status & 0b01000000))
            {
                cpu.pc = calcAddress(op.addressMode);
            }
        }
        break;
        case BVS:
        {
            if (cpu.status & 0b01000000)
            {
                cpu.pc = calcAddress(op.addressMode);
            }
        }
        break;
        case CLC:
        {
            cpu.status &= 0b11111110;
        }
        break;
        case CLD:
        {
            cpu.status &= 0b11110111;
        }
        break;
        case CLI:
        {
            cpu.status &= 0b11111011;
        }
        break;
        case CLV:
        {
            cpu.status &= 0b10111111;
        }
        break;
        case CMP:
        {
            uint8 data = readData(op.addressMode);
            uint8 result = cpu.accumulator - data;
            cpu.status &= 0b01111100;
            if ((int8)result >= 0)
            {
                cpu.status |= 0b00000011;
            }

            updateNegative(result);
            updateZero(result);
        }
        break;
        case CPX:
        {
            uint8 data = readData(op.addressMode);
            uint8 result = cpu.x - data;
            cpu.status &= 0b01111100;
            if ((uint8)result >= 0)
            {
                cpu.status |= 0b00000011;
            }

            updateNegative(result);
            updateZero(result);
        }
        break;
        case CPY:
        {
            uint8 data = readData(op.addressMode);
            uint8 result = cpu.y - data;
            cpu.status &= 0b01111100;
            if ((uint8)result >= 0)
            {
                cpu.status |= 0b00000011;
            }

            updateNegative(result);
            updateZero(result);
        }
        break;
        case DEC:
        {
            uint8 data = readData(op.addressMode);
            --data;
            updateNegative(data);
            updateZero(data);
            writeData(op.addressMode, data);
        }
        break;
        case DEX:
        {
            --cpu.x;
            updateNegative(cpu.x);
            updateZero(cpu.x);
        }
        break;
        case DEY:
        {
            --cpu.y;
            updateNegative(cpu.y);
            updateZero(cpu.y);
        }
        break;
        case EOR:
        {
            uint8 data = readData(op.addressMode);
            cpu.accumulator ^= data;
            updateNegative(cpu.accumulator);
            updateZero(cpu.accumulator);
        }
        break;
        case INC:
        {
            uint8 data = readData(op.addressMode);
            ++data;
            updateNegative(data);
            updateZero(data);
            writeData(op.addressMode, data);
        }
        break;
        case INX:
        {
            ++cpu.x;
            updateNegative(cpu.x);
            updateZero(cpu.x);
        }
        break;
        case INY:
        {
            ++cpu.y;
            updateNegative(cpu.y);
            updateZero(cpu.y);
        }
        break;
        case JMP:
        {
            cpu.pc = calcAddress(op.addressMode);
        }
        break;
        case JSR:
        {
            pushWord(cpu.instAddr + 2U);
            cpu.pc = calcAddress(op.addressMode);
        }
        break;
        case LDA:
        {
            uint8 data = readData(op.addressMode);
            cpu.accumulator = data;
            updateNegative(cpu.accumulator);
            updateZero(cpu.accumulator);
        }
        break;
        case LDX:
        {
            uint8 data = readData(op.addressMode);
            cpu.x |= data;
            updateNegative(cpu.x);
            updateZero(cpu.x);
        }
        break;
        case LDY:
        {
            uint8 data = readData(op.addressMode);
            cpu.y |= data;
            updateNegative(cpu.y);
            updateZero(cpu.y);
        }
        break;
        case LSR:
        {
            uint8 data = readData(op.addressMode);
            cpu.status &= ~0b00000001;
            cpu.status |= data & 0b00000001;
            data >>= 1;
            updateZero(data);
            writeData(op.addressMode, data);
        }
        break;
        case ORA:
        {
            uint8 data = readData(op.addressMode);
            cpu.accumulator |= data;
            updateNegative(cpu.accumulator);
            updateZero(cpu.accumulator);
        }
        break;
        case PHA:
        {
            push(cpu.accumulator);
        }
        break;
        case PHP:
        {
            push(cpu.status | 0b00110000);
        }
        break;
        case PLA:
        {
            cpu.accumulator = pull();
            updateNegative(cpu.accumulator);
            updateZero(cpu.accumulator);
        }
        break;
        case PLP:
        {
            cpu.status = pull();
        }
        break;
        case ROL:
        {
            uint8 data = readData(op.addressMode);
            uint8 carry = cpu.status & 0b00000001;
            cpu.status &= ~0b00000001;
            cpu.status |= (data & 0b10000000) >> 7;
            data = (data << 1) | carry;
            updateNegative(data);
            updateZero(data);
            writeData(op.addressMode, data);
        }
        break;
        case ROR:
        {
            uint8 data = readData(op.addressMode);
            uint8 carry = cpu.status & 0b00000001;
            cpu.status &= ~0b00000001;
            cpu.status |= data & 0b00000001;
            data = (data >> 1) | (carry << 7);
            updateNegative(data);
            updateZero(data);
            writeData(op.addressMode, data);
        }
        break;
        case RTI:
        {
            cpu.status = pull();
            cpu.pc = pullWord();
        }
        break;
        case RTS:
        {
            cpu.pc = pullWord() + 1;
        }
        break;
        case SBC:
        {
            uint8 data = readData(op.addressMode);
            data = ~data; // not sure if this will work but my understanding is the logic is the same 
            uint8 carry = cpu.status & 0b00000001;
            uint8 sum = data + carry + cpu.accumulator;

            // Slightly complicated, but overflow only kicks in if signs are different
            uint8 overflow = (cpu.accumulator ^ sum) & (data ^ sum) & 0b10000000;
            cpu.status &= 0b10111110;
            cpu.status |= overflow >> 1;

            if ((uint16)data + carry + cpu.accumulator > 0xFF)
            {
                cpu.status |= 0b00000001;
            }

            cpu.accumulator = sum;
            updateNegative(cpu.accumulator);
            updateZero(cpu.accumulator);
        }
        break;
        case SEC:
        {
            cpu.status |= 0b00000001;
        }
        break;
        case SED:
        {
            cpu.status |= 0b00001000;
        }
        break;
        case SEI:
        {
            cpu.status |= 0b00000100;
        }
        break;
        case STA:
        {
            writeData(op.addressMode, cpu.accumulator);
        }
        break;
        case STX:
        {
            writeData(op.addressMode, cpu.x);
        }
        break;
        case STY:
        {
            writeData(op.addressMode, cpu.y);
        }
        break;
        case TAX:
        {
            cpu.x = cpu.accumulator;
            updateNegative(cpu.x);
            updateZero(cpu.x);
        }
        break;
        case TAY:
        {
            cpu.y = cpu.accumulator;
            updateNegative(cpu.y);
            updateZero(cpu.y);
        }
        break;
        case TSX:
        {
            cpu.x = cpu.stack;
            updateNegative(cpu.x);
            updateZero(cpu.x);
        }
        break;
        case TXA:
        {
            cpu.accumulator = cpu.x;
            updateNegative(cpu.accumulator);
            updateZero(cpu.accumulator);
        }
        break;
        case TXS:
        {
            cpu.stack = cpu.x;
            updateNegative(cpu.stack);
            updateZero(cpu.stack);
        }
        break;
        case TYA:
        {
            cpu.accumulator = cpu.y;
            updateNegative(cpu.accumulator);
            updateZero(cpu.accumulator);
        }
        break;
    }
}

// This read and write is based on mapper000, will expand later
uint8 ppuRead(uint16 address)
{
    if (address < 0x2000)
    {
        return chrRom[address];
    }

    if (address < 0x3F00)
    {
        address &= 0x2FFF;
        // TODO: handle mirroring configuration
        return vram[address & 0x07FF];
    }

    // palettes http://wiki.nesdev.com/w/index.php/PPU_palettes
    address &= 0x001F;
    switch (address)
    {
        case 0x10: return paletteRam[0x00];
        case 0x14: return paletteRam[0x04];
        case 0x18: return paletteRam[0x08];
        case 0x1C: return paletteRam[0x0C];
        default: return paletteRam[address];
    }
}

void ppuWrite(uint16 address, uint8 val)
{
    if (address < 0x2000)
    {
        chrRom[address] = val;
    }

    if (address < 0x3F00)
    {
        address &= 0x2FFF;
        // TODO: handle mirroring configuration
        vram[address & 0x07FF] = val;
    }

    // palettes http://wiki.nesdev.com/w/index.php/PPU_palettes
    address &= 0x001F;
    switch (address)
    {
        case 0x10: paletteRam[0x00] = val;
        case 0x14: paletteRam[0x04] = val;
        case 0x18: paletteRam[0x08] = val;
        case 0x1C: paletteRam[0x0C] = val;
        default: paletteRam[address] = val;
    }
}

#define NES_SCREEN_HEIGHT 240
#define NES_SCREEN_WIDTH 256

static uint32 ppuCycle = 0;

static uint8 nametableByte;
static uint8 attributetableByte;
static uint8 patternLo;
static uint8 patternHi;

static CHAR_INFO palette[0x4F] = {
    { '#', 0 },
    { '#', 0x10 },
    { '#', 0x20 },
    { '#', 0x30 },
    { '#', 0x40 },
    { '#', 0x50 },
    { '#', 0x60 },
    { '#', 0x70 },
    { '#', 0x80 },
    { '#', 0x90 },
    { '#', 0xA0 },
    { '#', 0xB0 },
    { '#', 0xD0 },
    { '#', 0xE0 },
    { '#', 0xF0 },
    { ' ', 0 },
    { ' ', 0x10 },
    { ' ', 0x20 },
    { ' ', 0x30 },
    { ' ', 0x40 },
    { ' ', 0x50 },
    { ' ', 0x60 },
    { ' ', 0x70 },
    { ' ', 0x80 },
    { ' ', 0x90 },
    { ' ', 0xA0 },
    { ' ', 0xB0 },
    { ' ', 0xD0 },
    { ' ', 0xE0 },
    { ' ', 0xF0 },
    { '.', FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED },
    { '.', FOREGROUND_GREEN | FOREGROUND_RED },
    { '.', FOREGROUND_RED },
    { '.', FOREGROUND_BLUE | FOREGROUND_RED },
    { '.', FOREGROUND_BLUE },
    { '.', FOREGROUND_GREEN },
    { '.', BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED },
    { '.', BACKGROUND_GREEN | BACKGROUND_RED },
    { '.', BACKGROUND_RED },
    { '.', BACKGROUND_BLUE | BACKGROUND_RED },
    { '.', BACKGROUND_BLUE },
    { '.', BACKGROUND_GREEN },
    { '.', BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_RED },
    { '.', BACKGROUND_RED | FOREGROUND_BLUE },
    { '.', BACKGROUND_BLUE | BACKGROUND_RED },
    { '.', BACKGROUND_GREEN | BACKGROUND_RED },
    { '#', 0xBF },
    { '#', 0x0F },
    { '#', 0x1F },
    { '#', 0x2F },
    { '#', 0x3F },
    { '#', 0x4F },
    { '#', 0x5F },
    { '#', 0x6F },
    { '#', 0x7F },
    { '#', 0x8F },
    { '#', 0x9F },
    { '#', 0xAF },
    { '#', 0xDF },
    { '#', 0xEF },
    { '#', 0xFF },
};

static CHAR_INFO pixels[NES_SCREEN_WIDTH * NES_SCREEN_HEIGHT];

void drawRect(int paletteIndex, int x, int y, int width, int height)
{
    for (int py = y; py < NES_SCREEN_HEIGHT && py < y + height; ++py)
    {
        for (int px = x; px < NES_SCREEN_WIDTH && px < x + width; ++px)
        {
            pixels[NES_SCREEN_WIDTH * py + px] = palette[paletteIndex];
        }
    }
}

void gameView()
{
    drawRect(5, 0, 0, 16, 16);
    drawRect(10, 100, 100, 25, 25);

    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    SMALL_RECT location = {0, 0, NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT };
    WriteConsoleOutputA(console, pixels, { NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT }, { 0, 0 }, &location);
}

void activateRenderMode()
{
    setConsoleSize(NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT, 4, 4);
    renderMode = true;
}

void activateDebugMode()
{
    setConsoleSize(160, 40, 8, 16);
    renderMode = false;
}

bool singleStepMode = true;

int main(int argc, char *argv[])
{
    activateDebugMode();
    hideCursor();

    cpu.status = 0x34;
    cpu.stack = 0xfd;

    FILE* testFile = fopen("data/01-basics.nes", "rb");
    fseek(testFile, 0, SEEK_END);
    size_t fileSize = ftell(testFile);
    char* contents = new char[fileSize];

    rewind(testFile);
    size_t bytesRead = fread(contents, sizeof(char), fileSize, testFile);

    fclose(testFile);

    uint8* readHead = (uint8*)contents;

    // Initialize the PC to the value of FFFC = lo and FFFD = hi.
    // needs to be done after the mapper is created so we grab the right code from the rom
    INESHeader* header = (INESHeader*)readHead;
    readHead += sizeof(INESHeader);

    map000prgRomBank1 = map000prgRomBank2 = (uint8*)readHead;
    readHead += 16 * 1024 * header->prgRomSize;

    if (header->prgRomSize == 2)
    {
        map000prgRomBank2 += 0x4000;
    }

    chrRom = (uint8*)readHead;
    readHead += 8 * 1024 * header->chrRomSize;

    cpu.instAddr = cpu.pc = readWord(0xFFFC);

    debugView();

    LARGE_INTEGER counterFrequency;
    QueryPerformanceFrequency(&counterFrequency);
    cpuFreq = counterFrequency.QuadPart;

    LARGE_INTEGER lastCounter;
    QueryPerformanceCounter(&lastCounter);
    int64 refreshRate = cpuFreq / 30;
    LARGE_INTEGER lastRender = lastCounter;

    bool stepRequested = false;
    while (running)
    {
        if (_kbhit())
        {
            char input = _getch();
            if (input == 'q')
            {
                running = false;
            }

            if (input == 'd')
            {
                debugMemPage += 2;
                memViewRendered = false;
            }

            if (input == 'a')
            {
                debugMemPage -= 2;
                memViewRendered = false;
            }

            if (input == 'c')
            {
                asciiMode = !asciiMode;
            }

            if (input == 'p')
            {
                singleStepMode = !singleStepMode;
            }

            stepRequested = input == ' ';

            if (input == 'r')
            {
                if (renderMode)
                {
                    activateDebugMode();
                }
                else
                {
                    activateRenderMode();
                }
            }
        }
        
        if (!singleStepMode || stepRequested)
        {
            // TOOD: implement a cycle wait based on instruction
            // Or run correctly per cycle using the t member
            cpu.inst = cpuRead(cpu.pc++);
            Operation op = operations[cpu.inst];
            if (!singleStepMode && op.opCode == KILL)
            {
                --cpu.pc;
                singleStepMode = true;
            }
            else
            {
                loadOperands();
                executeInstruction();
            }
            
            cpu.instAddr = cpu.pc;
            stepRequested = false;
        }

        // TODO: This timing is Garbage, but all I need at the moment is to slow the renderer so the sim can run full force.
        LARGE_INTEGER preRenderCounter;
        QueryPerformanceCounter(&preRenderCounter);
        int64 frameGap = preRenderCounter.QuadPart - lastRender.QuadPart;

        if (frameGap > refreshRate)
        {
            if (renderMode)
            {
                gameView();
            }
            else
            {
                debugView();
            }

            frameElapsed = frameGap;
            lastRender = preRenderCounter;
        }
    }

    return 0;
}