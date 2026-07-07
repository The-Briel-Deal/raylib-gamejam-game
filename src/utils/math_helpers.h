/*
On MurmurHash:
 -> Left number = input count
 -> Right number = output count
 -> Returns a f32 between 0 and 1
*/
#ifndef MATH_H
#define MATH_H

#include <stdint.h>
#include <string.h>
#include <glm/glm.hpp>

#include <math.h>

using namespace glm;

//MurmurHash -> https://www.shadertoy.com/view/ttc3zr?__cf_chl_f_tk=DnspPjnIRvqXbcVjlhGqkpp10bN.uUHQe3GAEA0qCFw-1783380466-1.0.1.1-ovr2qMcEsvsnOTvtLhZFcRksOrcDOltICVDgOAHFM0M
u32 murmurHash11(u32 src) {
    const u32 M = 0x5bd1e995u;
    u32 h = 1190494759u;
    src *= M; src ^= src >> 24u; src *= M;
    h *= M; h ^= src;
    h ^= h>>13u; h *= M; h ^= h>>15u;
    return h;
}

/// @brief One input, one output
/// @param src 
/// @return 
f32 hash11(f32 src) {
    u32 h = murmurHash11(floatBitsToUint(src));
    return uintBitsToFloat(h & 0x007fffffu | 0x3f800000u) - 1.0;
}

u32 murmurHash12(u32 x, u32 y) {
    const u32 M = 0x5bd1e995u;
    u32 h = 1190494759u;
    x *= M; y *= M;
    x ^= x >> 24u; y ^= y >> 24u;
    h *= M; h ^= x; h *= M; h ^= y;
    h ^= h >> 13u; h *= M; h ^= h >> 15u;
    return h;
}

/// @brief Two inputs, one output
/// @param x 
/// @param y 
/// @return 
f32 hash12(f32 x, f32 y) {
    u32 h = murmurHash12(floatBitsToUint(x), floatBitsToUint(y));
    return uintBitsToFloat(h & 0x007fffffu | 0x3f800000u) - 1.0;
}

u32 murmurHash13(u32 x, u32 y, u32 z) {
    const u32 M = 0x5bd1e995u;
    u32 h = 1190494759u;
    x *= M; y *= M; z *= M;
    x ^= x >> 24u; y ^= y >> 24u; z ^= z >> 24u;
    h *= M; h ^= x; h *= M; h ^= y; h *= M; h ^= z;
    h ^= h >> 13u; h *= M; h ^= h >> 15u;
    return h;
}

/// @brief Three inputs, one output
/// @param x 
/// @param y 
/// @param z 
/// @return 
f32 hash13(f32 x, f32 y, f32 z) {
    u32 h = murmurHash13(floatBitsToUint(x), floatBitsToUint(y), floatBitsToUint(z));
    return uintBitsToFloat(h & 0x007fffffu | 0x3f800000u) - 1.0;
}

u32vec2 murmurHash21(u32 src) {
    const u32 M = 0x5bd1e995u;

    u32vec2 h = u32vec2(1190494759u, 2147483647u);

    src *= M;
    src ^= src >> 24u;
    src *= M;

    h.x *= M; h.y *= M;

    h.x ^= src; h.y ^= src;

    h.x *= M; h.y *= M;

    h.x ^= h.x >> 13u; h.y ^= h.y >> 13u;

    h.x *= M; h.y *= M;

    h.x ^= h.x >> 15u; h.y ^= h.y >> 15u;

    return h;
}

/// @brief One input two outputs
/// @param src 
/// @return 
f32vec2 hash21(f32 src) {
    u32vec2 h = murmurHash21(floatBitsToUint(src));
    h.x = (h.x & 0x007fffffu | 0x3f800000u);
    h.y = (h.y & 0x007fffffu | 0x3f800000u);
    return uintBitsToFloat(h & 0x007fffffu | 0x3f800000u) - 1.0f;
}

u32vec2 murmurHash22(u32vec2 src) {
    const u32 M = 0x5bd1e995u;

    u32vec2 h = u32vec2(1190494759u, 2147483647u);

    src.x *= M; src.y *= M;

    src.x ^= src.x >> 24u; src.y ^= src.y >> 24u;

    src.x *= M; src.y *= M;

    h.x *= M; h.y *= M;

    h.x ^= src.x; h.y ^= src.x;

    h.x *= M; h.y *= M;

    h.x ^= src.y; h.y ^= src.y;

    h.x ^= h.x >> 13u; h.y ^= h.y >> 13u;

    h.x *= M; h.y *= M;

    h.x ^= h.x >> 15u; h.y ^= h.y >> 15u;

    return h;
}

