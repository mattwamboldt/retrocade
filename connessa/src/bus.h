#pragma once
#include "common.h"

class IBus
{
    public:
        virtual uint8 read(uint16 address) = 0;
        virtual uint16 readWord(uint16 address) = 0;
        virtual void write(uint16 address, uint8 value) = 0;
};