/*
 * Copyright (C) 2012 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RenderMultiColumnSet.h"

#include "PaintInfo.h"
#include "RenderLayer.h"
#include "RenderMultiColumnBlock.h"
#include "RenderMultiColumnFlowThread.h"

using namespace std;

namespace WebCore {

RenderMultiColumnSet::RenderMultiColumnSet(RenderFlowThread* flowThread)
    : RenderRegionSet(0, flowThread)
    , m_computedColumnCount(1)
    , m_computedColumnWidth(0)
    , m_computedColumnHeight(0)
    , m_requiresBalancing(false)
    , m_minimumColumnHeight(0)
    , m_forcedBreaksCount(0)
    , m_maximumDistanceBetweenForcedBreaks(0)
    , m_forcedBreakOffset(0)
{
}

RenderMultiColumnSet* RenderMultiColumnSet::createAnonymous(RenderFlowThread* flowThread)
{
    Document* document = flowThread->document();
    RenderMultiColumnSet* renderer = new (document->renderArena()) RenderMultiColumnSet(flowThread);
    renderer->setDocumentForAnonymous(document);
    return renderer;
}

LayoutUnit RenderMultiColumnSet::pageLogicalTopForOffset(LayoutUnit offset) const
{
    LayoutUnit portionLogicalTop = (isHorizontalWritingMode() ? flowThreadPortionRect().y() : flowThreadPortionRect().x());
    unsigned columnIndex = (offset - portionLogicalTop) / computedColumnHeight();
    return portionLogicalTop + columnIndex * computedColumnHeight();
}

void RenderMultiColumnSet::updateLogicalWidth()
{
    // Our logical width starts off matching the column block itself.
    // This width will be fixed up after the flow thread lays out once it is determined exactly how many
    // columns we ended up holding.
    // FIXME: When we add regions support, we'll start it off at the width of the multi-column
    // block in that particular region.
    setLogicalWidth(parentBox()->contentLogicalWidth());
    
    RenderMultiColumnBlock* parentBlock = toRenderMultiColumnBlock(parent());
    setComputedColumnWidthAndCount(parentBlock->columnWidth(), parentBlock->columnCount()); // FIXME: This will eventually vary if we are contained inside regions.
}

void RenderMultiColumnSet::updateLogicalHeight()
{
    // FIXME: This is the only class that overrides updateLogicalHeight. If we didn't have to set computedColumnHeight,
    // we could remove this and make updateLogicalHeight non-virtual. https://bugs.webkit.org/show_bug.cgi?id=96804
    // Make sure our column height is up to date.
    LogicalExtentComputedValues computedValues;
    computeLogicalHeight(0, 0, computedValues);
    setComputedColumnHeight(computedValues.m_extent); // FIXME: Once we make more than one column set, this will become variable.
    
    // Our logical height is always just the height of our columns.
    setLogicalHeight(computedColumnHeight());
}

void RenderMultiColumnSet::computeLogicalHeight(LayoutUnit, LayoutUnit, LogicalExtentComputedValues& computedValues) const
{
    RenderMultiColumnBlock* parentBlock = toRenderMultiColumnBlock(parent());
    computedValues.m_extent = parentBlock->columnHeight();
}

LayoutUnit RenderMultiColumnSet::columnGap() const
{
    // FIXME: Eventually we will cache the column gap when the widths of columns start varying, but for now we just
    // go to the parent block to get the gap.
    RenderMultiColumnBlock* parentBlock = toRenderMultiColumnBlock(parent());
    if (parentBlock->style()->hasNormalColumnGap())
        return parentBlock->style()->fontDescription().computedPixelSize(); // "1em" is recommended as the normal gap setting. Matches <p> margins.
    return parentBlock->style()->columnGap();
}

unsigned RenderMultiColumnSet::columnCount() const
{
    if (!computedColumnHeight())
        return 0;
    
    // Our portion rect determines our column count. We have as many columns as needed to fit all the content.
    LayoutUnit logicalHeightInColumns = flowThread()->isHorizontalWritingMode() ? flowThreadPortionRect().height() : flowThreadPortionRect().width();
    return ceil(static_cast<float>(logicalHeightInColumns) / computedColumnHeight());
}

LayoutRect RenderMultiColumnSet::columnRectAt(unsigned index) const
{
    LayoutUnit colLogicalWidth = computedColumnWidth();
    LayoutUnit colLogicalHeight = computedColumnHeight();
    LayoutUnit colLogicalTop = borderBefore() + paddingBefore();
    LayoutUnit colLogicalLeft = borderAndPaddingLogicalLeft();
    LayoutUnit colGap = columnGap();
    if (style()->isLeftToRightDirection())
        colLogicalLeft += index * (colLogicalWidth + colGap);
    else
        colLogicalLeft += contentLogicalWidth() - colLogicalWidth - index * (colLogicalWidth + colGap);

    if (isHorizontalWritingMode())
        return LayoutRect(colLogicalLeft, colLogicalTop, colLogicalWidth, colLogicalHeight);
    return LayoutRect(colLogicalTop, colLogicalLeft, colLogicalHeight, colLogicalWidth);
}

unsigned RenderMultiColumnSet::columnIndexAtOffset(LayoutUnit offset) const
{
    LayoutRect portionRect(flowThreadPortionRect());
    LayoutUnit flowThreadLogicalTop = isHorizontalWritingMode() ? portionRect.y() : portionRect.x();
    LayoutUnit flowThreadLogicalBottom = isHorizontalWritingMode() ? portionRect.maxY() : portionRect.maxX();
    
    // Handle the offset being out of range.
    if (offset < flowThreadLogicalTop)
        return 0;
    if (offset >= flowThreadLogicalBottom)
        return columnCount() - 1;
    
    // Just divide by the column height to determine the correct column.
    return static_cast<float>(offset - flowThreadLogicalTop) / computedColumnHeight();
}

LayoutRect RenderMultiColumnSet::flowThreadPortionRectAt(unsigned index) const
{
    LayoutRect portionRect = flowThreadPortionRect();
    if (isHorizontalWritingMode())
        portionRect = LayoutRect(portionRect.x(), portionRect.y() + index * computedColumnHeight(), portionRect.width(), computedColumnHeight());
    else
        portionRect = LayoutRect(portionRect.x() + index * computedColumnHeight(), portionRect.y(), computedColumnHeight(), portionRect.height());
    return portionRect;
}

LayoutRect RenderMultiColumnSet::flowThreadPortionOverflowRect(const LayoutRect& portionRect, unsigned index, unsigned colCount, LayoutUnit colGap) const
{
    // This function determines the portion of the flow thread that paints for the column. Along the inline axis, columns are
    // unclipped at outside edges (i.e., the first and last column in the set), and they clip to half the column
    // gap along interior edges.
    //
    // In the block direction, we will not clip overflow out of the top of the first column, or out of the bottom of
    // the last column. This applies only to the true first column and last column across all column sets.
    //
    // FIXME: Eventually we will know overflow on a per-column basis, but we can't do this until we have a painting
    // mode that understands not to paint contents from a previous column in the overflow area of a following column.
    // This problem applies to regions and pages as well and is not unique to columns.
    bool isFirstColumn = !index;
    bool isLastColumn = index == colCount - 1;
    bool isLeftmostColumn = style()->isLeftToRightDirection() ? isFirstColumn : isLastColumn;
    bool isRightmostColumn = style()->isLeftToRightDirection() ? isLastColumn : isFirstColumn;
    LayoutRect overflowRect(portionRect);
    if (isHorizontalWritingMode()) {
        if (isLeftmostColumn) {
            // Shift to the logical left overflow of the flow thread to make sure it's all covered.
            overflowRect.shiftXEdgeTo(min(flowThread()->visualOverflowRect().x(), portionRect.x()));
        } else {
            // Expand into half of the logical left column gap.
            overflowRect.shiftXEdgeTo(portionRect.x() - colGap / 2);
        }
        if (isRightmostColumn) {
            // Shift to the logical right overflow of the flow thread to ensure content can spill out of the column.
            overflowRect.shiftMaxXEdgeTo(max(flowThread()->visualOverflowRect().maxX(), portionRect.maxX()));
        } else {
            // Expand into half of the logical right column gap.
            overflowRect.shiftMaxXEdgeTo(portionRect.maxX() + colGap / 2);
        }
    } else {
        if (isLeftmostColumn) {
            // Shift to the logical left overflow of the flow thread to make sure it's all covered.
            overflowRect.shiftYEdgeTo(min(flowThread()->visualOverflowRect().y(), portionRect.y()));
        } else {
            // Expand into half of the logical left column gap.
            overflowRect.shiftYEdgeTo(portionRect.y() - colGap / 2);
        }
        if (isRightmostColumn) {
            // Shift to the logical right overflow of the flow thread to ensure content can spill out of the column.
            overflowRect.shiftMaxYEdgeTo(max(flowThread()->visualOverflowRect().maxY(), portionRect.maxY()));
        } else {
            // Expand into half of the logical right column gap.
            overflowRect.shiftMaxYEdgeTo(portionRect.maxY() + colGap / 2);
        }
    }
    return overflowRectForFlowThreadPortion(overflowRect, isFirstRegion() && isFirstColumn, isLastRegion() && isLastColumn);
}

void RenderMultiColumnSet::setFlowThreadPortionRect(const LayoutRect& rect)
{
    RenderRegion::setFlowThreadPortionRect(rect);
    
    // Mutate the dimensions of the column set once our flow portion is set if the flow portion has more columns
    // than can fit inside our current dimensions.
    unsigned colCount = columnCount();
    if (!colCount)
        return;
    
    LayoutUnit colGap = columnGap();
    LayoutUnit minimumContentLogicalWidth = colCount * computedColumnWidth() + (colCount - 1) * colGap;
    LayoutUnit currentContentLogicalWidth = contentLogicalWidth();
    LayoutUnit delta = max(LayoutUnit(), minimumContentLogicalWidth - currentContentLogicalWidth);
    if (!delta)
        return;

    // Increase our logical width by the delta.
    setLogicalWidth(logicalWidth() + delta);
    
    // Shift our position left by the delta if we are RTL.
    if (!style()->isLeftToRightDirection())
        setLogicalLeft(logicalLeft() - delta);
}

void RenderMultiColumnSet::paintObject(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (style()->visibility() != VISIBLE)
        return;

    RenderBlock::paintObject(paintInfo, paintOffset);

    // FIXME: Right now we're only painting in the foreground phase.
    // Columns should technically respect phases and allow for background/float/foreground overlap etc., just like
    // RenderBlocks do. Note this is a pretty minor issue, since the old column implementation clipped columns
    // anyway, thus making it impossible for them to overlap one another. It's also really unlikely that the columns
    // would overlap another block.
    if (!m_flowThread || !isValid() || (paintInfo.phase != PaintPhaseForeground && paintInfo.phase != PaintPhaseSelection))
        return;

    paintColumnRules(paintInfo, paintOffset);
}

void RenderMultiColumnSet::paintColumnRules(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (paintInfo.context->paintingDisabled())
        return;

    RenderStyle* blockStyle = toRenderMultiColumnBlock(parent())->style();
    const Color& ruleColor = blockStyle->visitedDependentColor(CSSPropertyWebkitColumnRuleColor);
    bool ruleTransparent = blockStyle->columnRuleIsTransparent();
    EBorderStyle ruleStyle = blockStyle->columnRuleStyle();
    LayoutUnit ruleThickness = blockStyle->columnRuleWidth();
    LayoutUnit colGap = columnGap();
    bool renderRule = ruleStyle > BHIDDEN && !ruleTransparent;
    if (!renderRule)
        return;

    unsigned colCount = columnCount();
    if (colCount <= 1)
        return;

    bool antialias = shouldAntialiasLines(paintInfo.context);

    bool leftToRight = style()->isLeftToRightDirection();
    LayoutUnit currLogicalLeftOffset = leftToRight ? LayoutUnit() : contentLogicalWidth();
    LayoutUnit ruleAdd = borderAndPaddingLogicalLeft();
    LayoutUnit ruleLogicalLeft = leftToRight ? LayoutUnit() : contentLogicalWidth();
    LayoutUnit inlineDirectionSize = computedColumnWidth();
    BoxSide boxSide = isHorizontalWritingMode()
        ? leftToRight ? BSLeft : BSRight
        : leftToRight ? BSTop : BSBottom;

    for (unsigned i = 0; i < colCount; i++) {
        // Move to the next position.
        if (leftToRight) {
            ruleLogicalLeft += inlineDirectionSize + colGap / 2;
            currLogicalLeftOffset += inlineDirectionSize + colGap;
        } else {
            ruleLogicalLeft -= (inlineDirectionSize + colGap / 2);
            currLogicalLeftOffset -= (inlineDirectionSize + colGap);
        }

        // Now paint the column rule.
        if (i < colCount - 1) {
            LayoutUnit ruleLeft = isHorizontalWritingMode() ? paintOffset.x() + ruleLogicalLeft - ruleThickness / 2 + ruleAdd : paintOffset.x() + borderLeft() + paddingLeft();
            LayoutUnit ruleRight = isHorizontalWritingMode() ? ruleLeft + ruleThickness : ruleLeft + contentWidth();
            LayoutUnit ruleTop = isHorizontalWritingMode() ? paintOffset.y() + borderTop() + paddingTop() : paintOffset.y() + ruleLogicalLeft - ruleThickness / 2 + ruleAdd;
            LayoutUnit ruleBottom = isHorizontalWritingMode() ? ruleTop + contentHeight() : ruleTop + ruleThickness;
            IntRect pixelSnappedRuleRect = pixelSnappedIntRectFromEdges(ruleLeft, ruleTop, ruleRight, ruleBottom);
            drawLineForBoxSide(paintInfo.context, pixelSnappedRuleRect.x(), pixelSnappedRuleRect.y(), pixelSnappedRuleRect.maxX(), pixelSnappedRuleRect.maxY(), boxSide, ruleColor, ruleStyle, 0, 0, antialias);
        }
        
        ruleLogicalLeft = currLogicalLeftOffset;
    }
}

void RenderMultiColumnSet::repaintFlowThreadContent(const LayoutRect& repaintRect, bool immediate) const
{
    // Figure out the start and end columns and only check within that range so that we don't walk the
    // entire column set. Put the repaint rect into flow thread coordinates by flipping it first.
    LayoutRect flowThreadRepaintRect(repaintRect);
    flowThread()->flipForWritingMode(flowThreadRepaintRect);
    
    // Now we can compare this rect with the flow thread portions owned by each column. First let's
    // just see if the repaint rect intersects our flow thread portion at all.
    LayoutRect clippedRect(flowThreadRepaintRect);
    clippedRect.intersect(RenderRegion::flowThreadPortionOverflowRect());
    if (clippedRect.isEmpty())
        return;
    
    // Now we know we intersect at least one column. Let's figure out the logical top and logical
    // bottom of the area we're repainting.
    LayoutUnit repaintLogicalTop = isHorizontalWritingMode() ? flowThreadRepaintRect.y() : flowThreadRepaintRect.x();
    LayoutUnit repaintLogicalBottom = (isHorizontalWritingMode() ? flowThreadRepaintRect.maxY() : flowThreadRepaintRect.maxX()) - 1;
    
    unsigned startColumn = columnIndexAtOffset(repaintLogicalTop);
    unsigned endColumn = columnIndexAtOffset(repaintLogicalBottom);
    
    LayoutUnit colGap = columnGap();
    unsigned colCount = columnCount();
    for (unsigned i = startColumn; i <= endColumn; i++) {
        LayoutRect colRect = columnRectAt(i);
        
        // Get the portion of the flow thread that corresponds to this column.
        LayoutRect flowThreadPortion = flowThreadPortionRectAt(i);
        
        // Now get the overflow rect that corresponds to the column.
        LayoutRect flowThreadOverflowPortion = flowThreadPortionOverflowRect(flowThreadPortion, i, colCount, colGap);

        // Do a repaint for this specific column.
        repaintFlowThreadContentRectangle(repaintRect, immediate, flowThreadPortion, flowThreadOverflowPortion, colRect.location());
    }
}

void RenderMultiColumnSet::collectLayerFragments(LayerFragments& fragments, const LayoutRect& layerBoundingBox, const LayoutRect& dirtyRect)
{
    // Put the layer bounds into flow thread-local coordinates by flipping it first.
    LayoutRect layerBoundsInFlowThread(layerBoundingBox);
    flowThread()->flipForWritingMode(layerBoundsInFlowThread);

    // Do the same for the dirty rect.
    LayoutRect dirtyRectInFlowThread(dirtyRect);
    flowThread()->flipForWritingMode(dirtyRectInFlowThread);

    // Now we can compare with the flow thread portions owned by each column. First let's
    // see if the rect intersects our flow thread portion at all.
    LayoutRect clippedRect(layerBoundsInFlowThread);
    clippedRect.intersect(RenderRegion::flowThreadPortionOverflowRect());
    if (clippedRect.isEmpty())
        return;
    
    // Now we know we intersect at least one column. Let's figure out the logical top and logical
    // bottom of the area we're checking.
    LayoutUnit layerLogicalTop = isHorizontalWritingMode() ? layerBoundsInFlowThread.y() : layerBoundsInFlowThread.x();
    LayoutUnit layerLogicalBottom = (isHorizontalWritingMode() ? layerBoundsInFlowThread.maxY() : layerBoundsInFlowThread.maxX()) - 1;
    
    // Figure out the start and end columns and only check within that range so that we don't walk the
    // entire column set.
    unsigned startColumn = columnIndexAtOffset(layerLogicalTop);
    unsigned endColumn = columnIndexAtOffset(layerLogicalBottom);
    
    LayoutUnit colLogicalWidth = computedColumnWidth();
    LayoutUnit colGap = columnGap();
    unsigned colCount = columnCount();
    
    for (unsigned i = startColumn; i <= endColumn; i++) {
        // Get the portion of the flow thread that corresponds to this column.
        LayoutRect flowThreadPortion = flowThreadPortionRectAt(i);
        
        // Now get the overflow rect that corresponds to the column.
        LayoutRect flowThreadOverflowPortion = flowThreadPortionOverflowRect(flowThreadPortion, i, colCount, colGap);

        // In order to create a fragment we must intersect the portion painted by this column.
        LayoutRect clippedRect(layerBoundsInFlowThread);
        clippedRect.intersect(flowThreadOverflowPortion);
        if (clippedRect.isEmpty())
            continue;
        
        // We also need to intersect the dirty rect. We have to apply a translation and shift based off
        // our column index.
        LayoutPoint translationOffset;
        LayoutUnit inlineOffset = i * (colLogicalWidth + colGap);
        if (!style()->isLeftToRightDirection())
            inlineOffset = -inlineOffset;
        translationOffset.setX(inlineOffset);
        LayoutUnit blockOffset = isHorizontalWritingMode() ? -flowThreadPortion.y() : -flowThreadPortion.x();
        if (isFlippedBlocksWritingMode(style()->writingMode()))
            blockOffset = -blockOffset;
        translationOffset.setY(blockOffset);
        if (!isHorizontalWritingMode())
            translationOffset = translationOffset.transposedPoint();
        // FIXME: The translation needs to include the multicolumn set's content offset within the
        // multicolumn block as well. This won't be an issue until we start creating multiple multicolumn sets.

        // Shift the dirty rect to be in flow thread coordinates with this translation applied.
        LayoutRect translatedDirtyRect(dirtyRectInFlowThread);
        translatedDirtyRect.moveBy(-translationOffset);
        
        // See if we intersect the dirty rect.
        clippedRect = layerBoundsInFlowThread;
        clippedRect.intersect(translatedDirtyRect);
        if (clippedRect.isEmpty())
            continue;
        
        // Something does need to paint in this column. Make a fragment now and supply the physical translation
        // offset and the clip rect for the column with that offset applied.
        LayerFragment fragment;
        fragment.paginationOffset = translationOffset;
        
        LayoutRect flippedFlowThreadOverflowPortion(flowThreadOverflowPortion);
        flipForWritingMode(flippedFlowThreadOverflowPortion);
        fragment.paginationClip = flippedFlowThreadOverflowPortion;
        fragments.append(fragment);
    }
}

const char* RenderMultiColumnSet::renderName() const
{    
    return "RenderMultiColumnSet";
}

}
