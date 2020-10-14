#version 450 core

struct Circle
{
    vec3 pos;
    float radius;
};

layout(location = 0) out vec4 fColor;

layout(binding = 0) buffer Circles
{
    Circle[3] buf;
} circles;

float smin(float a, float b, float k)
{
    float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
    return mix( b, a, h ) - k*h*(1.0-h);
}

float distToCenter(vec2 point, Circle circle)
{
    return sqrt( pow(point.x - circle.pos.x, 2.0) + pow(point.y - circle.pos.y, 2.0) );
}

const float timeStep = 1.0;
const float smoothFactor = 0.02;

void main()
{
    vec2 normFrag = gl_FragCoord.xy/1080;

    float centerDist = distToCenter(normFrag, circles.buf[0]);
    float centerDist2 = distToCenter(normFrag, circles.buf[1]);  
    float centerDist3 = distToCenter(normFrag, circles.buf[2]);
    float closestCenterDist = min(min(centerDist, centerDist2), centerDist3);
    float circleDist = centerDist - circles.buf[0].radius;    

    circleDist = smin(circleDist, centerDist2 - circles.buf[1].radius, smoothFactor);
    circleDist = smin(circleDist, centerDist3 - circles.buf[2].radius, smoothFactor);

    if(circleDist <= .01)
    {        
        closestCenterDist /= .2;
        centerDist /= .2;
        centerDist2 /= .2;
        centerDist3 /= .2;

        float totalDist = centerDist + centerDist2 + centerDist3;

        float c1Infl = centerDist / totalDist;
        float c2Infl = centerDist2 / totalDist;
        float c3Infl = centerDist3 / totalDist;

        vec3 col1 = vec3(146, 0, 255) * c1Infl;
        vec3 col2 = vec3(255, 146, 0) * c2Infl;
        vec3 col3 = vec3(0, 255, 146) * c3Infl;
        
        fColor = vec4((col1 + col2 + col3)/255.0, 1.0);
    }
    else
    {
    	fColor = vec4(0);   
    }
}
