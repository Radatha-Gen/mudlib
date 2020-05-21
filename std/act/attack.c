/*
 *  /std/act/attack.c
 *
 *  Enhanced aggression code.
 */

#pragma strict_types

#include <macros.h>
#include <formulas.h>

public static int monster_attack_dare;
public static int monster_attack_brave;
public static int monster_attack_chance;
public static int monster_action_chance;
public static mixed monster_aggressive;
public static mixed monster_attack_action;

/*
 * Function name: set_aggressive
 * Description:   Make the monster aggressive, meaning it will attack on sight.
 * Arguments:	  mixed aggro - 1 aggressive, 0 - not aggressive (may be VBFC)
 *                int pct - % chance of being brave enough to attack anything.
 *                      Default = 15%
 */
public varargs void
set_aggressive(mixed aggro, int pct = 15)
{
    monster_aggressive = aggro;
    monster_attack_brave = pct;
}

/*
 * Function name: query_aggressive
 * Description  : Find out whether this monster is aggressive or a pussycat.
 * Returns      : mixed - if true, it's aggressive. Note this may be VBFC.
 */
public mixed
query_aggressive()
{
    return monster_aggressive;
}

/*
 * Function name: query_aggressive_brave
 * Description  : Find out % at which we are stupidly brave.
 * Returns      : int - the percentage.
 */
public int
query_aggressive_brave()
{
    return monster_attack_brave;
}

/*
 * Function name: set_attack_chance
 * Description:   Set chance the monster will attack
 * Arguments:	  int pct - the % chance it will attack, 0 = 100 %
 */
public void
set_attack_chance(int pct)
{
    monster_attack_chance = pct;
}

/*
 * Function name: query_attack_chance
 * Description:   Query chance the monster will attack
 * Returns:	  int - the % chance it will attack, 0 = 100 %
 */
public int
query_attack_chance()
{
    return monster_attack_chance;
}
 
/*
 * Function name: set_action_chance
 * Description:   Set chance the monster will perform an action.
 * Arguments:	  pct - the % chance it will act, 0 = 100 %
 */
public void
set_action_chance(int pct)
{
    monster_action_chance = pct;
}

/*
 * Function name: query_action_chance
 * Description:   Query chance the monster will perform an action.
 * Arguments:	  int - the % chance it will act, 0 = 100 %
 */
public int
query_action_chance()
{
    return monster_action_chance;
}

/*
 * Function name: set_attack_action
 * Description:   Commands the monster will do as they attack.
 * Arguments:	  act - Command, array of commands, or vbfc of either.
 */
public void
set_attack_action(mixed act)
{
    monster_attack_action = (pointerp(act) ? act : ({ act }));
}

/*
 * Function name: query_attack_action
 * Description:   Commands the monster will do as they attack.
 * Arguments:	  mixed - array of commands, or vbfc of either.
 */
public mixed
query_attack_action()
{
    return ({ }) + monster_attack_action;
}

/*
 * Function name: set_attack_dare
 * Description:   See if we should check F_DARE_ATTACK
 *                and abort early if it fails. Eliminates the
 *                'too scared to attack' messages / spam.
 * Arguments:	  check - True/1 - Check, False/0 - don't check.
 */
public void
set_attack_dare(int check)
{
    monster_attack_dare = check;
}

/*
 * Function name: query_attack_dare
 * Description:   Find out if we check F_DARE_ATTACK and abort early if it
 *                fails. Eliminates the 'too scared to attack' messages.
 * Returns:	  int - if true, check F_DARE_ATTACK.
 */
public int
query_attack_dare()
{
    return monster_attack_dare;
}

/*
 * Function name: aggressive_attack
 * Description  : We are aggressive, let us attack anyone in sight.
 * Arguments    : object target - the object to attack.
 */
public void
aggressive_attack(object target)
{
    int brave;

    /* Target isn't present. */
    if (!objectp(target) || !present(target, environment()))
	return;

    /* We are already in the midst of combat. Don't open a second front. */
    if (this_object()->query_attack())
	return;

    /* See if we are stupidly brave. */
    if (monster_attack_brave && (random(100) < monster_attack_brave))
    {
	if (!this_object()->query_prop(NPC_I_NO_FEAR))
	{
	    brave = 1;
	    this_object()->add_prop(NPC_I_NO_FEAR, 1);
	    command("emote is looking especially brave.");
	}
    }

    /* Check daring and possibly abort early.
     * Reduces the spam from 'scared' messages. */
    if (monster_attack_dare && !F_DARE_ATTACK(this_object(), target))
	return;

    /* Check for attacking actions. */
    if (sizeof(monster_attack_action))
    {
	foreach(mixed action : monster_attack_action)
	{
	    if (monster_action_chance &&
		(random(100) >= monster_action_chance))
	    {
		continue;
	    }

	    mixed res = this_object()->check_call(action);
	    if (strlen(res)) command(res);
	}
    }

    /* Go for the kill. */
    command("kill " + OB_NAME(target));

    /* Not feeling so brave now... */
    if (brave) this_object()->remove_prop(NPC_I_NO_FEAR);
}

/*
 * Function name: init_attack
 * Description:   Called from init_living() in monster.c
 */
public void
init_attack()
{
    /* We aren't aggressive on npc's, or just aren't aggressive. */
    if (this_player()->query_npc() ||
	!this_object()->check_call(monster_aggressive))
    {
	return;
    }

    /* Attack if the chance is good. */
    if (!monster_attack_chance || (random(100) < monster_attack_chance))
    {
	/* Randomize this more. */
	float delay = itof(1 + random(2)) + rnd();
	set_alarm(delay, 0.0, &aggressive_attack(this_player()));
    }
}

/*
 * Function name: notify_attack_on_team
 * Description  : This is called when someone attacks a team member of mine.
 * Arguments    : object friend - my team member
 *		  object attacker - the attacker
 */
public void
notify_attack_on_team(object friend, object attacker)
{
    /* Already busy. */
    if (this_object()->query_attack())
	return;

    /* Small chance of not attacking. */
    if (random(10))
    {
	set_alarm((rnd() + 1.0), 0.0, &aggressive_attack(attacker));
    }
}
