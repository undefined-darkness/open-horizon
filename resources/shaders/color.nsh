@uniform color "color"

@all
varying float alph;

@vertex

void main()
{
    alph = gl_Vertex.a;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0);
}

@fragment

uniform vec4 color;

void main()
{
    gl_FragColor = vec4(color.rgb, color.a * alph);
}
