/*
 * /lib/herb_support.c
 *
 * Support for herbs that is used for potions as well.
 */

#pragma save_binary
#pragma strict_types

#include <files.h>
#include <macros.h>
#include <stdproperties.h>
#include <ss_types.h>
#include <herb.h>
#include <composite.h>
#include <language.h>

#define NO_EFFECT_MESSAGE "You don't feel any effect.\n"

/*
 * Variables
 */
string poison_file;
mixed *effects;
mixed *poison_damage;
int    duration;

string *messages; /* Messages given by the herb effects. */

/*
 * Function name: add_effect_message
 * Description  : Buffers the effect messages when ingesting multiple herbs.
 * Arguments    : string str - the message to add.
 */
void
add_effect_message(string str)
{
    if (!IN_ARRAY(str, messages))
    {
        messages += ({ str });
    }
}

/* 
 * Function name: do_tmp_stat
 * Description:   This function is called from the effects if the herb is
 *                stat-affecting. One stat is randomly increased
 *                temporarily, lasting as long as strength * 10. If strength is
 *                negative, the stat is decreased instead.
 * Arguments:     stat     - The number of the stat
 *                strength - How strong the herb is - time effect will last
 */
void
do_tmp_stat(int stat, int strength)
{
    int dtime;

    dtime = strength / 2 + random(strength);
    if (strength > 0)
    {
        this_player()->add_tmp_stat(stat, random(3) + 1, dtime);

        switch(stat)
        {
        case SS_STR:
            add_effect_message("You feel strengthened.\n");
            break;
        case SS_DEX:
            add_effect_message("You feel more dexterous.\n");
            break;
        case SS_CON:
            add_effect_message("You feel healthier.\n");
            break;
        case SS_INT:
            add_effect_message("You feel brighter.\n");
            break;
        case SS_WIS:
            add_effect_message("You feel wiser.\n");
            break;
        case SS_DIS:
            add_effect_message("You feel more secure.\n");
            break;
        default:
            add_effect_message("You feel more clever.\n");
        }
    }

    if (strength < 0)
    {
        this_player()->add_tmp_stat(stat, -(random(3) + 1), -dtime);

        switch(stat)
        {
        case SS_STR:
            add_effect_message("You feel weakened.\n");
            break;
        case SS_DEX:
            add_effect_message("You feel slower.\n");
            break;
        case SS_CON:
            add_effect_message("You feel less healthy.\n");
            break;
        case SS_INT:
            add_effect_message("You feel stupider.\n");
            break;
        case SS_WIS:
            add_effect_message("You feel less wise.\n");
            break;
        case SS_DIS:
            add_effect_message("You feel more insecure.\n");
            break;
        default:
            add_effect_message("You feel less clever.\n");
        }
    }
}

/*
 * Function name: add_resistance
 * Description:   This function is called from the herb-effects, and adds some
 *                resistance in the player. Max strength is 40.
 *                The resistance added is an additive resistance.
 *                (See /doc/man/general/spells for more info on resistance)
 * Arguments:     string res_type - The resistance type
 *                int strength - How strong the herb is
 */
