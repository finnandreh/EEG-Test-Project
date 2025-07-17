#include <Arduino.h>
#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846
#endif

// Forward declarations
float goertzelPower(float targetFreq);
float EEGFilter(float input);

// BioAmp EEG Analyzer: Alpha, Theta, Delta, Beta Bands + Accurate Real-Time States
// ---------------------------------------------------------------------------------

#define FS           256
#define WINDOW       FS
#define BAUD         115200
#define INPUT_PIN    A0

float buf[WINDOW];
int   bufIdx = 0;
float sumSq  = 0.0;

int    spikeCount = 0;
bool   wasAbove   = false;
const float THRESH = 50.0;

// Alpha (10 Hz)
const float ALPHA_TARGET = 10.0;
const float K_alpha      = 0.5 + (WINDOW * ALPHA_TARGET) / FS;
const float w0_alpha     = (2.0 * PI * K_alpha) / WINDOW;
const float coeff_alpha  = 2.0 * cos(w0_alpha);
float Q1_alpha = 0, Q2_alpha = 0;

unsigned long sampleCount = 0;
unsigned long secCount    = 0;
unsigned long lastMicros  = 0;

void setup() {
  Serial.begin(BAUD);
  while (!Serial);
  for (int i = 0; i < WINDOW; i++) buf[i] = 0.0;
}

void loop() {
  unsigned long now = micros();
  if (now - lastMicros < 1000000UL / FS) return;
  lastMicros = now;

  float raw = analogRead(INPUT_PIN);
  float x   = EEGFilter(raw);

  sumSq -= buf[bufIdx]*buf[bufIdx];
  buf[bufIdx] = x;
  sumSq += x*x;

  bool above = x > THRESH;
  if (above && !wasAbove) spikeCount++;
  wasAbove = above;

  float Q0_alpha = coeff_alpha * Q1_alpha - Q2_alpha + x;
  Q2_alpha = Q1_alpha; Q1_alpha = Q0_alpha;

  bufIdx = (bufIdx + 1) % WINDOW;
  sampleCount++;

  if (sampleCount >= WINDOW) {
    secCount++;
    float rms = sqrt(sumSq / WINDOW);

    // Alpha Power
    float real_alpha = Q1_alpha - Q2_alpha * cos(w0_alpha);
    float imag_alpha = Q2_alpha * sin(w0_alpha);
    float alphaPower = real_alpha*real_alpha + imag_alpha*imag_alpha;
    float attention  = alphaPower / (alphaPower + rms*rms + 1e-6);

    // Theta Band
    float thetaFreqs[] = {4.0, 5.0, 6.0, 7.0, 8.0};
    float maxThetaPower = 0.0, dominantThetaFreq = 0.0;
    for (int i = 0; i < 5; i++) {
      float power = goertzelPower(thetaFreqs[i]);
      if (power > maxThetaPower) {
        maxThetaPower = power;
        dominantThetaFreq = thetaFreqs[i];
      }
    }
    float thetaPower = maxThetaPower;
    float thetaIndex = thetaPower / (thetaPower + rms*rms + 1e-6);

    // Delta Band
    float deltaFreqs[] = {0.5, 1.0, 2.0, 3.0, 4.0};
    float maxDeltaPower = 0.0, dominantDeltaFreq = 0.0;
    for (int i = 0; i < 5; i++) {
      float power = goertzelPower(deltaFreqs[i]);
      if (power > maxDeltaPower) {
        maxDeltaPower = power;
        dominantDeltaFreq = deltaFreqs[i];
      }
    }
    float deltaPower = maxDeltaPower;
    float deltaIndex = deltaPower / (deltaPower + rms*rms + 1e-6);

    // Beta Band
    float betaFreqs[] = {13.0, 15.0, 20.0, 25.0, 30.0};
    float maxBetaPower = 0.0, dominantBetaFreq = 0.0;
    for (int i = 0; i < 5; i++) {
      float power = goertzelPower(betaFreqs[i]);
      if (power > maxBetaPower) {
        maxBetaPower = power;
        dominantBetaFreq = betaFreqs[i];
      }
    }
    float betaPower = maxBetaPower;
    float betaIndex = betaPower / (betaPower + rms*rms + 1e-6);

    // Updated Classification Logic
    String state = "Unknown";
    if (rms > 50 || spikeCount > 3) {
      state = "Moving";
    } else if (betaPower > thetaPower && betaPower > deltaPower && attention > 0.95) {
      state = "Highly Focused";
    } else if (betaPower > thetaPower && betaPower > deltaPower && attention > 0.90) {
      state = "Fully Focused";
    } else if (attention > 0.85 && betaPower > thetaPower && betaPower > deltaPower) {
      state = "Focused";
    } else if (thetaIndex > 0.98 && attention > 0.95 && rms < 5) {
      state = "Fully Relaxed";
    } else if (thetaIndex > 0.95 && attention > 0.90) {
      state = "Relaxed";
    } else if (deltaIndex > 0.98 && rms < 3) {
      state = "Deep Sleep / Meditation";
    } else if (thetaIndex > 0.90 || attention > 0.85) {
      state = "Semi-Relaxed";
    }

    // Serial Output
    Serial.print(F("second:"));            Serial.print(secCount);            Serial.print(F(", "));
    Serial.print(F("RMS:"));               Serial.print(rms);                 Serial.print(F(", "));
    Serial.print(F("spikeCount:"));        Serial.print(spikeCount);          Serial.print(F(", "));
    Serial.print(F("alphaPower:"));        Serial.print(alphaPower);          Serial.print(F(", "));
    Serial.print(F("attention:"));         Serial.print(attention);           Serial.print(F(", "));
    Serial.print(F("thetaPower:"));        Serial.print(thetaPower);          Serial.print(F(", "));
    Serial.print(F("thetaIndex:"));        Serial.print(thetaIndex);          Serial.print(F(", "));
    Serial.print(F("dominantThetaFreq:")); Serial.print(dominantThetaFreq);   Serial.print(F(", "));
    Serial.print(F("deltaPower:"));        Serial.print(deltaPower);          Serial.print(F(", "));
    Serial.print(F("deltaIndex:"));        Serial.print(deltaIndex);          Serial.print(F(", "));
    Serial.print(F("dominantDeltaFreq:")); Serial.print(dominantDeltaFreq);   Serial.print(F(", "));
    Serial.print(F("betaPower:"));         Serial.print(betaPower);           Serial.print(F(", "));
    Serial.print(F("betaIndex:"));         Serial.print(betaIndex);           Serial.print(F(", "));
    Serial.print(F("dominantBetaFreq:"));  Serial.print(dominantBetaFreq);    Serial.print(F(", "));
    Serial.print(F("state:"));             Serial.println(state);

    // Reset
    sampleCount = 0;
    spikeCount  = 0;
    Q1_alpha = Q2_alpha = 0;
  }
}

