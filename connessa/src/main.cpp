#include <stdio.h>
#include <conio.h>

// Some common typedefs that I tend to use for uniformity
typedef char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

struct CPU
{
    // Actual Registers on the system
    uint16 pc;
    uint8 stack;
    uint8 status;
    uint8 accumulator;
    uint8 x;
    uint8 y;

    // Values to abstract the internal magic of the 6502
    // TODO: Use these to handle proper read and write timings
    uint8 t; // Cycle count since starting the operation
    uint8 inst; // Currently executing instruction
    uint16 instAddr; // Address of current instruction, used for debugging
};

struct PPU
{
    uint8 control;
    uint8 mask;
    uint8 status;
    uint8 oamAddress;
    uint8 oamData;
    uint8 address;
    uint8 data;
    uint8 oamdma;
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

static uint8* map000prgRomBank1;
static uint8* map000prgRomBank2;
static uint8* map000prgRam;

static uint8 vram[2 * 1024] = {};
static uint8* chrRom;

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
    }

    if (address < 0x4020)
    {
        // map to the apu/io registers
        // http://wiki.nesdev.com/w/index.php/2A03
    }

    // Cartridge space (logic depends on the mapper)
    // TODO: allow multiple mappers, for now using 000 since its the one in the test case
    if (address < 0x8000)
    {
        // INVALID READ ON THIS MAPPER
        // TODO: Figure out a way to handle these kinds of errors, not sure what hardware would do
    }

    if (address < 0xC000)
    {
        return map000prgRomBank1[address - 0x8000];
    }

    return map000prgRomBank2[address - 0xC000];
}

