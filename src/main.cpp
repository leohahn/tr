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
    Array<Vec3f> tex_coords;
    Array<Vec3i> faces_vertices;
    Array<Vec3i> faces_textures;
    Array<Vec3i> faces_normals;
};

struct Vertex3
{
    Vec3f vertice;
    Vec3f tex_coord;
    Vertex3(Vec3f vertice, Vec3f tex_coord): vertice(vertice), tex_coord(tex_coord) {}
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
    Array<Vec3f> tex_coords = array_make<Vec3f>();
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

        if (strncmp(buf, "vt", 2) == 0)
        {
            Vec3f v;
            sscanf(buf, "vt %f %f %f", &v.x, &v.y, &v.z);
            array_push(&tex_coords, v);
            continue;
        }

        if (strncmp(buf, "vn", 2) == 0)
        {
            continue;
        }

        if (buf[0] == 'v')
        {
            Vec3f v;
            sscanf(buf, "v %f %f %f", &v.x, &v.y, &v.z);
            array_push(&vertices, v);
            continue;
        }

        if (buf[0] == 'f')
        {
            Vec3i face_v, face_t, face_n;

            sscanf(buf, "f %d/%d/%d %d/%d/%d %d/%d/%d",
                   &face_v.x, &face_t.x, &face_n.x, &face_v.y, &face_t.y, &face_n.y,
                   &face_v.z, &face_t.z, &face_n.z);

            // The indexes start at index 1 in the file.
            face_v.x--; face_v.y--; face_v.z--;
            face_t.x--; face_t.y--; face_t.z--;
            face_n.x--; face_n.y--; face_n.z--;

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
    obj.tex_coords = tex_coords;
    obj.faces_vertices = faces_vertices;
    obj.faces_textures = faces_textures;
    obj.faces_normals = faces_normals;

    LT_Assert(faces_vertices.len == faces_textures.len);
    return obj;
}

internal void
obj_file_free(ObjFile *f)
{
    array_free(&f->vertices);
    array_free(&f->tex_coords);
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
draw_filled_triangle(TGAImageRGBA *img, TGAImageRGB *tex, i32 z_buffer[],
                     Vertex3 *v1, Vertex3 *v2, Vertex3 *v3, f32 intensity)
{
    //
    // Find the bounding box of the triangle
    //
    i32 min_x = lt_min(v1->vertice.x, v2->vertice.x, v3->vertice.x);
    i32 max_x = lt_max(v1->vertice.x, v2->vertice.x, v3->vertice.x);
    i32 min_y = lt_min(v1->vertice.y, v2->vertice.y, v3->vertice.y);
    i32 max_y = lt_max(v1->vertice.y, v2->vertice.y, v3->vertice.y);

    //
    // Rearrange the vertices in counter clockwise order
    //
    //TODO(leo): See if this is necessary.

    Vec3f p = {};
    for (p.y = min_y; p.y <= max_y; p.y++)
    {
        for (p.x = min_x; p.x <= max_x; p.x++)
        {
            Vec3f bc_screen = barycentric(v1->vertice, v2->vertice, v3->vertice, p);

            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;

            isize z_buffer_index = p.x + (p.y * IMAGE_WIDTH);
            p.z = (bc_screen.x * v1->vertice.z) + (bc_screen.y * v2->vertice.z) + (bc_screen.z * v3->vertice.z);

            if (p.z < z_buffer[z_buffer_index])
            {
                z_buffer[z_buffer_index] = p.z;
                Vec3i color1 = lt_image_get(tex,
                                            v1->tex_coord.x*(lt_image_width(tex)-1),
                                            v1->tex_coord.y*(lt_image_height(tex)-1));
                Vec3i color2 = lt_image_get(tex,
                                            v2->tex_coord.x*(lt_image_width(tex)-1),
                                            v2->tex_coord.y*(lt_image_height(tex)-1));
                Vec3i color3 = lt_image_get(tex,
                                            v3->tex_coord.x*(lt_image_width(tex)-1),
                                            v3->tex_coord.y*(lt_image_height(tex)-1));
                Vec3i color = color1*bc_screen.x + color2*bc_screen.y + color3*bc_screen.z;
                // Vec3i color(255,255,255);
                lt_image_set(img, p.x, p.y, Vec4i(color*intensity, 255));
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
        Vec3i face_v = obj.faces_vertices[f];
        Vec3i face_t = obj.faces_textures[f];

        Vertex3 v1(obj.vertices[face_v.val[0]], obj.tex_coords[face_t.val[0]]);
        Vertex3 v2(obj.vertices[face_v.val[1]], obj.tex_coords[face_t.val[1]]);
        Vertex3 v3(obj.vertices[face_v.val[2]], obj.tex_coords[face_t.val[2]]);
        Vec3f triangle_normal = vec_normalize(
            vec_cross(v3.vertice - v1.vertice, v2.vertice - v1.vertice)
        );

        f32 intensity = vec_dot(light_dir, triangle_normal);

        if (intensity > 0)
        {
            v1.vertice = normalized2screen(v1.vertice, IMAGE_WIDTH, IMAGE_HEIGHT);
            v2.vertice = normalized2screen(v2.vertice, IMAGE_WIDTH, IMAGE_HEIGHT);
            v3.vertice = normalized2screen(v3.vertice, IMAGE_WIDTH, IMAGE_HEIGHT);
            draw_filled_triangle(img, texture, z_buffer, &v1, &v2, &v3, intensity);
        }
    }

    // output and cleanup
    lt_image_write_to_file(img, "../test.tga");
    lt_image_write_to_file(texture, "../out-texture.tga");
    lt_image_free(img);
    lt_image_free(texture);
    obj_file_free(&obj);
}
