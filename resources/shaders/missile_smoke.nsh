@predefined camera_pos "nya camera pos"

@uniform pos "tr pos"
@uniform param "tr param"

@all

varying vec2 tc;
varying vec4 color;

@vertex

uniform vec4 pos[500];
uniform vec4 param;
uniform vec4 camera_pos;

void main()
{
    vec3 v = gl_Vertex.xyz;
    int idx = int(v.z);

    vec4 p = pos[idx];
    
    vec3 cam_dir = normalize(camera_pos.xyz - p.xyz);
    vec3 right = normalize(vec3(cam_dir.z, 0.0, -cam_dir.x));
    vec3 up = cross(cam_dir, right.xyz);

    float t = param.x - p.w;
    float size = min(0.5 + t * 3.0, 5.0);

    color = vec4(0.7);
    color.a *= min(8.0-t, 1.0) * param.y;

    p.xyz += (gl_Vertex.x * right + gl_Vertex.y * up) * size;

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
