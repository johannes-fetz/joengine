"""
The sounds of the first person shooter demo: cd/*.WAV (PCM 16 bits mono, little endian: the engine converts them for the SCSP, see
jo_audio_load_wav(); they can be listened to on a computer). Made here: the impacts of
the bullets, the empty trigger, and the echo of the rooms of the laboratory (concrete,
tiles). Free recordings (sounds/): the pistol (a 9 mm shot), the rifle (an AK-47), the reload, the pickups (a magazine
in), the doors, the cries of the guards (pain and death: the 4 cries of one recording), the grunt of the agent.

The methods:
- a shot: the blast (a pressure wave of 1 ms, then a crack of noise), the body of the explosion (a low thump and noise
  under 1.2 kHz), the action of the weapon (metal clicks), a saturation (the loudness of a shot), then the room;
- metal (clicks, the latch of a door, a magazine): modal synthesis, damped sines at inharmonic frequencies;
- the room: an impulse response made of early reflections (the walls of a room of the laboratory) and a diffuse tail of
  noise whose high frequencies die faster (tiles and concrete: RT60 about 0.7 s, 0.35 s above 4 kHz);

The echo of the rooms: made by the DSP of the SCSP when the game plays (DSP_ECHO, the reverberation of the engine: the
echo of the room where the agent is, see update_room_echo() in main.c): the sounds are dry, shorter (less sound RAM).
The ricochets: the impact, and the whine of the bullet made by the SCSP (a tone: play_ricochet() in weapon.c).

Run by tools/make_assets.py (build_sounds()), or alone: python3 tools/make_sounds.py
"""
import math
import os
import struct

import numpy as np
from scipy.signal import butter, lfilter, sosfilt

DEMO = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
CD = os.path.join(DEMO, 'cd')

# The echo of the rooms made by the DSP of the SCSP (the game is built with JO_COMPILE_WITH_AUDIO_DSP_MODULE = 1): the
# sounds without it. False: the echo in the sounds (a game without the DSP module)
DSP_ECHO = True

RATE = 22050                    # The sample rate of the effects (Hz)
VOICE_RATE = 11025              # The voices: their formants are under 4.5 kHz (less sound RAM)
MECHANICS_RATE = 16000          # The clicks of the weapons, the doors: up to 8 kHz
LOW_RATE = 11025                # The long sounds without much over 5 kHz: the reloads, the shots of the guards (far: the air took their highs)


# ---------------------------------------------------------------------------
# Tools
# ---------------------------------------------------------------------------
def save_wav(name, x, rate=RATE, peak=0.95):
    """cd/NAME: the sound normalized to peak, 16 bits mono, a little TPDF dither (no distortion of the quiet tails)"""
    x = np.asarray(x, dtype=float)
    m = np.max(np.abs(x))
    if m > 0:
        x = x / m * peak
    rng = np.random.default_rng(len(name))
    d = (rng.random(len(x)) - rng.random(len(x))) / 32767.0
    data = np.clip(np.round((x + d) * 32767), -32768, 32767).astype('<i2').tobytes()
    with open(os.path.join(CD, name), 'wb') as f:
        f.write(b'RIFF' + struct.pack('<I', 36 + len(data)) + b'WAVE')
        f.write(b'fmt ' + struct.pack('<IHHIIHH', 16, 1, 1, rate, rate * 2, 2, 16))
        f.write(b'data' + struct.pack('<I', len(data)) + data)
    return len(data)


def times(duration, rate=RATE):
    return np.arange(int(duration * rate)) / rate


def filt(x, lo=None, hi=None, order=2, rate=RATE):
    """Butterworth filter: lo only: high pass, hi only: low pass, both: band pass"""
    nyq = rate / 2
    if lo and hi:
        sos = butter(order, [lo / nyq, min(hi, nyq * 0.95) / nyq], btype='band', output='sos')
    elif lo:
        sos = butter(order, lo / nyq, btype='high', output='sos')
    else:
        sos = butter(order, min(hi, nyq * 0.95) / nyq, btype='low', output='sos')
    return sosfilt(sos, x)


def at(x, n, start, rate=RATE):
    """x placed at start (s) in a buffer of n samples"""
    out = np.zeros(n)
    s = int(round(start * rate))
    if s >= n:
        return out
    m = min(len(x), n - s)
    out[s:s + m] = x[:m]
    return out


