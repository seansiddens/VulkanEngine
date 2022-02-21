#version 450

// Lots of this code is thanks to this amazing resource: https://learnopengl.com/PBR/Lighting

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec2 fragTexCoord;

layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUbo{
    mat4 projection;
    mat4 view;
    vec3 lightPosition;
    vec3 lightColor;
    vec3 viewPos;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D metallicMap;
layout(set = 1, binding = 2) uniform sampler2D roughnessMap;
layout(set = 1, binding = 3) uniform sampler2D aoMap;
layout(set = 1, binding = 4) uniform Material {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
} mat;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

const int numPointLights = 1;
const float PI = 3.14159265359;

// Approximate the ratio between how much the surface reflects and how much it refracts.
// The F0 parameter is the surface reflection at zero incidence or how much the surface reflects
// if looking directly at the surface.
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Approximate the relative surface area of microfacets exactly aligned to H.
// Using Trowbridge-Reitz GGX.
float distributionGGX(vec3 N, vec3 H, float roughness) {
    // Based on observations by Disney and adopted by Epic Games, the lighting looks more correct
    // squaring the roughness in both the geometry and normal distribution function.
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

// Approximate the relative surface area where micro-facet details occlude light.
float geometrySchlickGGX(float NdotV, float roughness) {
    // NOTE: Roughness needs to be remapped depending on whether we are using direct lighting or IBL.
    float r = (roughness + 1.0);
    float kDirect = (r * r) / 8.0;
    float k = kDirect;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

// Smith's method: Take into account view direction (obstruction) and light direction (shadowing).
float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec4 srgb_to_linear(vec4 srgb) {
    vec3 color_srgb = srgb.rgb;
    vec3 selector = clamp(ceil(color_srgb - 0.04045), 0.0, 1.0); // 0 if under value, 1 if over
    vec3 under = color_srgb / 12.92;
    vec3 over = pow((color_srgb + 0.055) / 1.055, vec3(2.4));
    vec3 result = mix(under, over, selector);
    return vec4(result, srgb.a);
}

void main() {
    vec3 albedo = texture(albedoMap, fragTexCoord).rgb * mat.albedo;
    float metallic = srgb_to_linear(texture(metallicMap, fragTexCoord)).r * mat.metallic;
    float roughness = srgb_to_linear(texture(roughnessMap, fragTexCoord)).r * mat.roughness;
    float ao = srgb_to_linear(texture(aoMap, fragTexCoord)).r * mat.ao;

    vec3 N = normalize(fragNormalWorld); // Surface normal
    vec3 V = normalize(ubo.viewPos - fragPosWorld); // View direction

    // Total reflected radiance back to the viewer.
    vec3 Lo = vec3(0.0);

    // Sum the contributions of each point light in the scene to the outgoing radiance.
    for (int i = 0; i < numPointLights; i++) {
        // Light direction.
        vec3 L = normalize(ubo.lightPosition - fragPosWorld);
        // Vector halfway between the view and the light vector.
        vec3 H = normalize(V + L);

        // Attenuate light by the inverse square law.
        float distance = length(ubo.lightPosition - fragPosWorld);
        float attenuation = 1.0 / (distance * distance);

        vec3 radiance = ubo.lightColor * attenuation;

        // Compute the BRDF term using the Cook-Torrance BRDF
        // Fresnel (F)
        // Dielectric materials are assumed to have a constant F0 value of 0.04.
        vec3 F0 = vec3(0.04);
        // Metal will tint the base reflectivity by the surface's color.
        F0 = mix(F0, albedo, metallic);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        // Normal distribution function (D)
        float NDF = distributionGGX(N, H, roughness);
        // Geometry (G)
        float G = geometrySmith(N, V, L, roughness);

        // Cook-Torrance BRDF
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // Prevent divide by zero.
        vec3 specular = numerator / denominator;

        // Specular ratio.
        vec3 kS = F;
        // Diffuse ratio.
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic; // Metallic surfaces don't refract light, so we nullify the diffuse term.

        // Calculate the light's contribution to the reflectance equation.
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // Improvised ambient term.
    vec3 ambient = vec3(0.005) * albedo * ao;

    vec3 color = ambient + Lo;
    // Tone mapping and gamma correction.
    color = color / (color + vec3(1.0));

//    outColor = vec4(texture(texSampler_, fragTexCoord).rgb, 1.0);
    outColor = vec4(color, 1.0);
}
