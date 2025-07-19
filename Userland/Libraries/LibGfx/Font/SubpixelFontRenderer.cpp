/*
 * Copyright (c) 2023, SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Math.h>
#include <LibGfx/AntiAliasingPainter.h>
#include <LibGfx/Font/SubpixelFontRenderer.h>
#include <LibGfx/Painter.h>
#include <LibGfx/Path.h>

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
    
    // Use improved quality levels for grayscale rendering
    switch (settings.quality) {
    case FontRenderingSettings::Quality::Fast:
        dbgln("Grayscale: Using Sample8xAA for Fast quality");
        aa_painter.fill_path<Sample8xAA>(path, Color::White);
        break;
    case FontRenderingSettings::Quality::Good:
        dbgln("Grayscale: Using Sample16xAA for Good quality");
        aa_painter.fill_path<Sample16xAA>(path, Color::White);
        break;
    case FontRenderingSettings::Quality::Best:
        dbgln("Grayscale: Using Sample32xAA for Best quality");
    aa_painter.fill_path<Sample32xAA>(path, Color::White);
        break;
    }
    
    return result_bitmap;
}

RefPtr<Bitmap> SubpixelFontRenderer::render_subpixel_horizontal(Path const& path, IntSize size, SubpixelOrder order, FontRenderingSettings const& settings)
{
    dbgln("SubpixelFontRenderer: Implementing REAL horizontal subpixel rendering");
    
    // Create a bitmap that's 3x wider for subpixel rendering
    IntSize subpixel_size(size.width() * 3, size.height());
    auto subpixel_bitmap = Bitmap::create(BitmapFormat::BGRA8888, subpixel_size);
    if (subpixel_bitmap.is_error())
        return {};
    
    auto temp_bitmap = subpixel_bitmap.release_value();
    Painter painter(*temp_bitmap);
    AntiAliasingPainter aa_painter(painter);
    
    // Scale the path horizontally by 3x for subpixel resolution
    Path scaled_path;
    for (auto const& segment : path) {
        switch (segment.command()) {
        case PathSegment::MoveTo:
            scaled_path.move_to(FloatPoint(segment.point().x() * 3, segment.point().y()));
            break;
        case PathSegment::LineTo:
            scaled_path.line_to(FloatPoint(segment.point().x() * 3, segment.point().y()));
            break;
        case PathSegment::QuadraticBezierCurveTo:
            scaled_path.quadratic_bezier_curve_to(
                FloatPoint(segment.through().x() * 3, segment.through().y()),
                FloatPoint(segment.point().x() * 3, segment.point().y()));
            break;
        case PathSegment::CubicBezierCurveTo:
            scaled_path.cubic_bezier_curve_to(
                FloatPoint(segment.through_0().x() * 3, segment.through_0().y()),
                FloatPoint(segment.through_1().x() * 3, segment.through_1().y()),
                FloatPoint(segment.point().x() * 3, segment.point().y()));
            break;
        case PathSegment::ClosePath:
            scaled_path.close();
            break;
        }
    }
    
    // Render the scaled path with high quality anti-aliasing
    switch (settings.quality) {
    case FontRenderingSettings::Quality::Fast:
        dbgln("Subpixel horizontal: Using Sample8xAA for Fast quality");
        aa_painter.fill_path<Sample8xAA>(scaled_path, Color::White);
        break;
    case FontRenderingSettings::Quality::Good:
        dbgln("Subpixel horizontal: Using Sample16xAA for Good quality");
        aa_painter.fill_path<Sample16xAA>(scaled_path, Color::White);
        break;
    case FontRenderingSettings::Quality::Best:
        dbgln("Subpixel horizontal: Using Sample32xAA for Best quality");
        aa_painter.fill_path<Sample32xAA>(scaled_path, Color::White);
        break;
    }
    
    // Now convert the 3x wide bitmap back to normal size with subpixel weighting
    auto result_bitmap = Bitmap::create(BitmapFormat::BGRA8888, size);
    if (result_bitmap.is_error())
        return {};
    
    auto final_bitmap = result_bitmap.release_value();
    
    // Apply LCD filtering and subpixel weights
    apply_lcd_filtering(*temp_bitmap, *final_bitmap, order);
    
    dbgln("SubpixelFontRenderer succeeded, returning enhanced bitmap");
    return final_bitmap;
}

void SubpixelFontRenderer::apply_lcd_filtering(Bitmap const& source, Bitmap& target, SubpixelOrder order)
{
    bool is_bgr = (order == SubpixelOrder::BGR);
    
    for (int y = 0; y < target.height(); ++y) {
        for (int x = 0; x < target.width(); ++x) {
            
            // Direct subpixel sampling - no complex filtering to avoid shadows
            int src_x_r = x * 3 + 0;  // Red subpixel
            int src_x_g = x * 3 + 1;  // Green subpixel  
            int src_x_b = x * 3 + 2;  // Blue subpixel
            
            // Clamp to source bitmap bounds
            src_x_r = max(0, min(src_x_r, source.width() - 1));
            src_x_g = max(0, min(src_x_g, source.width() - 1));
            src_x_b = max(0, min(src_x_b, source.width() - 1));
            
            // Get alpha values (coverage) from source
            auto pixel_r = source.get_pixel(src_x_r, y);
            auto pixel_g = source.get_pixel(src_x_g, y);
            auto pixel_b = source.get_pixel(src_x_b, y);
            
            float r_coverage = pixel_r.alpha() / 255.0f;
            float g_coverage = pixel_g.alpha() / 255.0f;
            float b_coverage = pixel_b.alpha() / 255.0f;
            
            // Convert to 0-255 range and apply subpixel order
            u8 red, green, blue;
            if (is_bgr) {
                // BGR order: swap red and blue
                red = static_cast<u8>(b_coverage * 255);
                green = static_cast<u8>(g_coverage * 255);
                blue = static_cast<u8>(r_coverage * 255);
            } else {
                // RGB order (normal)
                red = static_cast<u8>(r_coverage * 255);
                green = static_cast<u8>(g_coverage * 255);
                blue = static_cast<u8>(b_coverage * 255);
            }
            
            // Use average coverage for clean alpha without shadows
            u8 alpha = static_cast<u8>((r_coverage + g_coverage + b_coverage) / 3.0f * 255);
            
            target.set_pixel(x, y, Color(red, green, blue, alpha));
        }
    }
}

RefPtr<Bitmap> SubpixelFontRenderer::render_subpixel_vertical(Path const& path, IntSize size, SubpixelOrder order, FontRenderingSettings const& settings)
{
    dbgln("SubpixelFontRenderer: Implementing REAL vertical subpixel rendering");
    
    // Create a bitmap that's 3x taller for vertical subpixel rendering
    IntSize subpixel_size(size.width(), size.height() * 3);
    auto subpixel_bitmap = Bitmap::create(BitmapFormat::BGRA8888, subpixel_size);
    if (subpixel_bitmap.is_error())
        return {};
    
    auto temp_bitmap = subpixel_bitmap.release_value();
    Painter painter(*temp_bitmap);
    AntiAliasingPainter aa_painter(painter);
    
    // Scale the path vertically by 3x for subpixel resolution
    Path scaled_path;
    for (auto const& segment : path) {
        switch (segment.command()) {
        case PathSegment::MoveTo:
            scaled_path.move_to(FloatPoint(segment.point().x(), segment.point().y() * 3));
            break;
        case PathSegment::LineTo:
            scaled_path.line_to(FloatPoint(segment.point().x(), segment.point().y() * 3));
            break;
        case PathSegment::QuadraticBezierCurveTo:
            scaled_path.quadratic_bezier_curve_to(
                FloatPoint(segment.through().x(), segment.through().y() * 3),
                FloatPoint(segment.point().x(), segment.point().y() * 3));
            break;
        case PathSegment::CubicBezierCurveTo:
            scaled_path.cubic_bezier_curve_to(
                FloatPoint(segment.through_0().x(), segment.through_0().y() * 3),
                FloatPoint(segment.through_1().x(), segment.through_1().y() * 3),
                FloatPoint(segment.point().x(), segment.point().y() * 3));
            break;
        case PathSegment::ClosePath:
            scaled_path.close();
            break;
        }
    }
    
    // Render the scaled path with high quality anti-aliasing
    switch (settings.quality) {
    case FontRenderingSettings::Quality::Fast:
        dbgln("Subpixel vertical: Using Sample8xAA for Fast quality");
        aa_painter.fill_path<Sample8xAA>(scaled_path, Color::White);
        break;
    case FontRenderingSettings::Quality::Good:
        dbgln("Subpixel vertical: Using Sample16xAA for Good quality");
        aa_painter.fill_path<Sample16xAA>(scaled_path, Color::White);
        break;
    case FontRenderingSettings::Quality::Best:
        dbgln("Subpixel vertical: Using Sample32xAA for Best quality");
        aa_painter.fill_path<Sample32xAA>(scaled_path, Color::White);
        break;
    }
    
    // Now convert the 3x tall bitmap back to normal size with subpixel weighting
    auto result_bitmap = Bitmap::create(BitmapFormat::BGRA8888, size);
    if (result_bitmap.is_error())
        return {};
    
    auto final_bitmap = result_bitmap.release_value();
    
    // Apply vertical LCD filtering and subpixel weights
    apply_vertical_lcd_filtering(*temp_bitmap, *final_bitmap, order);
    
    dbgln("Vertical SubpixelFontRenderer succeeded, returning enhanced bitmap");
    return final_bitmap;
}

void SubpixelFontRenderer::apply_vertical_lcd_filtering(Bitmap const& source, Bitmap& target, SubpixelOrder order)
{
    bool is_bgr = (order == SubpixelOrder::BGR);
    
    for (int y = 0; y < target.height(); ++y) {
        for (int x = 0; x < target.width(); ++x) {
            
            // Direct subpixel sampling - no complex filtering to avoid shadows
            int src_y_r = y * 3 + 0;  // Red subpixel
            int src_y_g = y * 3 + 1;  // Green subpixel  
            int src_y_b = y * 3 + 2;  // Blue subpixel
            
            // Clamp to source bitmap bounds
            src_y_r = max(0, min(src_y_r, source.height() - 1));
            src_y_g = max(0, min(src_y_g, source.height() - 1));
            src_y_b = max(0, min(src_y_b, source.height() - 1));
            
            // Get alpha values (coverage) from source
            auto pixel_r = source.get_pixel(x, src_y_r);
            auto pixel_g = source.get_pixel(x, src_y_g);
            auto pixel_b = source.get_pixel(x, src_y_b);
            
            float r_coverage = pixel_r.alpha() / 255.0f;
            float g_coverage = pixel_g.alpha() / 255.0f;
            float b_coverage = pixel_b.alpha() / 255.0f;
            
            // Convert to 0-255 range and apply subpixel order
            u8 red, green, blue;
            if (is_bgr) {
                // BGR order: swap red and blue
                red = static_cast<u8>(b_coverage * 255);
                green = static_cast<u8>(g_coverage * 255);
                blue = static_cast<u8>(r_coverage * 255);
            } else {
                // RGB order (normal)
                red = static_cast<u8>(r_coverage * 255);
                green = static_cast<u8>(g_coverage * 255);
                blue = static_cast<u8>(b_coverage * 255);
            }
            
            // Use average coverage for clean alpha without shadows
            u8 alpha = static_cast<u8>((r_coverage + g_coverage + b_coverage) / 3.0f * 255);
            
            target.set_pixel(x, y, Color(red, green, blue, alpha));
        }
    }
}

Color SubpixelFontRenderer::apply_gamma_correction(Color color, float gamma) const
{
    if (gamma == 1.0f)
        return color;
    
    float inv_gamma = 1.0f / gamma;
    u8 r = static_cast<u8>(powf(color.red() / 255.0f, inv_gamma) * 255);
    u8 g = static_cast<u8>(powf(color.green() / 255.0f, inv_gamma) * 255);
    u8 b = static_cast<u8>(powf(color.blue() / 255.0f, inv_gamma) * 255);
    
    return Color(r, g, b, color.alpha());
}

Color SubpixelFontRenderer::blend_with_gamma(Color foreground, Color background, float gamma) const
{
    if (!foreground.alpha())
        return background;
    if (foreground.alpha() == 255)
        return apply_gamma_correction(foreground, gamma);
    
    // Apply gamma correction during blending for more accurate results
    float alpha = foreground.alpha() / 255.0f;
    float inv_alpha = 1.0f - alpha;
    
    // Convert to linear space
    float fg_r = powf(foreground.red() / 255.0f, gamma);
    float fg_g = powf(foreground.green() / 255.0f, gamma);
    float fg_b = powf(foreground.blue() / 255.0f, gamma);
    
    float bg_r = powf(background.red() / 255.0f, gamma);
    float bg_g = powf(background.green() / 255.0f, gamma);
    float bg_b = powf(background.blue() / 255.0f, gamma);
    
    // Blend in linear space
    float result_r = fg_r * alpha + bg_r * inv_alpha;
    float result_g = fg_g * alpha + bg_g * inv_alpha;
    float result_b = fg_b * alpha + bg_b * inv_alpha;
    
    // Convert back to gamma space
    float inv_gamma = 1.0f / gamma;
    u8 r = static_cast<u8>(powf(result_r, inv_gamma) * 255);
    u8 g = static_cast<u8>(powf(result_g, inv_gamma) * 255);
    u8 b = static_cast<u8>(powf(result_b, inv_gamma) * 255);
    
    return Color(r, g, b, 255);
}

} 