@vertex

void main()
{
    gl_Position = vec4(gl_Vertex.xy, 1.0, 1.0);
}

@fragment

void main()
{
    gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
