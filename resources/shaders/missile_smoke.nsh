@predefined camera_pos "nya camera pos"

@uniform pos "tr pos"

@all

varying vec2 tc;

@vertex

uniform vec4 pos[500];

uniform vec4 camera_pos;

void main()
{
    vec3 v = gl_Vertex.xyz;
    int idx = int(v.z);

    float size = 3.0;

    vec4 p = pos[idx];
    
    vec3 cam_dir = normalize(camera_pos.xyz - p.xyz);
    vec3 right = normalize(vec3(cam_dir.z, 0.0, -cam_dir.x));
    vec3 up = cross(cam_dir, right.xyz);

    p.xyz += (gl_Vertex.x * right + gl_Vertex.y * up) * size;

    tc = gl_MultiTexCoord0.xy * 0.25;
    tc.y += 0.75;
    tc.x += p.w;

    p.w = 1.0;

    gl_Position = gl_ModelViewProjectionMatrix * p;
}

@sampler base_map "diffuse"

@fragment

uniform sampler2D base_map;

void main()
{
    gl_FragColor = vec4(vec3(0.7), texture2D(base_map, tc).a * 0.7);
}
