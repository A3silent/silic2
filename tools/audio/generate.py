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
VERSION = 'silic2-audio-v2-cosmic-horror'
DURATIONS = {'player_shot': .18, 'hit': .105, 'kill': .32, 'player_hurt': .32,
             'player_death': 1.5, 'rift_warning': 1.2, 'sentry_charge': 1.0,
             'sentry_fire': .4, 'slide': .7, 'transform': 4., 'ambience': 8.,
             'ui_confirm': .18, 'victory': 1.6}


class BandPass:
    """Constant-peak biquad: dry breath/cavity texture without external samples."""
    def __init__(self, hz, q):
        w = TAU * hz / RATE
        alpha = math.sin(w) / (2*q)
        self.b = alpha / (1+alpha)
        self.a1 = -2*math.cos(w) / (1+alpha)
        self.a2 = (1-alpha) / (1+alpha)
        self.x1 = self.x2 = self.y1 = self.y2 = 0.

    def tick(self, x):
        y = self.b*(x-self.x2)-self.a1*self.y1-self.a2*self.y2
        self.x2, self.x1, self.y2, self.y1 = self.x1, x, self.y1, y
        return y


def synth(name, duration, seed):
    rng = random.Random(seed)
    samples = []
    low_noise = 0.
    cavity = BandPass(440, 1.4)
    rasp = BandPass(1750, .85)
    # All atmospheric partials lie on the 1/8 Hz grid. FM/AM periods divide
    # eight seconds, so both value and slope continue across the loop boundary.
    texture_rng = random.Random(seed + 991)
    partials = [(texture_rng.randrange(640, 3200)/8, texture_rng.uniform(0, TAU), texture_rng.uniform(.006, .016))
                for _ in range(14)]
    for i in range(round(duration * RATE)):
        t, u = i / RATE, i / (duration * RATE)
        noise = rng.uniform(-1, 1)
        low_noise = .92 * low_noise + .08 * noise
        body_noise = cavity.tick(noise)
        rasp_noise = rasp.tick(noise)
        attack = min(1., t / .008)
        release = min(1., (duration-t) / .025)
        env = attack * release
        tone = lambda hz: math.sin(TAU * hz * t)
        if name == 'player_shot':
            s = (.55*math.sin(TAU*(160*t-270*t*t)) + .55*noise*math.exp(-45*t))*math.exp(-23*t)
        elif name == 'hit':
            s = (.6*tone(1450)+.25*tone(2180)+.2*noise)*math.exp(-38*t)
        elif name == 'kill':
            s = (.48*tone(173)+.22*tone(291)+.28*rasp_noise)*math.exp(-16*t)
        elif name == 'player_hurt':
            s = (.65*tone(92)+.35*low_noise*3)*math.exp(-10*t)
        elif name == 'player_death':
            s = (.48*math.sin(TAU*(135*t-29*t*t)+.35*tone(31))+.6*body_noise+.22*rasp_noise*math.exp(-8*t))*math.exp(-2.8*t)
        elif name == 'rift_warning':
            # Uneven double-beat with rough cavity pulses; distinct from charge.
            pulse = ((.5+.5*math.cos(TAU*3.333333*t))**10
                     +.55*(.5+.5*math.cos(TAU*3.333333*t-1.8))**14)
            throat = math.tanh(1.8*math.sin(TAU*147*t+.65*tone(37)))
            edge = math.sin(TAU*720*t+.8*tone(113))
            s = (.30*throat+.32*edge+.65*rasp_noise)*(.30+.65*pulse)*(.65+.35*u)
        elif name == 'sentry_charge':
            # A coarse rising formant, not a clean musical chirp. Keep 700–2000
            # Hz energy for stereo warning readability above the low ambient bed.
            phase = TAU*(570*t+380*t*t)+1.3*tone(93)
            pulse = .45+.55*(.5+.5*math.sin(TAU*(3*t+5*t*t)))**2
            lock = 1.0 if t < .65 else 1.14
            s = (.40*math.tanh(1.7*math.sin(phase))+.19*tone(117)
                 +.75*rasp_noise)*pulse*(.6+.4*u)*lock
        elif name == 'sentry_fire':
            s = (.5*math.sin(TAU*(260*t-170*t*t))+.45*noise*math.exp(-18*t))*math.exp(-9*t)
        elif name == 'slide':
            s = (noise*.15+low_noise*1.5)*math.sin(math.pi*u)**.7*.65
        elif name == 'transform':
            # Stressed structural ribs and an inward organ contraction, four seconds.
            pressure = math.sin(math.pi*u)**1.3
            scrape = (.5+.5*math.sin(TAU*(.8*t+.14*t*t)))**5
            s = (.30*math.sin(TAU*(48*t-1.7*t*t)+.6*tone(2.3))
                 +.20*math.sin(TAU*(121*t-5*t*t)+.9*tone(31))
                 +.15*tone(207.3)*math.exp(-.7*t)
                 +1.1*body_noise*(.3+.7*scrape)+.14*rasp_noise*scrape)*pressure
        elif name == 'ambience':
            # Inharmonic metal ribs, slow pressure and dry tissue movement.
            # No nonperiodic noise in this eight-second looping asset.
            breath = .57+.15*math.cos(TAU*t/8)+.09*math.sin(TAU*t/4)
            membrane = .19*math.sin(TAU*41.5*t+.65*math.sin(TAU*t/4))
            membrane += .12*math.sin(TAU*57*t+.4*math.sin(TAU*t/8))
            ribs = .065*tone(83.125)+.04*tone(137.375)+.025*tone(213.875)
            grain = sum(a*math.sin(TAU*f*t+phase) for f,phase,a in partials)
            s = (membrane+ribs+grain*(.5+.5*math.sin(TAU*t/8)**6))*breath
            env = 1.
        elif name == 'ui_confirm':
            # Muted mechanical detent, not a bright confirmation note.
            s = (.4*tone(183)+.22*tone(327)+.5*rasp_noise)*math.exp(-29*t)
        elif name == 'victory':
            # A pressure seal closing with one dull secondary latch; no fanfare.
            latch = max(0.,t-.22)
            s = (.30*tone(87)+.24*tone(139)+.55*body_noise)*math.exp(-4*t)
            if t >= .22:
                s += .18*math.sin(TAU*263*latch)*min(1.,latch/.004)*math.exp(-26*latch)
        samples.append(max(-.95, min(.95, s*env*.75)))
    return samples


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    manifest = {'content_version': VERSION, 'sample_rate': RATE,
                'channels': 1, 'bits': 16, 'origin': 'Original offline mathematical synthesis; no samples or recordings.', 'clips': []}
    for seed, (name,duration) in enumerate(DURATIONS.items(), 4200):
        samples = synth(name,duration,seed)
        path = OUT / (name+'.wav')
        pcm = [round(s*32767) for s in samples]
        delivered = [s/32768 for s in pcm]
        with wave.open(str(path),'wb') as out:
            out.setparams((1,2,RATE,0,'NONE','not compressed'))
            out.writeframes(b''.join(struct.pack('<h',s) for s in pcm))
        manifest['clips'].append({'file':path.name,'seconds':duration,'frames':len(samples),
                                  'peak':round(max(map(abs,delivered)),8),
                                  'rms':round(math.sqrt(sum(s*s for s in delivered)/len(delivered)),8),
                                  'mean':round(sum(delivered)/len(delivered),8),
                                  'loop': name == 'ambience',
                                  'boundary_step_lsb': abs(pcm[0]-pcm[-1]),
                                  'boundary_slope_error_lsb': abs((pcm[0]-pcm[-1])-(pcm[1]-pcm[0])),
                                  'max_adjacent_step_lsb': max(abs(b-a) for a,b in zip(pcm,pcm[1:])),
                                  'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
    (OUT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(f'Generated {len(DURATIONS)} original mono 48kHz PCM16 WAVs in {OUT}')

if __name__ == '__main__':
    main()
