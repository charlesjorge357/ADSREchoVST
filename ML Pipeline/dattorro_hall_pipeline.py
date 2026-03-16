#!/usr/bin/env python3
"""
dattorro_hall_pipeline.py

Complete ML pipeline for Dattorro Hall reverb preset generation.
Generates parameters -> Renders IRs -> Extracts features -> Clusters -> Selects best 10

Usage:
    python dattorro_hall_pipeline.py --output-dir output_hall --n-samples 100

Requirements:
    - NumPy
"""

import numpy as np
import json
import os
import wave
import shutil
from typing import Tuple, List
from dataclasses import dataclass, field


# =============================================================================
# STEP 1: PARAMETER GENERATION (Latin Hypercube Sampling)
# =============================================================================

def latin_hypercube_sampling(n_samples: int, n_dims: int, seed: int = 42) -> np.ndarray:
    np.random.seed(seed)
    samples = np.zeros((n_samples, n_dims))
    for dim in range(n_dims):
        intervals = np.linspace(0, 1, n_samples + 1)
        for i in range(n_samples):
            samples[i, dim] = np.random.uniform(intervals[i], intervals[i + 1])
        np.random.shuffle(samples[:, dim])
    return samples


def scale(val, lo, hi):
    return float(val * (hi - lo) + lo)


def generate_dattorro_params(n_samples: int = 100, seed: int = 42) -> List[dict]:
    """
    Generate parameter variations for Dattorro Hall reverb.
    
    ML Parameters (14):
        - early_allpass_scales[2]: Scale early diffusion delays
        - loop_delay_scales_l[4]: Scale left tank delays
        - loop_delay_scales_r[4]: Scale right tank delays
        - diffusion_coeff_1: First diffusion stage gain
        - diffusion_coeff_2: Second diffusion stage gain
        - decay_scale: Multiplier for decay feedback
        - damping_scale: Multiplier for damping cutoff
    
    Standard Parameters (6):
        - room_size, decay_time, damping, diffusion, mod_rate, mod_depth
    """
    n_dims = 20
    lhs = latin_hypercube_sampling(n_samples, n_dims, seed)
    
    param_sets = []
    for i in range(n_samples):
        s = lhs[i]
        idx = 0
        
        ml_params = {
            'early_allpass_scales': [
                scale(s[idx], 0.5, 2.0),
                scale(s[idx+1], 0.5, 2.0)
            ],
            'loop_delay_scales_l': [
                scale(s[idx+2], 0.3, 3.0),
                scale(s[idx+3], 0.3, 3.0),
                scale(s[idx+4], 0.3, 3.0),
                scale(s[idx+5], 0.3, 3.0)
            ],
            'loop_delay_scales_r': [
                scale(s[idx+6], 0.3, 3.0),
                scale(s[idx+7], 0.3, 3.0),
                scale(s[idx+8], 0.3, 3.0),
                scale(s[idx+9], 0.3, 3.0)
            ],
            'diffusion_coeff_1': scale(s[idx+10], 0.3, 0.95),
            'diffusion_coeff_2': scale(s[idx+11], 0.3, 0.95),
            'decay_scale': scale(s[idx+12], 0.5, 1.5),
            'damping_scale': scale(s[idx+13], 0.5, 3.0)
        }
        idx += 14
        
        reverb_params = {
            'room_size': scale(s[idx], 0.3, 1.5),
            'decay_time': scale(s[idx+1], 0.3, 0.85),
            'damping': scale(s[idx+2], 1500, 10000),
            'diffusion': scale(s[idx+3], 0.6, 1.0),
            'mod_rate': scale(s[idx+4], 0.1, 1.5),
            'mod_depth': scale(s[idx+5], 0.05, 0.4)
        }
        
        param_sets.append({
            'id': i,
            'ml_params': ml_params,
            'reverb_params': reverb_params,
            'audio_file': f'ir_{i}.wav'
        })
    
    return param_sets


# =============================================================================
# STEP 2: DATTORRO REVERB RENDERER
# =============================================================================

