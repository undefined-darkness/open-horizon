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

void main()
{
    vec4 p = gl_Vertex;
    vec2 d = gl_MultiTexCoord0.zw;

    p.xyz += d.x * right.xyz + d.y * up.xyz;

    eye = get_eye(p.xyz);
    vfogh = get_fogh(p.xyz);

    tc = gl_MultiTexCoord0.xy;
    
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
