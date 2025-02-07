/*
 * Copyright (C) 2016+     AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-GPL2
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

/*
Name: Boss_Hakkar
%Complete: 95
Comment: Blood siphon spell buggy cause of Core Issue.
Category: Zul'Gurub
*/

#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "zulgurub.h"

enum Says
{
    SAY_AGGRO                   = 0,
    SAY_FLEEING                 = 1,
    SAY_MINION_DESTROY          = 2,     // Where does it belong?
    SAY_PROTECT_ALTAR           = 3      // Where does it belong?
};

enum Spells
{
    SPELL_BLOOD_SIPHON          = 24322, // Buggy ?
    SPELL_CORRUPTED_BLOOD       = 24328,
    SPELL_CAUSE_INSANITY        = 24327, // Spell needs scripting.
    SPELL_WILL_OF_HAKKAR        = 24178,
    SPELL_ENRAGE                = 24318,
    // The Aspects of all High Priests spells
    SPELL_ASPECT_OF_JEKLIK      = 24687,
    SPELL_ASPECT_OF_VENOXIS     = 24688,
    SPELL_ASPECT_OF_MARLI       = 24686,
    SPELL_ASPECT_OF_THEKAL      = 24689,
    SPELL_ASPECT_OF_ARLOKK      = 24690
};

enum Events
{
    EVENT_BLOOD_SIPHON          = 1,
    EVENT_CORRUPTED_BLOOD       = 2,
    EVENT_CAUSE_INSANITY        = 3,     // Spell needs scripting. Event disabled
    EVENT_WILL_OF_HAKKAR        = 4,
    EVENT_ENRAGE                = 5,
    // The Aspects of all High Priests events
    EVENT_ASPECT_OF_JEKLIK      = 6,
    EVENT_ASPECT_OF_VENOXIS     = 7,
    EVENT_ASPECT_OF_MARLI       = 8,
    EVENT_ASPECT_OF_THEKAL      = 9,
    EVENT_ASPECT_OF_ARLOKK      = 10
};

class boss_hakkar : public CreatureScript
{
public:
    boss_hakkar() : CreatureScript("boss_hakkar") { }

    struct boss_hakkarAI : public BossAI
    {
        boss_hakkarAI(Creature* creature) : BossAI(creature, DATA_HAKKAR) { }

        void Reset() override
        {
            _Reset();
        }

        void JustDied(Unit* /*killer*/) override
        {
            _JustDied();
        }

        void EnterCombat(Unit* /*who*/) override
        {
            _EnterCombat();
            events.ScheduleEvent(EVENT_BLOOD_SIPHON, 90000);
            events.ScheduleEvent(EVENT_CORRUPTED_BLOOD, 25000);
            events.ScheduleEvent(EVENT_CAUSE_INSANITY, 17000);
            events.ScheduleEvent(EVENT_WILL_OF_HAKKAR, 17000);
            events.ScheduleEvent(EVENT_ENRAGE, 600000);
            if (instance->GetBossState(DATA_JEKLIK) != DONE)
                events.ScheduleEvent(EVENT_ASPECT_OF_JEKLIK, 4000);
            if (instance->GetBossState(DATA_VENOXIS) != DONE)
                events.ScheduleEvent(EVENT_ASPECT_OF_VENOXIS, 7000);
            if (instance->GetBossState(DATA_MARLI) != DONE)
                events.ScheduleEvent(EVENT_ASPECT_OF_MARLI, 12000);
            if (instance->GetBossState(DATA_THEKAL) != DONE)
                events.ScheduleEvent(EVENT_ASPECT_OF_THEKAL, 8000);
            if (instance->GetBossState(DATA_ARLOKK) != DONE)
                events.ScheduleEvent(EVENT_ASPECT_OF_ARLOKK, 18000);
            Talk(SAY_AGGRO);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_BLOOD_SIPHON:
                        DoCastVictim(SPELL_BLOOD_SIPHON, true);
                        events.ScheduleEvent(EVENT_BLOOD_SIPHON, 90000);
                        break;
                    case EVENT_CORRUPTED_BLOOD:
                        DoCastVictim(SPELL_CORRUPTED_BLOOD, true);
                        events.ScheduleEvent(EVENT_CORRUPTED_BLOOD, urand(30000, 45000));
                        break;
                    case EVENT_CAUSE_INSANITY:
                        // DoCast(SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true), SPELL_CAUSE_INSANITY);
                        // events.ScheduleEvent(EVENT_CAUSE_INSANITY, urand(35000, 45000));
                        break;
                    case EVENT_WILL_OF_HAKKAR:
                        // Xinef: Skip Tank
                        DoCast(SelectTarget(SELECT_TARGET_RANDOM, 1, 100, true), SPELL_WILL_OF_HAKKAR);
                        events.ScheduleEvent(EVENT_WILL_OF_HAKKAR, urand(25000, 35000));
                        break;
                    case EVENT_ENRAGE:
                        if (!me->HasAura(SPELL_ENRAGE))
                            DoCast(me, SPELL_ENRAGE);
                        events.ScheduleEvent(EVENT_ENRAGE, 90000);
                        break;
                    case EVENT_ASPECT_OF_JEKLIK:
                        DoCastVictim(SPELL_ASPECT_OF_JEKLIK, true);
                        events.ScheduleEvent(EVENT_ASPECT_OF_JEKLIK, urand(10000, 14000));
                        break;
                    case EVENT_ASPECT_OF_VENOXIS:
                        DoCastVictim(SPELL_ASPECT_OF_VENOXIS, true);
                        events.ScheduleEvent(EVENT_ASPECT_OF_VENOXIS, 8000);
                        break;
                    case EVENT_ASPECT_OF_MARLI:
                        DoCastVictim(SPELL_ASPECT_OF_MARLI, true);
                        events.ScheduleEvent(EVENT_ASPECT_OF_MARLI, 10000);
                        break;
                    case EVENT_ASPECT_OF_THEKAL:
                        DoCastVictim(SPELL_ASPECT_OF_THEKAL, true);
                        events.ScheduleEvent(EVENT_ASPECT_OF_THEKAL, 15000);
                        break;
                    case EVENT_ASPECT_OF_ARLOKK:
                        DoCastVictim(SPELL_ASPECT_OF_ARLOKK, true);
                        events.ScheduleEvent(EVENT_ASPECT_OF_ARLOKK, urand(10000, 15000));
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetInstanceAI<boss_hakkarAI>(creature);
    }
};

void AddSC_boss_hakkar()
{
    new boss_hakkar();
}
