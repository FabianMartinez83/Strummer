
// Strummer created by Fabian Martinez
// https://github.com/FabianMartinez    
// This code is licensed under the MIT License.

/*
============
MIT License
============

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.







*/










#include <distingnt/api.h>
#include <cmath>
#include <cstring>

// --- Constants ---
#define NUM_STANDARD_SCALES 16
#define NUM_EXOTIC_SCALES 117
#define NUM_SCALES (NUM_STANDARD_SCALES + NUM_EXOTIC_SCALES)
#define SCALE_MAX_LEN 20 // Maximum number of notes in a scale
#define SAMPLE_RATE 44100

// --- Parameter enum for clarity ---
enum {
    kParamGateUp,      // Input: Gate/trigger to start sequence forward
    kParamGateDown,    // Input: Gate/trigger to start sequence backward
    kParamOutputPitch, // Output: Sequenced pitch
    kParamScale,       // Scale selection
    kParamSpacing,     // Time between notes (ms)
    kParamLength,      // Number of notes in sequence
    kParamTranspose,   // Transpose in semitones
    kParamMaskRotate,  // Rotate mask (offset in scale)
    kParamTUpOut,      // Output: Mirror of Trig Up input
    kParamTDownOut,    // Output: Mirror of Trig Down input
    kParamAttack,      // Attack time (ms)
    kParamDecay,       // Decay time (ms)
    kParamSustain,     // Sustain level (0..1)
    kParamRelease,     // Release time (ms)
    kParamEnvShape,    // 0 = linear, 1 = exponential
    kParamEnvExponent, // <--- add this
    kParamGateLen,      //Gate length (ms)
    kParamGateUPOut,   // New: GateUP_Out
    kParamGateDNOut,   // New: GateDN_Out
    kNumParams         // last parameter
};

// --- State struct (holds persistent state for each instance) ---
struct GateEnvelope {
    enum Stage { Off, Attack, Decay, Sustain, Release } stage = Off;
    float value = 0.0f;
    int counter = 0;
};

struct StrumState {
    int stepIndex = -1;         // Current step in the sequence
    int stepInc = 1;            // Direction: 1 for up, -1 for down
    int msCounter = 0;          // Countdown for note spacing
    float lastGateUp = 0.0f;    // Last value of Trig Up input (for edge detection)
    float lastGateDown = 0.0f;  // Last value of Trig Down input (for edge detection)
    float currentPitch = 0.0f;  // Holds the current pitch to output
    int tUpPulse = 0;      // <-- add this
    int tDownPulse = 0;    // <-- add this
    GateEnvelope tUpEnv;
    GateEnvelope tDownEnv;
};

// --- All scale names (standard + exotic) ---
static const char* all_scale_names[NUM_SCALES] = {
    // Standard scales
    "Major", "Minor", "Harmonic Minor", "Melodic Minor", "Mixolydian", "Dorian", "Lydian", "Phrygian",
    "Aeolian", "Locrian", "Maj Pent", "Min Pent", "Whole Tone", "Octatonic HW", "Octatonic WH", "Ionian",
    // Exotic scales (names must match the order and count of your exotic scales)
    "Blues Major", "Blues Minor", "Folk", "Japanese", "Gamelan", "Gypsy", "Arabian", "Flamenco",
    "Whole Tone (Exotic)", "Pythagorean", "1/4-EB", "1/4-E", "1/4-EA", "Bhairav", "Gunakri", "Marwa",
    "Shree", "Purvi", "Bilawal", "Yaman", "Kafi", "Bhimpalasree", "Darbari", "Rageshree",
    "Khamaj", "Mimal", "Parameshwari", "Rangeshwari", "Gangeshwari", "Kameshwari", "Pa_Kafi", "Natbhairav",
    "M_Kauns", "Bairagi", "B_Todi", "Chandradeep", "Kaushik_Todi", "Jogeshwari",
    "Tartini-Vallotti", "13/22-tET", "13/19-tET", "Magic145", "Quartaminorthirds", "Armodue",
    "Hirajoshi", "Scottish Bagpipes", "Thai Ranat",
    "Sevish 31-EDO", "11TET Machine", "13TET Father", "15TET Blackwood", "16TET Mavila", "16TET Mavila9", "17TET Superpyth",
    "22TET Orwell", "22TET Pajara", "22TET Pajara2", "22TET Porcupine", "26TET Flattone", "26TET Lemba", "46TET Sensi",
    "53TET Orwell", "72TET Prent", "Zeus Trivalent", "202TET Octone", "313TET Elfmadagasgar", "Marvel Glumma", "TOP Parapyth",
    "16ED", "15ED", "14ED", "13ED", "11ED", "10ED", "9ED", "8ED", "7ED", "6ED", "5ED",
    "16HD2", "15HD2", "14HD2", "13HD2", "12HD2", "11HD2", "10HD2", "9HD2", "8HD2", "7HD2", "6HD2", "5HD2",
    "32-16SD2", "30-15SD2", "28-14SD2", "26-13SD2", "24-12SD2", "22-11SD2", "20-10SD2", "18-9SD2", "16-8SD2", "14-7SD2", "12-6SD2", "10-5SD2", "8-4SD2",
    "BP Equal", "BP Just", "BP Lambda",
    "8-24HD3", "7-21HD3", "6-18HD3", "5-15HD3", "4-12HD3", "24-8HD3", "21-7HD3", "18-6HD3", "15-5HD3", "12-4HD3"
    // Add more names if you have more exotic scales, up to NUM_EXOTIC_SCALES
};

// --- Exotic scale intervals (static, truncated for brevity) ---

