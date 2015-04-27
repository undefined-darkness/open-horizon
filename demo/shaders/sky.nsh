@sampler base_map "diffuse"
@sampler dith_map "dithering"
@uniform fog_color "fog color"
@predefined vp "nya viewport"

@all
varying vec4 color;
varying vec3 tc;

@vertex
uniform samplerCube base_map;
uniform vec4 fog_color;
uniform vec4 vp;

void main()
{
    vec3 ctc = gl_Vertex.xyz;
    ctc.z = -ctc.z;
    color = textureCube(base_map, ctc);

    if(gl_Vertex.y<0.5)
        color = fog_color;

    vec4 pos = gl_ModelViewProjectionMatrix * gl_Vertex;
    tc.xy = vp.zw / vec2(64.0) * 0.5 * pos.xy;
    tc.z = pos.w;

    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}

@fragment
uniform sampler2D dith_map;

void main()
{
    vec4 dith = texture2D(dith_map, tc.xy / tc.z);
    gl_FragColor = (color + dith*0.015) * 0.8;
}