void
add_resistance(string res_type, int strength)
{
    object res_obj;
    int dtime;

    res_obj = clone_object(RESISTANCE_OBJECT);
    if (strength > 40)
    {
        strength = 40;
    }
    res_obj->set_strength(strength);
    if (!(dtime = duration))
        dtime = 5 * (40 - strength / 2) + 5 * random(strength);
    res_obj->set_time(dtime);
    res_obj->set_res_type(res_type);
    res_obj->move(this_player(), 1);

    switch(res_type)
    {

    case MAGIC_I_RES_ACID:
        add_effect_message("Your skin is suddenly coverered with a glossy " +
            "substance, protecting you from acid.\n");
        break;
    case MAGIC_I_RES_AIR:
        add_effect_message("You feel an aura of calm issue from your pores, " +
            "protecting you from the air.\n");
        break;
    case MAGIC_I_RES_COLD:
        add_effect_message("A heat begins to radiate from your flesh, " +
            "helping to protect you from cold.\n");
        break;
    case MAGIC_I_RES_DEATH:
        add_effect_message("An indescribable sense of well-being flows " +
            "through you, protecting you from the elements of death.\n");
        break;
    case MAGIC_I_RES_EARTH:
        add_effect_message("Your entire body stiffens, growing hard to " +
            "ward off the elements of earth.\n");
        break;
    case MAGIC_I_RES_ELECTRICITY:
        add_effect_message("A smooth springy substance flows from your " +
            "pores to coat your skin, giving you enhanced protection from " +
            "electricity.\n");
        break;
    case MAGIC_I_RES_FIRE:
        add_effect_message("A chill washes over your flesh, and tiny ice " +
            "crystals form along your body as you gain protection from fire.\n");
        break;
    case MAGIC_I_RES_ILLUSION:
        add_effect_message("Your eyes tingle and seem to gain heightened " +
            "awareness, giving you protection to the powers of illusion.\n");
        break;
    case MAGIC_I_RES_LIFE:
        add_effect_message("A dark pall is cast outward from you to slowly " +
            "settle and cling to your body, protecting you from the powers " +
            "of life.\n");
        break;
    case MAGIC_I_RES_MAGIC:
        add_effect_message("The air seems to suddenly hang dully around you " +
            "in an aura of silence, protecting you from all forms of magic.\n");
        break;
    case MAGIC_I_RES_POISON:
        add_effect_message("With a surge, you feel the veins within your " +
            "body flood with a tingling sensation, protecting you from poison.\n");
        break;
    case MAGIC_I_RES_WATER:
        add_effect_message("Your skin suddenly takes on an unusual texture " +
            "to which water cannot easily cling, protecting you from its " +
            "effects.\n");
        break;
    default:
        add_effect_message("You feel more resistant.\n");
    }
}

/*
 * Function name: special_effect
 * Description:   Redefine this when you have done set_effect(HERB_SPECIAL);
 *                to do the effect of your herb.
 */
void
special_effect()
{
    add_effect_message(NO_EFFECT_MESSAGE);
}

/* 
 * Function name: do_one_herb_effect
 * Description  : In this function the effect(s) of the herb are resolved. It
 *                is called repeatedly for multiple effects.
 *                Read /doc/man/general/herbs and the header of set_herb_effect
 *                for more information.
 */
