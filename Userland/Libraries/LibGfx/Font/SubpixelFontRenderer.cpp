/*
 * Copyright (c) 2023, SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Math.h>
#include <LibGfx/AntiAliasingPainter.h>
#include <LibGfx/Font/SubpixelFontRenderer.h>
#include <LibGfx/Painter.h>

namespace Gfx {

SubpixelFontRenderer& SubpixelFontRenderer::the()
{
    static SubpixelFontRenderer s_the;
    return s_the;
}

RefPtr<Bitmap> SubpixelFontRenderer::render_glyph_with_subpixel(
    Path const& glyph_path,
    IntSize glyph_size,
    FontRenderingSettings const& settings)
{
    dbgln("SubpixelFontRenderer::render_glyph_with_subpixel called with size={}x{}, subpixel_order={}, quality={}", 
          glyph_size.width(), glyph_size.height(), (int)settings.subpixel_order, (int)settings.quality);
    
    if (glyph_size.width() == 0 || glyph_size.height() == 0)
        return {};

    switch (settings.subpixel_order) {
    case SubpixelOrder::None:
        dbgln("Using grayscale rendering with quality level {}", (int)settings.quality);
        return render_grayscale(glyph_path, glyph_size, settings);
    case SubpixelOrder::RGB:
    case SubpixelOrder::BGR:
        dbgln("Using horizontal subpixel rendering (order={}) with quality level {}", (int)settings.subpixel_order, (int)settings.quality);
        return render_subpixel_horizontal(glyph_path, glyph_size, settings.subpixel_order, settings);
    case SubpixelOrder::VRGB:
    case SubpixelOrder::VBGR:
        dbgln("Using vertical subpixel rendering (order={}) with quality level {}", (int)settings.subpixel_order, (int)settings.quality);
        return render_subpixel_vertical(glyph_path, glyph_size, settings.subpixel_order, settings);
    }
    
    dbgln("Falling back to grayscale rendering");
    return render_grayscale(glyph_path, glyph_size, settings);
}

RefPtr<Bitmap> SubpixelFontRenderer::render_grayscale(Path const& path, IntSize size, FontRenderingSettings const& settings)
{
    auto bitmap = Bitmap::create(BitmapFormat::BGRA8888, size);
    if (bitmap.is_error())
        return {};
    
    auto result_bitmap = bitmap.release_value();
    Painter painter(*result_bitmap);
    AntiAliasingPainter aa_painter(painter);
    
    // Use different quality levels based on settings
    switch (settings.quality) {
    case FontRenderingSettings::Quality::Fast:
        dbgln("Using Sample8xAA for Fast quality");
        aa_painter.fill_path<Sample8xAA>(path, Color::White);
        break;
    case FontRenderingSettings::Quality::Good:
        dbgln("Using Sample16xAA for Good quality");
        aa_painter.fill_path<Sample16xAA>(path, Color::White);
        break;
    case FontRenderingSettings::Quality::Best:
        dbgln("Using Sample32xAA for Best quality");
        aa_painter.fill_path<Sample32xAA>(path, Color::White);
        break;
    }
    
    return result_bitmap;
}

RefPtr<Bitmap> SubpixelFontRenderer::render_subpixel_horizontal(Path const& path, IntSize size, SubpixelOrder order, FontRenderingSettings const& settings)
{
    (void)order; // TODO: Use subpixel order when implementing true subpixel rendering
    
    // For now, use standard rendering with quality-based sampling
    // TODO: Implement true subpixel rendering (3x wider bitmap, RGB sampling)
    auto bitmap = Bitmap::create(BitmapFormat::BGRA8888, size);
    if (bitmap.is_error())
        return {};
    
    auto result_bitmap = bitmap.release_value();
    Painter painter(*result_bitmap);
    AntiAliasingPainter aa_painter(painter);
    
    // Use different quality levels - subpixel rendering should look better than grayscale
    switch (settings.quality) {
    case FontRenderingSettings::Quality::Fast:
        dbgln("Subpixel horizontal: Using Sample16xAA for Fast quality (better than grayscale)");
        aa_painter.fill_path<Sample16xAA>(path, Color::White);
        break;
    case FontRenderingSettings::Quality::Good:
        dbgln("Subpixel horizontal: Using Sample32xAA for Good quality");
        aa_painter.fill_path<Sample32xAA>(path, Color::White);
        break;
    case FontRenderingSettings::Quality::Best:
        dbgln("Subpixel horizontal: Using Sample32xAA for Best quality");
        aa_painter.fill_path<Sample32xAA>(path, Color::White);
        break;
    }
    
    return result_bitmap;
}

RefPtr<Bitmap> SubpixelFontRenderer::render_subpixel_vertical(Path const& path, IntSize size, SubpixelOrder order, FontRenderingSettings const& settings)
{
    (void)order; // TODO: Use subpixel order when implementing true subpixel rendering
    
    // For now, use standard rendering with quality-based sampling
    // TODO: Implement true vertical subpixel rendering
    auto bitmap = Bitmap::create(BitmapFormat::BGRA8888, size);
    if (bitmap.is_error())
        return {};
    
    auto result_bitmap = bitmap.release_value();
    Painter painter(*result_bitmap);
    AntiAliasingPainter aa_painter(painter);
    
    // Use different quality levels for vertical subpixel
    switch (settings.quality) {
    case FontRenderingSettings::Quality::Fast:
        dbgln("Subpixel vertical: Using Sample16xAA for Fast quality");
        aa_painter.fill_path<Sample16xAA>(path, Color::White);
        break;
    case FontRenderingSettings::Quality::Good:
        dbgln("Subpixel vertical: Using Sample32xAA for Good quality");
        aa_painter.fill_path<Sample32xAA>(path, Color::White);
        break;
    case FontRenderingSettings::Quality::Best:
        dbgln("Subpixel vertical: Using Sample32xAA for Best quality");
        aa_painter.fill_path<Sample32xAA>(path, Color::White);
        break;
    }
    
    return result_bitmap;
}

Color SubpixelFontRenderer::apply_gamma_correction(Color color, float gamma) const
{
    if (gamma == 1.0f)
        return color;
    
    float inv_gamma = 1.0f / gamma;
    
    auto gamma_correct = [inv_gamma](u8 component) -> u8 {
        float normalized = component / 255.0f;
        float corrected = AK::pow(normalized, inv_gamma);
        return static_cast<u8>(corrected * 255.0f + 0.5f);
    };
    
    return Color(
        gamma_correct(color.red()),
        gamma_correct(color.green()),
        gamma_correct(color.blue()),
        color.alpha()
    );
}

Color SubpixelFontRenderer::blend_with_gamma(Color foreground, Color background, float gamma) const
{
    // Convert to linear space, blend, then convert back
    auto to_linear = [gamma](u8 component) -> float {
        float normalized = component / 255.0f;
        return AK::pow(normalized, gamma);
    };
    
    auto from_linear = [gamma](float linear) -> u8 {
        float inv_gamma = 1.0f / gamma;
        float corrected = AK::pow(linear, inv_gamma);
        return static_cast<u8>(corrected * 255.0f + 0.5f);
    };
    
    float alpha = foreground.alpha() / 255.0f;
    
    float fg_r_linear = to_linear(foreground.red());
    float fg_g_linear = to_linear(foreground.green());
    float fg_b_linear = to_linear(foreground.blue());
    
    float bg_r_linear = to_linear(background.red());
    float bg_g_linear = to_linear(background.green());
    float bg_b_linear = to_linear(background.blue());
    
    float blended_r = fg_r_linear * alpha + bg_r_linear * (1.0f - alpha);
    float blended_g = fg_g_linear * alpha + bg_g_linear * (1.0f - alpha);
    float blended_b = fg_b_linear * alpha + bg_b_linear * (1.0f - alpha);
    
    return Color(
        from_linear(blended_r),
        from_linear(blended_g),
        from_linear(blended_b),
        255
    );
}

} 