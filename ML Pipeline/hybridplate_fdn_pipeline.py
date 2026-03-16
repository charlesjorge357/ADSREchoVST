#!/usr/bin/env python3
"""
hybridplate_fdn_pipeline.py

Complete ML pipeline for HybridPlate FDN reverb preset generation.
Generates parameters -> Renders IRs -> Extracts features -> Clusters -> Selects best 10

Usage:
    python hybridplate_fdn_pipeline.py --output-dir output_plate --n-samples 100

Requirements:
    - NumPy
"""

import numpy as np
import json
import os
import wave
import shutil
from typing import Tuple, List


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


def generate_hybridplate_params(n_samples: int = 100, seed: int = 42) -> List[dict]:
    """
    Generate parameter variations for HybridPlate FDN reverb.
    
    Standard Parameters (7):
        - room_size, decay_time, damping, mod_rate, mod_depth, pre_delay, mix
    
    ML Parameters (12):
        - early_delays[4]: Early diffusion allpass delay times (ms)
        - early_gain: Early diffusion allpass gain
        - fdn_delays[4]: FDN delay line times (ms)
        - feedback_safety: Feedback gain safety margin
        - high_shelf_freq: High shelf filter frequency
        - high_shelf_gain: High shelf filter gain
    """
    n_dims = 20
    lhs = latin_hypercube_sampling(n_samples, n_dims, seed)
    
    param_sets = []
    for i in range(n_samples):
        s = lhs[i]
        idx = 0
        
        standard_params = {
            'room_size': scale(s[idx], 0.3, 1.5),
            'decay_time': scale(s[idx+1], 0.5, 8.0),
            'damping': scale(s[idx+2], 1500, 10000),
            'mod_rate': scale(s[idx+3], 0.1, 1.5),
            'mod_depth': scale(s[idx+4], 0.05, 0.6),
            'pre_delay': scale(s[idx+5], 0.0, 80.0),
            'mix': scale(s[idx+6], 0.8, 1.0),
        }
        idx += 7
        
        ml_params = {
            'early_delays': [
                scale(s[idx], 1.5, 12.0),
                scale(s[idx+1], 2.0, 15.0),
                scale(s[idx+2], 3.0, 18.0),
                scale(s[idx+3], 5.0, 22.0),
            ],
            'early_gain': scale(s[idx+4], 0.55, 0.82),
            'fdn_delays': [
                scale(s[idx+5], 20.0, 80.0),
                scale(s[idx+6], 30.0, 100.0),
                scale(s[idx+7], 40.0, 120.0),
                scale(s[idx+8], 50.0, 140.0),
            ],
            'feedback_safety': scale(s[idx+9], 0.88, 0.96),
            'high_shelf_freq': scale(s[idx+10], 2000, 5000),
            'high_shelf_gain': scale(s[idx+11], 0.35, 0.75),
        }
        
        param_sets.append({
            'id': i,
            'standard_params': standard_params,
            'ml_params': ml_params,
            'audio_file': f'ir_{i}.wav'
        })
    
    return param_sets


# =============================================================================
# STEP 2: HYBRIDPLATE FDN REVERB RENDERER
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


class HighShelfFilter:
    """Simple high shelf approximation"""
    def __init__(self, freq: float, gain: float, sr: float):
        self.set_params(freq, gain, sr)
        self.state = 0.0
    
    def set_params(self, freq: float, gain: float, sr: float):
        w = 2.0 * np.pi * freq / sr
        self.alpha = np.clip(1.0 - np.exp(-w), 0.01, 0.99)
        self.gain = np.clip(gain, 0.1, 1.0)
    
    def process(self, x: float) -> float:
        self.state += self.alpha * (x - self.state)
        lp = self.state
        hp = x - lp
        return lp + hp * self.gain
    
    def reset(self):
        self.state = 0.0