class AllpassFilter:
    """Schroeder allpass filter"""
    def __init__(self, delay_samples: int, gain: float = 0.5):
        self.delay = max(1, int(delay_samples))
        self.gain = np.clip(gain, -0.95, 0.95)
        self.x_buf = np.zeros(self.delay, dtype=np.float64)
        self.y_buf = np.zeros(self.delay, dtype=np.float64)
        self.idx = 0
    
    def set_gain(self, g: float):
        self.gain = np.clip(g, -0.95, 0.95)
    
    def set_delay(self, samples: int):
        new_d = max(1, int(samples))
        if new_d != self.delay:
            self.x_buf = np.zeros(new_d, dtype=np.float64)
            self.y_buf = np.zeros(new_d, dtype=np.float64)
            self.delay = new_d
            self.idx = 0
    
    def process(self, x: float) -> float:
        x_del = self.x_buf[self.idx]
        y_del = self.y_buf[self.idx]
        y = -self.gain * x + x_del + self.gain * y_del
        self.x_buf[self.idx] = x
        self.y_buf[self.idx] = y
        self.idx = (self.idx + 1) % self.delay
        return y
    
    def reset(self):
        self.x_buf.fill(0.0)
        self.y_buf.fill(0.0)
        self.idx = 0


class DelayLine:
    """Simple delay line"""
    def __init__(self, max_samples: int):
        self.max_samples = max(1, int(max_samples))
        self.buf = np.zeros(self.max_samples, dtype=np.float64)
        self.write_idx = 0
        self.delay = 1
    
    def set_delay(self, samples: int):
        self.delay = max(1, min(int(samples), self.max_samples - 1))
    
    def read(self) -> float:
        rd = (self.write_idx - self.delay) % self.max_samples
        return self.buf[rd]
    
    def write(self, x: float):
        self.buf[self.write_idx] = x
        self.write_idx = (self.write_idx + 1) % self.max_samples
    
    def reset(self):
        self.buf.fill(0.0)
        self.write_idx = 0


class OnePoleLP:
    """One-pole lowpass for damping"""
    def __init__(self, cutoff: float, sr: float):
        self.set_cutoff(cutoff, sr)
        self.state = 0.0
    
    def set_cutoff(self, cutoff: float, sr: float):
        cutoff = np.clip(cutoff, 100, sr * 0.45)
        w = 2.0 * np.pi * cutoff / sr
        self.coeff = np.clip(1.0 - np.exp(-w), 0.01, 0.99)
    
    def process(self, x: float) -> float:
        self.state += self.coeff * (x - self.state)
        return self.state
    
    def reset(self):
        self.state = 0.0


