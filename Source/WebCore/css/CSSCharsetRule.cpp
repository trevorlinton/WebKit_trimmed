/*
 * Copyright (C) 2006, 2008, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@macrules.ru)
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
 */

#include "config.h"
#include "CSSCharsetRule.h"

#include "WebCoreMemoryInstrumentation.h"

namespace WebCore {

CSSCharsetRule::CSSCharsetRule(CSSStyleSheet* parent, const String& encoding)
    : CSSRule(parent)
    , m_encoding(encoding)
{
}

String CSSCharsetRule::cssText() const
{
    return "@charset \"" + m_encoding + "\";";
}

void CSSCharsetRule::reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    MemoryClassInfo info(memoryObjectInfo, this, WebCoreMemoryTypes::CSS);
    CSSRule::reportMemoryUsage(memoryObjectInfo);
    info.addMember(m_encoding, "encoding");
}

} // namespace WebCore
