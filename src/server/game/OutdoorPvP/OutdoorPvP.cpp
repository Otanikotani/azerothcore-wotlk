/*
 * Copyright (C) 2016+     AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-GPL2
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "Map.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "OutdoorPvP.h"
#include "OutdoorPvPMgr.h"
#include "WorldPacket.h"

OPvPCapturePoint::OPvPCapturePoint(OutdoorPvP* pvp):
    m_capturePointGUID(0), m_capturePoint(nullptr), m_maxValue(0.0f), m_minValue(0.0f), m_maxSpeed(0),
    m_value(0), m_team(TEAM_NEUTRAL), m_OldState(OBJECTIVESTATE_NEUTRAL),
    m_State(OBJECTIVESTATE_NEUTRAL), m_neutralValuePct(0), m_PvP(pvp)
{ }

bool OPvPCapturePoint::HandlePlayerEnter(Player* player)
{
    if (m_capturePoint)
    {
        player->SendUpdateWorldState(m_capturePoint->GetGOInfo()->capturePoint.worldState1, 1);
        player->SendUpdateWorldState(m_capturePoint->GetGOInfo()->capturePoint.worldstate2, (uint32)ceil((m_value + m_maxValue) / (2 * m_maxValue) * 100.0f));
        player->SendUpdateWorldState(m_capturePoint->GetGOInfo()->capturePoint.worldstate3, m_neutralValuePct);
    }
    return m_activePlayers[player->GetTeamId()].insert(player->GetGUID()).second;
}

void OPvPCapturePoint::HandlePlayerLeave(Player* player)
{
    if (m_capturePoint)
        player->SendUpdateWorldState(m_capturePoint->GetGOInfo()->capturePoint.worldState1, 0);
    m_activePlayers[player->GetTeamId()].erase(player->GetGUID());
}

void OPvPCapturePoint::SendChangePhase()
{
    if (!m_capturePoint)
        return;

    // send this too, sometimes the slider disappears, dunno why :(
    SendUpdateWorldState(m_capturePoint->GetGOInfo()->capturePoint.worldState1, 1);
    // send these updates to only the ones in this objective
    SendUpdateWorldState(m_capturePoint->GetGOInfo()->capturePoint.worldstate2, (uint32)ceil((m_value + m_maxValue) / (2 * m_maxValue) * 100.0f));
    // send this too, sometimes it resets :S
    SendUpdateWorldState(m_capturePoint->GetGOInfo()->capturePoint.worldstate3, m_neutralValuePct);
}

void OPvPCapturePoint::AddGO(uint32 type, uint32 guid, uint32 entry)
{
    if (!entry)
    {
        const GameObjectData* data = sObjectMgr->GetGOData(guid);
        if (!data)
            return;
        entry = data->id;
    }
    m_Objects[type] = MAKE_NEW_GUID(guid, entry, HIGHGUID_GAMEOBJECT);
    m_ObjectTypes[m_Objects[type]] = type;
}

void OPvPCapturePoint::AddCre(uint32 type, uint32 guid, uint32 entry)
{
    if (!entry)
    {
        const CreatureData* data = sObjectMgr->GetCreatureData(guid);
        if (!data)
            return;
        entry = data->id;
    }
    m_Creatures[type] = MAKE_NEW_GUID(guid, entry, HIGHGUID_UNIT);
    m_CreatureTypes[m_Creatures[type]] = type;
}

bool OPvPCapturePoint::AddObject(uint32 type, uint32 entry, uint32 map, float x, float y, float z, float o, float rotation0, float rotation1, float rotation2, float rotation3)
{
    if (uint32 guid = sObjectMgr->AddGOData(entry, map, x, y, z, o, 0, rotation0, rotation1, rotation2, rotation3))
    {
        AddGO(type, guid, entry);
        return true;
    }

    return false;
}

bool OPvPCapturePoint::AddCreature(uint32 type, uint32 entry, uint32 map, float x, float y, float z, float o, uint32 spawntimedelay)
{
    if (uint32 guid = sObjectMgr->AddCreData(entry, map, x, y, z, o, spawntimedelay))
    {
        AddCre(type, guid, entry);
        return true;
    }

    return false;
}

bool OPvPCapturePoint::SetCapturePointData(uint32 entry, uint32 map, float x, float y, float z, float o, float rotation0, float rotation1, float rotation2, float rotation3)
{
#if defined(ENABLE_EXTRAS) && defined(ENABLE_EXTRA_LOGS)
    sLog->outDebug(LOG_FILTER_OUTDOORPVP, "Creating capture point %u", entry);
#endif

    // check info existence
    GameObjectTemplate const* goinfo = sObjectMgr->GetGameObjectTemplate(entry);
    if (!goinfo || goinfo->type != GAMEOBJECT_TYPE_CAPTURE_POINT)
    {
        sLog->outError("OutdoorPvP: GO %u is not capture point!", entry);
        return false;
    }

    uint32 lowguid = sObjectMgr->AddGOData(entry, map, x, y, z, o, 0, rotation0, rotation1, rotation2, rotation3);
    if (!lowguid)
        return false;
    m_capturePointGUID = MAKE_NEW_GUID(lowguid, entry, HIGHGUID_GAMEOBJECT);

    // get the needed values from goinfo
    m_maxValue = (float)goinfo->capturePoint.maxTime;
    m_maxSpeed = m_maxValue / (goinfo->capturePoint.minTime ? goinfo->capturePoint.minTime : 60);
    m_neutralValuePct = goinfo->capturePoint.neutralPercent;
    m_minValue = CalculatePct(m_maxValue, m_neutralValuePct);
    return true;
}

bool OPvPCapturePoint::DelCreature(uint32 type)
{
    if (!m_Creatures[type])
    {
#if defined(ENABLE_EXTRAS) && defined(ENABLE_EXTRA_LOGS)
        sLog->outDebug(LOG_FILTER_OUTDOORPVP, "opvp creature type %u was already deleted", type);
#endif
        return false;
    }

    Creature* cr = HashMapHolder<Creature>::Find(m_Creatures[type]);
    if (!cr)
    {
        // can happen when closing the core
        m_Creatures[type] = 0;
        return false;
    }
#if defined(ENABLE_EXTRAS) && defined(ENABLE_EXTRA_LOGS)
    sLog->outDebug(LOG_FILTER_OUTDOORPVP, "deleting opvp creature type %u", type);
#endif
    uint32 guid = cr->GetDBTableGUIDLow();
    // Don't save respawn time
    cr->SetRespawnTime(0);
    cr->RemoveCorpse();
    // explicit removal from map
    // beats me why this is needed, but with the recent removal "cleanup" some creatures stay in the map if "properly" deleted
    // so this is a big fat workaround, if AddObjectToRemoveList and DoDelayedMovesAndRemoves worked correctly, this wouldn't be needed
    //if (Map* map = sMapMgr->FindMap(cr->GetMapId()))
    //    map->Remove(cr, false);
    // delete respawn time for this creature
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CREATURE_RESPAWN);
    stmt->setUInt32(0, guid);
    stmt->setUInt16(1, cr->GetMapId());
    stmt->setUInt32(2, 0);  // instance id, always 0 for world maps
    CharacterDatabase.Execute(stmt);

    cr->AddObjectToRemoveList();
    sObjectMgr->DeleteCreatureData(guid);
    m_CreatureTypes[m_Creatures[type]] = 0;
    m_Creatures[type] = 0;
    return true;
}

bool OPvPCapturePoint::DelObject(uint32 type)
{
    if (!m_Objects[type])
        return false;

    GameObject* obj = HashMapHolder<GameObject>::Find(m_Objects[type]);
    if (!obj)
    {
        m_Objects[type] = 0;
        return false;
    }
    uint32 guid = obj->GetDBTableGUIDLow();
    obj->SetRespawnTime(0);                                 // not save respawn time
    obj->Delete();
    sObjectMgr->DeleteGOData(guid);
    m_ObjectTypes[m_Objects[type]] = 0;
    m_Objects[type] = 0;
    return true;
}

bool OPvPCapturePoint::DelCapturePoint()
{
    sObjectMgr->DeleteGOData(GUID_LOPART(m_capturePointGUID));
    m_capturePointGUID = 0;

    if (m_capturePoint)
    {
        m_capturePoint->SetRespawnTime(0);                                 // not save respawn time
        m_capturePoint->Delete();
    }

    return true;
}

void OPvPCapturePoint::DeleteSpawns()
{
    for (std::map<uint32, uint64>::iterator i = m_Objects.begin(); i != m_Objects.end(); ++i)
        DelObject(i->first);
    for (std::map<uint32, uint64>::iterator i = m_Creatures.begin(); i != m_Creatures.end(); ++i)
        DelCreature(i->first);
    DelCapturePoint();
}

void OutdoorPvP::DeleteSpawns()
{
    for (OPvPCapturePointMap::iterator itr = m_capturePoints.begin(); itr != m_capturePoints.end(); ++itr)
    {
        itr->second->DeleteSpawns();
        delete itr->second;
    }
    m_capturePoints.clear();
}

OutdoorPvP::OutdoorPvP() : m_TypeId(0), m_sendUpdate(true) { }

OutdoorPvP::~OutdoorPvP()
{
    DeleteSpawns();
}

void OutdoorPvP::HandlePlayerEnterZone(Player* player, uint32 /*zone*/)
{
    m_players[player->GetTeamId()].insert(player->GetGUID());
}

