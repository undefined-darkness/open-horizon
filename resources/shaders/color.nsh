@uniform color "color"

@vertex

void main()
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}

@fragment

uniform vec4 color;

void main()
{
    gl_FragColor = color;
}
