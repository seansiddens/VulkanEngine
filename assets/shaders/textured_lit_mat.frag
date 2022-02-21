#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec2 fragTexCoord;

layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUbo{
    mat4 projection;
    mat4 view;
    vec4 ambientLightColor;
    vec3 lightPosition;
    vec4 lightColor;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D texSampler_;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

const float AMBIENT = 0.02;

void main() {
    // Fragment world positions are interpolated between vertex world positions.
    vec3 directionToLight = ubo.lightPosition - fragPosWorld;

    // Light intensity follows the inverse-square law.
    float attenuation = 1.0 / dot(directionToLight, directionToLight);

    // color * intensity * attenuation (effected by distance).
    vec3 lightColor = ubo.lightColor.rgb * ubo.lightColor.w * attenuation;

    // Scaled by intensity (w).
    vec3 ambientLightColor = ubo.ambientLightColor.rgb * ubo.ambientLightColor.w;

    // fragNormalWorld must be normalized again because the linear interpolation of normal vectors
    // isn't necessarily normal itself.
    vec3 diffuseLight = lightColor * max(dot(normalize(fragNormalWorld), normalize(directionToLight)), 0);

    // Per-frag diffuse shading + texture sampling.
    outColor = vec4(texture(texSampler_, fragTexCoord).rgb * (diffuseLight + ambientLightColor), 1.0);
}