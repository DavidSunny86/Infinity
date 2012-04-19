template<unsigned int mask>
__forceinline vec4 vi_swizzle(vec4 a)
{
	return _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(a), mask));
}

#ifdef USE_SSE4

template<unsigned int mask>
__forceinline vec4 vi_shuffle(vec4 a, vec4 b)
{
	vec4 t1 = vi_swizzle<mask&0xFF>(a);
	vec4 t2 = vi_swizzle<mask&0xFF>(b);
	return _mm_blend_ps(t1, t2, (mask>>8)&0xF);
}

#else

template<unsigned int mask>
__forceinline vec4 vi_shuffle(vec4 a, vec4 b)
{
	vec4 t1 = vi_swizzle<mask&0xFF>(a);
	vec4 t2 = vi_swizzle<mask&0xFF>(b);
	vec4 t3 = _mm_movelh_ps(t1, t2);
	vec4 t4 = _mm_movehl_ps(t2, t1);
	return _mm_shuffle_ps(t3, t4, (mask>>8)|0x44);
}

#endif

template<unsigned int mask>
__forceinline vec4 vi_select(vec4 a, vec4 b)
{
	return _mm_blend_ps(a, b, mask);
}

#ifdef USE_SSE4
__forceinline vec4 vi_select(vec4 mask, vec4 a, vec4 b)
{
	return _mm_blendv_ps(a, b, mask);
}
#else
//untested: http://realtimecollisiondetection.net/blog/?p=90
__forceinline vec4 vi_select(vec4 mask, vec4 a, vec4 b)
{
	return vi_xor(vi_and(vi_xor(a, b), mask), a);
}
#endif

__forceinline vec4 vi_set(float x, float y, float z, float w)
{
	return _mm_set_ps(w, z, y, x);
}

__forceinline vec4 vi_set_all(float x)
{
	return _mm_set_ps1(x);
}

__forceinline vec4 vi_load(float* v4)
{
	return _mm_load_ps(v4);
}

__forceinline vec4 vi_loadu(float* v4)
{
	return _mm_loadu_ps(v4);
}

__forceinline vec4 vi_load_zero()
{
	return _mm_setzero_ps();
}

__forceinline vec4 vi_load_one()
{
	return _mm_set_ps1(1.0f);
}

#ifdef USE_SSE4

__forceinline vec4 vi_dot4(vec4 a, vec4 b)
{
	return _mm_dp_ps(a, b, 0xFF);
}

#else

__forceinline vec4 vi_dot4(vec4 a, vec4 b)
{
	vec4 res   = _mm_mul_ps(a, b);
	vec4 shuff = vi_swizzle<VI_SWIZZLE_MASK(VI_Y, VI_X, VI_W, VI_Z)>(res);
	res = _mm_add_ps(res, shuff);
	shuff = vi_swizzle<VI_SWIZZLE_MASK(VI_Z, VI_W, VI_X, VI_Y)>(res);
	return _mm_add_ps(res, shuff);
}

__forceinline vec4 vi_dot3(vec4 a, vec4 b)
{
	vec4 res = _mm_mul_ps(a, b);
	vec4 shuff = vi_swizzle<VI_SWIZZLE_MASK(VI_Y, VI_X, VI_Z, VI_Z)>(res);
	vec4 res2 = _mm_add_ps(res, shuff);
	shuff = vi_swizzle<VI_SWIZZLE_MASK(VI_Z, VI_Z, VI_Z, VI_Z)>(res);
	res = _mm_add_ps(res2, shuff);
	return vi_swizzle<VI_SWIZZLE_XXXX>(res);
}

#endif

__forceinline vec4 vi_mad(vec4 a, vec4 b, vec4 c)
{
	return _mm_add_ps(_mm_mul_ps(a, b), c);
}

__forceinline vec4 vi_mul(vec4 a, vec4 b)
{
	return _mm_mul_ps(a, b);
}

__forceinline vec4 vi_add(vec4 a, vec4 b)
{
	return _mm_add_ps(a, b);
}

