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

float ExpressionAxis::combined (int voiceIndex) const noexcept
{
    if (! isValidSlot (voiceIndex))
        return config.outMin;

    // Geklemmt auf die vom Nutzer gesetzten Kurven-Ausgangsgrenzen (nicht
    // die Achsen-Kapazität config.outMin/outMax) -- ein niedrig gezogener
    // Kurven-Max darf nicht durch Weiterwischen/Offset überschritten werden.
    // Invertierte Kurven (Min > Max) sind erlaubt, daher sortiert klemmen.
    const auto lo = juce::jmin (curve.getOutputMin(), curve.getOutputMax());
    const auto hi = juce::jmax (curve.getOutputMin(), curve.getOutputMax());

    return juce::jlimit (lo, hi, curve.apply (raw[(size_t) voiceIndex]) + axisOffset);
}

void ExpressionAxis::reset() noexcept
{
    raw.fill (0.0f);
    active.fill (false);
}

} // namespace conduit::grid
