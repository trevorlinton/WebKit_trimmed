/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PageDebuggerAgent_h
#define PageDebuggerAgent_h

#if ENABLE(JAVASCRIPT_DEBUGGER) && ENABLE(INSPECTOR)
#include "InspectorDebuggerAgent.h"
#include "PageScriptDebugServer.h"

namespace WebCore {

class InspectorOverlay;
class InspectorPageAgent;
class Page;
class PageScriptDebugServer;

class PageDebuggerAgent : public InspectorDebuggerAgent {
    WTF_MAKE_NONCOPYABLE(PageDebuggerAgent);
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<PageDebuggerAgent> create(InstrumentingAgents*, InspectorCompositeState*, InspectorPageAgent*, InjectedScriptManager*, InspectorOverlay*);
    virtual ~PageDebuggerAgent();

    void didClearMainFrameWindowObject();

protected:
    virtual void enable();
    virtual void disable();

    virtual PageScriptDebugServer& scriptDebugServer();

private:
    virtual void startListeningScriptDebugServer();
    virtual void stopListeningScriptDebugServer();
    virtual void muteConsole();
    virtual void unmuteConsole();

    virtual InjectedScript injectedScriptForEval(ErrorString*, const int* executionContextId);
    virtual void setOverlayMessage(ErrorString*, const String*);

    PageDebuggerAgent(InstrumentingAgents*, InspectorCompositeState*, InspectorPageAgent*, InjectedScriptManager*, InspectorOverlay*);
    InspectorPageAgent* m_pageAgent;
    InspectorOverlay* m_overlay;
};

} // namespace WebCore

#endif // ENABLE(JAVASCRIPT_DEBUGGER) && ENABLE(INSPECTOR)

#endif // !defined(PageDebuggerAgent_h)
