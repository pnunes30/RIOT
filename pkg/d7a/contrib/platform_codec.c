/*
 * Copyright (C) 2018 CORTUS SA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @ingroup     pkg_d7a
 * @file
 * @brief       Implementation of codecs used by D7A
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 * @}
 */

#include <stdint.h>
#include <string.h>

#include "framework/inc/crc.h"
#include "framework/inc/pn9.h"
#include "framework/inc/compress.h"
#include "framework/inc/fec.h"
#include "framework/inc/crc.h"

#define INITIAL_FECSTATE 0x00
#define TRELLIS_TERMINATOR 0x0B
#define FEC_BUFFER_SIZE 128

#define INTERLEAVING

static const uint8_t fec_lut[16] = {0, 3, 1, 2, 3, 0, 2, 1, 3, 0, 2, 1, 0, 3, 1, 2};
static const uint8_t trellis0_lut[8] = {0, 1, 3, 2, 3, 2, 0, 1};
static const uint8_t trellis1_lut[8] = {3, 2, 0, 1, 0, 1, 3, 2};

static uint8_t data_buffer[FEC_BUFFER_SIZE];
static uint8_t* output_buffer;

static uint16_t packetlength;
static uint16_t output_packet_length;

static uint8_t processedbytes;
static uint16_t fecprocessedbytes;

static VITERBISTATE vstate;

static bool fec_decode(uint8_t* input);

#if defined(FRAMEWORK_LOG_ENABLED) && defined(FRAMEWORK_PHY_LOG_ENABLED) // TODO more granular (LOG_PHY_ENABLED)
#define DPRINT(...) log_print_stack_string(LOG_STACK_PHY, __VA_ARGS__)
#define DPRINT_DATA(...) log_print_data(__VA_ARGS__)
#else
#define DPRINT(...)
#define DPRINT_DATA(...)
#endif

static uint16_t crc;

static void update_crc(uint8_t x)
{
     uint16_t crc_new = (uint8_t)(crc >> 8) | (crc << 8);
     crc_new ^= x;
     crc_new ^= (uint8_t)(crc_new & 0xff) >> 4;
     crc_new ^= crc_new << 12;
     crc_new ^= (crc_new & 0xff) << 5;
     crc = crc_new;
}

uint16_t crc_calculate(uint8_t* data, uint8_t length)
{
    crc = 0xffff;
    uint8_t i = 0;

    for(; i<length; i++)
    {
        update_crc(data[i]);
    }
    return crc;
}

void pn9_next(uint16_t *last)
{
    uint16_t pn9_new;
    pn9_new =  (((*last & 0x20) >> 5) ^ *last) << 8;
    pn9_new |= (*last >> 1) & 0xff;
    *last = pn9_new & 0x1ff;
}

uint16_t pn9_generator(uint16_t *pn9)
{
    int i;

    for (i=0; i<8; i++) {
        pn9_next(pn9);
    }
    return *pn9;
}

void pn9_encode(uint8_t *data, uint16_t length)
{
    uint16_t pn9 = PN9_INITIALIZER; //// LFSR initialised to the specified polynomial
    uint8_t *p = data;

    for (; p < data + length; p++) {
        *p ^= pn9;
        pn9_generator(&pn9);
    }
}

uint8_t compress_data(uint16_t value, bool ceil)
{
    uint8_t mantissa;
    uint16_t remainder;

    for ( int i = 0; i < 8; i++)
    {
        if (value <= (pow(4, i) * 31))
        {
            mantissa = value / pow(4, i);
            remainder = value % (uint16_t)(pow(4, i));

            if (ceil && remainder)
                mantissa++;

            return (uint8_t)( i<<5 | mantissa);
        }
    }

    return 0;
}

