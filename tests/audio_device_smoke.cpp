// Standalone real-device lifecycle probe; not evidence of human listening.
#ifdef SILIC2_AUDIO_DEVICE_SMOKE_MAIN
#include "audio/audio_system.h"
#include <chrono>
#include <iostream>
#include <thread>
int main() {
    silic2::AudioSystem audio;
    if (!audio.initialize()) { std::cerr<<audio.statusText()<<'\n';return 2; }
    // Exercise actual callbacks at zero cue gain; listening is a separate gate.
    for (int run=0;run<50;++run) {
        audio.reset();
        audio.play(silic2::AudioCue::Ambience,{},0,false,0.0f);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        audio.setPaused(true);
        auto before=audio.diagnostics().renderedFrames;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if (audio.diagnostics().renderedFrames!=before) return 3;
        audio.setPaused(false);
        audio.stopAll();
        if(audio.diagnostics().activeVoices!=0) return 4;
    }
    audio.reset(); audio.setMuted(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    const auto frames=audio.diagnostics().renderedFrames;
    if (!audio.available() || frames==0) return 5;
    audio.shutdown();
    if(audio.available() || audio.diagnostics().loadedFrames!=0) return 6;
    std::cout<<"Real backend callback active; 50 audio-only reset/pause cycles passed; last frames="<<frames<<". No listening or full-game claim.\n";
}
#endif
