@predefined camera_pos "nya camera pos"

@uniform pos "tr pos"
@uniform param "tr param"
@uniform light_dir "light dir"=0.0,1.0,0.0

@all

varying vec2 tc;
varying vec4 color;

@vertex

uniform vec4 pos[500];
uniform vec4 param;
uniform vec4 camera_pos;
uniform vec4 light_dir;

void main()
{
    vec3 v = gl_Vertex.xyz;
    int idx = int(param.z + param.w * v.z);

    vec4 p = pos[idx];
    
    vec3 cam_dir = normalize(camera_pos.xyz - p.xyz);
    vec3 right = normalize(vec3(cam_dir.z, 0.0, -cam_dir.x));
    vec3 up = cross(cam_dir, right.xyz);

    float t = param.x - p.w;
    float size = min(0.5 + t * 3.0, 5.0);

    vec3 normal = (gl_Vertex.x * right + gl_Vertex.y * up);

    color = vec4(max(0.0, 0.6 + 0.4 * dot(light_dir.xyz,normalize(normal))));
    color.a *= min(8.0-t, 1.0) * param.y;

    p.xyz += normal * size;

    tc = gl_MultiTexCoord0.xy;
    p.w = 1.0;

    gl_Position = gl_ModelViewProjectionMatrix * p;
}

@sampler base_map "diffuse"

@fragment

uniform sampler2D base_map;

void main()
{
    gl_FragColor.rgb = color.rgb;
    gl_FragColor.a = color.a * texture2D(base_map, tc).a;
}
