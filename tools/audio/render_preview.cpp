// CPU-only rendering through the shipping mixer. Never opens an audio device.
#include "audio/audio_system.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace silic2;
namespace {
void check(bool value, const char* text) { if (!value) throw std::runtime_error(text); }
double energy(const std::vector<float>& data, int channel) {
    double sum=0; for (size_t i=channel;i<data.size();i+=2) sum+=data[i]*data[i]; return sum;
}
void wav(const char* path, const std::vector<float>& data) {
    std::ofstream out(path,std::ios::binary);
    check(bool(out),"cannot write preview");
    auto word=[&](uint32_t value,int bytes){for(int i=0;i<bytes;++i)out.put(char((value>>(i*8))&255));};
    out.write("RIFF",4);word(36+data.size()*2,4);out.write("WAVEfmt ",8);
    word(16,4);word(1,2);word(2,2);word(48000,4);word(192000,4);word(4,2);word(16,2);
    out.write("data",4);word(data.size()*2,4);
    for(float sample:data)word(static_cast<uint16_t>(static_cast<int16_t>(std::lround(std::clamp(sample,-1.f,1.f)*32767))),2);
    check(bool(out),"preview write failed");
}
}
int main() {
    try {
        AudioSystem audio;
        check(audio.initialize("res/audio",false),"offline preload failed");
        audio.setListener({0,0,0},{0,0,-1});
        std::ofstream report("tools/audio/previews/mix-metrics.json");
        report << "{\n  \"content_version\": \"silic2-audio-v2-cosmic-horror\",\n  \"device_opened\": false,\n  \"warning_checks\": [\n";
        // Environment at full transformation intensity, with a spatial warning 15m away.
        // Compare the actual limited stereo output. This is not perceptual validation.
        std::vector<float> warmup(72000*2), probe(48000*2);
        for(int cue=0;cue<2;++cue) {
            audio.reset();
            audio.play(AudioCue::Ambience,{},0,false);
            audio.play(AudioCue::Transform,{},0,false);
            audio.renderOffline(warmup.data(),72000);
            audio.play(cue==0?AudioCue::RiftWarning:AudioCue::SentryCharge,{15,0,0},1,true);
            audio.renderOffline(probe.data(),48000);
            const double db=10*std::log10(energy(probe,1)/energy(probe,0));
            std::cout<<(cue==0?"rift_warning":"sentry_charge")<<" 15m stereo contrast: "<<db<<" dB\n";
            check(db>6,"warning right/left energy contrast below 6 dB under atmosphere");
            report << "    {\"cue\": \""<<(cue==0?"rift_warning":"sentry_charge")<<"\", \"distance\": 15, \"right_over_left_db\": "<<db<<"}"<<(cue==0?",":"")<<"\n";
        }
        // Ten-second audition sequence: ambience; transformation; left rift;
        // right charge/fire; dry confirmations; ambience crossing its loop boundary.
        audio.reset();
        std::vector<float> mix(480000*2);
        for(size_t frame=0;frame<480000;frame+=480) {
            if(frame==0)audio.play(AudioCue::Ambience,{},0,false);
            if(frame==48000)audio.play(AudioCue::Transform,{},0,false);
            if(frame==96000)audio.play(AudioCue::RiftWarning,{-8,0,0},1,true);
            if(frame==163200)audio.play(AudioCue::SentryCharge,{8,0,0},2,true);
            if(frame==201600)audio.play(AudioCue::PlayerShot,{},0,false);
            if(frame==211200){audio.stopSource(2);audio.play(AudioCue::SentryFire,{8,0,0},2,true);}
            if(frame==288000)audio.play(AudioCue::UiConfirm,{},0,false);
            if(frame==312000)audio.play(AudioCue::Kill,{},0,false);
            if(frame==336000)audio.play(AudioCue::Victory,{},0,false);
            audio.renderOffline(mix.data()+frame*2,480);
        }
        float peak=0; double total=0;
        for(float sample:mix){check(std::isfinite(sample)&&std::abs(sample)<1,"invalid output");peak=std::max(peak,std::abs(sample));total+=sample*sample;}
        wav("tools/audio/previews/cosmic-horror-mix.wav",mix);
        report << "  ],\n  \"preview_seconds\": 10,\n  \"preview_peak\": "<<peak<<",\n  \"preview_rms\": "<<std::sqrt(total/mix.size())<<",\n  \"loaded_frames\": "<<audio.diagnostics().loadedFrames<<"\n}\n";
        check(bool(report),"report write failed");
        std::cout<<"Offline preview and warning contrast PASS; no playback device opened.\n";
    } catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
