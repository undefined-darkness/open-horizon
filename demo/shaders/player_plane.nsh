@all

varying vec2 tc;
varying vec3 normal;
varying vec3 tangent;
varying vec3 bitangent;
varying vec3 pos;

@vertex

void main()
{
	pos = gl_Vertex.xyz;
    tc = gl_MultiTexCoord0.xy;
	normal = gl_Normal.xyz;
    tangent = gl_MultiTexCoord1.xyz;
    bitangent = gl_MultiTexCoord2.xyz;

    gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;
}

@sampler amb_map "ambient"
@sampler base_map "diffuse"
@sampler norm_map "normal"
@sampler spec_map "specular"

@predefined camera_pos "nya camera position":local
@uniform light_dir "light dir":local_rot

@fragment

uniform sampler2D amb_map;
uniform sampler2D base_map;
uniform sampler2D norm_map;
uniform sampler2D spec_map;

uniform vec4 camera_pos;
uniform vec4 light_dir;

void main()
{
    vec4 amb=texture2D(amb_map, tc);
    vec4 base=texture2D(base_map, tc);
    vec4 spec=texture2D(spec_map, tc);

	vec3 v=normalize(camera_pos.xyz-pos);
    vec3 h=normalize(light_dir.xyz+v);

    vec3 n;
    vec3 norm = normalize(normal);
	n.xz = texture2D(norm_map, tc).wy * 2.0 - vec2(1.0);
	n.y = sqrt(1.0 - dot(n.xz, n.xz));
    n = normalize(n);
	n = normalize(n.x * normalize(tangent) + n.y * norm + n.z * normalize(bitangent));
    n = mix(norm, n, length(tangent));

    float sp=pow(max(0.0,dot(n,h)),20.0);

    vec3 s=spec.xyz*sp*5.0;

	gl_FragColor=vec4(base.rgb*vec3(amb.rgb*0.7+0.3*max(0.0,dot(n,light_dir.xyz)))+s,base.a+length(s));
    //gl_FragColor=vec4(base.rgb*mix(vec3(max(0.0,dot(n,light_dir.xyz))),vec3(1.0),amb.rgb)+s,base.a+length(s));
}