void OutdoorPvP::HandlePlayerLeaveZone(Player* player, uint32 /*zone*/)
{
    // inform the objectives of the leaving
    for (OPvPCapturePointMap::iterator itr = m_capturePoints.begin(); itr != m_capturePoints.end(); ++itr)
        itr->second->HandlePlayerLeave(player);
    // remove the world state information from the player (we can't keep everyone up to date, so leave out those who are not in the concerning zones)
    if (!player->GetSession()->PlayerLogout())
        SendRemoveWorldStates(player);
    m_players[player->GetTeamId()].erase(player->GetGUID());
#if defined(ENABLE_EXTRAS) && defined(ENABLE_EXTRA_LOGS)
    sLog->outDebug(LOG_FILTER_OUTDOORPVP, "Player %s left an outdoorpvp zone", player->GetName().c_str());
#endif
}

void OutdoorPvP::HandlePlayerResurrects(Player* /*player*/, uint32 /*zone*/)
{
}

bool OutdoorPvP::Update(uint32 diff)
{
    bool objective_changed = false;
    for (OPvPCapturePointMap::iterator itr = m_capturePoints.begin(); itr != m_capturePoints.end(); ++itr)
    {
        if (itr->second->Update(diff))
            objective_changed = true;
    }
    return objective_changed;
}

