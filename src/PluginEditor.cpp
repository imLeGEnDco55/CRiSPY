#include "PluginEditor.h"

//==============================================================================
CrispyEditor::CrispyEditor(CrispyProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(400, 400);
    setResizable(false, false);

    // Load logo from binary resources
    logoImage = juce::ImageCache::getFromMemory(BinaryData::Crispy_png, BinaryData::Crispy_pngSize);

    // Configure API Key text editor
    apiKeyEditor.setMultiLine(false);
    apiKeyEditor.setPasswordCharacter(L'*');
    apiKeyEditor.setColour(juce::TextEditor::backgroundColourId, colCardBg);
    apiKeyEditor.setColour(juce::TextEditor::textColourId, colTextWhite);
    apiKeyEditor.setColour(juce::TextEditor::outlineColourId, colRoyalBlue);
    apiKeyEditor.setColour(juce::TextEditor::focusedOutlineColourId, colCyan);
    apiKeyEditor.setFont(juce::Font("Segoe UI", 14.0f, juce::Font::plain));
    addChildComponent(apiKeyEditor);

    if (processor.getApiKey().isNotEmpty())
        apiKeyEditor.setText(processor.getApiKey(), false);

    // Start animation timer at 60fps
    startTimerHz(60);
}

CrispyEditor::~CrispyEditor()
{
    stopTimer();
}

//==============================================================================
void CrispyEditor::timerCallback()
{
    spinnerAngle += 2.5f;
    if (spinnerAngle >= 360.0f) spinnerAngle = 0.0f;

    gridOffset += 0.4f;
    if (gridOffset >= 40.0f) gridOffset = 0.0f;

    repaint();
}

//==============================================================================
void CrispyEditor::resized()
{
    // API key editor positioning is handled in paint via setBounds
}

//==============================================================================
// Drawing helpers

void CrispyEditor::drawLogo(juce::Graphics& g, float centerX, float centerY)
{
    if (logoImage.isValid())
    {
        float destW = 300.0f;
        float destH = (float)logoImage.getHeight() * (destW / (float)logoImage.getWidth());
        float destX = centerX - destW / 2.0f;
        float destY = centerY - destH / 2.0f;
        g.setOpacity(1.0f);
        g.drawImage(logoImage,
                    juce::Rectangle<float>(destX, destY, destW, destH),
                    juce::RectanglePlacement::centred);
    }
    else
    {
        g.setColour(colLimeGreen);
        g.setFont(juce::Font("Segoe UI", 32.0f, juce::Font::bold));
        g.drawText("CRiSPY", juce::Rectangle<float>(centerX - 100, centerY - 20, 200, 40),
                   juce::Justification::centred);
    }
}

void CrispyEditor::drawCheckmarkIcon(juce::Graphics& g, float cx, float cy, float radius, bool filled)
{
    if (filled)
    {
        g.setColour(colLimeGreen);
        g.fillEllipse(cx - radius, cy - radius, radius * 2, radius * 2);
        // Neon glow outer ring
        g.drawEllipse(cx - radius - 4, cy - radius - 4, (radius + 4) * 2, (radius + 4) * 2, 1.0f);
    }
    else
    {
        g.setColour(colLimeGreen);
        g.drawEllipse(cx - radius, cy - radius, radius * 2, radius * 2, 2.5f);
    }

    // Checkmark
    g.setColour(colTextWhite);
    juce::Path check;
    check.startNewSubPath(cx - radius * 0.4f, cy + radius * 0.05f);
    check.lineTo(cx - radius * 0.05f, cy + radius * 0.35f);
    check.lineTo(cx + radius * 0.45f, cy - radius * 0.25f);
    g.strokePath(check, juce::PathStrokeType(3.0f));
}

