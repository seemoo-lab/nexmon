
////////////////////////////////////////////////////////////////////////////
// Commands:
// <command>: holds the hex value representing the hci-command
// <command>_STR: holds the hex value but in form of a little-endian 
//                  hex string
//
// Commands prefixed with HCI:
//  These Commands are HCI-Commands 
//
// Commands prefixed with PATCHRAM:
//  These Commands are used in the patchram mechanism
//
// Device specific constants:
//   They can be found in their header files
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

// Patchram constants

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

struct tlv {
    uint8_t  tlv_type;
    uint16_t length;
    uint8_t * data;
};

//struct tlv_patchram {
//    uint8_t tlv_type;
//    uint16_t length;
//    union Data {
//        uint8_t[15] byte_data;
//        patchram_tlv_data* struct_data;
//    } data;
//}

struct patchram_tlv_data {
    uint8_t  slot_number;
    uint32_t target_address;
    uint32_t new_data;
    uint16_t null_bytes;
    uint32_t unknown_bytes;
};


/*
 * Takes an existing patchram_tlv_data structure and converts its content
 * into a uint8_t array of size 15.
 */ 
uint8_t* patchram_tlv_data_to_byte_array(patchram_tlv_data* patchram);

/*
 * Takes an uint8_t array of size 15 (must be exactly this size!) and converts 
 * its content to a patchram_tlv_data structure.
 */ 
patchram_tlv_data* byte_array_to_patchram_tlv_data(uint8_t[static 15]);

tlv* get_prepared_patchram_tlv();

#endif
