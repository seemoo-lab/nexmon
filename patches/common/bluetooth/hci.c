#include <hci.h>
// yet untested
uint8_t* patchram_tlv_data_to_byte_array(patchram_tlv_data* patchram){
	uint8_t* byte_array = malloc(15);
	byte_array[0] = patchram->slot_number;
	byte_array[1] = patchram->target_address;
	byte_array[5] = patchram->new_data;
	byte_array[9] = patchram_null_bytes;
	byte_array[11] = patchram->unknown_bytes;
	return byte_array;
}

patchram_tlv_data* byte_array_to_patchram_tlv_data(uint8_t[static15] byte_array){
	patchram_tlv_data* tlv_data = malloc(struct patchram_tlv_data);
	tlv_data->slot_number = byte_array[0];
	tlv_data->target_address = byte_array[1];
	tlv_data->target_new_data = byte_array[5];
	tlv_data->target_null_bytes = byte_array[9];
	tlv_data->unknown_bytes = byte_array[11];
	return tlv_data;
}

tlv* get_prepared_patchram_tlv(){
	struct tlv* prep_tlv = malloc(sizeof(struct tlv));
	prep_tlv->tlv_type = PATCHRAM_PATCH_ROM;
	prep_tlv->length   = (uint16_t) 15;
	return prep_tlv;
}