void CrispyEditor::drawRestartIcon(juce::Graphics& g, float cx, float cy, float radius, bool hover)
{
    auto col = hover ? colHotPink : colCyan;
    g.setColour(col);

    if (hover)
    {
        float rot = (float)juce::Time::getMillisecondCounter() * 0.15f;
        auto transform = juce::AffineTransform::rotation(juce::degreesToRadians(rot), cx, cy);
        juce::Path circle;
        circle.addEllipse(cx - radius, cy - radius, radius * 2, radius * 2);
        g.strokePath(circle, juce::PathStrokeType(2.0f), transform);

        // Arrow head
        juce::Path arrow;
        arrow.startNewSubPath(cx + radius - 4, cy - 2);
        arrow.lineTo(cx + radius + 1, cy + 3);
        arrow.startNewSubPath(cx + radius + 5, cy - 2);
        arrow.lineTo(cx + radius + 1, cy + 3);
        g.strokePath(arrow, juce::PathStrokeType(2.0f), transform);
    }
    else
    {
        g.drawEllipse(cx - radius, cy - radius, radius * 2, radius * 2, 2.0f);

        juce::Path arrow;
        arrow.startNewSubPath(cx + radius - 4, cy - 2);
        arrow.lineTo(cx + radius + 1, cy + 3);
        arrow.startNewSubPath(cx + radius + 5, cy - 2);
        arrow.lineTo(cx + radius + 1, cy + 3);
        g.strokePath(arrow, juce::PathStrokeType(2.0f));
    }
}

void CrispyEditor::drawSoundwaveIcon(juce::Graphics& g, float centerX, float centerY)
{
    float barWidth = 8.0f;
    float gap = 6.0f;
    float time = (float)juce::Time::getMillisecondCounter() / 1000.0f;
    float baseHeights[] = { 24.0f, 60.0f, 100.0f, 60.0f, 24.0f };
    juce::Colour barCols[] = { colRoyalBlue, colCyan, colLimeGreen, colCyan, colRoyalBlue };

    bool isProcessing = (processor.getAppState() == CRISPY_PROCESSING);
    float speed = isProcessing ? 14.0f : 3.5f;
    float amp = isProcessing ? 30.0f : 8.0f;

    for (int i = 0; i < 5; ++i)
    {
        float h = baseHeights[i] + std::sin(time * speed + i * 1.5f) * amp;
        if (h < 8.0f) h = 8.0f;

        float x = centerX + (i - 2) * (barWidth + gap);
        g.setColour(barCols[i]);
        g.fillRoundedRectangle(x - barWidth / 2, centerY - h / 2, barWidth, h, 4.0f);
    }
}

void CrispyEditor::drawFileIcon(juce::Graphics& g, float centerX, float centerY, bool glow)
{
    float width = 70.0f, height = 98.0f, fold = 20.0f;
    auto col = glow ? colHotPink : colCyan;

    // Background
    g.setColour(colCardBg);
    g.fillRect(centerX - width / 2, centerY - height / 2, width, height);

    // Border
    g.setColour(col);
    g.drawRect(centerX - width / 2, centerY - height / 2, width, height, 2.0f);

    // Fold
    juce::Path foldPath;
    foldPath.startNewSubPath(centerX + width / 2 - fold, centerY - height / 2);
    foldPath.lineTo(centerX + width / 2 - fold, centerY - height / 2 + fold);
    foldPath.lineTo(centerX + width / 2, centerY - height / 2 + fold);
    g.strokePath(foldPath, juce::PathStrokeType(2.0f));

    // Lines inside
    g.setColour(colRoyalBlue);
    g.drawLine(centerX - 18, centerY - 12, centerX + 18, centerY - 12, 2.5f);
    g.drawLine(centerX - 18, centerY, centerX + 8, centerY, 2.5f);
    g.drawLine(centerX - 18, centerY + 12, centerX + 18, centerY + 12, 2.5f);

    // Checkmark
    if (processor.getAppState() == CRISPY_COMPLETE)
    {
        g.setColour(colLimeGreen);
        juce::Path check;
        check.startNewSubPath(centerX + 11, centerY + 30);
        check.lineTo(centerX + 17, centerY + 34);
        check.lineTo(centerX + 25, centerY + 26);
        g.strokePath(check, juce::PathStrokeType(3.0f));
    }
}

