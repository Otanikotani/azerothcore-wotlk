/*
 * Copyright (C) 2016+     AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-GPL2
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

#ifndef _REFERENCE_H
#define _REFERENCE_H

#include "Dynamic/LinkedList.h"
#include "Errors.h" // for ASSERT

//=====================================================

template <class TO, class FROM> class Reference : public LinkedListElement
{
private:
    TO* iRefTo;
    FROM* iRefFrom;
protected:
    // Tell our refTo (target) object that we have a link
    virtual void targetObjectBuildLink() = 0;

    // Tell our refTo (taget) object, that the link is cut
    virtual void targetObjectDestroyLink() = 0;

    // Tell our refFrom (source) object, that the link is cut (Target destroyed)
    virtual void sourceObjectDestroyLink() = 0;
public:
    Reference() { iRefTo = nullptr; iRefFrom = nullptr; }
    virtual ~Reference() = default;

    // Create new link
    void link(TO* toObj, FROM* fromObj)
    {
        ASSERT(fromObj);                                // fromObj MUST not be nullptr
        if (isValid())
            unlink();
        if (toObj != nullptr)
        {
            iRefTo = toObj;
            iRefFrom = fromObj;
            targetObjectBuildLink();
        }
    }

    // We don't need the reference anymore. Call comes from the refFrom object
    // Tell our refTo object, that the link is cut
    void unlink()
    {
        targetObjectDestroyLink();
        delink();
        iRefTo = nullptr;
        iRefFrom = nullptr;
    }

    // Link is invalid due to destruction of referenced target object. Call comes from the refTo object
    // Tell our refFrom object, that the link is cut
    void invalidate()                                   // the iRefFrom MUST remain!!
    {
        sourceObjectDestroyLink();
        delink();
        iRefTo = nullptr;
    }

    [[nodiscard]] bool isValid() const                                // Only check the iRefTo
    {
        return iRefTo != nullptr;
    }

    Reference<TO, FROM>*        next()       { return ((Reference<TO, FROM>*) LinkedListElement::next()); }
    [[nodiscard]] Reference<TO, FROM> const* next() const { return ((Reference<TO, FROM> const*) LinkedListElement::next()); }
    Reference<TO, FROM>*        prev()       { return ((Reference<TO, FROM>*) LinkedListElement::prev()); }
    [[nodiscard]] Reference<TO, FROM> const* prev() const { return ((Reference<TO, FROM> const*) LinkedListElement::prev()); }

    Reference<TO, FROM>*        nocheck_next()       { return ((Reference<TO, FROM>*) LinkedListElement::nocheck_next()); }
    [[nodiscard]] Reference<TO, FROM> const* nocheck_next() const { return ((Reference<TO, FROM> const*) LinkedListElement::nocheck_next()); }
    Reference<TO, FROM>*        nocheck_prev()       { return ((Reference<TO, FROM>*) LinkedListElement::nocheck_prev()); }
    [[nodiscard]] Reference<TO, FROM> const* nocheck_prev() const { return ((Reference<TO, FROM> const*) LinkedListElement::nocheck_prev()); }

    TO* operator ->() const { return iRefTo; }
    [[nodiscard]] TO* getTarget() const { return iRefTo; }

    [[nodiscard]] FROM* GetSource() const { return iRefFrom; }
};

//=====================================================
#endif
