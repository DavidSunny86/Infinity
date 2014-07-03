#include "ml.h"

namespace ml
{
#define MUL_MAT4_VEC4(r, m0, m1, m2, m3, v)         \
    {                                               \
        v128 swizzle;                               \
                                                    \
        swizzle = vi_swizzle<VI_SWIZZLE_WWWW>(v);   \
        r = vi_mul(swizzle, m3);                    \
        swizzle = vi_swizzle<VI_SWIZZLE_ZZZZ>(v);   \
        r = vi_mad(swizzle, m2, r);                 \
        swizzle = vi_swizzle<VI_SWIZZLE_YYYY>(v);   \
        r = vi_mad(swizzle, m1, r);                 \
        swizzle = vi_swizzle<VI_SWIZZLE_XXXX>(v);   \
        r = vi_mad(swizzle, m0, r);                 \
    }                                               \


    //                                                  | w0  -z0   y0  x0|   |x1|   | w1   z1  -y1  x1|   |x0|
    //                                                  |                 |   |  |   |                 |   |  |
    //                                                  | z0   w0  -x0  y0|   |y1|   |-z1   w1   x1  y1|   |y0|
    //  q0 * q1 = (x0, y0, z0, w0) * (x1, y1, z1, w1) = |                 | * |  | = |                 | * |  |
    //                                                  |-y0   x0   w0  z0|   |z1|   | y1  -x1   w1  z1|   |z0|
    //                                                  |                 |   |  |   |                 |   |  |
    //                                                  |-x0  -y0  -z0  w0|   |w1|   |-x1  -y1  -z1  w1|   |w0|
    v128 mul_quat(v128 q0, v128 q1)
    {
        v128 r, m, s, m0, m1, m2;

        m  = vi_set_i000(FLT_SIGN);

        s  = vi_swizzle<VI_SWIZZLE_MASK(1, 1, 0, 0)>(m);
        m0 = vi_swizzle<VI_SWIZZLE_MASK(3, 2, 1, 0)>(q0);
        m0 = vi_xor(s, m0);  

        s  = vi_swizzle<VI_SWIZZLE_MASK(0, 1, 1, 0)>(m);
        m1 = vi_swizzle<VI_SWIZZLE_MASK(2, 3, 0, 1)>(q0);
        m1 = vi_xor(s, m1);

        s  = vi_swizzle<VI_SWIZZLE_MASK(1, 0, 1, 0)>(m);
        m2 = vi_swizzle<VI_SWIZZLE_MASK(1, 0, 3, 2)>(q0);
        m2 = vi_xor(s, m2);

        MUL_MAT4_VEC4(r, m0, m1, m2, q0, q1);

        return r;
    }

    v128 rotate_vec3_quat(v128 q, v128 v)
    {
        v128 r, two, wx4;

        r   = vi_cross3(q, v);
        wx4 = vi_swizzle<VI_SWIZZLE_WWWW>(q);
        r   = vi_mad(wx4, v, r);
        r   = vi_cross3(q, r);
        two = vi_set_ffff(2.0f);
        r   = vi_mad(two, r, v);

        return r;
    }

    v128 conjugate_quat(v128 q)
    {
        v128 mask, r;
        
        mask = vi_set_i000(FLT_SIGN);
        mask = vi_swizzle<VI_SWIZZLE_MASK(0, 0, 0, 3)>(mask);
        r    = vi_xor(mask, q);

        return r;
    }

    v128 lerp_quat(v128 q0, v128 q1, float t)
    {
        v128 r0;
        
        r0 = vi_dot4(q0, q1);

        if (vi_cmpx_lt(r0, vi_set_0000()))
        {
            q1 = vi_neg(q1);
        }

        return vi_lerp(q0, q1, t);
    }

    v128 normalize(v128 v)
    {
        v128 r0, r1;

        r0 = vi_dot4(v, v);

        if (vi_cmpx_lt(r0, vi_set_ffff(VI_EPS7)))
        {
            r0 = vi_set_0000();
        }
        else
        {
            r1 = vi_sqrt(r0);
            r0 = vi_div(v, r1);
        }

        return r0;
    }

