#version 450

layout(location = 0) in vec3 TexCoords;

layout(location = 0) out vec4 outColor;

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
    outColor = texture(cubeMapTexture, TexCoords);
}
