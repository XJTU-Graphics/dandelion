#include "shader.h"
#include "../utils/math.hpp"

#ifdef _WIN32
#undef min
#undef max
#endif

using Eigen::Vector3f;
using Eigen::Vector4f;

// vertex shader & fragement shader can visit
// all the static variables below from Uniforms structure
Eigen::Matrix4f Uniforms::MVP;
Eigen::Matrix4f Uniforms::inv_trans_M;
int Uniforms::width;
int Uniforms::height;

// vertex shader
VertexShaderPayload vertex_shader(const VertexShaderPayload& payload)
{
    VertexShaderPayload output_payload = payload;

    

    // Vertex position transformation
    

    // Viewport transformation
    

    // Vertex normal transformation
    

    return output_payload;
}

Vector3f phong_fragment_shader(const FragmentShaderPayload& payload, GL::Material material,
                               const std::list<Light>& lights, Camera camera)
{
    // these lines below are just for compiling and can be deleted
    (void)payload;
    (void)material;
    (void)lights;
    (void)camera;
    // these lines above are just for compiling and can be deleted
    
    Vector3f result = {0, 0, 0};
    
    // ka,kd,ks can be got from material.ambient,material.diffuse,material.specular
    

    // set ambient light intensity

    // Light Direction
        
    // View Direction
        
    // Half Vector
        
    // Light Attenuation
        
    // Ambient
        
    // Diffuse
        
    // Specular
        
    // set rendering result max threshold to 255
    
    return result * 255.f;
}