nomask void
do_one_herb_effect()
{
    int strength, res, i, n, a;
    string type;
    object poison, tp, to, *inv;

    seteuid(getuid());
    tp = this_player();
    to = this_object();
    i = 0;
    while (i < sizeof(effects))
    {
        switch(effects[i])
        {
        case HERB_HEALING:
            type = lower_case(effects[i + 1]);
            /* Allow for VBFC on the strength definition. */
            strength = this_object()->check_call(effects[i + 2]);
            res = 100 - tp->query_magic_res(MAGIC_I_RES_POISON);
            if (!type || type == "hp")
            {
                if (strength < 0) 
                { 
                    tp->reduce_hit_point(res * random(-strength) / 100);
                    if (tp->query_hp() <= 0)
                    {
                        write("You die from " + query_verb() + "ing " +
                            LANG_ADDART(this_object()->short()) + "!\n");
                        tp->do_die(to);
                        this_object()->remove_object();
                        break;
                    }
                    add_effect_message("You feel less healthy.\n");
                }
                else if (strength > 0)
                {
                    tp->heal_hp(strength);
                    add_effect_message("You feel healthier.\n");
                }
                else
                {
                    write(NO_EFFECT_MESSAGE);
                }
            }
            else if (type == "mana")
            {
                if (strength < 0)
                {
                    tp->set_mana(tp->query_mana() - res * random(-strength) / 100);
                    add_effect_message("You feel mentally weaker.\n");
                }
                else if (strength > 0)
                {
                    tp->set_mana(tp->query_mana() + strength);
                    add_effect_message("You feel mentally healthier.\n");
                }
                else
                {
                    add_effect_message(NO_EFFECT_MESSAGE);
                }
            }
            else if (type == "fatigue")
            {
                if (strength < 0)
                {
                    tp->set_fatigue(tp->query_fatigue() - res * 
                       random(-strength) / 100);
                    add_effect_message("You feel more tired.\n");
                }
                else if (strength > 0)
                {
                    write("You feel less tired.\n");
                    tp->set_fatigue(tp->query_fatigue() + strength);
                }
                else
                {
                    add_effect_message(NO_EFFECT_MESSAGE);
                }
            }
            else
            {
                add_effect_message(NO_EFFECT_MESSAGE);
            }
            break;
        case HERB_ENHANCING:
            type = lower_case(effects[i + 1]);
            /* Allow for VBFC on the strength definition. */
            strength = this_object()->check_call(effects[i + 2]);
            if (!strength || ((strength < 0) && (res > random(100))))
            {
                add_effect_message(NO_EFFECT_MESSAGE);
                break;
            }
            switch(type)
            {
            case "dex":
                do_tmp_stat(SS_DEX, strength);
                break;
            case "str":
                do_tmp_stat(SS_STR, strength);
                break;
            case "con":
                do_tmp_stat(SS_CON, strength);
                break;
            case "int":
                do_tmp_stat(SS_INT, strength);
                break;
            case "wis":
                do_tmp_stat(SS_WIS, strength);
                break;
            case "dis":
                do_tmp_stat(SS_DIS, strength);
                break;
            case "acid":
                add_resistance(MAGIC_I_RES_ACID, strength);
                break;
            case "air":
                add_resistance(MAGIC_I_RES_AIR, strength);
                break;
            case "cold":
                add_resistance(MAGIC_I_RES_COLD, strength);
                break;
            case "death":
                add_resistance(MAGIC_I_RES_DEATH, strength);
                break;
            case "electr":
                add_resistance(MAGIC_I_RES_ELECTRICITY, strength);
                break;
            case "earth":
                add_resistance(MAGIC_I_RES_EARTH, strength);
                break;
            case "fire":
                add_resistance(MAGIC_I_RES_FIRE, strength);
                break;
            case "illusion":
                add_resistance(MAGIC_I_RES_ILLUSION, strength);
                break;
            case "life":
                add_resistance(MAGIC_I_RES_LIFE, strength);
                break;
            case "magic":
                add_resistance(MAGIC_I_RES_MAGIC, strength);
                break;
            case "poison":
                add_resistance(MAGIC_I_RES_POISON, strength);
                break;
            case "water":
                add_resistance(MAGIC_I_RES_WATER, strength);
                break;
            default:
                write(NO_EFFECT_MESSAGE);
                break;
            }
            break;
        case HERB_POISONING:
            type = lower_case(effects[i + 1]);
            /* Allow for VBFC on the strength definition. */
            strength = this_object()->check_call(effects[i + 2]);
            if (poison_file)
            {
                poison = clone_object(poison_file);
                if (!poison)
                {
                    write(NO_EFFECT_MESSAGE);
                    break;
                }
                if (strength)
                    poison->set_strength(strength);
                if (type)
                    poison->set_poison_type(type);
                if (poison_damage)
                    poison->set_damage(poison_damage);
                poison->move(tp, 1);
                poison->start_poison();
            }
            else 
            {
                poison = clone_object(POISON_OBJECT);
                poison->set_strength(strength);
                poison->set_poison_type(type);
                if (poison_damage)
                    poison->set_damage(poison_damage);
                poison->move(tp, 1);
                poison->start_poison();
            }
            break;
        case HERB_CURING:
            type = lower_case(effects[i + 1]);
            /* Allow for VBFC on the strength definition. */
            strength = this_object()->check_call(effects[i + 2]);
            inv = all_inventory(tp);
            n = -1;
            a = 0;
            while(++n < sizeof(inv))
            {
                if (function_exists("cure_poison", inv[n]) != POISON_OBJECT)
                {
                    continue;
                }
                if (inv[n]->cure_poison( ({ type }),
                    ((strength / 2) + random(strength)) ))
                {
                    a++;
                    strength /= 2;
                }
            }
            if (a <= 0)
            {
                add_effect_message(NO_EFFECT_MESSAGE);
            }
            break;
        case HERB_SPECIAL:
            special_effect();
            break;
        default:
            add_effect_message(NO_EFFECT_MESSAGE);
            break;
        }
        i += 3;
    }
}

