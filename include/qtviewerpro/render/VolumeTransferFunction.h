#pragma once

#include <algorithm>
#include <utility>

namespace qvp
{

enum class VolumeRenderPreset
{
  Default = 0,
  CtBone = 1,
  CtLung = 2,
  Custom = 3
};

struct VolumeTransferFunctionState
{
  VolumeRenderPreset renderPreset = VolumeRenderPreset::Default;
  float globalOpacity = 1.0F;
  float intensityMinimum = 0.0F;
  float intensityMaximum = 1.0F;
};

inline float clampGlobalOpacity(float opacity)
{
  return std::clamp(opacity, 0.0F, 1.0F);
}

inline std::pair<float, float> clampIntensityRange(float requestedMinimum,
                                                   float requestedMaximum,
                                                   float sourceMinimum,
                                                   float sourceMaximum)
{
  if (sourceMinimum > sourceMaximum)
  {
    std::swap(sourceMinimum, sourceMaximum);
  }

  const auto [requestedLow, requestedHigh] = std::minmax(requestedMinimum, requestedMaximum);
  const float clampedMinimum = std::clamp(requestedLow, sourceMinimum, sourceMaximum);
  const float clampedMaximum = std::clamp(requestedHigh, sourceMinimum, sourceMaximum);
  return {clampedMinimum, clampedMaximum};
}

inline VolumeTransferFunctionState withRenderPreset(VolumeTransferFunctionState state,
                                                    VolumeRenderPreset preset)
{
  state.renderPreset = preset;
  return state;
}

inline VolumeTransferFunctionState withGlobalOpacity(VolumeTransferFunctionState state,
                                                     float opacity)
{
  state.globalOpacity = clampGlobalOpacity(opacity);
  return state;
}

inline VolumeTransferFunctionState withManualIntensityRange(VolumeTransferFunctionState state,
                                                            float requestedMinimum,
                                                            float requestedMaximum,
                                                            float sourceMinimum,
                                                            float sourceMaximum)
{
  const auto [clampedMinimum, clampedMaximum] =
      clampIntensityRange(requestedMinimum, requestedMaximum, sourceMinimum, sourceMaximum);
  state.intensityMinimum = clampedMinimum;
  state.intensityMaximum = clampedMaximum;
  state.renderPreset = VolumeRenderPreset::Custom;
  return state;
}

inline VolumeTransferFunctionState clampToSourceRange(VolumeTransferFunctionState state,
                                                      float sourceMinimum,
                                                      float sourceMaximum)
{
  const auto [clampedMinimum, clampedMaximum] =
      clampIntensityRange(state.intensityMinimum, state.intensityMaximum, sourceMinimum, sourceMaximum);
  state.intensityMinimum = clampedMinimum;
  state.intensityMaximum = clampedMaximum;
  state.globalOpacity = clampGlobalOpacity(state.globalOpacity);
  return state;
}

} // namespace qvp
