#version 460

layout(set = 0, binding = 0) uniform sampler2D input_image;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform TonemapParams {
    float temp;
    float tint;
    float exposure;
    float contrast;
    float saturation;
    float highlights;
    float midtones;
    float shadows;
    float whites;
    float blacks;
    float vibrance;
    int operator;
} params;

// tonemapping
vec3 reinhard(vec3 x) {
    return x / (1.0 + x);
}

vec3 uncharted2(vec3 x) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;

    vec3 num = ((x*(A*x+C*B))+D*E);
    vec3 den = ((x*(A*x+B))+D*F);
    return num/den - E/F;
}

vec3 aces(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// color grading helpers
vec3 white_balance(vec3 color, float temp, float tint) {
    float t = temp * 0.1;
    float ti = tint * 0.1;
    mat3 wbMatrix = mat3(
    1.0 + t + ti, 0.0, 0.0,
    0.0, 1.0 - ti, 0.0,
    0.0, 0.0, 1.0 - t - ti
    );
    return clamp(color * wbMatrix, 0.0, 1.0);
}

vec3 contrast_adjust(vec3 color, float contrast, float blacks, float whites) {
    float c = contrast;
    color = (color - 0.5) * c + 0.5;
    color = max(color + blacks, 0.0);
    color = min(color * (1.0 / (1.0 - whites + 0.0001)), 1.0);
    return color;
}

vec3 shadows_highlights(vec3 color, float shadows, float midtones, float highlights) {
    color = mix(color, pow(color, vec3(0.8)), -shadows * 0.5);

    color = mix(color, color * color * 2.0, midtones * 0.3);

    vec3 hl = color * color;
    color = mix(color, hl / (hl + 1.0), highlights * 0.6);

    return color;
}

vec3 saturation_adjust(vec3 color, float saturation) {
    float luminance = dot(color, vec3(0.299, 0.587, 0.114));
    return mix(vec3(luminance), color, saturation);
}

vec3 vibrance_adjust(vec3 color, float vibrance) {
    float maxColor = max(color.r, max(color.g, color.b));
    float minColor = min(color.r, min(color.g, color.b));
    float sat = maxColor - minColor;

    return mix(color, saturation_adjust(color, vibrance), sat * 0.7);
}

void main() {
    vec2 uv = gl_FragCoord.xy / textureSize(input_image, 0);
    vec3 color = texture(input_image, uv).rgb;

    // exposure
    color *= params.exposure;

    // tonemapping
    if (params.operator == 0) {
        color = reinhard(color);
    } else if (params.operator == 1) {
        color = uncharted2(color);
        color = color / uncharted2(vec3(11.2));
    } else {
        color = aces(color);
    }

    // color grading
    color = white_balance(color, params.temp, params.tint);
    color = contrast_adjust(color, params.contrast, params.blacks, params.whites);
    color = shadows_highlights(color, params.shadows, params.midtones, params.highlights);
    color = saturation_adjust(color, params.saturation);
    color = vibrance_adjust(color, params.vibrance);

    out_color = vec4(color, 1.0);
}