#version 450

layout(location = 0) in vec2 fragOffset;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUbo{
    mat4 projection;
    mat4 view;
    vec3 lightPosition;
    vec4 lightAmbient;
    vec4 lightDiffuse;
    vec4 lightSpecular;
    vec3 viewPos;
} ubo;

void main() {
    float dist_squared = dot(fragOffset, fragOffset);
    if (dist_squared >= 1.0) {
        discard;
    }

    outColor = vec4(ubo.lightSpecular.rgb, 1.0);
}