def decay(t, tau, attack=0.0005):
    return np.minimum(1.0, t / max(attack, 1e-5)) * np.exp(-t / tau)


def modal(duration, modes, rng, rate=RATE):
    """Damped sines: modes = [(frequency, decay (s), amplitude)], random phases, the frequencies a little detuned"""
    t = times(duration, rate)
    x = np.zeros_like(t)
    for f, tau, a in modes:
        f = f * (1 + rng.normal(0, 0.01))
        x += a * np.sin(2 * np.pi * f * t + rng.uniform(0, 2 * np.pi)) * decay(t, tau, 0.0002)
    return x


def click(duration, rng, lo=2000, hi=9000, tau=0.002, rate=RATE):
    """A short burst of noise (the contact of 2 metal parts)"""
    t = times(duration, rate)
    return filt(rng.normal(0, 1, len(t)), lo, hi, rate=rate) * decay(t, tau, 0.0001)


def metal_click(rng, strength=1.0, low=1.0, high=1.0, rate=RATE):
    """A click of a small steel part (a trigger, a slide, a latch): a contact and the ringing of its modes"""
    d = 0.09
    modes = [(1850 * low, 0.012, 0.5), (2650, 0.009, 0.8), (3700 * high, 0.007, 1.0), (5200 * high, 0.005, 0.7),
             (7100 * high, 0.004, 0.45), (950 * low, 0.02, 0.35 * low)]
    x = modal(d, modes, rng, rate) * 0.5 + click(d, rng, 1500, 9500, 0.0015, rate)
    return x * strength


def thump(duration, f0, f1, tau, rate=RATE):
    """A low thump: a sine falling from f0 to f1 (Hz)"""
    t = times(duration, rate)
    f = f1 + (f0 - f1) * np.exp(-t / (tau * 0.7))
    phase = 2 * np.pi * np.cumsum(f) / rate
    return np.sin(phase) * decay(t, tau, 0.001)


def soft_clip(x, drive):
    """The loudness of a shot: a saturation (tanh)"""
    m = np.max(np.abs(x))
    if m <= 0:
        return x
    return np.tanh(x / m * drive) / np.tanh(drive)


ROOMS = {
    # (the size of the room (m), RT60 under 1 kHz and above 4 kHz (s), the level of the reflections)
    # The washroom: tiles everywhere, 19 x 16 x 3.6 m
    'washroom': ((19.2, 16.0, 3.6), 0.85, 0.40, 0.9),
    # A concrete room (the corridor, the storage room): a little duller
    'concrete': ((12.8, 6.4, 3.6), 0.65, 0.28, 0.75),
}


def room_response(name, duration, seed, rate=RATE):
    """The impulse response of a room: early reflections (the 6 walls, the first 2 orders of images of a source and a
    listener near its middle), then a diffuse tail of noise with an echo density growing like t^2, whose bands die at
    their own rates (the high ones faster: the air, the absorption)"""
    (lx, ly, lz), rt_low, rt_high, level = ROOMS[name]
    rng = np.random.default_rng(seed)
    n = int(duration * rate)
    h = np.zeros(n)
    c = 343.0
    # (x across, y up, z along: a source and a listener near the middle, at the height of a man)
    dims = np.array((lx, lz, ly))
    src = np.array((lx * 0.45, 1.3, ly * 0.5))
    lis = np.array((lx * 0.55, 1.7, ly * 0.42))
    # (images of the source: per axis, mirrored k times by the 2 walls)
    for ix in range(-2, 3):
        for iy in range(-1, 2):
            for iz in range(-2, 3):
                order = abs(ix) + abs(iy) + abs(iz)
                if order == 0 or order > 3:
                    continue
                img = [k * dims[a] + (src[a] if k % 2 == 0 else dims[a] - src[a]) for a, k in enumerate((ix, iy, iz))]
                dist = np.linalg.norm(np.array(img) - lis)
                idx = int(dist / c * rate)
                if idx < n:
                    h[idx] += (0.8 ** order) / max(dist, 1.0) * (1 if rng.random() > 0.3 else -1)
    # The diffuse tail: from 30 ms, in 4 bands
    t = np.arange(n) / rate
    dens = np.clip((t / 0.06) ** 2, 0, 1)
    tail = np.zeros(n)
    bands = [(None, 500, rt_low), (500, 1500, rt_low * 0.95), (1500, 4000, (rt_low + rt_high) / 2), (4000, None, rt_high)]
    for lo, hi, rt in bands:
        noise = rng.normal(0, 1, n)
        noise = filt(noise, lo, hi, 2, rate) if (lo or hi) else noise
        tail += noise * np.exp(-6.91 * t / rt)
    tail *= dens * (t > 0.008)
    early = filt(h, 120, 7000, 1, rate)
    h = early / max(np.max(np.abs(early)), 1e-9) * level + tail / np.max(np.abs(tail)) * 0.22
    h[0] = 0
    return h


