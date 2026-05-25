#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexcoord;

layout(set=1, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColor;

void main() 
{
    vec2 uv = vec2(fragTexcoord.x,1.-fragTexcoord.y);
    outColor = 10.*texture(textureSampler, fragTexcoord);
}