class DattorroHallReverb:
    """
    Dattorro Hall reverb implementation.
    
    Structure:
        - 4 input diffusion allpasses
        - Late diffusion allpass (BUG FIX #1)
        - 4 delay lines per tank
        - Output sums all taps (BUG FIX #2)
    """
    def __init__(self, sr: float = 44100.0):
        self.sr = sr
        self.sr_scale = sr / 29761.0
        
        # Default params
        self.room_size = 0.5
        self.decay_time = 0.5
        self.damping = 4000.0
        self.diffusion = 1.0
        self.mod_rate = 0.5
        self.mod_depth = 0.3
        
        # ML params
        self.early_allpass_scales = [1.0, 1.0]
        self.loop_delay_scales_l = [1.0, 1.0, 1.0, 1.0]
        self.loop_delay_scales_r = [1.0, 1.0, 1.0, 1.0]
        self.diffusion_coeff_1 = 0.75
        self.diffusion_coeff_2 = 0.625
        self.decay_scale = 1.0
        self.damping_scale = 1.0
        
        max_delay = int(sr * 0.5)
        
        # Input diffusion
        self.in_ap = [
            AllpassFilter(int(142 * self.sr_scale), 0.75),
            AllpassFilter(int(107 * self.sr_scale), 0.75),
            AllpassFilter(int(379 * self.sr_scale), 0.625),
            AllpassFilter(int(277 * self.sr_scale), 0.625),
        ]
        
        # Late diffusion (BUG FIX #1)
        self.late_ap_l = AllpassFilter(int(672 * self.sr_scale), 0.5)
        self.late_ap_r = AllpassFilter(int(908 * self.sr_scale), 0.5)
        
        # Tank delays
        self.base_delays_l = [672, 1800, 2656, 3720]
        self.base_delays_r = [908, 2200, 3163, 3931]
        
        self.tank_delays_l = [DelayLine(max_delay) for _ in range(4)]
        self.tank_delays_r = [DelayLine(max_delay) for _ in range(4)]
        
        for i, d in enumerate(self.tank_delays_l):
            d.set_delay(int(self.base_delays_l[i] * self.sr_scale))
        for i, d in enumerate(self.tank_delays_r):
            d.set_delay(int(self.base_delays_r[i] * self.sr_scale))
        
        # Damping
        self.damp_l = OnePoleLP(4000, sr)
        self.damp_r = OnePoleLP(4000, sr)
        
        # Feedback state
        self.feedback_l = 0.0
        self.feedback_r = 0.0
        
        self._update()
    
    def _update(self):
        diff1 = np.clip(self.diffusion_coeff_1 * self.diffusion, 0.0, 0.85)
        diff2 = np.clip(self.diffusion_coeff_2 * self.diffusion, 0.0, 0.85)
        
        self.in_ap[0].set_gain(diff1)
        self.in_ap[1].set_gain(diff1)
        self.in_ap[2].set_gain(diff2)
        self.in_ap[3].set_gain(diff2)
        
        late_diff = np.clip(0.5 * self.diffusion, 0.0, 0.7)
        self.late_ap_l.set_gain(late_diff)
        self.late_ap_r.set_gain(late_diff)
        
        for i, d in enumerate(self.tank_delays_l):
            sc = self.loop_delay_scales_l[i] if i < len(self.loop_delay_scales_l) else 1.0
            d.set_delay(int(self.base_delays_l[i] * self.sr_scale * self.room_size * sc))
        
        for i, d in enumerate(self.tank_delays_r):
            sc = self.loop_delay_scales_r[i] if i < len(self.loop_delay_scales_r) else 1.0
            d.set_delay(int(self.base_delays_r[i] * self.sr_scale * self.room_size * sc))
        
        damp_freq = np.clip(self.damping * self.damping_scale, 500, 16000)
        self.damp_l.set_cutoff(damp_freq, self.sr)
        self.damp_r.set_cutoff(damp_freq, self.sr)
    
    def set_params(self, reverb_params: dict, ml_params: dict):
        self.room_size = reverb_params['room_size']
        self.decay_time = reverb_params['decay_time']
        self.damping = reverb_params['damping']
        self.diffusion = reverb_params['diffusion']
        self.mod_rate = reverb_params['mod_rate']
        self.mod_depth = reverb_params['mod_depth']
        
        self.early_allpass_scales = ml_params['early_allpass_scales']
        self.loop_delay_scales_l = ml_params['loop_delay_scales_l']
        self.loop_delay_scales_r = ml_params['loop_delay_scales_r']
        self.diffusion_coeff_1 = ml_params['diffusion_coeff_1']
        self.diffusion_coeff_2 = ml_params['diffusion_coeff_2']
        self.decay_scale = ml_params['decay_scale']
        self.damping_scale = ml_params['damping_scale']
        
        self._update()
    
    def reset(self):
        for ap in self.in_ap:
            ap.reset()
        self.late_ap_l.reset()
        self.late_ap_r.reset()
        for d in self.tank_delays_l:
            d.reset()
        for d in self.tank_delays_r:
            d.reset()
        self.damp_l.reset()
        self.damp_r.reset()
        self.feedback_l = 0.0
        self.feedback_r = 0.0
    
    def process(self, in_l: float, in_r: float) -> Tuple[float, float]:
        # Input diffusion
        mono = (in_l + in_r) * 0.5
        x = mono
        for ap in self.in_ap:
            x = ap.process(x)
        
        decay = np.clip(self.decay_time * self.decay_scale, 0.0, 0.95)
        
        # Left tank (with BUG FIX #1: late diffusion)
        tank_in_l = x + self.feedback_r * decay
        tank_in_l = self.late_ap_l.process(tank_in_l)
        
        # BUG FIX #2: Sum all taps
        signal_l = tank_in_l
        tap_sum_l = 0.0
        for i, delay in enumerate(self.tank_delays_l):
            delay.write(signal_l)
            signal_l = delay.read()
            if i == 1:
                signal_l = self.damp_l.process(signal_l)
            tap_sum_l += signal_l
        
        self.feedback_l = signal_l * decay
        
        # Right tank
        tank_in_r = x + self.feedback_l * decay
        tank_in_r = self.late_ap_r.process(tank_in_r)
        
        signal_r = tank_in_r
        tap_sum_r = 0.0
        for i, delay in enumerate(self.tank_delays_r):
            delay.write(signal_r)
            signal_r = delay.read()
            if i == 1:
                signal_r = self.damp_r.process(signal_r)
            tap_sum_r += signal_r
        
        self.feedback_r = signal_r * decay
        
        # Output (BUG FIX #2: sum of all taps)
        out_l = (tap_sum_l + tap_sum_r * 0.3) * 0.2
        out_r = (tap_sum_r + tap_sum_l * 0.3) * 0.2
        
        return out_l, out_r


