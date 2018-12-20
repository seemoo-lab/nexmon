## Bluetooth Firmware Patching

### What?



### Why?



### How?

#### HCD patching vs live patching


#### Compilation



### Paths

**Common between all bluetooth patches**
- buildtools: `nexmon/patches/blueooth/buildtools` 
- common - source/c-files: `nexmon/patches/common`
- include - source/h-files: `nexmo/patches/include`

**BCM 4330c0:**
- firmware: `nexmon/firmware/bcm433c0_BT/` 
- patch sources: `nexmon/patches/blueooth/bcm4335c0`
  + test patching: `test_patching`


### Basics

#### Compile-Assemble-Link-Chain
```
               -----------------
               | C-Code        |
               -----------------
                    | preprocessor
                    v
               -----------------
               | joined C-Code |
               -----------------
                    | compiler
                    v
               -----------------
               | intermediate  |  (*.s - file)
               | Code          |  (gcc -S)
               -----------------
                    | assembler
                    v
               -----------------  (*.o - file)
               | Object - File |  (gcc -C)
               -----------------
-------------       |
| other *.o |------>| linker
| or libs   |       |
| etc.      |       v
-------------  -----------------
               | Out- File     |
               -----------------
```
