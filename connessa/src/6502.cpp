#include "6502.h"

void MOS6502::reset()
{
    isHalted = false;
    status = 0;
    stack = 0xff;
    instAddr = pc = readWord(0xFFFC);
}

bool MOS6502::tick()
{
    if (isHalted)
    {
        return false;
    }

    if (waitCycles > 0)
    {
        --waitCycles;
        return false;
    }

    inst = bus->read(pc++);
    Operation op = operations[inst];
    if (op.opCode == KILL)
    {
        --pc;
        isHalted = true;
    }
    else
    {
        loadOperands();
        executeInstruction();
        waitCycles = op.cycleCount - 1; // exclude the current cycle
    }

    instAddr = pc;
    return true;
}

void MOS6502::addWithCarry(uint8 data)
{
    uint8 carry = status & STATUS_CARRY;
    uint8 sum = data + carry + accumulator;

    // Slightly complicated, but overflow only kicks in if signs are different
    uint8 overflow = (accumulator ^ sum) & (data ^ sum) & 0b10000000;
    status &= ~(STATUS_OVERFLOW|STATUS_CARRY);
    status |= overflow >> 1;

    if ((uint16)data + carry + accumulator > 0xFF)
    {
        status |= STATUS_CARRY;
    }

    accumulator = sum;
    updateNZ(accumulator);
}

void MOS6502::subtractWithCarry(uint8 data)
{
    addWithCarry(~data);
}

void MOS6502::andA(uint8 data)
{
    accumulator &= data;
    updateNZ(accumulator);
}

uint8 MOS6502::shiftLeft(uint8 data)
{
    status &= ~STATUS_CARRY;
    status |= (data & 0b10000000) >> 7;
    data <<= 1;
    updateNZ(data);
    return data;
}

uint8 MOS6502::shiftRight(uint8 data)
{
    status &= ~STATUS_CARRY;
    status |= data & 0b00000001;
    data >>= 1;
    updateNZ(data);
    return data;
}

void MOS6502::branchCarryClear(uint8 data)
{
    jumpOnFlagClear(STATUS_CARRY, data);
}

void MOS6502::branchCarrySet(uint8 data)
{
    jumpOnFlagSet(STATUS_CARRY, data);
}

void MOS6502::branchEqual(uint8 data)
{
    jumpOnFlagSet(STATUS_ZERO, data);
}

void MOS6502::branchNegative(uint8 data)
{
    jumpOnFlagSet(STATUS_NEGATIVE, data);
}

void MOS6502::branchNotZero(uint8 data)
{
    jumpOnFlagClear(STATUS_ZERO, data);
}

void MOS6502::branchPositive(uint8 data)
{
    jumpOnFlagClear(STATUS_NEGATIVE, data);
}

void MOS6502::branchOverflowSet(uint8 data)
{
    jumpOnFlagSet(STATUS_OVERFLOW, data);
}

void MOS6502::branchOverflowClear(uint8 data)
{
    jumpOnFlagClear(STATUS_NEGATIVE, data);
}

void MOS6502::bitTest(uint8 data)
{
    clearFlags(STATUS_NEGATIVE|STATUS_OVERFLOW|STATUS_ZERO);
    status |= (data & (STATUS_NEGATIVE|STATUS_OVERFLOW));

    if ((data & accumulator) == 0)
    {
        setFlags(STATUS_ZERO);
    }
}

void MOS6502::forceBreak()
{
    pushWord(instAddr + 2);
    push(status | 0b00110000);
    pc = readWord(0xFFFE);
}

void MOS6502::clearCarry()
{
    clearFlags(STATUS_CARRY);
}

void MOS6502::clearDecimal()
{
    clearFlags(STATUS_DECIMAL);
}

void MOS6502::clearInteruptDisable()
{
    clearFlags(STATUS_INT_DISABLE);
}

void MOS6502::clearOverflow()
{
    clearFlags(STATUS_OVERFLOW);
}

void MOS6502::setCarry()
{
    setFlags(STATUS_CARRY);
}

void MOS6502::setDecimal()
{
    setFlags(STATUS_DECIMAL);
}

void MOS6502::setInteruptDisable()
{
    setFlags(STATUS_INT_DISABLE);
}

void MOS6502::compareA(uint8 data)
{
    compare(accumulator, data);
}

void MOS6502::compareX(uint8 data)
{
    compare(x, data);
}

void MOS6502::compareY(uint8 data)
{
    compare(y, data);
}

uint8 MOS6502::decrement(uint8 data)
{
    --data;
    updateNZ(data);
    return data;
}

void MOS6502::decrementX()
{
    --x;
    updateNZ(x);
}

void MOS6502::decrementY()
{
    --y;
    updateNZ(y);
}

void MOS6502::exclusiveOrA(uint8 data)
{
    accumulator ^= data;
    updateNZ(accumulator);
}

void MOS6502::orA(uint8 data)
{
    accumulator |= data;
    updateNZ(accumulator);
}

uint8 MOS6502::increment(uint8 data)
{
    ++data;
    updateNZ(data);
    return data;
}

