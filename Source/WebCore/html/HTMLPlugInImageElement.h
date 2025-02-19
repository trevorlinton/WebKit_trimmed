/*
 * Copyright (C) 2008, 2009, 2011, 2012 Apple Inc. All rights reserved.
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

#ifndef HTMLPlugInImageElement_h
#define HTMLPlugInImageElement_h

#include "HTMLPlugInElement.h"

#include "RenderStyle.h"
#include <wtf/OwnPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class HTMLImageLoader;
class FrameLoader;
class Image;
class MouseEvent;
class Widget;

enum PluginCreationOption {
    CreateAnyWidgetType,
    CreateOnlyNonNetscapePlugins,
};

enum PreferPlugInsForImagesOption {
    ShouldPreferPlugInsForImages,
    ShouldNotPreferPlugInsForImages
};

// Base class for HTMLObjectElement and HTMLEmbedElement
class HTMLPlugInImageElement : public HTMLPlugInElement {
public:
    virtual ~HTMLPlugInImageElement();

    RenderEmbeddedObject* renderEmbeddedObject() const;

    virtual void setDisplayState(DisplayState) OVERRIDE;

    virtual void updateWidget(PluginCreationOption) = 0;

    const String& serviceType() const { return m_serviceType; }
    const String& url() const { return m_url; }
    bool shouldPreferPlugInsForImages() const { return m_shouldPreferPlugInsForImages; }

    // Public for FrameView::addWidgetToUpdate()
    bool needsWidgetUpdate() const { return m_needsWidgetUpdate; }
    void setNeedsWidgetUpdate(bool needsWidgetUpdate) { m_needsWidgetUpdate = needsWidgetUpdate; }

    void userDidClickSnapshot(PassRefPtr<MouseEvent>);
    void updateSnapshotInfo();
    Image* snapshotImage() const { return m_snapshotImage.get(); }

    // Plug-in URL might not be the same as url() with overriding parameters.
    void subframeLoaderWillCreatePlugIn(const KURL& plugInURL);
    void subframeLoaderDidCreatePlugIn(const Widget*);

protected:
    HTMLPlugInImageElement(const QualifiedName& tagName, Document*, bool createdByParser, PreferPlugInsForImagesOption);

    bool isImageType();

    OwnPtr<HTMLImageLoader> m_imageLoader;
    String m_serviceType;
    String m_url;
    
    static void updateWidgetCallback(Node*, unsigned = 0);
    virtual void attach();
    virtual void detach();

    bool allowedToLoadFrameURL(const String& url);
    bool wouldLoadAsNetscapePlugin(const String& url, const String& serviceType);

    virtual void didMoveToNewDocument(Document* oldDocument) OVERRIDE;
    
    virtual void documentWillSuspendForPageCache() OVERRIDE;
    virtual void documentDidResumeFromPageCache() OVERRIDE;

    virtual PassRefPtr<RenderStyle> customStyleForRenderer() OVERRIDE;

private:
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
    virtual bool willRecalcStyle(StyleChange);

    void didAddUserAgentShadowRoot(ShadowRoot*) OVERRIDE;

    virtual void finishParsingChildren();

    void updateWidgetIfNecessary();
    virtual bool useFallbackContent() const { return false; }

    virtual void updateSnapshot(PassRefPtr<Image>) OVERRIDE;
    virtual void dispatchPendingMouseClick() OVERRIDE;
    void simulatedMouseClickTimerFired(DeferrableOneShotTimer<HTMLPlugInImageElement>*);

    void swapRendererTimerFired(Timer<HTMLPlugInImageElement>*);

    virtual bool isPlugInImageElement() const OVERRIDE { return true; }

    bool m_needsWidgetUpdate;
    bool m_shouldPreferPlugInsForImages;
    bool m_needsDocumentActivationCallbacks;
    RefPtr<RenderStyle> m_customStyleForPageCache;
    RefPtr<MouseEvent> m_pendingClickEventFromSnapshot;
    DeferrableOneShotTimer<HTMLPlugInImageElement> m_simulatedMouseClickTimer;
    Timer<HTMLPlugInImageElement> m_swapRendererTimer;
    RefPtr<Image> m_snapshotImage;
};

} // namespace WebCore

#endif // HTMLPlugInImageElement_h
