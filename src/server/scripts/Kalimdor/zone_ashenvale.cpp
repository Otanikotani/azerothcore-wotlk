/*
 * Copyright (C) 2016+     AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-GPL2
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

/* ScriptData
SDName: Ashenvale
SD%Complete: 70
SDComment: Quest support: 6544, 6482
SDCategory: Ashenvale Forest
EndScriptData */

#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"
#include "ScriptMgr.h"


enum Muglash
{
    FACTION_QUEST           = 113,
    SAY_MUG_START1          = 0,
    SAY_MUG_START2          = 1,
    SAY_MUG_BRAZIER         = 2,
    SAY_MUG_BRAZIER_WAIT    = 3,
    SAY_MUG_ON_GUARD        = 4,
    SAY_MUG_REST            = 5,
    SAY_MUG_DONE            = 6,
    SAY_MUG_GRATITUDE       = 7,
    SAY_MUG_PATROL          = 8,
    SAY_MUG_RETURN          = 9,

    QUEST_VORSHA            = 6641,

    GO_NAGA_BRAZIER         = 178247,

    NPC_WRATH_RIDER         = 3713,
    NPC_WRATH_SORCERESS     = 3717,
    NPC_WRATH_RAZORTAIL     = 3712,

    NPC_WRATH_PRIESTESS     = 3944,
    NPC_WRATH_MYRMIDON      = 3711,
    NPC_WRATH_SEAWITCH      = 3715,

    NPC_VORSHA              = 12940,
    NPC_MUGLASH             = 12717
};

Position const FirstNagaCoord[3] =
{
    { 3603.504150f, 1122.631104f,  1.635f, 0.0f },        // rider
    { 3589.293945f, 1148.664063f,  5.565f, 0.0f },        // sorceress
    { 3609.925537f, 1168.759521f, -1.168f, 0.0f }         // razortail
};

Position const SecondNagaCoord[3] =
{
    { 3609.925537f, 1168.759521f, -1.168f, 0.0f },        // witch
    { 3645.652100f, 1139.425415f, 1.322f,  0.0f },        // priest
    { 3583.602051f, 1128.405762f, 2.347f,  0.0f }         // myrmidon
};

Position const VorshaCoord = {3633.056885f, 1172.924072f, -5.388f, 0.0f};

class npc_muglash : public CreatureScript
{
public:
    npc_muglash() : CreatureScript("npc_muglash") { }

    struct npc_muglashAI : public npc_escortAI
    {
        npc_muglashAI(Creature* creature) : npc_escortAI(creature) { }

        void Reset() override
        {
            eventTimer = 10000;
            waveId = 0;
            _isBrazierExtinguished = false;
        }

        void EnterCombat(Unit* /*who*/) override
        {
            if (Player* player = GetPlayerForEscort())
                if (HasEscortState(STATE_ESCORT_PAUSED))
                {
                    if (urand(0, 1))
                        Talk(SAY_MUG_ON_GUARD, player);
                    return;
                }
        }

        void JustDied(Unit* /*killer*/) override
        {
            if (HasEscortState(STATE_ESCORT_ESCORTING))
                if (Player* player = GetPlayerForEscort())
                    player->FailQuest(QUEST_VORSHA);
        }

        void JustSummoned(Creature* summoned) override
        {
            summoned->AI()->AttackStart(me);
        }

        void sQuestAccept(Player* player, Quest const* quest) override
        {
            if (quest->GetQuestId() == QUEST_VORSHA)
            {
                Talk(SAY_MUG_START1);
                me->setFaction(FACTION_QUEST);
                npc_escortAI::Start(true, false, player->GetGUID());
            }
        }

        void WaypointReached(uint32 waypointId) override
        {
            if (Player* player = GetPlayerForEscort())
            {
                switch (waypointId)
                {
                    case 0:
                        Talk(SAY_MUG_START2, player);
                        break;
                    case 24:
                        Talk(SAY_MUG_BRAZIER, player);

                        if (GameObject* go = GetClosestGameObjectWithEntry(me, GO_NAGA_BRAZIER, INTERACTION_DISTANCE * 2))
                        {
                            go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                            SetEscortPaused(true);
                        }
                        break;
                    case 25:
                        Talk(SAY_MUG_GRATITUDE);
                        player->GroupEventHappens(QUEST_VORSHA, me);
                        break;
                    case 26:
                        Talk(SAY_MUG_PATROL);
                        break;
                    case 27:
                        Talk(SAY_MUG_RETURN);
                        break;
                }
            }
        }

        void DoWaveSummon()
        {
            switch (waveId)
            {
                case 1:
                    me->SummonCreature(NPC_WRATH_RIDER,     FirstNagaCoord[0], TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                    me->SummonCreature(NPC_WRATH_SORCERESS, FirstNagaCoord[1], TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                    me->SummonCreature(NPC_WRATH_RAZORTAIL, FirstNagaCoord[2], TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                    break;
                case 2:
                    me->SummonCreature(NPC_WRATH_PRIESTESS, SecondNagaCoord[0], TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                    me->SummonCreature(NPC_WRATH_MYRMIDON,  SecondNagaCoord[1], TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                    me->SummonCreature(NPC_WRATH_SEAWITCH,  SecondNagaCoord[2], TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                    break;
                case 3:
                    me->SummonCreature(NPC_VORSHA, VorshaCoord, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                    break;
                case 4:
                    SetEscortPaused(false);
                    Talk(SAY_MUG_DONE);
                    break;
            }
        }

        void UpdateAI(uint32 diff) override
        {
            npc_escortAI::UpdateAI(diff);

            if (!me->GetVictim())
            {
                if (HasEscortState(STATE_ESCORT_PAUSED) && _isBrazierExtinguished)
                {
                    if (eventTimer < diff)
                    {
                        ++waveId;
                        DoWaveSummon();
                        eventTimer = 10000;
                    }
                    else
                        eventTimer -= diff;
                }
                return;
            }
            DoMeleeAttackIfReady();
        }

    private:
        uint32 eventTimer;
        uint8  waveId;
    public:
        bool   _isBrazierExtinguished;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_muglashAI(creature);
    }
};

class go_naga_brazier : public GameObjectScript
{
public:
    go_naga_brazier() : GameObjectScript("go_naga_brazier") { }

    bool OnGossipHello(Player* /*player*/, GameObject* go) override
    {
        if (Creature* creature = GetClosestCreatureWithEntry(go, NPC_MUGLASH, INTERACTION_DISTANCE * 2))
        {
            if (npc_muglash::npc_muglashAI* pEscortAI = CAST_AI(npc_muglash::npc_muglashAI, creature->AI()))
            {
                creature->AI()->Talk(SAY_MUG_BRAZIER_WAIT);

                pEscortAI->_isBrazierExtinguished = true;
                return false;
            }
        }

        return true;
    }
};

void AddSC_ashenvale()
{
    new npc_muglash();
    new go_naga_brazier();
}