static const float exotic_scales[NUM_EXOTIC_SCALES][SCALE_MAX_LEN] = {


   // Blues major (From midipal/BitT source code)
    { 0.0f, 3.0f, 4.0f, 7.0f, 9.0f, 10.0f },
    // Blues minor (From midipal/BitT source code)
    { 0.0f, 3.0f, 5.0f, 6.0f, 7.0f, 10.0f },

    // Folk (From midipal/BitT source code)
    { 0.0f, 1.0f, 3.0f, 4.0f, 5.0f, 7.0f, 8.0f, 10.0f },
    // Japanese (From midipal/BitT source code)
    { 0.0f, 1.0f, 5.0f, 7.0f, 8.0f },
    // Gamelan (From midipal/BitT source code)
    { 0.0f, 1.0f, 3.0f, 7.0f, 8.0f },
    // Gypsy
    { 0.0f, 2.0f, 3.0f, 6.0f, 7.0f, 8.0f, 11.0f },
    // Arabian
    { 0.0f, 1.0f, 4.0f, 5.0f, 7.0f, 8.0f, 11.0f },
    // Flamenco
    { 0.0f, 1.0f, 4.0f, 5.0f, 7.0f, 8.0f, 10.0f },
    // Whole tone (From midipal/BitT source code)
    { 0.0f, 2.0f, 4.0f, 6.0f, 8.0f, 10.0f },
    // pythagorean (From yarns source code)
    { 0.0f, 0.898f, 2.039f, 2.938f, 4.078f, 4.977f, 6.117f, 7.023f, 7.922f, 9.062f, 9.961f, 11.102f },
    // 1_4_eb (From yarns source code)
    { 0.0f, 1.0f, 2.0f, 3.0f, 3.5f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 10.5f },
    // 1_4_e (From yarns source code)
    { 0.0f, 1.0f, 2.0f, 3.0f, 3.5f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f },
    // 1_4_ea (From yarns source code)
    { 0.0f, 1.0f, 2.0f, 3.0f, 3.5f, 5.0f, 6.0f, 7.0f, 8.0f, 8.5f, 10.0f, 11.0f },
    // bhairav (From yarns source code)
    { 0.0f, 0.898f, 3.859f, 4.977f, 7.023f, 7.922f, 10.883f },
    // gunakri (From yarns source code)
    { 0.0f, 1.117f, 4.977f, 7.023f, 8.141f },
    // marwa (From yarns source code)
    { 0.0f, 1.117f, 3.859f, 5.898f, 8.844f, 10.883f },
    // shree (From yarns source code)
    { 0.0f, 0.898f, 3.859f, 5.898f, 7.023f, 7.922f, 10.883f },
    // purvi (From yarns source code)
    { 0.0f, 1.117f, 3.859f, 5.898f, 7.023f, 8.141f, 10.883f },
    // bilawal (From yarns source code)
    { 0.0f, 2.039f, 3.859f, 4.977f, 7.023f, 9.062f, 10.883f },
    // yaman (From yarns source code)
    { 0.0f, 2.039f, 4.078f, 6.117f, 7.023f, 9.062f, 11.102f },
    // kafi (From yarns source code)
    { 0.0f, 1.820f, 2.938f, 4.977f, 7.023f, 8.844f, 9.961f },
    // bhimpalasree (From yarns source code)
    { 0.0f, 2.039f, 3.156f, 4.977f, 7.023f, 9.062f, 10.180f },
    // darbari (From yarns source code)
    { 0.0f, 2.039f, 2.938f, 4.977f, 7.023f, 7.922f, 9.961f },
    // rageshree (From yarns source code)
    { 0.0f, 2.039f, 3.859f, 4.977f, 7.023f, 8.844f, 9.961f },
    // khamaj (From yarns source code)
    { 0.0f, 2.039f, 3.859f, 4.977f, 7.023f, 9.062f, 9.961f, 11.102f },
    // mimal (From yarns source code)
    { 0.0f, 2.039f, 2.938f, 4.977f, 7.023f, 8.844f, 9.961f, 10.883f },
    // parameshwari (From yarns source code)
    { 0.0f, 0.898f, 2.938f, 4.977f, 8.844f, 9.961f },
    // rangeshwari (From yarns source code)
    { 0.0f, 2.039f, 2.938f, 4.977f, 7.023f, 10.883f },
    // gangeshwari (From yarns source code)
    { 0.0f, 3.859f, 4.977f, 7.023f, 7.922f, 9.961f },
    // kameshwari (From yarns source code)
    { 0.0f, 2.039f, 5.898f, 7.023f, 8.844f, 9.961f },
    // pa__kafi (From yarns source code)
    { 0.0f, 2.039f, 2.938f, 4.977f, 7.023f, 9.062f, 9.961f },
    // natbhairav (From yarns source code)
    { 0.0f, 2.039f, 3.859f, 4.977f, 7.023f, 7.922f, 10.883f },
    // m_kauns (From yarns source code)
    { 0.0f, 2.039f, 4.078f, 4.977f, 7.922f, 9.961f },
    // bairagi (From yarns source code)
    { 0.0f, 0.898f, 4.977f, 7.023f, 9.961f },
    // b_todi (From yarns source code)
    { 0.0f, 0.898f, 2.938f, 7.023f, 9.961f },
    // chandradeep (From yarns source code)
    { 0.0f, 2.938f, 4.977f, 7.023f, 9.961f },
    // kaushik_todi (From yarns source code)
    { 0.0f, 2.938f, 4.977f, 5.898f, 7.922f },
    // jogeshwari (From yarns source code)
    { 0.0f, 2.938f, 3.859f, 4.977f, 8.844f, 9.961f },

    // Tartini-Vallotti [12]
    { 0.0f, 0.9375f, 1.9609f, 2.9766f, 3.9219f, 5.0234f, 5.9219f, 6.9766f, 7.9609f, 8.9375f, 10.0f, 10.8984f },
    // 13 out of 22-tET, generator = 5 [13]
    { 0.0f, 1.0938f, 2.1797f, 3.2734f, 3.8203f, 4.9063f, 6.0f, 6.5469f, 7.6328f, 8.7266f, 9.2734f, 10.3672f, 11.4531f },
    // 13 out of 19-tET, Mandelbaum [13]
    { 0.0f, 1.2656f, 1.8984f, 3.1563f, 3.7891f, 5.0547f, 5.6875f, 6.9453f, 7.5781f, 8.8438f, 9.4766f, 10.7344f, 11.3672f },
    // Magic[16] in 145-tET [16]
    { 0.0f, 1.4922f, 2.0703f, 2.6484f, 3.2266f, 3.8047f, 4.3828f, 5.8750f, 6.4531f, 7.0313f, 7.6172f, 8.1953f, 9.6797f, 10.2656f, 10.8438f, 11.4219f },
    // g=9 steps of 139-tET. Gene Ward Smith "Quartaminorthirds" 7-limit temperament [16]
    { 0.0f, 0.7734f, 1.5547f, 2.3281f, 3.1094f, 3.8828f, 4.6641f, 5.4375f, 6.2188f, 6.9922f, 7.7734f, 8.5469f, 9.3203f, 10.1016f, 10.8750f, 11.6563f },
    // Armodue semi-equalizzato [16]
    { 0.0f, 0.7734f, 1.5469f, 2.3203f, 3.0938f, 3.8672f, 4.6484f, 5.4219f, 6.1953f, 6.9688f, 7.7422f, 8.5156f, 9.2891f, 9.6797f, 10.4531f, 11.2266f },

    // Hirajoshi[5]
    { 0.0f, 1.8516f, 3.3672f, 6.8281f, 7.8984f },
    // Scottish bagpipes[7]
    { 0.0f, 1.9688f, 3.4063f, 4.9531f, 7.0313f, 8.5313f, 10.0938f },
    // Thai ranat[7]
    { 0.0f, 1.6094f, 3.4609f, 5.2578f, 6.8594f, 8.6172f, 10.2891f },

    // Sevish quasi-12-equal mode from 31-EDO
    { 0.0f, 1.1641f, 2.3203f, 3.0938f, 4.2578f, 5.0313f, 6.1953f, 7.3516f, 8.1328f, 9.2891f, 10.0625f, 11.2266f },
    // 11 TET Machine[6]
    { 0.0f, 2.1797f, 4.3672f, 5.4531f, 7.6328f, 9.8203f },
    // 13 TET Father[8]
    { 0.0f, 1.8438f, 3.6953f, 4.6172f, 6.4609f, 8.3047f, 9.2344f, 11.0781f },
    // 15 TET Blackwood[10]
    { 0.0f, 1.6016f, 2.3984f, 4.0f, 4.7969f, 6.3984f, 7.2031f, 8.7969f, 9.6016f, 11.2031f },
    // 16 TET Mavila[7]
    { 0.0f, 1.5f, 3.0f, 5.25f, 6.75f, 8.25f, 9.75f },
    // 16 TET Mavila[9]
    { 0.0f, 0.75f, 2.25f, 3.75f, 5.25f, 6.0f, 7.5f, 9.0f, 10.5f },
    // 17 TET Superpyth[12]
    { 0.0f, 0.7031f, 1.4141f, 2.8203f, 3.5313f, 4.9375f, 5.6484f, 6.3516f, 7.7578f, 8.4688f, 9.8828f, 10.5859f },

    // 22 TET Orwell[9]
    { 0.0f, 1.0938f, 2.7266f, 3.8203f, 5.4531f, 6.5469f, 8.1797f, 9.2734f, 10.9063f },
    // 22 TET Pajara[10] Static Symmetrical Maj
    { 0.0f, 1.0938f, 2.1797f, 3.8203f, 4.9063f, 6.0f, 7.0938f, 8.1797f, 9.8203f, 10.9063f },
    // 22 TET Pajara[10] Std Pentachordal Maj
    { 0.0f, 1.0938f, 2.1797f, 3.8203f, 4.9063f, 6.0f, 7.0938f, 8.7266f, 9.8203f, 10.9063f },
    // 22 TET Porcupine[7]
    { 0.0f, 1.6328f, 3.2734f, 4.9063f, 7.0938f, 8.7266f, 10.3672f },
    // 26 TET Flattone[12]
    { 0.0f, 0.4609f, 1.8438f, 2.3047f, 3.6953f, 5.0781f, 5.5391f, 6.9219f, 7.3828f, 8.7656f, 9.2266f, 10.6172f },
    // 26 TET Lemba[10]
    { 0.0f, 1.3828f, 2.3047f, 3.6953f, 4.6172f, 6.0f, 7.3828f, 8.3047f, 9.6875f, 10.6172f },
    // 46 TET Sensi[11]
    { 0.0f, 1.3047f, 2.6094f, 3.9141f, 4.4375f, 5.7422f, 7.0469f, 8.3516f, 8.8672f, 10.1719f, 11.4766f },
    // 53 TET Orwell[9]
    { 0.0f, 1.1328f, 2.7188f, 3.8516f, 5.4375f, 6.5625f, 8.1484f, 9.2813f, 10.8672f },
    // 12 out of 72-TET scale by Prent Rodgers
    { 0.0f, 2.0f, 2.6641f, 3.8359f, 4.3359f, 5.0f, 5.5f, 7.0f, 8.8359f, 9.6641f, 10.5f, 10.8359f },
    // Trivalent scale in zeus temperament[7]
    { 0.0f, 1.5781f, 3.8750f, 5.4531f, 7.0313f, 9.3359f, 10.9063f },
    // 202 TET tempering of octone[8]
    { 0.0f, 1.1875f, 3.5078f, 3.8594f, 6.1797f, 7.0078f, 9.3281f, 9.6797f },
    // 313 TET elfmadagasgar[9]
    { 0.0f, 2.0313f, 2.4922f, 4.5234f, 4.9844f, 7.0156f, 7.4766f, 9.5078f, 9.9688f },
    // Marvel woo version of glumma[12]
    { 0.0f, 0.4922f, 2.3281f, 3.1719f, 3.8359f, 5.4922f, 6.1641f, 7.0078f, 8.8359f, 9.3281f, 9.6797f, 11.6563f },
    // TOP Parapyth[12]
    { 0.0f, 0.5859f, 2.0703f, 2.6563f, 4.1406f, 4.7266f, 5.5469f, 7.0469f, 7.6172f, 9.1094f, 9.6875f, 11.1797f },

    // 16-ED (ED2 or ED3)
    { 0.0f, 0.75f, 1.5f, 2.25f, 3.0f, 3.75f, 4.5f, 5.25f, 6.0f, 6.75f, 7.5f, 8.25f, 9.0f, 9.75f, 10.5f, 11.25f },
    // 15-ED (ED2 or ED3)
    { 0.0f, 0.7969f, 1.6016f, 2.3984f, 3.2031f, 4.0f, 4.7969f, 5.6016f, 6.3984f, 7.2031f, 8.0f, 8.7969f, 9.6016f, 10.3984f, 11.2031f },
    // 14-ED (ED2 or ED3)
    { 0.0f, 0.8594f, 1.7109f, 2.5703f, 3.4297f, 4.2891f, 5.1484f, 6.0f, 6.8594f, 7.7188f, 8.5781f, 9.4375f, 10.2969f, 11.1563f },
    // 13-ED (ED2 or ED3)
    { 0.0f, 0.9219f, 1.8438f, 2.7656f, 3.6953f, 4.6328f, 5.6328f, 6.5703f, 7.4922f, 8.4141f, 9.3359f, 10.2578f, 11.1797f },
    // 11-ED (ED2 or ED3)
    { 0.0f, 1.0938f, 2.1797f, 3.2734f, 4.3672f, 5.4531f, 6.5469f, 7.6328f, 8.7266f, 9.8203f, 10.9063f },
    // 10-ED (ED2 or ED3)
    { 0.0f, 1.2031f, 2.3984f, 3.6016f, 4.7969f, 6.0f, 7.2031f, 8.3984f, 9.6016f, 10.7969f },
    // 9-ED (ED2 or ED3)
    { 0.0f, 1.3359f, 2.6641f, 4.0f, 5.3359f, 6.6641f, 8.0f, 9.3359f, 10.6641f },
    // 8-ED (ED2 or ED3)
    { 0.0f, 1.5f, 3.0f, 4.5f, 6.0f, 7.5f, 9.0f, 10.5f },
    // 7-ED (ED2 or ED3)
    { 0.0f, 1.7109f, 3.4297f, 5.1484f, 6.8594f, 8.5781f, 10.2969f },
    // 6-ED (ED2 or ED3)
    { 0.0f, 2.0f, 4.0f, 6.0f, 8.0f, 10.0f },
    // 5-ED (ED2 or ED3)
    { 0.0f, 2.3984f, 4.7969f, 7.2031f, 9.6016f },

    // 16-HD2 (16 step harmonic series scale on the octave)
    { 0.0f, 1.0469f, 2.0391f, 2.9766f, 3.8594f, 4.7109f, 5.5156f, 6.2813f, 7.0234f, 7.7266f, 8.4063f, 9.0625f, 9.6875f, 10.2969f, 10.8906f, 11.4531f },
    // 15-HD2 (15 step harmonic series scale on the octave)
    { 0.0f, 1.1172f, 2.1641f, 3.1563f, 4.0938f, 4.9766f, 5.8203f, 6.6328f, 7.4141f, 8.1641f, 8.8828f, 9.5703f, 10.2266f, 10.852f, 11.4453f },
    // 14-HD2 (14 step harmonic series scale on the octave)
    { 0.0f, 1.1953f, 2.3125f, 3.3594f, 4.3516f, 5.2891f, 6.1797f, 7.0313f, 7.8516f, 8.6406f, 9.3984f, 10.125f, 10.8203f, 11.4844f },
    // 13-HD2 (13 step harmonic series scale on the octave)
    { 0.0f, 1.2813f, 2.4766f, 3.5938f, 4.6406f, 5.6328f, 6.5703f, 7.4609f, 8.3125f, 9.125f, 9.9063f, 10.6484f, 11.3594f },
    // 12-HD2 (12 step harmonic series scale on the octave)
    { 0.0f, 1.3828f, 2.6719f, 3.8594f, 5.0078f, 6.0313f, 6.9922f, 7.9531f, 8.8438f, 9.6875f, 10.4844f, 11.2656f },
    // 11-HD2 (11 step harmonic series scale on the octave)
    { 0.0f, 1.5078f, 2.8906f, 4.1719f, 5.3672f, 6.4844f, 7.5391f, 8.5234f, 9.4688f, 10.3672f, 11.2109f },
    // 10-HD2 (10 step harmonic series scale on the octave)
    { 0.0f, 1.6484f, 3.1563f, 4.5391f, 5.8672f, 7.0234f, 8.0703f, 9.1875f, 10.1797f, 11.1094f },
    // 9-HD2 (9 step harmonic series scale on the octave)
    { 0.0f, 1.8203f, 3.4766f, 5.0938f, 6.6797f, 8.2422f, 9.7891f, 11.3203f, 12.0f },
    // 8-HD2 (8 step harmonic series scale on the octave)
    { 0.0f, 2.0391f, 3.8594f, 5.5156f, 7.0234f, 8.4063f, 9.6875f, 10.8906f },
    // 7-HD2 (7 step harmonic series scale on the octave)
    { 0.0f, 2.3125f, 4.3516f, 6.1797f, 7.8516f, 9.3984f, 10.8203f },
    // 6-HD2 (6 step harmonic series scale on the octave)
    { 0.0f, 3.0313f, 6.0313f, 9.0625f, 12.0f, 15.0f },
    // 5-HD2 (5 step harmonic series scale on the octave)
    { 0.0f, 4.0f, 8.0f, 12.0f, 16.0f },

    // 32-16-SD2 (16 step subharmonic series scale on the octave)
    { 0.0f, 0.5469f, 1.1172f, 1.7031f, 2.3125f, 2.9375f, 3.5938f, 4.2734f, 4.9766f, 5.7188f, 6.4844f, 7.2891f, 8.0234f, 8.9297f, 9.9609f, 10.9531f },
    // 30-15-SD2 (15 step subharmonic series scale on the octave)
    { 0.0f, 0.5859f, 1.1953f, 1.8203f, 2.4766f, 3.1563f, 3.8594f, 4.6016f, 5.3672f, 6.1797f, 7.0313f, 7.9063f, 8.8438f, 9.8359f, 10.8828f },
    // 28-14-SD2 (14 step subharmonic series scale on the octave)
    { 0.0f, 0.6328f, 1.2813f, 1.9609f, 2.6719f, 3.4063f, 4.1719f, 4.977f, 5.8203f, 6.6953f, 7.6328f, 8.6328f, 9.6875f, 10.8047f, 12.0f },
    // 26-13-SD2 (13 step subharmonic series scale on the octave)
    { 0.0f, 0.6797f, 1.3828f, 2.125f, 2.8906f, 3.6953f, 4.5391f, 5.4219f, 6.3516f, 7.3203f, 8.3281f, 9.375f, 10.4609f },
    // 24-12-SD2 (12 step subharmonic series scale on the octave)
    { 0.0f, 0.7344f, 1.5078f, 2.3125f, 3.1563f, 4.0469f, 4.9766f, 5.9531f, 6.9688f, 8.0234f, 9.1172f, 10.25f },
    // 22-11-SD2 (11 step subharmonic series scale on the octave)
    { 0.0f, 0.8047f, 1.6484f, 2.5391f, 3.4766f, 4.4609f, 5.4922f, 6.5703f, 7.6953f, 8.8672f, 10.0859f },
    // 20-10-SD2 (10 step subharmonic series scale on the octave)
    { 0.0f, 0.8906f, 1.8203f, 2.8125f, 3.8594f, 4.9609f, 6.1172f, 7.3281f, 8.5938f, 9.9141f },
    // 18-9-SD2 (9 step subharmonic series scale on the octave)
    { 0.0f, 0.9922f, 2.0391f, 3.1563f, 4.3359f, 5.5781f, 6.8828f, 8.25f, 9.6797f },
    // 16-8-SD2 (8 step subharmonic series scale on the octave)
    { 0.0f, 1.1172f, 2.3125f, 3.5938f, 4.9609f, 6.4141f, 7.9531f, 9.5781f },
    // 14-7-SD2 (7 step subharmonic series scale on the octave)
    { 0.0f, 1.2813f, 2.6719f, 4.1719f, 5.7891f, 7.5234f, 9.375f },
    // 12-6-SD2 (6 step subharmonic series scale on the octave)
    { 0.0f, 1.5078f, 3.1563f, 4.9609f, 6.9219f, 9.0391f },
    // 10-5-SD2 (5 step subharmonic series scale on the octave)
    { 0.0f, 1.8203f, 3.8594f, 6.1719f, 8.8438f },
    // 8-4-SD2 (4 step subharmonic series scale on the octave)
    { 0.0f, 2.3125f, 4.9766f, 8.1406f },

    // Bohlen-Pierce (equal)
    { 0.0f, 0.9219f, 1.8438f, 2.7656f, 3.6953f, 4.6172f, 5.5391f, 6.4609f, 7.3828f, 8.3047f, 9.2344f, 10.1563f, 11.0781f },
    // Bohlen-Pierce (just)
    { 0.0f, 0.8438f, 1.9063f, 2.7422f, 3.6719f, 4.6484f, 5.5781f, 6.4219f, 7.3516f, 8.3281f, 9.2578f, 10.0938f, 11.1563f },
    // Bohlen-Pierce (lambda)
    { 0.0f, 1.9063f, 2.7422f, 3.6719f, 5.5781f, 6.4219f, 8.3281f, 9.2578f, 11.1563f },

    // 8-24-HD3 (16 step harmonic series scale on the tritave)
    { 0.0f, 1.2891f, 2.4375f, 3.4766f, 4.4297f, 5.3047f, 6.1172f, 6.8828f, 7.6172f, 8.3203f, 9.0f, 9.6641f, 10.3125f, 10.9453f, 11.5625f, 12.1563f },
    // 7-21-HD3 (14 step harmonic series scale on the tritave)
    { 0.0f, 1.4609f, 2.7422f, 3.8984f, 4.9375f, 5.8672f, 6.6953f, 7.4297f, 8.0781f, 8.6484f, 9.1484f, 9.5859f, 9.9688f, 10.3047f },
    // 6-18-HD3 (12 step harmonic series scale on the tritave)
    { 0.0f, 1.6875f, 3.1406f, 4.4297f, 5.5703f, 6.5703f, 7.4375f, 8.1797f, 8.8047f, 9.3203f, 9.7344f, 10.0547f },
    // 5-15-HD3 (10 step harmonic series scale on the tritave)
    { 0.0f, 1.9922f, 3.6719f, 5.1328f, 6.3828f, 7.4297f, 8.2813f, 8.9453f, 9.4297f, 9.7422f },
    // 4-12-HD3 (8 step harmonic series scale on the tritave)
    { 0.0f, 2.4375f, 4.4297f, 6.1172f, 7.6172f, 9.0f, 10.3125f, 11.5625f },

    // 24-8-HD3 (16 step subharmonic series scale on the tritave)
    { 0.0f, 0.4688f, 0.9531f, 1.4609f, 1.9922f, 2.5469f, 3.125f, 3.7266f, 4.3516f, 5.0f, 5.6719f, 6.3672f, 7.0859f, 7.8281f, 8.5938f, 9.3828f },
    // 21-7-HD3 (14 step subharmonic series scale on the tritave)
    { 0.0f, 0.5313f, 1.0938f, 1.6875f, 2.3047f, 2.9453f, 3.6094f, 4.2969f, 5.0078f, 5.7422f, 6.5f, 7.2813f, 8.0859f, 8.9141f },
    // 18-6-HD3 (12 step subharmonic series scale on the tritave)
    { 0.0f, 0.625f, 1.2891f, 1.9922f, 2.7344f, 3.5156f, 4.3359f, 5.1953f, 6.0938f, 7.0313f, 8.0078f, 9.0234f },
    // 15-5-HD3 (10 step subharmonic series scale on the tritave)
    { 0.0f, 0.75f, 1.5625f, 2.4375f, 3.375f, 4.375f, 5.4375f, 6.5625f, 7.75f, 9.0f },
    // 12-4-HD3 (8 step subharmonic series scale on the tritave)
    { 0.0f, 0.9531f, 1.9922f, 3.125f, 4.3516f, 5.6719f, 7.0859f, 8.5938f }
};


