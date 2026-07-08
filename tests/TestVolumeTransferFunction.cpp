#include "qtviewerpro/render/VolumeTransferFunction.h"

#include <QtTest/QtTest>

class TestVolumeTransferFunction : public QObject
{
  Q_OBJECT

private slots:
  void clampsManualIntensityRangeToSourceBounds();
  void manualIntensityRangeSwitchesToCustomMode();
  void namedPresetSelectionPreservesManualValues();
};

void TestVolumeTransferFunction::clampsManualIntensityRangeToSourceBounds()
{
  const auto clamped = qvp::clampIntensityRange(500.0F, -1500.0F, -1024.0F, 3071.0F);

  QCOMPARE(clamped.first, -1024.0F);
  QCOMPARE(clamped.second, 500.0F);
}

void TestVolumeTransferFunction::manualIntensityRangeSwitchesToCustomMode()
{
  qvp::VolumeTransferFunctionState state;
  state = qvp::withGlobalOpacity(state, 1.25F);
  state = qvp::withManualIntensityRange(state, 500.0F, -1500.0F, -1024.0F, 3071.0F);

  QCOMPARE(state.renderPreset, qvp::VolumeRenderPreset::Custom);
  QCOMPARE(state.globalOpacity, 1.0F);
  QCOMPARE(state.intensityMinimum, -1024.0F);
  QCOMPARE(state.intensityMaximum, 500.0F);
}

void TestVolumeTransferFunction::namedPresetSelectionPreservesManualValues()
{
  qvp::VolumeTransferFunctionState state;
  state = qvp::withGlobalOpacity(state, 0.35F);
  state = qvp::withManualIntensityRange(state, -200.0F, 600.0F, -1024.0F, 3071.0F);

  const auto presetState = qvp::withRenderPreset(state, qvp::VolumeRenderPreset::CtLung);

  QCOMPARE(presetState.renderPreset, qvp::VolumeRenderPreset::CtLung);
  QCOMPARE(presetState.globalOpacity, 0.35F);
  QCOMPARE(presetState.intensityMinimum, -200.0F);
  QCOMPARE(presetState.intensityMaximum, 600.0F);
}

QTEST_MAIN(TestVolumeTransferFunction)

#include "TestVolumeTransferFunction.moc"
