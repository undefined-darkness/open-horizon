@include "common.nsh"
@uniform up "up" = 0.0,1.0,0.0
@uniform right "right" = 1.0,0.0,0.0

@all

varying vec2 tc;
varying vec3 eye;
varying float vfogh;
varying float vfogf;

@vertex
uniform vec4 up;
uniform vec4 right;

float rand(vec2 co) { return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453); }

void main()
{
    vec4 p = vec4(gl_Vertex.x, floor(gl_Vertex.y/128.0) * (10000.0 / 131072.0), gl_Vertex.z, 1.0);

    float idx = mod(gl_Vertex.y, 4.0);
    vec2 d = vec2(2.0 * step(1.5, idx) - 1.0, 1.0 - 2.0 * step(0.5, idx) * (1.0 - step(2.5, idx)));

    float tree_hsize = 7.0 + rand(gl_Vertex.xz) * 3.0;
    p.xyz += (d.x * right.xyz + d.y * up.xyz) * tree_hsize;
    p.y += tree_hsize;

    eye = get_eye(p.xyz);
    vfogh = get_fogh(p.xyz);

    tc = vec2(mod(floor(gl_Vertex.y / 4.0), 32.0) / up.w, d.y * 0.5 + 0.5);

    p = gl_ModelViewProjectionMatrix * p;
    vfogf = get_fogv(p);
    gl_Position = p;
}

@sampler base_map "diffuse"

@fragment

uniform sampler2D base_map;

void main()
{
    vec4 color = texture2D(base_map, tc);
    if(color.a < 0.4)
        discard;
        
    float fog = get_fog(vfogf, vfogh, eye);
	color.xyz = mix(fog_color.xyz, color.xyz * 1.2, fog) * 0.8;
    gl_FragColor = color;
}
