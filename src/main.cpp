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
struct ObjFile
{
    Array<Vec3f> vertices;
    Array<Vec3i> faces_vertices;
    Array<Vec3i> faces_textures;
    Array<Vec3i> faces_normals;
};

internal ObjFile
obj_file_load(const char *filepath)
{
    FILE *fp = fopen(filepath, "rb");
    if (!fp)
    {
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
            // TODO: Does normals and textures also start at index 1?

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

Vec3f
barycentric(const Vec3f A, const Vec3f B, const Vec3f C, const Vec3f P)
{
    Vec3f s[2];
    for (int i=2; i--; ) {
        s[i].val[0] = C.val[i]-A.val[i];
        s[i].val[1] = B.val[i]-A.val[i];
        s[i].val[2] = A.val[i]-P.val[i];
    }
    Vec3f u = vec_cross(s[0], s[1]);
    // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
    if (std::abs(u.val[2])>1e-2)
        return Vec3f(1.f-(u.x+u.y)/u.z, u.y/u.z, u.x/u.z);
    // in this case generate negative coordinates, it will be thrown away by the rasterizator
    return Vec3f(-1,1,1);
}

internal void
draw_filled_triangle(TGAImageRGBA *img, i32 z_buffer[],
                     Vec3f v1, Vec3f v2, Vec3f v3, const Vec4i color)
{
    //
    // Find the bounding box of the triangle
    //
    i32 min_x = lt_min(v1.x, v2.x, v3.x);
    i32 max_x = lt_max(v1.x, v2.x, v3.x);
    i32 min_y = lt_min(v1.y, v2.y, v3.y);
    i32 max_y = lt_max(v1.y, v2.y, v3.y);

    //
    // Rearrange the vertices in counter clockwise order
    //
    //TODO(leo): See if this is necessary.

    Vec3f p = {};
    for (p.y = min_y; p.y <= max_y; p.y++)
    {
        for (p.x = min_x; p.x <= max_x; p.x++)
        {
            Vec3f bc_screen = barycentric(v1, v2, v3, p);

            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;

            // if ((0.0f <= a && a <= 1.0f) && (0.0f <= b && b <= 1.0f) && (0.0f <= c && c <= 1.0f))
            // {
                isize z_buffer_index = p.x + (p.y * IMAGE_WIDTH);
                p.z = (bc_screen.x * v1.z) + (bc_screen.y * v2.z) + (bc_screen.z * v3.z);

                if (p.z < z_buffer[z_buffer_index])
                {
                    z_buffer[z_buffer_index] = p.z;
                    lt_image_set(img, p.x, p.y, color);
                }
            // }
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

inline Vec3f
normalized2screen(const Vec3f n, const i32 width, const i32 height)
{
    return Vec3f((i32)((n.x+1.)*(width/2.-1.)+.5), (i32)((n.y+1.)*(height/2.-1.)+.5), n.z);
}

int
main(void)
{
    const Vec4i black(0, 0, 0, 255);
    const Vec4i blue(0, 0, 255, 255);
    const Vec4i red(255, 0, 0, 255);
    const Vec4i white(255, 255, 255, 255);

    ObjFile obj = obj_file_load("resources/african_head.obj");
    TGAImageRGB *texture = lt_image_load_rgb("resources/african_head_diffuse.tga");
    TGAImageRGBA *img = lt_image_make_rgba(IMAGE_WIDTH, IMAGE_HEIGHT);

    lt_image_fill(img, blue);

    local_persist i32 z_buffer[IMAGE_WIDTH * IMAGE_HEIGHT] = {};

    // Initialize the z buffer.
    for (isize y = 0; y < IMAGE_HEIGHT; y++)
        for (isize x = 0; x < IMAGE_WIDTH; x++)
            z_buffer[x + (y * IMAGE_WIDTH)] = INT_MAX;

    // FIXME(leo): Changing the light direction kind of breaks the lighting.
    Vec3f light_dir(0.0f, 0.0f, -1.0f);
    for (isize f = 0; f < obj.faces_vertices.len; f++)
    {
        Vec3i face = obj.faces_vertices.data[f];
        Vec3f v1_world = obj.vertices[face.val[0]];
        Vec3f v2_world = obj.vertices[face.val[1]];
        Vec3f v3_world = obj.vertices[face.val[2]];
        Vec3f triangle_normal = vec_normalize(
            vec_cross(v3_world - v1_world, v2_world - v1_world)
        );

        f32 intensity = vec_dot(light_dir, triangle_normal);

        if (intensity > 0)
        {
            // TODO(leo): Pass the z-buffer.
            const i32 base_color = 255;
            draw_filled_triangle(img, z_buffer,
                                 normalized2screen(v1_world, IMAGE_WIDTH, IMAGE_HEIGHT),
                                 normalized2screen(v2_world, IMAGE_WIDTH, IMAGE_HEIGHT),
                                 normalized2screen(v3_world, IMAGE_WIDTH, IMAGE_HEIGHT),
                                 Vec4i(base_color*intensity,base_color*intensity,base_color*intensity,255));
        }
    }

    // output and cleanup
    lt_image_write_to_file(img, "../test.tga");
    // lt_image_write_to_file(texture, "../out-texture.tga");
    lt_image_free(img);
    lt_image_free(texture);
    obj_file_free(&obj);
}
