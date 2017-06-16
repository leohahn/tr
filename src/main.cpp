#include <stdio.h>
#include <string.h>

#define LT_IMPLEMENTATION
#include "lt.hpp"
#include "lt_math.hpp"
#define LT_IMAGE_IMPLEMENTATION
#include "lt_image.hpp"

// TODO(leo): this is far from complete.
struct ObjFile {
    Array<Vec3f> vertices;
    Array<Vec3i> faces_vertices;
    Array<Vec3i> faces_textures;
    Array<Vec3i> faces_normals;
};

internal ObjFile
obj_file_load(const char *filepath)
{
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        LT_FAIL("Failed to open %s\n", filepath);
    }

    const i32 BUF_SIZE = 1000;
    char buf[BUF_SIZE] = {0};

    Array<Vec3f> vertices = array_make<Vec3f>();
    Array<Vec3i> faces_vertices = array_make<Vec3i>();
    Array<Vec3i> faces_textures = array_make<Vec3i>();
    Array<Vec3i> faces_normals = array_make<Vec3i>();

    while(fgets(buf, BUF_SIZE, fp) != NULL)
    {
        // Check if buffer was completely filled.
        if (buf[BUF_SIZE-1] != '\n' && buf[BUF_SIZE-1] != 0)
        {
            LT_FAIL("Buffer was overrun\n");
        }
        // Ignore certain lines.
        if (buf[0] == '#' || buf[0] == '\n' || buf[0] == 'g' || buf[0] == 's')
        {
            continue;
        }
        // Ignore vertex normals and texture coordinates.
        if (strncmp(buf, "vt", 2) == 0 ||
            strncmp(buf, "vn", 2) == 0)
        {
            continue;
        }

        if (buf[0] == 'v')
        {
            Vec3f v;
            sscanf(buf, "v %f %f %f", &v.x, &v.y, &v.z);
            array_push(&vertices, v);
        }
        else if (buf[0] == 'f')
        {
            Vec3i face_v, face_t, face_n;

            sscanf(buf, "f %d/%d/%d %d/%d/%d %d/%d/%d",
                   &face_v.x, &face_t.x, &face_n.x, &face_v.y, &face_t.y, &face_n.y,
                   &face_v.z, &face_t.z, &face_n.z);

            array_push(&faces_vertices, face_v);
            array_push(&faces_textures, face_t);
            array_push(&faces_normals, face_n);
        }
        else
        {
            fclose(fp);
            // TODO(leo): Better error handling
            LT_FAIL("Line started with %c\n", buf[0]);
        }

        memset(buf, 0, BUF_SIZE);
    }

    fclose(fp);

    ObjFile obj;
    obj.vertices = vertices;
    obj.faces_vertices = faces_vertices;
    obj.faces_textures = faces_textures;
    obj.faces_normals = faces_normals;
    return obj;
}

internal void
obj_file_free(ObjFile *f)
{
    array_free(&f->vertices);
    array_free(&f->faces_vertices);
    array_free(&f->faces_textures);
    array_free(&f->faces_normals);
}


internal void
draw_line(TGAImageRGBA *img, Vec2i p0, Vec2i p1, const Vec4i color)
{
    //LT_ASSERT(p0.x != p1.x || p0.y != p1.y);

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

    LT_ASSERT(p0.x <= p1.x);

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
    ObjFile obj = obj_file_load("resources/african_head.obj");

    const i32 IMAGE_WIDTH = 1024;
    const i32 IMAGE_HEIGHT = 768;
    TGAImageRGBA *img = lt_image_make_rgba(IMAGE_WIDTH, IMAGE_HEIGHT);

    Vec4i black(0, 0, 0, 255);
    Vec4i white(255, 255, 255, 255);

    lt_image_fill(img, black);

    printf("Starting loop\n");
    for (isize f = 0; f < obj.faces_vertices.len; f++) {
        Vec3i face = obj.faces_vertices.data[f];

        for (isize v = 0; v < 3; v++) {
            Vec3f v0 = obj.vertices.data[face.vals[v]];
            Vec3f v1 = obj.vertices.data[face.vals[(v+1)%3]];
            draw_line(img, Vec2i((v0.x+1) * IMAGE_WIDTH/2, (v0.y+1) * IMAGE_HEIGHT/2),
                      Vec2i((v1.x+1) * IMAGE_WIDTH/2, (v1.y+1) * IMAGE_HEIGHT/2), white);
        }
    }

    // output and cleanup
    lt_image_write_to_file(&img->header, "../test.tga");
    lt_image_free(img);
    obj_file_free(&obj);
}
