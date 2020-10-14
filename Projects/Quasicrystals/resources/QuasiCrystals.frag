#version 450 core

layout(location = 0) out vec4 fColor;

layout(binding = 0) uniform i
{
    float time;
    float xMult;
    float yMult;
    float padding;
} info;

const float PI = 3.14159265358979323846264;

vec4 color(float t) 
{
    const float ar = 0.50, ag = 0.50, ab = 0.50;
    const float br = 0.50, bg = 0.50, bb = 0.50;
    const float cr = 1.00, cg = 1.00, cb = 1.00;
    const float dr = 0.00, dg = 0.10, db = 0.20;
    return vec4(ar + br * cos(2.0 * PI * (cr * t + dr)),
        ag + bg * cos(2.0 * PI * (cg * t + dg)),
        ab + bb * cos(2.0 * PI * (cb * t + db)),
        1.0);
}

void main() 
{
    const int n = 8;
    const float step = PI / float(n);
    float alpha;

    for (int i = 0; i < n; i++) {
        float theta = float(i) * step;
        alpha += cos(cos(theta) * gl_FragCoord.x/info.xMult + sin(theta) * gl_FragCoord.y/info.yMult + info.time);
    }

    fColor = color(alpha * 2.0 / float(n));
}