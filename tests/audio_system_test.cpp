#include "audio/audio_system.h"
#include "test_support.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
using silic2::AudioSystem;
using silic2::AudioCue;
using silic2::test::require;
namespace {
double energy(const std::vector<float>& pcm, int channel) {
    double result=0;
    for (size_t i=channel;i<pcm.size();i+=2) result+=pcm[i]*pcm[i];
    return result;
}
}
void runAudioTests() {
    AudioSystem audio;
    require(!audio.initialize("res/audio/does-not-exist",false),"missing asset fails visibly");
    require(!audio.available() && std::string(audio.statusText())=="AUDIO UNAVAILABLE","failure status");
    require(!audio.play(AudioCue::Hit),"failed audio is safe no-op");
    require(audio.initialize("res/audio",false),"all WAVs preload");
    require(!audio.available() && std::string(audio.statusText())=="AUDIO OFFLINE TEST","offline is not device success");
    require(audio.diagnostics().loadedFrames > 48000*10,"all PCM resident");
    audio.setListener({0,0,0},{0,0,-1});
    std::vector<float> block(4800*2);
    require(audio.play(AudioCue::SentryCharge,{10,0,0},1),"right cue accepted");
    audio.renderOffline(block.data(),4800);
    require(energy(block,1)>1 && energy(block,0)<1e-9,"right source renders right");
    const double nearEnergy=energy(block,1);
    audio.reset();
    audio.play(AudioCue::SentryCharge,{30,0,0},1);
    audio.renderOffline(block.data(),4800);
    require(energy(block,1)<nearEnergy*.4,"distance attenuates");
    audio.reset();
    audio.play(AudioCue::SentryCharge,{-10,0,0},1);
    audio.renderOffline(block.data(),4800);
    require(energy(block,0)>1 && energy(block,1)<1e-9,"left source renders left");
    // Paused output is silent and resumes at exactly the same sample.
    audio.reset(); audio.play(AudioCue::SentryCharge,{},1,false);
    audio.setPaused(true); audio.renderOffline(block.data(),4800);
    require(energy(block,0)==0 && audio.diagnostics().renderedFrames==0,"pause freezes cursor");
    audio.setPaused(false); audio.renderOffline(block.data(),4800);
    auto resumed=block;
    audio.reset(); audio.play(AudioCue::SentryCharge,{},1,false); audio.renderOffline(block.data(),4800);
    require(block==resumed,"pause preserves waveform position");
    audio.reset();
    for(int i=0;i<40;++i) audio.play(AudioCue::PlayerShot,{},0,false);
    require(audio.diagnostics().activeVoices==16,"ordinary sounds preserve 8 reserved voices");
    for(int i=1;i<=8;++i) require(audio.play(AudioCue::SentryCharge,{},i),"critical voice admitted");
    require(audio.diagnostics().activeVoices==24 && audio.diagnostics().criticalVoices==8,"voice budget");
    audio.play(AudioCue::Transform);
    require(audio.diagnostics().criticalVoices==8,"decoration cannot steal threat cues");
    audio.renderOffline(block.data(),4800);
    require(std::all_of(block.begin(),block.end(),[](float s){return std::isfinite(s)&&std::abs(s)<1;}),"overlap bounded");
    audio.stopSource(1);
    require(audio.diagnostics().criticalVoices==7,"source cancellation stops warning");
    audio.stopAll(); require(audio.diagnostics().activeVoices==0,"death clears voices");
    audio.play(AudioCue::PlayerDeath,{},0,false);
    std::vector<float> longBlock(48000*4);
    audio.renderOffline(longBlock.data(),96000);
    require(audio.diagnostics().activeVoices==0,"death cue finishes");
    audio.play(AudioCue::Ambience,{},0,false); audio.setMuted(true);
    audio.renderOffline(block.data(),4800);
    require(energy(block,0)==0 && std::string(audio.statusText())=="AUDIO MUTED","visible mute");
    audio.setMuted(false);
    const auto loaded=audio.diagnostics().loadedFrames;
    for(int i=0;i<50;++i) {
        audio.reset(); require(audio.diagnostics().activeVoices==0,"restart clears all voices");
        require(audio.diagnostics().loadedFrames==loaded,"restart reuses PCM");
        audio.play(AudioCue::Ambience,{},0,false); audio.play(AudioCue::RiftWarning,{},1);
        audio.setPaused(true); audio.reset();
        require(audio.play(AudioCue::PlayerShot,{},0,false),"restart clears pause");
        audio.renderOffline(block.data(),4800);
    }
    // Horror bed/transform must preserve measurable lateral threat contrast.
    // This checks mix routing/headroom, not a human audibility threshold.
    std::vector<float> warmup(72000*2), warningBlock(48000*2);
    for (const auto cue : {AudioCue::RiftWarning, AudioCue::SentryCharge}) {
        for (const float side : {-1.0f, 1.0f}) {
            audio.reset();
            audio.play(AudioCue::Ambience,{},0,false);
            audio.play(AudioCue::Transform,{},0,false);
            audio.renderOffline(warmup.data(),72000);
            audio.play(cue,{15*side,0,0},1,true);
            audio.renderOffline(warningBlock.data(),48000);
            const int nearChannel=side>0?1:0;
            const double contrast=energy(warningBlock,nearChannel)/energy(warningBlock,1-nearChannel);
            require(contrast>4,"15m warning keeps >6dB lateral contrast under transform and ambience");
        }
    }
    audio.reset(); audio.play(AudioCue::Ambience,{},0,false);
    std::vector<float> loopA(384000*2), loopB(384000*2);
    audio.renderOffline(loopA.data(),384000); audio.renderOffline(loopB.data(),384000);
    require(loopA==loopB,"ambient loop preserves exact repeated PCM across eight-second boundary");
    require(audio.diagnostics().activeVoices==1,"ambient loop remains active");
    audio.shutdown(); audio.shutdown();
    require(audio.diagnostics().loadedFrames==0 && !audio.available(),"shutdown frees PCM and is idempotent");
    std::cout << "Audio offline tests passed (headphone/runtime acceptance pending).\n";
}
#ifdef SILIC2_AUDIO_TEST_MAIN
int main() { try {runAudioTests();return 0;} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;} }
#endif
