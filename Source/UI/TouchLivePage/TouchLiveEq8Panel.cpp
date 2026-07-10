#include "TouchLiveEq8Panel.h"

#include <cmath>
#include <complex>

#include "UI/PushLookAndFeel.h"

namespace conduit
{

namespace
{
    constexpr double sampleRateForDisplay = 48000.0;   // nur Kurven-Näherung
    constexpr double minHz = 10.0, maxHz = 22000.0;
    constexpr int curvePoints = 96;

    // Band-Punktfarben (Anlehnung an Lives EQ-Eight-Nummerierung)
    const juce::Colour bandColours[TouchLiveEq8Panel::bandCount] = {
        juce::Colour (0xffff8447), juce::Colour (0xffffc247),
        juce::Colour (0xffe8e847), juce::Colour (0xff7ce847),
        juce::Colour (0xff47e8c2), juce::Colour (0xff47a8ff),
        juce::Colour (0xffb47cff), juce::Colour (0xffff6b8a),
    };

    // Wire-Norm ↔ Hz (Kalibrier-Kandidat Feldtest, §11): log 10 Hz – 22 kHz
    [[nodiscard]] double normToHz (double norm)
    {
        return minHz * std::pow (maxHz / minHz, juce::jlimit (0.0, 1.0, norm));
    }

    // Wire-Norm ↔ Q (Kalibrier-Kandidat): log 0.1 – 18
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

        if (! band.isMapped())
            return;

        band.resonanceNorm = qSlider.getValue();
        sendParameter (band.resonanceIndex, (float) band.resonanceNorm, true);
        curveDirty = true;
        repaint();
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

