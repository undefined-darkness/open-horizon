@sampler base_map "diffuse"
@uniform fog_color "fog color"

@all
varying vec4 color;

@vertex
uniform samplerCube base_map;
uniform vec4 fog_color;

void main()
{
    vec3 tc = gl_Vertex.xyz;
    tc.z = -tc.z;
    color = textureCube(base_map, tc);
    
    if(gl_Vertex.y<0.5)
        color = fog_color;

    gl_Position=gl_ModelViewProjectionMatrix * gl_Vertex;
}

@fragment
void main() { gl_FragColor=color * 0.8; }