    v128 length(v128 v)
    {
        return vi_sqrt(vi_dot4(v, v));

    }

    v128 make_p3(v128 v)
    {
        return vi_xor(make_v3(v), vi_set_0001f());
    }

    v128 make_v3(v128 v)
    {
        v128 mask = vi_swizzle<VI_SWIZZLE_MASK(0, 0, 0, 3)>(vi_set_i000(0xFFFFFFFF));
        return vi_and(v, mask);
    }

    void mul_quat(quat* result, quat* a, quat* b)
    {
        v128 r, q0, q1;

        q0 = vi_loadu_v4(a);
        q1 = vi_loadu_v4(b);

        r = mul_quat(q0, q1);

        vi_storeu_v4(result, r);
    }

    void conjugate_quat(quat* result, quat* q)
    {
        v128 r, vq;

        vq   = vi_loadu_v4(q);

        r = conjugate_quat(vq);

        vi_storeu_v4(result, r);
    }

    void rotate_vec3_quat(vec3* result, quat* rot, vec3* pos)
    {
        v128 r, q, p;

        q = vi_loadu_v4(rot);
        p = vi_load_v3(pos);

        r = rotate_vec3_quat(q, p);

        vi_store_v3(result, r);
    }

    v128 translation_dual_quat(v128 rq, v128 dq)
    {
        v128 t, w;

        t = vi_cross3(rq, dq);
        w = vi_swizzle<VI_SWIZZLE_WWWW>(rq);
        t = vi_mad(w, dq, t);
        w = vi_swizzle<VI_SWIZZLE_WWWW>(dq);
        t = vi_sub(t, vi_mul(w, rq));
        t = vi_mul(vi_set_ffff(2.0f), t);

        return t;
    }

    void set_identity_dual_quat(dual_quat* dq)
    {
        vi_storeu_v4(&dq->real, vi_set_0001f());
        vi_storeu_v4(&dq->dual, vi_set_0000());
    }

#define COPY_QUAT(dst, src)                     \
    {                                           \
        v128 t = vi_loadu_v4(src);              \
        vi_storeu_v4(dst, t);                   \
    }

    void create_dual_quat(dual_quat* result, quat* orient, vec3* offset)
    {
        v128 r, c, q, p;

        COPY_QUAT(&result->real, orient);

        p = vi_load_v3(offset);
        q = vi_loadu_v4(orient);

        r = mul_quat(p, q);
        c = vi_set_ffff(0.5f);
        vi_storeu_v4(&result->dual, vi_mul(c, r));
    }

    void transform_vec3_dual_quat(vec3* result, dual_quat* dq, vec3* p)
    {
        v128 v, t, r, d;

        v = vi_load_v3(p);
        r = vi_loadu_v4(&dq->real);
        d = vi_loadu_v4(&dq->dual);

        v = rotate_vec3_quat(r, v);
        t = translation_dual_quat(r, d);

        t = vi_add(v, t);
        
        vi_store_v3(result, t);
    }

    void mul_dual_quat(dual_quat* result, dual_quat* dq0, dual_quat* dq1)
    {
        v128 qr0, qd0;
        v128 qr1, qd1;
        v128 r0, r1;

        qr0 = vi_loadu_v4(&dq0->real);
        qd0 = vi_loadu_v4(&dq0->dual);
        qr1 = vi_loadu_v4(&dq1->real);
        qd1 = vi_loadu_v4(&dq1->dual);

        r0 = mul_quat(qr0, qr1);

        vi_storeu_v4(&result->real, r0);

        r0 = mul_quat(qd0, qr1);
        r1 = mul_quat(qr0, qd1);
        r0 = vi_add(r0, r1);

        vi_storeu_v4(&result->dual, r0);
    }

    void conjugate_dual_quat(dual_quat* result, dual_quat* dq)
    {
        v128 r, q;

        q = vi_loadu_v4(&dq->real);
        r = conjugate_quat(q);
        vi_storeu_v4(&result->real, r);

        q = vi_loadu_v4(&dq->dual);
        r = conjugate_quat(q);
        vi_storeu_v4(&result->dual, r);
    }

