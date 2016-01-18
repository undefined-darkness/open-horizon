@predefined camera_pos "nya camera pos"

@uniform pos "tr pos"
@uniform dir "tr dir"

@all

varying vec2 tc;

@vertex

uniform vec4 pos[240];
uniform vec4 dir[240];

uniform vec4 camera_pos;

void main()
{
    vec2 v = gl_Vertex.xy;
    int idx = int(v.y);
    
    tc.y = dir[idx].w * 0.005;
    tc.x = (v.x * 0.5 + 0.5) * 0.25 + 0.25;
    
    vec3 p = pos[idx].xyz;
    vec3 diff = camera_pos.xyz - p;
    float width = 0.01 * sqrt(length(diff));
    p += normalize(cross(camera_pos.xyz - p, dir[idx].xyz)) * v.x * width;

    gl_Position = gl_ModelViewProjectionMatrix * vec4(p, 1.0);
}

@fragment

void main()
{
    gl_FragColor = vec4(vec3(1.0), 0.1);
}
