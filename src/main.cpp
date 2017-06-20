#include <stdio.h>
#include <string.h>
#include <limits.h>

#define LT_IMPLEMENTATION
#include "lt.hpp"
#include "lt_math.hpp"
#define LT_IMAGE_IMPLEMENTATION
#include "lt_image.hpp"

#define IMAGE_WIDTH  800
#define IMAGE_HEIGHT 768


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
        LT_Fail("Failed to open %s\n", filepath);
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
            LT_Fail("Buffer was overrun\n");
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

            // The vertices start at index 1 in the file.
            face_v.x--; face_v.y--; face_v.z--;

            array_push(&faces_vertices, face_v);
            array_push(&faces_textures, face_t);
            array_push(&faces_normals, face_n);
        }
        else
        {
            fclose(fp);
            // TODO(leo): Better error handling
            LT_Fail("Line started with %c\n", buf[0]);
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
draw_filled_triangle(TGAImageRGBA *img, i32 z_buffer[],
                     Vec3f v1, Vec3f v2, Vec3f v3, const Vec4i color)
{
    Vec2i screen_v1 = Vec2i((v1.x+1)*(IMAGE_WIDTH/2-1), (v1.y+1)*(IMAGE_HEIGHT/2-1));
    Vec2i screen_v2 = Vec2i((v2.x+1)*(IMAGE_WIDTH/2-1), (v2.y+1)*(IMAGE_HEIGHT/2-1));
    Vec2i screen_v3 = Vec2i((v3.x+1)*(IMAGE_WIDTH/2-1), (v3.y+1)*(IMAGE_HEIGHT/2-1));

    //
    // Find the bounding box of the triangle
    //
    i32 min_x = lt_min(screen_v1.x, screen_v2.x, screen_v3.x);
    i32 max_x = lt_max(screen_v1.x, screen_v2.x, screen_v3.x);
    i32 min_y = lt_min(screen_v1.y, screen_v2.y, screen_v3.y);
    i32 max_y = lt_max(screen_v1.y, screen_v2.y, screen_v3.y);

    //
    // Rearrange the vertices in counter clockwise order
    //
    //TODO(leo): See if this is necessary.

    //
    //     (y2 - y3)(x - x3) + (x3 - x2)(y - y3)
    // a = -------------------------------------
    //     (y2 - y3)(x1 - x3) + (x3 - x2)(y1 - y3)
    //
    //     (y3 - y1)(x - x3) + (x1 - x3)(y - y3)
    // b = -------------------------------------
    //     (y2 - y3)(x1 - x3) + (x3 - x2)(y1 - y3)
    //
    // TODO(leo): Figure out a way of not using integer values here.
    // This is probably the reason for the render artifacts.
    i32 y2_minus_y3 = screen_v2.y - screen_v3.y;
    i32 x3_minus_x2 = screen_v3.x - screen_v2.x;
    i32 x1_minus_x3 = screen_v1.x - screen_v3.x;
    i32 y1_minus_y3 = screen_v1.y - screen_v3.y;
    i32 y3_minus_y1 = screen_v3.y - screen_v1.y;

    for (isize y = min_y; y <= max_y; y++)
    {
        for (isize x = min_x; x <= max_x; x++)
        {
            Vec2i p(x, y);

            i32 x_minus_x3 = p.x - screen_v3.x;
            i32 y_minus_y3 = p.y - screen_v3.y;

            f32 a = (f32)((y2_minus_y3 * x_minus_x3) + (x3_minus_x2 * y_minus_y3)) /
                    ((y2_minus_y3 * x1_minus_x3) + (x3_minus_x2 * y1_minus_y3));
            f32 b = (f32)((y3_minus_y1 * x_minus_x3) + (x1_minus_x3 * y_minus_y3)) /
                    ((y2_minus_y3 * x1_minus_x3) + (x3_minus_x2 * y1_minus_y3));
            f32 c = 1.0f - a - b;

            // printf("a + b + b = %.2f\n", a+b+c);

            if ((0 <= a && a <= 1) && (0 <= b && b <= 1) && (0 <= c && c <= 1))
            {
                isize z_buffer_index = x + (y * IMAGE_WIDTH);
                i32 z_value = (a * v1.z) + (b * v2.z) + (c * v3.z);

                if (z_value < z_buffer[z_buffer_index])
                {
                    z_buffer[z_buffer_index] = z_value;
                    lt_image_set(img, x, y, color);
                }
            }
        }
    }
}


internal void
draw_line(TGAImageRGBA *img, Vec2i p0, Vec2i p1, const Vec4i color)
{
    i32 size_x = lt_abs(p0.x - p1.x);
    i32 size_y = lt_abs(p0.y - p1.y);

    bool steep = false;

    if (size_y > size_x)
    {
        // Transposition
        steep = true;
        lt_swap(&p0.x, &p0.y);
        lt_swap(&p1.x, &p1.y);
    }

    if (p0.x > p1.x)
    {
        // Make sure that p0's x is always smaller than p1's x.
        lt_swap(&p0.x, &p1.x);
        lt_swap(&p0.y, &p1.y);
    }

    LT_Assert(p0.x <= p1.x);

    i32 dx = p1.x - p0.x;
    i32 dy = p1.y - p0.y;
    i32 derr = lt_abs(dy)*2;
    i32 err = 0;

    i32 y = p0.y;

    for (isize x = p0.x; x < p1.x; x++)
    {
        if (steep)
            lt_image_set(img, y, x, color);
        else
            lt_image_set(img, x, y, color);

        err += derr;
        if (err > dx)
        {
            y += (p1.y > p0.y) ? 1 : -1;
            err -= 2*dx;
        }
    }
}

int
main(void)
{
    ObjFile obj = obj_file_load("resources/african_head.obj");

    TGAImageRGBA *img = lt_image_make_rgba(IMAGE_WIDTH, IMAGE_HEIGHT);

    const Vec4i black(0, 0, 0, 255);
    const Vec4i red(255, 0, 0, 255);
    const Vec4i white(255, 255, 255, 255);

    lt_image_fill(img, black);

    local_persist i32 z_buffer[IMAGE_WIDTH * IMAGE_HEIGHT] = {};

    for (isize y = 0; y < IMAGE_HEIGHT; y++)
        for (isize x = 0; x < IMAGE_WIDTH; x++)
            z_buffer[x + (y * IMAGE_WIDTH)] = INT_MAX;

    // FIXME(leo): Changing the light direction kind of breaks the lighting.
    Vec3f light_dir(0.0f, 0.0f, -1.0f);
    for (isize f = 0; f < obj.faces_vertices.len; f++)
    {
        Vec3i face = obj.faces_vertices.data[f];

        Vec3f v1_world = obj.vertices[face.vals[0]];
        Vec3f v2_world = obj.vertices[face.vals[1]];
        Vec3f v3_world = obj.vertices[face.vals[2]];
        Vec3f triangle_normal = vec_normalize(
            vec_cross(v3_world - v1_world, v2_world - v1_world)
        );

        f32 intensity = vec_dot(light_dir, triangle_normal);

        if (intensity > 0)
        {
            // TODO(leo): Pass the z-buffer.
            draw_filled_triangle(img, z_buffer, v1_world, v2_world, v3_world,
                                 Vec4i(255*intensity,255*intensity,255*intensity,255));
        }
    }

    // output and cleanup
    lt_image_write_to_file(&img->header, "../test.tga");
    lt_image_free(img);
    obj_file_free(&obj);
}
