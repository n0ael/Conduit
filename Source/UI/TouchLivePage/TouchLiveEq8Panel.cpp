#include "TouchLiveEq8Panel.h"

#include <cmath>
#include <complex>

#include "UI/PushLookAndFeel.h"

namespace conduit
{

namespace
{
    constexpr double minHz = 10.0, maxHz = 22000.0;

    // Lives Anzeige-Farben (aus den Kalibrier-Screenshots gemessen)
    const juce::Colour curveColour  { 0xff03cfde };   // Lives Kurven-Cyan
    const juce::Colour handleColour { 0xfff0a53c };   // Lives Handle-Orange
    const juce::Colour handleOff    { 0xff6a6f74 };   // Band aus

    // Butterworth-8-Stufen-Qs (48-dB-Cuts, §10j)
    constexpr double butter8[4] = { 0.5098, 0.6013, 0.8999, 2.5629 };

    // Wire-Norm ↔ Hz / Q — gegen Lives Anzeige verifiziert (§10i)
    [[nodiscard]] double normToHz (double norm)
    {
        return minHz * std::pow (maxHz / minHz, juce::jlimit (0.0, 1.0, norm));
    }

    [[nodiscard]] double normToQ (double norm)
    {
        return 0.1 * std::pow (180.0, juce::jlimit (0.0, 1.0, norm));
    }

    [[nodiscard]] juce::String hzText (double hz)
    {
        return hz >= 1000.0 ? juce::String (hz / 1000.0, 2) + " kHz"
                            : juce::String (hz, 0) + " Hz";
    }

