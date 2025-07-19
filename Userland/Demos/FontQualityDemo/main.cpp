#include <LibCore/System.h>
#include <LibGUI/Application.h>
#include <LibGUI/BoxLayout.h>
#include <LibGUI/Frame.h>
#include <LibGUI/Label.h>
#include <LibGUI/Painter.h>
#include <LibGUI/ScrollableContainerWidget.h>
#include <LibGUI/Widget.h>
#include <LibGUI/Window.h>
#include <LibGfx/Font/FontConfiguration.h>
#include <LibGfx/Font/FontDatabase.h>
#include <LibGfx/Palette.h>
#include <LibMain/Main.h>
#include <AK/FlyString.h>
#include <typeinfo>

class FontContentWidget final : public GUI::Widget {
    C_OBJECT(FontContentWidget);

public:
    virtual ~FontContentWidget() override = default;

private:
    FontContentWidget()
    {
        m_sample_text = "ABCDEFGHIJKLMNOPRSTUVWXYZ abcdefghijklmnoprstuvwxyz 1234567890"sv;
        m_font_sizes = { 16, 28, 32, 36 };
        calculate_total_height();
    }

    void calculate_total_height()
    {
        int total_height = 10; // Initial margin
        
        // Calculate height for each section
        auto add_section_height = [&](StringView) {
            total_height += 30; // Section title
            for (auto size : m_font_sizes) {
                total_height += size + 10; // Text height + spacing
            }
            total_height += 20; // Section spacing
        };

        add_section_height("Fixed Width Font"sv);
        add_section_height("Fast Quality"sv);
        add_section_height("Good Quality"sv);
        add_section_height("Best Quality"sv);
        add_section_height("Default Font"sv);

        resize(980, total_height);
    }

    virtual void paint_event(GUI::PaintEvent& event) override
    {
        GUI::Painter painter(*this);
        painter.add_clip_rect(event.rect());
        painter.fill_rect(event.rect(), palette().base());

        int y = 5;
        int x_offset = 5;

        // Draw with different fonts and qualities
        draw_quality_section(painter, y, x_offset, Gfx::FontRenderingSettings::Quality::Fast, "Fast Quality"sv);
        y += 10;
        draw_quality_section(painter, y, x_offset, Gfx::FontRenderingSettings::Quality::Good, "Good Quality"sv);
        y += 10;
        draw_quality_section(painter, y, x_offset, Gfx::FontRenderingSettings::Quality::Best, "Best Quality"sv);
        y += 10;
        draw_font_section(painter, y, x_offset, Gfx::FontDatabase::default_font(), "Default Font"sv);
    }

    void draw_font_section(GUI::Painter& painter, int& y, int x_offset, Gfx::Font const& base_font, StringView title)
    {
        (void)base_font; // Unused parameter - we get fonts directly now
        auto& title_font = Gfx::FontDatabase::default_fixed_width_font().bold_variant();
        
        // Draw section title with background
        auto title_rect = Gfx::IntRect(0, y, width(), 25);
        painter.fill_rect(title_rect, palette().hover_highlight());
        painter.draw_text(title_rect.shrunken(10, 0), title, title_font, Gfx::TextAlignment::CenterLeft);
        y += 30;

        // Draw sample text in different sizes
        for (auto size : m_font_sizes) {
            // Use TrueType font instead of bitmap font to see OpenType rendering differences
            auto base_font = Gfx::FontDatabase::the().get_by_name(String::formatted("Liberation Mono {} 400 0", size).release_value_but_fixme_should_propagate_errors());
            if (!base_font) {
                // Fallback to SerenitySans if Liberation Mono isn't available
                base_font = Gfx::FontDatabase::the().get_by_name(String::formatted("SerenitySans {} 400 0", size).release_value_but_fixme_should_propagate_errors());
            }
            if (!base_font) {
                // Final fallback to default fixed width font (bitmap)
                auto& default_font = Gfx::FontDatabase::default_fixed_width_font();
                base_font = default_font.with_size(size);
            }
            
            auto size_text = String::formatted("{}px: ", size).release_value_but_fixme_should_propagate_errors();
            
            // Draw size label with right alignment
            auto label_rect = Gfx::IntRect(x_offset, y, 60, size + 5);
            painter.draw_text(label_rect, size_text, title_font, Gfx::TextAlignment::CenterRight);
            
            // Draw sample text
            auto text_rect = Gfx::IntRect(x_offset + 70, y, width() - x_offset - 90, size + 5);
            dbgln("Drawing text at size {} with current config using font: {}", size, base_font->human_readable_name());
            dbgln("Font type: {}", typeid(*base_font).name());
            
            painter.draw_text(text_rect, m_sample_text, *base_font, Gfx::TextAlignment::CenterLeft);
            
            y += size + 10;
        }
    }

