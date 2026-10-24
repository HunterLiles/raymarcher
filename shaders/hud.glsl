#include "common.glsl"

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

uint hud_number(float value, int column) {
    if (value < 0.0)
        return 45u;

    if (column == 4)
        return 46u;

    uint number = uint(round(clamp(value, 0.0, 9999.99) * 100.0));

    const uint divisors[6] = uint[6](100000u, 10000u, 1000u, 100u, 10u, 1u);

    int digit = column < 4 ? column : column - 1;
    uint divisor = divisors[digit];

    if (column < 3 && number < divisor)
        return 32u;

    return 48u + (number / divisor) % 10u;
}

// Debug HUD
vec3 draw_debug_hud(vec3 background, ivec2 pixel) {
    const int scale = 1;
    ivec2 p = pixel - ivec2(4);

    if (any(lessThan(p, ivec2(0))) || any(greaterThanEqual(p, ivec2(64, 28) * scale)))
        return background;

    p /= scale;

    int row = p.y / 7;
    int column = p.x / 4;

    const uint labels[20] = uint[20](70u, 80u, 83u, 32u, 32u, 70u, 82u, 65u, 77u, 69u, 67u, 80u,
                                     85u, 32u, 32u, 71u, 80u, 85u, 32u, 32u);

    uint character = 32u;

    if (column < 5)
        character = labels[row * 5 + column];
    else if (column >= 6 && column <= 12)
        character = hud_number(game.metrics[row], column - 6);
    else if (row > 0 && column == 14)
        character = 77u;
    else if (row > 0 && column == 15)
        character = 83u;

    ivec2 cell = ivec2(p.x % 4, p.y % 7);
    bool ink = false;

    if (cell.x < 3 && cell.y < 5) {
        uint bit = uint(cell.y * 3 + cell.x);
        ink = ((hud_glyph(character) >> bit) & 1u) != 0u;
    }

    vec3 panel = background * 0.2;
    return ink ? vec3(0.95, 0.9, 0.75) : panel;
}

// Game HUD
vec3 hud_bar(vec3 background, ivec2 p, ivec2 size, float amount, vec3 fill) {

    if (any(lessThan(p, ivec2(0))) || any(greaterThanEqual(p, size)))
        return background;

    bool edge_x = p.x == 0 || p.x == size.x - 1;
    bool edge_y = p.y == 0 || p.y == size.y - 1;

    if (edge_x && edge_y)
        return background;

    if (edge_x || edge_y)
        return vec3(0.48, 0.39, 0.24);

    int filled = int(round(float(size.x - 4) * clamp(amount, 0.0, 1.0)));

    bool inside_fill = p.x >= 2 && p.x < 2 + filled && p.y >= 2 && p.y < size.y - 2;

    if (inside_fill)
        return fill * (p.y == 2 ? 1.2 : 1.0);

    return background * 0.15;
}

vec3 draw_hud(vec3 background, ivec2 pixel) {
    ivec2 screen = imageSize(image);
    ivec2 origin = ivec2(8, screen.y - 31);

    ivec2 offset = abs(pixel - screen / 2);
    bool crosshair = (offset.x <= 3 && offset.y == 0) || (offset.y <= 3 && offset.x == 0);

    if (crosshair)
        background = vec3(0.9);

    background =
        hud_bar(background, pixel - origin, ivec2(88, 7), game.health, vec3(0.65, 0.16, 0.12));

    background = hud_bar(background, pixel - origin - ivec2(0, 9), ivec2(72, 7), game.mana,
                         vec3(0.20, 0.35, 0.65));

    background = hud_bar(background, pixel - origin - ivec2(0, 18), ivec2(56, 5), game.stamina,
                         vec3(0.42, 0.52, 0.28));

    return background;
}
