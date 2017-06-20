#ifndef LT_IMAGE_HPP
#define LT_IMAGE_HPP

#include <stdio.h>
#include "lt.hpp"

#define TGA_IMAGE_HEADER_SIZE 18
#define TGA_IMAGE_FOOTER_SIZE 26

union Vec4i;

enum TGAPixel {
    TGAPixel_Gray = 8,
    TGAPixel_RGB = 24,
    TGAPixel_RGBA = 32,
};

struct TGAImageHeader {
    u8     id_length;
    u8     colormap_type;
    u8     image_type;
    // Color Map Specification
    u16    first_entry_index;
    u16    colormap_length;
    u8     colormap_entry_size;
    // Image Specification
    u16    xorigin;
    u16    yorigin;
    u16    image_width;
    u16    image_height;
    u8     pixel_depth;
    u8     image_descriptor;
};

struct TGAImageFooter {
    u32    extension_area_offset;
    u32    developer_dir_offset;
    u8     signature[16];
    u8     reserved;
    u8     zero_string_terminator;
};

struct TGAImageGray {
    TGAImageHeader   header;
    TGAImageFooter   footer;
    u8              *data;
};

struct TGAImageRGBA {
    TGAImageHeader    header;
    TGAImageFooter    footer;
    u32              *data;
};

TGAImageGray *lt_image_make_gray     (u16 width, u16 height);
TGAImageRGBA *lt_image_make_rgba     (u16 width, u16 height);
TGAImageRGBA *lt_image_load_rgba     (const char *filepath);
void          lt_image_write_to_file (const TGAImageHeader *header, const char *filepath);
void          lt_image_fill          (TGAImageGray *img, u8 v);
void          lt_image_fill          (TGAImageRGBA *img, const Vec4i c);
void          lt_image_set           (TGAImageGray *img, u16 x, u16 y, u8 v);
void          lt_image_set           (TGAImageRGBA *img, u16 x, u16 y, const Vec4i c);

#ifndef lt_image_free
#define lt_image_free(img) do { \
        lt_free(img->data);     \
        lt_free(img);           \
    } while(0)
#endif

#endif // LT_IMAGE_HPP
