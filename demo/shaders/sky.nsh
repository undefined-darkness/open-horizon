@all
varying vec3 tc;

@vertex
void main()
{
    tc=gl_Vertex.xyz;
    gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;
}

@sampler base_map "diffuse"

@fragment
uniform samplerCube base_map;
void main() { gl_FragColor=textureCube(base_map,tc); }
