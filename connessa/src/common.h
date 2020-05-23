#pragma once
#include <stdint.h>

// Some common typedefs that I tend to use for uniformity
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int32 bool32;

typedef float real32;
typedef double real64;

// Converts character codes into uint32 representation, useful for file format processing
#define fourCC(A, B, C, D) \
    ((uint32)((uint8)(A) << 0) | (uint32)((uint8)(B) << 8) | \
     (uint32)((uint8)(C) << 16) | (uint32)((uint8)(D) << 24))

// Simplifies memory and size calculations
#define kilobytes(value) ((value) * 1024)
#define megabytes(value) (kilobytes(value) * 1024)
#define gigabytes(value) (megabytes(value) * 1024)
#define terabytes(value) (gigabytes(value) * 1024)

// Lots of code is just passing around this kind of info so made a struct for it
struct Buffer
{
    uint32 size;
    void* memory;
};

typedef void(*WriteCallback)(uint16 adddress, uint8 value);