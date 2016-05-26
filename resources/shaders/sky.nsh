@sampler dith_map "dithering"
@predefined vp "nya viewport"

@all
varying vec4 color;
varying vec3 tc;

@vertex
uniform vec4 vp;

void main()
{
    vec4 pos = gl_ModelViewProjectionMatrix * gl_Vertex;
    tc.xy = vp.zw / vec2(64.0) * 0.5 * pos.xy;
    tc.z = pos.w;
    color = gl_Color;

    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    gl_Position.z = min(gl_Position.z / gl_Position.w, 1.0 - 2e-7) * gl_Position.w;
}

@fragment
uniform sampler2D dith_map;

void main()
{
    vec4 dith = texture2D(dith_map, tc.xy / tc.z);
    gl_FragColor = (color + dith*0.015) * 0.8;
}
