#version 450

#include "toto.header"

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexcoord;

layout(set=1, binding = 0) uniform sampler2D samplerAlbedo;
layout(set=1, binding = 1) uniform sampler2D samplerNormal;
layout(set=1, binding = 2) uniform sampler2D samplerAO;
layout(set=1, binding = 3) uniform sampler2D samplerRoughness;
layout(set=1, binding = 4) uniform sampler2D samplerMetalness;
layout(set=1, binding = 5) uniform sampler2D samplerEmissive;

layout(location = 0) out vec4 outColor;

void main() 
{
    vec3 albedo = texture(samplerAlbedo, fragTexcoord).rgb;
    vec3 normals = texture(samplerNormal, fragTexcoord).rgb;
	vec3 ao = texture(samplerAO, fragTexcoord).rgb;
	vec3 roughness = texture(samplerRoughness, fragTexcoord).rgb;
	vec3 metalness = texture(samplerMetalness, fragTexcoord).rgb;
	vec3 emissive = texture(samplerEmissive, fragTexcoord).rgb;

    outColor.rgb = normals;
    outColor.a = 1.0;
}