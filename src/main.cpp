#include <stdio.h>

#define LT_IMPLEMENTATION
#include "lt.hpp"
#include "lt_math.hpp"
#define LT_IMAGE_IMPLEMENTATION
#include "lt_image.hpp"

// TODO(leo): this is far from complete.
struct ObjFile {
    Array<Vec3f> *vertices;
    isize         num_vertices;
    Array<Vec3i> *faces;
    isize         num_faces;
};

internal ObjFile *
load_obj(const char *filepath)
{
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        LT_FAIL("Failed to open %s\n", filepath);
    }

#define BUF_SIZE 1000

    char buf[BUF_SIZE] = {0};

    Array<Vec3f> *vertices;
    isize num_vertices;
    Array<Vec3i> *faces;
    isize num_faces;

    while(fgets(buf, BUF_SIZE, fp) != NULL) {
        if (buf[0] == 'v') {
            Vec3f vertice;
            sscanf(buf, "v %f %f %f", &vertice.x, &vertice.y, &vertice.z);
        } else if (buf[0] == 'f') {
            Vec3i face;

        } else {
            LT_FAIL("Line started with %c\n", buf[0]);
        }
    }

#undef BUF_SIZE
}

internal void
draw_line(TGAImageRGBA *img, Vec2i p0, Vec2i p1, const Vec4i color)
{
    i32 size_x = lt_abs(p0.x - p1.x);
    i32 size_y = lt_abs(p0.y - p1.y);

    bool steep = false;

    if (size_y > size_x) {
        // Transposition
        steep = true;
        lt_swap(&p0.x, &p0.y);
        lt_swap(&p1.x, &p1.y);
    }
    if (p0.x > p1.x) {
        // Make sure that p0's x is always smaller than p1's x.
        lt_swap(&p0.x, &p1.x);
        lt_swap(&p0.y, &p1.y);
    }

    LT_ASSERT(p0.x < p1.x);

    i32 dx = p1.x - p0.x;
    i32 dy = p1.y - p0.y;
    i32 derr = lt_abs(dy)*2;
    i32 err = 0;

    i32 y = p0.y;

    for (isize x = p0.x; x < p1.x; x++) {
        if (steep) {
            lt_image_set(img, y, x, color);
        } else {
            lt_image_set(img, x, y, color);
        }

        err += derr;
        if (err > dx) {
            y += (p1.y > p0.y) ? 1 : -1;
            err -= 2*dx;
        }
    }
}

int
main(void)
{
    TGAImageRGBA *img = lt_image_make_rgba(800, 600);
    /* TGAImageGray *img = lt_image_make_gray(800, 600); */

    Vec4i black(0, 0, 0, 255);
    Vec4i red(255, 0, 0, 255);

    lt_image_fill(img, black);

    draw_line(img, Vec2i(0, 0), Vec2i(50, 50), red);
    draw_line(img, Vec2i(30, 599), Vec2i(100, 100), red);
    draw_line(img, Vec2i(30, 299), Vec2i(400, 100), red);
    draw_line(img, Vec2i(500, 409), Vec2i(300, 100), red);
    draw_line(img, Vec2i(500, 500), Vec2i(700, 500), red);
    draw_line(img, Vec2i(500, 500), Vec2i(700, 470), red);

    // output and cleanup
    lt_image_write_to_file(&img->header, "../test.tga");
    lt_image_free(img);
}
