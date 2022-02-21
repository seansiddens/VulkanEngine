#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec2 fragTexCoord;

layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUbo{
    mat4 projection;
    mat4 view;
    vec3 lightPosition;
    vec4 lightAmbient;
    vec4 lightDiffuse;
    vec4 lightSpecular;
    vec3 viewPos;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D texSampler_;
layout(set = 1, binding = 1) uniform Material {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float shininess;
} mat;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

void main() {

    // Surface normal needs to be normalized again because the linear interpolation of normal vectors isn't
    // necessarily normal itself.
    vec3 norm = normalize(fragNormalWorld);

    // Direction to light source from frag.
    vec3 lightDistance = ubo.lightPosition - fragPosWorld;
    float attenuation = 1.0 / dot(lightDistance, lightDistance);
    vec3 lightDir = normalize(lightDistance);

    // Ambient
    vec3 ambient = ubo.lightAmbient.rgb * mat.ambient.rgb;

    // Diffuse
    float diff = max(dot(norm, lightDir),  0.0);
    vec3 diffuse = ubo.lightDiffuse.rgb * (diff * mat.diffuse.rgb);

    // Specular
    vec3 viewDir = normalize(ubo.viewPos - fragPosWorld); // Direction to the camera from frag.
    vec3 reflectDir = reflect(-lightDir, norm); // Light ray reflected about surface normal.
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), mat.shininess); // Amount of light reflected back to viewer.
    vec3 specular = ubo.lightSpecular.rgb * (spec * mat.specular.rgb);

    // Attenuate
//    ambient *= attenuation;
//    diffuse *= attenuation;
//    specular *= attenuation;

    vec3 result = ambient + diffuse;
    outColor = vec4(result, 1.0);
}