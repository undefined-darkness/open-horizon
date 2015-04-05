@all

varying vec2 tc;
varying vec2 ocean_tc;
varying vec2 detail_tc;
varying vec2 normal_tc;
varying vec2 normal_tc2;
varying float dist;
varying vec3 pos;

uniform vec4 camera_pos;

@uniform anim "anim"

@vertex

uniform vec4 anim;

void main()
{
    tc=gl_MultiTexCoord0.xy;
    ocean_tc=gl_Vertex.xz*0.0000076-vec2(0.5,0.5);
    detail_tc=gl_Vertex.xz*0.01;
    normal_tc=gl_Vertex.xz*0.005+anim.xy;
    normal_tc2=gl_Vertex.xz*0.005+anim.zw;
	dist=length(camera_pos.xyz-gl_Vertex.xyz);
    pos=gl_Vertex.xyz;
    gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;
}

@predefined camera_pos "nya camera position"
@uniform fog_color "fog color" = 0.69,0.74,0.76,-0.0002
@uniform light_dir "light dir"

@sampler base_map "diffuse"
@sampler detail_map "detail"
@sampler ocean_map "ocean"
@sampler normal_map "normal"

@fragment

uniform sampler2D base_map;
uniform sampler2D detail_map;
uniform sampler2D ocean_map;
uniform sampler2D normal_map;

uniform vec4 light_dir;

//uniform vec4 fog_color;

void main()
{
    vec4 base=texture2D(base_map,tc);
    vec4 ocean=texture2D(ocean_map,ocean_tc);
    vec4 detail=texture2D(detail_map,detail_tc)*0.5+vec4(0.5);
    vec3 normal=normalize(texture2D(normal_map,normal_tc).rgb
                          +texture2D(normal_map,normal_tc2).rgb
                          -vec3(1.0));
    
    float ol=1.0+dot(normal,light_dir.xyz);
    vec3 v=normalize(pos-camera_pos.xyz);
    vec3 h=normalize(v+light_dir.xyz);
    float s=pow(max(0.0,dot(normal,h)),60.0)*0.5;

    base.rgb=mix(ocean.rgb*ol+vec3(s),base.rgb*detail.rgb,base.a);

    vec4 fog_color=vec4(0.69,0.74,0.76,-0.0002);
	//vec4 fog_color=vec4(0.4,0.5,0.9,-0.0002);

	float fa=dist*fog_color.a;
	//float f=exp(-fa*fa);
	float f=exp(fa);

	gl_FragColor=vec4(mix(fog_color.xyz,base.xyz,f),1.0);
}