def with_room(x, room, wet, duration, seed, rate=RATE, predelay=0.0):
    """The sound in a room: dry + wet (the convolution with its response), the end faded out. DSP_ECHO: the dry sound
    (the DSP makes the echo)"""
    # (the end of the dry sound faded: a sound cut while it rings clicks)
    x = fade_end(np.asarray(x, dtype=float), rate, 0.03)
    if DSP_ECHO:
        return fade_end(trim(x, rate), rate, 0.02)
    h = room_response(room, duration, seed, rate)
    from scipy.signal import fftconvolve
    y = fftconvolve(x, h)[:len(x) + len(h) - 1]
    y = y / max(np.max(np.abs(y)), 1e-9) * np.max(np.abs(x))
    out = np.zeros(len(y))
    out[:len(x)] += x
    pd = int(predelay * rate)
    out[pd:] += wet * y[:len(y) - pd]
    return fade_end(trim(out, rate), rate)


def trim(x, rate=RATE, floor_db=-45):
    """The end cut where the sound stays under floor_db of its peak"""
    m = np.max(np.abs(x))
    if m <= 0:
        return x
    env = np.abs(x) > m * 10 ** (floor_db / 20)
    last = np.nonzero(env)[0]
    end = int(last[-1]) + int(0.01 * rate) if len(last) else len(x)
    return x[:min(end, len(x))]


def fade_end(x, rate=RATE, length=0.04):
    n = min(len(x), int(length * rate))
    x = x.copy()
    x[-n:] *= np.linspace(1, 0, n) ** 2
    return x


# ---------------------------------------------------------------------------
# The weapons
# ---------------------------------------------------------------------------
def sound_empty():
    """The trigger pulled on an empty weapon: the trigger, the hammer on nothing (2 dry clicks)"""
    rng = np.random.default_rng(5)
    n = int(0.12 * RATE)
    x = at(metal_click(rng, 0.5, 1.2, 1.2), n, 0.0) + at(metal_click(rng, 1.0, 1.0, 1.25), n, 0.018)
    return fade_end(x)


# ---------------------------------------------------------------------------
# The impacts of the bullets
# ---------------------------------------------------------------------------
def sound_hit(seed):
    """A bullet in the tiles or the concrete: the crack of the impact, the chips and the dust falling (small clicks
    getting fewer and quieter)"""
    rng = np.random.default_rng(60 + seed)
    n = int(0.3 * RATE)
    t = times(0.3)
    x = filt(rng.normal(0, 1, n), 1200, 10000) * decay(t, 0.004, 0.0001) * 1.0
    x += filt(rng.normal(0, 1, n), 250, 2500) * decay(t, 0.012, 0.0003) * 0.7
    x += thump(0.3, 220, 120, 0.012) * 0.4
    # The chips
    for k in range(14):
        when = 0.008 + rng.exponential(0.05)
        if when > 0.25:
            continue
        g = 0.35 * math.exp(-when / 0.07) * rng.uniform(0.4, 1.0)
        x += at(click(0.02, rng, 2500, 10000, 0.0008) * g, n, when)
    return with_room(soft_clip(x, 1.3), 'washroom', 0.22, 0.4, 20 + seed)


