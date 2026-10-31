"""Test of jo_engine/dsp/reverb.scsp: the simulator of the DSP of the SCSP (the semantics of Mednafen, see
Compiler/COMMON/scsp_dsp_asm.py) against a float model of the same reverberation; the decay time measured on the
impulse response, the saturation. Run: python3 tools/test_scsp_reverb.py [samples]"""
import os
import sys
import numpy as np

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
sys.path.insert(0, os.path.join(ROOT, 'Compiler', 'COMMON'))
import scsp_dsp_asm as asm

FS = 44100
COMB = [1116, 1188, 1277, 1356]


def coefs(decay_ms, damping, early, tail):
    """The coefficients computed by jo_audio_set_reverb() (same formulas, see audio.c)"""
    mean = sum(COMB) / len(COMB)
    fb = 10 ** (-3.0 * mean / (decay_ms / 1000.0 * FS))
    d = damping / 127.0 * 0.6
    e = early / 127.0
    w = tail / 127.0
    return {'damp': d, 'damp1': 1 - d, 'fbd1': fb * (1 - d), 'fbd': fb * d,
            'er1': 0.45 * e, 'er2': 0.4 * e, 'er3': 0.3 * e, 'er4': 0.28 * e, 'wet': w}


def float_model(x, c):
    """The same structure in floats"""
    n = len(x)
    pre = np.zeros(1015 + n)
    out = np.zeros((n, 2))
    combs = [np.zeros(d + 1 + n) for d in COMB]
    lp = [0.0] * 4
    aps = {k: np.zeros(d + 1 + n) for k, d in (('l1', 556), ('l2', 441), ('r1', 579), ('r2', 464))}
    apd = {'l1': 556, 'l2': 441, 'r1': 579, 'r2': 464}

    def delay(buf, t, d):
        return buf[t - d] if t - d >= 0 else 0.0

    for t in range(n):
        u = x[t] * 0.25
        pre[t] = u
        ci = delay(pre, t, 353)
        ys = [delay(combs[k], t, COMB[k]) for k in range(4)]
        for k in range(4):
            prev = lp[k]
            lp[k] = ys[k] * c['damp1'] + prev * c['damp']
            combs[k][t] = ci + c['fbd1'] * ys[k] + c['fbd'] * prev
        s = sum(ys) * 0.25

        def ap(name, v):
            bo = delay(aps[name], t, apd[name])
            aps[name][t] = v + 0.5 * bo
            return bo - v
        left = ap('l2', ap('l1', s))
        right = ap('r2', ap('r1', s))
        out[t, 0] = 2 * (left * c['wet'] + c['er1'] * delay(pre, t, 397) + c['er3'] * delay(pre, t, 750))
        out[t, 1] = 2 * (right * c['wet'] + c['er2'] * delay(pre, t, 573) + c['er4'] * delay(pre, t, 1014))
    return out


def run_dsp(x, c, prog):
    sim = asm.Simulator(prog, c)
    out = np.zeros((len(x), 2))
    for t, v in enumerate(x):
        # (the send of a slot: its 16 bits sample << 4 at IMXL 7, 20 bits)
        mix = (int(v * 32767) << 4) & 0xFFFFF
        ef = sim.run(mix)
        out[t] = (asm.sx(16, ef[0]) / 32768.0, asm.sx(16, ef[1]) / 32768.0)
    return out, sim


def rt60(y):
    """The decay time from the Schroeder integral of the energy (-5 to -25 dB, extrapolated to -60 dB)"""
    e = (y ** 2).sum(axis=1)
    sch = np.cumsum(e[::-1])[::-1]
    db = 10 * np.log10(sch / sch[0] + 1e-30)
    a = np.argmax(db < -5)
    b = np.argmax(db < -25)
    if b <= a:
        return float('nan')
    return (b - a) / FS * 3.0


def main():
    prog = asm.load(os.path.join(ROOT, 'jo_engine', 'dsp', 'reverb.scsp'))
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 44100
    x = np.zeros(n)
    x[0] = 0.9
    for decay, damp in ((800, 0), (800, 60), (400, 90)):
        c = coefs(decay, damp, 90, 100)
        ref = float_model(x, c)
        dsp, sim = run_dsp(x, c, prog)
        err = np.abs(dsp - ref).max()
        print('decay %d ms damping %d: RT60 float %.0f ms, DSP %.0f ms; max |DSP - float| %.5f (peak %.3f); peak of '
              'the accumulator %.3f' % (decay, damp, rt60(ref) * 1000, rt60(dsp) * 1000, err, np.abs(ref).max(),
                                         sim.max_abs / 0x800000))
    # Saturation: a loud noise (RMS -16 dB: a shot sent at 0 dB) for 0.3 s, the longest decay, the loudest levels
    rng = np.random.default_rng(1)
    x = np.zeros(max(n, 26000))
    x[:13000] = np.clip(rng.normal(0, 0.16, 13000), -1, 1)
    dsp, sim = run_dsp(x, coefs(1500, 30, 127, 127), prog)
    print('loud noise: peak output %.3f, peak of the accumulator %.3f (1: saturated)' % (np.abs(dsp).max(), sim.max_abs / 0x800000))

if __name__ == '__main__':
    main()