bool OPvPCapturePoint::Update(uint32 diff)
{
    if (!m_capturePoint)
        return false;

    float radius = (float)m_capturePoint->GetGOInfo()->capturePoint.radius;

    for (uint32 team = 0; team < 2; ++team)
    {
        for (PlayerSet::iterator itr = m_activePlayers[team].begin(); itr != m_activePlayers[team].end();)
        {
            uint64 playerGuid = *itr;
            ++itr;

            if (Player* player = ObjectAccessor::FindPlayer(playerGuid))
                if (!m_capturePoint->IsWithinDistInMap(player, radius) || !player->IsOutdoorPvPActive())
                    HandlePlayerLeave(player);
        }
    }

    std::list<Player*> players;
    acore::AnyPlayerInObjectRangeCheck checker(m_capturePoint, radius);
    acore::PlayerListSearcher<acore::AnyPlayerInObjectRangeCheck> searcher(m_capturePoint, players, checker);
    m_capturePoint->VisitNearbyWorldObject(radius, searcher);

    for (std::list<Player*>::iterator itr = players.begin(); itr != players.end(); ++itr)
    {
        Player* const player = *itr;
        if (player->IsOutdoorPvPActive())
        {
            if (m_activePlayers[player->GetTeamId()].insert(player->GetGUID()).second)
                HandlePlayerEnter(*itr);
        }
    }

    // get the difference of numbers
    float fact_diff = ((float)m_activePlayers[0].size() - (float)m_activePlayers[1].size()) * diff / OUTDOORPVP_OBJECTIVE_UPDATE_INTERVAL;
    if (!fact_diff)
        return false;

    TeamId ChallengerId = TEAM_NEUTRAL;
    float maxDiff = m_maxSpeed * diff;

    if (fact_diff < 0)
    {
        // horde is in majority, but it's already horde-controlled -> no change
        if (m_State == OBJECTIVESTATE_HORDE && m_value <= -m_maxValue)
            return false;

        if (fact_diff < -maxDiff)
            fact_diff = -maxDiff;

        ChallengerId = TEAM_HORDE;
    }
    else
    {
        // ally is in majority, but it's already ally-controlled -> no change
        if (m_State == OBJECTIVESTATE_ALLIANCE && m_value >= m_maxValue)
            return false;

        if (fact_diff > maxDiff)
            fact_diff = maxDiff;

        ChallengerId = TEAM_ALLIANCE;
    }

    float oldValue = m_value;
    TeamId oldTeam = m_team;

    m_OldState = m_State;

    m_value += fact_diff;

    if (m_value < -m_minValue) // red
    {
        if (m_value < -m_maxValue)
            m_value = -m_maxValue;
        m_State = OBJECTIVESTATE_HORDE;
        m_team = TEAM_HORDE;
    }
    else if (m_value > m_minValue) // blue
    {
        if (m_value > m_maxValue)
            m_value = m_maxValue;
        m_State = OBJECTIVESTATE_ALLIANCE;
        m_team = TEAM_ALLIANCE;
    }
    else if (oldValue * m_value <= 0) // grey, go through mid point
    {
        // if challenger is ally, then n->a challenge
        if (ChallengerId == TEAM_ALLIANCE)
            m_State = OBJECTIVESTATE_NEUTRAL_ALLIANCE_CHALLENGE;
        // if challenger is horde, then n->h challenge
        else if (ChallengerId == TEAM_HORDE)
            m_State = OBJECTIVESTATE_NEUTRAL_HORDE_CHALLENGE;
        m_team = TEAM_NEUTRAL;
    }
    else // grey, did not go through mid point
    {
        // old phase and current are on the same side, so one team challenges the other
        if (ChallengerId == TEAM_ALLIANCE && (m_OldState == OBJECTIVESTATE_HORDE || m_OldState == OBJECTIVESTATE_NEUTRAL_HORDE_CHALLENGE))
            m_State = OBJECTIVESTATE_HORDE_ALLIANCE_CHALLENGE;
        else if (ChallengerId == TEAM_HORDE && (m_OldState == OBJECTIVESTATE_ALLIANCE || m_OldState == OBJECTIVESTATE_NEUTRAL_ALLIANCE_CHALLENGE))
            m_State = OBJECTIVESTATE_ALLIANCE_HORDE_CHALLENGE;
        m_team = TEAM_NEUTRAL;
    }

    if (m_value != oldValue)
        SendChangePhase();

    if (m_OldState != m_State)
    {
        //sLog->outError(LOG_FILTER_OUTDOORPVP, "%u->%u", m_OldState, m_State);
        if (oldTeam != m_team)
            ChangeTeam(oldTeam);
        ChangeState();
        return true;
    }

    return false;
}

