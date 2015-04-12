@include "common.nsh"

@sampler base_map "diffuse"
@sampler detail_map "detail"
@sampler ocean_map "ocean"
@sampler normal_map "normal"

@uniform anim "anim"
@uniform light_dir "light dir"

@all

varying vec2 tc;
varying vec2 ocean_tc;
varying vec2 detail_tc;
varying vec2 normal_tc;
varying vec2 normal_tc2;
varying vec3 pos;

varying vec3 vpos;
varying float vfogh;
varying float vfogf;

@vertex

uniform vec4 anim;

void main()
{
    tc=gl_MultiTexCoord0.xy;
    ocean_tc=gl_Vertex.xz*0.0000076-vec2(0.5,0.5);
    detail_tc=gl_Vertex.xz*0.01;
    normal_tc=gl_Vertex.xz*0.005+anim.xy;
    normal_tc2=gl_Vertex.xz*0.005+anim.zw;

    vpos = gl_Vertex.xyz;
    vec4 pos = gl_ModelViewProjectionMatrix * gl_Vertex;
    vfogh = get_fogh(vpos.xyz);
    vfogf = get_fogv(pos);
    gl_Position = pos;
}

@fragment

uniform sampler2D base_map;
uniform sampler2D detail_map;
uniform sampler2D ocean_map;
uniform sampler2D normal_map;

uniform vec4 light_dir;

void main()
{
    vec4 base=texture2D(base_map,tc);
    vec4 ocean=texture2D(ocean_map,ocean_tc);
    vec4 detail=texture2D(detail_map,detail_tc)*0.5+vec4(0.5);
    vec3 normal=normalize(texture2D(normal_map,normal_tc).rgb
                          +texture2D(normal_map,normal_tc2).rgb
                          -vec3(1.0));

	vec3 e = get_eye(vpos.xyz);

    float ol=1.0+dot(normal,light_dir.xyz);
    vec3 h=normalize(light_dir.xyz-e);
    float s=pow(max(0.0,dot(normal,h)),60.0)*0.5;

    base.rgb=mix(ocean.rgb*ol+vec3(s),base.rgb*detail.rgb,base.a);

	float fog = get_fog(vfogf, vfogh, e);

	gl_FragColor=vec4(mix(fog_color.xyz,base.xyz,fog) * 0.8,1.0);
}
