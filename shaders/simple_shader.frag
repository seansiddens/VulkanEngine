#version 450

layout(location = 0) in vec3 fragColor;
// layout(location = 1) in vec3 modelNormal;

layout (location = 0) out vec4 outColor;

// Set and binding numbers must match the descriptor set layout.
// layout(set = 0, binding = 0) uniform GlobalUbo{
//     mat4 projectionViewMatrix;
//     vec3 directionToLight;
// } ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

const float AMBIENT = 0.02;

void main() {
    // Vertex's surface normal in world space. 
    // vec3 normalWorldSpace = normalize(mat3(push.normalMatrix) * modelNormal);
    // float lightIntensity = AMBIENT + max(dot(normalWorldSpace, ubo.directionToLight), 0);

    // Per-fragment diffuse
    // outColor = vec4(vec3(1.0) * lightIntensity, 1.0);

    outColor = vec4(fragColor, 1.0);
}