    void get_translation_from_dual_quat(vec3* result, dual_quat* dq)
    {
        v128 t, r, d;

        r = vi_loadu_v4(&dq->real);
        d = vi_loadu_v4(&dq->dual);

        t = translation_dual_quat(r, d);

        vi_store_v3(result, t);
    }

    void transpose_mat4(v128* t/*[4]*/, v128* m/*[4]*/)
    {
        __m128i xmm0 = _mm_unpacklo_epi32(_mm_castps_si128(m[0]), _mm_castps_si128(m[1]));
        __m128i xmm1 = _mm_unpackhi_epi32(_mm_castps_si128(m[0]), _mm_castps_si128(m[1]));
        __m128i xmm2 = _mm_unpacklo_epi32(_mm_castps_si128(m[2]), _mm_castps_si128(m[3]));
        __m128i xmm3 = _mm_unpackhi_epi32(_mm_castps_si128(m[2]), _mm_castps_si128(m[3]));

        t[0] = _mm_castsi128_ps(_mm_unpacklo_epi64(xmm0, xmm2));
        t[1] = _mm_castsi128_ps(_mm_unpackhi_epi64(xmm0, xmm2));
        t[2] = _mm_castsi128_ps(_mm_unpacklo_epi64(xmm1, xmm3));
        t[3] = _mm_castsi128_ps(_mm_unpackhi_epi64(xmm1, xmm3));
    }

    v128 mul_mat4_vec4(v128* m/*4*/, v128 v)
    {
        v128 res;
        MUL_MAT4_VEC4(res, m[0], m[1], m[2], m[3], v);

        return res;
    }

    void mul_mat4(v128* r/*4*/, v128* a/*4*/, v128* b/*4*/)
    {
        v128 res;
        MUL_MAT4_VEC4(res, a[0], a[1], a[2], a[3], b[0]);
        r[0] = res;
        MUL_MAT4_VEC4(res, a[0], a[1], a[2], a[3], b[1]);
        r[1] = res;
        MUL_MAT4_VEC4(res, a[0], a[1], a[2], a[3], b[2]);
        r[2] = res;
        MUL_MAT4_VEC4(res, a[0], a[1], a[2], a[3], b[3]);
        r[3] = res;
    }

    //  | a11   a12   a13  a14|   | w -z  y  x|   | w -z  y -x|
    //  |                     |   |           |   |           |
    //  | a21   a22   a23  a24|   | z  w -x  y|   | z  w -x -y|
    //  |                     | = |           | * |           |
    //  | a31   a32   a33  a34|   |-y  x  w  z|   |-y  x  w -z|
    //  |                     |   |           |   |           |
    //  | a41   a42   a43  a44|   |-x -y -z  w|   | x  y  z  w|
    void quat_to_mat4x3(v128* mat/*[3]*/, v128 q)
    {
        v128 r, m, s, m0, m1, m2, v;

        m  = vi_set_i000(FLT_SIGN);

        s  = vi_swizzle<VI_SWIZZLE_MASK(1, 1, 0, 0)>(m);
        m0 = vi_swizzle<VI_SWIZZLE_MASK(3, 2, 1, 0)>(q);
        m0 = vi_xor(s, m0);  

        s  = vi_swizzle<VI_SWIZZLE_MASK(0, 1, 1, 0)>(m);
        m1 = vi_swizzle<VI_SWIZZLE_MASK(2, 3, 0, 1)>(q);
        m1 = vi_xor(s, m1);

        s  = vi_swizzle<VI_SWIZZLE_MASK(1, 0, 1, 0)>(m);
        m2 = vi_swizzle<VI_SWIZZLE_MASK(1, 0, 3, 2)>(q);
        m2 = vi_xor(s, m2);

        s = vi_swizzle<VI_SWIZZLE_MASK(1, 1, 1, 0)>(m);
        v = vi_xor(m0, s);
        MUL_MAT4_VEC4(r, m0, m1, m2, q, v);
        mat[0] = r;

        s = vi_swizzle<VI_SWIZZLE_MASK(1, 1, 1, 0)>(m);
        v = vi_xor(m1, s);
        MUL_MAT4_VEC4(r, m0, m1, m2, q, v);
        mat[1] = r;

        s = vi_swizzle<VI_SWIZZLE_MASK(1, 1, 1, 0)>(m);
        v = vi_xor(m2, s);
        MUL_MAT4_VEC4(r, m0, m1, m2, q, v);
        mat[2] = r;
    }

