#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void free_binary_data(unsigned char *data);
size_t binary_data_len(unsigned char *data);
void * alloc_binary_data_memory(size_t size);

static unsigned char encoding_table[] = {	'0', '1', '2', '3',
											'4', '5', '6', '7',
											'8', '9', 'A', 'B',
											'C', 'D', 'E', 'F'
										};


static unsigned char decoding_table[256] = {
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

/*
 * ### unsigned char *hex_encode(const unsigned char *data, size_t input_length, size_t *output_length)
 *
 * DESCRIPTION:
 *              Encodes the given input binary data to HEX string format.
 *              Memory needed for the returned string is allocated within and the calling functions is
 *              expected to free the memory.
 * INPUT:
 *              const unsigned char *data : binary data
 *              size_t input_length       : size of the binary data
 *              size_t *output_length     : pointer to variable of type size_t where the length of the output
 *                                          buffer will be written
 * OUTPUT:
 *              output_length
 *              unsigned char *           : output buffer
 */
unsigned char *hex_encode(const unsigned char *data, size_t input_length, size_t *output_length)
{
	int i = 0, j = 0;

	*output_length = 2 * input_length;

	unsigned char *encoded_data = malloc(*output_length + 1); if (encoded_data == NULL) return NULL;
	memset(encoded_data, 0, (*output_length + 1));

	for (i = 0, j = 0; i < input_length; i++) {
		encoded_data[j++] = encoding_table[((data[i] & 0xF0)>>4)];
		encoded_data[j++] = encoding_table[(data[i] & 0x0F)];
	}

	return encoded_data;
}

/*
 * ### unsigned char *hex_decode(const unsigned char *data, size_t input_length, size_t *output_length)
 *
 * DESCRIPTION:
 *              Decodes the input hex string
 *              Memory needed for the returned binary data is allocated within and the calling functions is
 *              expected to free the memory.
 * INPUT:
 *              const unsigned char *data : string data
 *              size_t input_length       : size of the string data
 *              size_t *output_length     : pointer to variable of type size_t where the length of the output
 *                                          buffer will be written
 * OUTPUT:
 *              output_length
 *              unsigned char *           : output buffer
 */
unsigned char *hex_decode(const unsigned char *data, size_t input_length, size_t *output_length)
{
	int i = 0, j = 0;
	if (input_length % 2) { return NULL; } // There must be even number of bytes in a hex string
	*output_length = input_length / 2;

	unsigned char * decoded_data = alloc_binary_data_memory(*output_length);
	if (decoded_data == NULL) { return NULL; }

	unsigned char one = 0;
	unsigned char two = 0;
	for (i = 0, j = 0; i < *output_length; i++) {
		one = data[j++];
		two = data[j++];
		if (decoding_table[one] == 0xFF || decoding_table[two] == 0xFF) {
			free_binary_data(decoded_data);
			return NULL;
		}
		decoded_data[i] = (decoding_table[one] << 4) | (decoding_table[two]);
	}


	return (decoded_data);
}

