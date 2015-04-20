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
    color = mix(fog_color, color, min(gl_Vertex.y, 1.0));
    gl_Position=gl_ModelViewProjectionMatrix * gl_Vertex;
}

@fragment
void main() { gl_FragColor=color * 0.8; }
