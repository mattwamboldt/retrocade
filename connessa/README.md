= ConNessa

A Console Based implementation of an nes emulator

This implementation only deals with the raw execution of the component, ensuring memory is written to correctly.
It's used for debugging the enumaltor a low level only.
The full graphics and sound will be done in retrocade, though I may mess around with some basic ppu tests with conio

== iNES file format
http://wiki.nesdev.com/w/index.php/INES

== System
- CPU modified 6502, 2KB dedicated, 64k address space using a memory mapper
- PPU 8 registers repeated many times
- APU included on APU, it has IO registers, 24 bytes
- extra bytes for APU and IO test functionality

== Notes
The main point of confusion for me was between address space and physical components. Realizing what parts of the address are fixed vs configurable helped a lot.
I initially thought everything went through the mapper and it was a free for all.

=== CPU Memory
6502 has 16 byte bus, hence the "mappers" depending on hardware on the cartridge certain address' may go off device onto extra cart memory
Each device has an address space available to it and the cartridge determines how that address space is mapped to physical chips
this allows the cart to expand the capabilities based on the manufacturer

First 16k + 32 bytes is dedicated to memory registers etc on the nes itself

cpu -  0000 - 07ff 2kb RAM which is mirrored 4 times
ppu -  2000 - 2007 mapped to registers on the ppu, mirrored every 8 bytes through to 3FFF
apu -  4000 - 401f mapped to registers in the apu
cart - 4020 - ffff mapped to the cart, which may use custom logic to pull various shenanigans like bank switching

== PPU Memory
ppu separate 16kb address space
- VRAM 2k
- pallettes 3f00 - 3fff
- oam 256 bytes on ppu addressed by cpu through mapped registers

- cpu dma to oam that stalls the cpu execution during copy

== Current Task Plans
1 - Try to write a manually stepped 6502 emulator connected to a flat 64k ram module.
since it is kind of standalone and has the most documentation. was used in several machines so lots to pull from
may be better to do this in a console app with simpler debugging measures than a windows one and then pull it in after
http://wiki.nesdev.com/w/index.php/CPU_ALL
https://www.masswerk.at/6502/6502_instruction_set.html

2 - Try loading and running some of the cpu test roms
http://wiki.nesdev.com/w/index.php/Emulator_tests

3 - Try getting the ppu to work and run the relevant tests
http://wiki.nesdev.com/w/index.php/PPU_programmer_reference

4 - Hook up the master clock
5 - Connect the ppu to an actual display using gdi to blit a backbuffer filled by the GPU
6 - Add the APU functions to the CPU emulator (play back using directsound, since I've used it before)
7 - Add a basic mapper that follows the standard layout
8 - Add keyboard input
9 - Try more test roms and clean up anything remaining
10 - Test out some commercial games that I have
11 - Start to add an interface, record modes, save states, (etc/whatever strikes my fancy)