/// @brief two inputs two outputs
f32vec2 hash22(f32 x, f32 y) {
    u32vec2 h = murmurHash22(u32vec2(floatBitsToUint(x), floatBitsToUint(y)));
    h.x = (h.x & 0x007fffffu | 0x3f800000u);
    h.y = (h.y & 0x007fffffu | 0x3f800000u);
    return uintBitsToFloat(h & 0x007fffffu | 0x3f800000u) - 1.0f;
}

u32vec2 murmurHash23(u32 src_x, u32 src_y, u32 src_z) {
    const u32 M = 0x5bd1e995u;

    u32vec2 h = u32vec2(1190494759u, 2147483647u);

    src_x *= M; src_y *= M; src_z *= M;

    src_x ^= src_x >> 24u; src_y ^= src_y >> 24u; src_z ^= src_z >> 24u;

    src_x *= M; src_y *= M; src_z *= M;

    h.x *= M; h.y *= M;

    h.x ^= src_x; h.y ^= src_x;

    h.x *= M; h.y *= M;

    h.x ^= src_y; h.y ^= src_y;

    h.x *= M; h.y *= M;

    h.x ^= src_z; h.y ^= src_z;

    h.x ^= h.x >> 13u; h.y ^= h.y >> 13u;

    h.x *= M; h.y *= M;

    h.x ^= h.x >> 15u; h.y ^= h.y >> 15u;

    return h;
}

/// @brief three inputs two outputs
f32vec2 hash23(f32 x, f32 y, f32 z) {
    u32vec2 h = murmurHash23(floatBitsToUint(x), floatBitsToUint(y), floatBitsToUint(z));
    h.x = (h.x & 0x007fffffu | 0x3f800000u);
    h.y = (h.y & 0x007fffffu | 0x3f800000u);
    return uintBitsToFloat(h & 0x007fffffu | 0x3f800000u) - 1.0f;
}

uvec3 murmurHash33(uvec3 src) {
    const uint M = 0x5bd1e995u;
    uvec3 h = uvec3(1190494759u, 2147483647u, 3559788179u);
    src *= M; src ^= src>>24u; src *= M;
    h *= M; h ^= src.x; h *= M; h ^= src.y; h *= M; h ^= src.z;
    h ^= h>>13u; h *= M; h ^= h>>15u;
    return h;
}

/// @brief 3 outputs 3 inputs
/// @param src 
/// @return 
vec3 hash33(vec3 src) {
    uvec3 h = murmurHash33(floatBitsToUint(src));
    return uintBitsToFloat(h & 0x007fffffu | 0x3f800000u) - 1.0f;
}

/// Noise: https://www.shadertoy.com/view/4sc3z2

float perlin_noise(vec3 p)
{
    vec3 pi = floor(p);
    vec3 pf = p - pi;
    
    vec3 w = pf * pf * (3.0f - 2.0f * pf);
    
    return 	mix(
        		mix(
                	mix(dot(pf - vec3(0, 0, 0), hash33(pi + vec3(0, 0, 0))), 
                        dot(pf - vec3(1, 0, 0), hash33(pi + vec3(1, 0, 0))),
                       	w.x),
                	mix(dot(pf - vec3(0, 0, 1), hash33(pi + vec3(0, 0, 1))), 
                        dot(pf - vec3(1, 0, 1), hash33(pi + vec3(1, 0, 1))),
                       	w.x),
                	w.z),
        		mix(
                    mix(dot(pf - vec3(0, 1, 0), hash33(pi + vec3(0, 1, 0))), 
                        dot(pf - vec3(1, 1, 0), hash33(pi + vec3(1, 1, 0))),
                       	w.x),
                   	mix(dot(pf - vec3(0, 1, 1), hash33(pi + vec3(0, 1, 1))), 
                        dot(pf - vec3(1, 1, 1), hash33(pi + vec3(1, 1, 1))),
                       	w.x),
                	w.z),
    			w.y);
}

#endif