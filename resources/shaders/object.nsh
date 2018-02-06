@sampler base_map "diffuse"
@sampler params_map "params"

@uniform light_dir "light dir":local_rot

@include "skeleton.nsh"

@all

varying vec2 tc;
varying vec3 normal;
varying vec4 alpha_clip; //-1.0
varying vec4 diff_k; //0.6,0.4

@vertex

uniform sampler2D params_map;

void main()
{
	vec3 pos = tr(gl_Vertex.xyz, gl_MultiTexCoord3, gl_MultiTexCoord4);
    normal = trn(gl_Normal, gl_MultiTexCoord3, gl_MultiTexCoord4);

    float ptc = gl_MultiTexCoord0.z;
    alpha_clip = texture2D(params_map,vec2(ptc, (0.5 + 3.0) / 5.0));
    diff_k = texture2D(params_map,vec2(ptc, (0.5 + 4.0) / 5.0));

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
