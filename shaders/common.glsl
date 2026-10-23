layout(local_size_x = 8, local_size_y = 8) in;
layout(set = 0, binding = 0, rgba8) uniform writeonly image2D image;

layout(push_constant) uniform Frame {
    vec4 camera_rot;
    vec4 camera_pos;
    vec2 resolution;
    float time;
}
frame;
