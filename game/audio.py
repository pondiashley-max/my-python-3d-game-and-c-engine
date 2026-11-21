"""
Procedural Douala Makossa / Afrobeat-inspired synthesizer & dynamic SFX engine
using NumPy + pygame.mixer (zero external audio files required).
"""

from __future__ import annotations

from typing import Dict, List, Optional

import numpy as np
import pygame


class DoualaAudio:
    SAMPLE_RATE = 44100

    def __init__(self, eng_mod, muted: bool = False) -> None:
        self.eng_mod = eng_mod
        self.muted = muted
        self.enabled = False
        self.sounds: Dict[int, pygame.mixer.Sound] = {}
        self.coin_scale: List[pygame.mixer.Sound] = []
        self.music_loop: Optional[pygame.mixer.Sound] = None
        self.music_channel: Optional[pygame.mixer.Channel] = None
        self._coin_step = 0

        try:
            if not pygame.mixer.get_init():
                pygame.mixer.init(frequency=self.SAMPLE_RATE, size=-16, channels=2, buffer=512)
            pygame.mixer.set_num_channels(16)
            self.enabled = True
            self._synthesize_all()
        except Exception:
            self.enabled = False

    def _to_sound(self, mono: np.ndarray, volume: float = 0.5) -> pygame.mixer.Sound:
        mono = np.clip(mono * volume, -0.98, 0.98)
        pcm = (mono * 32767.0).astype(np.int16)
        stereo = np.column_stack((pcm, pcm))
        return pygame.sndarray.make_sound(np.ascontiguousarray(stereo))

    def _tone(self, freq: float, dur: float, wave: str = "sine", decay: float = 8.0) -> np.ndarray:
        t = np.linspace(0.0, dur, int(self.SAMPLE_RATE * dur), endpoint=False)
        env = np.exp(-t * decay) * np.minimum(1.0, t / 0.006)
        if wave == "sine":
            sig = np.sin(2.0 * np.pi * freq * t)
        elif wave == "tri":
            sig = 2.0 * np.abs(2.0 * ((t * freq) % 1.0) - 1.0) - 1.0
        elif wave == "square":
            sig = np.sign(np.sin(2.0 * np.pi * freq * t)) * 0.65
        else:
            sig = np.sin(2.0 * np.pi * freq * t) + 0.35 * np.sin(4.0 * np.pi * freq * t)
        return sig * env

    def _sweep(self, f0: float, f1: float, dur: float, noise_mix: float = 0.0) -> np.ndarray:
        n = int(self.SAMPLE_RATE * dur)
        t = np.linspace(0.0, dur, n, endpoint=False)
        freqs = np.linspace(f0, f1, n)
        phase = 2.0 * np.pi * np.cumsum(freqs) / self.SAMPLE_RATE
        env = np.sin(np.pi * (t / dur)) ** 0.7
        sig = np.sin(phase)
        if noise_mix > 0.0:
            rng = np.random.default_rng(42)
            sig = (1.0 - noise_mix) * sig + noise_mix * rng.uniform(-1.0, 1.0, n)
        return sig * env

    def _synthesize_all(self) -> None:
        E = self.eng_mod
        # Pentatonic ascending FCFA coin chimes (joyful West/Central African marimba vibe!)
        pentatonic = [587.33, 659.25, 783.99, 880.0, 1046.5, 1174.66, 1318.5, 1567.98]
        for f in pentatonic:
            sig = self._tone(f, 0.14, "bell", 16.0) + 0.35 * self._tone(f * 2.0, 0.14, "sine", 24.0)
            self.coin_scale.append(self._to_sound(sig, 0.36))

        self.sounds[E.SFX_JUMP] = self._to_sound(self._sweep(220.0, 620.0, 0.17, 0.08), 0.38)
        self.sounds[E.SFX_ROLL] = self._to_sound(self._sweep(240.0, 90.0, 0.16, 0.35), 0.40)
        self.sounds[E.SFX_LANE_SWITCH] = self._to_sound(self._sweep(340.0, 480.0, 0.08, 0.18), 0.25)
        self.sounds[E.SFX_NEAR_MISS] = self._to_sound(self._sweep(880.0, 320.0, 0.20, 0.28), 0.35)

        # Authentic Douala Yellow Taxi double "Bip-Bip!" horn
        bip = (
            self._tone(440.0, 0.11, "square", 3.0)
            + self._tone(554.37, 0.11, "square", 3.0)
        ) * 0.45
        gap = np.zeros(int(self.SAMPLE_RATE * 0.06))
        self.sounds[E.SFX_HORN] = self._to_sound(np.concatenate([bip, gap, bip]), 0.24)

        # Traffic officer whistle trill on stumble!
        t_w = np.linspace(0.0, 0.32, int(self.SAMPLE_RATE * 0.32), endpoint=False)
        trill = np.sin(2.0 * np.pi * (2650.0 + 180.0 * np.sin(2.0 * np.pi * 34.0 * t_w)) * t_w)
        whistle = trill * np.exp(-t_w * 4.5) * np.minimum(1.0, t_w / 0.02)
        self.sounds[E.SFX_STUMBLE] = self._to_sound(whistle, 0.34)

        # Crash impact burst
        n_cr = int(self.SAMPLE_RATE * 0.42)
        t_cr = np.linspace(0.0, 0.42, n_cr, endpoint=False)
        rng = np.random.default_rng(237)
        crash = (
            rng.uniform(-1.0, 1.0, n_cr) * np.exp(-t_cr * 8.0) * 0.7
            + self._sweep(160.0, 42.0, 0.42) * 0.8
        )
        self.sounds[E.SFX_CRASH] = self._to_sound(crash, 0.55)

        # Power-up rising triad fanfare
        pu = np.concatenate([
            self._tone(523.25, 0.09, "bell", 10.0),
            self._tone(659.25, 0.09, "bell", 10.0),
            self._tone(783.99, 0.10, "bell", 9.0),
            self._tone(1046.5, 0.22, "bell", 6.0),
        ])
        self.sounds[E.SFX_POWERUP] = self._to_sound(pu, 0.42)
        self.sounds[E.SFX_HOVERBOARD_ON] = self._to_sound(self._sweep(260.0, 920.0, 0.28, 0.12), 0.42)
        self.sounds[E.SFX_HOVERBOARD_BREAK] = self._to_sound(self._sweep(760.0, 110.0, 0.36, 0.45), 0.48)

        # Level Complete celebratory Afrobeat / Makossa brass-marimba fanfare!
        win_notes = [523.25, 659.25, 783.99, 659.25, 1046.5, 1318.5]
        win = np.concatenate([
            self._tone(f, 0.11 if i < 4 else 0.24, "bell", 7.0 if i < 4 else 4.0)
            for i, f in enumerate(win_notes)
        ])
        self.sounds[E.SFX_LEVEL_COMPLETE] = self._to_sound(win, 0.48)
        self.sounds[E.SFX_MILESTONE] = self._to_sound(win[: int(self.SAMPLE_RATE * 0.45)], 0.38)

        # Upbeat 4-bar Douala Makossa / Afrobeat rhythmic groove loop (~122 BPM)
        self.music_loop = self._build_makossa_groove()

    def _build_makossa_groove(self) -> pygame.mixer.Sound:
        sr = self.SAMPLE_RATE
        bpm = 122.0
        step_dur = 60.0 / bpm / 4.0  # 16th-note step
        steps = 64  # 4 bars
        total_samples = int(steps * step_dur * sr)
        mix = np.zeros(total_samples, dtype=np.float64)
        rng = np.random.default_rng(2377)

        # Syncopated Makossa bassline (key of C major / A minor infectious bounce!)
        bass_pattern = {
            0: 130.81, 3: 130.81, 6: 164.81, 8: 196.00, 11: 164.81, 14: 146.83,
            16: 110.00, 19: 110.00, 22: 130.81, 24: 164.81, 27: 146.83, 30: 130.81,
            32: 174.61, 35: 174.61, 38: 196.00, 40: 220.00, 43: 196.00, 46: 174.61,
            48: 196.00, 51: 196.00, 54: 246.94, 56: 196.00, 59: 164.81, 62: 146.83,
        }
        # Bright tropical balafon / guitar riff accents
        riff_pattern = {
            2: 523.25, 4: 659.25, 7: 783.99, 10: 659.25, 12: 587.33,
            18: 523.25, 20: 659.25, 23: 880.00, 26: 783.99, 28: 659.25,
            34: 698.46, 36: 880.00, 39: 1046.5, 42: 880.00, 44: 783.99,
            50: 783.99, 52: 987.77, 55: 1174.66, 58: 987.77, 60: 783.99,
        }

        for st in range(steps):
            idx0 = int(st * step_dur * sr)

            # Warm 4-on-the-floor + syncopated Makossa kick
            if st % 4 == 0 or st % 16 == 14:
                klen = int(0.11 * sr)
                tk = np.linspace(0.0, 0.11, klen, endpoint=False)
                kf = 125.0 * np.exp(-tk * 28.0) + 46.0
                kick = np.sin(2.0 * np.pi * np.cumsum(kf) / sr) * np.exp(-tk * 16.0) * 0.42
                end = min(total_samples, idx0 + klen)
                mix[idx0:end] += kick[: end - idx0]

            # Crisp shaker on every offbeat
            if st % 2 == 1:
                slen = int(0.045 * sr)
                ts = np.linspace(0.0, 0.045, slen, endpoint=False)
                shaker = rng.uniform(-1.0, 1.0, slen) * np.exp(-ts * 65.0) * 0.08
                end = min(total_samples, idx0 + slen)
                mix[idx0:end] += shaker[: end - idx0]

            # Rimshot clave groove (classic Central African rhythm!)
            if st % 16 in (4, 10, 12):
                rlen = int(0.05 * sr)
                tr = np.linspace(0.0, 0.05, rlen, endpoint=False)
                rim = np.sin(2.0 * np.pi * 880.0 * tr) * np.exp(-tr * 55.0) * 0.14
                end = min(total_samples, idx0 + rlen)
                mix[idx0:end] += rim[: end - idx0]

            # Warm bass bounce
            if st in bass_pattern:
                f = bass_pattern[st]
                blen = int(0.24 * sr)
                tb = np.linspace(0.0, 0.24, blen, endpoint=False)
                bass = (np.sin(2.0 * np.pi * f * tb) + 0.35 * np.sin(4.0 * np.pi * f * tb)) * np.exp(-tb * 7.5) * 0.28
                end = min(total_samples, idx0 + blen)
                mix[idx0:end] += bass[: end - idx0]

            # Sunny balafon/marimba melody
            if st in riff_pattern:
                f = riff_pattern[st]
                mlen = int(0.18 * sr)
                tm = np.linspace(0.0, 0.18, mlen, endpoint=False)
                mar = (np.sin(2.0 * np.pi * f * tm) + 0.25 * np.sin(6.0 * np.pi * f * tm)) * np.exp(-tm * 13.0) * 0.16
                end = min(total_samples, idx0 + mlen)
                mix[idx0:end] += mar[: end - idx0]

        return self._to_sound(mix, 0.34)

    def start_music(self) -> None:
        if not self.enabled or self.muted or self.music_loop is None:
            return
        if self.music_channel is None or not self.music_channel.get_busy():
            self.music_channel = self.music_loop.play(loops=-1)

    def stop_music(self) -> None:
        if self.music_channel is not None:
            self.music_channel.stop()

    def set_muted(self, muted: bool) -> None:
        self.muted = muted
        if muted:
            self.stop_music()
        else:
            self.start_music()

    def play_events(self, events: List[int]) -> None:
        if not self.enabled or self.muted:
            return
        E = self.eng_mod
        for ev in events:
            if ev == E.SFX_COIN and self.coin_scale:
                self.coin_scale[self._coin_step % len(self.coin_scale)].play()
                self._coin_step = (self._coin_step + 1) % len(self.coin_scale)
            elif ev in self.sounds:
                if ev in (E.SFX_CRASH, E.SFX_STUMBLE):
                    self._coin_step = 0
                self.sounds[ev].play()
