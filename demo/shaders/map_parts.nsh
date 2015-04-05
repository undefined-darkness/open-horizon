@sampler base_map "diffuse"
@sampler specular_map "specular"
@sampler env_map "reflection"
@sampler color_map "color"

@predefined camera_pos "nya camera position":local
@uniform light_dir "light dir":local_rot
@uniform fog_color "fog color" = 0.69,0.74,0.76,-0.0002

@uniform color_coord "color coord"

@all

varying vec2 tc;
varying vec3 normal;
varying float dist;
varying vec4 color;

uniform vec4 camera_pos;

uniform vec4 color_coord;

uniform sampler2D color_map;

@vertex

void main()
{
    tc=gl_MultiTexCoord0.xy;
	normal=gl_Normal.xyz;
	dist=length(camera_pos.xyz-gl_Vertex.xyz);
    color=texture2D(color_map,vec2(gl_MultiTexCoord0.z,color_coord.x));
    gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;
}

@fragment

uniform sampler2D base_map;
uniform sampler2D specular_map;
uniform samplerCube env_map;

uniform vec4 light_dir;
//uniform vec4 fog_color;

void main()
{
    vec4 base=texture2D(base_map,tc);
	if(base.a<0.5)
		discard;

    base *= color;

    vec4 spec=texture2D(specular_map,tc);

    vec4 env=textureCube(env_map,vec3(tc,0.0));

    vec4 fog_color=vec4(0.69,0.74,0.76,-0.0004);
	//vec4 fog_color=vec4(0.4,0.5,0.9,-0.0004);

	float fa=dist*fog_color.a;
	//float f=exp(-fa*fa);
	float f=exp(fa);

	gl_FragColor=vec4(mix(fog_color.xyz,base.xyz,f),1.0);
}
