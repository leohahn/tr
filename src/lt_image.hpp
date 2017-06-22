#ifndef LT_IMAGE_HPP
#define LT_IMAGE_HPP

#include <stdio.h>
#include "lt.hpp"

#define TGA_IMAGE_HEADER_SIZE 18
#define TGA_IMAGE_FOOTER_SIZE 26

union Vec4i;
union Vec3i;

enum TGAPixel {
    TGAPixel_Gray = 8,
    TGAPixel_RGB = 24,
    TGAPixel_RGBA = 32,
};

enum TGAType {
    TGAType_NoData = 0,
    TGAType_Uncompressed_ColorMapped = 1,
    TGAType_Uncompressed_TrueColor = 2,
    TGAType_Uncompressed_BlackWhite = 3,
    TGAType_RunLength_ColorMapped = 9,
    TGAType_RunLength_TrueColor = 10,
    TGAType_RunLength_BlackWhite = 11,
};

static_assert(TGAPixel_Gray/8 == sizeof(u8), "8 bits");
static_assert(TGAPixel_RGB/8 == sizeof(u8)*3, "24 bits");
static_assert(TGAPixel_RGBA/8 == sizeof(u32), "32 bits");

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

struct TGAImageRGB {
    TGAImageHeader    header;
    TGAImageFooter    footer;
    u32              *data;
};

TGAImageGray *lt_image_make_gray     (u16 width, u16 height);
TGAImageRGBA *lt_image_make_rgba     (u16 width, u16 height);
TGAImageRGB  *lt_image_load_rgb      (const char *filepath);
void          lt_image_fill          (TGAImageGray *img, u8 v);
void          lt_image_fill          (TGAImageRGBA *img, const Vec4i c);
void          lt_image_set           (TGAImageGray *img, u16 x, u16 y, u8 v);
void          lt_image_set           (TGAImageRGBA *img, u16 x, u16 y, const Vec4i c);
Vec3i         lt_image_get           (TGAImageRGB  *img, u16 x, u16 y);
template<typename T> void lt_image_write_to_file(const T *img, const char *filepath);
template<typename T> i32 lt_image_height(T *img);
template<typename T> i32 lt_image_width(T *img);
template<typename T> i32 lt_image_area(T *img);

#ifndef lt_image_free
#define lt_image_free(img) do { \
        lt_free(img->data);     \
        lt_free(img);           \
    } while(0)
#endif


#endif // LT_IMAGE_HPP

/* =========================================================================
 *
 *
 *
 *
 *
 *
 *  Implementation
 *
 *
 *
 *
 *
 *
 * ========================================================================= */

#if defined(LT_IMAGE_IMPLEMENTATION) && !defined(LT_IMAGE_IMPLEMENTATION_DONE)
#define LT_IMAGE_IMPLEMENTATION_DONE

#include <stdio.h>
#include <stdlib.h>
#include "lt_math.hpp"

typedef u8 RepetitionCount;

inline u32
pack_rgba(const Vec4i c)
{
    u32 color = (c.a << 24) | (c.r << 16) | (c.g << 8) | c.b;
    return color;
}

inline Vec3i
unpack_rgb(u32 c)
{
    LT_Assert(((c >> 24) & 0xff) == 0);
    u32 r = (c >> 16) & 0xff;
    u32 g = (c >> 8) & 0xff;
    u32 b = c & 0xff;
    return Vec3i(r, g, b);
}

