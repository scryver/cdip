typedef enum MouseBtn
{
    Mouse_Left   = 1 << 0,
    Mouse_Right  = 1 << 1,
    Mouse_Middle = 1 << 2,
} MouseBtn;

typedef struct Mouse
{
    v2i pos;
    u8  btnPress;
    u8  btnDown;
    u8  btnRelease;
    u8  reserved;
    i32 scroll;
} Mouse;

typedef struct UiContext
{
    DrawContext *draw;
    Mouse mouse;
    v2i clickPos;
} UiContext;

#define UI_BACKGROUND_COLOR    0xFF404040
#define UI_HIGHLIGHT_COLOR     0xFF606060
#define UI_INLAY_COLOR         0xFF202020
#define UI_DARKEN_COLOR        0xFF000000
#define UI_ACCENT_COLOR        0xFFE95A00
#define UI_ACCENT_DARK_COLOR   0xFFAA562F
#define UI_SHADOW_COLOR        0x80000000
#define UI_BORDER_COLOR        0xFFF0F0F0
#define UI_TEXT_COLOR          0xFF000000

func b32 ui_slider_vert(UiContext *ui, i32 x, i32 y, i32 w, i32 h, f32 *val, i32 lineCount, f32 *linePos, i32 zeroLineIdx)
{
    i32 radius = 15;
    i32 trackH = h - 2 * radius;
    RectI activeRect = rect_init(v2i_init(x, y + radius), v2i_init(w, trackH));
    b32 mouseOver = rect_is_inside(activeRect, ui->mouse.pos);
    if (mouseOver && (ui->mouse.btnPress & Mouse_Left)) {
        ui->clickPos = ui->mouse.pos;
    }
    b32 mouseClick = rect_is_inside(activeRect, ui->clickPos);
    if (mouseClick && ((ui->mouse.btnDown & Mouse_Left) == 0)) {
        ui->clickPos = v2i_init(0, 0);
        mouseClick = false;
    }
    if (mouseClick) {
        *val = (f32)(ui->mouse.pos.y - y) / (f32)trackH;
    } else if (mouseOver && ui->mouse.scroll) {
        f32 newVal = *val + 2.0f * (f32)-ui->mouse.scroll / (f32)trackH;
        *val = clamp_f32(0.0f, newVal, 1.0f);
    }

    draw_rect(ui->draw, x, y, w, h, UI_BACKGROUND_COLOR);

    // NOTE(michiel): Lines
    for (i32 lineIdx = 0; lineIdx < lineCount; ++lineIdx)
    {
        i32 lineX = x;
        i32 lineY = (i32)((f32)y + linePos[lineIdx] * (f32)trackH + 0.5f) + radius;
        i32 lineDeadzone = 3;
        if (lineIdx == zeroLineIdx) {
            draw_rect(ui->draw, lineX, lineY - 1, w, 3, UI_TEXT_COLOR);
            lineDeadzone *= 2;
        } else {
            draw_horz_line(ui->draw, lineX, lineY, w, UI_TEXT_COLOR);
        }

        if (mouseClick && ((lineY - lineDeadzone) <= ui->mouse.pos.y) && ((lineY + lineDeadzone) >= ui->mouse.pos.y)) {
            *val = (f32)(lineY - y) / (f32)trackH;
        }
    }

    // NOTE(michiel): Track
    draw_round_rect(ui->draw, x + (w - 11) / 2, y + 2 + radius, 11, h - 2 - 2 * radius, 5, UI_HIGHLIGHT_COLOR);
    draw_round_rect(ui->draw, x + (w - 9) / 2, y + radius, 9, h - 2 - 2 * radius, 4, UI_DARKEN_COLOR);
    draw_round_rect(ui->draw, x + (w - 7) / 2, y + 3 + radius, 7, h - 6 - 2 * radius, 3, UI_INLAY_COLOR);

    // NOTE(michiel): Handle
    i32 handleX = x + w / 2;
    i32 handleY = y + (i32)((f32)(h - 2 * radius) * (*val));
    draw_circ(ui->draw, handleX, handleY + 3, radius, UI_SHADOW_COLOR);
    draw_circ(ui->draw, handleX, handleY, radius, UI_BORDER_COLOR);
    draw_circ_grad(ui->draw, handleX, handleY, radius - 2, UI_ACCENT_COLOR, UI_ACCENT_DARK_COLOR);

    return false;
}