// --- Standard scale intervals (algorithmic) ---
void get_standard_scale_intervals(int scaleIdx, int* out, int* outLen) {
    // Fills out[] with scale intervals in semitones, sets outLen to number of notes
    switch (scaleIdx) {
        case 0: out[0]=0; out[1]=2; out[2]=4; out[3]=5; out[4]=7; out[5]=9; out[6]=11; out[7]=12; *outLen=8; break; // Major
        case 1: out[0]=0; out[1]=2; out[2]=3; out[3]=5; out[4]=7; out[5]=8; out[6]=10; out[7]=12; *outLen=8; break; // Minor
        case 2: out[0]=0; out[1]=2; out[2]=3; out[3]=5; out[4]=7; out[5]=8; out[6]=11; out[7]=12; *outLen=8; break; // Harmonic Minor
        case 3: out[0]=0; out[1]=2; out[2]=3; out[3]=5; out[4]=7; out[5]=9; out[6]=11; out[7]=12; *outLen=8; break; // Melodic Minor
        case 4: out[0]=0; out[1]=2; out[2]=4; out[3]=5; out[4]=7; out[5]=9; out[6]=10; out[7]=12; *outLen=8; break; // Mixolydian
        case 5: out[0]=0; out[1]=2; out[2]=3; out[3]=5; out[4]=7; out[5]=9; out[6]=10; out[7]=12; *outLen=8; break; // Dorian
        case 6: out[0]=0; out[1]=2; out[2]=4; out[3]=6; out[4]=7; out[5]=9; out[6]=11; out[7]=12; *outLen=8; break; // Lydian
        case 7: out[0]=0; out[1]=1; out[2]=3; out[3]=5; out[4]=7; out[5]=8; out[6]=10; out[7]=12; *outLen=8; break; // Phrygian
        case 8: out[0]=0; out[1]=2; out[2]=3; out[3]=5; out[4]=7; out[5]=8; out[6]=10; out[7]=12; *outLen=8; break; // Aeolian
        case 9: out[0]=0; out[1]=1; out[2]=3; out[3]=5; out[4]=6; out[5]=8; out[6]=10; out[7]=12; *outLen=8; break; // Locrian
        case 10: out[0]=0; out[1]=2; out[2]=4; out[3]=7; out[4]=9; out[5]=12; out[6]=0; out[7]=0; *outLen=5; break; // Maj Pent
        case 11: out[0]=0; out[1]=3; out[2]=5; out[3]=7; out[4]=10; out[5]=12; out[6]=0; out[7]=0; *outLen=5; break; // Min Pent
        case 12: out[0]=0; out[1]=2; out[2]=4; out[3]=6; out[4]=8; out[5]=10; out[6]=12; out[7]=0; *outLen=7; break; // Whole Tone
        case 13: out[0]=0; out[1]=1; out[2]=3; out[3]=4; out[4]=6; out[5]=7; out[6]=9; out[7]=10; *outLen=8; break; // Octatonic HW
        case 14: out[0]=0; out[1]=2; out[2]=3; out[3]=5; out[4]=6; out[5]=8; out[6]=9; out[7]=11; *outLen=8; break; // Octatonic WH
        case 15: out[0]=0; out[1]=2; out[2]=4; out[3]=5; out[4]=7; out[5]=9; out[6]=11; out[7]=12; *outLen=8; break; // Ionian
        default: for (int i = 0; i < SCALE_MAX_LEN; ++i) out[i] = 0; *outLen = 0;
    }
}

