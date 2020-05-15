#pragma once

// Some common typedefs that I tend to use for uniformity
typedef char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef void(*WriteCallback)(uint16 adddress, uint8 value);