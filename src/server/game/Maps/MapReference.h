/*
 * Copyright (C) 2016+     AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-GPL2
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

#ifndef _MAPREFERENCE_H
#define _MAPREFERENCE_H

#include "Map.h"
#include "Reference.h"

class MapReference : public Reference<Map, Player>
{
protected:
    void targetObjectBuildLink() override
    {
        // called from link()
        getTarget()->m_mapRefManager.insertFirst(this);
        getTarget()->m_mapRefManager.incSize();
    }
    void targetObjectDestroyLink() override
    {
        // called from unlink()
        if (isValid()) getTarget()->m_mapRefManager.decSize();
    }
    void sourceObjectDestroyLink() override
    {
        // called from invalidate()
        getTarget()->m_mapRefManager.decSize();
    }
public:
    MapReference() : Reference<Map, Player>() {}
    ~MapReference() override { unlink(); }
    MapReference* next() { return (MapReference*)Reference<Map, Player>::next(); }
    [[nodiscard]] MapReference const* next() const { return (MapReference const*)Reference<Map, Player>::next(); }
    MapReference* nockeck_prev() { return (MapReference*)Reference<Map, Player>::nocheck_prev(); }
    [[nodiscard]] MapReference const* nocheck_prev() const { return (MapReference const*)Reference<Map, Player>::nocheck_prev(); }
};
#endif
