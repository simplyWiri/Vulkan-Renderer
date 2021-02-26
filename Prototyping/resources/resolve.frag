#version 450 core
#extension GL_EXT_nonuniform_qualifier : require


layout(location = 0) out vec4 FragColor;
  

layout(location = 0) in vec2 TexCoords;

layout(set = 1, binding = 0) uniform sampler2D[] tex;

// We send no (push constant) information to the vert shader so we don't need an offset here
layout(push_constant) uniform PushConstant {
    uint gPosition;
    uint gNormal;
    uint gAlbedo;
};

struct Light {
    vec3 Position;
    vec3 Color;
};

const Light l = Light(vec3(1,1,1), vec3(0.5, 0.7, 0.9));

layout(binding = 3) uniform V {
	vec3 viewPos;
};

void main()
{             
    // retrieve data from G-buffer
    vec3 FragPos = texture( tex[gPosition], TexCoords).rgb;
    vec3 Normal = texture( tex[gNormal], TexCoords).rgb;
    vec3 Albedo = texture( tex[gAlbedo], TexCoords).rgb;
    
    // then calculate lighting as usual
    vec3 lighting = Albedo * 0.1; // hard-coded ambient component
    vec3 viewDir = normalize(viewPos - FragPos);
	// diffuse
	vec3 lightDir = normalize(l.Position - FragPos);
	vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Albedo * l.Color;
    lighting += diffuse;
    
    FragColor = vec4(lighting, 1.0);
}  
