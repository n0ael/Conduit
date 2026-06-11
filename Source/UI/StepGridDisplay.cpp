#include "StepGridDisplay.h"

#include "Modules/StepSequencerModule.h"

namespace conduit
{

namespace
{
    constexpr int numRows = StepSequencerModule::numRows;
    constexpr int numColumns = StepSequencerModule::stepsPerRow;

    const std::array<juce::Colour, 4> rowColours {
        juce::Colour (0xff7bc8f6),   // A — blau
        juce::Colour (0xfff6a97b),   // B — orange
        juce::Colour (0xff8fd0a0),   // C — grün
        juce::Colour (0xffc9a0f0),   // D — violett
    };
}

StepGridDisplay::StepGridDisplay (juce::ValueTree nodeTreeToBind, GraphManager& graphManagerToUse)
    : nodeTree (std::move (nodeTreeToBind)),
      graphManager (graphManagerToUse),
      nodeUuid (nodeTree.getProperty (id::nodeId).toString())
{
    nodeTree.addListener (this);
    startTimerHz (30);  // Playhead-Refresh (CLAUDE.md 10)
}

StepGridDisplay::~StepGridDisplay()
{
    nodeTree.removeListener (this);
}

void StepGridDisplay::stopUpdates()
{
    stopTimer();
}

//==============================================================================
void StepGridDisplay::timerCallback()
{
    // Transienter Lookup statt gehaltenem Pointer (5.3)
    if (auto* sequencer = dynamic_cast<StepSequencerModule*> (graphManager.getModuleFor (nodeUuid)))
    {
        const auto cell = sequencer->getCurrentCell();

        if (cell != highlightedCell)
        {
            highlightedCell = cell;
            repaint();
        }
    }
}

void StepGridDisplay::valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property)
{
    // Step-Werte, length, mode … — alles sichtbar, also nachzeichnen
    if (property == id::paramValue && tree.hasType (id::parameter))
        repaint();
}

//==============================================================================
juce::ValueTree StepGridDisplay::parameterFor (const juce::String& parameterId) const
{
    return nodeTree.getChildWithName (id::parameters)
               .getChildWithProperty (id::paramId, parameterId);
}

float StepGridDisplay::parameterValue (const juce::String& parameterId, double fallback) const
{
    return static_cast<float> ((double) parameterFor (parameterId)
                                   .getProperty (id::paramValue, fallback));
}

juce::Rectangle<float> StepGridDisplay::cellBounds (int row, int column) const
{
    const auto cellWidth  = static_cast<float> (getWidth())  / numColumns;
    const auto cellHeight = static_cast<float> (getHeight()) / numRows;

    return { static_cast<float> (column) * cellWidth,
             static_cast<float> (row) * cellHeight,
             cellWidth, cellHeight };
}

//==============================================================================
void StepGridDisplay::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff15171a));

    const auto mode   = juce::jlimit (0, 2, juce::roundToInt (parameterValue ("mode", 0.0)));
    const auto length = juce::jlimit (1, numColumns, juce::roundToInt (parameterValue ("length", 16.0)));

    const auto highlightRow    = highlightedCell >= 0 ? highlightedCell / numColumns : -1;
    const auto highlightColumn = highlightedCell >= 0 ? highlightedCell % numColumns : -1;

    for (int row = 0; row < numRows; ++row)
    {
        for (int column = 0; column < numColumns; ++column)
        {
            const auto bounds = cellBounds (row, column).reduced (1.5f);
            const auto value = juce::jlimit (0.0f, 1.0f,
                parameterValue (StepSequencerModule::stepParameterId (row, column), 0.0));

            const auto active = column < length;
            const auto isPlayhead = mode == 0 ? column == highlightColumn
                                              : (row == highlightRow && column == highlightColumn);

            // Zellrahmen + Wert-Balken von unten
            g.setColour (juce::Colours::white.withAlpha (active ? 0.08f : 0.03f));
            g.fillRoundedRectangle (bounds, 2.0f);

            auto barColour = rowColours[static_cast<size_t> (row)]
                                 .withAlpha (active ? 0.9f : 0.25f);
            g.setColour (barColour);
            g.fillRoundedRectangle (bounds.withTrimmedTop (bounds.getHeight() * (1.0f - value)),
                                    2.0f);

            if (isPlayhead)
            {
                g.setColour (juce::Colours::white.withAlpha (0.85f));
                g.drawRoundedRectangle (bounds, 2.0f, 1.5f);
            }
        }
    }
}

void StepGridDisplay::mouseDown (const juce::MouseEvent& event)
{
    editCellAt (event.getPosition());
}

void StepGridDisplay::mouseDrag (const juce::MouseEvent& event)
{
    editCellAt (event.getPosition());
}

void StepGridDisplay::editCellAt (juce::Point<int> position)
{
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    const auto column = juce::jlimit (0, numColumns - 1, position.x * numColumns / getWidth());
    const auto row    = juce::jlimit (0, numRows - 1,    position.y * numRows / getHeight());

    // Wert = vertikale Position INNERHALB der Zelle (oben = 1)
    const auto cell = cellBounds (row, column);
    const auto value = juce::jlimit (0.0f, 1.0f,
        1.0f - (static_cast<float> (position.y) - cell.getY()) / cell.getHeight());

    // Schreibt NUR in den Tree (ohne Undo — Parameter-Sweep wie Slider/OSC);
    // der GraphManager spiegelt aufs Echtzeit-Atomic
    if (auto parameter = parameterFor (StepSequencerModule::stepParameterId (row, column));
        parameter.isValid())
        parameter.setProperty (id::paramValue, value, nullptr);
}

} // namespace conduit
