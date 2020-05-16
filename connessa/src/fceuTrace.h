#pragma once
#include "6502.h"

// Logs instructions in the style of fceu for comparison/debugging
int formatInstruction(char* dest, uint16 address, MOS6502* cpu, IBus* bus);

void logInstruction(const char* filename, uint16 address, MOS6502* cpu, IBus* cpuBus);
void flushLog();