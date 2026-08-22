# Jay-D Library
> Arduino library for programming Jay-D, your own DJ mixtable.


With Jay-D, you’ll learn how microcomputers and other electronic components are used for sound production and get a bit closer to becoming a DJ superstar. You can learn more [here](https://circuitmess.com/jay-d/).

Jay-D is also a part of [CircuitMess STEM Box](https://igg.me/at/stem-box/x#/) - a series of fun electronic kits to help children and adults understand the basics of technologies everybody's talking about.


![](https://circuitmess.com/wp-content/uploads/2021/05/jayd-nobg-resized-min.png)

# Installation

The library is automatically installed when you install the CircuitMess ESP32 Arduino platform, which contains the Jay-D board. More info on [CircuitMess/Arduino-Packages](https://github.com/CircuitMess/Arduino-Packages).

# Development setup

The library uses several dependency libraries:
- [CircuitOS](https://github.com/CircuitMess/CircuitOS) by CircuitMess
- [FDK-AAC](https://github.com/mstorsjo/fdk-aac) by Martin Storsjö
- [libhelix-mp3 and libhelix-aac](https://github.com/ultraembedded/libhelix-mp3) by RealNetworks

These libraries are automatically installed when you install the CircuitMess ESP32 Arduino platform.

## Using Arduino IDE

Simply open JayD-Library.ino using Arduino IDE, set the board to Jay-D, and compile.

## Using CMake

To run a test compilation you need to have [CMake](https://cmake.org/) and [arduino-cli](https://github.com/arduino/arduino-cli)  installed. You also need to have both of them registered in the PATH.

In the CMakeLists.txt file change the port to your desired COM port (default is /dev/ttyUSB0):
```
set(PORT /dev/ttyUSB0)
```
Then in the root directory of the repository type:
```
mkdir cmake
cd cmake
cmake ..
cmake --build . --target CMBuild
```
This will compile the binaries, and place the .bin and .elf files in the build/ directory located in the root of the repository.

## AAC timing and seeking

`SourceAAC` measures time in source sample frames: one frame is one sample
instant across all encoded channels, at the ADTS sample rate. Channel count
does not multiply duration. The seconds-based API remains available and rounds
down to whole seconds.

ADTS indexing runs while a source is opened, before it is published to the
audio thread. Each seek entry is 8 bytes. The bounded index uses at most 128 KiB
per deck in PSRAM (16,384 frames), or 16 KiB without PSRAM (2,048 frames);
the temporary scan cache is 4 KiB. The larger strict-frame decode buffers add
35 KiB per source over the previous buffers. `getFrameIndexQuality()` reports
whether the whole track is seekable; duration remains frame-counted even when
the seek index reaches its cap. A `cm:esp32:jayd` build measured 1,026,334
bytes of flash and 44,464 bytes of static RAM: +312 bytes of flash and no
static-RAM increase versus `4ad5108`.

Seek targets use a 64-bit source-frame API, but the compact index stores 32-bit
frame positions, limiting frame-accurate seeking to the first 2^32 source
frames (about 24.9 hours at 48 kHz). Seeking starts at the preceding ADTS frame
and discards decoded PCM up to the target. Accuracy is therefore bounded by
the decoder's sample-rate conversion and AAC priming; one 256-sample output
block of accuracy has not been established on hardware.

To compile the binary, and upload it according to the port set in CMakeLists.txt, run

```cmake --build . --target CMBuild```

in the cmake directory.
# Used libraries and copyright notices
[See NOTICE](https://github.com/CircuitMess/JayD-Library/blob/master/NOTICE.md)

Code that runs on the Nuvoton N76E616 chip is located in a [separate repo](https://github.com/CircuitMess/JayD-Nuvoton-Firmware).

# Meta


**CircuitMess**  - https://circuitmess.com/

**Facebook** - https://www.facebook.com/thecircuitmess/

**Instagram** - https://www.instagram.com/thecircuitmess/

**Twitter** - https://twitter.com/circuitmess

**YouTube** - https://www.youtube.com/channel/UCVUvt1CeoZpCSnwg3oBMsOQ

----
Copyright © 2021 CircuitMess

Licensed under [MIT License](https://opensource.org/licenses/MIT).