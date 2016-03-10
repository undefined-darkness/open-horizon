//
// open horizon -- undefined_darkness@outlook.com
//

#include "math/simd.h"

#pragma once

//------------------------------------------------------------

struct float4: public nya_math::simd_vec4
{
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
};

//------------------------------------------------------------

struct vec3_float4 //four vec3, optimized for simd
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