def render_dattorro_ir(reverb: DattorroHallReverb, length_sec: float = 2.5) -> Tuple[np.ndarray, np.ndarray]:
    n = int(reverb.sr * length_sec)
    ir_l = np.zeros(n, dtype=np.float32)
    ir_r = np.zeros(n, dtype=np.float32)
    reverb.reset()
    for i in range(n):
        inp = 1.0 if i == 0 else 0.0
        ir_l[i], ir_r[i] = reverb.process(inp, inp)
    return ir_l, ir_r


# =============================================================================
# STEP 3: FEATURE EXTRACTION
# =============================================================================

def read_wav(filepath: str) -> Tuple[np.ndarray, int]:
    import struct
    with wave.open(filepath, 'rb') as wav:
        n_ch = wav.getnchannels()
        sr = wav.getframerate()
        n_frames = wav.getnframes()
        raw = wav.readframes(n_frames)
        fmt = f'<{n_frames * n_ch}h'
        samples = np.array(struct.unpack(fmt, raw), dtype=np.float32) / 32768.0
        if n_ch == 2:
            samples = samples.reshape(-1, 2).mean(axis=1)
        return samples, sr


def calc_rt60(ir: np.ndarray, sr: int) -> float:
    energy = ir ** 2
    schroeder = np.cumsum(energy[::-1])[::-1]
    schroeder = schroeder / (schroeder[0] + 1e-10)
    schroeder_db = 10 * np.log10(schroeder + 1e-10)
    try:
        idx_5 = np.where(schroeder_db <= -5)[0][0]
        idx_25 = np.where(schroeder_db <= -25)[0][0]
        return float(np.clip(3 * (idx_25 - idx_5) / sr, 0, 10))
    except:
        return 0.0


def calc_edt(ir: np.ndarray, sr: int) -> float:
    energy = ir ** 2
    schroeder = np.cumsum(energy[::-1])[::-1]
    schroeder = schroeder / (schroeder[0] + 1e-10)
    schroeder_db = 10 * np.log10(schroeder + 1e-10)
    try:
        idx_10 = np.where(schroeder_db <= -10)[0][0]
        return float(np.clip(6 * idx_10 / sr, 0, 10))
    except:
        return 0.0


def calc_c80(ir: np.ndarray, sr: int) -> float:
    split = int(0.08 * sr)
    early = np.sum(ir[:split] ** 2)
    late = np.sum(ir[split:] ** 2)
    if late < 1e-10:
        return 20.0
    return float(np.clip(10 * np.log10((early + 1e-10) / (late + 1e-10)), -20, 20))


def calc_centroid(ir: np.ndarray, sr: int) -> float:
    n_fft = 4096
    spec = np.abs(np.fft.rfft(ir[:n_fft]))
    freqs = np.fft.rfftfreq(n_fft, 1/sr)
    return float(np.sum(freqs * spec) / (np.sum(spec) + 1e-10))


def calc_density(ir: np.ndarray, sr: int) -> float:
    early = int(0.1 * sr)
    zc = np.sum(np.abs(np.diff(np.sign(ir[:early]))) > 0)
    return float(zc / early * sr)


