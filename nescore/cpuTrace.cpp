#include "cpuTrace.h"
#include <stdio.h>
#include <string.h>

FILE* logFile = 0;

// TODO: This is basically a low rent custom purpose string builder. maybe make a class to use in other places
static const char hexValues[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
int formatByte(char* dest, uint8 d)
{
    *dest++ = hexValues[(d & 0xF0) >> 4];
    *dest++ = hexValues[d & 0x0F];
    return 2;
}

int formatWord(char* dest, uint16 d)
{
    uint8 lo = d & 0x00FF;
    uint8 hi = (d & 0xFF00) >> 8;
    dest += formatByte(dest, hi);
    dest += formatByte(dest, lo);
    return 4;
}

int formatHex(char* dest, uint8 d)
{
    *dest++ = '$';
    formatByte(dest, d);
    return 3;
}

int formatAddress(char* dest, uint8 lo, uint8 hi)
{
    *dest++ = '$';
    dest += formatByte(dest, hi);
    dest += formatByte(dest, lo);
    return 5;
}

int formatAddress(char* dest, uint16 address)
{
    *dest++ = '$';
    return formatWord(dest, address) + 1;
}

int formatString(char* dest, const char* s, uint32 length)
{
    memcpy(dest, s, length);
    return length;
}

int formatString(char* dest, const char* s)
{
    return formatString(dest, s, (uint32)strlen(s));
}

int formatInstruction(char* dest, uint16 address, MOS6502* cpu, IBus* bus)
{
    uint8 opcode = bus->read(address);
    uint8 p1 = bus->read(address + 1);
    uint8 p2 = bus->read(address + 2);

    Operation op = operations[opcode];
    const char* opCodeName = opCodeNames[op.opCode];
    int32 opCodeLength = (int32)strlen(opCodeName);
    char* s = dest;
    *s++ = ' ';
    memcpy(s, opCodeName, opCodeLength);
    s += opCodeLength;
    *s++ = ' ';

    address = cpu->calcAddress(op.addressMode, address, p1, p2);

    switch (op.addressMode)
    {
        case Absolute:
        {
            s += formatAddress(s, address);
            if (op.opCode != JMP && op.opCode != JSR)
            {
                uint8 result = bus->read(address);
                s += formatString(s, " = ");
                s += formatByte(s, result);
            }
        }
        break;
        case AbsoluteX:
        {
            s += formatAddress(s, p1, p2);
            s += formatString(s, ",X @ ");
            s += formatWord(s, address);

            uint8 result = bus->read(address);
            s += formatString(s, " = ");
            s += formatByte(s, result);
        }
        break;
        case AbsoluteY:
        {
            s += formatAddress(s, p1, p2);
            s += formatString(s, ",Y @ ");
            s += formatWord(s, address);

            uint8 result = bus->read(address);
            s += formatString(s, " = ");
            s += formatByte(s, result);
        }
        break;
        case Immediate:
            *s++ = '#';
            s += formatHex(s, p1);
            break;
        case Indirect:
            *s++ = '(';
            s += formatAddress(s, p1, p2);
            s += formatString(s, ") = ");
            s += formatWord(s, address);
            break;
        case Relative:
            s += formatAddress(s, address);
            break;
        case IndirectX:
        {
            *s++ = '(';
            s += formatHex(s, p1);
            s += formatString(s, ",X) @ ");
            s += formatByte(s, (uint8)(p1 + cpu->x));
            s += formatString(s, " = ");
            s += formatWord(s, address);

            uint8 result = bus->read(address);
            s += formatString(s, " = ");
            s += formatByte(s, result);
        }
        break;
        case IndirectY:
        {
            *s++ = '(';
            s += formatHex(s, p1);
            s += formatString(s, "),Y = ");
            s += formatWord(s, address);
            s += formatString(s, " @ ");
            s += formatWord(s, address);

            uint8 result = bus->read(address);
            s += formatString(s, " = ");
            s += formatByte(s, result);
        }
        break;
        case ZeroPage:
        {
            s += formatHex(s, p1);

            uint8 result = bus->read(address);
            s += formatString(s, " = ");
            s += formatByte(s, result);
        }
        break;
        case ZeroPageX:
        {
            s += formatHex(s, p1);
            s += formatString(s, ",X @ ");
            s += formatByte(s, (uint8)address);

            uint8 result = bus->read(address);
            s += formatString(s, " = ");
            s += formatByte(s, result);
        }
        break;
        case ZeroPageY:
        {
            s += formatHex(s, p1);
            s += formatString(s, ",Y @ ");
            s += formatByte(s, (uint8)address);

            uint8 result = bus->read(address);
            s += formatString(s, " = ");
            s += formatByte(s, result);
        }
        break;
        case Accumulator:
            *s++ = 'A';
            break;
    }

    *s = 0;
    return (int32)(s - dest);
}

int32 formatHexInstruction(char* dest, uint16 address, uint8 opcode, AddressingMode addressMode, uint8 p1, uint8 p2)
{
    char* start = dest;
    dest += formatWord(dest, address);
    *dest++ = ' ';
    *dest++ = ' ';
    dest += formatByte(dest, opcode);

    switch (addressMode)
    {
    case Absolute:
    case AbsoluteX:
    case AbsoluteY:
    case Indirect:
        *dest++ = ' ';
        dest += formatByte(dest, p1);
        *dest++ = ' ';
        dest += formatByte(dest, p2);
        break;
    case Immediate:
    case IndirectX:
    case IndirectY:
    case Relative:
    case ZeroPage:
    case ZeroPageX:
    case ZeroPageY:
        *dest++ = ' ';
        dest += formatByte(dest, p1);
        break;
    }

    return (int32)(dest - start);
}

int formatRegistersFCEU(char* dest, MOS6502* cpu)
{
    memcpy(dest, "A:00 X:00 Y:00 S:00 P:nvubdizc\n", 32);

    formatByte(dest + 2, cpu->accumulator);
    formatByte(dest + 7, cpu->x);
    formatByte(dest + 12, cpu->y);
    formatByte(dest + 17, cpu->stack);

    if (cpu->isFlagSet(STATUS_NEGATIVE)) dest[22] = 'N';
    if (cpu->isFlagSet(STATUS_OVERFLOW)) dest[23] = 'V';
    if (cpu->isFlagSet(0b00100000)) dest[24] = 'U';
    if (cpu->isFlagSet(0b00010000)) dest[25] = 'B';
    if (cpu->isFlagSet(STATUS_DECIMAL)) dest[26] = 'D';
    if (cpu->isFlagSet(STATUS_INT_DISABLE)) dest[27] = 'I';
    if (cpu->isFlagSet(STATUS_ZERO)) dest[28] = 'Z';
    if (cpu->isFlagSet(STATUS_CARRY)) dest[29] = 'C';

    return 32;
}

int formatRegistersNesTest(char* dest, MOS6502* cpu)
{
    memcpy(dest, "A:00 X:00 Y:00 P:00 SP:00\n", 27);

    formatByte(dest + 2, cpu->accumulator);
    formatByte(dest + 7, cpu->x);
    formatByte(dest + 12, cpu->y);
    formatByte(dest + 17, cpu->status);
    formatByte(dest + 23, cpu->stack);

    return 27;
}

// makes no assumptions buffer must be long enough
void padRight(char* start, int32 length, int32 desiredLength, char padChar = ' ')
{
    char* end = start + length;
    while (end - start < desiredLength)
    {
        *end++ = padChar;
    }

    *end = 0;
}

void logInstruction(const char* filename, uint16 address, MOS6502* cpu, IBus* cpuBus)
{
    if (!logFile)
    {
        logFile = fopen(filename, "w");
    }

    uint8 opcode = cpuBus->read(address);
    uint8 p1 = cpuBus->read(address + 1);
    uint8 p2 = cpuBus->read(address + 2);

    Operation op = operations[opcode];

    const int32 HEX_WIDTH = 15;
    const int32 INST_WIDTH = 33;
    const int32 REG_WIDTH = 27;

    char line[HEX_WIDTH + INST_WIDTH + REG_WIDTH] = {};
    char* columnStart = line;
    int32 hexLength = formatHexInstruction(line, address, opcode, op.addressMode, p1, p2);
    padRight(columnStart, hexLength, HEX_WIDTH);
    columnStart += HEX_WIDTH;

    int32 instLength = formatInstruction(columnStart, address, cpu, cpuBus);
    padRight(columnStart, instLength, INST_WIDTH);
    columnStart += INST_WIDTH;

    formatRegistersNesTest(columnStart, cpu);
    fputs(line, logFile);
}

void flushLog()
{
    fflush(logFile);
}