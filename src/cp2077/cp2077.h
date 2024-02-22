#ifndef SRC_CP2077_CP2077_H_
#define SRC_CP2077_CP2077_H_

// Must be 32bit aligned
// Should be 4x32
struct ShaderInjectData {
  float toneMapperType;
  float toneMapperPeakNits;
  float toneMapperPaperWhite;
  float toneMapperColorSpace;
  float toneMapperWhitePoint;
  float toneMapperExposure;
  float toneMapperHighlights;
  float toneMapperShadows;
  float toneMapperContrast;
  float toneMapperDechroma;
  float colorGradingWorkflow;
  float colorGradingStrength;
  float colorGradingScaling;
  float colorGradingSaturation;
  float colorGradingCorrection;
  float effectBloom;
  float effectVignette;
  float effectFilmGrain;
  float debugValue00;
  float debugValue01;
  float debugValue02;
  float debugValue03;
};

#define TONE_MAPPER_TYPE__NONE    0.f
#define TONE_MAPPER_TYPE__VANILLA 1.f
#define TONE_MAPPER_TYPE__ACES    2.f
#define TONE_MAPPER_TYPE__OPENDRT 3.f

#define OUTPUT_TYPE_SRGB8  0u
#define OUTPUT_TYPE_PQ     1u
#define OUTPUT_TYPE_SCRGB  2u
#define OUTPUT_TYPE_SRGB10 3u

#endif  // SRC_CP2077_CP2077_H_
