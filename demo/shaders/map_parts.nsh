@sampler color_map "color"
@sampler base_map "diffuse"
@sampler specular_map "specular"
@sampler env_map "reflection"

@predefined camera_pos "nya camera position":local
@uniform tr "transform"

@uniform light_dir "light dir":local_rot=0.433,0.5,0.75,0.0
@uniform fog_color "fog color" = 0.688,0.749,0.764,-0.0002
@uniform fog_height "fog height" = 1,0,0,0
@uniform map_param_vs "map param vs" = 60
@uniform map_param_ps "map param ps" = 1,3,0.3,-0.1
@uniform map_param2_ps "map param2 ps" = 500,1.5,20,4
@uniform specular_color "specular color" = 0.6,0.572,0.466,2.5

@all

varying vec2 tc;
varying vec3 normal;
varying vec4 vcolor;
varying vec3 eye;
varying float vfogh;
varying float vfogf;
varying float vspec;

@vertex

uniform vec4 camera_pos;
uniform vec4 tr[500];

uniform sampler2D color_map;

uniform vec4 light_dir;
uniform vec4 fog_height;
uniform vec4 map_param_vs;

void main()
{
    tc = gl_MultiTexCoord0.xy;
	normal = gl_Normal.xyz;

    float pckd = tr[gl_InstanceID].w;
    float unpckd = floor(pckd);
    float color_coord_y = pckd - unpckd;
    float a = unpckd/512.0;
    vec2 rot = vec2(sin(a),cos(a));

    vcolor = texture2D(color_map, vec2(gl_MultiTexCoord0.z, color_coord_y));
    eye = normalize(camera_pos.xyz - gl_Vertex.xyz);
    
    float ndoth = max(dot(normal, normalize(light_dir.xyz + eye)), 0.0);
    vspec = pow(2.0, map_param_vs.x * log(ndoth) / log(2.0));

    vfogh = fog_height.y - (gl_Vertex.y);// + model_pos.y); //ToDo
    vec4 pos = gl_Vertex;

    pos.xz = pos.xx * vec2(rot.y,-rot.x) + pos.zz * rot.xy;
    pos.xyz += tr[gl_InstanceID].xyz;
    pos = gl_ModelViewProjectionMatrix * pos;
    vfogf = pos.z;
    gl_Position = pos;
}

@fragment

uniform sampler2D base_map;
uniform sampler2D specular_map;
uniform samplerCube env_map;

uniform vec4 fog_color;
uniform vec4 fog_height;
uniform vec4 map_param_ps;
uniform vec4 map_param2_ps;
uniform vec4 specular_color;

void main()
{
    vec4 color = texture2D(base_map, tc);
	if(color.a < 0.001) discard;
	float cy = color.y;

	//fog

	float fog_h = clamp(pow(2.0, 1.44269502 * fog_height.w * vfogh), 0.0, 1.0);
	float fog_f = clamp(pow(2.0, 1.44269502 * fog_height.z * vfogf), 0.0, 1.0);
	float fog_hf = mix(fog_h, 1.0, fog_f);
	float f = 1.0 - fog_hf;

	vec3 e = normalize(eye);

	float f2 = max(e.y, 0.0);
	f2 -= 1.0 - fog_height.x;
	f += f2;
	if(f2 < 0.0) f = 0.0;
	f += fog_hf;

	float fog_p = pow(2.0, 1.44269502 * fog_color.w * vfogf);
	float fog = min(f, fog_p);

	//env

	vec3 n = normalize(normal);
	vec3 env_dir = normalize(-n * dot(-e, n) * 2.0 - e);
	env_dir.z = -env_dir.z;
	vec4 env = textureCube(env_map, env_dir);
	env *= map_param2_ps.w;

	color *= vcolor;

	color.xyz = env.xyz * texture2D(specular_map, tc).xyz + color.xyz;
	color.xyz = mix(fog_color.xyz, color.xyz, fog);

	//specular

    fog = clamp(fog * map_param_ps.y, 0.0, 1.0);
	vec3 spec = specular_color.xyz * vspec * map_param_ps.x * fog * cy;
	float s = pow(cy, specular_color.w);
	color.xyz += s * spec;
	spec = s * specular_color.xyz;
	float s2 = 1.0 - dot(n, e);
	s = s2 * s2;
	s = s * s;
	s = s2 * s + map_param_ps.w;
	s = clamp(s, 0.0, map_param_ps.z);
	spec = s * spec;
	color.xyz += spec * fog;

	color.xyz *= 0.8;
	gl_FragColor = color;
}