// --- Algorithm struct (holds state pointer) ---
struct _strumAlgorithm : public _NT_algorithm {
    StrumState* state;
};

// --- Parameters array (controls and outputs) ---
static const char* envShapeStrings[] = { "Linear", "Simple Exp", "Classic Exp", NULL };

static const _NT_parameter parameters[] = {
    { .name = "Trig Up", .min = 1, .max = 28, .def = 1, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL },
    { .name = "Trig Down", .min = 1, .max = 28, .def = 2, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL },
    { .name = "Strum Out", .min = 1, .max = 28, .def = 13, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL },
    { .name = "Scale", .min = 0, .max = NUM_SCALES-1, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = all_scale_names },
    { .name = "Spacing ms", .min = 1, .max = 1000, .def = 100, .unit = kNT_unitMs, .scaling = 0, .enumStrings = NULL },
    { .name = "Strings", .min = 1, .max = SCALE_MAX_LEN, .def = 6, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL },
    { .name = "Transpose", .min = -48, .max = 48, .def = 0, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL },
    { .name = "Mask Rotate", .min = -15, .max = 15, .def = 0, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL },
    { .name = "TUp-Out", .min = 1, .max = 28, .def = 15, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL },
    { .name = "TDown-Out", .min = 1, .max = 28, .def = 16, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL },
    { .name = "Attack ms", .min = 0, .max = 10000, .def = 0, .unit = kNT_unitMs, .scaling = 0, .enumStrings = NULL },
    { .name = "Decay ms", .min = 0, .max = 10000, .def = 0, .unit = kNT_unitMs, .scaling = 0, .enumStrings = NULL },
    { .name = "Sustain %", .min = 0, .max = 100, .def = 100, .unit = kNT_unitPercent, .scaling = 2, .enumStrings = NULL },
    { .name = "Release ms", .min = 0, .max = 10000, .def = 0, .unit = kNT_unitMs, .scaling = 0, .enumStrings = NULL },
    { .name = "Env Shape", .min = 0, .max = 2, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = envShapeStrings },
    { .name = "Env Exponent", .min = 1, .max = 8, .def = 2, .unit = kNT_unitNone, .scaling = 2, .enumStrings = NULL },
    { .name = "Gate Len ms", .min = 1, .max = 30000, .def = 100, .unit = kNT_unitMs, .scaling = 0, .enumStrings = NULL },
    { .name = "GateUP_Out", .min = 1, .max = 28, .def = 17, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL },
    { .name = "GateDN_Out", .min = 1, .max = 28, .def = 18, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL },
    
};