float goertzelPower(float targetFreq) {
  float k = 0.5 + (WINDOW * targetFreq) / FS;
  float w0 = (2.0 * PI * k) / WINDOW;
  float coeff = 2.0 * cos(w0);
  float Q1 = 0, Q2 = 0;
  for (int i = 0; i < WINDOW; i++) {
    float x = buf[i];
    float Q0 = coeff * Q1 - Q2 + x;
    Q2 = Q1; Q1 = Q0;
  }
  float real = Q1 - Q2 * cos(w0);
  float imag = Q2 * sin(w0);
  return real*real + imag*imag;
}

// Bandpass Filter (0.5–29.5 Hz)
float EEGFilter(float input) {
  float y = input;
  {
    static float z1, z2;
    float in = y - -0.95391350*z1 - 0.25311356*z2;
    y = 0.00735282*in + 0.01470564*z1 + 0.00735282*z2;
    z2 = z1; z1 = in;
  }
  {
    static float z1, z2;
    float in = y - -1.20596630*z1 - 0.60558332*z2;
    y = 1.00000000*in + 2.00000000*z1 + 1.00000000*z2;
    z2 = z1; z1 = in;
  }
  {
    static float z1, z2;
    float in = y - -1.97690645*z1 - 0.97706395*z2;
    y = 1.00000000*in + -2.00000000*z1 + 1.00000000*z2;
    z2 = z1; z1 = in;
  }
  {
    static float z1, z2;
    float in = y - -1.99071687*z1 - 0.99086813*z2;
    y = 1.00000000*in + -2.00000000*z1 + 1.00000000*z2;
    z2 = z1; z1 = in;
  }
  return y;
}