def sound_ricochet(seed):
    """A ricochet: the impact, then the bullet whistling away (a tone falling with the Doppler effect, tumbling: its
    loudness beats, with a breath of noise around it)"""
    rng = np.random.default_rng(70 + seed)
    d = 0.45
    n = int(d * RATE)
    t = times(d)
    imp = filt(rng.normal(0, 1, n), 1500, 10000) * decay(t, 0.003, 0.0001)
    f0 = rng.uniform(2600, 3400)
    f1 = f0 * rng.uniform(0.45, 0.6)
    f = f1 + (f0 - f1) * np.exp(-t / rng.uniform(0.12, 0.2))
    f *= 1 + 0.012 * np.sin(2 * np.pi * rng.uniform(25, 45) * t)
    phase = 2 * np.pi * np.cumsum(f) / RATE
    tumble = 0.6 + 0.4 * np.sin(2 * np.pi * rng.uniform(30, 55) * t)
    env = np.minimum(1, t / 0.012) * np.exp(-t / rng.uniform(0.16, 0.22))
    whine = (np.sin(phase) + 0.25 * np.sin(2 * phase + 0.7)) * tumble * env
    breath = filt(rng.normal(0, 1, n), 1500, 6000) * env * 0.25
    x = imp * 1.0 + whine * 0.55 + breath
    return with_room(x, 'washroom', 0.3, 0.45, 30 + seed)


def sound_hit_metal():
    """A bullet on a steel door: the impact and the ring of the panel (low modes, slow)"""
    rng = np.random.default_rng(80)
    d = 0.45
    n = int(d * RATE)
    t = times(d)
    x = filt(rng.normal(0, 1, n), 1000, 10000) * decay(t, 0.003, 0.0001) * 0.9
    x += modal(d, [(385, 0.28, 0.6), (612, 0.22, 0.5), (1047, 0.18, 0.55), (1530, 0.12, 0.45), (2290, 0.09, 0.4),
                   (3180, 0.06, 0.3), (4420, 0.04, 0.25)], rng) * 0.8
    return with_room(soft_clip(x, 1.2), 'washroom', 0.2, 0.35, 40)


# ---------------------------------------------------------------------------
# The recordings (sounds/: free sounds of Pixabay, Pixabay Content License: free to use, no attribution required;
# converted from their MP3 to mono WAV, see sounds/CREDITS.txt): the doors, the AK-47 (the rifle), the reload, the
# magazine (the pickups)
# ---------------------------------------------------------------------------
DOOR_RECORDING = os.path.join(DEMO, 'sounds', 'door_open_close.wav')
SLIDE_RECORDING = os.path.join(DEMO, 'sounds', 'slide_door.wav')
GLASS_RECORDING = os.path.join(DEMO, 'sounds', 'broken_glass.wav')


def read_wav(path):
    """A WAV file (16 bits): its samples (-1 to 1, mono) and its rate"""
    import wave
    with wave.open(path) as w:
        rate = w.getframerate()
        x = np.frombuffer(w.readframes(w.getnframes()), '<i2').astype(float) / 32768.0
        if w.getnchannels() == 2:
            x = x.reshape(-1, 2).mean(axis=1)
    return x, rate