// --- Envelope processing function ---
float processEnvelope(GateEnvelope& env, bool gate, float attack, float decay, float sustain, float release, int sampleRate, int envShape, float exponent) {
    switch (env.stage) {
        case GateEnvelope::Off:
            if (gate) {
                env.stage = GateEnvelope::Attack;
                env.counter = 0;
            }
            env.value = 0.0f;
            break;
        case GateEnvelope::Attack:
            env.value += 1.0f / (attack * sampleRate);
            if (env.value >= 1.0f || attack == 0.0f) {
                env.value = 1.0f;
                env.stage = GateEnvelope::Decay;
            }
            break;
        case GateEnvelope::Decay:
            env.value -= (1.0f - sustain) / (decay * sampleRate);
            if (env.value <= sustain || decay == 0.0f) {
                env.value = sustain;
                env.stage = GateEnvelope::Sustain;
            }
            break;
        case GateEnvelope::Sustain:
            if (!gate) env.stage = GateEnvelope::Release;
            break;
        case GateEnvelope::Release:
            env.value -= sustain / (release * sampleRate);
            if (env.value <= 0.0f || release == 0.0f) {
                env.value = 0.0f;
                env.stage = GateEnvelope::Off;
            }
            break;
    }
    // --- Shape selection ---
    switch (envShape) {
        case 1: // Simple Exp
            return powf(env.value, exponent);
        case 2: { // Classic Exp
            float k = exponent * 2.0f; // scale exponent for classic exp
            return (1.0f - expf(-k * env.value)) / (1.0f - expf(-k));
        }
        case 0: // Linear
        default:
            return env.value;
    }
}