void OutdoorPvP::SendUpdateWorldState(uint32 field, uint32 value)
{
    if (m_sendUpdate)
        for (int i = 0; i < 2; ++i)
            for (PlayerSet::iterator itr = m_players[i].begin(); itr != m_players[i].end(); ++itr)
                if (Player* const player = ObjectAccessor::FindPlayer(*itr))
                    player->SendUpdateWorldState(field, value);
}

void OPvPCapturePoint::SendUpdateWorldState(uint32 field, uint32 value)
{
    for (uint32 team = 0; team < 2; ++team)
    {
        // send to all players present in the area
        for (PlayerSet::iterator itr = m_activePlayers[team].begin(); itr != m_activePlayers[team].end(); ++itr)
            if (Player* const player = ObjectAccessor::FindPlayer(*itr))
                player->SendUpdateWorldState(field, value);
    }
}

void OPvPCapturePoint::SendObjectiveComplete(uint32 id, uint64 guid)
{
    uint32 team;
    switch (m_State)
    {
        case OBJECTIVESTATE_ALLIANCE:
            team = 0;
            break;
        case OBJECTIVESTATE_HORDE:
            team = 1;
            break;
        default:
            return;
    }

    // send to all players present in the area
    for (PlayerSet::iterator itr = m_activePlayers[team].begin(); itr != m_activePlayers[team].end(); ++itr)
        if (Player* const player = ObjectAccessor::FindPlayer(*itr))
            player->KilledMonsterCredit(id, guid);
}