def recorded_door(pitch=1.0):
    """The opening and the closing of the recording (the 2 parts between its silences), at RATE; pitch > 1: higher and
    shorter"""
    from scipy.signal import resample_poly
    x, rate = read_wav(DOOR_RECORDING)
    # (the rate of the recording divided by the pitch: played at RATE, it is pitch times higher)
    target = int(round(RATE * pitch))
    g = math.gcd(target, rate)
    x = resample_poly(x, target // g, rate // g)
    # The parts: where the sound is over -40 dB of its peak (smoothed on 50 ms), separated by 0.3 s of silence at least
    env = np.convolve(np.abs(x), np.ones(int(0.05 * RATE)) / (0.05 * RATE), mode='same')
    loud = env > np.max(env) * 0.01
    parts = []
    start = None
    quiet = 0
    for k, v in enumerate(loud):
        if v:
            if start is None:
                start = k
            quiet = 0
        elif start is not None:
            quiet += 1
            if quiet > 0.3 * RATE:
                parts.append((start, k - quiet))
                start = None
    if start is not None:
        parts.append((start, len(x)))
    if len(parts) < 2:
        raise SystemExit('%s: an opening and a closing expected (2 parts separated by silence)' % DOOR_RECORDING)
    out = []
    for a, b in parts[:2]:
        a = max(0, a - int(0.003 * RATE))
        b = min(len(x), b + int(0.08 * RATE))
        part = x[a:b].copy()
        n = int(0.005 * RATE)
        part[:n] *= np.linspace(0, 1, n)
        out.append(fade_end(part, RATE, 0.05))
    return out[0], out[1]


AK47_RECORDING = os.path.join(DEMO, 'sounds', 'ak47_firing.wav')
RELOAD_RECORDING = os.path.join(DEMO, 'sounds', 'gun_reload.wav')
PICKUP_RECORDING = os.path.join(DEMO, 'sounds', 'handgun_clip_in.wav')
PISTOL_RECORDING = os.path.join(DEMO, 'sounds', 'gunshot_9mm.wav')
# The cries of pain of the guards: the 4 cries of one recording (guards.c plays them a little higher or lower each time)
PAIN_RECORDING = os.path.join(DEMO, 'sounds', 'guard_pain_noises.wav')
PAIN_COUNT = 4
# The grunt of the agent when he is hit
GRUNT_RECORDING = os.path.join(DEMO, 'sounds', 'player_grunt.wav')


def recording(path):
    """A recording at RATE"""
    from scipy.signal import resample_poly
    x, rate = read_wav(path)
    g = math.gcd(RATE, rate)
    return resample_poly(x, RATE // g, rate // g)


def sound_rifle(distant=False, seed=0):
    """The rifle: one shot of the recording of an AK-47 firing a burst, its last one (its echo is not covered by the next
    shot), in the laboratory (its rooms). Fired by a guard (distant): the air took the highest frequencies, the room is
    louder than the shot"""
    x = recording(AK47_RECORDING)
    env = np.convolve(np.abs(x), np.ones(int(0.002 * RATE)) / (0.002 * RATE), mode='same')
    # (the shots: the peaks of the envelope, 50 ms apart at least; the start of the last one: 3 ms before its peak)
    peaks = [k for k in range(1, len(env) - 1) if env[k] > 0.3 * np.max(env) and env[k] >= env[k - 1] and env[k] >= env[k + 1]]
    starts = []
    for k in peaks:
        if not starts or k - starts[-1] > 0.05 * RATE:
            starts.append(k)
    shot = x[max(0, starts[-1] - int(0.003 * RATE)):].copy()
    n = int(0.002 * RATE)
    shot[:n] *= np.linspace(0, 1, n)
    shot = fade_end(trim(shot), RATE, 0.03)
    if distant:
        # (2 distances: the air took more of the highs of the second one)
        shot = filt(shot, None, 4200 if seed % 2 == 0 else 3000, 2)
        return with_room(shot, 'washroom' if seed % 2 == 0 else 'concrete', 0.5, 0.7, 10 + seed, predelay=0.006)
    return with_room(shot, 'washroom', 0.22, 0.7, 2)


def recording_parts(path, count, gap=0.15):
    """The sounds of a recording separated by silences (-30 dB of its peak, smoothed on 20 ms, gap s at least), their
    silences cut (5 ms kept before, the end faded)"""
    x = recording(path)
    env = np.convolve(np.abs(x), np.ones(int(0.02 * RATE)) / (0.02 * RATE), mode='same')
    loud = env > np.max(env) * 10 ** (-30 / 20)
    parts = []
    start = None
    quiet = 0
    for k, v in enumerate(loud):
        if v:
            if start is None:
                start = k
            quiet = 0
        elif start is not None:
            quiet += 1
            if quiet > gap * RATE:
                parts.append((start, k - quiet))
                start = None
    if start is not None:
        parts.append((start, len(x)))
    if len(parts) != count:
        raise SystemExit('%s: %d sounds expected, %d found' % (path, count, len(parts)))
    out = []
    for a, b in parts:
        part = x[max(0, a - int(0.005 * RATE)):min(len(x), b + int(0.06 * RATE))].copy()
        n = int(0.003 * RATE)
        part[:n] *= np.linspace(0, 1, n)
        out.append(fade_end(trim(part), RATE, 0.04))
    return out


def sound_recording(path, start_pad=0.005):
    """A recording, its silences at the start (kept: start_pad s) and at the end cut"""
    x = recording(path)
    m = np.max(np.abs(x))
    loud = np.nonzero(np.abs(x) > m * 0.02)[0]
    a = max(0, int(loud[0]) - int(start_pad * RATE))
    x = x[a:]
    n = int(0.003 * RATE)
    x[:n] *= np.linspace(0, 1, n)
    return fade_end(trim(x), RATE, 0.04)


def door_close_hit_ms():
    """When the door hits its frame in the closing sound (ms, its loudest moment): the game starts the sound so that
    it hits when the door is closed (the property close_hit of the doors)"""
    close = recorded_door()[1]
    return int(round(np.argmax(np.abs(close)) * 1000.0 / RATE))


# ---------------------------------------------------------------------------
def build_sounds():
    """All the sounds: cd/*.WAV (the old cd/*.PCM removed)"""
    total = 0
    made = []

    def out(name, x, rate=RATE, store=None):
        # (store: the rate of the file, lower for the sounds without high frequencies: less sound RAM)
        nonlocal total
        if store is not None and store != rate:
            from scipy.signal import resample_poly
            g = math.gcd(store, rate)
            x = resample_poly(x, store // g, rate // g)
            rate = store
        total += save_wav(name, x, rate)
        made.append(name.split('.')[0])
    # (the pistol: the recording of a 9 mm shot, its silences cut, in the room)
    out('SHOT.WAV', with_room(sound_recording(PISTOL_RECORDING), 'washroom', 0.18, 0.5, 1))
    out('RIFLE.WAV', sound_rifle())
    out('GSHOT1.WAV', sound_rifle(True, 0), store=LOW_RATE)
    out('GSHOT2.WAV', sound_rifle(True, 1), store=LOW_RATE)
    out('EMPTY.WAV', sound_empty(), store=MECHANICS_RATE)
    # (the reload of both weapons: no silence at the start (no delay after the button), at the end (less sound RAM))
    out('RELOAD.WAV', sound_recording(RELOAD_RECORDING), store=MECHANICS_RATE)
    out('PICKUP.WAV', sound_recording(PICKUP_RECORDING), store=MECHANICS_RATE)
    out('HIT1.WAV', sound_hit(0))
    out('HIT2.WAV', sound_hit(1))
    if not DSP_ECHO:
        # (with the DSP module, the game makes the whine of a ricochet: a tone of the SCSP after the impact)
        out('RICO1.WAV', sound_ricochet(0))
        out('RICO2.WAV', sound_ricochet(1))
    out('HITM.WAV', sound_hit_metal(), store=MECHANICS_RATE)
    door_open, door_close = recorded_door()
    out('DOOROPEN.WAV', door_open, store=MECHANICS_RATE)
    out('DOORCLOS.WAV', door_close, store=MECHANICS_RATE)
    # (the sliding doors of the airlock: a recording, its silences cut)
    out('SLIDE.WAV', sound_recording(SLIDE_RECORDING), store=LOW_RATE)
    # (the window shot: a recording of glass breaking, its silences cut, its long tail too (-32 dB): sound RAM)
    out('GLASS.WAV', fade_end(trim(sound_recording(GLASS_RECORDING), RATE, -32), RATE, 0.08), store=MECHANICS_RATE)
    # (the doors of the stalls: the same files played higher, the property pitch of the doors, see doors.c)
    for k, x in enumerate(recording_parts(PAIN_RECORDING, PAIN_COUNT)):
        out('GPAIN%d.WAV' % (k + 1), with_room(x, 'washroom', 0.15, 0.4, 95 + k), store=VOICE_RATE)
    # (the agent hit: a grunt, its silences cut (no delay after the hit, less sound RAM), dry: the DSP makes the echo of
    # the room)
    out('HURT.WAV', sound_recording(GRUNT_RECORDING, 0.002), store=VOICE_RATE)
    for name in os.listdir(CD):
        # (EXPLODE.PCM: the explosion of the barrels, kept in the cd, see make_assets.check_kept_files())
        if (name.endswith('.PCM') and name != 'EXPLODE.PCM') or (DSP_ECHO and name in ('RICO1.WAV', 'RICO2.WAV')):
            os.remove(os.path.join(CD, name))
    print('  sounds: %s (%d KB of sound RAM)' % (' '.join(made), total // 1024))
    return total


if __name__ == '__main__':
    build_sounds()
