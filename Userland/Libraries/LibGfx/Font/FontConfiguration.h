/*
 * Copyright (c) 2023, SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibGfx/Font/Font.h>

namespace Gfx {

class FontConfiguration {
public:
    static FontConfiguration& the();

    FontRenderingSettings const& default_settings() const { return m_default_settings; }
    void set_default_settings(FontRenderingSettings const& settings);

    // Convenience methods for common configurations
    void enable_subpixel_rendering(SubpixelOrder order = SubpixelOrder::RGB);
    void disable_subpixel_rendering();
    
    void enable_hinting() { m_default_settings.use_hinting = true; }
    void disable_hinting() { m_default_settings.use_hinting = false; }
    
    void set_quality(FontRenderingSettings::Quality quality) { m_default_settings.quality = quality; }
    
    void enable_gamma_correction(float gamma = 2.2f) {
        m_default_settings.use_gamma_correction = true;
        m_default_settings.gamma_value = gamma;
    }
    void disable_gamma_correction() { m_default_settings.use_gamma_correction = false; }

    // Auto-detect the best subpixel order for the current display
    SubpixelOrder detect_subpixel_order() const;
    
    // Configuration will be managed by WindowServer through WindowServer.ini

private:
    FontConfiguration() {
        // Set high-quality defaults
        m_default_settings.subpixel_order = SubpixelOrder::RGB; // Most common
        m_default_settings.use_hinting = true;
        m_default_settings.use_gamma_correction = true;
        m_default_settings.gamma_value = 2.2f;
        m_default_settings.quality = FontRenderingSettings::Quality::Best; // High quality by default
    }

    FontRenderingSettings m_default_settings;
};

} 