void MOS6502::incrementX()
{
    ++x;
    updateNZ(x);
}

void MOS6502::incrementY()
{
    ++y;
    updateNZ(y);
}

void MOS6502::jump(uint16 address)
{
    pc = address;
}

void MOS6502::jumpSubroutine(uint16 address)
{
    pc = address;
    pushWord(instAddr + 2);
}

void MOS6502::loadA(uint8 data)
{
    accumulator = data;
    updateNZ(accumulator);
}

void MOS6502::loadX(uint8 data)
{
    x = data;
    updateNZ(x);
}

void MOS6502::loadY(uint8 data)
{
    y = data;
    updateNZ(y);
}

void MOS6502::pushA()
{
    push(accumulator);
}

void MOS6502::pushStatus()
{
    push(status | 0b00110000);
}

void MOS6502::pullA()
{
    accumulator = pull();
    updateNZ(accumulator);
}

void MOS6502::pullStatus()
{
    status = pull();
}

uint8 MOS6502::rotateLeft(uint8 data)
{
    uint8 carry = status & STATUS_CARRY;
    clearCarry();
    status |= (data & 0b10000000) >> 7;
    data = (data << 1) | carry;
    updateNZ(data);
    return data;
}

uint8 MOS6502::rotateRight(uint8 data)
{
    uint8 carry = status & STATUS_CARRY;
    clearCarry();
    status |= data & 0b00000001;
    data = (data >> 1) | (carry << 7);
    updateNZ(data);
    return data;
}

void MOS6502::returnIntertupt()
{
    status = pull();
    pc = pullWord();
}

void MOS6502::returnSubroutine()
{
    pc = pullWord() + 1;
}

void MOS6502::transferAtoX()
{
    x = accumulator;
    updateNZ(x);
}

void MOS6502::transferAtoY()
{
    y = accumulator;
    updateNZ(y);
}

void MOS6502::transferStoX()
{
    x = stack;
    updateNZ(x);
}

void MOS6502::transferXtoA()
{
    accumulator = x;
    updateNZ(accumulator);
}

void MOS6502::transferXtoS()
{
    stack = x;
    updateNZ(stack);
}

void MOS6502::transferYtoA()
{
    accumulator = y;
    updateNZ(accumulator);
}

uint16 MOS6502::calcAddress(AddressingMode addressMode, uint16 address, uint8 a, uint8 b)
{
    switch (addressMode)
    {
        case Absolute:
            return (uint16)b << 8 | a;

        case AbsoluteX:
            return x + ((uint16)b << 8 | a);

        case AbsoluteY:
            return y + ((uint16)b << 8 | a);

        case Indirect:
            return bus->read((uint16)b << 8 | a);

        case IndirectX:
            return readWord(a + x);

        case IndirectY:
        {
            uint16 lo = bus->read(a) + (uint16)y;
            uint16 hi = bus->read(a + 1);
            return (hi << 8) + lo;
        }
        case Relative:
            return (address + 2) + (int8)a;

        case ZeroPage:
            return a;

        case ZeroPageX:
            return a + x;

        case ZeroPageY:
            return a + y;
    }

    return 0;
}

uint16 MOS6502::calcAddress(AddressingMode addressMode)
{
    return calcAddress(addressMode, instAddr, p1, p2);
}

void MOS6502::loadOperands()
{
    uint8 opcode = bus->read(instAddr);
    Operation op = operations[opcode];
    switch (op.addressMode)
    {
        case Absolute:
        case AbsoluteX:
        case AbsoluteY:
        case Indirect:
            p1 = bus->read(pc++);
            p2 = bus->read(pc++);
            break;

        case Immediate:
        case IndirectX:
        case IndirectY:
        case Relative:
        case ZeroPage:
        case ZeroPageX:
        case ZeroPageY:
            p1 = bus->read(pc++);
            break;
    }
}

bool MOS6502::requireRead(uint8 opCode)
{
    switch (opCode)
    {
        case ADC:
        case AND:
        case ASL:
        case BIT:
        case CMP:
        case CPX:
        case CPY:
        case DEC:
        case EOR:
        case INC:
        case LDA:
        case LDX:
        case LDY:
        case LSR:
        case ORA:
        case ROL:
        case ROR:
        case SBC:
            return true;
    }

    return false;
}

