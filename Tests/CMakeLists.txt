# Diagnostics — built only with -DAMPBASS_BUILD_TESTS=ON

# DSP unit diagnostics (header-only stages, no plugin wrapper)
juce_add_console_app(AmpBassDSPTests PRODUCT_NAME "AmpBassDSPTests")
target_sources(AmpBassDSPTests PRIVATE DSPTests.cpp TestSignals.h)
target_include_directories(AmpBassDSPTests PRIVATE ${CMAKE_SOURCE_DIR}/Source/DSP)
target_compile_definitions(AmpBassDSPTests PRIVATE JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0)
target_link_libraries(AmpBassDSPTests PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

# Offline host that loads the built VST3 and drives it like a DAW
juce_add_console_app(AmpBassHostRender PRODUCT_NAME "AmpBassHostRender")
target_sources(AmpBassHostRender PRIVATE HostRender.cpp TestSignals.h)
target_compile_definitions(AmpBassHostRender PRIVATE
    JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0
    JUCE_PLUGINHOST_VST3=1)
target_link_libraries(AmpBassHostRender PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_events
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
