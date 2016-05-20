@predefined bones_pos "nya bones pos transform"
@predefined bones_rot "nya bones rot transform"

@vertex

uniform vec3 bones_pos[250];
uniform vec4 bones_rot[250];

vec3 tr(vec3 v, vec4 q) { return v + cross(q.xyz, cross(q.xyz, v) + v * q.w) * 2.0; }

vec3 tr(vec3 pos, int idx) { return bones_pos[idx] + tr(pos, bones_rot[idx]); }

vec3 trn(vec3 normal, int idx) { return tr(normal, bones_rot[idx]); }

vec3 tr(vec3 v, vec4 idx, vec4 weight)
{
    vec3 r = tr(v, int(idx[0])) * weight[0];
    for (int i = 1; i < 4; ++i)
        r += tr(v, int(idx[i])) * weight[i];
    return r;
}

vec3 trn(vec3 v, vec4 idx, vec4 weight)
{
    vec3 r = trn(v, int(idx[0])) * weight[0];
    for (int i = 1; i < 4; ++i)
        r += trn(v, int(idx[i])) * weight[i];
    return r;
}
