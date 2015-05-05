@uniform screen_radius "screen_radius"
@uniform saturation "saturation"
@uniform fade_color "fade_color"
@uniform damage_frame "damage_frame"
@uniform damage_frame_color "damage_frame_color"

@sampler base_map "diffuse"
@sampler heat_blur_map "heat_blur"
@sampler motion_blur_map "motion_blur"
@sampler lens_map "lens"
@sampler curve_map0 "curve0"
@sampler curve_map1 "curve1"
@sampler curve_map2 "curve2"

@all

varying vec2 tc;
varying vec2 tc_r;
varying float fov_y;

@vertex

uniform vec4 screen_radius;

void main()
{
    tc=gl_MultiTexCoord0.xy;
    tc_r = screen_radius.xy * gl_Vertex.xy;
    fov_y = clamp(gl_ProjectionMatrix[1][1], 0.0, 1.0);
    gl_Position = gl_Vertex;
}

@fragment

uniform vec4 fade_color;
uniform vec4 saturation;
uniform vec4 damage_frame;
uniform vec4 damage_frame_color;

uniform sampler2D base_map;
uniform sampler2D heat_blur_map;
uniform sampler2D motion_blur_map;
uniform sampler2D lens_map;
uniform sampler2D curve_map0;
uniform sampler2D curve_map1;
uniform sampler2D curve_map2;

void main()
{
    //main color
	vec4 color=texture2D(base_map, tc);

	//heat blur
    vec4 heat_vec = texture2D(heat_blur_map, tc);
	heat_vec.xy += vec2(-0.5);
	heat_vec.xy = heat_vec.xy * heat_vec.z + tc;
	color = mix(texture2D(base_map, heat_vec.xy), color, heat_vec.w);

	//motion blur
	vec4 motion_blur = texture2D(motion_blur_map, tc);
	color = mix(color, motion_blur, motion_blur.w);

	//lens
	vec4 lens_color = texture2D(lens_map, tc);
	color = clamp(lens_color + color, vec4(0.0),vec4(1.0));

    //vignetting
	float r = length(tc_r) - 1.0;
	r = clamp(r / fov_y, 0.0, 1.0);
	color *= 1.0 - r * r * (3.0 - 2.0 * r);
	color = max(color, vec4(0.0));

	//saturation
	float lum = dot(color.rgb, vec3(0.3, 0.587, 0.114));
	color = (1.0 + saturation.x) * (color - vec4(lum)) + vec4(lum);

	//color correction
	color.r = texture2D(curve_map0,color.ra).r;
	color.g = texture2D(curve_map1,color.ga).g;
	color.b = texture2D(curve_map2,color.ba).b;

	//damage frame
	r = length(damage_frame.xy * tc_r);
	r = clamp(r - damage_frame.w, 0.0, 1.0);
	r = clamp(r * damage_frame.z, 0.0, 1.0);
	color += r * damage_frame_color * damage_frame_color.a;
	color = clamp(color, vec4(0.0), vec4(1.0));

	//fade
    color += (fade_color - color)*fade_color.w;

    gl_FragColor = color;
}