    [[nodiscard]] juce::String stringField (const juce::var& object, const char* field)
    {
        if (auto* dyn = object.getDynamicObject())
            return dyn->getProperty (field).toString();

        return {};
    }
}

//==============================================================================
TouchLiveEq8Panel::TouchLiveEq8Panel (TouchLiveClient& clientToUse)
    : client (clientToUse)
{
    qSlider.setRange (0.0, 1.0, 0.0);

    qSlider.onValueChange = [this]
    {
        auto& band = bands[(size_t) selectedBand];

        if (band.isMapped())
            setResonanceNorm (band, qSlider.getValue());
    };

    addAndMakeVisible (qSlider);

    typePrevTile.onClick = [this] { stepFilterType (-1); };
    typeNextTile.onClick = [this] { stepFilterType (1); };
    addAndMakeVisible (typePrevTile);
    addAndMakeVisible (typeNextTile);

    bandOnTile.setTooltip (juce::String::fromUTF8 ("Band an/aus"));
    bandOnTile.onClick = [this] { toggleBandOn (selectedBand); };
    addAndMakeVisible (bandOnTile);
}

//==============================================================================
void TouchLiveEq8Panel::setDevice (const juce::String& deviceKeyToUse,
                                   const juce::var& parmeta)
{
    deviceKey = deviceKeyToUse;
    bands = {};
    mappedBandCount = 0;
    adaptiveQIndex = -1;

    if (const auto* meta = parmeta.getArray())
    {
        for (int index = 0; index < meta->size(); ++index)
        {
            const auto& entry = meta->getReference (index);
            const auto name = stringField (entry, "name");

            if (name == "Adaptive Q")
            {
                adaptiveQIndex = index;
                continue;
            }

            for (int bandIndex = 0; bandIndex < bandCount; ++bandIndex)
            {
                const auto prefix = juce::String (bandIndex + 1) + " ";
                auto& band = bands[(size_t) bandIndex];

                if (name == prefix + "Filter On A")
                    band.onIndex = index;
                else if (name == prefix + "Frequency A")
                    band.frequencyIndex = index;
                // Live 12 nennt den Q-Parameter "{n} Q A" (Feldtest
                // 10.07.2026) — "Resonance A" als Alias für ältere Versionen
                else if (name == prefix + "Q A" || name == prefix + "Resonance A")
                    band.resonanceIndex = index;
                else if (name == prefix + "Gain A")
                {
                    band.gainIndex = index;

                    if (auto* object = entry.getDynamicObject())
                    {
                        band.gainMin = (double) object->getProperty ("min");
                        band.gainMax = (double) object->getProperty ("max");

                        if (band.gainMax <= band.gainMin)
                        {
                            band.gainMin = -15.0;
                            band.gainMax = 15.0;
                        }
                    }
                }
                else if (name == prefix + "Filter Type A")
                {
                    band.typeIndex = index;
                    band.typeItems.clear();

                    if (auto* object = entry.getDynamicObject())
                        if (const auto* items = object->getProperty ("items").getArray())
                            for (const auto& item : *items)
                                band.typeItems.add (item.toString());
                }
            }
        }
    }

    for (const auto& band : bands)
        if (band.isMapped())
            ++mappedBandCount;

    if (! bands[(size_t) selectedBand].isMapped())
        for (int bandIndex = 0; bandIndex < bandCount; ++bandIndex)
            if (bands[(size_t) bandIndex].isMapped())
            {
                selectedBand = bandIndex;
                break;
            }

    dragActive = false;
    pinchActive = false;
    primaryTouchIndex = secondaryTouchIndex = -1;
    curveDirty = true;
    updateFooterFromSelection();
    repaint();
}

void TouchLiveEq8Panel::setValues (const juce::var& parvals)
{
    const auto* values = parvals.getArray();

    if (values == nullptr)
        return;

    const auto valueAt = [values] (int index, double fallback)
    {
        return index >= 0 && index < values->size()
                   ? (double) values->getReference (index) : fallback;
    };

    adaptiveQ = valueAt (adaptiveQIndex, adaptiveQ ? 1.0 : 0.0) > 0.5;

    for (int bandIndex = 0; bandIndex < bandCount; ++bandIndex)
    {
        auto& band = bands[(size_t) bandIndex];

        if (! band.isMapped())
            continue;

        // Lokal-optimistisch (§5.1): das gezogene Band folgt NUR dem Finger
        const auto touched = (dragActive || pinchActive)
                                 && bandIndex == selectedBand;

        band.on = valueAt (band.onIndex, band.on ? 1.0 : 0.0) > 0.5;
        band.typeValue = juce::jlimit (0, juce::jmax (0, band.typeItems.size() - 1),
                                       (int) std::lround (valueAt (band.typeIndex,
                                                                   band.typeValue)));

        if (! touched)
        {
            band.frequencyNorm = juce::jlimit (0.0, 1.0,
                                               valueAt (band.frequencyIndex,
                                                        band.frequencyNorm));
            band.gainDb = juce::jlimit (band.gainMin, band.gainMax,
                                        valueAt (band.gainIndex, band.gainDb));
            band.resonanceNorm = juce::jlimit (0.0, 1.0,
                                               valueAt (band.resonanceIndex,
                                                        band.resonanceNorm));
        }
    }

    curveDirty = true;
    updateFooterFromSelection();
    repaint();
}

//==============================================================================
void TouchLiveEq8Panel::sendParameter (int parameterIndex, float value, bool continuous)
{
    if (deviceKey.isEmpty() || parameterIndex < 0)
        return;

    client.noteTouchedParameter (TouchLiveClient::makeParameterKey (
        "devices", "parvals:" + deviceKey));

    juce::OSCMessage message { juce::OSCAddressPattern ("/live/device/set/parameter") };
    message.addString (deviceKey);
    message.addInt32 (parameterIndex);
    message.addFloat32 (value);

    if (continuous)
        client.sendTouchValue (message);
    else
        client.sendCommand (message);
}

void TouchLiveEq8Panel::setResonanceNorm (Band& band, double newNorm)
{
    band.resonanceNorm = juce::jlimit (0.0, 1.0, newNorm);
    sendParameter (band.resonanceIndex, (float) band.resonanceNorm, true);
    qSlider.setValue (band.resonanceNorm, juce::dontSendNotification);
    curveDirty = true;
    repaint();
}

//==============================================================================
int TouchLiveEq8Panel::bandAt (juce::Point<float> position) const
{
    int best = -1;
    auto bestDistance = touchRadius;

    for (int bandIndex = 0; bandIndex < bandCount; ++bandIndex)
    {
        if (! bands[(size_t) bandIndex].isMapped())
            continue;

        const auto distance = bandPosition (bandIndex).getDistanceFrom (position);

        if (distance < bestDistance)
        {
            bestDistance = distance;
            best = bandIndex;
        }
    }

    return best;
}

void TouchLiveEq8Panel::selectBand (int band)
{
    if (band < 0 || band >= bandCount || ! bands[(size_t) band].isMapped())
        return;

    selectedBand = band;
    updateFooterFromSelection();
    repaint();
}

void TouchLiveEq8Panel::beginDrag (juce::Point<float> position)
{
    const auto band = bandAt (position);

    if (band < 0)
        return;

    selectBand (band);
    dragActive = true;
}

void TouchLiveEq8Panel::dragTo (juce::Point<float> position)
{
    if (! dragActive || pinchActive)   // Pinch friert Freq/Gain ein
        return;

    auto& band = bands[(size_t) selectedBand];

    if (! band.isMapped())
        return;

    band.frequencyNorm = normForX (position.x);
    sendParameter (band.frequencyIndex, (float) band.frequencyNorm, true);

    if (shapeHasGain (shapeOf (band)))
    {
        band.gainDb = juce::jlimit (band.gainMin, band.gainMax, dbForY (position.y));
        sendParameter (band.gainIndex, (float) band.gainDb, true);
    }

    curveDirty = true;
    repaint();
}

void TouchLiveEq8Panel::endDrag()
{
    dragActive = false;
}

void TouchLiveEq8Panel::beginPinch (float distance)
{
    auto& band = bands[(size_t) selectedBand];

    if (! band.isMapped())
        return;

    pinchActive = true;
    pinchStartDistance = juce::jmax (1.0f, distance);
    pinchStartResonance = band.resonanceNorm;
}

void TouchLiveEq8Panel::pinchTo (float distance)
{
    if (! pinchActive)
        return;

    auto& band = bands[(size_t) selectedBand];

    if (! band.isMapped())
        return;

    // Abstand verdoppeln ≈ +0.25 auf der Q-Norm (Faktor ~3.7 in Q)
    const auto octaves = std::log2 (juce::jmax (1.0f, distance)
                                    / pinchStartDistance);
    setResonanceNorm (band, pinchStartResonance + 0.25 * octaves);
}

void TouchLiveEq8Panel::endPinch()
{
    pinchActive = false;
}

void TouchLiveEq8Panel::toggleBandOn (int band)
{
    if (band < 0 || band >= bandCount || ! bands[(size_t) band].isMapped())
        return;

    auto& state = bands[(size_t) band];
    state.on = ! state.on;
    sendParameter (state.onIndex, state.on ? 1.0f : 0.0f, false);

    curveDirty = true;
    updateFooterFromSelection();
    repaint();
}

void TouchLiveEq8Panel::stepFilterType (int delta)
{
    auto& band = bands[(size_t) selectedBand];

    if (! band.isMapped() || band.typeItems.isEmpty())
        return;

    const auto next = juce::jlimit (0, band.typeItems.size() - 1,
                                    band.typeValue + delta);

    if (next == band.typeValue)
        return;

    band.typeValue = next;
    sendParameter (band.typeIndex, (float) next, false);
    curveDirty = true;
    repaint();
}

//==============================================================================
bool TouchLiveEq8Panel::isBandOn (int band) const
{
    return band >= 0 && band < bandCount && bands[(size_t) band].on;
}

double TouchLiveEq8Panel::getResonanceNorm (int band) const
{
    return band >= 0 && band < bandCount ? bands[(size_t) band].resonanceNorm : 0.0;
}

int TouchLiveEq8Panel::frequencyIndexOf (int band) const
{
    return band >= 0 && band < bandCount ? bands[(size_t) band].frequencyIndex : -1;
}

int TouchLiveEq8Panel::gainIndexOf (int band) const
{
    return band >= 0 && band < bandCount ? bands[(size_t) band].gainIndex : -1;
}

int TouchLiveEq8Panel::resonanceIndexOf (int band) const
{
    return band >= 0 && band < bandCount ? bands[(size_t) band].resonanceIndex : -1;
}

juce::Point<float> TouchLiveEq8Panel::bandPosition (int band) const
{
    if (band < 0 || band >= bandCount)
        return {};

    const auto& state = bands[(size_t) band];
    const auto gainForY = shapeHasGain (shapeOf (state)) ? state.gainDb : 0.0;
    return { xForNorm (state.frequencyNorm), yForDb (gainForY) };
}

//==============================================================================
TouchLiveEq8Panel::Shape TouchLiveEq8Panel::shapeOf (const Band& band) const
{
    // Semantik aus den items-Strings (nie feste Indizes annehmen)
    const auto label = band.typeValue >= 0 && band.typeValue < band.typeItems.size()
                           ? band.typeItems[band.typeValue]
                                 .toLowerCase().removeCharacters (" -/")
                           : juce::String();

    const auto has48 = label.contains ("48");

    if (label.contains ("lowcut") || label.contains ("highpass"))
        return has48 ? Shape::lowCut48 : Shape::lowCut12;

    if (label.contains ("highcut") || label.contains ("lowpass"))
        return has48 ? Shape::highCut48 : Shape::highCut12;

    if (label.contains ("lowshelf"))
        return Shape::lowShelf;

    if (label.contains ("highshelf"))
        return Shape::highShelf;

    if (label.contains ("notch"))
        return Shape::notch;

    if (label.contains ("bell") || label.contains ("peak"))
        return Shape::bell;

    // Fallback: Live-12-Reihenfolge (LC48 LC12 LS Bell Notch HS HC12 HC48)
    switch (band.typeValue)
    {
        case 0:  return Shape::lowCut48;
        case 1:  return Shape::lowCut12;
        case 2:  return Shape::lowShelf;
        case 4:  return Shape::notch;
        case 5:  return Shape::highShelf;
        case 6:  return Shape::highCut12;
        case 7:  return Shape::highCut48;
        default: return Shape::bell;
    }
}

bool TouchLiveEq8Panel::shapeHasGain (Shape shape) const noexcept
{
    return shape == Shape::bell || shape == Shape::lowShelf
        || shape == Shape::highShelf;
}

double TouchLiveEq8Panel::effectiveQ (Shape shape, double q, double gainDb) const
{
    // Kalibrier-Kampagne 10.07.2026 (§10j): Lives Anzeige-Q relativ zum
    // RBJ-Prototyp. Der Gain-Term ist Lives "Adaptive Q" (alle Messungen
    // mit On); Off lässt nach dieser Modellierung nur den Term entfallen.
    const auto g = adaptiveQ ? std::abs (gainDb) : 0.0;

    switch (shape)
    {
        case Shape::bell:
            return q * 0.5151 * std::pow (10.0, 0.04908 * g);

        case Shape::lowShelf:
        case Shape::highShelf:
        {
            const auto lq = std::log10 (juce::jmax (1.0e-3, q));
            return std::pow (10.0, -0.36661 + 0.45166 * lq
                                       + 0.04382 * g - 0.00685 * lq * g);
        }

        default:
            return q;   // Cuts/Notch: 1:1 (gemessen, kein Adaptive Q)
    }
}

double TouchLiveEq8Panel::responseDbAt (double hz) const
{
    double totalDb = 0.0;

    for (const auto& band : bands)
    {
        if (! band.isMapped() || ! band.on)
            continue;

        const auto shape = shapeOf (band);
        const auto f0 = normToHz (band.frequencyNorm);
        const auto q = juce::jmax (0.025,
                                   effectiveQ (shape, normToQ (band.resonanceNorm),
                                               band.gainDb));
        const auto a = std::pow (10.0, band.gainDb / 40.0);
        const auto sqrtA = std::sqrt (a);

        // Analoge Prototypen (s-Domain) — Lives Anzeige zeigt keine
        // Bilinear-Stauchung, und alle Fits liefen analog (§10j)
        const std::complex<double> s (0.0, hz / f0);
        const auto s2 = s * s;
        std::complex<double> h;

        switch (shape)
        {
            case Shape::bell:
                h = (s2 + s * (a / q) + 1.0) / (s2 + s / (a * q) + 1.0);
                break;

            case Shape::lowShelf:
                h = a * (s2 + s * (sqrtA / q) + a)
                        / (a * s2 + s * (sqrtA / q) + 1.0);
                break;

            case Shape::highShelf:
                h = a * (a * s2 + s * (sqrtA / q) + 1.0)
                        / (s2 + s * (sqrtA / q) + a);
                break;

            case Shape::notch:
                h = (s2 + 1.0) / (s2 + s / q + 1.0);
                break;

            case Shape::lowCut12:
                h = s2 / (s2 + s / q + 1.0);
                break;

            case Shape::highCut12:
                h = 1.0 / (s2 + s / q + 1.0);
                break;

            case Shape::lowCut48:
            case Shape::highCut48:
            default:
            {
                // Butterworth-8-Kaskade, alle Stufen-Qs skaliert (§10j)
                const auto lambda = juce::jmax (
                    0.15, 1.097 + 0.611 * std::log10 (juce::jmax (1.0e-3, q)));
                h = 1.0;

                for (const auto stageQ : butter8)
                {
                    const auto denom = s2 + s / (lambda * stageQ) + 1.0;
                    h *= shape == Shape::lowCut48 ? s2 / denom : 1.0 / denom;
                }
                break;
            }
        }

        totalDb += 20.0 * std::log10 (juce::jmax (1.0e-9, std::abs (h)));
    }

    return totalDb;
}

void TouchLiveEq8Panel::rebuildCurve()
{
    curve.clear();
    const auto area = plotArea();

    // Auflösung an die Pixelbreite gekoppelt (User-Feedback: Spitzen
    // wurden mit fixen 96 Stützstellen eckig)
    const auto points = juce::jlimit (128, 1024, (int) (area.getWidth() / 2.0f));

    for (int i = 0; i < points; ++i)
    {
        const auto norm = (double) i / (points - 1);
        const auto db = juce::jlimit (-plotDbRange * 1.4, plotDbRange * 1.4,
                                      responseDbAt (normToHz (norm)));
        const juce::Point<float> point { xForNorm (norm), yForDb (db) };

        if (i == 0)
            curve.startNewSubPath (point);
        else
            curve.lineTo (point);
    }

    curveDirty = false;
}

//==============================================================================
juce::Rectangle<float> TouchLiveEq8Panel::plotArea() const
{
    return getLocalBounds().toFloat().reduced (8.0f, 4.0f)
                           .withTrimmedBottom ((float) footerHeight);
}

float TouchLiveEq8Panel::xForNorm (double norm) const
{
    const auto area = plotArea();
    return area.getX() + (float) juce::jlimit (0.0, 1.0, norm) * area.getWidth();
}

double TouchLiveEq8Panel::normForX (float x) const
{
    const auto area = plotArea();
    return area.getWidth() <= 0.0f
               ? 0.5 : juce::jlimit (0.0, 1.0, (double) ((x - area.getX())
                                                         / area.getWidth()));
}

float TouchLiveEq8Panel::yForDb (double db) const
{
    const auto area = plotArea();
    return area.getCentreY()
         - (float) (db / plotDbRange) * area.getHeight() * 0.5f;
}

double TouchLiveEq8Panel::dbForY (float y) const
{
    const auto area = plotArea();
    return area.getHeight() <= 0.0f
               ? 0.0 : (double) (area.getCentreY() - y)
                           / (area.getHeight() * 0.5) * plotDbRange;
}

//==============================================================================
void TouchLiveEq8Panel::updateFooterFromSelection()
{
    const auto& band = bands[(size_t) selectedBand];
    const auto usable = band.isMapped();

    qSlider.setEnabled (usable);
    typePrevTile.setEnabled (usable);
    typeNextTile.setEnabled (usable);
    bandOnTile.setEnabled (usable);
    bandOnTile.setActive (usable && band.on);

    if (usable)
        qSlider.setValue (band.resonanceNorm, juce::dontSendNotification);
}

void TouchLiveEq8Panel::resized()
{
    auto footer = getLocalBounds().removeFromBottom (footerHeight).reduced (8, 5);

    typePrevTile.setBounds (footer.removeFromLeft (40));
    footer.removeFromLeft (2);
    footer.removeFromLeft (110);   // Typ-Label (paint)
    footer.removeFromLeft (2);
    typeNextTile.setBounds (footer.removeFromLeft (40));

    bandOnTile.setBounds (footer.removeFromRight (56));
    footer.removeFromRight (6);
    qSlider.setBounds (footer.reduced (10, 4));

    curveDirty = true;
}

//==============================================================================
void TouchLiveEq8Panel::mouseDown (const juce::MouseEvent& event)
{
    const auto touchIndex = event.source.getIndex();

    if (primaryTouchIndex < 0)
    {
        primaryTouchIndex = touchIndex;
        primaryTouchPosition = event.position;
        beginDrag (event.position);
    }
    else if (secondaryTouchIndex < 0 && touchIndex != primaryTouchIndex)
    {
        secondaryTouchIndex = touchIndex;
        secondaryTouchPosition = event.position;
        beginPinch (event.position.getDistanceFrom (primaryTouchPosition));
    }
}

void TouchLiveEq8Panel::mouseDrag (const juce::MouseEvent& event)
{
    const auto touchIndex = event.source.getIndex();

    if (touchIndex == primaryTouchIndex)
    {
        primaryTouchPosition = event.position;

        if (pinchActive)
            pinchTo (primaryTouchPosition.getDistanceFrom (secondaryTouchPosition));
        else
            dragTo (event.position);
    }
    else if (touchIndex == secondaryTouchIndex)
    {
        secondaryTouchPosition = event.position;
        pinchTo (primaryTouchPosition.getDistanceFrom (secondaryTouchPosition));
    }
}

void TouchLiveEq8Panel::mouseUp (const juce::MouseEvent& event)
{
    const auto touchIndex = event.source.getIndex();

    if (touchIndex == secondaryTouchIndex)
    {
        secondaryTouchIndex = -1;
        endPinch();
    }
    else if (touchIndex == primaryTouchIndex)
    {
        primaryTouchIndex = -1;
        secondaryTouchIndex = -1;
        endPinch();
        endDrag();
    }
}

void TouchLiveEq8Panel::mouseDoubleClick (const juce::MouseEvent& event)
{
    toggleBandOn (bandAt (event.position));
}

//==============================================================================
void TouchLiveEq8Panel::paint (juce::Graphics& g)
{
    const auto area = plotArea();

    g.setColour (juce::Colour (0xff0e1112));   // Lives Plot-Hintergrund
    g.fillRoundedRectangle (area, 4.0f);

    if (! isUsable())
    {
        g.setColour (push::colours::textDim);
        g.setFont (push::scaledFont (13.0f));
        g.drawText (juce::String::fromUTF8 (
                        "EQ-Eight-Parameter nicht zuordenbar — BANK nutzen"),
                    area.toNearestInt(), juce::Justification::centred);
        return;
    }

    // Frequenz-Raster wie Live: Zwischenlinien je Dekade dim,
    // Dekaden (100/1k/10k) heller + Label
    const auto logSpan = std::log10 (maxHz / minHz);

    for (double decade = 10.0; decade < maxHz; decade *= 10.0)
    {
        for (int k = 1; k < 10; ++k)
        {
            const auto hz = decade * k;

            if (hz <= minHz || hz >= maxHz)
                continue;

            const auto x = xForNorm (std::log10 (hz / minHz) / logSpan);
            const auto major = k == 1;

            g.setColour (juce::Colours::white.withAlpha (major ? 0.14f : 0.05f));
            g.drawVerticalLine ((int) x, area.getY(), area.getBottom());

            if (major && hz >= 100.0)
            {
                g.setColour (push::colours::textDim.withAlpha (0.8f));
                g.setFont (push::scaledFont (10.0f));
                g.drawText (hz >= 1000.0 ? juce::String (hz / 1000.0, 0) + "k"
                                         : juce::String (hz, 0),
                            (int) x + 4, (int) area.getBottom() - 15, 40, 12,
                            juce::Justification::left);
            }
        }
    }

    // dB-Raster ±12/±6, 0-Linie heller — Zahlen links wie Live
    g.setFont (push::scaledFont (10.0f));

    for (const auto db : { 12.0, 6.0, 0.0, -6.0, -12.0 })
    {
        const auto y = yForDb (db);
        g.setColour (juce::Colours::white.withAlpha (db == 0.0 ? 0.20f : 0.08f));
        g.drawHorizontalLine ((int) y, area.getX(), area.getRight());

        g.setColour (push::colours::textDim.withAlpha (0.8f));
        g.drawText (juce::String ((int) db),
                    (int) area.getX() + 4, (int) y - 13, 30, 12,
                    juce::Justification::left);
    }

    if (curveDirty)
        rebuildCurve();

    g.setColour (curveColour);
    g.strokePath (curve, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Band-Punkte im Ableton-Look: orange Kreise mit Nummer; Auswahl
    // gefüllt (dunkle Zahl), aktive als Ring, ausgeschaltete grau
    for (int bandIndex = 0; bandIndex < bandCount; ++bandIndex)
    {
        const auto& band = bands[(size_t) bandIndex];

        if (! band.isMapped())
            continue;

        const auto selected = bandIndex == selectedBand;
        const auto diameter = selected ? selectedHandleDiameter : handleDiameter;
        const auto circle = juce::Rectangle<float> (diameter, diameter)
                                .withCentre (bandPosition (bandIndex));

        if (selected)
        {
            g.setColour (band.on ? handleColour : handleOff);
            g.fillEllipse (circle);
            g.setColour (juce::Colour (0xff141414));
        }
        else
        {
            const auto ring = band.on ? handleColour : handleOff;
            g.setColour (juce::Colour (0xff0e1112).withAlpha (0.55f));
            g.fillEllipse (circle);   // Punkt hebt sich von der Kurve ab
            g.setColour (ring);
            g.drawEllipse (circle.reduced (1.0f), 2.0f);
        }

        g.setFont (push::scaledFont (selected ? 18.0f : 15.0f));
        g.drawText (juce::String (bandIndex + 1), circle.toNearestInt(),
                    juce::Justification::centred);
    }

    // Readout des gewählten Bands (oben rechts, Live-Look)
    const auto& selected = bands[(size_t) selectedBand];

    if (selected.isMapped())
    {
        juce::String readout;
        readout << hzText (normToHz (selected.frequencyNorm));

        if (shapeHasGain (shapeOf (selected)))
            readout << juce::String::fromUTF8 ("  \xC2\xB7  ")
                    << (selected.gainDb >= 0.0 ? "+" : "")
                    << juce::String (selected.gainDb, 1) << " dB";

        readout << juce::String::fromUTF8 ("  \xC2\xB7  Q ")
                << juce::String (normToQ (selected.resonanceNorm), 2);

        g.setColour (push::colours::text.withAlpha (0.9f));
        g.setFont (push::scaledFont (12.0f));
        g.drawText (readout, area.toNearestInt().reduced (8, 4),
                    juce::Justification::topRight);
    }

    // Typ-Label zwischen den Stepper-Kacheln
    const auto footer = getLocalBounds().removeFromBottom (footerHeight).reduced (8, 5);
    const auto typeLabel = selected.typeValue >= 0
                                   && selected.typeValue < selected.typeItems.size()
                               ? selected.typeItems[selected.typeValue]
                               : juce::String ("—");

    g.setColour (push::colours::text);
    g.setFont (push::scaledFont (12.0f));
    g.drawFittedText (typeLabel,
                      footer.withX (footer.getX() + 42).withWidth (110),
                      juce::Justification::centred, 1, 1.0f);
}

//==============================================================================
std::unique_ptr<TouchLiveBespokePanel>
createBespokePanel (const juce::String& className, TouchLiveClient& client)
{
    if (className == "Eq8")
        return std::make_unique<TouchLiveEq8Panel> (client);

    return nullptr;   // kein bespoke UI → generische Bank (§6b)
}

} // namespace conduit
