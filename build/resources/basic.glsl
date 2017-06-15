/* ====================================
 *
 *   Vertex Shader
 *
 * ==================================== */
#ifdef COMPILING_VERTEX

layout (location = 0) in vec3 position;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model = mat4(1.0f);

void main() {
    gl_Position = projection * view * model * vec4(position, 1.0);
}

#endif

/* ====================================
 *
 *   Fragment Shader
 *
 * ==================================== */
#ifdef COMPILING_FRAGMENT

out vec4 color;

void main() {
    color = vec4(0.3f, 1.0f, 0.3f, 1.0f);
}

#endif