__forceinline vec4 vi_sub(vec4 a, vec4 b)
{
	return _mm_sub_ps(a, b);
}

__forceinline vec4 vi_neg(vec4 a)
{
	const union {unsigned int iv; float fv;} val = {0x80000000};
	return _mm_xor_ps(a, _mm_set1_ps(val.fv));
}

__forceinline vec4 vi_abs(vec4 a)
{
	const union {unsigned int iv; float fv;} val = {0x7FFFFFFF};
	return _mm_and_ps(a, _mm_set1_ps(val.fv));
}

__forceinline vec4 vi_sign(vec4 a)
{
	const union {unsigned int iv; float fv;} v1 = {0x3F800000};
	const union {unsigned int iv; float fv;} vs = {0x80000000};
	vec4 one = vi_and(vi_cmp_ne(a, vi_load_zero()), _mm_set1_ps(v1.fv));
	vec4 sign = vi_and(a, _mm_set1_ps(vs.fv));
	return vi_or(one, sign);
}

__forceinline vec4 vi_max(vec4 a, vec4 b)
{
	return _mm_max_ps(a, b);
}

__forceinline vec4 vi_min(vec4 a, vec4 b)
{
	return _mm_min_ps(a, b);
}

__forceinline vec4 vi_clamp(vec4 value, vec4 lower, vec4 upper)
{
	return vi_max(vi_min(value, upper), lower);
}

__forceinline vec4 vi_sat(vec4 value)
{
	return vi_clamp(value, _vi_c_zero, _vi_c_one);
}

__forceinline vec4 vi_rcp(vec4 a)
{
	return _mm_rcp_ps(a);
}

__forceinline vec4 vi_rsqrt(vec4 a)
{
	return _mm_rsqrt_ps(a);
}

__forceinline vec4 vi_sqrt(vec4 a)
{
	return _mm_sqrt_ps(a);
}

__forceinline vec4 vi_cmp_gt(vec4 a, vec4 b)
{
	return _mm_cmpgt_ps(a, b);
}

__forceinline vec4 vi_cmp_ge(vec4 a, vec4 b)
{
	return _mm_cmpge_ps(a, b);
}

__forceinline vec4 vi_cmp_lt(vec4 a, vec4 b)
{
	return _mm_cmplt_ps(a, b);
}

__forceinline vec4 vi_cmp_le(vec4 a, vec4 b)
{
	return _mm_cmple_ps(a, b);
}

__forceinline vec4 vi_cmp_eq(vec4 a, vec4 b)
{
	return _mm_cmpeq_ps(a, b);
}

__forceinline vec4 vi_cmp_ne(vec4 a, vec4 b)
{
	return _mm_cmpneq_ps(a, b);
}

__forceinline vec4 vi_xor(vec4 a, vec4 b)
{
	return _mm_xor_ps(a, b);
}

__forceinline vec4 vi_or(vec4 a, vec4 b)
{
	return _mm_or_ps(a, b);
}

__forceinline vec4 vi_and(vec4 a, vec4 b)
{
	return _mm_and_ps(a, b);
}

__forceinline vec4 vi_andnot(vec4 a, vec4 b)
{
	return _mm_andnot_ps(b, a);
}

__forceinline bool vi_all(vec4 a)
{
	return _mm_movemask_ps(a) == 0x0F;
}

__forceinline bool vi_any(vec4 a)
{
	return _mm_movemask_ps(a) != 0x00;
}

__forceinline vec4 vi_cvt_ubyte4_to_vec4(__int32 ub4)
{
	__m128i tmp, zero;

	//tmp.m128i_u32[0] = ub4;
	tmp  = _mm_cvtsi32_si128(ub4);
	zero = _mm_setzero_si128();
	tmp  = _mm_unpacklo_epi8(tmp, zero);
	tmp  = _mm_unpacklo_epi16(tmp, zero);

	vec4 res = _mm_cvtepi32_ps(tmp);
	
	res = _mm_div_ps(res, _mm_set_ps1(255.0f));

	return res;
}

//__forceinline vec4 vi_cvt_vec4_to_ubyte4(__int32 ub4)
//{
//}
