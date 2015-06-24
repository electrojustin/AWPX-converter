#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <byteswap.h>
#include <string.h>
#include <GL/glut.h>
#include "format.h"

int width;
int height;
char* pic;

void draw (void);
void resize (int new_width, int new_height);

int main (int argc, char** argv)
{
	FILE* input_file;
	char* input_buf;
	size_t input_size;
	awpx_hdr* file_header;
	awpx_hdr actual_file_header;
	memset (&actual_file_header, 0, sizeof (awpx_hdr));
	layer_hdr* layer_table;
	layer_hdr actual_layer_hdr;
	memset (&actual_layer_hdr, 0, sizeof (layer_hdr));
	int i;
	int j = 0;
	int current_color_ind;

	if (argc != 2)
	{
		printf ("Usage: viewer <filename>\n");
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

	//Print some basic information about the image
	printf ("Layers: %d\n", actual_file_header.layer_count);
	printf ("Size: %d by %d\n", actual_layer_hdr.width, actual_layer_hdr.height);
	printf ("Pixel packing type: %d\n", actual_layer_hdr.pix_pack_type);
	printf ("Color type: %d\n", actual_layer_hdr.color_pack_type);
	printf ("Compression: %d\n", actual_layer_hdr.pix_comp_type);
	printf ("Lacing: %d\n", actual_layer_hdr.pix_lace_type);
	printf ("Num colors: %d\n", actual_layer_hdr.color_count);
	printf ("Depth: %d\n", actual_layer_hdr.depth);
	printf ("Ignore color: %08x\n", actual_layer_hdr.trans_color);
	printf ("Row bytes: %d\n", actual_layer_hdr.row_bytes);

	//Allocate space for the draw buffer
	width = actual_layer_hdr.width;
	height = actual_layer_hdr.height;
	pic = malloc (width*height*3);


	for (i = 0; i < width*height; i ++)
	{
		//Get the index into the color pallette the current pixel is using
		current_color_ind = *(unsigned char*)(input_buf + sizeof (awpx_hdr) + sizeof (layer_hdr) + actual_layer_hdr.pixel_offset + i);

		//Copy the color into our draw buffer
		//NOTE: DESPITE WHAT THE DOC SAYS, THE COLOR PALLETTE IS ALWAYS RGBA, NOT RGB EVEN THOUGH colorPackType IS 0
		//This is why I have "current_color_index*4" instead of "current_color_index*3"
		pic [j] = *(input_buf + sizeof (awpx_hdr) + sizeof (layer_hdr) + actual_layer_hdr.color_offset + current_color_ind*4);
		pic [j+1] = *(input_buf + sizeof (awpx_hdr) + sizeof (layer_hdr) + actual_layer_hdr.color_offset + current_color_ind*4 + 1);
		pic [j+2] = *(input_buf + sizeof (awpx_hdr) + sizeof (layer_hdr) + actual_layer_hdr.color_offset + current_color_ind*4 + 2);

		//Handle transparency
		if ((unsigned char)pic [j] == actual_layer_hdr.trans_color >> 24 &&
		    (unsigned char)pic [j+1] == ((actual_layer_hdr.trans_color >> 16) & 0xFF) &&
		    (unsigned char)pic [j+2] == ((actual_layer_hdr.trans_color >> 8) & 0xFF))
		{
			pic [j] = 0xFF;
			pic [j+1] = 0xFF;
			pic [j+2] = 0xFF;
		}

		j += 3;
	}

	//Some OpenGL nonsense
        glutInitWindowSize (width, height);
        glutInit (&argc, argv);
	glutInitDisplayMode (GLUT_RGB | GLUT_DOUBLE);
	glutCreateWindow ("test");
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
	glOrtho (0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
	glClear (GL_COLOR_BUFFER_BIT);	
	glutDisplayFunc (draw);
	glutReshapeFunc (resize);

	//Show picture!
	glutMainLoop ();

	free (input_buf);
	free (pic);
}

void draw (void)
{
	glDrawPixels (width, height, GL_RGB, GL_UNSIGNED_BYTE, pic);
	glutSwapBuffers ();
	glutPostRedisplay ();
	usleep (1000);
}

void resize (int new_width, int new_height)
{
	glutReshapeWindow (width, height);
}
