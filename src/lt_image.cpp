#include "lt_image.hpp"

#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

#include "lt.hpp"
#include "lt_math.hpp"

internal inline u32
pack_rgba(const Vec4i c)
{
    u32 color = (c.a << 24) | (c.r << 16) | (c.g << 8) | c.b;
    return color;
}

internal void
write_image_data(const TGAImageHeader *header, FILE* fp)
{
    switch (header->pixel_depth) {
    case TGAPixel_Gray: {
        TGAImageGray *img = (TGAImageGray*)header;
        i32 n = fwrite(img->data, sizeof(u8), header->image_width * header->image_height, fp);
        if (n < 0) {
            printf("Error writing to file, aborting\n");
            abort();
        }
    } break;
    case TGAPixel_RGBA: {
        TGAImageRGBA *img = (TGAImageRGBA*)header;
        i32 n = fwrite(img->data, sizeof(u32), header->image_width * header->image_height, fp);
        if (n < 0) {
            printf("Error writing to file, aborting\n");
            abort();
        }
    } break;

    default:
        LT_Assert(false);
    }
}

internal void
write_header(const TGAImageHeader *header, FILE* fp)
{
    LT_Assert(lt_is_little_endian());

    u8 buf[TGA_IMAGE_HEADER_SIZE];

    i32 offset = 0;
    memcpy(buf + offset, &header->id_length, sizeof(u8));
    offset = 1;
    memcpy(buf + offset, &header->colormap_type, sizeof(u8));
    offset = 2;
    memcpy(buf + offset, &header->image_type, sizeof(u8));
    offset = 3;
    memcpy(buf + offset, &header->first_entry_index, sizeof(u16));
    offset = 5;
    memcpy(buf + offset, &header->colormap_length, sizeof(u16));
    offset = 7;
    memcpy(buf + offset, &header->colormap_entry_size, sizeof(u8));
    offset = 8;
    memcpy(buf + offset, &header->xorigin, sizeof(u16));
    offset = 10;
    memcpy(buf + offset, &header->yorigin, sizeof(u16));
    offset = 12;
    memcpy(buf + offset, &header->image_width, sizeof(u16));
    offset = 14;
    memcpy(buf + offset, &header->image_height, sizeof(u16));
    offset = 16;
    memcpy(buf + offset, &header->pixel_depth, sizeof(u8));
    offset = 17;
    memcpy(buf + offset, &header->image_descriptor, sizeof(u8));

    fwrite(buf, sizeof(u8), TGA_IMAGE_HEADER_SIZE, fp);
}

internal void
load_header(TGAImageHeader *header, FILE* fd)
{
    LT_Assert(lt_is_little_endian());

    u8 header_buf[TGA_IMAGE_HEADER_SIZE] = {};
    usize bytes_read = fread(header_buf, TGA_IMAGE_HEADER_SIZE, 1, fd);

    LT_Assert(bytes_read == TGA_IMAGE_HEADER_SIZE);

    i32 offset = 0;
    memcpy(&header->id_length, header_buf, 1);
    offset = 1;
    memcpy(&header->colormap_type, header_buf + offset, 1);
    offset = 2;
    memcpy(&header->image_type, header_buf + offset, 1);
    offset = 3;
    memcpy(&header->first_entry_index, header_buf + offset, sizeof(u16));
    offset = 5;
    memcpy(&header->colormap_length, header_buf + offset, sizeof(u16));
    offset = 7;
    memcpy(&header->colormap_entry_size, header_buf + offset, sizeof(u8));
    offset = 8;
    memcpy(&header->xorigin, header_buf + offset, sizeof(u16));
    offset = 10;
    memcpy(&header->yorigin, header_buf + offset, sizeof(u16));
    offset = 12;
    memcpy(&header->image_width, header_buf + offset, sizeof(u16));
    offset = 14;
    memcpy(&header->image_height, header_buf + offset, sizeof(u16));
    offset = 16;
    memcpy(&header->pixel_depth, header_buf + offset, sizeof(u8));
    offset = 17;
    memcpy(&header->image_descriptor, header_buf + offset, sizeof(u8));
}

internal void
write_footer(const TGAImageFooter *footer, FILE* fp)
{
    fwrite(&footer->extension_area_offset, sizeof(u32), 1, fp);
    fwrite(&footer->developer_dir_offset, sizeof(u32), 1, fp);
    fwrite(footer->signature, sizeof(u8), 16, fp);
    fwrite(&footer->reserved, sizeof(u8), 1, fp);
    fwrite(&footer->zero_string_terminator, sizeof(u8), 1, fp);
}

