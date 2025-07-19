/*
 * Copyright (c) 2023, SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibGfx/Font/FontConfiguration.h>
#include <LibGfx/Font/SubpixelFontRenderer.h>
#include <LibGfx/Font/FontDatabase.h>

namespace Gfx {

FontConfiguration& FontConfiguration::the()
{
    static FontConfiguration s_the;
    return s_the;
}

void FontConfiguration::set_default_settings(FontRenderingSettings const& settings)
{
    m_default_settings = settings;
    
    // Update the subpixel renderer with the new settings
    SubpixelFontRenderer::the().set_default_settings(m_default_settings);
    
    // Invalidate all glyph caches so quality changes take effect immediately
    FontDatabase::the().invalidate_all_glyph_caches();
}

void FontConfiguration::enable_subpixel_rendering(SubpixelOrder order)
{
    m_default_settings.subpixel_order = order;
    
    // Update the subpixel renderer with the new settings
    SubpixelFontRenderer::the().set_default_settings(m_default_settings);
    
    // Invalidate all glyph caches so quality changes take effect immediately
    FontDatabase::the().invalidate_all_glyph_caches();
}

void FontConfiguration::disable_subpixel_rendering()
{
    m_default_settings.subpixel_order = SubpixelOrder::None;
    
    // Update the subpixel renderer with the new settings
    SubpixelFontRenderer::the().set_default_settings(m_default_settings);
    
    // Invalidate all glyph caches so quality changes take effect immediately
    FontDatabase::the().invalidate_all_glyph_caches();
}

SubpixelOrder FontConfiguration::detect_subpixel_order() const
{
    // For now, return RGB as it's the most common
    // A full implementation would query the display system
    // to determine the actual subpixel layout
    return SubpixelOrder::RGB;
}



} 