/**
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
 *           (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)  
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "RenderTextControl.h"

#include "HTMLTextFormControlElement.h"
#include "HitTestResult.h"
#include "RenderText.h"
#include "RenderTheme.h"
#include "ScrollbarTheme.h"
#include "StyleInheritedData.h"
#include "TextIterator.h"
#include "VisiblePosition.h"
#include <wtf/unicode/CharacterNames.h>

using namespace std;

namespace WebCore {

RenderTextControl::RenderTextControl(Element* element)
    : RenderBlock(element)
{
    ASSERT(isHTMLTextFormControlElement(element));
}

RenderTextControl::~RenderTextControl()
{
}

HTMLTextFormControlElement* RenderTextControl::textFormControlElement() const
{
    return static_cast<HTMLTextFormControlElement*>(node());
}

HTMLElement* RenderTextControl::innerTextElement() const
{
    return textFormControlElement()->innerTextElement();
}

void RenderTextControl::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBlock::styleDidChange(diff, oldStyle);
    Element* innerText = innerTextElement();
    if (!innerText)
        return;
    RenderBlock* innerTextRenderer = toRenderBlock(innerText->renderer());
    if (innerTextRenderer) {
        // We may have set the width and the height in the old style in layout().
        // Reset them now to avoid getting a spurious layout hint.
        innerTextRenderer->style()->setHeight(Length());
        innerTextRenderer->style()->setWidth(Length());
        innerTextRenderer->setStyle(createInnerTextStyle(style()));
        innerText->setNeedsStyleRecalc();
    }
    textFormControlElement()->updatePlaceholderVisibility(false);
}

static inline bool updateUserModifyProperty(Node* node, RenderStyle* style)
{
    bool isEnabled = true;
    bool isReadOnlyControl = false;

    if (node->isElementNode()) {
        Element* element = static_cast<Element*>(node);
        isEnabled = element->isEnabledFormControl();
        isReadOnlyControl = element->isTextFormControl() && static_cast<HTMLTextFormControlElement*>(element)->readOnly();
    }

    style->setUserModify((isReadOnlyControl || !isEnabled) ? READ_ONLY : READ_WRITE_PLAINTEXT_ONLY);
    return !isEnabled;
}

void RenderTextControl::adjustInnerTextStyle(const RenderStyle* startStyle, RenderStyle* textBlockStyle) const
{
    // The inner block, if present, always has its direction set to LTR,
    // so we need to inherit the direction and unicode-bidi style from the element.
    textBlockStyle->setDirection(style()->direction());
    textBlockStyle->setUnicodeBidi(style()->unicodeBidi());

    bool disabled = updateUserModifyProperty(node(), textBlockStyle);
    if (disabled)
        textBlockStyle->setColor(theme()->disabledTextColor(textBlockStyle->visitedDependentColor(CSSPropertyColor), startStyle->visitedDependentColor(CSSPropertyBackgroundColor)));
}

int RenderTextControl::textBlockHeight() const
{
    return height() - borderAndPaddingHeight();
}

int RenderTextControl::textBlockWidth() const
{
    Element* innerText = innerTextElement();
    ASSERT(innerText);

    LayoutUnit unitWidth = width() - borderAndPaddingWidth();
    if (innerText->renderer())
        unitWidth -= innerText->renderBox()->paddingLeft() + innerText->renderBox()->paddingRight();

    return unitWidth;
}

void RenderTextControl::updateFromElement()
{
    Element* innerText = innerTextElement();
    if (innerText && innerText->renderer())
        updateUserModifyProperty(node(), innerText->renderer()->style());
}

VisiblePosition RenderTextControl::visiblePositionForIndex(int index) const
{
    if (index <= 0)
        return VisiblePosition(firstPositionInNode(innerTextElement()), DOWNSTREAM);
    RefPtr<Range> range = Range::create(document());
    range->selectNodeContents(innerTextElement(), ASSERT_NO_EXCEPTION);
    CharacterIterator it(range.get());
    it.advance(index - 1);
    return VisiblePosition(it.range()->endPosition(), UPSTREAM);
}

int RenderTextControl::scrollbarThickness() const
{
    // FIXME: We should get the size of the scrollbar from the RenderTheme instead.
    return ScrollbarTheme::theme()->scrollbarThickness();
}

void RenderTextControl::computeLogicalHeight(LayoutUnit logicalHeight, LayoutUnit logicalTop, LogicalExtentComputedValues& computedValues) const
{
    HTMLElement* innerText = innerTextElement();
    ASSERT(innerText);
    if (RenderBox* innerTextBox = innerText->renderBox()) {
        LayoutUnit nonContentHeight = innerTextBox->borderAndPaddingHeight() + innerTextBox->marginHeight();
        logicalHeight = computeControlHeight(innerTextBox->lineHeight(true, HorizontalLine, PositionOfInteriorLineBoxes), nonContentHeight) + borderAndPaddingHeight();

        // We are able to have a horizontal scrollbar if the overflow style is scroll, or if its auto and there's no word wrap.
        if (style()->overflowX() == OSCROLL ||  (style()->overflowX() == OAUTO && innerText->renderer()->style()->overflowWrap() == NormalOverflowWrap))
            logicalHeight += scrollbarThickness();
    }

    RenderBox::computeLogicalHeight(logicalHeight, logicalTop, computedValues);
}

void RenderTextControl::hitInnerTextElement(HitTestResult& result, const LayoutPoint& pointInContainer, const LayoutPoint& accumulatedOffset)
{
    HTMLElement* innerText = innerTextElement();
    if (!innerText->renderer())
        return;

    LayoutPoint adjustedLocation = accumulatedOffset + location();
    LayoutPoint localPoint = pointInContainer - toLayoutSize(adjustedLocation + innerText->renderBox()->location());
    if (hasOverflowClip())
        localPoint += scrolledContentOffset();
    result.setInnerNode(innerText);
    result.setInnerNonSharedNode(innerText);
    result.setLocalPoint(localPoint);
}

static const char* fontFamiliesWithInvalidCharWidth[] = {
    "American Typewriter",
    "Arial Hebrew",
    "Chalkboard",
    "Cochin",
    "Corsiva Hebrew",
    "Courier",
    "Euphemia UCAS",
    "Geneva",
    "Gill Sans",
    "Hei",
    "Helvetica",
    "Hoefler Text",
    "InaiMathi",
    "Kai",
    "Lucida Grande",
    "Marker Felt",
    "Monaco",
    "Mshtakan",
    "New Peninim MT",
    "Osaka",
    "Raanana",
    "STHeiti",
    "Symbol",
    "Times",
    "Apple Braille",
    "Apple LiGothic",
    "Apple LiSung",
    "Apple Symbols",
    "AppleGothic",
    "AppleMyungjo",
    "#GungSeo",
    "#HeadLineA",
    "#PCMyungjo",
    "#PilGi",
};

// For font families where any of the fonts don't have a valid entry in the OS/2 table
// for avgCharWidth, fallback to the legacy webkit behavior of getting the avgCharWidth
// from the width of a '0'. This only seems to apply to a fixed number of Mac fonts,
// but, in order to get similar rendering across platforms, we do this check for
// all platforms.
bool RenderTextControl::hasValidAvgCharWidth(AtomicString family)
{
    static HashSet<AtomicString>* fontFamiliesWithInvalidCharWidthMap = 0;

    if (family.isEmpty())
        return false;

    if (!fontFamiliesWithInvalidCharWidthMap) {
        fontFamiliesWithInvalidCharWidthMap = new HashSet<AtomicString>;

        for (size_t i = 0; i < WTF_ARRAY_LENGTH(fontFamiliesWithInvalidCharWidth); ++i)
            fontFamiliesWithInvalidCharWidthMap->add(AtomicString(fontFamiliesWithInvalidCharWidth[i]));
    }

    return !fontFamiliesWithInvalidCharWidthMap->contains(family);
}

float RenderTextControl::getAvgCharWidth(AtomicString family)
{
    if (hasValidAvgCharWidth(family))
        return roundf(style()->font().primaryFont()->avgCharWidth());

    const UChar ch = '0';
    const String str = String(&ch, 1);
    const Font& font = style()->font();
    TextRun textRun = constructTextRun(this, font, str, style(), TextRun::AllowTrailingExpansion);
    textRun.disableRoundingHacks();
    return font.width(textRun);
}

float RenderTextControl::scaleEmToUnits(int x) const
{
    // This matches the unitsPerEm value for MS Shell Dlg and Courier New from the "head" font table.
    float unitsPerEm = 2048.0f;
    return roundf(style()->font().size() * x / unitsPerEm);
}

void RenderTextControl::computeIntrinsicLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const
{
    // Use average character width. Matches IE.
    AtomicString family = style()->font().family().family();
    maxLogicalWidth = preferredContentWidth(const_cast<RenderTextControl*>(this)->getAvgCharWidth(family));
    if (RenderBox* innerTextRenderBox = innerTextElement()->renderBox())
        maxLogicalWidth += innerTextRenderBox->paddingLeft() + innerTextRenderBox->paddingRight();
    if (!style()->width().isPercent())
        minLogicalWidth = maxLogicalWidth;
}

void RenderTextControl::computePreferredLogicalWidths()
{
    ASSERT(preferredLogicalWidthsDirty());

    m_minPreferredLogicalWidth = 0;
    m_maxPreferredLogicalWidth = 0;

    if (style()->width().isFixed() && style()->width().value() >= 0)
        m_minPreferredLogicalWidth = m_maxPreferredLogicalWidth = adjustContentBoxLogicalWidthForBoxSizing(style()->width().value());
    else
        computeIntrinsicLogicalWidths(m_minPreferredLogicalWidth, m_maxPreferredLogicalWidth);

    if (style()->minWidth().isFixed() && style()->minWidth().value() > 0) {
        m_maxPreferredLogicalWidth = max(m_maxPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(style()->minWidth().value()));
        m_minPreferredLogicalWidth = max(m_minPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(style()->minWidth().value()));
    }

    if (style()->maxWidth().isFixed()) {
        m_maxPreferredLogicalWidth = min(m_maxPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(style()->maxWidth().value()));
        m_minPreferredLogicalWidth = min(m_minPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(style()->maxWidth().value()));
    }

    LayoutUnit toAdd = borderAndPaddingWidth();

    m_minPreferredLogicalWidth += toAdd;
    m_maxPreferredLogicalWidth += toAdd;

    setPreferredLogicalWidthsDirty(false);
}

void RenderTextControl::addFocusRingRects(Vector<IntRect>& rects, const LayoutPoint& additionalOffset, const RenderLayerModelObject*)
{
    if (!size().isEmpty())
        rects.append(pixelSnappedIntRect(additionalOffset, size()));
}

RenderObject* RenderTextControl::layoutSpecialExcludedChild(bool relayoutChildren)
{
    HTMLElement* placeholder = toHTMLTextFormControlElement(node())->placeholderElement();
    RenderObject* placeholderRenderer = placeholder ? placeholder->renderer() : 0;
    if (!placeholderRenderer)
        return 0;
    if (relayoutChildren) {
        // The markParents arguments should be false because this function is
        // called from layout() of the parent and the placeholder layout doesn't
        // affect the parent layout.
        placeholderRenderer->setChildNeedsLayout(true, MarkOnlyThis);
    }
    return placeholderRenderer;
}

bool RenderTextControl::canBeReplacedWithInlineRunIn() const
{
    return false;
}

} // namespace WebCore
