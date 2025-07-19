@GUI::Widget {
    fill_with_background_color: true
    layout: @GUI::VerticalBoxLayout {
        margins: [8]
        spacing: 8
    }

    @GUI::Widget {
        preferred_height: "fit"
        layout: @GUI::HorizontalBoxLayout {
            spacing: 6
        }

        @GUI::Label {
            fixed_width: 100
            text: "Default font:"
            text_alignment: "CenterLeft"
        }

        @GUI::Label {
            background_role: "Base"
            frame_style: "SunkenContainer"
            fill_with_background_color: true
            name: "default_font_label"
        }

        @GUI::Button {
            text: "..."
            name: "default_font_button"
            fixed_width: 30
        }
    }

    @GUI::Widget {
        preferred_height: "fit"
        layout: @GUI::HorizontalBoxLayout {
            spacing: 6
        }

        @GUI::Label {
            fixed_width: 100
            text: "Window title font:"
            text_alignment: "CenterLeft"
        }

        @GUI::Label {
            background_role: "Base"
            frame_style: "SunkenContainer"
            fill_with_background_color: true
            name: "window_title_font_label"
        }

        @GUI::Button {
            text: "..."
            name: "window_title_font_button"
            fixed_width: 30
        }
    }

    @GUI::Widget {
        preferred_height: "fit"
        layout: @GUI::HorizontalBoxLayout {
            spacing: 6
        }

        @GUI::Label {
            fixed_width: 100
            text: "Fixed-width font:"
            text_alignment: "CenterLeft"
        }

        @GUI::Label {
            background_role: "Base"
            frame_style: "SunkenContainer"
            fill_with_background_color: true
            name: "fixed_width_font_label"
        }

        @GUI::Button {
            text: "..."
            name: "fixed_width_font_button"
            fixed_width: 30
        }
    }

    @GUI::GroupBox {
        title: "Font Rendering Quality"
        preferred_height: "fit"
        layout: @GUI::VerticalBoxLayout {
            margins: [8]
            spacing: 6
        }

        @GUI::Widget {
            preferred_height: "fit"
            layout: @GUI::HorizontalBoxLayout {
                spacing: 6
            }

            @GUI::Label {
                fixed_width: 100
                text: "Quality:"
                text_alignment: "CenterLeft"
            }

            @GUI::ComboBox {
                name: "quality_combo"
                model_only: true
            }
        }

        @GUI::Widget {
            preferred_height: "fit"
            layout: @GUI::HorizontalBoxLayout {
                spacing: 6
            }

            @GUI::CheckBox {
                name: "hinting_checkbox"
                text: "Enable font hinting (improves small text)"
            }
        }

        @GUI::Widget {
            preferred_height: "fit"
            layout: @GUI::HorizontalBoxLayout {
                spacing: 6
            }

            @GUI::CheckBox {
                name: "gamma_correction_checkbox" 
                text: "Enable gamma correction"
            }
        }

        @GUI::Widget {
            preferred_height: "fit"
            layout: @GUI::HorizontalBoxLayout {
                spacing: 6
            }

            @GUI::Label {
                fixed_width: 100
                text: "Subpixel order:"
                text_alignment: "CenterLeft"
            }

            @GUI::ComboBox {
                name: "subpixel_combo"
                model_only: true
            }
        }
    }

    @GUI::Layout::Spacer {}
}
