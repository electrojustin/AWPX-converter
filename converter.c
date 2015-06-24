#include <stdio.h>
#include <stdlib.h>
#include <byteswap.h>
#include <string.h>
#include <png.h>
#include "format.h"

int main (int argc, char** argv)
{
	FILE* input_file;
	FILE* output_file;
	png_structp png;
	png_infop png_info;
	char* input_buf;
	size_t input_size;
	awpx_hdr* file_header;
	awpx_hdr actual_file_header;
	memset (&actual_file_header, 0, sizeof (awpx_hdr));
	layer_hdr* layer_table;
	layer_hdr actual_layer_hdr;
	memset (&actual_layer_hdr, 0, sizeof (layer_hdr));
	int i;
	int current_color_ind;
	png_bytep* rows;
	png_byte* current_row;

	if (argc != 3)
	{
		printf ("Usage: converter <input filename> <output filename>\n");
		exit (-1);
	}

	//Load the file into memory
	input_file = fopen (argv [1], "r");
	if (!input_file)
	{
		printf ("No such file or directory\n");
		exit (-1);
	}
	fseek (input_file, 0, SEEK_END);
	input_size = ftell (input_file);
	fseek (input_file, 0, SEEK_SET);
	input_buf = malloc (input_size);
	fread (input_buf, 1, input_size, input_file);
	fclose (input_file);

	file_header = (awpx_hdr*)input_buf;
	layer_table = (layer_hdr*)(input_buf + sizeof (awpx_hdr));

	//Basic sanity check
	if (file_header->format != 0x58505741)
	{
		printf ("Not an AWPX file\n");
		exit (-1);
	}

	//Deal with endian nonsense
	actual_file_header.version = __bswap_16 (file_header->version);
	actual_file_header.layer_count = __bswap_16 (file_header->layer_count);

	actual_layer_hdr.width = __bswap_32 (layer_table [0].width);
	actual_layer_hdr.height = __bswap_32 (layer_table [0].height);
	actual_layer_hdr.row_bytes = __bswap_32 (layer_table [0].row_bytes);
	actual_layer_hdr.misc_data_size = __bswap_32 (layer_table [0].misc_data_size);
	actual_layer_hdr.color_offset = __bswap_32 (layer_table [0].color_offset);
	actual_layer_hdr.pixel_offset = __bswap_32 (layer_table [0].pixel_offset);
	actual_layer_hdr.misc_offset = __bswap_32 (layer_table [0].misc_offset);
	actual_layer_hdr.trans_color = __bswap_32 (layer_table [0].trans_color);
	actual_layer_hdr.color_count = __bswap_16 (layer_table [0].color_count);
	actual_layer_hdr.depth = layer_table [0].depth;
	actual_layer_hdr.pix_pack_type = layer_table [0].pix_pack_type;
	actual_layer_hdr.pix_comp_type = layer_table [0].pix_comp_type;
	actual_layer_hdr.pix_lace_type = layer_table [0].pix_lace_type;
	actual_layer_hdr.color_pack_type = layer_table [0].color_pack_type;
	actual_layer_hdr.opts = layer_table [0].opts;

	//Makes sure the width matches the number of bytes per row.
	if (actual_layer_hdr.width != actual_layer_hdr.row_bytes)
	{
		//They don't match, somethings not right.
		printf ("WARNING: AWPX file appears to be corrupt! Bytes per row does not match width!\nAttempting to repair...\n");
		//Go with the row bytes, they're more accurate in my experience
		actual_layer_hdr.width = actual_layer_hdr.row_bytes;
	}

	//Allocate memory for the final image
	rows = (png_bytep*)malloc (actual_layer_hdr.height * sizeof (png_bytep));
	for (i = 0; i < actual_layer_hdr.height; i ++)
		rows [i] = (png_byte*)malloc (actual_layer_hdr.width * 4 * sizeof (png_byte));

	for (i = 0; i < actual_layer_hdr.height * actual_layer_hdr.width; i ++)
	{
		//Get the index into the color pallette the current pixel is using
		current_color_ind = *(unsigned char*)(input_buf + sizeof (awpx_hdr) + sizeof (layer_hdr) + actual_layer_hdr.pixel_offset + i);

		//Copy pixel data
		current_row = rows [i / actual_layer_hdr.width];
		current_row [i % actual_layer_hdr.width * 4] = *(input_buf + sizeof (awpx_hdr) + sizeof (layer_hdr) + actual_layer_hdr.color_offset + current_color_ind*4);
		current_row [i % actual_layer_hdr.width * 4 + 1] = *(input_buf + sizeof (awpx_hdr) + sizeof (layer_hdr) + actual_layer_hdr.color_offset + current_color_ind*4 + 1);
		current_row [i % actual_layer_hdr.width * 4 + 2] = *(input_buf + sizeof (awpx_hdr) + sizeof (layer_hdr) + actual_layer_hdr.color_offset + current_color_ind*4 + 2);
		//All these images have 0's in their alpha channels, which libpng interprets as full transparency.
		current_row [i % actual_layer_hdr.width * 4 + 3] = 0xFF;

		//Handle transparency
		if ((unsigned char)current_row [i % actual_layer_hdr.width * 4] == actual_layer_hdr.trans_color >> 24 &&
		    (unsigned char)current_row [i % actual_layer_hdr.width * 4 + 1] == ((actual_layer_hdr.trans_color >> 16) & 0xFF) &&
		    (unsigned char)current_row [i % actual_layer_hdr.width * 4 + 2] == ((actual_layer_hdr.trans_color >> 8) & 0xFF))
			current_row [i % actual_layer_hdr.width * 4 + 3] = 0;
	}

	//Open output file for writing
	output_file = fopen (argv [2], "w");

	//Muck about with libpng
	png = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_info = png_create_info_struct (png);
	png_init_io (png, output_file);
	png_set_IHDR (png, png_info, actual_layer_hdr.width, actual_layer_hdr.height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	
	//Write to output file
	png_write_info (png, png_info);
	png_write_image (png, rows);
	png_write_end (png, NULL);

	//Cleanup
	fclose (output_file);
	for (i = 0; i < actual_layer_hdr.height; i ++)
		free (rows [i]);
	free (rows);
	free (input_buf);
}
