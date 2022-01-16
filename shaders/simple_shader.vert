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
    vec4 ambientLightColor;
    vec3 lightPosition;
    vec4 lightColor;
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

void main() {
    // Transform vertex position to world space
    vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);
    gl_Position = ubo.projectionViewMatrix * positionWorld;

    // Vertex's surface normal in world space. 
    vec3 normalWorldSpace = normalize(mat3(push.normalMatrix) * normal);

    // Compute direction to the point light.
    vec3 directionToLight = ubo.lightPosition - positionWorld.xyz;
    float attenuation = 1.0 / dot(directionToLight, directionToLight);

    vec3 lightColor = ubo.lightColor.rgb * ubo.lightColor.w * attenuation;
    vec3 ambientLightColor = ubo.ambientLightColor.rgb * ubo.ambientLightColor.w;
    vec3 diffuseLight = lightColor * max(dot(normalWorldSpace, normalize(directionToLight)), 0);


    // Per-vertex normal coloring
    // fragColor = (normalWorldSpace + 1.0) * 0.5;

    // Per vertex diffuse shading
    fragColor = color * (diffuseLight + ambientLightColor);

    // modelNormal = normal;
}