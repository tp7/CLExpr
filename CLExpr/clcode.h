#ifndef __EXPR_COMMON__
#define __EXPR_COMMON__


const char* common_ocl_functions = " \
uint clip_uint(float x) { \
    return convert_uint_sat(round(x)); \
} \
    \
int clip_int(float x) { \
    return convert_int_sat(round(x)); \
} \
 \
float interrogation(float x, float y, float z) { \
	return x > 0 ? y : z; \
} \
 \
float equal(float x, float y) { \
	return fabs(x - y) < 0.000001f ? 1.0f : -1.0f;  \
} \
 \
float notEqual(float x, float y) { \
	return fabs(x - y) >= 0.000001f ? 1.0f : -1.0f;  \
} \
 \
float inferior(float x, float y) { \
	return x <= y ? 1.0f : -1.0f;  \
} \
 \
float inferiorStrict(float x, float y) { \
	return x < y ? 1.0f : -1.0f;  \
} \
 \
float superior(float x, float y) { \
	return x >= y ? 1.0f : -1.0f;  \
} \
 \
float superiorStrict(float x, float y) { \
	return x > y ? 1.0f : -1.0f;  \
} \
 \
float mt_and(float x, float y) { \
	return ((x > 0) && (y > 0)) ? 1.0f : -1.0f;  \
} \
 \
float mt_or(float x, float y) { \
	return ((x > 0) || (y > 0)) ? 1.0f : -1.0f;  \
} \
 \
float mt_andNot(float x, float y) { \
	return ((x > 0) && (y <= 0)) ? 1.0f : -1.0f;  \
} \
 \
float mt_xor(float x, float y) { \
	return (((x > 0) && (y <= 0)) || ((x <= 0) && (y > 0))) ? 1.0f : -1.0f;  \
} \
 \
float andUB(float x, float y) { \
	return (float)(clip_uint(x) & clip_uint(y));  \
} \
 \
float orUB(float x, float y) { \
	return (float)(clip_uint(x) | clip_uint(y));  \
} \
 \
float xorUB(float x, float y) { \
	return (float)(clip_uint(x) ^ clip_uint(y));  \
} \
 \
float negateUB(float x) { \
	return (float)(~clip_uint(x));  \
} \
 \
float posshiftUB(float x, float y) { \
	return y >= 0 ? (float)(clip_uint(x) << clip_int(y)) : (float)(clip_uint(x) >> clip_int(-y));  \
} \
 \
float negshiftUB(float x, float y) { \
	return y >= 0 ? (float)(clip_uint(x) >> clip_int(y)) : (float)(clip_uint(x) << clip_int(-y));  \
} \
 \
float andSB(float x, float y) { \
	return (float)(clip_int(x) & clip_int(y));  \
} \
 \
float orSB(float x, float y) { \
	return (float)(clip_int(x) | clip_int(y));  \
} \
 \
float xorSB(float x, float y) { \
	return (float)(clip_int(x) ^ clip_int(y));  \
} \
 \
float negateSB(float x) { \
	return (float)(~clip_int(x));  \
} \
 \
float posshiftSB(float x, float y) { \
	return y >= 0 ? (float)(clip_int(x) << clip_int(y)) : (float)(clip_int(x) >> clip_int(-y));  \
} \
 \
float negshiftSB(float x, float y) { \
	return y >= 0 ? (float)(clip_int(x) >> clip_int(y)) : (float)(clip_int(x) << clip_int(-y));  \
} \
\
";


const char* expr_source = "__kernel void expr(__global uchar *dstp, __global uchar* srcp, int width) {    \
    int offset = get_global_id(0);   \
    float x = srcp[offset];   \
    dstp[offset] = (uchar)clamp((int)round({{expression}}), 0, 255);  \
}";

const char* exprxy_source = "__kernel void expr(__global uchar *dstp, __global uchar* srcp1, __global uchar* srcp2, int width) {    \
    int offset = get_global_id(0);   \
    float x = srcp1[offset];   \
    float y = srcp2[offset];   \
    dstp[offset] = (uchar)clamp((int)round({{expression}}), 0, 255);  \
}";

const char* exprxyz_source = "__kernel void expr(__global uchar *dstp, __global uchar* srcp1, __global uchar* srcp2, __global uchar* srcp3, int width) {    \
    int offset = get_global_id(0);   \
    float x = srcp1[offset];   \
    float y = srcp2[offset];   \
    float z = srcp3[offset];   \
    dstp[offset] = (uchar)clamp((int)round({{expression}}), 0, 255);  \
}";

const char* expr_source_lsb = "__kernel void expr(__global uchar *dstp, __global uchar* srcp, int width, int height) {      \
    int offset = get_global_id(0);    \
    float x = upsample(srcp[offset], srcp[offset+width*height]);    \
    ushort result = clamp((int)round({{expression}}), 0, 65535);   \
    dstp[offset] = (uchar)((result >> 8) & 0xFF);  \
    dstp[offset+width*height] = (uchar)(result & 0xFF); \
}";

const char* exprxy_source_lsb = "__kernel void expr(__global uchar *dstp, __global uchar* srcp1, __global uchar* srcp2, int width, int height) {    \
    int offset = get_global_id(0);   \
    float x = upsample(srcp1[offset], srcp1[offset+width*height]);   \
    float y = upsample(srcp2[offset], srcp2[offset+width*height]);   \
    ushort result = clamp((int)round({{expression}}), 0, 65535);   \
    dstp[offset] = (uchar)((result >> 8) & 0xFF);  \
    dstp[offset+width*height] = (uchar)(result & 0xFF); \
}";

const char* exprxyz_source_lsb = "__kernel void expr(__global uchar *dstp, __global uchar* srcp1, __global uchar* srcp2, __global uchar* srcp3, int width, int height) {    \
    int offset = get_global_id(0);   \
    float x = upsample(srcp1[offset], srcp1[offset+width*height]);   \
    float y = upsample(srcp2[offset], srcp2[offset+width*height]);   \
    float z = upsample(srcp3[offset], srcp3[offset+width*height]);   \
    ushort result = clamp((int)round({{expression}}), 0, 65535);   \
    dstp[offset] = (uchar)((result >> 8) & 0xFF);  \
    dstp[offset+width*height] = (uchar)(result & 0xFF); \
}";

#endif