    if (const auto* meta = parmeta.getArray())
    {
        for (int index = 0; index < meta->size(); ++index)
        {
            const auto& entry = meta->getReference (index);
            const auto name = stringField (entry, "name");

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

    // Auswahl auf ein gemapptes Band setzen
    if (! bands[(size_t) selectedBand].isMapped())
        for (int bandIndex = 0; bandIndex < bandCount; ++bandIndex)
            if (bands[(size_t) bandIndex].isMapped())
            {
                selectedBand = bandIndex;
                break;
            }

    dragActive = false;
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

    for (int bandIndex = 0; bandIndex < bandCount; ++bandIndex)
    {
        auto& band = bands[(size_t) bandIndex];

        if (! band.isMapped())
            continue;

        // Lokal-optimistisch (§5.1): das gezogene Band folgt NUR dem Finger
        const auto dragged = dragActive && bandIndex == selectedBand;

        band.on = valueAt (band.onIndex, band.on ? 1.0 : 0.0) > 0.5;
        band.typeValue = juce::jlimit (0, juce::jmax (0, band.typeItems.size() - 1),
                                       (int) std::lround (valueAt (band.typeIndex,
                                                                   band.typeValue)));

        if (! dragged)
        {
            band.frequencyNorm = juce::jlimit (0.0, 1.0,
                                               valueAt (band.frequencyIndex,
                                                        band.frequencyNorm));
            band.gainDb = juce::jlimit (band.gainMin, band.gainMax,
                                        valueAt (band.gainIndex, band.gainDb));
        }

        band.resonanceNorm = juce::jlimit (0.0, 1.0,
                                           valueAt (band.resonanceIndex,
                                                    band.resonanceNorm));
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
    if (! dragActive)
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

int TouchLiveEq8Panel::frequencyIndexOf (int band) const
{
    return band >= 0 && band < bandCount ? bands[(size_t) band].frequencyIndex : -1;
}

int TouchLiveEq8Panel::gainIndexOf (int band) const
{
    return band >= 0 && band < bandCount ? bands[(size_t) band].gainIndex : -1;
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

double TouchLiveEq8Panel::responseDbAt (double hz) const
{
    double totalDb = 0.0;

    for (const auto& band : bands)
    {
        if (! band.isMapped() || ! band.on)
            continue;

        const auto shape = shapeOf (band);
        const auto f0 = normToHz (band.frequencyNorm);
        const auto q = juce::jmax (0.05, normToQ (band.resonanceNorm));
        const auto w0 = juce::MathConstants<double>::twoPi
                            * juce::jlimit (minHz, sampleRateForDisplay * 0.499, f0)
                            / sampleRateForDisplay;
        const auto cw = std::cos (w0);
        const auto sw = std::sin (w0);
        const auto alpha = sw / (2.0 * q);
        const auto a = std::pow (10.0, band.gainDb / 40.0);
        const auto sqrtA = std::sqrt (a);

        double b0, b1, b2, a0, a1, a2;   // RBJ Audio-EQ-Cookbook

        switch (shape)
        {
            case Shape::bell:
                b0 = 1.0 + alpha * a;  b1 = -2.0 * cw;      b2 = 1.0 - alpha * a;
                a0 = 1.0 + alpha / a;  a1 = -2.0 * cw;      a2 = 1.0 - alpha / a;
                break;

            case Shape::lowShelf:
                b0 = a * ((a + 1.0) - (a - 1.0) * cw + 2.0 * sqrtA * alpha);
                b1 = 2.0 * a * ((a - 1.0) - (a + 1.0) * cw);
                b2 = a * ((a + 1.0) - (a - 1.0) * cw - 2.0 * sqrtA * alpha);
                a0 = (a + 1.0) + (a - 1.0) * cw + 2.0 * sqrtA * alpha;
                a1 = -2.0 * ((a - 1.0) + (a + 1.0) * cw);
                a2 = (a + 1.0) + (a - 1.0) * cw - 2.0 * sqrtA * alpha;
                break;

            case Shape::highShelf:
                b0 = a * ((a + 1.0) + (a - 1.0) * cw + 2.0 * sqrtA * alpha);
                b1 = -2.0 * a * ((a - 1.0) + (a + 1.0) * cw);
                b2 = a * ((a + 1.0) + (a - 1.0) * cw - 2.0 * sqrtA * alpha);
                a0 = (a + 1.0) - (a - 1.0) * cw + 2.0 * sqrtA * alpha;
                a1 = 2.0 * ((a - 1.0) - (a + 1.0) * cw);
                a2 = (a + 1.0) - (a - 1.0) * cw - 2.0 * sqrtA * alpha;
                break;

            case Shape::notch:
                b0 = 1.0;              b1 = -2.0 * cw;      b2 = 1.0;
                a0 = 1.0 + alpha;      a1 = -2.0 * cw;      a2 = 1.0 - alpha;
                break;

            case Shape::lowCut12:
            case Shape::lowCut48:
                b0 = (1.0 + cw) / 2.0; b1 = -(1.0 + cw);    b2 = (1.0 + cw) / 2.0;
                a0 = 1.0 + alpha;      a1 = -2.0 * cw;      a2 = 1.0 - alpha;
                break;

            case Shape::highCut12:
            case Shape::highCut48:
            default:
                b0 = (1.0 - cw) / 2.0; b1 = 1.0 - cw;       b2 = (1.0 - cw) / 2.0;
                a0 = 1.0 + alpha;      a1 = -2.0 * cw;      a2 = 1.0 - alpha;
                break;
        }

        // |H(e^jw)| an der Auswertungsfrequenz
        const auto w = juce::MathConstants<double>::twoPi * hz / sampleRateForDisplay;
        const std::complex<double> z1 = std::polar (1.0, -w);
        const auto z2 = z1 * z1;
        const auto magnitude = std::abs ((b0 + b1 * z1 + b2 * z2)
                                         / (a0 + a1 * z1 + a2 * z2));
        auto db = 20.0 * std::log10 (juce::jmax (1.0e-6, magnitude));

        // 48er-Cuts: vierfach kaskadierte 12er (Anzeige-Näherung)
        if (shape == Shape::lowCut48 || shape == Shape::highCut48)
            db *= 4.0;

        totalDb += db;
    }

    return totalDb;
}

void TouchLiveEq8Panel::rebuildCurve()
{
    curve.clear();
    const auto area = plotArea();

    for (int i = 0; i < curvePoints; ++i)
    {
        const auto norm = (double) i / (curvePoints - 1);
        const auto db = juce::jlimit (-plotDbRange * 1.5, plotDbRange * 1.5,
                                      responseDbAt (normToHz (norm)));
        const juce::Point<float> point { xForNorm (norm), yForDb (db) };

        if (i == 0)
            curve.startNewSubPath (point);
        else
            curve.lineTo (point);
    }

    juce::ignoreUnused (area);
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

void TouchLiveEq8Panel::mouseDown (const juce::MouseEvent& event)
{
    beginDrag (event.position);
}

void TouchLiveEq8Panel::mouseDrag (const juce::MouseEvent& event)
{
    dragTo (event.position);
}

void TouchLiveEq8Panel::mouseUp (const juce::MouseEvent&)
{
    endDrag();
}

void TouchLiveEq8Panel::mouseDoubleClick (const juce::MouseEvent& event)
{
    toggleBandOn (bandAt (event.position));
}

//==============================================================================
void TouchLiveEq8Panel::paint (juce::Graphics& g)
{
    const auto area = plotArea();

    g.setColour (juce::Colour (0xff16181b));
    g.fillRoundedRectangle (area, 5.0f);

    if (! isUsable())
    {
        g.setColour (push::colours::textDim);
        g.setFont (push::scaledFont (13.0f));
        g.drawText (juce::String::fromUTF8 (
                        "EQ-Eight-Parameter nicht zuordenbar — BANK nutzen"),
                    area.toNearestInt(), juce::Justification::centred);
        return;
    }

    // Frequenz-Raster (Dekaden hell, Zwischenwerte dim) + dB-Linien
    g.setFont (push::scaledFont (9.0f));

    for (const auto hz : { 20.0, 50.0, 100.0, 200.0, 500.0, 1000.0,
                           2000.0, 5000.0, 10000.0, 20000.0 })
    {
        const auto norm = std::log (hz / minHz) / std::log (maxHz / minHz);
        const auto x = xForNorm (norm);
        const auto decade = hz == 100.0 || hz == 1000.0 || hz == 10000.0;

        g.setColour (push::colours::outline.withAlpha (decade ? 0.5f : 0.2f));
        g.drawVerticalLine ((int) x, area.getY(), area.getBottom());

        if (decade)
        {
            g.setColour (push::colours::textDim);
            g.drawText (hzText (hz), (int) x + 3, (int) area.getBottom() - 14,
                        50, 12, juce::Justification::left);
        }
    }

    for (const auto db : { -12.0, -6.0, 0.0, 6.0, 12.0 })
    {
        const auto y = yForDb (db);
        g.setColour (push::colours::outline.withAlpha (db == 0.0 ? 0.6f : 0.2f));
        g.drawHorizontalLine ((int) y, area.getX(), area.getRight());
    }

    // Summenkurve + dezente Füllung zur 0-dB-Linie
    if (curveDirty)
        rebuildCurve();

    auto fill = curve;
    fill.lineTo (area.getRight(), yForDb (0.0));
    fill.lineTo (area.getX(), yForDb (0.0));
    fill.closeSubPath();

    g.setColour (push::colours::ledCyan.withAlpha (0.10f));
    g.fillPath (fill);
    g.setColour (push::colours::ledCyan.withAlpha (0.85f));
    g.strokePath (curve, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Band-Punkte (gefüllt = an, Ring = Auswahl)
    for (int bandIndex = 0; bandIndex < bandCount; ++bandIndex)
    {
        const auto& band = bands[(size_t) bandIndex];

        if (! band.isMapped())
            continue;

        const auto centre = bandPosition (bandIndex);
        const auto colour = bandColours[bandIndex];
        const auto circle = juce::Rectangle<float> (22.0f, 22.0f).withCentre (centre);

        if (band.on)
        {
            g.setColour (colour.withAlpha (0.9f));
            g.fillEllipse (circle);
            g.setColour (juce::Colours::black.withAlpha (0.8f));
        }
        else
        {
            g.setColour (colour.withAlpha (0.55f));
            g.drawEllipse (circle, 1.5f);
        }

        g.setFont (push::scaledFont (11.0f));
        g.drawText (juce::String (bandIndex + 1), circle.toNearestInt(),
                    juce::Justification::centred);

        if (bandIndex == selectedBand)
        {
            g.setColour (push::colours::text);
            g.drawEllipse (circle.expanded (3.5f), 1.6f);
        }
    }

    // Readout des gewählten Bands (oben rechts im Plot)
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
