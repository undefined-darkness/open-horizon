@all
varying vec2 tc;

@vertex
void main()
{
    vec4 pos=gl_Vertex; tc=pos.xy;
    gl_Position=vec4(vec2(-1.0,-1.0)+pos.xy*2.0,0.0,1.0);
}

@sampler base_map "base"
@sampler colx_map "colx"
@sampler coly_map "coly"
@uniform colors "colors"

@fragment
uniform sampler2D base_map;
uniform sampler2D colx_map;
uniform sampler2D coly_map;

uniform vec4 colors[6];

void main()
{
    vec4 base=texture2D(base_map,tc);
    vec4 colx=texture2D(colx_map,tc);
    vec4 coly=texture2D(coly_map,tc);

    vec4 col=mix(colors[2],colors[1],coly.a);
    col=mix(colors[3],col,colx.g);
    col=mix(colors[4],col,colx.b);
    col=mix(colors[5],col,colx.r);
    col=mix(col,colors[0],coly.g);
    col=mix(col,base,colx.a);

    gl_FragColor=vec4(col.rgb,base.a);
}
