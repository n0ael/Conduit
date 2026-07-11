#include "FigmaSnap.h"

#include <cmath>

namespace conduit::grid
{

std::vector<float> FigmaSnap::centreCandidates (const std::vector<float>& otherCentres)
{
    std::vector<float> candidates;
    candidates.reserve (otherCentres.size() * 4);

    // Achs-Flucht: das Zentrum jedes anderen Elements.
    for (const auto centre : otherCentres)
        candidates.push_back (centre);

    // Gleicher Abstand (Masterplan: "bei dreien wird der gleiche Abstand
    // angeboten"): pro Paar der Mittelpunkt zwischen beiden und die
    // beidseitigen Verlaengerungen der Zweierreihe.
    for (size_t i = 0; i < otherCentres.size(); ++i)
    {
        for (size_t j = i + 1; j < otherCentres.size(); ++j)
        {
            const auto a = otherCentres[i];
            const auto b = otherCentres[j];

            candidates.push_back ((a + b) * 0.5f);
            candidates.push_back (2.0f * b - a);
            candidates.push_back (2.0f * a - b);
        }
    }

    return candidates;
}

FigmaSnap::Result FigmaSnap::snap (juce::Rectangle<float> moving,
                                   const std::vector<juce::Rectangle<float>>& others,
                                   float thresholdX, float thresholdY) noexcept
{
    Result result;
    result.x = moving.getX();
    result.y = moving.getY();

    std::vector<float> centresX, centresY;
    centresX.reserve (others.size());
    centresY.reserve (others.size());

    for (const auto& other : others)
    {
        centresX.push_back (other.getCentreX());
        centresY.push_back (other.getCentreY());
    }

    const auto snapAxis = [] (float movingCentre, const std::vector<float>& otherCentres,
                              float threshold, bool& snapped, float& guide) -> float
    {
        auto bestDistance = threshold;
        auto bestCentre   = movingCentre;

        for (const auto candidate : FigmaSnap::centreCandidates (otherCentres))
        {
            const auto distance = std::abs (candidate - movingCentre);
            if (distance <= bestDistance)
            {
                bestDistance = distance;
                bestCentre   = candidate;
                snapped      = true;
                guide        = candidate;
            }
        }

        return bestCentre;
    };

    const auto snappedCentreX = snapAxis (moving.getCentreX(), centresX, thresholdX,
                                          result.snappedX, result.guideX);
    const auto snappedCentreY = snapAxis (moving.getCentreY(), centresY, thresholdY,
                                          result.snappedY, result.guideY);

    if (result.snappedX)
        result.x = snappedCentreX - moving.getWidth() * 0.5f;
    if (result.snappedY)
        result.y = snappedCentreY - moving.getHeight() * 0.5f;

    return result;
}

} // namespace conduit::grid
