serenity_component(
    FontQualityDemo
    TARGETS FontQualityDemo
)

set(SOURCES
    main.cpp
)

serenity_app(FontQualityDemo ICON app-font-editor)
target_link_libraries(FontQualityDemo PRIVATE LibGUI LibIPC LibGfx LibCore LibMain) 