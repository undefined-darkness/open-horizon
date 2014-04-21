@all

varying vec2 tc;
varying vec3 normal;
varying float dist;

uniform vec4 camera_pos;

@vertex

void main()
{
    tc=gl_MultiTexCoord0.xy;
	normal=gl_Normal.xyz;
	dist=length(camera_pos.xyz-gl_Vertex.xyz);
    gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;
}

@predefined camera_pos "nya camera position":local
@uniform light_dir "light dir":local_rot
@uniform fog_color "for color" = 0.69,0.74,0.76,-0.0002

@sampler base_map "diffuse"

@fragment

uniform sampler2D base_map;
uniform vec4 light_dir;
//uniform vec4 fog_color;

void main()
{
    vec4 base=texture2D(base_map,tc);
	if(base.a<0.5)
		discard;

    vec4 fog_color=vec4(0.69,0.74,0.76,-0.0002);
	//vec4 fog_color=vec4(0.4,0.5,0.9,-0.0002);

	vec3 l=vec3(0.3+0.7*max(dot(normal,light_dir.xyz),0.0));
	float fa=dist*fog_color.a;
	//float f=exp(-fa*fa);
	float f=exp(fa);

	gl_FragColor=vec4(mix(fog_color.xyz,base.xyz*l,f),1.0);
}
