# 8086 dissassembler

For [Computer, Enhance!](https://www.computerenhance.com/)

Examples gotten from: https://github.com/cmuratori/computer_enhance/tree/main/perfaware/part1

## Building & Running

### CLI
```shell
make cli
./build/cli
```

### Web
```shell
# This assumes that you already have `emcc` in your path somewhere
make web
make serve-web
```

## Manual

8086 reference manual: https://edge.edx.org/c4x/BITSPilani/EEE231/asset/8086_family_Users_Manual_1_.pdf
Important pages in manual:
* Registers - 24
* Instruction structures - 163
* Memory addressing - 83
* Clocks per instruction - 66

Reference emulators:
* https://yassinebridi.github.io/asm-docs/
* https://yjdoc2.github.io/8086-emulator-web/
* https://carlosrafaelgn.com.br/Asm86/
* https://idrist11.github.io/8086-Online-IDE/app.html