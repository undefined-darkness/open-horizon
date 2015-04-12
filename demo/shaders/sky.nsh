@sampler base_map "diffuse"

@all
varying vec4 color;

@vertex
uniform samplerCube base_map;

void main()
{
    color=textureCube(base_map,gl_Vertex.rgb);
    gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;
}

@fragment
void main() { gl_FragColor=color * 0.8; //*1.2 * 0.8
}