    void draw_quality_section(GUI::Painter& painter, int& y, int x_offset, Gfx::FontRenderingSettings::Quality quality, StringView title)
    {
        auto& config = Gfx::FontConfiguration::the();
        
        // Draw section title with background
        auto& title_font = Gfx::FontDatabase::default_fixed_width_font().bold_variant();
        auto title_rect = Gfx::IntRect(0, y, width(), 25);
        painter.fill_rect(title_rect, palette().hover_highlight());
        painter.draw_text(title_rect.shrunken(10, 0), title, title_font, Gfx::TextAlignment::CenterLeft);
        y += 30;

        // Configure font settings for this section
        dbgln("=== Configuring {} ===", title);
        config.set_quality(quality);
        
        if (quality == Gfx::FontRenderingSettings::Quality::Best) {
            dbgln("Setting Best quality: RGB subpixel + hinting + gamma correction");
            config.enable_subpixel_rendering(Gfx::SubpixelOrder::RGB);
            config.enable_hinting();
            config.enable_gamma_correction(2.2f);
        } else if (quality == Gfx::FontRenderingSettings::Quality::Good) {
            dbgln("Setting Good quality: RGB subpixel + hinting + no gamma");
            config.enable_subpixel_rendering(Gfx::SubpixelOrder::RGB);
            config.enable_hinting();
            config.disable_gamma_correction();
        } else {
            dbgln("Setting Fast quality: no subpixel + no hinting + no gamma");
            config.disable_subpixel_rendering();
            config.disable_hinting();
            config.disable_gamma_correction();
        }

        // Verify settings were applied
        auto const& settings = config.default_settings();
        dbgln("Applied settings: Quality={}, Subpixel={}, Hinting={}, Gamma={}", 
              (int)settings.quality, (int)settings.subpixel_order, 
              settings.use_hinting, settings.use_gamma_correction);

        // Draw sample text in different sizes
        for (auto size : m_font_sizes) {
            // Use TrueType font instead of bitmap font to see OpenType rendering differences
            auto sized_font = Gfx::FontDatabase::the().get_by_name(String::formatted("Liberation Mono {} 400 0", size).release_value_but_fixme_should_propagate_errors());
            if (!sized_font) {
                // Fallback to SerenitySans if Liberation Mono isn't available
                sized_font = Gfx::FontDatabase::the().get_by_name(String::formatted("SerenitySans {} 400 0", size).release_value_but_fixme_should_propagate_errors());
            }
            if (!sized_font) {
                // Final fallback to default fixed width font (bitmap)
                sized_font = Gfx::FontDatabase::default_fixed_width_font().with_size(size);
            }
            
            auto size_text = String::formatted("{}px: ", size).release_value_but_fixme_should_propagate_errors();
            
            // Draw size label with right alignment
            auto label_rect = Gfx::IntRect(x_offset, y, 60, size + 5);
            painter.draw_text(label_rect, size_text, title_font, Gfx::TextAlignment::CenterRight);
            
            // Draw sample text
            auto text_rect = Gfx::IntRect(x_offset + 70, y, width() - x_offset - 90, size + 5);
            dbgln("Drawing text at size {} with current config using font: {}", size, sized_font->human_readable_name());
            dbgln("Font type: {}", typeid(*sized_font).name());
            
            painter.draw_text(text_rect, m_sample_text, *sized_font, Gfx::TextAlignment::CenterLeft);
            
            y += size + 10;
        }
        dbgln("=== Finished {} section ===", title);
    }

    StringView m_sample_text;
    Vector<int> m_font_sizes;
};

class FontDemoWidget final : public GUI::Frame {
    C_OBJECT(FontDemoWidget);

public:
    virtual ~FontDemoWidget() override = default;

private:
    FontDemoWidget()
    {
        set_fill_with_background_color(true);
        set_layout<GUI::VerticalBoxLayout>();
        
        auto& scrollable = add<GUI::ScrollableContainerWidget>();
        scrollable.set_should_hide_unnecessary_scrollbars(true);
        
        auto& content = scrollable.add<FontContentWidget>();
        scrollable.set_widget(&content);
    }
};

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    TRY(Core::System::pledge("stdio recvfd sendfd rpath unix"));

    auto app = TRY(GUI::Application::create(arguments));
    
    TRY(Core::System::pledge("stdio recvfd sendfd rpath"));
    TRY(Core::System::unveil("/res", "r"));
    TRY(Core::System::unveil(nullptr, nullptr));

    auto window = GUI::Window::construct();
    window->set_title("Font Quality Demo");
    window->resize(1000, 600);

    auto main_widget = window->set_main_widget<FontDemoWidget>();
    window->show();

    return app->exec();
} 