def extract_features(ir: np.ndarray, sr: int) -> dict:
    return {
        'rt60': calc_rt60(ir, sr),
        'edt': calc_edt(ir, sr),
        'c80': calc_c80(ir, sr),
        'spectral_centroid': calc_centroid(ir, sr),
        'density': calc_density(ir, sr),
    }


# =============================================================================
# STEP 4: QUALITY SCORING & K-MEANS CLUSTERING
# =============================================================================

def score_quality(features: dict) -> float:
    score = 0.0
    
    rt60 = features['rt60']
    if 0.5 <= rt60 <= 3.0:
        score += 2.0
    elif 0.3 <= rt60 <= 4.0:
        score += 1.0
    
    edt = features['edt']
    if rt60 > 0.1:
        ratio = edt / rt60
        if 0.7 <= ratio <= 1.3:
            score += 1.5
        elif 0.5 <= ratio <= 1.5:
            score += 0.5
    
    c80 = features['c80']
    if -5 <= c80 <= 5:
        score += 1.5
    elif -10 <= c80 <= 10:
        score += 0.5
    
    cent = features['spectral_centroid']
    if 500 <= cent <= 3000:
        score += 1.0
    elif 300 <= cent <= 5000:
        score += 0.5
    
    dens = features['density']
    if dens > 2000:
        score += 2.0
    elif dens > 1000:
        score += 1.0
    elif dens > 500:
        score += 0.5
    
    return score


def kmeans(X: np.ndarray, k: int, max_iter: int = 100, seed: int = 42) -> np.ndarray:
    np.random.seed(seed)
    n = X.shape[0]
    idx = np.random.choice(n, k, replace=False)
    centers = X[idx].copy()
    
    for _ in range(max_iter):
        dists = np.zeros((n, k))
        for j in range(k):
            dists[:, j] = np.linalg.norm(X - centers[j], axis=1)
        assigns = np.argmin(dists, axis=1)
        
        new_centers = np.zeros_like(centers)
        for j in range(k):
            mask = assigns == j
            if np.sum(mask) > 0:
                new_centers[j] = X[mask].mean(axis=0)
            else:
                new_centers[j] = X[np.random.randint(n)]
        
        if np.allclose(centers, new_centers):
            break
        centers = new_centers
    
    return assigns


# =============================================================================
# STEP 5: UTILITY FUNCTIONS
# =============================================================================

