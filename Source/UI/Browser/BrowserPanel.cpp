#include "BrowserPanel.h"

#include "UI/PushIcons.h"
#include "UI/PushLookAndFeel.h"

namespace conduit
{

namespace
{
    constexpr int slideMs = 180;

    /** Core-Icon-Referenz → Push-Icon (das Modell kennt keine UI-Header). */
    push::Icon toPushIcon (BrowserModel::Icon icon)
    {
        switch (icon)
        {
            case BrowserModel::Icon::projects:  return push::Icon::browserProjects;
            case BrowserModel::Icon::audio:     return push::Icon::browserAudio;
            case BrowserModel::Icon::cvControl: return push::Icon::browserCvControl;
            case BrowserModel::Icon::audioFx:   return push::Icon::browserAudioFx;
            case BrowserModel::Icon::none:      break;
        }

        return push::Icon::browserPanel;  // unbenutzt (none zeichnet nichts)
    }
} // namespace

//==============================================================================
BrowserPanel::BrowserPanel (BrowserModel& modelToUse)
    : model (modelToUse)
{
    backTile.setTooltip (juce::String::fromUTF8 ("Zurück zur Bereichsübersicht"));
    backTile.onClick = [this] { model.goBack(); };
    addChildComponent (backTile);   // sichtbar nur wenn canGoBack()

    breadcrumbLabel.setColour (juce::Label::textColourId, push::colours::text);
    breadcrumbLabel.setJustificationType (juce::Justification::centredLeft);
    breadcrumbLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (breadcrumbLabel);

    list.setModel (this);
    list.setRowHeight (rowHeight);
    list.setColour (juce::ListBox::backgroundColourId, push::colours::panel);
    list.getViewport()->setScrollBarsShown (true, false);
    // Touch-Flick-Scrolling: Drag auf den Zeilen scrollt die Liste
    list.getViewport()->setScrollOnDragMode (juce::Viewport::ScrollOnDragMode::nonHover);
    addAndMakeVisible (list);

    model.onRowsChanged = [this] { refreshFromModel(); };

    slide.onUpdate = [this] (float value)
    {
        setVisible (value > 0.0f);

        if (onDockWidthChanged != nullptr)
            onDockWidthChanged();
    };

    setVisible (false);
    updateHeader();
}

BrowserPanel::~BrowserPanel()
{
    model.onRowsChanged = nullptr;
    list.setModel (nullptr);
}

//==============================================================================
void BrowserPanel::setOpen (bool shouldBeOpen, bool animate)
{
    if (open == shouldBeOpen)
        return;

    open = shouldBeOpen;

    if (open)
    {
        model.openStartSection();   // Startbereich der aktiven Page
        setVisible (true);
    }

    if (animate)
        slide.animateTo (open ? 1.0f : 0.0f, slideMs);
    else
        slide.snapTo (open ? 1.0f : 0.0f);
}

int BrowserPanel::currentDockWidth() const
{
    return juce::roundToInt (slide.getCurrent() * (float) dockWidth);
}

//==============================================================================
void BrowserPanel::paint (juce::Graphics& g)
{
    g.fillAll (push::colours::panel);

    // Trennkante zum Canvas + Unterkante des Headers
    g.setColour (push::colours::outline);
    g.fillRect (0, 0, 1, getHeight());
    g.fillRect (0, headerHeight - 1, getWidth(), 1);
}

void BrowserPanel::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromLeft (1);   // Trennkante

    auto header = bounds.removeFromTop (headerHeight);
    backTile.setBounds (header.removeFromLeft (headerHeight).reduced (4));
    breadcrumbLabel.setBounds (header.reduced (6, 0));

    list.setBounds (bounds);
}

//==============================================================================
int BrowserPanel::getNumRows()
{
    return (int) model.rows().size();
}

void BrowserPanel::paintListBoxItem (int rowNumber, juce::Graphics& g,
                                     int width, int height, bool rowIsSelected)
{
    const auto& rows = model.rows();

    if (rowNumber < 0 || rowNumber >= (int) rows.size())
        return;

    const auto& row = rows[(size_t) rowNumber];
    const auto isHint = row.kind == BrowserModel::Row::Kind::hint;

    if (rowIsSelected && ! isHint)
    {
        g.setColour (push::colours::tileActive);
        g.fillRect (0, 0, width, height);

        // EIN Akzent: schmaler Selektionsbalken links (Ableton-Look)
        g.setColour (push::colours::ledOrange);
        g.fillRect (0, 0, 3, height);
    }

    // Icon links im 24-px-Raster
    auto area = juce::Rectangle<int> (0, 0, width, height).reduced (12, 0);

    const auto iconBox = area.removeFromLeft (24).withSizeKeepingCentre (24, 24);
    if (row.icon != BrowserModel::Icon::none)
        push::draw (g, toPushIcon (row.icon), iconBox.toFloat().reduced (1.0f),
                    isHint ? push::colours::textDim : push::colours::text);

    area.removeFromLeft (10);

    // Navigierbare Zeilen zeigen ein PERMANENTES ▸ (keine Hover-Affordance)
    const auto navigates = row.kind == BrowserModel::Row::Kind::section
                        || row.kind == BrowserModel::Row::Kind::branch
                        || row.kind == BrowserModel::Row::Kind::category;
    if (navigates)
    {
        const auto chevron = area.removeFromRight (16);
        juce::Path arrow;
        arrow.addTriangle (0.0f, -5.0f, 0.0f, 5.0f, 5.5f, 0.0f);
        g.setColour (push::colours::textDim);
        g.fillPath (arrow, juce::AffineTransform::translation (
                               (float) chevron.getX() + 4.0f,
                               (float) chevron.getCentreY()));
    }

    g.setColour (isHint ? push::colours::textDim : push::colours::text);
    g.setFont (push::scaledFont (16.0f));
    // Nie horizontal stauchen (User-Regel 07/2026) — kürzen statt quetschen
    g.drawText (row.label, area, juce::Justification::centredLeft, true);
}

void BrowserPanel::listBoxItemClicked (int row, const juce::MouseEvent&)
{
    model.activateRow (row);
}

//==============================================================================
void BrowserPanel::refreshFromModel()
{
    list.deselectAllRows();
    list.updateContent();
    updateHeader();
    repaint();
}

void BrowserPanel::updateHeader()
{
    backTile.setVisible (model.canGoBack());
    breadcrumbLabel.setText (model.breadcrumbText(), juce::dontSendNotification);
    breadcrumbLabel.setFont (push::scaledFont (15.0f, true));
}

} // namespace conduit
