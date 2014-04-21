@all

varying vec2 tc;
varying float dist;

uniform vec4 camera_pos;

@vertex

void main()
{
    tc=gl_MultiTexCoord0.xy;
	dist=length(camera_pos.xyz-gl_Vertex.xyz);
    gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;
}

@predefined camera_pos "nya camera position"
@uniform fog_color "for color" = 0.69,0.74,0.76,-0.0002

@sampler base_map "diffuse"

@fragment

uniform sampler2D base_map;
//uniform vec4 fog_color;

void main()
{
    vec4 base=texture2D(base_map,tc);
	if(base.a<0.5)
		discard;

    vec4 fog_color=vec4(0.69,0.74,0.76,-0.0002);
	//vec4 fog_color=vec4(0.4,0.5,0.9,-0.0002);

	float fa=dist*fog_color.a;
	//float f=exp(-fa*fa);
	float f=exp(fa);

	gl_FragColor=vec4(mix(fog_color.xyz,base.xyz,f),1.0);
}