def normalize(ir_l: np.ndarray, ir_r: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
    peak = max(np.abs(ir_l).max(), np.abs(ir_r).max())
    if peak > 0.001:
        gain = 0.7 / peak
        return ir_l * gain, ir_r * gain
    return ir_l, ir_r


def write_wav(path: str, ir_l: np.ndarray, ir_r: np.ndarray, sr: int):
    with wave.open(path, 'wb') as w:
        w.setnchannels(2)
        w.setsampwidth(2)
        w.setframerate(sr)
        data = np.zeros(len(ir_l) * 2, dtype=np.int16)
        data[0::2] = (np.clip(ir_l, -1, 1) * 32767).astype(np.int16)
        data[1::2] = (np.clip(ir_r, -1, 1) * 32767).astype(np.int16)
        w.writeframes(data.tobytes())


# =============================================================================
# MAIN PIPELINE
# =============================================================================

def run_pipeline(output_dir: str, n_samples: int = 100, n_clusters: int = 10, 
                 sr: int = 44100, ir_length: float = 2.5, seed: int = 42):
    """
    Run complete Dattorro Hall ML pipeline.
    
    Steps:
        1. Generate parameters using Latin Hypercube Sampling
        2. Render impulse responses
        3. Extract audio features
        4. Score quality and filter
        5. Cluster using K-Means
        6. Select best from each cluster
    """
    os.makedirs(output_dir, exist_ok=True)
    os.makedirs(os.path.join(output_dir, 'selected'), exist_ok=True)
    
    # Step 1: Generate parameters
    print(f"Step 1: Generating {n_samples} parameter sets...")
    param_sets = generate_dattorro_params(n_samples, seed)
    
    with open(os.path.join(output_dir, 'metadata.json'), 'w') as f:
        json.dump({'algorithm': 'DattorroHall', 'n_samples': n_samples, 'parameter_sets': param_sets}, f, indent=2)
    
    # Step 2: Render IRs
    print(f"Step 2: Rendering {n_samples} impulse responses...")
    reverb = DattorroHallReverb(sr)
    
    for i, ps in enumerate(param_sets):
        reverb.set_params(ps['reverb_params'], ps['ml_params'])
        ir_l, ir_r = render_dattorro_ir(reverb, ir_length)
        ir_l, ir_r = normalize(ir_l, ir_r)
        write_wav(os.path.join(output_dir, ps['audio_file']), ir_l, ir_r, sr)
        if (i + 1) % 20 == 0:
            print(f"   Rendered {i + 1}/{n_samples}")
    
    # Step 3: Extract features
    print(f"Step 3: Extracting features...")
    for ps in param_sets:
        ir, _ = read_wav(os.path.join(output_dir, ps['audio_file']))
        ps['features'] = extract_features(ir, sr)
        ps['quality_score'] = score_quality(ps['features'])
    
    # Step 4: Filter
    print(f"Step 4: Filtering by quality...")
    scores = np.array([ps['quality_score'] for ps in param_sets])
    threshold = np.percentile(scores, 50)
    filtered = [ps for ps in param_sets if ps['quality_score'] >= threshold]
    print(f"   Kept {len(filtered)}/{n_samples} (threshold={threshold:.2f})")
    
    # Step 5: Cluster
    print(f"Step 5: Clustering into {n_clusters} groups...")
    X_ml = np.zeros((len(filtered), 14))
    for i, ps in enumerate(filtered):
        ml = ps['ml_params']
        X_ml[i, 0:2] = ml['early_allpass_scales']
        X_ml[i, 2:6] = ml['loop_delay_scales_l']
        X_ml[i, 6:10] = ml['loop_delay_scales_r']
        X_ml[i, 10] = ml['diffusion_coeff_1']
        X_ml[i, 11] = ml['diffusion_coeff_2']
        X_ml[i, 12] = ml['decay_scale']
        X_ml[i, 13] = ml['damping_scale']
    
    X_norm = (X_ml - X_ml.mean(axis=0)) / (X_ml.std(axis=0) + 1e-10)
    assigns = kmeans(X_norm, n_clusters, seed=seed)
    
    # Step 6: Select best from each cluster
    print(f"Step 6: Selecting best from each cluster...")
    selected = []
    for k in range(n_clusters):
        cluster_idx = np.where(assigns == k)[0]
        if len(cluster_idx) == 0:
            continue
        cluster_scores = np.array([filtered[i]['quality_score'] for i in cluster_idx])
        best_idx = cluster_idx[np.argmax(cluster_scores)]
        rep = filtered[best_idx].copy()
        rep['cluster'] = k
        selected.append(rep)
        
        # Copy WAV
        src = os.path.join(output_dir, rep['audio_file'])
        dst = os.path.join(output_dir, 'selected', f"cluster_{k}_{rep['audio_file']}")
        shutil.copy2(src, dst)
    
    # Save results
    with open(os.path.join(output_dir, 'selected', 'selected.json'), 'w') as f:
        json.dump({'n': len(selected), 'representatives': selected}, f, indent=2)
    
    print(f"\n=== RESULTS ===")
    print(f"Selected {len(selected)} presets:")
    for s in selected:
        print(f"   Cluster {s['cluster']}: {s['audio_file']} (score={s['quality_score']:.2f})")
    
    print(f"\nFiles saved to: {output_dir}/selected/")
    return selected


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='Dattorro Hall ML Pipeline')
    parser.add_argument('--output-dir', type=str, default='output_dattorro')
    parser.add_argument('--n-samples', type=int, default=100)
    parser.add_argument('--n-clusters', type=int, default=10)
    parser.add_argument('--sample-rate', type=int, default=44100)
    parser.add_argument('--ir-length', type=float, default=2.5)
    parser.add_argument('--seed', type=int, default=42)
    args = parser.parse_args()
    
    run_pipeline(
        output_dir=args.output_dir,
        n_samples=args.n_samples,
        n_clusters=args.n_clusters,
        sr=args.sample_rate,
        ir_length=args.ir_length,
        seed=args.seed
    )