class HybridPlateReverb:
    """
    HybridPlate FDN reverb implementation.
    
    Structure:
        - Pre-delay
        - 4 early diffusion allpasses per channel
        - Late diffusion allpass (BUG FIX #1)
        - 4-line FDN with Hadamard feedback matrix
        - Damping + high shelf per line
        - Output sums all taps (BUG FIX #2)
    """
    
    # Hadamard feedback matrix
    FEEDBACK_MATRIX = np.array([
        [ 0.5,  0.5,  0.5,  0.5],
        [ 0.5, -0.5,  0.5, -0.5],
        [ 0.5,  0.5, -0.5, -0.5],
        [ 0.5, -0.5, -0.5,  0.5]
    ], dtype=np.float64)
    
    def __init__(self, sr: float = 44100.0):
        self.sr = sr
        
        # Default standard params
        self.room_size = 1.0
        self.decay_time = 2.0
        self.damping = 4000.0
        self.mod_rate = 0.5
        self.mod_depth = 0.3
        self.pre_delay_ms = 10.0
        
        # Default ML params
        self.early_delays_ms = [2.5, 4.0, 6.0, 8.5]
        self.early_gain = 0.72
        self.fdn_delays_ms = [32.0, 44.0, 57.0, 70.0]
        self.feedback_safety = 0.95
        self.high_shelf_freq = 3000.0
        self.high_shelf_gain = 0.5
        
        # Pre-delay
        self.pre_delay_l = DelayLine(int(sr * 0.3))
        self.pre_delay_r = DelayLine(int(sr * 0.3))
        
        # Early diffusion: 4 allpasses per channel
        self.early_ap_l = [AllpassFilter(100, 0.72) for _ in range(4)]
        self.early_ap_r = [AllpassFilter(100, 0.72) for _ in range(4)]
        
        # Late diffusion (BUG FIX #1)
        self.late_ap_l = AllpassFilter(int(15.0 * sr / 1000), 0.6)
        self.late_ap_r = AllpassFilter(int(18.0 * sr / 1000), 0.6)
        
        # FDN: 4 delay lines
        self.fdn_lines = [DelayLine(int(sr * 0.5)) for _ in range(4)]
        self.fdn_base_samples = [0.0] * 4
        self.fdn_current_samples = [0.0] * 4
        
        # Damping per FDN line
        self.damp_filters = [OnePoleLP(4000, sr) for _ in range(4)]
        
        # High shelf per FDN line
        self.shelf_filters = [HighShelfFilter(3000, 0.5, sr) for _ in range(4)]
        
        # LFO state
        self.lfo_phase = 0.0
        
        # Estimated loop time
        self.est_loop_time = 0.05
        
        self._update()
    
    def _update(self):
        sr = self.sr
        
        # Pre-delay
        pd_samples = self.pre_delay_ms * 0.001 * sr
        self.pre_delay_l.set_delay(int(pd_samples))
        self.pre_delay_r.set_delay(int(pd_samples))
        
        # Early diffusion
        for i in range(4):
            d_l = int(self.early_delays_ms[i] * 0.001 * sr)
            d_r = int(self.early_delays_ms[i] * 1.11 * 0.001 * sr)
            self.early_ap_l[i].set_delay(d_l)
            self.early_ap_r[i].set_delay(d_r)
            self.early_ap_l[i].set_gain(self.early_gain)
            self.early_ap_r[i].set_gain(self.early_gain)
        
        # FDN delays
        total_delay = 0.0
        for i in range(4):
            base = self.fdn_delays_ms[i] * 0.001 * sr
            self.fdn_base_samples[i] = base
            self.fdn_current_samples[i] = base * self.room_size
            self.fdn_lines[i].set_delay(int(self.fdn_current_samples[i]))
            total_delay += base
        
        self.est_loop_time = (total_delay / 4) / sr
        
        # Damping
        for f in self.damp_filters:
            f.set_cutoff(self.damping, sr)
        
        # High shelf
        for f in self.shelf_filters:
            f.set_params(self.high_shelf_freq, self.high_shelf_gain, sr)
    
    def set_params(self, standard_params: dict, ml_params: dict):
        self.room_size = standard_params['room_size']
        self.decay_time = standard_params['decay_time']
        self.damping = standard_params['damping']
        self.mod_rate = standard_params['mod_rate']
        self.mod_depth = standard_params['mod_depth']
        self.pre_delay_ms = standard_params['pre_delay']
        
        self.early_delays_ms = ml_params['early_delays']
        self.early_gain = ml_params['early_gain']
        self.fdn_delays_ms = ml_params['fdn_delays']
        self.feedback_safety = ml_params['feedback_safety']
        self.high_shelf_freq = ml_params['high_shelf_freq']
        self.high_shelf_gain = ml_params['high_shelf_gain']
        
        self._update()
    
    def reset(self):
        self.pre_delay_l.reset()
        self.pre_delay_r.reset()
        for ap in self.early_ap_l:
            ap.reset()
        for ap in self.early_ap_r:
            ap.reset()
        self.late_ap_l.reset()
        self.late_ap_r.reset()
        for dl in self.fdn_lines:
            dl.reset()
        for f in self.damp_filters:
            f.reset()
        for f in self.shelf_filters:
            f.reset()
        self.lfo_phase = 0.0
    
    def process(self, in_l: float, in_r: float) -> Tuple[float, float]:
        # Pre-delay
        self.pre_delay_l.write(in_l)
        self.pre_delay_r.write(in_r)
        pd_l = self.pre_delay_l.read()
        pd_r = self.pre_delay_r.read()
        
        # Early diffusion
        e_l = pd_l
        for ap in self.early_ap_l:
            e_l = ap.process(e_l)
        
        e_r = pd_r
        for ap in self.early_ap_r:
            e_r = ap.process(e_r)
        
        # Late diffusion (BUG FIX #1)
        e_l = self.late_ap_l.process(e_l)
        e_r = self.late_ap_r.process(e_r)
        
        mono_in = 0.5 * (e_l + e_r)
        
        # LFO
        lfo_sin = np.sin(2.0 * np.pi * self.lfo_phase)
        lfo_cos = np.cos(2.0 * np.pi * self.lfo_phase)
        self.lfo_phase = (self.lfo_phase + self.mod_rate / self.sr) % 1.0
        
        lfo_vals = [
            lfo_sin,
            lfo_cos,
            np.tanh(lfo_sin + 0.5 * lfo_cos),
            np.tanh(lfo_cos - 0.5 * lfo_sin)
        ]
        
        # Feedback gain from RT60
        effective_loop = self.est_loop_time * self.room_size
        fb_raw = np.exp(-3.0 * effective_loop / max(0.1, self.decay_time))
        fb_gain = np.clip(fb_raw * self.feedback_safety, 0.0, 0.90)
        
        # Read FDN outputs
        fdn_out = [0.0] * 4
        for i in range(4):
            base = self.fdn_base_samples[i] * self.room_size
            mod_samples = base * 0.003 * self.mod_depth * lfo_vals[i]
            target = np.clip(base + mod_samples, 1.0, self.sr * 0.4)
            self.fdn_current_samples[i] += 0.001 * (target - self.fdn_current_samples[i])
            self.fdn_lines[i].set_delay(int(self.fdn_current_samples[i]))
            fdn_out[i] = self.fdn_lines[i].read()
        
        # Output: sum all taps (BUG FIX #2)
        tap_sum_l = 0.35 * (fdn_out[0] + fdn_out[2]) + 0.15 * (fdn_out[1] - fdn_out[3])
        tap_sum_r = 0.35 * (fdn_out[1] + fdn_out[3]) + 0.15 * (fdn_out[0] - fdn_out[2])
        
        # Feedback via Hadamard matrix
        mixed = np.dot(self.FEEDBACK_MATRIX, fdn_out)
        
        # Push new samples into FDN
        for i in range(4):
            new_sample = mono_in + mixed[i] * fb_gain
            damped = self.damp_filters[i].process(new_sample)
            softened = self.shelf_filters[i].process(damped)
            self.fdn_lines[i].write(softened)
        
        return tap_sum_l, tap_sum_r


