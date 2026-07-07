#include "ExpressionAxis.h"

namespace conduit::grid
{

namespace
{
    bool isValidSlot (int voiceIndex) noexcept
    {
        return voiceIndex >= 0 && voiceIndex < VoiceAllocator::kMaxVoices;
    }
}

ExpressionAxis::ExpressionAxis() noexcept : ExpressionAxis (Config{})
{
}

ExpressionAxis::ExpressionAxis (const Config& cfg) noexcept : config (cfg)
{
}

void ExpressionAxis::setRaw (int voiceIndex, float rawValue) noexcept
{
    if (isValidSlot (voiceIndex))
        raw[(size_t) voiceIndex] = rawValue;
}

void ExpressionAxis::setOffset (float bipolarOffset) noexcept
{
    axisOffset = juce::jlimit (-config.offsetScale, config.offsetScale, bipolarOffset);
}

float ExpressionAxis::offset() const noexcept
{
    return axisOffset;
}

void ExpressionAxis::activate (int voiceIndex) noexcept
{
    if (! isValidSlot (voiceIndex))
        return;

    active[(size_t) voiceIndex] = true;
    raw[(size_t) voiceIndex]    = 0.0f;
}

void ExpressionAxis::deactivate (int voiceIndex) noexcept
{
    if (isValidSlot (voiceIndex))
        active[(size_t) voiceIndex] = false;
}

bool ExpressionAxis::isActive (int voiceIndex) const noexcept
{
    return isValidSlot (voiceIndex) && active[(size_t) voiceIndex];
}

float ExpressionAxis::rawValue (int voiceIndex) const noexcept
{
    if (! isValidSlot (voiceIndex))
        return 0.0f;

    return raw[(size_t) voiceIndex];
}

void ExpressionAxis::setOffsetBeyondMax (bool shouldAllow) noexcept
{
    offsetBeyondMaxFlag = shouldAllow;
}

bool ExpressionAxis::offsetBeyondMax() const noexcept
{
    return offsetBeyondMaxFlag;
}

float ExpressionAxis::combined (int voiceIndex) const noexcept
{
    if (! isValidSlot (voiceIndex))
        return config.outMin;

    // Geklemmt auf die vom Nutzer gesetzten Kurven-Ausgangsgrenzen (nicht
    // die Achsen-Kapazität config.outMin/outMax) -- ein niedrig gezogener
    // Kurven-Max darf nicht durch Weiterwischen/Offset überschritten werden.
    // Invertierte Kurven (Min > Max) sind erlaubt, daher sortiert klemmen.
    const auto curveLo = juce::jmin (curve.getOutputMin(), curve.getOutputMax());
    const auto curveHi = juce::jmax (curve.getOutputMin(), curve.getOutputMax());
    const auto shaped  = curve.apply (raw[(size_t) voiceIndex]);

    if (! offsetBeyondMaxFlag)
        return juce::jlimit (curveLo, curveHi, shaped + axisOffset);   // Default: hart an der Kurven-Grenze

    // "Offset über Max erlauben": die Kurve klemmt zuerst auf ihre eigene
    // Grenze, DANACH schiebt der Offset noch darüber hinaus -- begrenzt nur
    // durch die Achsen-Kapazität (Config).
    const auto capLo = juce::jmin (config.outMin, config.outMax);
    const auto capHi = juce::jmax (config.outMin, config.outMax);

    return juce::jlimit (capLo, capHi, juce::jlimit (curveLo, curveHi, shaped) + axisOffset);
}

void ExpressionAxis::reset() noexcept
{
    raw.fill (0.0f);
    active.fill (false);
}

} // namespace conduit::grid
