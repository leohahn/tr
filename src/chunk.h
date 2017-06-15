#ifndef CHUNK_H
#define CHUNK_H

#include "lt.h"

#define WORLD_SIZE     8
#define CHUNK_SIZE    16
#define BLOCK_SIZE     1

typedef struct Chunk {
    isize    num_blocks;
    isize    num_vertices;

    GLuint   vao;
    GLuint   vbo;
} Chunk;

#endif // CHUNK_H
