//////////////////////// Athena Directional Light Shadow Map Shader////////////////////////

#version 460 core
#pragma stage : vertex


layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoords;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in vec3 a_Tangent;
layout(location = 4) in vec3 a_Bitangent;


layout(push_constant) uniform MaterialData
{
    mat4 u_Transform;
};


void main()
{
    gl_Position = u_Transform * vec4(a_Position, 1.0);
}

#version 460 core
#pragma stage : geometry

#include "Include/Shadows.glslh"

layout(triangles, invocations = SHADOW_CASCADES_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;
    

void main()
{          
    for (int i = 0; i < 3; ++i)
    {
        gl_Position = u_DirLightViewProjection[gl_InvocationID] * gl_in[i].gl_Position;
        gl_Layer = gl_InvocationID;
        EmitVertex();
    }

    EndPrimitive();
} 