    void quat_to_mat4(v128* m/*[4]*/, v128 q)
    {
        quat_to_mat4x3(m, q);
        m[3] = vi_set_0001f();
    }

    //  q.x = sign(m21 - m12) * 0.5 * sqrt( max( 0, 1 + m00 - m11 - m22 ) );
    //  q.y = sign(m02 - m20) * 0.5 * sqrt( max( 0, 1 - m00 + m11 - m22 ) );
    //  q.z = sign(m10 - m01) * 0.5 * sqrt( max( 0, 1 - m00 - m11 + m22 ) );
    //  q.w =                   0.5 * sqrt( max( 0, 1 + m00 + m11 + m22 ) );
    v128 mat4x3_to_quat(v128* m/*[4]*/)
    {
        v128 s, q, sc, m00, m11, m22, ml, mr, cmp;

        s = vi_set_i000(FLT_SIGN);

        q  = vi_set_iiii(FLT_1_0_ASINT);

        sc  = vi_swizzle<VI_SWIZZLE_MASK(1, 0, 0, 1)>(s);
        m00 = vi_set_ffff(m[0].m128_f32[0]);
        m00 = vi_xor(m00, sc);
        q   = vi_add(m00, q);

        sc  = vi_swizzle<VI_SWIZZLE_MASK(0, 1, 0, 1)>(s);
        m11 = vi_set_ffff(m[1].m128_f32[1]);
        m11 = vi_xor(m11, sc);
        q   = vi_add(m11, q);

        sc  = vi_swizzle<VI_SWIZZLE_MASK(0, 0, 1, 1)>(s);
        m22 = vi_set_ffff(m[2].m128_f32[2]);
        m22 = vi_xor(m22, sc);
        q   = vi_add(m22, q);

        q = vi_mul(vi_set_ffff(0.5), vi_sqrt(q));

        ml = vi_set(m[1].m128_f32[2], m[2].m128_f32[0], m[0].m128_f32[1], 0.0f);
        mr = vi_set(m[2].m128_f32[1], m[0].m128_f32[2], m[1].m128_f32[0], 0.0f);

        cmp = vi_cmp_lt(ml, mr);
        sc  = vi_and(cmp, vi_set_iiii(FLT_SIGN));

        q = vi_or(sc, q);

        return q;
    }

    v128 axis_angle_to_quat(v128 axis, float angle)
    {
        float a  = angle*0.5f;
        v128  co;
        v128  si;

        co = vi_set_f000(cosf(a));
        si = vi_set_f000(sinf(a));

        co = vi_swizzle<VI_SWIZZLE_MASK(1, 1, 1, 0)>(co);
        si = vi_swizzle<VI_SWIZZLE_MASK(0, 0, 0, 1)>(si);

        return vi_xor(vi_mul(axis, si), co);
    }
}

#include <math.h>

// Scalar math functions
namespace ml
{
    int32_t asint(float f)
    {
        return _mm_cvtsi128_si32(_mm_castps_si128(_mm_set_ss(f)));
    }

    float asfloat(int32_t i)
    {
        return _mm_cvtss_f32(_mm_castsi128_ps(_mm_cvtsi32_si128(i)));
    }

    float absf(float x)
    {
        return asfloat(asint(x) & 0x7FFFFFFF);
    }

    float sqrtf(float x)
    {
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(x)));
    }

    float fmodf(float x, float y)
    {
        return ::fmodf(x, y);
    }

    float floorf(float x)
    {
        return ::floorf(x);
    }

    float ceilf(float x)
    {
        return ::ceilf(x);
    }

    float sinf(float x)
    {
        return ::sinf(x);
    }

    float cosf(float x)
    {
        return ::cosf(x);
    }

    float asinf(float x)
    {
        return ::asinf(x);
    }

    float atan2(float y, float x)
    {
        return ::atan2(y, x);
    }
}
