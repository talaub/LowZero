#version 450

layout(location = 0) out vec4 fragColor;

#include "fullscreen_effect.glsl"

layout(set = 3, binding = 0) uniform SsgiKernel {
    vec4 samples[16];
};

layout(set = 3, binding = 1) uniform sampler2D texNoise;

layout(push_constant) uniform constants {
    vec2 noiseScale;
    float radius;
    float strength;
} params;

void main() {
    vec2 texCoords = gl_FragCoord.xy * (g_ViewInfo.dimensions.zw * 2.0);

    float depth = texture(VIEW_GBUFFER_DEPTH, texCoords).r;
    if (depth >= 1.0) {
        fragColor = vec4(0.0);
        return;
    }

    vec3 fragPos = texture(VIEW_GBUFFER_VIEWPOSITION, texCoords).xyz;

    vec3 normalWorld = normalize(texture(VIEW_GBUFFER_NORMAL, texCoords).rgb * 2.0 - 1.0);
    vec3 normal = normalize(mat3(g_ViewInfo.viewMatrix) * normalWorld);

    vec3 randomVec = normalize(texture(texNoise, texCoords * params.noiseScale).xyz);
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    vec3 indirectLight = vec3(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < 16; ++i) {
        vec3 sampleDir = TBN * samples[i].xyz;
        vec3 samplePos = fragPos + sampleDir * params.radius;

        vec4 proj = g_ViewInfo.projectionMatrix * vec4(samplePos, 1.0);
        proj.xyz /= proj.w;
        vec2 sampleUV = proj.xy * 0.5 + 0.5;

        if (any(lessThan(sampleUV, vec2(0.0))) || any(greaterThan(sampleUV, vec2(1.0)))) {
            continue;
        }

        vec3 sampleViewPos = texture(VIEW_GBUFFER_VIEWPOSITION, sampleUV).xyz;
        float rangeCheck = smoothstep(0.0, 1.0, params.radius / max(abs(fragPos.z - sampleViewPos.z), 0.0001));

        if (abs(sampleViewPos.z - samplePos.z) < params.radius * 1.5) {
            vec3 litColor = texture(TEX2D(g_ViewInfo.textureIndices.z), sampleUV).rgb;
            float cosWeight = max(dot(normalize(sampleDir), normal), 0.0);
            indirectLight += litColor * cosWeight * rangeCheck;
            totalWeight += cosWeight;
        }
    }

    if (totalWeight > 0.0) {
        indirectLight /= totalWeight;
    }

    fragColor = vec4(indirectLight * params.strength, 1.0);
}
