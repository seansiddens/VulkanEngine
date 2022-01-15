#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;
// layout(location = 1) out vec3 modelNormal;

// Set and binding numbers must match the descriptor set layout.
layout(set = 0, binding = 0) uniform GlobalUbo{
    mat4 projectionViewMatrix;
    vec3 directionToLight;
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

const float AMBIENT = 0.02;

void main() {
    gl_Position = ubo.projectionViewMatrix * push.modelMatrix * vec4(position, 1.0);

    // Vertex's surface normal in world space. 
    vec3 normalWorldSpace = normalize(mat3(push.normalMatrix) * normal);

    float lightIntensity = AMBIENT + max(dot(normalWorldSpace, ubo.directionToLight), 0);

    // Per-vertex normal coloring
    // fragColor = (normalWorldSpace + 1.0) * 0.5;

    // Per vertex diffuse shading
    fragColor = lightIntensity * color;

    // modelNormal = normal;
}