template<typename T> internal void
write_image_data(const T *img, FILE *fd)
{
    for (isize i = 0; i < img->header.image_width*img->header.image_height; i++)
        LT_Assert(fwrite(&img->data[i], img->header.pixel_depth/8, 1, fd) == 1);
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
    // Make sure that the descriptor is at the start of the file.
    LT_Assert(fseek(fd, 0L, SEEK_SET) == 0);

    const i32 num_elements_to_read = 1;
    u8 header_buf[TGA_IMAGE_HEADER_SIZE] = {};
    usize bytes_read = fread(header_buf, TGA_IMAGE_HEADER_SIZE, num_elements_to_read, fd);

    LT_Assert(bytes_read == num_elements_to_read);

    i32 offset = 0;
    memcpy(&header->id_length, header_buf, sizeof(u8));
    offset = 1;
    memcpy(&header->colormap_type, header_buf + offset, sizeof(u8));
    offset = 2;
    memcpy(&header->image_type, header_buf + offset, sizeof(u8));
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
load_footer(TGAImageFooter *footer, FILE* fd)
{
    LT_Assert(lt_is_little_endian());

    u8 footer_buf[TGA_IMAGE_FOOTER_SIZE] = {};

    // Go to the end of the file and go back the size of the footer.
    // After read the size of the footer.
    LT_Assert(fseek(fd, -TGA_IMAGE_FOOTER_SIZE, SEEK_END) == 0);
    const i32 num_elements_to_read = 1;
    usize bytes_read = fread(footer_buf, TGA_IMAGE_FOOTER_SIZE, num_elements_to_read, fd);
    LT_Assert(bytes_read == num_elements_to_read);
    // Copy the buffer contents to the footer structure.
    i32 offset = 0;
    memcpy(&footer->extension_area_offset, footer_buf + offset, sizeof(u32));
    offset += sizeof(u32);
    memcpy(&footer->developer_dir_offset, footer_buf + offset, sizeof(u32));
    offset += sizeof(u32);
    memcpy(footer->signature, footer_buf + offset, 16);
    offset += 16;
    memcpy(&footer->reserved, footer_buf + offset, 1);
    offset += 1;
    memcpy(&footer->zero_string_terminator, footer_buf + offset, 1);
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
    TGAImageGray *img = (TGAImageGray*)calloc(1, sizeof(*img));

    initialize_header(&img->header, width, height, TGAPixel_Gray);
    img->data = (u8*)calloc(width * height,  sizeof(u8));
    initialize_footer(&img->footer);

    return img;
}

TGAImageRGBA *
lt_image_make_rgba(u16 width, u16 height)
{
    TGAImageRGBA *img = (TGAImageRGBA*)calloc(1, sizeof(*img));

    initialize_header(&img->header, width, height, TGAPixel_RGBA);
    img->data = (u32*)calloc(width * height,  sizeof(u32));
    initialize_footer(&img->footer);

    return img;
}

TGAImageRGB *
lt_image_load_rgb(const char *filepath)
{
    FILE *fd = fopen(filepath, "rb");
    LT_Assert(fd);

    TGAImageRGB *img = (TGAImageRGB*)calloc(1, sizeof(*img));
    load_header(&img->header, fd);
    load_footer(&img->footer, fd);

    // NOTE(leo): This assertions facilitate the implementation, since I do not have to
    // implement all of the possible image combinations.
    LT_Assert(strncmp((const char*)img->footer.signature, "TRUEVISION-XFILE", 16) == 0);
    LT_Assert(img->header.id_length == 0);
    LT_Assert(img->header.pixel_depth == TGAPixel_RGB);
    LT_Assert((img->header.image_descriptor & (0x03 << 4)) == 0);

    switch (img->header.image_type)
    {
    case TGAType_RunLength_TrueColor:
    {
        LT_Assert(img->header.colormap_type == 0);

        // Seek descriptor to the correct position.
        u32 colormap_size = img->header.colormap_entry_size * img->header.colormap_length;
        LT_Assert(fseek(fd, TGA_IMAGE_HEADER_SIZE + img->header.id_length + colormap_size, SEEK_SET) == 0);

        usize image_data_length = img->header.image_width*img->header.image_height;
        img->data = (u32*)calloc(image_data_length, sizeof(u32));

        // NOTE(leo): A run-length encoded packed is composed of two fields.
        // RepetitionCount (first byte):
        //    - 1st bit: 1 = run-length encoded packet, 0 = raw packet.
        //    - 7 bits left: number of pixels - 1. Always add 1 to get the real number of pixels.
        // PixelValue (variable):
        //    - The actual pixel values. If it is a raw packet, it will contain N pixels.
        //    - If it is run-length encoded, it will contain only one pixel, but repeated based on the 7 bits from
        //    the header.
        const u32 MAX_PIXELS = 0x7f + 1; // 128
        RepetitionCount header;
        u8 pixel_value[MAX_PIXELS * 3];

        isize image_data_pixel = 0;
        while (image_data_pixel < image_data_length)
        {
            // Clear buffer at the start of the iteration.
            memset(pixel_value, 0, MAX_PIXELS * 3);

            i32 c = getc(fd);
            LT_Assert(c != EOF);

            header = c;
            const bool run_length_packet = ((header >> 7) == 1);
            const isize num_pixels = (header & 0x7f) + 1;
            const isize until_pixel = image_data_pixel + num_pixels;
            LT_Assert(num_pixels <= MAX_PIXELS);
            if (run_length_packet)
            {
                LT_Assert(fread(pixel_value, 3, 1, fd) == 1);
                // Copy the data from pixel_value to image data.
                for (; image_data_pixel < until_pixel; image_data_pixel++)
                    memcpy(&img->data[image_data_pixel], pixel_value, 3);
            }
            else
            {
                LT_Assert(fread(pixel_value, 3, num_pixels, fd) == num_pixels);
                for (isize pixel_i = 0; pixel_i < num_pixels*3; pixel_i+=3)
                {
                    memcpy(&img->data[image_data_pixel], pixel_value + pixel_i, 3);
                    image_data_pixel++;
                }
            }
        }
    } break;
    default:
        LT_Fail("Case not yet implemented.");
    }

    img->header.image_type = TGAType_Uncompressed_TrueColor;
    fclose(fd);
    return img;
}

template<typename T> void
lt_image_write_to_file(const T *img, const char *filepath)
{
    FILE* fp = fopen(filepath, "w");
    if (!fp)
    {
        printf("Could not open %s", filepath);
        abort();
    }

    write_header(&img->header, fp);
    write_image_data(img, fp);
    write_footer(&img->footer, fp);

    fclose(fp);
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

void
lt_image_set(TGAImageGray *img, u16 x, u16 y, u8 shade)
{
    LT_Assert((img->header.image_descriptor & (0x03 << 4)) == 0);
    LT_Assert(x < img->header.image_width);
    LT_Assert(y < img->header.image_height);

    u32 index = (y * img->header.image_width) + x;
    LT_Assert(index < (u32)img->header.image_height * (u32)img->header.image_width);
    img->data[index] = shade;
}

void
lt_image_set(TGAImageRGBA *img, u16 x, u16 y, const Vec4i shade)
{
    LT_Assert((img->header.image_descriptor & (0x03 << 4)) == 0);
    LT_Assert(x < img->header.image_width);
    LT_Assert(y < img->header.image_height);

    u32 index = (y * img->header.image_width) + x;
    LT_Assert(index < (u32)img->header.image_height * (u32)img->header.image_width);

    img->data[index] = pack_rgba(shade);
}

Vec3i
lt_image_get(TGAImageRGB  *img, u16 x, u16 y)
{
    LT_Assert((img->header.image_descriptor & (0x03 << 4)) == 0);
    LT_Assert(x < img->header.image_width);
    LT_Assert(y < img->header.image_height);

    u32 index = (y * img->header.image_width) + x;
    LT_Assert(index < (u32)img->header.image_height * (u32)img->header.image_width);

    return unpack_rgb(img->data[index]);
}

template<typename T> i32 lt_image_height(T *img) {return img->header.image_height;}
template<typename T> i32 lt_image_width(T *img) {return img->header.image_width;}
template<typename T> i32 lt_image_area(T *img) {return img->header.image_width*img->header.image_height;}

#endif