void MOS6502::executeInstruction()
{
    Operation op = operations[inst];
    if (requireRead(op.opCode))
    {
        tempData = readData(op.addressMode);
    }

    switch (op.opCode)
    {
        case ADC: addWithCarry(tempData); break;
        case AND: andA(tempData); break;
        case ASL:
        {
            tempData = shiftLeft(tempData);
            writeData(op.addressMode, tempData);
        }
        break;
        case BCC: branchCarryClear(p1); break;
        case BCS: branchCarrySet(p1); break;
        case BEQ: branchEqual(p1); break;
        case BIT: bitTest(tempData); break;
        case BMI: branchNegative(p1); break;
        case BNE: branchNotZero(p1); break;
        case BPL: branchPositive(p1); break;
        case BRK: forceBreak(); break;
        case BVC: branchOverflowClear(p1); break;
        case BVS: branchOverflowSet(p1); break;
        case CLC: clearCarry(); break;
        case CLD: clearDecimal(); break;
        case CLI: clearInteruptDisable(); break;
        case CLV: clearOverflow(); break;
        case CMP: compareA(tempData); break;
        case CPX: compareX(tempData); break;
        case CPY: compareY(tempData); break;
        case DEC:
        {
            tempData = decrement(tempData);
            writeData(op.addressMode, tempData);
        }
        break;
        case DEX: decrementX(); break;
        case DEY: decrementY(); break;
        case EOR: exclusiveOrA(tempData); break;
        case INC:
        {
            tempData = increment(tempData);
            writeData(op.addressMode, tempData);
        }
        break;
        case INX: incrementX(); break;
        case INY: incrementY(); break;
        case JMP: jump(calcAddress(op.addressMode)); break;
        case JSR: jumpSubroutine(calcAddress(op.addressMode)); break;
        case LDA: loadA(tempData); break;
        case LDX: loadX(tempData); break;
        case LDY: loadY(tempData); break;
        case LSR:
        {
            tempData = shiftRight(tempData);
            writeData(op.addressMode, tempData);
        }
        break;
        case ORA: orA(tempData); break;
        case PHA: pushA(); break;
        case PHP: pushStatus(); break;
        case PLA: pullA(); break;
        case PLP: pullStatus(); break;
        case ROL:
        {
            tempData = rotateLeft(tempData);
            writeData(op.addressMode, tempData);
        }
        break;
        case ROR:
        {
            tempData = rotateRight(tempData);
            writeData(op.addressMode, tempData);
        }
        break;
        case RTI: returnIntertupt(); break;
        case RTS: returnSubroutine(); break;
        case SBC: subtractWithCarry(tempData); break;
        case SEC: setCarry(); break;
        case SED: setDecimal(); break;
        case SEI: setInteruptDisable(); break;
        case STA: writeData(op.addressMode, accumulator); break;
        case STX: writeData(op.addressMode, x); break;
        case STY: writeData(op.addressMode, y); break;
        case TAX: transferAtoX(); break;
        case TAY: transferAtoY(); break;
        case TSX: transferStoX(); break;
        case TXA: transferXtoA(); break;
        case TXS: transferXtoS(); break;
        case TYA: transferYtoA(); break;
    }
}

// =========
// Utils
// =========
void MOS6502::setFlags(uint8 flags)
{
    status |= flags;
}

void MOS6502::clearFlags(uint8 flags)
{
    status &= ~flags;
}

void MOS6502::updateNZ(uint8 val)
{
    clearFlags(STATUS_NEGATIVE | STATUS_ZERO);
    status |= val & STATUS_NEGATIVE;
    if (val == 0)
    {
        setFlags(STATUS_ZERO);
    }
}

void MOS6502::compare(uint8 a, uint8 b)
{
    uint8 result = a - b;
    status &= 0b01111100;
    if ((uint8)result >= 0)
    {
        status |= 0b00000011;
    }

    updateNZ(result);
}

void MOS6502::jumpOnFlagSet(uint8 flag, uint8 offset)
{
    if (status & flag)
    {
        jumpRelative(offset);
    }
}

void MOS6502::jumpOnFlagClear(uint8 flag, uint8 offset)
{
    if (!(status & flag))
    {
        jumpRelative(offset);
    }
}

void MOS6502::jumpRelative(uint8 offset)
{
    pc = instAddr + 2 + (int8)offset;
}

uint8 MOS6502::pull()
{
    ++stack;
    return bus->read(0x0100 | stack);
}

uint16 MOS6502::pullWord()
{
    uint16 lo = pull();
    uint16 hi = pull();
    return (hi << 8) + lo;
}

void MOS6502::push(uint8 v)
{
    bus->write(0x0100 | stack, v);
    stack--;
}

void MOS6502::pushWord(uint16 v)
{
    uint8 lo = v & 0x00FF;
    uint8 hi = (v & 0xFF00) >> 8;
    push(hi);
    push(lo);
}

void MOS6502::nonMaskableInterrupt()
{
    // TODO:
}

void MOS6502::requestInterrupt()
{
    // TODO:
}

uint16 MOS6502::readWord(uint16 base)
{
    uint8 lo = bus->read(base);
    uint8 hi = bus->read(base + 1);
    return ((uint16)hi << 8) + lo;
}

uint8 MOS6502::readData(AddressingMode addressMode)
{
    if (addressMode == Immediate)
    {
        return p1;
    }

    if (addressMode == Accumulator)
    {
        return accumulator;
    }

    uint16 address = calcAddress(addressMode);
    return bus->read(address);
}

void MOS6502::writeData(AddressingMode addressMode, uint8 val)
{
    if (addressMode == Accumulator)
    {
        accumulator = val;
        return;
    }

    uint16 address = calcAddress(addressMode);
    bus->write(address, val);
}