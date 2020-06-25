## PPU
The ppu stuff is scattered all over the place and it took me way to long to piece it
together. First thing is that it's purely an output device. It's gonna do it's thing
no matter what the cpu is going through. It basically runs a static program each cycle
which uses the cycle count to mediate which sort of phase its in.

The ppu has access to a couple different devices based on memory which is affected by
the mapper. Each cycle it uses the bits in the various registers to determine what
address to pull from memory.

http://wiki.nesdev.com/w/index.php/PPU_rendering

That is super helpful once you have any clue what the hell the thing is doing. Ignore
all the stuff about shift registers for now.

PPUADDR and PPUDATA actually point into the vram, and control where the PPU is looking.
Thats why they mention that writing to them in the middle of rendering is BAD!
https://nesdoug.com/2018/03/21/ppu-writes-during-rendering/

I initially thought this was some other area that was used to share data between the
two chips but it's actually just an access to vram directly. This would imply that
these don't incremented during idle periods, like vblank and hblank, which would
explain the timing diagram.

Now we can get back to the shift registers, which I didn't even know were a thing until this.
they basically shift their low value into their output each time they are clocked.
which during rendering would be each cycle. So they basically use these to preload
the data for several pixels in advance using these reads, and then combine together
the results using these shift registers with the bit from each one contributing to
the overall pixel thats rendered.

http://wiki.nesdev.com/w/index.php/PPU_registers#PPUCTRL
So we start with PPUCTRL set to zeros to select the first name table, increment the
address by one per 8 cycle read, use 0x000 offset for our sprite table lookup and
0x0000 offset for our pattern table lookup. Then we go by the timing diagram and
start doing reads, every time we hit 8 cycles we flush our temporary reads into the
shift registers and start again.

Meanwhile the shift registers are spitting out bits combining them together with the
pallette info and the sprite data to produce the pixel. it takes 8 cycles to do the reads
and 8 bits are stored in the pixel so it keeps up with itself.

Thats all just background/general stuff. sprites are a whole other can of worms

======

Or all of that is BS! Well not completely, but I hate how this wiki is laid out
https://wiki.nesdev.com/w/index.php/PPU_scrolling

About half way through they explain the internal registers related to the stuff I'm talking about.
I wish this was just in the page that explained registers.

======
This now makes sense..! I hope... Took awhile to collate all this into one process
Eventually I'll have to write this up properly from what I've managed to understand in a blog or something
http://wiki.nesdev.com/w/index.php/PPU_rendering#Cycles_1-256

vram address is pointing to a location in the nametable
first byte we read the value at this address, which is an index into the pattern table
http://wiki.nesdev.com/w/index.php/PPU_nametables

second byte, we read the attribute byte for that nametable location
http://wiki.nesdev.com/w/index.php/PPU_attribute_tables

third and forth byte, we use the nametable value to get the two planes of the pattern data
http://wiki.nesdev.com/w/index.php/PPU_pattern_tables

then we advance by the vramincrement and store our results into the shift registers
Repeat... forever!! with some exceptions for vblanking, and some other stuff

The attribute byte determines which of four pallettes to use and
the pattern bits give us which of four colours in that palette to use
These actually form an address into paletteRam, which is filled with colors
From the available nes palette. http://wiki.nesdev.com/w/index.php/PPU_palettes
These palettes are in ram and can be adjusted at run time, which would explain how
tetris level colorization would have worked, also means you can only have
16 colors at once without mid frame shenanigans

Sprites are evaluated in a slightly more complex but similar way out of object memory
then rendered instead of the background value depending on priority

Scrolling within an 8 bit tile is done by reading bits further in on the shift registers
and lower down in the nametable than a given scanline is supposed to
http://wiki.nesdev.com/w/index.php/PPU_scrolling (lots of other good stuff on this page)