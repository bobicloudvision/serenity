/*
 * Copyright (c) 2023, SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibGfx/Font/FontConfiguration.h>
#include <LibGfx/Font/SubpixelFontRenderer.h>

namespace Gfx {

FontConfiguration& FontConfiguration::the()
{
    static FontConfiguration s_the;
    return s_the;
}

void FontConfiguration::enable_subpixel_rendering(SubpixelOrder order)
{
    m_default_settings.subpixel_order = order;
    
    // Update the subpixel renderer with the new settings
    SubpixelFontRenderer::the().set_default_settings(m_default_settings);
}

void FontConfiguration::disable_subpixel_rendering()
{
    m_default_settings.subpixel_order = SubpixelOrder::None;
    
    // Update the subpixel renderer with the new settings
    SubpixelFontRenderer::the().set_default_settings(m_default_settings);
}

SubpixelOrder FontConfiguration::detect_subpixel_order() const
{
    // For now, return RGB as it's the most common
    // A full implementation would query the display system
    // to determine the actual subpixel layout
    return SubpixelOrder::RGB;
}

} 