#!/usr/bin/env python3
"""Original Silic2 sounds. Python standard library only; never run during combat."""
import hashlib
import json
import math
from pathlib import Path
import random
import struct
import wave

RATE = 48000
ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'res/audio'
TAU = math.tau
DURATIONS = {'player_shot': .18, 'hit': .105, 'kill': .32, 'player_hurt': .32,
             'player_death': 1.5, 'rift_warning': 1.2, 'sentry_charge': 1.0,
             'sentry_fire': .4, 'slide': .7, 'transform': 4., 'ambience': 8.,
             'ui_confirm': .18, 'victory': 1.6}


def synth(name, duration, seed):
    rng = random.Random(seed)
    samples = []
    low_noise = 0.
    for i in range(round(duration * RATE)):
        t, u = i / RATE, i / (duration * RATE)
        noise = rng.uniform(-1, 1)
        low_noise = .92 * low_noise + .08 * noise
        attack = min(1., t / .008)
        release = min(1., (duration-t) / .025)
        env = attack * release
        tone = lambda hz: math.sin(TAU * hz * t)
        if name == 'player_shot':
            s = (.55*math.sin(TAU*(160*t-270*t*t)) + .55*noise*math.exp(-45*t))*math.exp(-23*t)
        elif name == 'hit':
            s = (.6*tone(1450)+.25*tone(2180)+.2*noise)*math.exp(-38*t)
        elif name == 'kill':
            s = (.5*tone(720)+.3*tone(1080)+.2*tone(1440))*math.exp(-10*t)
        elif name == 'player_hurt':
            s = (.65*tone(92)+.35*low_noise*3)*math.exp(-10*t)
        elif name == 'player_death':
            s = (.5*math.sin(TAU*(180*t-43*t*t))+.4*low_noise)*math.exp(-2.8*t)
        elif name == 'rift_warning':
            pulse = (.5+.5*math.cos(TAU*5*t))**3
            s = (.5*tone(530)+.22*tone(795))*(.25+.75*pulse)*(.55+.45*u)
        elif name == 'sentry_charge':
            phase = TAU*(650*t+450*t*t)
            # Increasing pulse rate and pitch communicate the one-second countdown.
            pulse = .35+.65*(.5+.5*math.sin(TAU*(3*t+5*t*t)))**2
            s = (.6*math.sin(phase)+.18*math.sin(2*phase))*pulse*(.55+.45*u)
        elif name == 'sentry_fire':
            s = (.5*math.sin(TAU*(260*t-170*t*t))+.45*noise*math.exp(-18*t))*math.exp(-9*t)
        elif name == 'slide':
            s = (noise*.15+low_noise*1.5)*math.sin(math.pi*u)**.7*.65
        elif name == 'transform':
            s = (.27*tone(65)+.18*math.sin(TAU*(160*t+28*t*t))+.12*tone(240)+.3*low_noise)*math.sin(math.pi*u)**1.5
        elif name == 'ambience':
            # Integer cycles in eight seconds yield a seam-free periodic ambient bed.
            s = .32*tone(55)+.16*tone(82.5)+.08*tone(165)+.06*tone(220)
            s *= .65+.15*math.cos(TAU*t/8)
            env = 1.
        elif name == 'ui_confirm':
            s = (.6*tone(880)+.25*tone(1320))*math.exp(-20*t)
        elif name == 'victory':
            s = sum(.23*math.sin(TAU*hz*t)*max(0., min(1., (t-start)/.02))*math.exp(-3*max(0.,t-start))
                    for hz,start in ((440,0),(550,.15),(660,.3),(880,.45)))
        samples.append(max(-.95, min(.95, s*env*.75)))
    return samples


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    manifest = {'content_version': 'silic2-audio-v1', 'sample_rate': RATE,
                'channels': 1, 'bits': 16, 'origin': 'Original offline mathematical synthesis; no samples or recordings.', 'clips': []}
    for seed, (name,duration) in enumerate(DURATIONS.items(), 4200):
        samples = synth(name,duration,seed)
        path = OUT / (name+'.wav')
        with wave.open(str(path),'wb') as out:
            out.setparams((1,2,RATE,0,'NONE','not compressed'))
            out.writeframes(b''.join(struct.pack('<h',round(s*32767)) for s in samples))
        manifest['clips'].append({'file':path.name,'seconds':duration,'frames':len(samples),
                                  'peak':round(max(map(abs,samples)),6),
                                  'rms':round(math.sqrt(sum(s*s for s in samples)/len(samples)),6),
                                  'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
    (OUT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(f'Generated {len(DURATIONS)} original mono 48kHz PCM16 WAVs in {OUT}')

if __name__ == '__main__':
    main()
