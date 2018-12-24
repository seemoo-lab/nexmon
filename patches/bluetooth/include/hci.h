
////////////////////////////////////////////////////////////////////////////
// Commands:
// <command>: holds the hex value representing the hci-command
// <command>_STR: holds the hex value but in form of a little-endian 
//                  hex string
// <command>_STR_<device_name>: holds prebuilt device-specific static
//                               little-endian hex strings
//
// Commands prefixed with HCI:
//  These Commands are HCI-Commands 
//
// Commands prefixed with PATCHRAM:
//  These Commands are used in the patchram mechanism
////////////////////////////////////////////////////////////////////////////

#ifndef HCI_HEADER
#define HCI_HEADER

// WRITE_RAM <addr: 32 bit> <data: up to 251 bytes>
#define HCI_WRITE_RAM 0xFC4C
#define HCI_WRITE_RAM_STR "\x4c\xfc"

// LAUNCH_RAM 
#define HCI_LAUNCH_RAM 0xFC4E
#define HCI_LAUNCH_RAM_STR "\x4e\xfc"
#define HCI_LAUNCH_RAM_STR_NEXUS_5 "\x4e\xfc\x04\xff\xff\xff\xff"

// Update Baudrate
#define HCI_UPDATE_BAUDRATE 0xFC18
#define HCI_UPDATE_BAUDRATE_STR "\x18\xfc"

// READ RAM <addr: 32 bit> <amount of bytes to read: 8 bit - up to 251 byte>
#define HCI_READ_RAM 0xFC4D
#define HCI_READ_RAM_STR "\x4d\xfc"

// Patchram:

// Issues a reboot and then continues to process the list
#define PATCHRAM_ISSUE_REBOOT 0x02

// Patch a 32 bit word in the ROM
#define PATCHRAM_PATCH_ROM 0x08

// Patch an arbitary length of bytes in the RAM
#define PATCHRAM_PATCH_RAM 0x0A

// Set default bluetooth device address
#define PATCHRAM_SET_DEFAULT_ADDRESS 0x40

// An ASCII string which is to be the new local device name
#define PATCHRAM_SET_LOCAL_DEVICE_NAME 0x041

// Indicates the end of the patchram list
#define PATCHRAM_END_OF_LIST 0xFE

#endif
