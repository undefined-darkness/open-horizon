//
// open horizon -- undefined_darkness@outlook.com
//

#include "math/simd.h"

#pragma once

//------------------------------------------------------------

struct align16 float4 : public nya_math::simd_vec4
{
#ifdef SIMD_NEON
    float4 (): float4(vdupq_n_f32(0)) {}
    float4 (float32x4_t v): nya_math::simd_vec4 (v) {}
    float4 (float v): float4(vdupq_n_f32(v)) {}

    float4 operator - () const { return float4(vsubq_f32(vdupq_n_f32(0), xmm)); }

    float4 operator + (const float4 &v) const { return float4(vaddq_f32(xmm, v.xmm)); }
    float4 operator - (const float4 &v) const { return float4(vsubq_f32(xmm, v.xmm)); }
    float4 operator * (const float4 &v) const { return float4(vmulq_f32(xmm, v.xmm)); }

    float4 operator / (const float4 &v) const
	{
		const float32x4_t inv0 = vrecpeq_f32(v.xmm);
		const float32x4_t step = vrecpsq_f32(inv0, v.xmm);
		const float32x4_t inv1 = vmulq_f32(step, inv0);
		return float4(vmulq_f32(xmm, inv1));
	}

    float4 operator | (const float4 &v) const { return vreinterpretq_f32_s32(vorrq_s32(vreinterpretq_s32_f32(xmm), vreinterpretq_s32_f32(v.xmm))); }
    float4 operator < (const float4 &v) const { return float4(vreinterpretq_f32_u32(vcltq_f32((xmm), (v.xmm)))); }
    float4 operator <= (const float4 &v) const { return float4(vreinterpretq_f32_u32(vcleq_f32((xmm), (v.xmm)))); }

    bool is_zero_or_nan() const
    {
        const uint32x4_t v = vorrq_u32(vreinterpretq_u32_f32(xmm), vmvnq_u32(vceqq_f32(xmm, xmm)));
        const uint32x2_t t = vorr_u32(vget_low_u32(v), vget_high_u32(v));
        return vget_lane_u32(vpmax_u32(t, t), 0);
    }
#else
    float4 (): float4(_mm_set1_ps(0)) {}
    float4 (__m128 v): nya_math::simd_vec4 (v) {}
    float4 (float v): float4(_mm_set1_ps(v)) {}

    float4 operator - () const { return float4(_mm_xor_ps(xmm, _mm_set1_ps(-0.f))); }

    float4 operator + (const float4 &v) const { return float4(_mm_add_ps(xmm, v.xmm)); }
    float4 operator - (const float4 &v) const { return float4(_mm_sub_ps(xmm, v.xmm)); }
    float4 operator * (const float4 &v) const { return float4(_mm_mul_ps(xmm, v.xmm)); }
    float4 operator / (const float4 &v) const { return float4(_mm_div_ps(xmm, v.xmm)); }

    float4 operator | (const float4 &v) const { return float4(_mm_or_ps(xmm, v.xmm)); }
    float4 operator < (const float4 &v) const { return float4(_mm_cmplt_ps(xmm, v.xmm)); }
    float4 operator <= (const float4 &v) const { return float4(_mm_cmple_ps(xmm, v.xmm)); }

    bool is_zero_or_nan() const { const static float4 zero; return _mm_movemask_ps(_mm_cmpeq_ps(zero.xmm, xmm)) != 0; }
#endif
};

//------------------------------------------------------------

struct align16 vec3_float4 //four vec3, optimized for simd
{
    float4 x, y, z;

    vec3_float4() {}
    vec3_float4(const nya_math::vec3 &v): x(v.x), y(v.y), z(v.z) {}
    vec3_float4(const float4 &x,const float4 &y,const float4 &z): x(x), y(y), z(z) {}

    float4 dot(const vec3_float4 &v) const { return x * v.x + y * v.y + z * v.z; }
    vec3_float4 cross(const vec3_float4 &v) const { return vec3_float4(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x); }
    vec3_float4 operator - () const { return vec3_float4(-x, -y, -z); }
    vec3_float4 operator - (const vec3_float4 &v) const { return vec3_float4(x-v.x,y-v.y,z-v.z); }
};

//------------------------------------------------------------