//==============================================================================
void CrispyEditor::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    float W = area.getWidth();
    float H = area.getHeight();

    // Background gradient
    g.setGradientFill(juce::ColourGradient(colBgStart, 0, 0, colBgEnd, 0, H, false));
    g.fillRect(area);

    // Animated cyber grid
    g.setColour(colGrid);
    float gridSpacing = 40.0f;
    float startY = std::fmod(gridOffset, gridSpacing);
    for (float y = startY; y < H; y += gridSpacing)
        g.drawLine(0, y, W, y, 0.5f);
    for (float x = 0; x < W; x += gridSpacing)
        g.drawLine(x, 0, x, H, 0.5f);

    auto state = processor.getAppState();

    if (state == CRISPY_KEY_INPUT)
    {
        // Show API key editor
        apiKeyEditor.setVisible(true);
        apiKeyEditor.setBounds(58, 158, 284, 20);

        // Draw border around editor
        g.setColour(apiKeyEditor.hasKeyboardFocus(false) ? colCyan : colRoyalBlue);
        g.drawRoundedRectangle(50.0f, 150.0f, 300.0f, 36.0f, 6.0f, apiKeyEditor.hasKeyboardFocus(false) ? 2.0f : 1.0f);

        g.setColour(colCardBg);
        g.fillRoundedRectangle(50.0f, 150.0f, 300.0f, 36.0f, 6.0f);
        // Re-draw border on top of fill
        g.setColour(apiKeyEditor.hasKeyboardFocus(false) ? colCyan : colRoyalBlue);
        g.drawRoundedRectangle(50.0f, 150.0f, 300.0f, 36.0f, 6.0f, apiKeyEditor.hasKeyboardFocus(false) ? 2.0f : 1.0f);

        // OK button
        float btnCx = 200.0f, btnCy = 230.0f, btnRadius = 24.0f;
        rectBtnSave = juce::Rectangle<float>(btnCx - btnRadius, btnCy - btnRadius, btnRadius * 2, btnRadius * 2);
        bool hoverSave = rectBtnSave.contains(mousePos);

        float pulse = hoverSave ? 0.0f : std::sin((float)juce::Time::getMillisecondCounter() * 0.006f) * 1.5f;
        drawCheckmarkIcon(g, btnCx, btnCy, btnRadius + pulse, hoverSave);
    }
    else
    {
        apiKeyEditor.setVisible(false);

        if (state == CRISPY_READY)
        {
            drawLogo(g, 200.0f, 60.0f);

            float circleX = 200.0f, circleY = 235.0f, circleRadius = 80.0f;
            bool hoverCard = (mousePos - juce::Point<float>(circleX, circleY)).getDistanceFromOrigin() <= circleRadius;

            // Rotating outer rings
            {
                juce::Graphics::ScopedSaveState sss(g);
                auto t1 = juce::AffineTransform::rotation(juce::degreesToRadians(spinnerAngle), circleX, circleY);
                g.setColour(colRoyalBlue);
                juce::Path ring1;
                ring1.addEllipse(circleX - circleRadius - 6, circleY - circleRadius - 6, (circleRadius + 6) * 2, (circleRadius + 6) * 2);
                g.strokePath(ring1, juce::PathStrokeType(1.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt), t1);

                auto t2 = juce::AffineTransform::rotation(juce::degreesToRadians(-spinnerAngle * 1.5f), circleX, circleY);
                g.setColour(colCyan);
                juce::Path ring2;
                ring2.addEllipse(circleX - circleRadius - 12, circleY - circleRadius - 12, (circleRadius + 12) * 2, (circleRadius + 12) * 2);
                g.strokePath(ring2, juce::PathStrokeType(1.0f), t2);
            }

            // Center circle
            float drawRadius = circleRadius + (hoverCard ? std::sin((float)juce::Time::getMillisecondCounter() * 0.01f) * 2.0f : 0.0f);
            g.setColour(colCardBg);
            g.fillEllipse(circleX - drawRadius, circleY - drawRadius, drawRadius * 2, drawRadius * 2);
            g.setColour(hoverCard ? colCyan : colCardBorder);
            g.drawEllipse(circleX - drawRadius, circleY - drawRadius, drawRadius * 2, drawRadius * 2, hoverCard ? 2.5f : 1.5f);

            drawSoundwaveIcon(g, circleX, circleY);
        }
        else if (state == CRISPY_PROCESSING)
        {
            drawLogo(g, 200.0f, 60.0f);

            float circleX = 200.0f, circleY = 235.0f, circleRadius = 80.0f;

            g.setColour(colCardBg);
            g.fillEllipse(circleX - circleRadius, circleY - circleRadius, circleRadius * 2, circleRadius * 2);
            g.setColour(colCardBorder);
            g.drawEllipse(circleX - circleRadius, circleY - circleRadius, circleRadius * 2, circleRadius * 2, 1.5f);

            // Dual-spinner orbits
            {
                auto t1 = juce::AffineTransform::rotation(juce::degreesToRadians(spinnerAngle), circleX, circleY);
                g.setColour(colCyan);
                juce::Path orbit1;
                orbit1.addEllipse(circleX - circleRadius - 6, circleY - circleRadius - 6, (circleRadius + 6) * 2, (circleRadius + 6) * 2);
                g.strokePath(orbit1, juce::PathStrokeType(1.0f), t1);
                // Dot on orbit
                float dotX = circleX + circleRadius + 6.0f;
                float dotY = circleY;
                juce::Point<float> dotPos(dotX, dotY);
                dotPos = dotPos.transformedBy(t1);
                g.fillEllipse(dotPos.x - 5, dotPos.y - 5, 10, 10);

                auto t2 = juce::AffineTransform::rotation(juce::degreesToRadians(-spinnerAngle * 1.5f), circleX, circleY);
                g.setColour(colHotPink);
                juce::Path orbit2;
                orbit2.addEllipse(circleX - circleRadius - 12, circleY - circleRadius - 12, (circleRadius + 12) * 2, (circleRadius + 12) * 2);
                g.strokePath(orbit2, juce::PathStrokeType(1.0f), t2);
                juce::Point<float> dotPos2(circleX + circleRadius + 12.0f, circleY);
                dotPos2 = dotPos2.transformedBy(t2);
                g.fillEllipse(dotPos2.x - 4, dotPos2.y - 4, 8, 8);
            }

            // Sweep scanner line
            float sweepProgress = std::sin((float)juce::Time::getMillisecondCounter() * 0.0035f);
            float sweepY = circleY + sweepProgress * (circleRadius - 5.0f);
            float lineHalfWidth = std::sqrt(circleRadius * circleRadius - (sweepY - circleY) * (sweepY - circleY));
            g.setColour(colCyan);
            g.drawLine(circleX - lineHalfWidth, sweepY, circleX + lineHalfWidth, sweepY, 2.0f);

            drawSoundwaveIcon(g, circleX, circleY);
        }
        else if (state == CRISPY_COMPLETE)
        {
            drawCheckmarkIcon(g, 200.0f, 60.0f, 24.0f, true);

            float cardW = 160.0f, cardH = 160.0f;
            rectDragCard = juce::Rectangle<float>(200.0f - cardW / 2, 235.0f - cardH / 2, cardW, cardH);
            bool hoverDrag = rectDragCard.contains(mousePos);

            g.setColour(colCardBg);
            g.fillRoundedRectangle(rectDragCard, 8.0f);
            g.setColour(hoverDrag ? colHotPink : colCyan);
            g.drawRoundedRectangle(rectDragCard, 8.0f, hoverDrag ? 2.5f : 1.5f);

            // Corner accents
            float accentOff = 12.0f;
            g.drawLine(rectDragCard.getX(), rectDragCard.getY() + accentOff, rectDragCard.getX(), rectDragCard.getY(), 2.0f);
            g.drawLine(rectDragCard.getX(), rectDragCard.getY(), rectDragCard.getX() + accentOff, rectDragCard.getY(), 2.0f);
            g.drawLine(rectDragCard.getRight(), rectDragCard.getBottom() - accentOff, rectDragCard.getRight(), rectDragCard.getBottom(), 2.0f);
            g.drawLine(rectDragCard.getRight(), rectDragCard.getBottom(), rectDragCard.getRight() - accentOff, rectDragCard.getBottom(), 2.0f);

            drawFileIcon(g, 200.0f, 235.0f, hoverDrag);

            // Restart icon
            float restX = 350.0f, restY = 350.0f, restR = 16.0f;
            rectBtnReset = juce::Rectangle<float>(restX - restR, restY - restR, restR * 2, restR * 2);
            drawRestartIcon(g, restX, restY, restR, rectBtnReset.contains(mousePos));
        }
        else if (state == CRISPY_ERROR)
        {
            g.setColour(colCoral);
            g.setFont(juce::Font("Segoe UI", 32.0f, juce::Font::bold));
            g.drawText("ERROR", juce::Rectangle<float>(0, 40, W, 60), juce::Justification::centred);

            float cardW = 320.0f, cardH = 140.0f;
            float cardX = (W - cardW) / 2, cardY = 120.0f;
            g.setColour(colCardBg);
            g.fillRoundedRectangle(cardX, cardY, cardW, cardH, 8.0f);
            g.setColour(colCoral);
            g.drawRoundedRectangle(cardX, cardY, cardW, cardH, 8.0f, 1.5f);

            g.setColour(colTextWhite);
            g.setFont(juce::Font("Segoe UI", 11.0f, juce::Font::plain));
            g.drawFittedText(processor.getErrorMessage(),
                             juce::Rectangle<int>((int)(cardX + 10), (int)(cardY + 15), (int)(cardW - 20), (int)(cardH - 25)),
                             juce::Justification::centredLeft, 5);

            float btnCx = 200.0f, btnCy = 295.0f, btnR = 18.0f;
            rectBtnAgain = juce::Rectangle<float>(btnCx - btnR, btnCy - btnR, btnR * 2, btnR * 2);
            drawRestartIcon(g, btnCx, btnCy, btnR, rectBtnAgain.contains(mousePos));
        }
    }

    // Signature
    g.setColour(colTextWhite);
    g.setFont(juce::Font("Segoe UI", 11.0f, juce::Font::plain));
    g.drawText(juce::String::fromUTF8("2026 - imLeGEnDco. - Vicodeado en Antigravity con Gemini"),
               juce::Rectangle<float>(0, 378, W, 15), juce::Justification::centred);
}