// --- Main processing loop ---
// This function is called for each audio block to process triggers and output the note sequence.
void step(_NT_algorithm* self, float* busFrames, int numFramesBy4) {
    _strumAlgorithm* alg = (_strumAlgorithm*)self;
    StrumState* state = alg->state;
    int numFrames = numFramesBy4 * 4;

    // --- Read parameters ---
    int scale = alg->v[kParamScale];
    int spacingMs = alg->v[kParamSpacing];
    int length = alg->v[kParamLength];
    int transpose = alg->v[kParamTranspose];
    int maskRotate = alg->v[kParamMaskRotate];
    int gateLen = alg->v[kParamGateLen]; // in ms
    int gateLenSamples = (SAMPLE_RATE * gateLen) / 1000;

    // --- ADSR parameters ---
    float attack = alg->v[kParamAttack] / 1000.0f;   // ms to seconds
    float decay = alg->v[kParamDecay] / 1000.0f;
    float sustain = alg->v[kParamSustain] / 100.0f;  // percent to 0..1
    float release = alg->v[kParamRelease] / 1000.0f;

    // --- Bounds check for sequence length ---
    if (length < 1) length = 1;
    if (length > SCALE_MAX_LEN) length = SCALE_MAX_LEN;

    // --- Prepare scale intervals ---
    int intervals[SCALE_MAX_LEN];
    int scaleIntervals[SCALE_MAX_LEN];
    int scaleLen = 0;
    if (scale < NUM_STANDARD_SCALES) {
        get_standard_scale_intervals(scale, scaleIntervals, &scaleLen);
    } else if (scale < NUM_SCALES) {
        int exoticIdx = scale - NUM_STANDARD_SCALES;
        scaleLen = 0;
        for (int i = 0; i < SCALE_MAX_LEN; ++i) {
            if (exotic_scales[exoticIdx][i] == 0 && i != 0) break;
            scaleIntervals[scaleLen++] = (int)exotic_scales[exoticIdx][i];
        }
    }
    if (scaleLen == 0) return; // No valid scale

    // --- Apply mask rotation and fill intervals array ---
    for (int i = 0; i < length; ++i) {
        intervals[i] = scaleIntervals[(i + maskRotate + scaleLen) % scaleLen];
    }

    // --- Get pointers to input and output buffers ---
    float* gateUp = busFrames + (alg->v[kParamGateUp] - 1) * numFrames;
    float* gateDown = busFrames + (alg->v[kParamGateDown] - 1) * numFrames;
    float* outPitch = busFrames + (alg->v[kParamOutputPitch] - 1) * numFrames;
    float* tUpOut = busFrames + (alg->v[kParamTUpOut] - 1) * numFrames;
    float* tDownOut = busFrames + (alg->v[kParamTDownOut] - 1) * numFrames;
    float* gateUPOut = busFrames + (alg->v[kParamGateUPOut] - 1) * numFrames;
    float* gateDNOut = busFrames + (alg->v[kParamGateDNOut] - 1) * numFrames;

    // --- Main sample loop ---
    for (int i = 0; i < numFrames; ++i) {
        // --- Detect rising edge on Trig Up and Trig Down ---
        bool trigUp = (gateUp[i] > 1.0f && state->lastGateUp <= 1.0f);
        bool trigDown = (gateDown[i] > 1.0f && state->lastGateDown <= 1.0f);
        state->lastGateUp = gateUp[i];
        state->lastGateDown = gateDown[i];

        // --- Envelope processing for TUp-Out and TDown-Out ---
        int envShape = alg->v[kParamEnvShape];
        float envExponent = alg->v[kParamEnvExponent] / 1.0f; // allow 1.00 to 8.00

        float envUp = processEnvelope(state->tUpEnv, gateUp[i] > 1.0f, attack, decay, sustain, release, SAMPLE_RATE, envShape, envExponent);
        float envDown = processEnvelope(state->tDownEnv, gateDown[i] > 1.0f, attack, decay, sustain, release, SAMPLE_RATE, envShape, envExponent);

        tUpOut[i] = envUp * 5.0f;
        tDownOut[i] = envDown * 5.0f;

        // --- Gate pulse logic (unchanged) ---
        if (trigUp) state->tUpPulse = gateLenSamples;
        if (trigDown) state->tDownPulse = gateLenSamples;

        gateUPOut[i] = (state->tUpPulse > 0) ? 5.0f : 0.0f;
        gateDNOut[i] = (state->tDownPulse > 0) ? 5.0f : 0.0f;

        if (state->tUpPulse > 0) state->tUpPulse--;
        if (state->tDownPulse > 0) state->tDownPulse--;

        // --- Start sequence on Trig Up (forward) ---
        if (trigUp) {
            state->stepIndex = 0;
            state->stepInc = 1;
            state->msCounter = 0;
        }
        // --- Start sequence on Trig Down (backward) ---
        if (trigDown) {
            state->stepIndex = length - 1;
            state->stepInc = -1;
            state->msCounter = 0;
        }

        // --- Sequence running ---
        if (state->stepIndex >= 0 && state->stepIndex < length) {
            if (state->msCounter <= 0) {
                // Output new note
                state->currentPitch = (intervals[state->stepIndex] + transpose) / 12.0f;
                state->msCounter = spacingMs * (SAMPLE_RATE / 1000);
                state->stepIndex += state->stepInc;
            } else {
                state->msCounter--;
            }
            outPitch[i] = state->currentPitch; // Always output current pitch while running
        } else {
            outPitch[i] = 0.0f; // Not running: output 0V
            // Do NOT reset state variables here, so retriggering works
        }
    }
}