void OutdoorPvP::HandleKill(Player* killer, Unit* killed)
{
    if (Group* group = killer->GetGroup())
    {
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* groupGuy = itr->GetSource();

            if (!groupGuy)
                continue;

            // skip if too far away
            if (!groupGuy->IsAtGroupRewardDistance(killed) && killer != groupGuy)
                continue;

            // creature kills must be notified, even if not inside objective / not outdoor pvp active
            // player kills only count if active and inside objective
            if ((groupGuy->IsOutdoorPvPActive() && IsInsideObjective(groupGuy)) || killed->GetTypeId() == TYPEID_UNIT)
            {
                HandleKillImpl(groupGuy, killed);
            }
        }
    }
    else
    {
        // creature kills must be notified, even if not inside objective / not outdoor pvp active
        if (killer && ((killer->IsOutdoorPvPActive() && IsInsideObjective(killer)) || killed->GetTypeId() == TYPEID_UNIT))
        {
            HandleKillImpl(killer, killed);
        }
    }
}

bool OutdoorPvP::IsInsideObjective(Player* player) const
{
    for (OPvPCapturePointMap::const_iterator itr = m_capturePoints.begin(); itr != m_capturePoints.end(); ++itr)
        if (itr->second->IsInsideObjective(player))
            return true;

    return false;
}

bool OPvPCapturePoint::IsInsideObjective(Player* player) const
{
    PlayerSet const& plSet = m_activePlayers[player->GetTeamId()];
    return plSet.find(player->GetGUID()) != plSet.end();
}

bool OutdoorPvP::HandleCustomSpell(Player* player, uint32 spellId, GameObject* go)
{
    for (OPvPCapturePointMap::iterator itr = m_capturePoints.begin(); itr != m_capturePoints.end(); ++itr)
        if (itr->second->HandleCustomSpell(player, spellId, go))
            return true;

    return false;
}

bool OPvPCapturePoint::HandleCustomSpell(Player* player, uint32 /*spellId*/, GameObject* /*go*/)
{
    if (!player->IsOutdoorPvPActive())
        return false;
    return false;
}

bool OutdoorPvP::HandleOpenGo(Player* player, uint64 guid)
{
    for (OPvPCapturePointMap::iterator itr = m_capturePoints.begin(); itr != m_capturePoints.end(); ++itr)
        if (itr->second->HandleOpenGo(player, guid) >= 0)
            return true;

    return false;
}

bool OutdoorPvP::HandleGossipOption(Player* player, uint64 guid, uint32 id)
{
    for (OPvPCapturePointMap::iterator itr = m_capturePoints.begin(); itr != m_capturePoints.end(); ++itr)
        if (itr->second->HandleGossipOption(player, guid, id))
            return true;

    return false;
}

bool OutdoorPvP::CanTalkTo(Player* player, Creature* c, GossipMenuItems const& gso)
{
    for (OPvPCapturePointMap::iterator itr = m_capturePoints.begin(); itr != m_capturePoints.end(); ++itr)
        if (itr->second->CanTalkTo(player, c, gso))
            return true;

    return false;
}

