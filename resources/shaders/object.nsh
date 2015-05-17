@sampler base_map "diffuse"
@uniform light_dir "light dir":local_rot
@uniform alpha_clip "alpha clip"=-1.0
@uniform diff_k "diff k"=0.6,0.4

@predefined bones_pos_map "nya bones pos texture"
@predefined bones_rot_map "nya bones rot texture"

@all

varying vec2 tc;
varying vec3 normal;

@vertex

uniform sampler2D bones_pos_map;
uniform sampler2D bones_rot_map;

vec3 tr(vec3 v, vec4 q) { return v + cross(q.xyz, cross(q.xyz, v) + v * q.w) * 2.0; }

void main()
{
	vec3 pos = gl_Vertex.xyz;
    normal = gl_Normal.xyz;

    if (gl_MultiTexCoord0.z > -0.5)
    {
        vec2 btc=vec2(gl_MultiTexCoord0.z,0.0);
        vec4 q=texture2D(bones_rot_map,btc);

        vec3 pos = texture2D(bones_pos_map,btc).xyz + tr(pos, q);
        normal = tr(normal, q);
    }

    tc = gl_MultiTexCoord0.xy;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 1.0);
}

@fragment

uniform sampler2D base_map;
uniform vec4 light_dir;
uniform vec4 alpha_clip;
uniform vec4 diff_k;

void main()
{
    vec4 base = texture2D(base_map, tc);
    if(base.a < alpha_clip.x) discard;

    vec3 light = vec3(diff_k.x + diff_k.y*max(0.0,dot(normal,light_dir.xyz)));
    base.rgb *= light;
    gl_FragColor = base;
}