const char *int_to_binary(uint16_t x)
{
    static char b[17];
    b[0] = '\0';

    uint16_t z;
    for (z = 0x8000; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}

#ifdef ENABLE_PRINT_VSTATE
static void print_vstate(void)
{
//	typedef struct {
//		uint8_t cost;
//		uint16_t path;
//	} VITERBIPATH;
//	typedef struct {
//		uint8_t path_size;
//		VITERBIPATH* old;
//		VITERBIPATH* new;
//		VITERBIPATH states1[8];
//		VITERBIPATH states2[8];
//	} VITERBISTATE;

  DPRINT("VSTATE:\n");
  DPRINT(" - path_size: %d\n", vstate.path_size);
	//printf(" - old: %03d - %s\n", vstate.old->cost, int_to_binary(vstate.old->path));
	//printf(" - new: %03d - %s\n", vstate.new->cost, int_to_binary(vstate.new->path));
	int i;
	for (i=0;i<8;i++)
    DPRINT(" - states - %d: %03d - %s\n", i, vstate.old[i].cost, int_to_binary(vstate.old[i].path));

}
#endif

uint16_t fec_calculated_decoded_length(uint16_t packet_length)
{
	return 2* (packet_length + 2 - (packet_length % 2));
}

/* Convolutional encoder */
uint16_t fec_encode(uint8_t *data, uint16_t nbytes)
{
	memcpy(data_buffer, data, nbytes);
	uint8_t *input = data_buffer;
	unsigned int encstate = 0;
	int i;

	int termintor_bytes = 2 + nbytes%2;
	//printf("Length %d -> terminator %d\n", nbytes, termintor_bytes);
	nbytes+=termintor_bytes;
	uint16_t length = 0;
	uint8_t fecbuffer[4] = {0,0,0,0};

	int8_t buffer_pointer = 0;
	while(nbytes-- > 0){
		if (nbytes < termintor_bytes) *input = TRELLIS_TERMINATOR;

		//printf("%02X:", *input);

		int j = 6;
		for(i=7;i>=0;i--){
				encstate = (encstate << 1) | ((*input >> i) & 1);
				fecbuffer[buffer_pointer] |= fec_lut[encstate & 0x0F] << j;
				j -=2;

				//printf("%d: %d - %d -> %s\n",j+2, ((*input >> i) & 1), encstate & 0x0F, byte_to_binary(fec_lut[encstate & 0x0F]));
				if (j < 0)
				{
//						//fecbuffer[buffer_pointer] = 0;
					buffer_pointer++;
					j = 6;
//						length++;
				}
		}

		if (buffer_pointer == 4)
		{
#ifdef INTERLEAVING
			//printf("Non: "); print_array(fecbuffer, 4); printf("\n");
			//Interleaving and write to output buffer
			*data++ = ((fecbuffer[0] & 0x03)) |\
						((fecbuffer[1] & 0x03) << 2) |\
						((fecbuffer[2] & 0x03) << 4) |\
						((fecbuffer[3] & 0x03) << 6);
			*data++ = (((fecbuffer[0] >> 2) & 0x03)) |\
							(((fecbuffer[1] >> 2) & 0x03) << 2) |\
							(((fecbuffer[2] >> 2) & 0x03) << 4) |\
							(((fecbuffer[3] >> 2) & 0x03) << 6);
			*data++ = (((fecbuffer[0] >> 4) & 0x03)) |\
							(((fecbuffer[1] >> 4) & 0x03) << 2) |\
							(((fecbuffer[2] >> 4) & 0x03) << 4) |\
							(((fecbuffer[3] >> 4) & 0x03) << 6);
			*data++ = (((fecbuffer[0] >> 6) & 0x03)) |\
							(((fecbuffer[1] >> 6) & 0x03) << 2) |\
							(((fecbuffer[2] >> 6) & 0x03) << 4) |\
							(((fecbuffer[3] >> 6) & 0x03) << 6);
			//printf("Int: "); print_array(output-4, 4); printf("\n");
#else
			*output++ = fecbuffer[0];
			*output++ = fecbuffer[1];
			*output++ = fecbuffer[2];
			*output++ = fecbuffer[3];

#endif
			buffer_pointer=0;
			fecbuffer[0] = 0;
			fecbuffer[1] = 0;
			fecbuffer[2] = 0;
			fecbuffer[3] = 0;
			length+=4;
		}


		//printf("%02X%02X ", *(output-2), *(output-1));

		input++;

	}


	//printf("\n");
	return length;
}

uint16_t fec_decode_packet(uint8_t* data, uint16_t packet_length, uint16_t output_length)
{
	uint8_t* output = data_buffer;
	if(output_length < packet_length)
	{
		DPRINT("FEC decoding error: buffer to small\n");
		return 0;
	}

	if(packet_length % 4 != 0)
	{
		DPRINT("FEC decoding error: data 32 bit aligned\n");
		return 0;
	}

	output_buffer = output;
	packetlength = packet_length;
	output_packet_length = output_length;

	processedbytes = 0;
	fecprocessedbytes = 0;

	vstate.path_size = 0;

	vstate.states1[0].cost = 0;
	int16_t i;
	for (i=1;i<8;i++)
			vstate.states1[i].cost = 100;

	vstate.old = vstate.states1;
	vstate.new = vstate.states2;

	uint16_t decoded_length = 0;

	for(i = 0; i < packet_length; i = i + 4)
	{
		//printf("FEC encoding i = %d\n", i);

		bool err = fec_decode(&data[i]);
		decoded_length+=2;
		if (!err)
		{
			DPRINT("FEC encoding error\n");
		}
	}

	memcpy(data, data_buffer, decoded_length);

	return decoded_length;
}

static bool fec_decode(uint8_t* input)
{
	uint8_t i, k;
	int8_t j;
	uint8_t min_state;
	uint8_t symbol;
	uint8_t fecbuffer[4];
	VITERBIPATH* vstate_tmp;

	if(fecprocessedbytes >= packetlength)
		return false;

	//Deinterleaving (symbols are stored in reverse as this is easier for Viterbi decoding)

#ifdef INTERLEAVING
	//printf("Int: "); print_array(input, 4); printf("\n");
	fecbuffer[0] = ((input[0] & 0x03)) |\
					((input[1] & 0x03) << 2) |\
					((input[2] & 0x03) << 4) |\
					((input[3] & 0x03) << 6);
	fecbuffer[1] = (((input[0] >> 2) & 0x03)) |\
					(((input[1] >> 2) & 0x03) << 2) |\
					(((input[2] >> 2) & 0x03) << 4) |\
					(((input[3] >> 2) & 0x03) << 6);
	fecbuffer[2] = (((input[0] >> 4) & 0x03)) |\
					(((input[1] >> 4) & 0x03) << 2) |\
					(((input[2] >> 4) & 0x03) << 4) |\
					(((input[3] >> 4) & 0x03) << 6);
	fecbuffer[3] = (((input[0] >> 6) & 0x03)) |\
					(((input[1] >> 6) & 0x03) << 2) |\
					(((input[2] >> 6) & 0x03) << 4) |\
					(((input[3] >> 6) & 0x03) << 6);
	//printf("DeI: "); print_array(fecbuffer, 4); printf("\n");
#else
	fecbuffer[0] = input[0];
	fecbuffer[1] = input[1];
	fecbuffer[2] = input[2];
	fecbuffer[3] = input[3];
#endif
	//printf(" input = %04X%04X\n", fecbuffer[0],fecbuffer[1]);
	fecprocessedbytes +=4;

	for (i = 0; i < 3; i=i+2) {
		//Viterbi decoding
		//printf(" Encode i  %d -> %04X %s\n", i, fecbuffer[i], int_to_binary(fecbuffer[i]));

		// todo: fix for loop
		for (j = 7; j >= 0; j--) {
			if (j>3)
				symbol = (fecbuffer[i] >> (j-4)*2) & 0x03;
			else
				symbol = (fecbuffer[i+1] >> j*2) & 0x03;
			//printf(" Symbol %d %x - %s\n", j, symbol, int_to_binary(symbol));
			//fecbuffer[i] >>= 2;

			for(k = 0; k < 8; k++) {
				uint8_t cost0, cost1;
				uint8_t state0, state1;
				uint8_t hamming0, hamming1;

				state0 = k >> 1;
				state1 = state0 + 4;

				cost0  = vstate.old[state0].cost;
				cost1  = vstate.old[state1].cost;

				//butterfly operation for 0
				hamming0 = cost0 + (((trellis0_lut[state0] ^ symbol) + 1) >> 1);
				hamming1 = cost1 + (((trellis0_lut[state1] ^ symbol) + 1) >> 1);

				if(hamming0 <= hamming1) {
					vstate.new[k].cost = hamming0;
					vstate.new[k].path = vstate.old[state0].path << 1;
				} else {
					vstate.new[k].cost = hamming1;
					vstate.new[k].path = vstate.old[state1].path << 1;
				}

				//printf("k %d part 1\n");
#ifdef ENABLE_PRINT_VSTATE
				print_vstate();
#endif

				k++;

				//butterfly operation for 1
				hamming0 = cost0 + (((trellis1_lut[state0] ^ symbol) + 1) >> 1);
				hamming1 = cost1 + (((trellis1_lut[state1] ^ symbol) + 1) >> 1);

				if(hamming0 <= hamming1) {
					vstate.new[k].cost = hamming0;
					vstate.new[k].path = vstate.old[state0].path << 1 | 0x01;
				} else {
					vstate.new[k].cost = hamming1;
					vstate.new[k].path = vstate.old[state1].path << 1 | 0x01;
				}

				//printf("k %d part 2\n");
				//print_vstate();
			}

			//Swap Viterbi paths
			vstate_tmp = vstate.new;
			vstate.new = vstate.old;
			vstate.old = vstate_tmp;

#ifdef ENABLE_PRINT_VSTATE
			print_vstate();
#endif
		}

		vstate.path_size++;

		//Flush out byte if path is full
		if ((vstate.path_size == 2) && (processedbytes < packetlength)) {
			//Calculate path with lowest cost
			min_state = 0;
			for (j = 7; j != 0; j--) {
				if(vstate.old[j].cost < vstate.old[min_state].cost)
					min_state = j;
			}

	        //Normalize costs
			if (vstate.old[min_state].cost > 0)
				for (j = 0; j < 8; j++) vstate.old[j].cost -= vstate.old[min_state].cost;

			*output_buffer++ = vstate.old[min_state].path >> 8;
			vstate.path_size--;

			processedbytes++;

			if (processedbytes+ 2 == packetlength)
				*output_buffer = (uint8_t) (vstate.old[min_state].path);
		}
	}

	//print_vstate();
	return true;
}