bool OutdoorPvP::HandleDropFlag(Player* player, uint32 id)
{
    for (OPvPCapturePointMap::iterator itr = m_capturePoints.begin(); itr != m_capturePoints.end(); ++itr)
        if (itr->second->HandleDropFlag(player, id))
            return true;

    return false;
}

bool OPvPCapturePoint::HandleGossipOption(Player* /*player*/, uint64 /*guid*/, uint32 /*id*/)
{
    return false;
}

bool OPvPCapturePoint::CanTalkTo(Player* /*player*/, Creature* /*c*/, GossipMenuItems const& /*gso*/)
{
    return false;
}

bool OPvPCapturePoint::HandleDropFlag(Player* /*player*/, uint32 /*id*/)
{
    return false;
}

int32 OPvPCapturePoint::HandleOpenGo(Player* /*player*/, uint64 guid)
{
    std::map<uint64, uint32>::iterator itr = m_ObjectTypes.find(guid);
    if (itr != m_ObjectTypes.end())
    {
        return itr->second;
    }
    return -1;
}

bool OutdoorPvP::HandleAreaTrigger(Player* /*player*/, uint32 /*trigger*/)
{
    return false;
}

void OutdoorPvP::BroadcastPacket(WorldPacket& data) const
{
    // This is faster than sWorld->SendZoneMessage
    for (uint32 team = 0; team < 2; ++team)
        for (PlayerSet::const_iterator itr = m_players[team].begin(); itr != m_players[team].end(); ++itr)
            if (Player* const player = ObjectAccessor::FindPlayer(*itr))
                player->GetSession()->SendPacket(&data);
}

void OutdoorPvP::RegisterZone(uint32 zoneId)
{
    sOutdoorPvPMgr->AddZone(zoneId, this);
}

bool OutdoorPvP::HasPlayer(Player const* player) const
{
    PlayerSet const& plSet = m_players[player->GetTeamId()];
    return plSet.find(player->GetGUID()) != plSet.end();
}

void OutdoorPvP::TeamCastSpell(TeamId team, int32 spellId, Player* sameMapPlr)
{
    if (spellId > 0)
    {
        for (PlayerSet::iterator itr = m_players[team].begin(); itr != m_players[team].end(); ++itr)
            if (Player* const player = ObjectAccessor::FindPlayer(*itr))
                if (!sameMapPlr || sameMapPlr->FindMap() == player->FindMap())
                    player->CastSpell(player, (uint32)spellId, true);
    }
    else
    {
        for (PlayerSet::iterator itr = m_players[team].begin(); itr != m_players[team].end(); ++itr)
            if (Player* const player = ObjectAccessor::FindPlayer(*itr))
                if (!sameMapPlr || sameMapPlr->FindMap() == player->FindMap())
                    player->RemoveAura((uint32) - spellId); // by stack?
    }
}

void OutdoorPvP::TeamApplyBuff(TeamId teamId, uint32 spellId, uint32 spellId2, Player* sameMapPlr)
{
    TeamCastSpell(teamId, spellId, sameMapPlr);
    TeamCastSpell(teamId == TEAM_ALLIANCE ? TEAM_HORDE : TEAM_ALLIANCE, spellId2 ? -(int32)spellId2 : -(int32)spellId, sameMapPlr);
}

void OutdoorPvP::OnGameObjectCreate(GameObject* go)
{
    if (go->GetGoType() != GAMEOBJECT_TYPE_CAPTURE_POINT)
        return;

    if (OPvPCapturePoint* cp = GetCapturePoint(go->GetDBTableGUIDLow()))
        cp->m_capturePoint = go;
}

void OutdoorPvP::OnGameObjectRemove(GameObject* go)
{
    if (go->GetGoType() != GAMEOBJECT_TYPE_CAPTURE_POINT)
        return;

    if (OPvPCapturePoint* cp = GetCapturePoint(go->GetDBTableGUIDLow()))
        cp->m_capturePoint = nullptr;
}
