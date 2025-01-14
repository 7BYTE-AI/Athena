//////////////////////// Athena Irradiance Map Convolution Shader ////////////////////////

// References:
// https://learnopengl.com/PBR/IBL/Diffuse-irradiance


#version 460 core
#pragma stage : compute

#include "Include/Common.glslh"

#define SAMPLE_DELTA 0.025

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(set = 1, binding = 0) uniform samplerCube u_Cubemap;
layout(r11f_g11f_b10f, set = 1, binding = 1) uniform imageCube u_IrradianceMap;

void main()
{
    ivec3 unnormalizedTexCoords = ivec3(gl_GlobalInvocationID.xyz);
    vec3 direction = GetWorldDirectionFromCubeCoords(unnormalizedTexCoords, vec2(gl_NumWorkGroups * gl_WorkGroupSize));
    
    // the sample direction equals the hemisphere's orientation 
    vec3 normal = direction;

    vec3 irradiance = vec3(0.0);

    vec3 up = vec3(0.0, 1.0, 0.0);
    if(normal.y == 1 || normal.y == -1)
        up = vec3(1.0, 0.0, 0.0);

    vec3 right = normalize(cross(up, normal));
    up = normalize(cross(normal, right));

    float nrSamples = 0.0; 
    for(float phi = 0.0; phi < 2.0 * PI; phi += SAMPLE_DELTA)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += SAMPLE_DELTA)
        {
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal; 

            vec3 color = texture(u_Cubemap, sampleVec).rgb;
            irradiance += color * cos(theta) * sin(theta);
            nrSamples++;
        }
    }

    irradiance = PI * irradiance * (1.0 / float(nrSamples));

    imageStore(u_IrradianceMap, unnormalizedTexCoords, vec4(irradiance, 1));
}
