#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

// Per-vertex values which will be interpolated on frag shader.
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec2 fragTexCoord;

// Set and binding numbers must match the descriptor set layout.
layout(set = 0, binding = 0) uniform GlobalUbo{
    mat4 projection;
    mat4 view;
    vec3 lightPosition;
    vec4 lightAmbient;
    vec4 lightDiffuse;
    vec4 lightSpecular;
    vec3 viewPos;
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

void main() {
    // Transform vertex position to world space
    vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);

    // Apply view and then projection.
    gl_Position = ubo.projection * ubo.view * positionWorld;

    // After scaling, the model's normals will not be properly aligned in world space anymore,
    // so we must apply this transformation to transform it back to world space.
    fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);

    // The fragments position in world space will be interpolated in frag shader.
    fragPosWorld = positionWorld.xyz;
    
    // Texture coordinates.
    fragTexCoord = uv;

    // Vertex color.
    fragColor = color;
}