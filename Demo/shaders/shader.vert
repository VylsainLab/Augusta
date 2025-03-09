#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexcoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexcoord;

layout(binding = 0) uniform UniformBufferObject 
{
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConstantData 
{
  mat4 model;
} pcd;

void main() {
    gl_Position = ubo.proj * ubo.view * pcd.model * vec4(inPosition, 1.0);
    fragNormal = inNormal;
	fragTexcoord = inTexcoord;
}