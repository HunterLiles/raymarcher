#ifndef CACHE_GLSL
#define CACHE_GLSL

struct Cache {
    uint offset;
    uint count;
};

layout(set = 0, binding = 1, std430) buffer CacheStorage {
    uvec4 entries[];
}
cache_storage;

Cache cache_get(uint id) {
    uvec2 info = cache_storage.entries[id].xy;
    return Cache(info.x, info.y);
}

vec4 cache_read(Cache cache, uint index) {
    return uintBitsToFloat(cache_storage.entries[cache.offset + index]);
}

void cache_write(Cache cache, uint index, vec4 value) {
    cache_storage.entries[cache.offset + index] = floatBitsToUint(value);
}

uint engine_sdf_index(ivec3 p, ivec3 size) {
    return uint(p.x + size.x * (p.y + size.y * p.z));
}

vec3 engine_sdf_position(uint i, vec3 lower, vec3 upper, ivec3 size) {
    uint nx = uint(size.x), ny = uint(size.y);
    uvec3 p = uvec3(i % nx, (i / nx) % ny, i / (nx * ny));
    return mix(lower, upper, vec3(p) / vec3(size - 1));
}

float engine_sdf_sample(Cache cache, ivec3 p, ivec3 size) {
    return cache_read(cache, engine_sdf_index(p, size)).x;
}

float engine_sdf_lookup(uint id, vec3 p, vec3 lower, vec3 upper, ivec3 size) {
    vec3 q = clamp(p, lower, upper);
    vec3 u = (q - lower) / (upper - lower) * vec3(size - 1);
    ivec3 a = min(ivec3(floor(u)), size - 2);
    vec3 f = u - vec3(a);
    Cache cache = cache_get(id);

    float z0 = mix(mix(engine_sdf_sample(cache, a, size),
                       engine_sdf_sample(cache, a + ivec3(1, 0, 0), size), f.x),
                   mix(engine_sdf_sample(cache, a + ivec3(0, 1, 0), size),
                       engine_sdf_sample(cache, a + ivec3(1, 1, 0), size), f.x),
                   f.y);

    float z1 = mix(mix(engine_sdf_sample(cache, a + ivec3(0, 0, 1), size),
                       engine_sdf_sample(cache, a + ivec3(1, 0, 1), size), f.x),
                   mix(engine_sdf_sample(cache, a + ivec3(0, 1, 1), size),
                       engine_sdf_sample(cache, a + ivec3(1, 1, 1), size), f.x),
                   f.y);

    return (mix(z0, z1, f.z) + length(p - q)) * 0.57735026919;
}

#define CACHE_SDF(id, name, lower, upper, size, fps)                                               \
    float cached_##name(vec3 p, vec3 pos, float scale) {                                           \
        return engine_sdf_lookup(id, (p - pos) / scale, lower, upper, size) * scale;               \
    }

#endif
