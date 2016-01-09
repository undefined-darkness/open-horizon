@include "common.nsh"

@all

varying vec2 tc;
varying vec3 eye;
varying float vfogh;
varying float vfogf;

@vertex

void main()
{
    vec4 p = gl_Vertex;
    vec2 d = gl_MultiTexCoord0.zw;
        
    vfogh = get_fogh(p.xyz);

    eye = get_eye(p.xyz);
    vec3 right = normalize(vec3(eye.z, 0.0, -eye.x));
    vec3 up = cross(eye, right.xyz);

    p.xyz += d.x * right + d.y * up;

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
    vec4 color = texture2D(base_map, tc) * 1.2;
    if(color.a < 0.1)
        discard;
        //gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); else
        
    float fog = get_fog(vfogf, vfogh, eye);
	color.xyz = mix(fog_color.xyz, color.xyz, fog);

    gl_FragColor = color * 0.8;
}