// --- Forward declarations for factory ---
void calculateRequirements(_NT_algorithmRequirements& req, const int32_t*);
_NT_algorithm* construct(const _NT_algorithmMemoryPtrs& ptrs, const _NT_algorithmRequirements&, const int32_t*);

// --- Factory definition ---
static const _NT_factory factory = {
    .guid = NT_MULTICHAR('S','T','R','M'),
    .name = "Strummer",
    .description = "6notes strummer",
    .numSpecifications = 0,
    .calculateRequirements = calculateRequirements,
    .construct = construct,
    .parameterChanged = NULL,
    .step = step,
    .draw = NULL,
    .midiMessage = NULL,
};

// --- Plugin entry point ---
uintptr_t pluginEntry(_NT_selector selector, uint32_t data) {
    switch (selector) {
        case kNT_selector_version: return kNT_apiVersionCurrent;
        case kNT_selector_numFactories: return 1;
        case kNT_selector_factoryInfo: return (uintptr_t)((data == 0) ? &factory : NULL);
    }
    return 0;
}

// --- Algorithm requirements calculation ---
void calculateRequirements(_NT_algorithmRequirements& req, const int32_t*) {
    req.numParameters = kNumParams;
    req.sram = sizeof(_strumAlgorithm);
    req.dram = sizeof(StrumState);
    req.dtc = 0;
    req.itc = 0;
}

// --- Algorithm construction ---
_NT_algorithm* construct(const _NT_algorithmMemoryPtrs& ptrs, const _NT_algorithmRequirements&, const int32_t*) {
    _strumAlgorithm* alg = reinterpret_cast<_strumAlgorithm*>(ptrs.sram);
    alg->state = reinterpret_cast<StrumState*>(ptrs.dram);
    *alg->state = StrumState{}; // Zero-initialize state
    alg->parameters = parameters;
    alg->parameterPages = NULL;
    return alg;
}