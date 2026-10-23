uint hud_glyph(uint character) {
    switch (character) {
    case 48u:
        return 0x7B6Fu; // 0
    case 49u:
        return 0x749Au; // 1
    case 50u:
        return 0x73E7u; // 2
    case 51u:
        return 0x79E7u; // 3
    case 52u:
        return 0x49EDu; // 4
    case 53u:
        return 0x79CFu; // 5
    case 54u:
        return 0x7BCFu; // 6
    case 55u:
        return 0x24A7u; // 7
    case 56u:
        return 0x7BEFu; // 8
    case 57u:
        return 0x79EFu; // 9
    case 65u:
        return 0x5BEAu; // A
    case 67u:
        return 0x724Fu; // C
    case 69u:
        return 0x72CFu; // E
    case 70u:
        return 0x12CFu; // F
    case 71u:
        return 0x7B4Fu; // G
    case 77u:
        return 0x5BFDu; // M
    case 80u:
        return 0x12EBu; // P
    case 82u:
        return 0x5AEBu; // R
    case 83u:
        return 0x79CFu; // S
    case 85u:
        return 0x7B6Du; // U
    case 46u:
        return 0x2000u; // .
    case 45u:
        return 0x01C0u; // -
    default:
        return 0u; // Space
    }
}

// Seven characters: four integer digits, a dot, two decimals.
uint hud_number(float value, int column) {
    if (value < 0.0)
        return 45u; // Unavailable: dashes.

    if (column == 4)
        return 46u; // Decimal point.

    uint number = uint(round(clamp(value, 0.0, 9999.99) * 100.0));

    const uint divisors[6] = uint[6](100000u, 10000u, 1000u, 100u, 10u, 1u);

    int digit = column < 4 ? column : column - 1;
    uint divisor = divisors[digit];

    // Hide leading zeroes, retaining the units digit.
    if (column < 3 && number < divisor)
        return 32u;

    return 48u + (number / divisor) % 10u;
}

vec3 draw_hud(vec3 background, ivec2 pixel) {
    const int scale = 1;
    ivec2 p = pixel - ivec2(4);

    if (any(lessThan(p, ivec2(0))) || any(greaterThanEqual(p, ivec2(64, 28) * scale)))
        return background;

    p /= scale;

    int row = p.y / 7;
    int column = p.x / 4;

    // Five characters per label: FPS, FRAME, CPU, GPU.
    const uint labels[20] = uint[20](70u, 80u, 83u, 32u, 32u, 70u, 82u, 65u, 77u, 69u, 67u, 80u,
                                     85u, 32u, 32u, 71u, 80u, 85u, 32u, 32u);

    uint character = 32u;

    if (column < 5)
        character = labels[row * 5 + column];
    else if (column >= 6 && column <= 12)
        character = hud_number(frame.metrics[row], column - 6);
    else if (row > 0 && column == 14)
        character = 77u; // M
    else if (row > 0 && column == 15)
        character = 83u; // S

    ivec2 cell = ivec2(p.x % 4, p.y % 7);
    bool ink = false;

    if (cell.x < 3 && cell.y < 5) {
        uint bit = uint(cell.y * 3 + cell.x);
        ink = ((hud_glyph(character) >> bit) & 1u) != 0u;
    }

    vec3 panel = background * 0.2;
    return ink ? vec3(0.95, 0.9, 0.75) : panel;
}
