@all

varying vec2 tc;
varying vec3 normal;
varying vec3 pos;

@vertex

void main()
{
    tc=gl_MultiTexCoord0.xy;
	normal=gl_Normal.xyz;
	pos=gl_Vertex.xyz;
    gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;
}

@sampler amb_map "ambient"
@sampler base_map "diffuse"
@sampler spec_map "specular"

@predefined camera_pos "nya camera position":local
@uniform light_dir "light dir":local_rot

@fragment

uniform sampler2D amb_map;
uniform sampler2D base_map;
uniform sampler2D spec_map;

uniform vec4 camera_pos;
uniform vec4 light_dir;

void main()
{
    vec4 amb=texture2D(amb_map,tc);
    vec4 base=texture2D(base_map,tc);
    vec4 spec=texture2D(spec_map,tc);

	vec3 v=normalize(camera_pos.xyz-pos);
    vec3 h=normalize(light_dir.xyz+v);
    float sp=pow(max(0.0,dot(normal,h)),50.0);

    vec3 s=spec.xyz*sp;

	gl_FragColor=vec4(base.rgb*vec3(amb.rgb*0.7+0.3*max(0.0,dot(normal,light_dir.xyz)))+s,base.a+length(s));
}
