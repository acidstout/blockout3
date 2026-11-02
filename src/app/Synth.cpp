/*
  File:        Synth.cpp
  Description: Procedural sound effects (no OS)
  Program:     Block Out III

  Every sound is built from a few oscillators and filtered noise, shaped
  after the BlockOut II samples and the PC speaker sounds of the 1989 DOS
  Blockout (frequencies and timings measured from the originals).

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/
#include "Synth.h"
#include <algorithm>
#include <cmath>

namespace bo3 {

namespace {

const double TAU = 6.283185307179586;

enum Wave { SINE, SQUARE, TRIANGLE, SAW };

// PolyBLEP correction around a discontinuity at phase 0 (dt: phase step)
double Blep(double p,double dt) {
  if( p<dt )     { p /= dt;       return p+p-p*p-1.0; }
  if( p>1.0-dt ) { p = (p-1.0)/dt; return p*p+p+p+1.0; }
  return 0.0;
}

// Band-limited oscillators: no aliasing of the square and saw edges
double Osc(Wave w,double phase,double dt) {   // phase in cycles
  double p = phase - floor(phase);
  switch(w) {
    case SINE:     return sin(TAU*p);
    case SQUARE: {
      double q = p+0.5 - floor(p+0.5);
      return (p<0.5 ? 1.0 : -1.0) + Blep(p,dt) - Blep(q,dt);
    }
    case TRIANGLE: return p<0.5 ? 4.0*p-1.0 : 3.0-4.0*p;
    case SAW:      return 2.0*p-1.0 - Blep(p,dt);
  }
  return 0.0;
}

struct Buffer {

  std::vector<float> s;
  explicit Buffer(double seconds) : s((size_t)(seconds*SYNTH_RATE),0.0f) {}

  // Envelope: linear attack, then exponential decay (tau seconds) or, with
  // tau 0, a flat level; a 3 ms fade at both ends avoids clicks
  static double Env(double t,double dur,double attack,double tau) {
    double e = attack>0 && t<attack ? t/attack : (tau>0 ? exp(-(t-attack)/tau) : 1.0);
    double edge = std::min(t,dur-t) / 0.003;
    return edge<1.0 ? e*std::max(0.0,edge) : e;
  }

  // Oscillator gliding exponentially from f0 to f1
  void Tone(double t0,double dur,double f0,double f1,Wave w,double amp,double attack = 0.002,double tau = 0.0,
            double vibrato = 0.0) {
    size_t a = (size_t)(t0*SYNTH_RATE), n = (size_t)(dur*SYNTH_RATE);
    double phase = 0.0;
    for(size_t i=0;i<n && a+i<s.size();i++) {
      double t = (double)i/SYNTH_RATE;
      double f = f0 * pow(f1/f0,t/dur) * (1.0 + vibrato*sin(TAU*6.0*t));
      phase += f/SYNTH_RATE;
      s[a+i] += (float)(amp * Env(t,dur,attack,tau) * Osc(w,phase,f/SYNTH_RATE));
    }
  }

  // Low-passed white noise (two poles), optional tremolo
  void Noise(double t0,double dur,double cutoff,double amp,double attack,double tau,double tremolo = 0.0) {
    size_t a = (size_t)(t0*SYNTH_RATE), n = (size_t)(dur*SYNTH_RATE);
    double k = 1.0 - exp(-TAU*cutoff/SYNTH_RATE), y = 0.0, y2 = 0.0;
    uint32_t seed = 0x12345678u;
    for(size_t i=0;i<n && a+i<s.size();i++) {
      seed = seed*1664525u + 1013904223u;
      double white = (double)(seed>>8) / 8388608.0 - 1.0;
      y += k * (white - y);
      y2 += k * (y - y2);
      double t = (double)i/SYNTH_RATE;
      double trem = 1.0 - tremolo*0.5*(1.0 + sin(TAU*7.0*t)*sin(TAU*3.1*t));
      // the filter loses level at low cutoffs: compensate roughly
      s[a+i] += (float)(amp * Env(t,dur,attack,tau) * trem * y2 / sqrt(k*0.5));
    }
  }

  std::vector<int16_t> Pcm() const {
    std::vector<int16_t> out(s.size());
    for(size_t i=0;i<s.size();i++)
      out[i] = (int16_t)lrintf(std::clamp(s[i],-1.0f,1.0f) * 32767.0f);
    return out;
  }
};

// PC speaker: square notes {frequency Hz (0 = silence), seconds}
struct Note { double f, d; };

std::vector<int16_t> Speaker(const std::vector<Note> &notes,double amp) {
  double total = 0.0;
  for(const Note &n : notes) total += n.d;
  Buffer b(total);
  double t = 0.0;
  for(const Note &n : notes) {
    if( n.f>0 ) b.Tone(t,n.d,n.f,n.f,SQUARE,amp,0.001);
    t += n.d;
  }
  return b.Pcm();
}

} // namespace

//-----------------------------------------------------------------------------

const char *SoundName(Sound s) {
  static const char *names[] = { "blub", "wozz", "tchh", "line", "level", "empty", "welldone",
                                 "line2", "level2", "empty2", "welldone2", "hit" };
  return names[(int)s];
}

std::vector<int16_t> Synthesize(Sound s) {

  switch(s) {

    case Sound::Blub: {         // menu move: short rising bubble
      Buffer b(0.11);
      b.Tone(0.0,0.11,380,900,SINE,0.42,0.002,0.035);
      b.Tone(0.0,0.11,760,1800,SINE,0.06,0.002,0.025);
      return b.Pcm();
    }

    case Sound::Tchh: {         // intro letter lands: noise burst
      Buffer b(0.10);
      b.Noise(0.0,0.10,2500,0.26,0.001,0.030);
      return b.Pcm();
    }

    case Sound::Hit: {          // blocked rotation: low thump
      Buffer b(0.12);
      b.Tone(0.0,0.12,140,55,SINE,0.38,0.001,0.045);
      b.Tone(0.0,0.12,210,195,TRIANGLE,0.18,0.001,0.050);
      b.Noise(0.0,0.012,3000,0.15,0.0,0.004);
      return b.Pcm();
    }

    case Sound::Wozz: {         // intro letters fly: low rumbling whoosh
      Buffer b(1.15);
      b.Noise(0.0,1.15,140,0.75,0.03,0.45,0.8);
      b.Noise(0.0,1.15,900,0.05,0.05,0.30,0.5);
      return b.Pcm();
    }

    case Sound::Line: {         // layer removed: low buzzing thud
      Buffer b(0.56);
      b.Tone(0.0,0.56,52,40,SINE,0.55,0.004,0.18,0.04);
      b.Tone(0.0,0.56,100,82,TRIANGLE,0.20,0.004,0.15);
      b.Noise(0.0,0.05,300,0.20,0.001,0.02);
      return b.Pcm();
    }

    case Sound::Level: {        // next level: two bell strikes
      Buffer b(2.1);
      const double first[3][2]  = { {1050,0.14}, {1760,0.09}, {2083,0.06} };
      const double second[3][2] = { {791,0.14}, {1184,0.09}, {1776,0.05} };
      for(const auto &p : first)  b.Tone(0.0,2.1,p[0],p[0]*1.001,SINE,p[1],0.003,0.55);
      for(const auto &p : second) b.Tone(0.2,1.9,p[0],p[0]*1.001,SINE,p[1],0.003,0.55);
      return b.Pcm();
    }

    case Sound::Empty: {        // empty pit: falling sweep
      Buffer b(1.08);
      b.Tone(0.0,1.08,2700,560,SAW,0.22,0.20,0.20);
      b.Tone(0.0,1.08,2700,560,SINE,0.18,0.20,0.20);
      return b.Pcm();
    }

    case Sound::WellDone: {     // new high score. The original is a voice
      Buffer b(1.35);           // sample; a short fanfare replaces it
      const double notes[4][3] = { {523.25,0.00,0.14}, {659.25,0.14,0.14}, {783.99,0.28,0.14}, {1046.5,0.42,0.93} };
      for(const auto &n : notes) {
        bool last = n[0]>1000;
        b.Tone(n[1],n[2],n[0],n[0],SQUARE,0.20,0.005,last ? 0.45 : 0.0,last ? 0.006 : 0.0);
        b.Tone(n[1],n[2],n[0]*0.5,n[0]*0.5,SAW,0.16,0.005,last ? 0.45 : 0.0);
      }
      return b.Pcm();
    }

    // ---- PC speaker sounds (DOS Blockout) --------------------------------

    case Sound::Line2:          // layer removed: short click
      return Speaker({ {409,0.025}, {0,0.01} },0.06);

    case Sound::Level2: {       // next level: three falling chirps
      Buffer b(0.11);
      for(int i=0;i<3;i++) b.Tone(i*0.036,0.036,5000,900,SQUARE,0.13);
      return b.Pcm();
    }

    case Sound::Empty2:         // empty pit: fast high warble
      return Speaker({ {6826,0.021}, {4619,0.021}, {5437,0.042}, {4005,0.021}, {5615,0.021},
                       {4813,0.021}, {4210,0.021}, {4188,0.021}, {2810,0.021}, {3623,0.042} },0.11);

    case Sound::WellDone2:      // new high score: little tune
      return Speaker({ {851,0.07}, {0,0.01}, {757,0.04}, {0,0.01}, {632,0.07}, {0,0.01},
                       {752,0.04}, {0,0.02}, {630,0.06}, {0,0.02}, {554,0.08}, {0,0.02},
                       {554,0.06}, {0,0.02}, {630,0.06}, {0,0.02}, {755,0.12}, {0,0.02},
                       {754,0.04}, {0,0.02}, {754,0.07}, {0,0.02} },0.06);
  }
  return {};

}

} // namespace bo3
