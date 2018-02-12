@sampler amb_map "ambient"
@sampler base_map "diffuse"
@sampler norm_map "normal"
@sampler spec_map "specular"
@sampler env_map "reflection"
@sampler shadow_map "shadows"
@sampler ibl_map "ibl"
@sampler params_map "params"

@predefined camera_pos "nya camera position":local
@predefined model_rot "nya model rot"

@uniform light_dir "light dir":local_rot
@uniform shadow_tr "shadows tr"

@include "skeleton.nsh"

@all

varying vec2 tc;
varying vec3 normal;
varying vec3 tangent;
varying vec3 bitangent;
varying vec3 pos;
varying vec3 normal_tr;
varying vec3 env_dir;
varying vec3 shadow_tc;

varying vec4 specular_param; //0.675,1.035,0.0,0.225
varying vec4 ibl_param; //1,1,0,0
varying vec4 rim_light_mtr; //0.9,2.0,0.345,0.0
varying vec4 alpha_clip;

uniform vec4 camera_pos;

@vertex

uniform vec4 shadow_tr[4];

uniform sampler2D params_map;
uniform vec4 model_rot;

void main()
{
    float ptc = gl_MultiTexCoord0.z;
    specular_param = texture2D(params_map,vec2(ptc, (0.5 + 0.0) / 5.0));
    ibl_param = texture2D(params_map,vec2(ptc, (0.5 + 1.0) / 5.0));
    rim_light_mtr = texture2D(params_map,vec2(ptc, (0.5 + 2.0) / 5.0));
    alpha_clip = texture2D(params_map,vec2(ptc, (0.5 + 3.0) / 5.0));
    
    pos = tr(gl_Vertex.xyz, gl_MultiTexCoord3, gl_MultiTexCoord4);
    normal = trn(gl_Normal, gl_MultiTexCoord3, gl_MultiTexCoord4);
    tangent = trn(gl_MultiTexCoord1.xyz, gl_MultiTexCoord3, gl_MultiTexCoord4);
    bitangent = trn(gl_MultiTexCoord2.xyz, gl_MultiTexCoord3, gl_MultiTexCoord4);

    tc = gl_MultiTexCoord0.xy;
    normal_tr = tr(normal, model_rot);

    env_dir = tr(-reflect(normalize(camera_pos.xyz - pos), normal), model_rot);
    env_dir.z = -env_dir.z;

    gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 1.0);

    vec4 stc = mat4(shadow_tr[0], shadow_tr[1], shadow_tr[2], shadow_tr[3]) * vec4(tr(pos,model_rot), 1.0);
    shadow_tc = stc.xyz / stc.w * 0.5 + 0.5;
    shadow_tc.z -= 0.0001;
}

@fragment

uniform sampler2D amb_map;
uniform sampler2D base_map;
uniform sampler2D norm_map;
uniform sampler2D spec_map;
uniform samplerCube env_map;
uniform samplerCube ibl_map;
uniform sampler2D shadow_map;

uniform vec4 light_dir;

void main()
{
    vec4 base=texture2D(base_map, tc);
    if(base.a < alpha_clip.x) discard;

    float shadow = 0.0;
    if (texture2D(shadow_map, shadow_tc.xy + vec2(-0.94201624, -0.39906216) / 1024.0).z < shadow_tc.z)
        shadow+=0.25;
    if (texture2D(shadow_map, shadow_tc.xy + vec2(0.94558609, -0.76890725) / 1024.0).z < shadow_tc.z)
        shadow+=0.25;
    if (texture2D(shadow_map, shadow_tc.xy + vec2(-0.094184101, -0.92938870) / 1024.0).z < shadow_tc.z)
        shadow+=0.25;
    if (texture2D(shadow_map, shadow_tc.xy + vec2(0.34495938, 0.29387760) / 1024.0).z < shadow_tc.z)
        shadow+=0.25;

    vec4 amb=texture2D(amb_map, tc);
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
    vec3 light = vec3(0.6 + 0.6*max(0.0,dot(n,light_dir.xyz)-0.3) * (1.0-shadow));
    vec3 amb_light = amb.rgb;

//-----------------------------------------------------------------------------------------

    color.xyz = base.xyz * ibl.xyz * max(fresnel_pow, 0.0) * (vec3(1.0) - ibl_param.z * shadow);

	color.xyz += amb_light * (light * base.xyz + env);
	color.xyz += spec * spec_mask.xyz;
	color.a = min(color.a + spe_pow, 1.0);

    //color.xyz = mix(fog_color.xyz, color.xyz, fog); //ToDo

	color.xyz *= 0.8;
    gl_FragColor = color;
}
