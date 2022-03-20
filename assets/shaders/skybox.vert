#version 450

const vec3 vertices[36] = vec3[](
    vec3(-1.0,  1.0, -1.0),
    vec3(-1.0, -1.0, -1.0),
    vec3(1.0, -1.0, -1.0),
    vec3(1.0, -1.0, -1.0),
    vec3(1.0,  1.0, -1.0),
    vec3(-1.0,  1.0, -1.0),

    vec3(-1.0, -1.0,  1.0),
    vec3(-1.0, -1.0, -1.0),
    vec3(-1.0,  1.0, -1.0),
    vec3(-1.0,  1.0, -1.0),
    vec3(-1.0,  1.0,  1.0),
    vec3(-1.0, -1.0,  1.0),

    vec3(1.0, -1.0, -1.0),
    vec3(1.0, -1.0,  1.0),
    vec3(1.0,  1.0,  1.0),
    vec3(1.0,  1.0,  1.0),
    vec3(1.0,  1.0, -1.0),
    vec3(1.0, -1.0, -1.0),

    vec3(-1.0, -1.0,  1.0),
    vec3(-1.0,  1.0,  1.0),
    vec3(1.0,  1.0,  1.0),
    vec3(1.0,  1.0,  1.0),
    vec3(1.0, -1.0,  1.0),
    vec3(-1.0, -1.0,  1.0),

    vec3(-1.0,  1.0, -1.0),
    vec3(1.0,  1.0, -1.0),
    vec3(1.0,  1.0,  1.0),
    vec3(1.0,  1.0,  1.0),
    vec3(-1.0,  1.0,  1.0),
    vec3(-1.0,  1.0, -1.0),

    vec3(-1.0, -1.0, -1.0),
    vec3(-1.0, -1.0,  1.0),
    vec3(1.0, -1.0, -1.0),
    vec3(1.0, -1.0, -1.0),
    vec3(-1.0, -1.0,  1.0),
    vec3(1.0, -1.0,  1.0)
);

// Cubemap is sampled using a direction vector.
layout(location = 0) out vec3 TexCoords;

// Set and binding numbers must match the descriptor set layout.
layout(set = 0, binding = 0) uniform GlobalUbo{
    mat4 projection;
    mat4 view;
    vec3 lightPosition;
    vec3 lightColor;
    vec3 viewPos;
} ubo;

layout (set = 1, binding = 0) uniform samplerCube cubeMapTexture;

void main() {
  vec3 position = vertices[gl_VertexIndex];
  TexCoords = position;

  mat4 view_no_translate = mat4(mat3(ubo.view));
  gl_Position = ubo.projection * view_no_translate * vec4(position, 1.0);
}
