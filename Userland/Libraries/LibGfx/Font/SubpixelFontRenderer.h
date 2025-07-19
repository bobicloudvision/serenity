/*
 * Copyright (c) 2023, SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibGfx/Bitmap.h>
#include <LibGfx/Color.h>
#include <LibGfx/Font/Font.h>
#include <LibGfx/Path.h>

namespace Gfx {

class SubpixelFontRenderer {
public:
    static SubpixelFontRenderer& the();

    RefPtr<Bitmap> render_glyph_with_subpixel(
        Path const& glyph_path,
        IntSize glyph_size,
        FontRenderingSettings const& settings = {});

    void set_default_settings(FontRenderingSettings const& settings) { m_default_settings = settings; }
    FontRenderingSettings const& default_settings() const { return m_default_settings; }

private:
    SubpixelFontRenderer() = default;

    RefPtr<Bitmap> render_grayscale(Path const& path, IntSize size, FontRenderingSettings const& settings);
    RefPtr<Bitmap> render_subpixel_horizontal(Path const& path, IntSize size, SubpixelOrder order, FontRenderingSettings const& settings);
    RefPtr<Bitmap> render_subpixel_vertical(Path const& path, IntSize size, SubpixelOrder order, FontRenderingSettings const& settings);
    
    void apply_lcd_filtering(Bitmap const& source, Bitmap& target, SubpixelOrder order);
    void apply_vertical_lcd_filtering(Bitmap const& source, Bitmap& target, SubpixelOrder order);
    
    Color apply_gamma_correction(Color color, float gamma) const;
    Color blend_with_gamma(Color foreground, Color background, float gamma) const;

    FontRenderingSettings m_default_settings;
};

} 