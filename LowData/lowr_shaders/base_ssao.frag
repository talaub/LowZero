#version 450

layout(location = 0) out float fragColor;

#include "fullscreen_effect.glsl"

layout(set = 3, binding = 0) uniform Kernel {
    vec3 samples[64]; // precomputed hemisphere samples
};

layout(set = 3, binding = 1) uniform sampler2D texNoise;

layout(push_constant) uniform constants
{
    vec2 noiseScale;
    float radius;
    float bias;
    float power;
} params;

vec3 reconstructViewPos(vec2 uv, float depth) {
    // Convert UV to NDC space (-1..1)
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth, 1.0);

    // Transform to view space
    vec4 viewPos = g_ViewInfo.inverseProjectionMatrix * clipPos;
    viewPos /= viewPos.w;

    return viewPos.xyz;
}

void main() {
    vec2 texCoords = gl_FragCoord.xy * (g_ViewInfo.dimensions.zw*2);

     float depth = texture(VIEW_GBUFFER_DEPTH, texCoords).r;
     //vec3 fragPos = reconstructViewPos(texCoords, depth);
     vec3 fragPos = texture(VIEW_GBUFFER_VIEWPOSITION, texCoords).xyz;

    vec3 normal = normalize(texture(VIEW_GBUFFER_NORMAL, texCoords).xyz);

    // Get random vector from noise texture
    vec3 randomVec = normalize(texture(texNoise, texCoords * params.noiseScale).xyz);

    // Construct TBN matrix (Tangent, Bitangent, Normal)
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    // Accumulate occlusion
    float occlusion = 0.0;
    for (int i = 0; i < 64; ++i) {
        // Sample in tangent space, transform to view space
        vec3 samplePos = TBN * samples[i];
        samplePos = fragPos + samplePos * params.radius;

        // Project sample position into clip space
        vec4 offset = g_ViewInfo.projectionMatrix * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        vec2 sampleUV = offset.xy * 0.5 + 0.5;

        // Get depth at sample UV
        float sampleDepth = texture(VIEW_GBUFFER_DEPTH, sampleUV).r;
        vec3 sampleViewPos = texture(VIEW_GBUFFER_VIEWPOSITION, sampleUV).xyz;

        // Check if sample is occluded
        float rangeCheck = smoothstep(0.0, 1.0, params.radius / abs(fragPos.z - sampleViewPos.z));
        occlusion += (sampleViewPos.z >= samplePos.z + params.bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / 64.0); // Normalize
    fragColor = pow(occlusion, params.power); // Optional contrast adjustment
}