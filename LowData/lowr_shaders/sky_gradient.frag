#version 450

layout(location = 0) out vec4 outColor;

#include "fullscreen_effect.glsl"

struct DirectionalLight {
    vec4 color;
    vec3 direction;
};

layout(std140, set = 2, binding = 6) uniform DirectionalLightBuffer {
    DirectionalLight g_DirectionalLight;
};

void main() {
    vec2 uv = gl_FragCoord.xy * g_ViewInfo.dimensions.zw;
    float depth = texture(VIEW_GBUFFER_DEPTH, uv).r;
    if (depth < 1.0) discard;

    vec2 ndc = uv * 2.0 - 1.0;
    vec4 viewDir = g_ViewInfo.inverseProjectionMatrix * vec4(ndc, 1.0, 1.0);
    viewDir /= viewDir.w;
    vec3 worldDir = normalize((g_ViewInfo.inverseViewMatrix * vec4(viewDir.xyz, 0.0)).xyz);
    float y = worldDir.y;

    // World-space sun direction and height
    vec3  sunDir    = normalize((g_ViewInfo.inverseViewMatrix * vec4(-g_DirectionalLight.direction, 0.0)).xyz);
    float sunHeight = sunDir.y; // -1 = directly below, 0 = horizon, 1 = directly above

    // Two blend factors drive all three palettes:
    //   sunsetBlend: 0=night, 1=sunset-or-day
    //   dayBlend:    0=night/sunset, 1=day
    float sunsetBlend = smoothstep(-0.20,  0.06, sunHeight);
    float dayBlend    = smoothstep( 0.06,  0.35, sunHeight);

    // --- Night palette ---
    vec3 n_zenith  = vec3(0.01, 0.02, 0.08);
    vec3 n_midSky  = vec3(0.02, 0.04, 0.14);
    vec3 n_horizon = vec3(0.03, 0.04, 0.12);
    vec3 n_ground  = vec3(0.01, 0.01, 0.02);

    // --- Sunrise / sunset palette ---
    vec3 s_zenith  = vec3(0.05, 0.10, 0.42);
    vec3 s_midSky  = vec3(0.14, 0.18, 0.58);
    vec3 s_horizon = vec3(0.96, 0.38, 0.07);
    vec3 s_ground  = vec3(0.06, 0.04, 0.03);

    // --- Day palette ---
    vec3 d_zenith  = vec3(0.04, 0.16, 0.58);
    vec3 d_midSky  = vec3(0.30, 0.56, 0.88);
    vec3 d_horizon = vec3(0.76, 0.88, 0.97);
    vec3 d_ground  = vec3(0.08, 0.07, 0.06);

    // Blend: night -> sunset -> day
    vec3 zenithColor  = mix(mix(n_zenith,  s_zenith,  sunsetBlend), d_zenith,  dayBlend);
    vec3 midSkyColor  = mix(mix(n_midSky,  s_midSky,  sunsetBlend), d_midSky,  dayBlend);
    vec3 horizonColor = mix(mix(n_horizon, s_horizon, sunsetBlend), d_horizon, dayBlend);
    vec3 groundColor  = mix(mix(n_ground,  s_ground,  sunsetBlend), d_ground,  dayBlend);

    // --- Base gradient ---
    vec3 skyColor;
    if (y >= 0.0) {
        float blueT       = pow(y, 0.45);
        vec3  blueLayer   = mix(midSkyColor, zenithColor, blueT);
        float horizonFade = 1.0 - smoothstep(0.0, 0.14, y);
        skyColor = mix(blueLayer, horizonColor, horizonFade);
    } else {
        skyColor = mix(horizonColor, groundColor, pow(-y, 0.55));
    }

    // --- Horizon scatter band ---
    // Peaks at sunrise/sunset, subtle during day, gone at night
    float sunsetPeak    = clamp(1.0 - abs(sunHeight) / 0.22, 0.0, 1.0);
    float scatterStr    = mix(dayBlend * 0.10, 0.55, sunsetPeak);
    float bandWidth     = mix(10.0, 5.0, sunsetPeak); // wider band during sunset
    float warmBand      = exp(-abs(y) * bandWidth);
    vec3  scatterColor  = mix(vec3(0.98, 0.88, 0.72), vec3(1.0, 0.42, 0.07), sunsetPeak);
    skyColor = mix(skyColor, scatterColor, warmBand * scatterStr);

    // --- Sun directional glow ---
    float sunDot     = max(dot(worldDir, sunDir), 0.0);
    float glowPower  = mix(5.0, 3.5, sunsetPeak); // broader glow at sunset
    float glowStr    = mix(0.45, 0.75, sunsetPeak);
    float horizonGlow = pow(sunDot, glowPower) * exp(-abs(y) * 4.0);
    horizonGlow = clamp(horizonGlow, 0.0, 1.0);
    vec3 glowColor = g_DirectionalLight.color.rgb * 0.85 + mix(vec3(0.10, 0.04, 0.0), vec3(0.22, 0.06, 0.0), sunsetPeak);
    skyColor = mix(skyColor, glowColor, horizonGlow * glowStr);

    // --- Sun disk (fades out when sun is below horizon) ---
    float diskVisibility = smoothstep(-0.05, 0.02, sunHeight);
    float sunDisk = smoothstep(0.9985, 0.9998, sunDot);
    skyColor += g_DirectionalLight.color.rgb * sunDisk * 2.5 * diskVisibility;

    outColor = vec4(skyColor, 1.0);
}
