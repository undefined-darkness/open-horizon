@predefined camera_pos "nya camera pos"

@uniform pos "tr pos"
@uniform dir "tr dir"
@uniform param "tr param"

@all

varying vec2 tc;

@vertex

uniform vec4 pos[240];
uniform vec4 dir[240];
uniform vec4 param;
uniform vec4 camera_pos;

void main()
{
    vec2 v = gl_Vertex.xy;
    int idx = int(v.y);

    float fade = 5.0;
    tc.y = min(1.0 - (dir[idx].w - (param.x - fade)) / fade, 1.0);
    tc.x = v.x;

    vec3 p = pos[idx].xyz;
    vec3 diff = camera_pos.xyz - p;
    float width = 0.02 * sqrt(length(diff));
    p += normalize(cross(camera_pos.xyz - p, dir[idx].xyz)) * v.x * width;

    gl_Position = gl_ModelViewProjectionMatrix * vec4(p, 1.0);
}

@fragment

void main()
{
    float k = tc.x * tc.x;
    gl_FragColor = vec4(vec3(1.0), 0.05 * (1.0 - k * k) * tc.y);
}