void cpuWrite(uint16 address, uint8 value)
{
    if (address < 0x2000)
    {
        // map to the internal 2kb ram
        ram[address & 0x07FF] = value;
    }
    else if (address < 0x4000)
    {
        // map to the ppu register based on the last 3 bits
        // http://wiki.nesdev.com/w/index.php/PPU_registers
    }
    else if (address < 0x4020)
    {
        // map to the apu/io registers
        // http://wiki.nesdev.com/w/index.php/2A03
    }
    // Cartridge space (logic depends on the mapper)
    // TODO: allow multiple mappers, for now using 000 since its the one in the test case
    else
    {
        // INVALID Write ON THIS MAPPER
        // TODO: Figure out a way to handle these kinds of errors, not sure what hardware would do
        // probably nothing, mapper000 is rom so you can't physically write to it
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
    CPM,
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
    LDS,
    LDX,
    LDY,
    LSR,
    NOP,
    ORA,
    PHA,
    PHP,
    PLA,
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
    IndirectY,
    Relative,
    ZeroPage,
    ZeroPageX,
    ZeroPageY,
};

struct Operation
{
    OpCode code;
    AddressingMode mode;
};

Operation operations[] = {
    {BRK, Implied}
};


int main(int argc, char *argv[])
{
    cpu.status = 0x34;
    cpu.stack = 0xfd;

    FILE* testFile = fopen("data/registers.nes", "rb");
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

    cpu.pc = cpuRead(0xFFFD);
    cpu.pc = (cpu.pc << 8) | cpuRead(0xFFFC);

    while (running)
    {
        char input = _getch();
        if (input == 'q')
        {
            break;
        }

        if (input == ' ')
        {
            cpu.instAddr = cpu.pc;
            cpu.inst = cpuRead(cpu.pc++);
            switch (cpu.inst)
            {
                case 0x00:
                    printf("BRK\n");
                    break;

                case 0x01:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    printf("ORA (%x,X)\n", p1);
                }
                break;

                case 0x05:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    printf("ORA %x\n", p1);
                }
                break;

                case 0x06:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    printf("ASL %x\n", p1);
                }
                break;

                case 0x08:
                    printf("PHP\n");
                    break;

                case 0x09:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    cpu.accumulator |= p1;
                    printf("ORA #%x\n", p1);
                }
                break;

                case 0x0A:
                    printf("ASL A\n");
                    break;

                case 0x0D:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("ORA %x%x\n", p1, p2);
                }
                break;

                case 0x0E:
                {
                    uint8 lo = cpuRead(cpu.pc++);
                    uint8 hi = cpuRead(cpu.pc++);
                    printf("ASL %x%x\n", hi, lo);
                }
                break;

                case 0x10:
                {
                    uint8 offset = cpuRead(cpu.pc++);
                    printf("BPL %d\n", (int8)offset);
                }
                break;

                case 0x11:
                    printf("ORA (%x),Y\n", cpuRead(cpu.pc++));
                    break;
                case 0x15:
                    printf("ORA %x,X\n", cpuRead(cpu.pc++));
                    break;
                case 0x16:
                    printf("ASl %x,X\n", cpuRead(cpu.pc++));
                    break;
                case 0x18:
                    printf("CLC\n");
                    break;
                case 0x19:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("ORA %x%x,Y\n", p1, p2);
                }
                break;
                case 0x1D:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("ORA %x%x,X\n", p1, p2);
                }
                break;
                case 0x1E:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("ASL %x%x,X\n", p1, p2);
                }
                break;

                case 0x20:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("JSR %x%x\n", p1, p2);
                }
                break;
                case 0x21:
                    printf("AND (%x,X)\n", cpuRead(cpu.pc++));
                    break;
                case 0x24:
                    printf("BIT %x\n", cpuRead(cpu.pc++));
                    break;
                case 0x25:
                    printf("AND %x\n", cpuRead(cpu.pc++));
                    break;
                case 0x26:
                    printf("ROL %x\n", cpuRead(cpu.pc++));
                    break;
                case 0x28:
                    printf("PLP\n");
                    break;
                case 0x29:
                    printf("AND #%x\n", cpuRead(cpu.pc++));
                    break;
                case 0x2A:
                    printf("ROL A\n");
                    break;
                case 0x2C:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("BIT %x%x\n", p1, p2);
                }
                break;
                case 0x2D:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("AND %x%x\n", p1, p2);
                }
                break;
                case 0x2E:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("ROL %x%x\n", p1, p2);
                }
                break;

                case 0x30:
                    printf("BMI %x\n", cpuRead(cpu.pc++));
                    break;
                case 0x31:
                    printf("AND (%x),Y\n", cpuRead(cpu.pc++));
                    break;
                case 0x35:
                    printf("AND %x,X\n", cpuRead(cpu.pc++));
                    break;
                case 0x36:
                    printf("ROL %x,X\n", cpuRead(cpu.pc++));
                    break;
                case 0x38:
                    printf("SEC\n");
                    break;
                case 0x39:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("AND %x%x,X\n", p1, p2);
                }
                break;
                case 0x3D:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("AND %x%x,X\n", p1, p2);
                }
                break;
                case 0x3E:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("ROL %x%x,X\n", p1, p2);
                }
                break;

                case 0x40:
                    printf("RTI\n");
                    break;
                case 0x41:
                    printf("EOR (%x,X)\n", cpuRead(cpu.pc++));
                    break;
                case 0x45:
                    printf("EOR %x\n", cpuRead(cpu.pc++));
                    break;
                case 0x46:
                    printf("LSR %x\n", cpuRead(cpu.pc++));
                    break;
                case 0x48:
                    printf("PHA\n");
                    break;
                case 0x49:
                    printf("EOR #%x\n", cpuRead(cpu.pc++));
                    break;
                case 0x4A:
                    printf("LSR A\n");
                    break;
                case 0x4C:
                {
                    uint8 lo = cpuRead(cpu.pc++);
                    uint8 hi = cpuRead(cpu.pc++);
                    printf("JMP %x%x\n", hi, lo);
                }
                break;
                case 0x4D:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("EOR %x%x\n", p1, p2);
                }
                break;
                case 0x4E:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("LSR %x%x\n", p1, p2);
                }
                break;

                case 0x50:
                    printf("BVC %x\n", cpuRead(cpu.pc++));
                    break;
                case 0x51:
                    printf("EOR (%x),Y\n", cpuRead(cpu.pc++));
                    break;
                case 0x55:
                    printf("EOR %x,X\n", cpuRead(cpu.pc++));
                    break;
                case 0x56:
                    printf("LSR %x,X\n", cpuRead(cpu.pc++));
                    break;
                case 0x58:
                    printf("CLI A\n");
                    break;
                case 0x59:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("EOR %x%x,Y\n", p1, p2);
                }
                break;
                case 0x5D:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("EOR %x%x,Y\n", p1, p2);
                }
                break;
                case 0x5E:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("LSR %x%x,Y\n", p1, p2);
                }
                break;

                case 0x60:
                    printf("RTS\n");
                    break;
                case 0x61:
                    printf("ADC (%x,X)\n", cpuRead(cpu.pc++));
                    break;
                case 0x65:
                    printf("ADC %x\n", cpuRead(cpu.pc++));
                    break;
                case 0x66:
                    printf("ROR %x\n", cpuRead(cpu.pc++));
                    break;
                case 0x68:
                    printf("PLA\n");
                    break;
                case 0x69:
                    printf("ADC #%x\n", cpuRead(cpu.pc++));
                    break;
                case 0x6A:
                    printf("ROR A\n");
                    break;
                case 0x6C:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("JMP (%x%x)\n", p1, p2);
                }
                break;
                case 0x6D:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("ADC %x%x\n", p1, p2);
                }
                break;
                case 0x6E:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("ROR %x%x\n", p1, p2);
                }
                break;

                case 0x70:
                    printf("BVC %x\n", cpuRead(cpu.pc++));
                    break;
                case 0x71:
                    printf("ADC (%x),Y\n", cpuRead(cpu.pc++));
                    break;
                case 0x75:
                    printf("ADC %x,X\n", cpuRead(cpu.pc++));
                    break;
                case 0x76:
                    printf("ROR %x,X\n", cpuRead(cpu.pc++));
                    break;
                case 0x78:
                    printf("SEI\n");
                    break;
                case 0x79:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("ADC %x%x,Y\n", p1, p2);
                }
                break;
                case 0x7D:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("ADC %x%x,X\n", p1, p2);
                }
                break;
                case 0x7E:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("ROR %x%x,X\n", p1, p2);
                }
                break;

                case 0x81:
                    printf("STA (%x,X)\n", cpuRead(cpu.pc++));
                    break;
                case 0x84:
                    printf("STY %x\n", cpuRead(cpu.pc++));
                    break;
                case 0x85:
                    printf("STA %x\n", cpuRead(cpu.pc++));
                    break;
                case 0x86:
                    printf("STX #%x\n", cpuRead(cpu.pc++));
                    break;
                case 0x88:
                    printf("DEC\n");
                    break;
                case 0x8A:
                    printf("TXA\n");
                    break;
                case 0x8C:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("STX %x%x\n", p1, p2);
                }
                break;
                case 0x8D:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("STY %x%x\n", p1, p2);
                }
                break;
                case 0x8E:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("STA %x%x\n", p1, p2);
                }
                break;

                case 0x90:
                    printf("BCC %x\n", cpuRead(cpu.pc++));
                    break;
                case 0x91:
                    printf("STA (%x),Y\n", cpuRead(cpu.pc++));
                    break;
                case 0x94:
                    printf("STY %x,X\n", cpuRead(cpu.pc++));
                    break;
                case 0x95:
                    printf("STA %x,X\n", cpuRead(cpu.pc++));
                    break;
                case 0x96:
                    printf("STX %x,Y\n", cpuRead(cpu.pc++));
                    break;
                case 0x98:
                    printf("TYA\n");
                    break;
                case 0x99:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("STA %x%x,Y\n", p1, p2);
                }
                break;
                case 0x9A:
                    printf("TXS\n");
                    break;
                case 0x9D:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("STA %x%x,X\n", p1, p2);
                }
                break;

                case 0xA0:
                    printf("LDY #%x\n", cpuRead(cpu.pc++));
                    break;
                case 0xA1:
                    printf("LDA (%x,X)\n", cpuRead(cpu.pc++));
                    break;
                case 0xA2:
                    printf("LDX #%x\n", cpuRead(cpu.pc++));
                    break;
                case 0xA4:
                    printf("LDY %x\n", cpuRead(cpu.pc++));
                    break;
                case 0xA5:
                    printf("LDA %x\n", cpuRead(cpu.pc++));
                    break;
                case 0xA6:
                    printf("LDX %x\n", cpuRead(cpu.pc++));
                    break;
                case 0xA8:
                    printf("TAY\n");
                    break;
                case 0xA9:
                    printf("LDA #%x\n", cpuRead(cpu.pc++));
                    break;
                case 0xAA:
                    printf("TAX\n");
                    break;
                case 0xAC:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("LDY %x%x\n", p1, p2);
                }
                break;
                case 0xAD:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("LDA %x%x\n", p1, p2);
                }
                break;
                case 0xAE:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("LDX %x%x\n", p1, p2);
                }
                break;

                case 0xB0:
                    printf("BSC %x\n", cpuRead(cpu.pc++));
                    break;
                case 0xB1:
                    printf("LDA (%x),Y\n", cpuRead(cpu.pc++));
                    break;
                case 0xB4:
                    printf("LDY %x,X\n", cpuRead(cpu.pc++));
                    break;
                case 0xB5:
                    printf("LDA %x,X\n", cpuRead(cpu.pc++));
                    break;
                case 0xB6:
                    printf("LDX %x,Y\n", cpuRead(cpu.pc++));
                    break;
                case 0xB8:
                    printf("CLV\n");
                    break;
                case 0xB9:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("LDA %x%x,Y\n", p1, p2);
                }
                break;
                case 0xBA:
                    printf("TSX\n");
                    break;
                case 0xBC:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("LDY %x%x,X\n", p1, p2);
                }
                break;
                case 0xBD:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("LDA %x%x,X\n", p1, p2);
                }
                break;
                case 0xBE:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("LDX %x%x,Y\n", p1, p2);
                }
                break;

                case 0xC0:
                    printf("CPY #%x\n", cpuRead(cpu.pc++));
                    break;
                case 0xC1:
                    printf("CMP (%x,X)\n", cpuRead(cpu.pc++));
                    break;
                case 0xC4:
                    printf("CPY %x\n", cpuRead(cpu.pc++));
                    break;
                case 0xC5:
                    printf("CMP %x\n", cpuRead(cpu.pc++));
                    break;
                case 0xC6:
                    printf("DEC %x\n", cpuRead(cpu.pc++));
                    break;
                case 0xC8:
                    printf("INY\n");
                    break;
                case 0xC9:
                    printf("CMP #%x\n", cpuRead(cpu.pc++));
                    break;
                case 0xCA:
                    printf("DEC\n");
                    break;
                case 0xCC:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("CPY %x%x\n", p1, p2);
                }
                break;
                case 0xCD:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("CMP %x%x\n", p1, p2);
                }
                break;
                case 0xCE:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("DEC %x%x\n", p1, p2);
                }
                break;

                case 0xD0:
                    printf("BNE %x\n", cpuRead(cpu.pc++));
                    break;
                case 0xD1:
                    printf("CMP (%x),Y\n", cpuRead(cpu.pc++));
                    break;
                case 0xD5:
                    printf("CMP %x,X\n", cpuRead(cpu.pc++));
                    break;
                case 0xD6:
                    printf("DEC %x,X\n", cpuRead(cpu.pc++));
                    break;
                case 0xD8:
                    printf("CLD\n");
                    break;
                case 0xD9:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("CMP %x%x,Y\n", p1, p2);
                }
                break;
                case 0xDD:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("CMP %x%x,X\n", p1, p2);
                }
                break;
                case 0xDE:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("DEC %x%x,X\n", p1, p2);
                }
                break;

                case 0xE0:
                    printf("CPX #%x\n", cpuRead(cpu.pc++));
                    break;
                case 0xE1:
                    printf("SBC (%x,X)\n", cpuRead(cpu.pc++));
                    break;
                case 0xE4:
                    printf("CPX %x\n", cpuRead(cpu.pc++));
                    break;
                case 0xE5:
                    printf("SBC %x\n", cpuRead(cpu.pc++));
                    break;
                case 0xE6:
                    printf("INC %x\n", cpuRead(cpu.pc++));
                    break;
                case 0xE8:
                    printf("INX\n");
                    break;
                case 0xE9:
                    printf("SBC #%x\n", cpuRead(cpu.pc++));
                    break;
                case 0xEA:
                    printf("NOP\n");
                    break;
                case 0xEC:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("CPX %x%x\n", p1, p2);
                }
                break;
                case 0xED:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("SBC %x%x\n", p1, p2);
                }
                break;
                case 0xEE:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("INC %x%x\n", p1, p2);
                }
                break;

                case 0xF0:
                    printf("BEQ %x\n", cpuRead(cpu.pc++));
                    break;
                case 0xF1:
                    printf("SBC (%x),Y\n", cpuRead(cpu.pc++));
                    break;
                case 0xF5:
                    printf("SBC %x,X\n", cpuRead(cpu.pc++));
                    break;
                case 0xF6:
                    printf("INC %x,X\n", cpuRead(cpu.pc++));
                    break;
                case 0xF8:
                    printf("SED\n");
                    break;
                case 0xF9:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("SBC %x%x,Y\n", p1, p2);
                }
                break;
                case 0xFD:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("SBC %x%x,X\n", p1, p2);
                }
                break;
                case 0xFE:
                {
                    uint8 p1 = cpuRead(cpu.pc++);
                    uint8 p2 = cpuRead(cpu.pc++);
                    printf("INC %x%x,X\n", p1, p2);
                }
                break;

                default:
                    printf("Illegal Opcode: %x\n", cpu.inst);
            }
        }
    }

    return 0;
}