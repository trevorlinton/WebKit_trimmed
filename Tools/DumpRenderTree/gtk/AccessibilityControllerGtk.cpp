/*
 * Copyright (C) 2008, 2009, 2010 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009 Jan Michael Alonzo
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AccessibilityController.h"

#include "AccessibilityCallbacks.h"
#include "AccessibilityUIElement.h"
#include "DumpRenderTree.h"
#include "WebCoreSupport/DumpRenderTreeSupportGtk.h"

#include <atk/atk.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <wtf/gobject/GOwnPtr.h>

static AtkObject* childElementById(AtkObject* parent, const char* id)
{
    if (!ATK_IS_OBJECT(parent))
        return 0;

    AtkAttributeSet* attributeSet = atk_object_get_attributes(parent);
    for (GSList* attributes = attributeSet; attributes; attributes = attributes->next) {
        AtkAttribute* attribute = static_cast<AtkAttribute*>(attributes->data);
        if (!strcmp(attribute->name, "html-id")) {
            if (!strcmp(attribute->value, id))
                return parent;
            break;
        }
    }

    int childCount = atk_object_get_n_accessible_children(parent);
    for (int i = 0; i < childCount; i++) {
        AtkObject* result = childElementById(atk_object_ref_accessible_child(parent, i), id);
        if (ATK_IS_OBJECT(result))
            return result;
    }

    return 0;
}

AccessibilityUIElement AccessibilityController::focusedElement()
{
    AtkObject* accessible =  DumpRenderTreeSupportGtk::getFocusedAccessibleElement(mainFrame);
    if (!accessible)
        return 0;

    return AccessibilityUIElement(accessible);
}

AccessibilityUIElement AccessibilityController::rootElement()
{
    AtkObject* accessible = DumpRenderTreeSupportGtk::getRootAccessibleElement(mainFrame);
    if (!accessible)
        return 0;

    return AccessibilityUIElement(accessible);
}

AccessibilityUIElement AccessibilityController::accessibleElementById(JSStringRef id)
{
    AtkObject* root = DumpRenderTreeSupportGtk::getRootAccessibleElement(mainFrame);
    if (!root)
        return 0;

    size_t bufferSize = JSStringGetMaximumUTF8CStringSize(id);
    GOwnPtr<gchar> idBuffer(static_cast<gchar*>(g_malloc(bufferSize)));
    JSStringGetUTF8CString(id, idBuffer.get(), bufferSize);

    AtkObject* result = childElementById(root, idBuffer.get());
    if (ATK_IS_OBJECT(result))
        return AccessibilityUIElement(result);

    return 0;

}