/* 
 * Function name: do_herb_effects
 * Description  : In this function the effect(s) of the herb are resolved.
 *                Read /doc/man/general/herbs and the header of set_herb_effect
 *                for more information.
 * Arguments    : int count - the number of herbs being ingested (default = 1)
 */
nomask int
do_herb_effects(int count = 1)
{
    messages = ({ });

    while(--count >= 0)
    {
        do_one_herb_effect();
    }

    /* If there are other messages, don't display the no-effect message. */
    if (sizeof(messages) > 1)
    {
        messages -= ({ NO_EFFECT_MESSAGE });
    }
    write(implode(messages, ""));
    return 1;
}

/*
 * Function name: set_effect
 * Description:   Give the herb or potion an effect (see herb.h)
 *                One effect per herb should be the norm, but adding one or
 *                two is ok, as long as they don't make the herb too good.
 * Arguments:     int herb_type - What type of effect
 *                string affect_type - And what exactly do we affect?
 *                mixed strength - The strength, may contain VBFC resolving to int.
 */
void
set_effect(int herb_type, string affect_type, mixed strength)
{
    effects = ({ herb_type, affect_type, strength });
}

/*
 * Function name: add_effect
 * Description:   Adds one more effect to a herb or potion
 *                One effect per herb should be the norm, but adding one or
 *                two is ok, as long as they don't make the herb too good.
 * Arguments:     int herb_type   - What type of effect
 *                string affect_type - And what exactly do we affect?
 *                mixed strength - The strength, may contain VBFC resolving to int.
 */
void
add_effect(int herb_type, string affect_type, mixed strength)
{
    effects += ({ herb_type, affect_type, strength });
}

/*
 * Function name: clear_effect
 * Description:   Remove all earlier set effects
 */
void
clear_effect()
{
    effects = ({ });
}

/*
 * Function name: query_effect
 * Description:   Get the effect array
 * Returns:       The array
 */
mixed *
query_effect()
{
    return secure_var(effects);
}

/*
 * Function name: set_duration
 * Description  : Set the duration of the herb effect to use instead of the
 *                default calculation. This is especially used for resistance
 *                effects. For temp stat effects, use the strength.
 * Arguments    : int i - the duration in seconds.
 */
public void
set_duration(int i)
{
    duration = i;
}

/*
 * Function name: query_duration
 * Description  : The set duration.
 * Returns      : int - the duration.
 */
 public int
 query_duration()
 {
     return duration;
 }

/*
 * Function name: set_poison_file
 * Description:   Set the file name of poison to use instead of standard
 * Arguments:     str - The file name
 */
void
set_poison_file(string str)
{
    poison_file = str;
}

/*
 * Function name: query_poison_file
 * Description:   Query the poison file (if any)
 * Returns:       The file name if set
 */
string
query_poison_file()
{
    return poison_file;
}

/*
 * Function name: set_poison_damage
 * Description:   Set the array to be sent to set_damage in the poison
 * Arguments:     damage - The damage array
 */
void
set_poison_damage(mixed *damage)
{
    poison_damage = damage;
}

/*
 * Function name: query_poison_damage
 * Description:   Query the poison damage array (if any)
 * Returns:       The damage array
 */
mixed *
query_poison_damage()
{
    return poison_damage;
}
