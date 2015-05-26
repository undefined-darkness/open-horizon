@sampler base_map "diffuse"
@sampler params_map "params"

@uniform light_dir "light dir":local_rot

@predefined bones_pos "nya bones pos"
@predefined bones_rot "nya bones rot"

//@predefined bones_pos_map "nya bones pos texture"=1024
//@predefined bones_rot_map "nya bones rot texture"=1024

@all

varying vec2 tc;
varying vec3 normal;
varying vec4 alpha_clip; //-1.0
varying vec4 diff_k; //0.6,0.4

@vertex

uniform vec3 bones_pos[250];
uniform vec4 bones_rot[250];

//uniform sampler2D bones_pos_map;
//uniform sampler2D bones_rot_map;
uniform sampler2D params_map;

vec3 tr(vec3 v, vec4 q) { return v + cross(q.xyz, cross(q.xyz, v) + v * q.w) * 2.0; }

void main()
{
	vec3 pos = gl_Vertex.xyz;
    normal = gl_Normal.xyz;

    float ptc = gl_MultiTexCoord3.x;
    alpha_clip = texture2D(params_map,vec2(ptc, (0.5 + 3.0) / 5.0));
    diff_k = texture2D(params_map,vec2(ptc, (0.5 + 4.0) / 5.0));

    if (gl_MultiTexCoord0.z > -0.5)
    {
        int bidx = int(gl_MultiTexCoord0.z);

        //vec4 q = texture2D(bones_rot_map, btc);
	    //pos = texture2D(bones_pos_map,btc).xyz + tr(pos, q);
        vec4 q = bones_rot[bidx];
	    pos = bones_pos[bidx] + tr(pos, q);

        normal = tr(normal, q);
    }

    tc = gl_MultiTexCoord0.xy;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 1.0);
}

@fragment

uniform sampler2D base_map;
uniform vec4 light_dir;

void main()
{
    vec4 base = texture2D(base_map, tc);
    if(base.a < alpha_clip.x) discard;

    vec3 light = vec3(diff_k.x + diff_k.y*max(0.0,dot(normal,light_dir.xyz)));
    base.rgb *= light;
    gl_FragColor = base;
}
