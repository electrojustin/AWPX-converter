#include <inttypes.h>

struct awpx_hdr
{
	uint32_t format;
	uint16_t version;
	uint16_t rsvd_a;
	uint32_t rsvd_b;
	uint16_t rsvd_c;
	uint16_t layer_count;
};
typedef struct awpx_hdr awpx_hdr;

struct layer_hdr
{
	uint32_t width;
	uint32_t height;
	uint32_t row_bytes;
	uint32_t misc_data_size;
	uint32_t color_offset;
	uint32_t pixel_offset;
	uint32_t misc_offset;
	uint32_t trans_color;
	uint16_t rsvd_a;
	uint16_t color_count;
	uint16_t rsvd_b;
	uint16_t rsvd_c;
	uint8_t depth;
	uint8_t pix_pack_type;
	uint8_t pix_comp_type;
	uint8_t pix_lace_type;
	uint8_t color_pack_type;
	uint8_t opts;
	uint8_t rsvd_d;
	uint8_t rsvd_e;
};
typedef struct layer_hdr layer_hdr;
