#version 450

layout(location = 0) in vec3 position;

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

void main() {
  TexCoords = position;
  gl_Position = ubo.projection * ubo.view * vec4(position, 1.0);
}
