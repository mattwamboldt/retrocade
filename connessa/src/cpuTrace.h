#pragma once
#include "6502.h"

int formatInstruction(char* dest, uint16 address, MOS6502* cpu, IBus* bus);
void logInstruction(const char* filename, uint16 address, MOS6502* cpu, IBus* cpuBus);
void flushLog();