internal void
initialize_header(TGAImageHeader *header, u16 width, u16 height, TGAPixel p)
{
    header->id_length = 0;
    header->colormap_type = 0; // No color map is used.
    header->image_type = 2;

    // This is related to colormap
    header->first_entry_index = 0;
    header->colormap_length = 0;
    header->colormap_entry_size = 0;

    header->xorigin = 0;
    header->yorigin = 0;
    header->image_width = width;
    header->image_height = height;
    header->pixel_depth = p;
    header->image_descriptor = (1 << 3);
}

internal void
initialize_footer(TGAImageFooter *footer)
{
    footer->extension_area_offset = 0;
    footer->developer_dir_offset = 0;
    memcpy(footer->signature, "TRUEVISION-XFILE", 16);
    footer->reserved = '.';
    footer->zero_string_terminator = 0;
}
/* ---------------------------------------------------------------
                      TGA Image Grayscale
 * --------------------------------------------------------------- */
TGAImageGray *
lt_image_make_gray(u16 width, u16 height)
{
    TGAImageGray *img = (TGAImageGray*)malloc(sizeof(*img));

    initialize_header(&img->header, width, height, TGAPixel_Gray);
    img->data = (u8*)calloc(width * height,  sizeof(u8));
    initialize_footer(&img->footer);

    return img;
}

TGAImageRGBA *
lt_image_make_rgba(u16 width, u16 height)
{
    TGAImageRGBA *img = (TGAImageRGBA*)malloc(sizeof(*img));

    initialize_header(&img->header, width, height, TGAPixel_RGBA);
    img->data = (u32*)calloc(width * height,  sizeof(u32));
    initialize_footer(&img->footer);

    return img;
}

TGAImageRGBA *
lt_image_load_rgba(const char *filepath)
{
    FILE *fd = fopen(filepath, "rb");
    LT_Assert(fd);

    TGAImageHeader header;
    load_header(&header, fd);

    LT_Assert(header.pixel_depth == TGAPixel_RGBA);

    // TODO(leo): Finish loading the data and the footer as well.
    LT_Fail("Not implemented yet\n");

    fclose(fd);
}

void
lt_image_set(TGAImageGray *img, u16 x, u16 y, u8 shade)
{
    LT_Assert(x < img->header.image_width);
    LT_Assert(y < img->header.image_height);

    u32 index = (y * img->header.image_width) + x;
    LT_Assert(index < (u32)img->header.image_height * (u32)img->header.image_width);
    img->data[index] = shade;
}

void
lt_image_set(TGAImageRGBA *img, u16 x, u16 y, const Vec4i shade)
{
    // printf("y: %u\n", y);
    fflush(stdout);
    LT_Assert(x < img->header.image_width);
    LT_Assert(y < img->header.image_height);

    u32 index = (y * img->header.image_width) + x;
    LT_Assert(index < (u32)img->header.image_height * (u32)img->header.image_width);

    img->data[index] = pack_rgba(shade);
}

void
lt_image_write_to_file(const TGAImageHeader *header, const char* filename)
{
    if (header->pixel_depth == TGAPixel_Gray)
    {
        FILE* fp = fopen(filename, "w");
        if (!fp)
        {
            printf("Could not open %s", filename);
            abort();
        }
        TGAImageGray *img = (TGAImageGray*)header;

        write_header(header, fp);
        write_image_data(header, fp);
        write_footer(&img->footer, fp);

        fclose(fp);
    }
    else if (header->pixel_depth == TGAPixel_RGBA)
    {
        FILE* fp = fopen(filename, "w");
        if (fp == NULL) {
            printf("Could not open %s", filename);
            abort();
        }
        TGAImageRGBA *img = (TGAImageRGBA*)header;

        write_header(header, fp);
        write_image_data(header, fp);
        write_footer(&img->footer, fp);

        fclose(fp);
    }
    else
    {
        LT_Assert(false);
    }
}

void
lt_image_fill(TGAImageGray *img, u8 gray)
{
    for (isize i = 0; i < img->header.image_width * img->header.image_height; i++)
        img->data[i] = gray;
}

void
lt_image_fill(TGAImageRGBA *img, const Vec4i c)
{
    for (isize i = 0; i < img->header.image_width * img->header.image_height; i++)
        img->data[i] = pack_rgba(c);
}
