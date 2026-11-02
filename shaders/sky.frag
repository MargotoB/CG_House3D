#version 330 core
// Процедурно небе: градиент според височината на погледа, диск на слънцето/луната
// и звезди през нощта.

in vec2 vNdc;
out vec4 FragColor;

uniform mat4  uInvViewProj;
uniform vec3  uZenithColor;
uniform vec3  uHorizonColor;
uniform vec3  uGroundColor;
uniform vec3  uSunDirection;
uniform vec3  uSunDiskColor;
uniform float uNight;          // 0 – ден, 1 – нощ

float hash(vec3 p) {
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

void main() {
    // Посока на лъча през пиксела в световни координати.
    vec4 nearP = uInvViewProj * vec4(vNdc, -1.0, 1.0);
    vec4 farP  = uInvViewProj * vec4(vNdc,  1.0, 1.0);
    vec3 dir = normalize(farP.xyz / farP.w - nearP.xyz / nearP.w);

    float h = dir.y;
    vec3 sky = mix(uHorizonColor, uZenithColor, pow(clamp(h, 0.0, 1.0), 0.6));
    if (h < 0.0) sky = mix(uHorizonColor, uGroundColor, clamp(-h * 4.0, 0.0, 1.0));

    // Слънце / луна
    float s = dot(dir, normalize(uSunDirection));
    sky += uSunDiskColor * smoothstep(0.9990, 0.9996, s);
    sky += uSunDiskColor * 0.25 * pow(max(s, 0.0), 64.0) * (1.0 - uNight * 0.6);

    // Звезди
    if (uNight > 0.01 && h > 0.0) {
        vec3 cell = floor(dir * 220.0);
        float star = step(0.9975, hash(cell));
        sky += vec3(star) * uNight * clamp(h * 3.0, 0.0, 1.0);
    }

    FragColor = vec4(sky, 1.0);
}
