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
    uint16 pc;
    uint8 stack;
    uint8 status;
    uint8 accumulator;
    uint8 x;
    uint8 y;
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

int main(int argc, char *argv[])
{
    CPU cpu = {};
    cpu.status = 0x34;
    cpu.stack = 0xfd;

    uint8 ram[2 * 1024] = {};
    uint8 vram[2 * 1024] = {};

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

    uint8* prgRom = (uint8*)readHead;
    readHead += 16 * 1024 * header->prgRomSize;

    uint8* chrRom = (uint8*)readHead;
    readHead += 8 * 1024 * header->chrRomSize;

    cpu.pc = prgRom[0x7FFD];
    cpu.pc = (cpu.pc << 8) | prgRom[0x7FFC];

    while (running)
    {
        char input = _getch();
        if (input == 'q')
        {
            break;
        }

        if (input == ' ')
        {
            uint8 next = prgRom[cpu.pc++ - 0x8000];
            switch (next)
            {
                case 0x00:
                {
                    printf("BRK\n");
                }
                break;

                case 0x01:
                {
                    uint8 p1 = prgRom[cpu.pc++ - 0x8000];
                    printf("ORA (%x,X)\n", p1);
                }
                break;

                case 0x05:
                {
                    uint8 p1 = prgRom[cpu.pc++ - 0x8000];
                    printf("ORA %x\n", p1);
                }
                break;

                case 0x06:
                {
                    uint8 p1 = prgRom[cpu.pc++ - 0x8000];
                    printf("ASL %x\n", p1);
                }
                break;

                case 0x08:
                {
                    printf("PHP\n");
                }
                break;

                case 0x09:
                {
                    uint8 p1 = prgRom[cpu.pc++ - 0x8000];
                    cpu.accumulator |= p1;
                    printf("ORA #%x\n", p1);
                }
                break;

                case 0x0A:
                {
                    uint8 p1 = prgRom[cpu.pc++ - 0x8000];
                    printf("ASL %x\n", p1);
                }
                break;

                case 0x0D:
                {
                    uint8 p1 = prgRom[cpu.pc++ - 0x8000];
                    printf("ORA #%x\n", p1);
                }
                break;

                case 0x0E:
                {
                    uint8 lo = prgRom[cpu.pc++ - 0x8000];
                    uint8 hi = prgRom[cpu.pc++ - 0x8000];
                    printf("ASL %x%x\n", hi, lo);
                }
                break;

                default:
                    printf("Illegal Opcode: %x\n", next);
            }
        }
    }

    return 0;
}