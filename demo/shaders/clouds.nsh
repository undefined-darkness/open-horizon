@all

varying vec2 texCoords;

@uniform tr "transform"

@vertex

uniform vec4 tr;

void main()
{
    vec4 pos=gl_Vertex;
    texCoords.xy = pos.xy;
    //gl_Position=vec4(vec2(-1.0,-1.0)+pos.xy*2.0,0.0,1.0);
    gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;
}

@predefined vp "nya viewport"

@fragment

uniform vec4 vp;

void main( void ) { gl_FragColor=vec4(1.0); }
