@sampler amb_map "ambient"
@sampler base_map "diffuse"
@sampler norm_map "normal"
@sampler spec_map "specular"
@sampler env_map "reflection"
@sampler ibl_map "ibl"

@predefined camera_pos "nya camera position":local
@predefined model_rot "nya model rot"

@predefined bones_pos_map "nya bones pos texture"
@predefined bones_rot_map "nya bones rot texture"

@uniform light_dir "light dir":local_rot
@uniform specular_param "specular param"=0.675,1.035,0.0,0.225
@uniform ibl_param "ibl param"=1,1,0,0
@uniform rim_light_mtr "rim light mtr"=0.9,2.0,0.345,0.0

@all

varying vec2 tc;
varying vec3 normal;
varying vec3 tangent;
varying vec3 bitangent;
varying vec3 pos;
varying vec3 normal_tr;

@vertex

uniform sampler2D bones_pos_map;
uniform sampler2D bones_rot_map;

uniform vec4 model_rot;

vec3 tr(vec3 v, vec4 q) { return v + cross(q.xyz, cross(q.xyz, v) + v * q.w) * 2.0; }

void main()
{
    vec2 btc=vec2(gl_MultiTexCoord0.z,0.0);

    pos = gl_Vertex.xyz;
    normal = gl_Normal.xyz;
    tangent = gl_MultiTexCoord1.xyz;
    bitangent = gl_MultiTexCoord2.xyz;

    if (gl_MultiTexCoord0.z > -0.5)
    {
        vec4 q=texture2D(bones_rot_map,btc);
	    pos = texture2D(bones_pos_map,btc).xyz + tr(pos, q);
        normal = tr(normal, q);
        tangent = tr(tangent, q);
        bitangent = tr(bitangent, q);
    }

    tc = gl_MultiTexCoord0.xy;
    normal_tr = tr(normal, model_rot);

    gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 1.0);
}

@fragment

uniform sampler2D amb_map;
uniform sampler2D base_map;
uniform sampler2D norm_map;
uniform sampler2D spec_map;
uniform samplerCube env_map;
uniform samplerCube ibl_map;

uniform vec4 camera_pos;
uniform vec4 light_dir;

uniform vec4 specular_param;
uniform vec4 ibl_param;
uniform vec4 rim_light_mtr;

void main()
{
    float shadow = 0.0; //ToDo

    vec4 amb=texture2D(amb_map, tc);
    vec4 base=texture2D(base_map, tc);
    vec4 spec_mask=texture2D(spec_map, tc);

	vec3 v=normalize(camera_pos.xyz-pos);
    vec3 h=normalize(light_dir.xyz+v);

    vec3 n;
    vec3 norm = normalize(normal);
	n.xz = texture2D(norm_map, tc).wy * 2.0 - vec2(1.0);
	n.y = sqrt(1.0 - dot(n.xz, n.xz));
    n = normalize(n);
	n = normalize(n.x * normalize(tangent) + n.y * norm + n.z * normalize(bitangent));
    n = mix(norm, n, length(tangent));

    float nv = dot(n, v);
    float nv_p = max(nv, 0.0);
	float fresnel_diff = clamp(specular_param.y - nv_p, 0.0, 1.0);

	vec3 env_dir = -reflect(v,n);
	env_dir.z = -env_dir.z;
	vec3 env = textureCube(env_map, env_dir).xyz * spec_mask.xyz * fresnel_diff;

    vec4 color;

    color.a = base.a; //ToDo: max(ACE_vCommonParam1.x, base.a);

    float base_max = max(base.x, max(base.y, base.z));

	float fresnel_pow;
	float c = 0.325;
	if(base_max < c)
		fresnel_pow = (c - base_max) * 20.77 + 1.75;
	else
		fresnel_pow = max((0.8 - base_max) * 3.6842, 0.0);
	fresnel_pow *= ibl_param.y;

	float fr = rim_light_mtr.z - nv_p;
	fresnel_pow *= fr;

	vec4 ibl = textureCube(ibl_map, normal_tr) * ibl_param.x;

//-----------------------------------------------------------------------------------------

    float spe_pow=pow(max(0.0,dot(n,h)),20.0);
    vec3 spec=spec_mask.xyz*spe_pow*10.0;
    vec3 light = vec3(0.7 + 0.3*max(0.0,dot(n,light_dir.xyz)));
    vec3 amb_light = amb.rgb;

//-----------------------------------------------------------------------------------------

    color.xyz = base.xyz * ibl.xyz * max(fresnel_pow, 0.0) * (vec3(1.0) - ibl_param.z * shadow);

	color.xyz += amb_light * (light * base.xyz + env);
	color.xyz += spec * spec_mask.xyz;
	color.a = min(color.a + spe_pow, 1.0);

    //color.xyz = mix(fog_color.xyz, color.xyz, fog); //ToDo

	color.xyz *= 0.8;
	if(1.0 - shadow < 0.015)
	    color = vec4(0.0);

	gl_FragColor = color;
}
