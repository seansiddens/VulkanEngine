#version 450

layout(location = 0) in vec2 fragOffset;

layout(location = 0) out vec4 outColor;

// Set and binding numbers must match the descriptor set layout.
layout(set = 0, binding = 0) uniform GlobalUbo{
    mat4 projection;
    mat4 view;
    vec3 lightPosition;
    vec3 lightColor;
    vec3 viewPos;
} ubo;

void main() {
    float dist_squared = dot(fragOffset, fragOffset);
    if (dist_squared >= 1.0) {
        discard;
    }

    outColor = vec4(ubo.lightColor, 1.0);
}
