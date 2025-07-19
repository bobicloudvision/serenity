/*
 * Copyright (c) 2021, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2021, Thomas Keppler <winfr34k@gmail.com>
 * Copyright (c) 2022, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "FontSettingsWidget.h"
#include <Applications/DisplaySettings/FontSettingsGML.h>
#include <LibCore/ConfigFile.h>
#include <LibGUI/Button.h>
#include <LibGUI/CheckBox.h>
#include <LibGUI/ComboBox.h>
#include <LibGUI/ConnectionToWindowServer.h>
#include <LibGUI/FontPicker.h>
#include <LibGUI/ItemListModel.h>
#include <LibGfx/Font/FontDatabase.h>

namespace DisplaySettings {

static void update_label_with_font(GUI::Label&, Gfx::Font const&);

ErrorOr<NonnullRefPtr<FontSettingsWidget>> FontSettingsWidget::try_create()
{
    auto font_settings_widget = TRY(adopt_nonnull_ref_or_enomem(new (nothrow) FontSettingsWidget()));
    TRY(font_settings_widget->setup_interface());
    return font_settings_widget;
}

ErrorOr<void> FontSettingsWidget::setup_interface()
{
    TRY(load_from_gml(font_settings_gml));

    auto& default_font = Gfx::FontDatabase::default_font();
    m_default_font_label = *find_descendant_of_type_named<GUI::Label>("default_font_label");
    update_label_with_font(*m_default_font_label, default_font);

    auto& default_font_button = *find_descendant_of_type_named<GUI::Button>("default_font_button");
    default_font_button.on_click = [this](auto) {
        auto font_picker = GUI::FontPicker::construct(window(), &m_default_font_label->font(), false);
        if (font_picker->exec() == GUI::Dialog::ExecResult::OK) {
            update_label_with_font(*m_default_font_label, *font_picker->font());
            set_modified(true);
        }
    };

    auto& window_title_font = Gfx::FontDatabase::window_title_font();
    m_window_title_font_label = *find_descendant_of_type_named<GUI::Label>("window_title_font_label");
    update_label_with_font(*m_window_title_font_label, window_title_font);

    auto& window_title_font_button = *find_descendant_of_type_named<GUI::Button>("window_title_font_button");
    window_title_font_button.on_click = [this](auto) {
        auto font_picker = GUI::FontPicker::construct(window(), &m_window_title_font_label->font(), false);
        if (font_picker->exec() == GUI::Dialog::ExecResult::OK) {
            update_label_with_font(*m_window_title_font_label, *font_picker->font());
            set_modified(true);
        }
    };

    auto& default_fixed_width_font = Gfx::FontDatabase::default_fixed_width_font();
    m_fixed_width_font_label = *find_descendant_of_type_named<GUI::Label>("fixed_width_font_label");
    update_label_with_font(*m_fixed_width_font_label, default_fixed_width_font);

    auto& fixed_width_font_button = *find_descendant_of_type_named<GUI::Button>("fixed_width_font_button");
    fixed_width_font_button.on_click = [this](auto) {
        auto font_picker = GUI::FontPicker::construct(window(), &m_fixed_width_font_label->font(), true);
        if (font_picker->exec() == GUI::Dialog::ExecResult::OK) {
            update_label_with_font(*m_fixed_width_font_label, *font_picker->font());
            set_modified(true);
        }
    };

    // Set up font quality controls
    TRY(setup_font_quality_controls());

    return {};
}

ErrorOr<void> FontSettingsWidget::setup_font_quality_controls()
{
    // Read current font rendering settings directly from WindowServer.ini (like other DisplaySettings widgets)
    auto ws_config = TRY(Core::ConfigFile::open("/etc/WindowServer.ini"));
    
    auto quality = static_cast<int>(ws_config->read_entry("FontRendering", "Quality", "2").to_number<int>().value_or(2));
    auto use_hinting = ws_config->read_bool_entry("FontRendering", "UseHinting", true);
    auto use_gamma_correction = ws_config->read_bool_entry("FontRendering", "UseGammaCorrection", true);
    auto subpixel_order = static_cast<int>(ws_config->read_entry("FontRendering", "SubpixelOrder", "1").to_number<int>().value_or(1));

    // Set up quality combo box
    m_quality_combo = *find_descendant_of_type_named<GUI::ComboBox>("quality_combo");
    m_quality_options = { "Fast", "Good", "Best" };
    auto quality_model = GUI::ItemListModel<ByteString>::create(m_quality_options);
    m_quality_combo->set_model(quality_model);
    m_quality_combo->set_selected_index(quality);
    m_quality_combo->on_change = [this](auto, auto&) {
        set_modified(true);
    };

    // Set up hinting checkbox
    m_hinting_checkbox = *find_descendant_of_type_named<GUI::CheckBox>("hinting_checkbox");
    m_hinting_checkbox->set_checked(use_hinting);
    m_hinting_checkbox->on_checked = [this](auto) {
        set_modified(true);
    };

    // Set up gamma correction checkbox
    m_gamma_correction_checkbox = *find_descendant_of_type_named<GUI::CheckBox>("gamma_correction_checkbox");
    m_gamma_correction_checkbox->set_checked(use_gamma_correction);
    m_gamma_correction_checkbox->on_checked = [this](auto) {
        set_modified(true);
    };

    // Set up subpixel order combo box
    m_subpixel_combo = *find_descendant_of_type_named<GUI::ComboBox>("subpixel_combo");
    m_subpixel_options = { "None", "RGB", "BGR", "V-RGB", "V-BGR" };
    auto subpixel_model = GUI::ItemListModel<ByteString>::create(m_subpixel_options);
    m_subpixel_combo->set_model(subpixel_model);
    m_subpixel_combo->set_selected_index(subpixel_order);
    m_subpixel_combo->on_change = [this](auto, auto&) {
        set_modified(true);
    };

    return {};
}

static void update_label_with_font(GUI::Label& label, Gfx::Font const& font)
{
    label.set_text(font.human_readable_name());
    label.set_font(font);
}

void FontSettingsWidget::apply_settings()
{
    GUI::ConnectionToWindowServer::the().set_system_fonts(
        m_default_font_label->font().qualified_name().to_byte_string(),
        m_fixed_width_font_label->font().qualified_name().to_byte_string(),
        m_window_title_font_label->font().qualified_name().to_byte_string());

    // Save font quality settings via WindowServer IPC (automatically applies settings and notifies all clients)
    auto save_success = GUI::ConnectionToWindowServer::the().set_font_rendering_settings(
        static_cast<u32>(m_quality_combo->selected_index()),
        m_hinting_checkbox->is_checked(),
        m_gamma_correction_checkbox->is_checked(),
        2.2f, // Standard gamma value
        static_cast<u32>(m_subpixel_combo->selected_index())
    );
    
    if (!save_success) {
        dbgln("Failed to save font rendering settings via WindowServer");
    }
}

}
