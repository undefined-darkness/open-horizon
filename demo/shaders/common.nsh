@predefined camera_pos "nya camera position": local

@uniform fog_color "fog color"
@uniform fog_height "fog height"

@vertex

uniform vec4 camera_pos;

uniform vec4 fog_height;

float get_fogh(vec3 pos)
{
    return fog_height.y - pos.y;
}

float get_fogv(vec4 transformed_pos)
{
    return transformed_pos.z;
}

vec3 get_eye(vec3 pos)
{
    return normalize(camera_pos.xyz - pos.xyz);
}

@fragment

uniform vec4 fog_color;
uniform vec4 fog_height;

float get_fog(float fogf, float fogh, vec3 eye)
{
	float fog_h = clamp(pow(2.0, 1.44269502 * fog_height.w * fogh), 0.0, 1.0);
	float fog_f = clamp(pow(2.0, 1.44269502 * fog_height.z * fogf), 0.0, 1.0);
	float fog_hf = mix(fog_h, 1.0, fog_f);
	float f = 1.0 - fog_hf;

	float f2 = max(eye.y, 0.0);
	f2 -= 1.0 - fog_height.x;
	f += f2;
	if(f2 < 0.0) f = 0.0;
	f += fog_hf;

	float fog_p = pow(2.0, 1.44269502 * fog_color.w * fogf);
    return min(f, fog_p);
}
