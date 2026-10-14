#!/usr/bin/env python3
"""Validate delivered PCM and compare it with a fresh deterministic synthesis."""
import hashlib
import json
import math
import struct
import wave
from generate import DURATIONS, OUT, VERSION, RATE, synth

manifest = json.loads((OUT/'manifest.json').read_text())
assert manifest['content_version'] == VERSION
assert len(manifest['clips']) == len(DURATIONS)
for seed, ((name, seconds), clip) in enumerate(zip(DURATIONS.items(), manifest['clips']), 4200):
    path = OUT / (name+'.wav')
    assert clip['file'] == path.name
    assert clip['frames'] == round(seconds*RATE) and clip['seconds'] == seconds
    assert hashlib.sha256(path.read_bytes()).hexdigest() == clip['sha256']
    with wave.open(str(path), 'rb') as wav:
        assert (wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getnframes()) == (1,2,RATE,clip['frames'])
        data = wav.readframes(wav.getnframes())
    pcm = struct.unpack('<'+'h'*clip['frames'], data)
    fresh = b''.join(struct.pack('<h',round(s*32767)) for s in synth(name,seconds,seed))
    assert data == fresh, f'{name}: regenerated PCM differs'
    peak = max(map(abs,pcm))/32768
    rms = math.sqrt(sum(s*s for s in pcm)/len(pcm))/32768
    assert abs(peak-clip['peak']) < 1e-8 and abs(rms-clip['rms']) < 1e-8
    assert peak < .95 and abs(sum(pcm)/len(pcm)/32768) < .002
    if clip['loop']:
        step = abs(pcm[0]-pcm[-1])
        slope_error = abs((pcm[0]-pcm[-1])-(pcm[1]-pcm[0]))
        assert step == clip['boundary_step_lsb'] and slope_error == clip['boundary_slope_error_lsb']
        assert step <= clip['max_adjacent_step_lsb'] and slope_error <= 8
        print(f'{name}: loop boundary step {step} LSB; slope error {slope_error} LSB')
print('PASS: 13 PCM/hash/format/duration/peak/RMS/DC checks and fresh deterministic synthesis; no device opened.')
