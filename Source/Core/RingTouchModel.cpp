#include "RingTouchModel.h"

namespace conduit::grid
{

RingTouchModel::RingTouchModel() noexcept : RingTouchModel (Config{})
{
}

RingTouchModel::RingTouchModel (const Config& cfg) noexcept : config (cfg)
{
}

RingTouchModel::DownResult RingTouchModel::onDown (uint32_t fingerId, juce::Point<float> pos) noexcept
{
    PrimaryFinger* nearest = nullptr;
    float nearestDist = 0.0f;

    for (auto& primary : primaries)
    {
        if (primary.ringFinger != 0)
            continue;

        const auto dist = pos.getDistanceFrom (primary.center);
        if (dist <= config.grabRadiusPx && (nearest == nullptr || dist < nearestDist))
        {
            nearest = &primary;
            nearestDist = dist;
        }
    }

    if (nearest != nullptr)
    {
        nearest->ringFinger = fingerId;
        nearest->curRadiusPx = nearestDist;
        return { TouchKind::Ring, nearest->id };
    }

    primaries.push_back ({ fingerId, pos, 0, config.minRadiusPx });
    return { TouchKind::Primary, 0 };
}

RingTouchModel::MoveResult RingTouchModel::onMove (uint32_t fingerId, juce::Point<float> pos) noexcept
{
    for (auto& primary : primaries)
    {
        if (primary.ringFinger != fingerId)
            continue;

        const auto radius = pos.getDistanceFrom (primary.center);
        primary.curRadiusPx = radius;

        const auto range = config.maxRadiusPx - config.minRadiusPx;
        const auto slide  = range > 0.0f ? (radius - config.minRadiusPx) / range : 0.0f;

        return { true, primary.id, juce::jlimit (0.0f, 1.0f, slide) };
    }

    return {};
}

RingTouchModel::UpResult RingTouchModel::onUp (uint32_t fingerId) noexcept
{
    for (auto& primary : primaries)
    {
        if (primary.ringFinger == fingerId)
        {
            const auto owner = primary.id;
            primary.ringFinger = 0;
            return { false, true, 0, owner };
        }
    }

    for (std::size_t i = 0; i < primaries.size(); ++i)
    {
        if (primaries[i].id == fingerId)
        {
            primaries.erase (primaries.begin() + (std::ptrdiff_t) i);
            return { true, false, fingerId, 0 };
        }
    }

    return {};
}

std::vector<RingTouchModel::Circle> RingTouchModel::activeCircles() const
{
    std::vector<Circle> circles;
    circles.reserve (primaries.size());

    for (const auto& primary : primaries)
        circles.push_back ({ primary.center, primary.curRadiusPx });

    return circles;
}

void RingTouchModel::reset() noexcept
{
    primaries.clear();
}

} // namespace conduit::grid
