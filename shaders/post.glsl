vec3 psx_color(vec3 rgb, ivec2 pixel) {
    const float bayer[16] = float[16](0.0, 8.0, 2.0, 10.0, 12.0, 4.0, 14.0, 6.0, 3.0, 11.0, 1.0,
                                      9.0, 15.0, 7.0, 13.0, 5.0);

    int index = (pixel.y % 4) * 4 + (pixel.x % 4);
    float threshold = (bayer[index] + 0.5) / 16.0;

    const float levels = 32.0;
    rgb = clamp(rgb, 0.0, 1.0);

    return floor(rgb * (levels - 1.0) + threshold) / (levels - 1.0);
}