//==============================================================================
void CrispyEditor::mouseDown(const juce::MouseEvent& e)
{
    auto pos = e.position;
    auto state = processor.getAppState();

    if (state == CRISPY_KEY_INPUT)
    {
        if (rectBtnSave.contains(pos))
        {
            auto key = apiKeyEditor.getText().trim();
            if (key.isNotEmpty())
                processor.setApiKey(key);
        }
    }
    else if (state == CRISPY_COMPLETE)
    {
        if (rectBtnReset.contains(pos))
        {
            processor.resetToReady();
        }
    }
    else if (state == CRISPY_ERROR)
    {
        if (rectBtnAgain.contains(pos))
        {
            processor.resetToReady();
        }
    }
}

void CrispyEditor::mouseMove(const juce::MouseEvent& e)
{
    mousePos = e.position;

    // Set cursor to hand on interactive elements
    auto state = processor.getAppState();
    bool isHand = false;

    if (state == CRISPY_KEY_INPUT && rectBtnSave.contains(mousePos))
        isHand = true;
    else if (state == CRISPY_COMPLETE && (rectBtnReset.contains(mousePos) || rectDragCard.contains(mousePos)))
        isHand = true;
    else if (state == CRISPY_ERROR && rectBtnAgain.contains(mousePos))
        isHand = true;

    setMouseCursor(isHand ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void CrispyEditor::mouseDrag(const juce::MouseEvent& e)
{
    auto state = processor.getAppState();
    if (state == CRISPY_COMPLETE && rectDragCard.contains(e.mouseDownPosition))
    {
        auto cleanPath = processor.getCleanFilePath();
        if (cleanPath.isNotEmpty() && juce::File(cleanPath).existsAsFile())
        {
            juce::StringArray files;
            files.add(cleanPath);
            juce::DragAndDropContainer::performExternalDragDropOfFiles(files, false);
        }
    }
}

//==============================================================================
bool CrispyEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    auto state = processor.getAppState();
    if (state != CRISPY_READY)
        return false;

    for (auto& f : files)
    {
        auto ext = juce::File(f).getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".mp3" || ext == ".flac" || ext == ".ogg" || ext == ".aiff")
            return true;
    }
    return false;
}

void CrispyEditor::filesDropped(const juce::StringArray& files, int, int)
{
    if (processor.getAppState() == CRISPY_READY && files.size() > 0)
    {
        processor.launchProcessing(files[0]);
    }
}