def render_hybridplate_ir(reverb: HybridPlateReverb, length_sec: float = 2.5) -> Tuple[np.ndarray, np.ndarray]:
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
    if 0.5 <= rt60 <= 4.0:
        score += 2.0
    elif 0.3 <= rt60 <= 6.0:
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
                 sr: int = 44100, ir_length: float = 2.0, seed: int = 42):
    """
    Run complete HybridPlate FDN ML pipeline.
    
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
    param_sets = generate_hybridplate_params(n_samples, seed)
    
    with open(os.path.join(output_dir, 'metadata.json'), 'w') as f:
        json.dump({'algorithm': 'HybridPlate', 'n_samples': n_samples, 'parameter_sets': param_sets}, f, indent=2)
    
    # Step 2: Render IRs
    print(f"Step 2: Rendering {n_samples} impulse responses...")
    reverb = HybridPlateReverb(sr)
    
    for i, ps in enumerate(param_sets):
        reverb.set_params(ps['standard_params'], ps['ml_params'])
        ir_l, ir_r = render_hybridplate_ir(reverb, ir_length)
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
    X_ml = np.zeros((len(filtered), 12))
    for i, ps in enumerate(filtered):
        ml = ps['ml_params']
        X_ml[i, 0:4] = ml['early_delays']
        X_ml[i, 4] = ml['early_gain']
        X_ml[i, 5:9] = ml['fdn_delays']
        X_ml[i, 9] = ml['feedback_safety']
        X_ml[i, 10] = ml['high_shelf_freq']
        X_ml[i, 11] = ml['high_shelf_gain']
    
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
    parser = argparse.ArgumentParser(description='HybridPlate FDN ML Pipeline')
    parser.add_argument('--output-dir', type=str, default='output_hybridplate')
    parser.add_argument('--n-samples', type=int, default=100)
    parser.add_argument('--n-clusters', type=int, default=10)
    parser.add_argument('--sample-rate', type=int, default=44100)
    parser.add_argument('--ir-length', type=float, default=2.0)
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
