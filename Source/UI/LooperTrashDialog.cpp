#include "LooperTrashDialog.h"

#include "PushLookAndFeel.h"

namespace conduit
{

namespace
{
    constexpr int rowHeight = 48;      // 44px-Touch-Target + Luft
    constexpr int maxVisibleRows = 6;  // darüber scrollt der Viewport
    constexpr int dialogWidth = 340;
}

LooperTrashDialog::LooperTrashDialog (std::vector<Item> items)
{
    titleLabel.setText (juce::String::fromUTF8 ("↺ Papierkorb — antippen stellt wieder her"),
                        juce::dontSendNotification);
    titleLabel.setFont (push::scaledFont (14.0f, true));
    titleLabel.setColour (juce::Label::textColourId, push::colours::text);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    const auto dismiss = [this]
    {
        if (auto* box = findParentComponentOfClass<juce::CallOutBox>())
            box->dismiss();
    };

    for (const auto& item : items)
    {
        auto button = std::make_unique<juce::TextButton> (
            item.label + juce::String::fromUTF8 ("   ·   ") + item.timeLabel);
        button->onClick = [this, dismiss, entryId = item.entryId]
        {
            if (onRestore != nullptr)
                onRestore (entryId);
            dismiss();
        };
        rowContainer.addAndMakeVisible (*button);
        rowButtons.push_back (std::move (button));
    }

    viewport.setViewedComponent (&rowContainer, false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);

    const auto visibleRows = juce::jmin ((int) rowButtons.size(), maxVisibleRows);
    setSize (dialogWidth, 34 + visibleRows * rowHeight + 10);
}

void LooperTrashDialog::resized()
{
    auto bounds = getLocalBounds().reduced (10, 4);
    titleLabel.setBounds (bounds.removeFromTop (26));

    viewport.setBounds (bounds);
    rowContainer.setSize (bounds.getWidth(), (int) rowButtons.size() * rowHeight);

    auto rows = rowContainer.getLocalBounds();
    for (auto& button : rowButtons)
        button->setBounds (rows.removeFromTop (rowHeight).reduced (0, 3));
}

} // namespace conduit
