#version 450

const vec2 OFFSETS[6] = vec2[](
  vec2(-1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, -1.0),
  vec2(1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, 1.0)
);

layout(location = 0) out vec2 fragOffset;

layout(set = 0, binding = 0) uniform GlobalUbo{
    mat4 projection;
    mat4 view;
    vec4 ambientLightColor;
    vec3 lightPosition;
    vec4 lightColor;
} ubo;

const float LIGHT_RADIUS = 0.1;

void main() {
    fragOffset = OFFSETS[gl_VertexIndex];

    // Get the light's position in camera space.
    vec4 lightPosCam = ubo.view * vec4(ubo.lightPosition, 1.0);

    // Calculate the vertex's position in camera space.
    // Billboard will always be axis-aligned in camera space.
    vec4 positionCam = lightPosCam + LIGHT_RADIUS * vec4(fragOffset, 0.0, 0.0);

    // We are already in camera space, so we just need to transform to clip space.
    gl_Position = ubo.projection * positionCam;

    // If using point list.
    // gl_Position = ubo.projection * lightPosCam;
    // gl_PointSize = 5.0;
}
