/*
 * /cmd/live/state.c
 *
 * General commands for finding out a livings state.
 * The following commands are:
 *
 * - adverbs
 * - compare
 * - email
 * - h
 * - health
 * - levels
 * - options
 * - second
 * - skills
 * - stats
 * - v
 * - vitals
 */

#pragma no_clone
#pragma no_inherit
#pragma save_binary
#pragma strict_types

inherit "/cmd/std/command_driver";

#include <adverbs.h>
#include <cmdparse.h>
#include <composite.h>
#include <const.h>
#include <files.h>
#include <filter_funs.h>
#include <formulas.h>
#include <gmcp.h>
#include <language.h>
#include <login.h>
#include <mail.h>
#include <macros.h>
#include <options.h>
#include <ss_types.h>
#include <state_desc.h>
#include <std.h>
#include <stdproperties.h>
#include <time.h>

#define SUBLOC_MISCEXTRADESC "_subloc_misc_extra"

/*
 * Global constants
 */
private mixed beauty_strings, compare_strings;
private string *stat_names, *health_state, *mana_state, *enc_weight;
private string *intox_state, *stuff_state, *soak_state, *progress_fact;
private string *brute_fact, *panic_state, *fatigue_state;
private mapping lev_map;
private mapping skillmap;

/* Prototype */
public int second(string str);


void
create()
{
    seteuid(getuid(this_object()));

    /* These global arrays are created once for all since they are used
     * quite often. They should be considered constant, so do not mess
     * with them. */
    skillmap =        SS_SKILL_DESC;
    stat_names =      SD_STAT_NAMES;

    compare_strings = ({ SD_COMPARE_STR, SD_COMPARE_DEX, SD_COMPARE_CON,
                         SD_COMPARE_INT, SD_COMPARE_WIS, SD_COMPARE_DIS,
                         SD_COMPARE_HIT, SD_COMPARE_PEN, SD_COMPARE_AC });

    beauty_strings =  ({ SD_BEAUTY_FEMALE, SD_BEAUTY_MALE });
    brute_fact =      SD_BRUTE_FACT;
    health_state =    SD_HEALTH;
    mana_state =      SD_MANA;
    panic_state =     SD_PANIC;
    fatigue_state =   SD_FATIGUE;
    soak_state =      SD_SOAK;
    stuff_state =     SD_STUFF;
    intox_state =     SD_INTOX;
    progress_fact =   SD_PROGRESS;
    enc_weight =      SD_ENC_WEIGHT;
    lev_map =         SD_LEVEL_MAP;
}

/* **************************************************************************
 * Return a proper name of the soul in order to get a nice printout.
 */
string
get_soul_id()
{
    return "state";
}

/* **************************************************************************
 * This is a command soul.
 */
int
query_cmd_soul()
{
    return 1;
}

/* **************************************************************************
 * The list of verbs and functions. Please add new in alfabetical order.
 */
mapping
query_cmdlist()
{
    return ([
              "adverbs":"adverbs",

              "compare":"compare",

              "email":"email",

              "h":"health",
              "health":"health",

              "levels":"levels",

              "options":"options",

              "second":"second",
              "skills":"show_skills",
              "stats":"show_stats",

              "v":"vitals",
              "vitals":"vitals",
            ]);
}

/*
 * Function name: using_soul
 * Description:   Called once by the living object using this soul. Adds
 *                  sublocations responsible for extra descriptions of the
 *                  living object.
 */
public void
using_soul(object live)
{
    live->add_subloc(SUBLOC_MISCEXTRADESC, file_name(this_object()));
    live->add_textgiver(file_name(this_object()));
}

/* **************************************************************************
 * Here follows some support functions.
 * **************************************************************************/

/*
 * Function name: beauty_text
 * Description:   Return corresponding text to a certain combination of
 *                appearance and opinion
 */
public string
beauty_text(int num, int sex)
{
    if (sex != G_FEMALE)
        sex = G_MALE;

    return GET_PROC_DESC(num, beauty_strings[sex]);
}

public string
show_subloc_size(object on, object for_obj)
{
    string race = on->query_race();

    if (!IN_ARRAY(race, RACES) ||
        !strlen(on->query_height_desc()))
    {
	return "";
    }

    return " " + on->query_height_desc() + " and " + on->query_width_desc() +
        " for " + LANG_ADDART(on->query_race_name()) + ".\n";
}

public string
show_subloc_looks(object on, object for_obj)
{
    if (on->query_prop(NPC_I_NO_LOOKS))
        return "";

    return (for_obj == on ? "You look " : capitalize(on->query_pronoun()) + " looks ") +
        beauty_text(for_obj->my_opinion(on),
        (on->query_gender() == G_FEMALE ? G_MALE : G_FEMALE)) + ".\n";
}

public string
show_subloc_fights(object on, object for_obj)
{
    object eob = (object)on->query_attack();

    return " fighting " + (eob == for_obj ? "you" :
        (string)eob->query_the_name(for_obj)) + ".\n";
}

public string
show_subloc_health(object on, object for_obj)
{
    return GET_NUM_DESC(on->query_hp(), on->query_max_hp(), health_state);
}

/*
 * Function name: show_subloc
 * Description:   Shows the specific sublocation description for a living
 */
public string
show_subloc(string subloc, object on, object for_obj)
{
    string res, cap_pronoun, cap_pronoun_verb, tmp;

    if (on->query_prop(TEMP_SUBLOC_SHOW_ONLY_THINGS))
        return "";

    if (for_obj == on)
    {
        res = "You are";
        cap_pronoun_verb = res;
        cap_pronoun = "You are ";
    }
    else
    {
        res = capitalize(on->query_pronoun()) + " is";
        cap_pronoun_verb = res;
        cap_pronoun = capitalize(on->query_pronoun()) + " seems to be ";
    }

    if (strlen(tmp = show_subloc_size(on, for_obj)))
        res += tmp;
    else
        res = "";

    res += show_subloc_looks(on, for_obj);

    if (on->query_attack())
        res += cap_pronoun_verb + show_subloc_fights(on, for_obj);

    res += cap_pronoun + show_subloc_health(on, for_obj) + ".\n";

    return res;
}

/*
 * Function name: get_proc_text
 * Description  : This is a service function kept for backward compatibility
 *                with existing domain code.
 *                For new code, please use one of the following macros:
 *                 GET_NUM_DESC, GET_NUM_DESC_SUB
 *                 GET_PROC_DESC, GET_PROC_DESC_SUB
 */
public varargs string
get_proc_text(int proc, mixed maindescs, int turnindex = 0, mixed subdescs = 0)
{
    if (sizeof(subdescs))
    {
        return GET_PROC_DESC_SUB(proc, maindescs, subdescs, turnindex);
    }
    else
    {
        return GET_PROC_DESC(proc, maindescs);
    }
}

/* **************************************************************************
 * Here follows the actual functions. Please add new functions in the
 * same order as in the function name list.
 * **************************************************************************/

/*
 * Adverbs - list the adverbs available to the game.
 */

/*
 * Function name: write_adverbs
 * Description  : Print the list of found adverbs to the screen of the player
 *                in a nice and tabulated way. It also prints the adverb
 *                replacements suitable for this list.
 * Arguments    : string *adverb_list - the list of adverbs to print.
 *                int total - the total number of adverbs.
 */
void
write_adverbs(string *adverb_list, int total)
{
    string  text;
    string *words;
    mapping replacements;
    int     index;
    int     size;

    size = sizeof(adverb_list);
    write("From the total of " + total + " adverbs, " + size +
        (size == 1 ? " matches" : " match") + " your inquiry.\n\n");

    index = -1;
    while(++index < ALPHABET_LEN)
    {
	words = filter(adverb_list,
            &wildmatch((ALPHABET[index..index] + "*"), ));

	if (!sizeof(words))
	{
	    continue;
	}

	if (strlen(words[0]) < 16)
	{
	    words[0] = (words[0] + "                ")[..15];
	}
	write(sprintf("%-76#s\n\n", implode(words, "\n")));
    }

    text = "";
    index = -1;
    size = sizeof(adverb_list);
    replacements = (mapping)ADVERBS_FILE->query_all_adverb_replacements();
    while(++index < size)
    {
        if (strlen(replacements[adverb_list[index]]))
        {
            text += sprintf("%-16s: %s\n", adverb_list[index],
                replacements[adverb_list[index]]);
        }
    }
    if (strlen(text))
    {
        write("Of these, the following adverbs are replaced with a more " +
            "suitable phrase:\n\n" + text);
    }
}

int
adverbs(string str)
{
    string *adverb_list;
    string *words;
    string *parts;
    mapping replacements;
    int     index;
    int     size;

    if (!strlen(str))
    {
        notify_fail("Syntax: adverbs all  or  list  or  *\n" +
            "        adverbs <letter>\n" +
            "        adverbs <wildcards>\n" +
            "        adverbs replace[ments]\n" +
            "Commas may be used to specify multiple letters or wildcards.\n");
        return 0;
    }

    /* Player wants to see the replacements only. */
    if (wildmatch("replace*", str))
    {
        write("\nThe following adverbs are replaced with a more suitable " +
            "phrase:\n\n");
        replacements = ADVERBS_FILE->query_all_adverb_replacements();
        adverb_list = sort_array(m_indices(replacements));
        index = -1;
        size = sizeof(adverb_list);
        while(++index < size)
        {
            write(sprintf("%-16s: %s\n", adverb_list[index],
                replacements[adverb_list[index]]));
        }
        return 1;
    }

    adverb_list = (string *)ADVERBS_FILE->query_all_adverbs();
    /* Player wants to see all adverbs. */
    if ((str == "list") ||
        (str == "all") ||
        (str == "*"))
    {
        write(".   Use the period to not use any adverb at all, not even " +
            "the default.\n\n");
        write_adverbs(adverb_list, sizeof(adverb_list));
        return 1;
    }

    /* Make lower case, remove spaces and split on commas. */
    parts = explode(implode(explode(lower_case(str), " "), ""), ",");
    index = -1;
    size = sizeof(parts);
    words = ({ });
    while(++index < size)
    {
        if (parts[index] == ".")
        {
            write(".   Use the period to not use any adverb at all, not " +
                "even the default.\n\n");
            continue;
        }
        if (strlen(parts[index]) == 1)
        {
            parts[index] += "*";
        }
        words |= filter(adverb_list, &wildmatch(parts[index], ));
    }

    if (!sizeof(words))
    {
        write("No adverbs found with those specifications.\n");
        return 1;
    }

    write_adverbs(sort_array(words), sizeof(adverb_list));
    return 1;
}

/*
 * Compare - compare the stats of two livings, or compare two items.
 */

void
compare_living(object living1, object living2)
{
    /* Allow the player high precision up to 3x their skill vs stat average */
    int skill = this_player()->query_skill(SS_APPR_MON) * 3;
    int seed  = atoi(OB_NUM(living1)) + atoi(OB_NUM(living2));
    int index = -1;
    int stat1;
    int stat2;
    int swap;
    string str1 = ((this_player() == living1) ? "you" :
                  living1->query_the_name(this_player()));
    string str2 = living2->query_the_name(this_player());

    /* Someone might actually want to compare two livings with the same
     * description.
     */
    if (str1 == str2)
    {
        str1 = "the first " + living1->query_nonmet_name();
        str2 = "the second " + living2->query_nonmet_name();
    }

    /* Use high precision for avg up to skill, low precision for remainder */
    int high_precision;
    int low_precision;
     /* Find the average of the query_average_stat up to their skill.
        The player has high precision on their own stats regardless of skill.
      */
    high_precision = (living1->query_average_stat() > skill
        && (this_player() != living1)
        ? skill : living1->query_average_stat());
    high_precision += (living2->query_average_stat() > skill
        ? skill : living2->query_average_stat());
    high_precision /= 2;

    /* Find the average of the query_average_stat above their skill */
    low_precision = (living1->query_average_stat() > skill
        && (this_player() != living1)
        ? living1->query_average_stat() - skill : 0);
    low_precision += (living2->query_average_stat() > skill
        ? living2->query_average_stat() - skill : 0);
    low_precision /= 2;


    /* Loop over all known stats. */
    while(++index < SS_NO_EXP_STATS)
    {
        stat1 = living1->query_stat(index)
            + random(high_precision / 10, seed)
            + random(low_precision / 2, seed + 1);
        stat2 = living2->query_stat(index)
            + random(high_precision / 10, seed + 27)
            + random(low_precision / 2, seed + 28);

        if (stat1 > stat2)
        {
            stat1 = (100 - ((80 * stat2) / stat1));
            swap = 0;
        }
        else
        {
            stat1 = (100 - ((80 * stat1) / stat2));
            swap = 1;
        }
        stat1 = ((stat1 * sizeof(compare_strings[index])) / 100);
        stat1 = ((stat1 > 3) ? 3 : stat1);

        /* Print the message. */
        if (swap)
        {
            write(capitalize(str2) + " is " +
                compare_strings[index][stat1] + " " + str1 + ".\n");
        }
        else
        {
            write(((str1 == "you") ? "You are " :
                (capitalize(str1) + " is ")) +
                compare_strings[index][stat1] + " " + str2 + ".\n");
        }
    }
}


/*
 * Function name: compare_weapon
 * Description  : Compares the stats of two weapons.
 * Arguments    : object weapon1 - the left hand side to compare.
 *                object weapon2 - the right hand side to compare.
 */
void
compare_weapon(object weapon1, object weapon2)
{
    /* Weapon skill and appraise are both used, so use half their average */
    int skill = (this_player()->query_skill(SS_APPR_OBJ) +
        this_player()->query_skill(SS_WEP_FIRST + weapon1->query_wt())) / 4;
    int seed = atoi(OB_NUM(weapon1)) + atoi(OB_NUM(weapon2));
    int swap;
    int stat1;
    int stat2;
    string str1;
    string str2;
    string print1;
    string print2;
    object tmp;

    /* Always use the same order. After all, we don't want "compare X with Y"
     * to differ from "compare Y with X".
     */
    if (OB_NUM(weapon1) > OB_NUM(weapon2))
    {
        tmp = weapon1;
        weapon1 = weapon2;
        weapon2 = tmp;
        swap = 1;
    }

    str1 = weapon1->short(this_player());
    str2 = weapon2->short(this_player());

    /* Some people will want to compare items with the same description. */
    if (str1 == str2)
    {
        if (swap)
        {
            str1 = "first " + str1;
            str2 = "second " + str2;
        }
        else
        {
            str2 = "first " + str2;
            str1 = "second " + str1;
        }
    }

    /* Use high precision for hit up to skill, low precision for remainder */
    int high_precision;
    int low_precision;

    /* Find the average of the query_hit up to their skill */
    high_precision = (weapon1->query_hit() > skill
        ? skill : weapon1->query_hit());
    high_precision += (weapon2->query_hit() > skill
        ? skill : weapon2->query_hit());
    high_precision /= 2;

    /* Find the average of the query_hit above their skill */
    low_precision = (weapon1->query_hit() > skill
        ? weapon1->query_hit() - skill : 0);
    low_precision += (weapon2->query_hit() > skill
        ? weapon2->query_hit() - skill : 0);
    low_precision /= 2;

    /* Gather the to-hit values. */
    stat1 = weapon1->query_hit() + random(high_precision / 10, seed)
        + random(low_precision / 2, seed + 1);
    stat2 = weapon2->query_hit() + random(high_precision / 10, seed + 27)
        + random(low_precision / 2, seed + 28);

    if (stat1 > stat2)
    {
        stat1 = (100 - ((80 * stat2) / stat1));
        print1 = str1;
        print2 = str2;
    }
    else
    {
        stat1 = (100 - ((80 * stat1) / stat2));
        print1 = str2;
        print2 = str1;
    }

    stat1 = ((stat1 * sizeof(compare_strings[6])) / 100);
    write("Hitting someone with the " + print1 + " is " +
        compare_strings[6][((stat1 > 3) ? 3 : stat1)] + " the " +
        print2 + " and ");

    /* Find the average of the query_pen up to their skill */
    high_precision = (weapon1->query_pen() > skill
        ? skill : weapon1->query_pen());
    high_precision += (weapon2->query_pen() > skill
        ? skill : weapon2->query_pen());
    high_precision /= 2;

    /* Find the average of the query_pen above their skill */
    low_precision = (weapon1->query_pen() > skill
        ? weapon1->query_pen() - skill : 0);
    low_precision += (weapon2->query_hit() > skill
        ? weapon2->query_pen() - skill : 0);
    low_precision /= 2;

    /* Compare the penetration values. */
    stat1 = weapon1->query_pen() + random(high_precision / 10, seed)
        + random(low_precision / 2, seed + 1);
    stat2 = weapon2->query_pen() + random(high_precision / 10, seed + 27)
        + random(low_precision / 2, seed + 28);

    if (stat1 > stat2)
    {
        stat1 = (100 - ((80 * stat2) / stat1));
        print1 = str1;
        print2 = str2;
    }
    else
    {
        stat1 = (100 - ((80 * stat1) / stat2));
        print1 = str2;
        print2 = str1;
    }

    stat1 = ((stat1 * sizeof(compare_strings[7])) / 100);
    write("damage inflicted by the " + print1 + " is " +
        compare_strings[7][((stat1 > 3) ? 3 : stat1)] + " the " +
        print2 + ".\n");
}

/*
 * Function name: compare_unarmed_enhancer
 * Description  : Compares the stats of two unarmed enhancers.
 * Arguments    : object enh1 - the left hand side to compare.
 *                object enh2 - the right hand side to compare.
 */
void
compare_unarmed_enhancer(object enh1, object enh2)
{
    /* Weapon skill and appraise are both used, so use half their average */
    int skill = (this_player()->query_skill(SS_APPR_OBJ) +
        this_player()->query_skill(SS_UNARM_COMBAT)) / 4;
    int seed = atoi(OB_NUM(enh1)) + atoi(OB_NUM(enh2));
    int swap;
    int stat1;
    int stat2;
    string str1;
    string str2;
    string print1;
    string print2;
    object tmp;

    /* Always use the same order. After all, we don't want "compare X with Y"
     * to differ from "compare Y with X".
     */
    if (OB_NUM(enh1) > OB_NUM(enh2))
    {
        tmp = enh1;
        enh1 = enh2;
        enh2 = tmp;
        swap = 1;
    }

    str1 = enh1->short(this_player());
    str2 = enh2->short(this_player());

    /* Some people will want to compare items with the same description. */
    if (str1 == str2)
    {
        if (swap)
        {
            str1 = "first " + str1;
            str2 = "second " + str2;
        }
        else
        {
            str2 = "first " + str2;
            str1 = "second " + str1;
        }
    }

    /* Use high precision for hit up to skill, low precision for remainder */
    int high_precision;
    int low_precision;

    /* Find the average of the query_hit up to their skill */
    high_precision = (enh1->query_hit() > skill
        ? skill : enh1->query_hit());
    high_precision += (enh2->query_hit() > skill
        ? skill : enh2->query_hit());
    high_precision /= 2;

    /* Find the average of the query_hit above their skill */
    low_precision = (enh1->query_hit() > skill
        ? enh1->query_hit() - skill : 0);
    low_precision += (enh2->query_hit() > skill
        ? enh2->query_hit() - skill : 0);
    low_precision /= 2;

    /* Gather the to-hit values. */
    stat1 = enh1->query_hit() + random(high_precision / 10, seed)
        + random(low_precision / 2, seed + 1);
    stat2 = enh2->query_hit() + random(high_precision / 10, seed + 27)
        + random(low_precision / 2, seed + 28);

    if (stat1 > stat2)
    {
        stat1 = (100 - ((80 * stat2) / stat1));
        print1 = str1;
        print2 = str2;
    }
    else
    {
        stat1 = (100 - ((80 * stat1) / stat2));
        print1 = str2;
        print2 = str1;
    }

    stat1 = ((stat1 * sizeof(compare_strings[6])) / 100);
    write("Hitting someone with the " + print1 + " is " +
        compare_strings[6][((stat1 > 3) ? 3 : stat1)] + " the " +
        print2 + " and ");

    /* Find the average of the query_pen up to their skill */
    high_precision = (enh1->query_pen() > skill
        ? skill : enh1->query_pen());
    high_precision += (enh2->query_pen() > skill
        ? skill : enh2->query_pen());
    high_precision /= 2;

    /* Find the average of the query_pen above their skill */
    low_precision = (enh1->query_pen() > skill
        ? enh1->query_pen() - skill : 0);
    low_precision += (enh2->query_hit() > skill
        ? enh2->query_pen() - skill : 0);
    low_precision /= 2;

    /* Compare the penetration values. */
    stat1 = enh1->query_pen() + random(high_precision / 10, seed)
        + random(low_precision / 2, seed + 1);
    stat2 = enh2->query_pen() + random(high_precision / 10, seed + 27)
        + random(low_precision / 2, seed + 28);

    if (stat1 > stat2)
    {
        stat1 = (100 - ((80 * stat2) / stat1));
        print1 = str1;
        print2 = str2;
    }
    else
    {
        stat1 = (100 - ((80 * stat1) / stat2));
        print1 = str2;
        print2 = str1;
    }

    stat1 = ((stat1 * sizeof(compare_strings[7])) / 100);
    write("damage inflicted by the " + print1 + " is " +
        compare_strings[7][((stat1 > 3) ? 3 : stat1)] + " the " +
        print2 + ".\n");
}

/*
 * Function name: compare_living_to_unarmed_enhancer
 * Description  : Compares attack_ids of living to unarmed enhancer.
 * Arguments    : object living1 - the left hand side to compare.
 *                object enh2 - the right hand side to compare.
 */
void
compare_living_to_unarmed_enhancer(object living1, object enh2)
{
    /* Weapon skill and appraise are both used, so use half their average */
    int skill = (this_player()->query_skill(SS_APPR_OBJ) +
        this_player()->query_skill(SS_UNARM_COMBAT)) / 4;
    int seed = atoi(OB_NUM(living1)) + atoi(OB_NUM(enh2));
    int stat1;
    int stat2;
    string str1;
    string str2;
    string print1;
    string print2;
    object cobj = living1->query_combat_object();

    str2 = enh2->short(this_player());

    foreach (int aid : cobj->query_attack_id())
    {
        if (aid == W_BOTH)
        {
            continue;
        }
        mixed attack = cobj->query_attack(aid);
        int hid = living1->cr_convert_attack_id_to_slot(aid);
        if (!attack || (hid & enh2->query_at()) == 0)
        {
            continue;
        }

        /* Get hit and pen */
        int hit = attack[0];
        /* Array of pens */
        int *pens = attack[1];
        /* Damage type */
        int dt = attack[2];
        int *ac = ({});
        
        if (dt & W_IMPALE)
        {
            ac += ({ pens[0] });
        }
        if (dt & W_SLASH)
        {
            ac += ({ pens[1] });   
        }
        if (dt & W_BLUDGEON)
        {
            ac += ({ pens[2] });   
        }
        if (sizeof(ac) == 0)
        {
            // Should never happen
            continue;
        }
        int pen = one_of_list(ac);

        /* Attack desc */
        str1 = living1->cr_attack_desc(aid);
        if (this_player() == living1)
        {
            str1 = "your " + str1;
        }
        else
        {
            str1 = living1->query_the_possessive_name() + " " + str1;
        }
        mixed arm = living1->query_armour(hid);
        if (objectp(arm) && IS_UNARMED_ENH_OBJECT(arm))
        {
            str1 = str1 + " wearing " + arm->short(this_player());
        }

        /* Use high precision for hit up to skill, low precision for remainder */
        int high_precision;
        int low_precision;

        /* Find the average of the query_hit up to their skill */
        high_precision = (hit > skill
            ? skill : hit);
        high_precision += (enh2->query_hit() > skill
            ? skill : enh2->query_hit());
        high_precision /= 2;

        /* Find the average of the query_hit above their skill */
        low_precision = (hit > skill
            ? hit - skill : 0);
        low_precision += (enh2->query_hit() > skill
            ? enh2->query_hit() - skill : 0);
        low_precision /= 2;

        /* Gather the to-hit values. */
        stat1 = hit + random(high_precision / 10, seed)
            + random(low_precision / 2, seed + 1);
        stat2 = enh2->query_hit() + random(high_precision / 10, seed + 27)
            + random(low_precision / 2, seed + 28);

        if (stat1 > stat2)
        {
            stat1 = (100 - ((80 * stat2) / stat1));
            print1 = str1;
            print2 = str2;
        }
        else
        {
            stat1 = (100 - ((80 * stat1) / stat2));
            print1 = str2;
            print2 = str1;
        }

        stat1 = ((stat1 * sizeof(compare_strings[6])) / 100);
        write("Hitting someone with " + print1 + " is " +
            compare_strings[6][((stat1 > 3) ? 3 : stat1)] + " the " +
            print2 + " and ");

        /* Find the average of the query_pen up to their skill */
        high_precision = (pen > skill
            ? skill : pen);
        high_precision += (enh2->query_pen() > skill
            ? skill : enh2->query_pen());
        high_precision /= 2;

        /* Find the average of the query_pen above their skill */
        low_precision = (pen > skill
            ? pen - skill : 0);
        low_precision += (enh2->query_hit() > skill
            ? enh2->query_pen() - skill : 0);
        low_precision /= 2;

        /* Compare the penetration values. */
        stat1 = pen + random(high_precision / 10, seed)
            + random(low_precision / 2, seed + 1);
        stat2 = enh2->query_pen() + random(high_precision / 10, seed + 27)
            + random(low_precision / 2, seed + 28);

        if (stat1 > stat2)
        {
            stat1 = (100 - ((80 * stat2) / stat1));
            print1 = str1;
            print2 = str2;
        }
        else
        {
            stat1 = (100 - ((80 * stat1) / stat2));
            print1 = str2;
            print2 = str1;
        }

        stat1 = ((stat1 * sizeof(compare_strings[7])) / 100);
        write("damage inflicted by " + print1 + " is " +
            compare_strings[7][((stat1 > 3) ? 3 : stat1)] + " the " +
            print2 + ".\n");
    }
}

/*
 * Function name: compare_armour
 * Description  : Compares the stats of two armour.
 * Arguments    : object armour1 - the left hand side to compare.
 *                object armour2 - the right hand side to compare.
 */
void
compare_armour(object armour1, object armour2)
{
    int skill = (this_player()->query_skill(SS_APPR_OBJ)) / 2;
    int seed  = atoi(OB_NUM(armour1)) + atoi(OB_NUM(armour2));
    int swap;
    int stat1;
    int stat2;
    string str1;
    string str2;
    string print1;
    string print2;
    object tmp;

    /* Always use the same order. After all, we don't want "compare X with Y"
     * to differ from "compare Y with X".
     */
    if (OB_NUM(armour1) > OB_NUM(armour2))
    {
        tmp = armour1;
        armour1 = armour2;
        armour2 = tmp;
    swap = 1;
    }

    str1 = armour1->short(this_player());
    str2 = armour2->short(this_player());

    /* Some people will want to compare items with the same description. */
    if (str1 == str2)
    {
        if (swap)
        {
            str1 = "first " + str1;
            str2 = "second " + str2;
        }
        else
        {
            str2 = "first " + str2;
            str1 = "second " + str1;
        }
    }

    /* Use high precision for AC up to skill, low precision for remainder */
    int high_precision;
    int low_precision;
     /* Find the average of the query_ac up to their skill */
    high_precision = (armour1->query_ac() > skill
        ? skill : armour1->query_ac());
    high_precision += (armour2->query_ac() > skill
        ? skill : armour2->query_ac());
    high_precision /= 2;

    /* Find the average of the query_ac above their skill */
    low_precision = (armour1->query_ac() > skill
        ? armour1->query_ac() - skill : 0);
    low_precision += (armour2->query_ac() > skill
        ? armour2->query_ac() - skill : 0);
    low_precision /= 2;

     /* Gather the armour class. */
    stat1 = armour1->query_ac() + random(high_precision / 10, seed)
        + random(low_precision / 2, seed + 1);
    stat2 = armour2->query_ac() + random(high_precision / 10, seed + 27)
        + random(low_precision / 2, seed + 28);

    if (stat1 > stat2)
    {
        stat1 = (100 - ((80 * stat2) / stat1));
        print1 = str1;
        print2 = str2;
    }
    else
    {
        stat1 = (100 - ((80 * stat1) / stat2));
        print1 = str2;
        print2 = str1;
    }

    stat1 = ((stat1 * sizeof(compare_strings[8])) / 100);
    write("The " + print1 + " gives " +
        compare_strings[8][((stat1 > 3) ? 3 : stat1)] + " the " +
        print2 + ".\n");

    if (IS_UNARMED_ENH_OBJECT(armour1) && IS_UNARMED_ENH_OBJECT(armour2) &&
        (armour1->query_at() & armour2->query_at() & (A_HANDS | A_FEET)) != 0)
    {
        compare_unarmed_enhancer(armour1, armour2);
    }
}

/*
 * Function name:   compare_projectiles
 * Description:     Compares the stats of two projectiles.
 * Arguments:       (object) projectile1 - the left hand side to compare.
 *                  (object) projectile2 - the right hand side to compare.
 * Returns:         Nothing
 */
void
compare_projectiles(object projectile1, object projectile2)
{
    object  tmp;
    string  str1, str2, print1, print2;
    int     skill, seed, swap, stat1, stat2;

    /* Weapon skill and appraise are both used, so use half their average */
    skill = (this_player()->query_skill(SS_APPR_OBJ) +
            this_player()->query_skill(SS_WEP_MISSILE)) / 4;
    seed = atoi(OB_NUM(projectile1)) + atoi(OB_NUM(projectile2));

    /* Always use the same order. After all, we don't want "compare X with Y"
     * to differ from "compare Y with X".
     */
    if (OB_NUM(projectile1) > OB_NUM(projectile2))
    {
        tmp = projectile1;
        projectile1 = projectile2;
        projectile2 = tmp;
        swap = 1;
    }

    str1 = projectile1->singular_short(this_player());
    str2 = projectile2->singular_short(this_player());

    /* Some people will want to compare items with the same description. */
    if (str1 == str2)
    {
        if (swap)
        {
            str1 = "first " + str1;
            str2 = "second " + str2;
        }
        else
        {
            str2 = "first " + str2;
            str1 = "second " + str1;
        }
    }

    /* Use high precision for hit up to skill, low precision for remainder */
    int high_precision;
    int low_precision;
     /* Find the average of the query_hit up to their skill */
    high_precision = (projectile1->query_hit() > skill
        ? skill : projectile1->query_hit());
    high_precision += (projectile2->query_hit() > skill
        ? skill : projectile2->query_hit());
    high_precision /= 2;

    /* Find the average of the query_hit above their skill */
    low_precision = (projectile1->query_hit() > skill
        ? projectile1->query_hit() - skill : 0);
    low_precision += (projectile2->query_hit() > skill
        ? projectile2->query_hit() - skill : 0);
    low_precision /= 2;

    /* Gather the to-hit values. */
    stat1 = projectile1->query_hit() + random(high_precision / 10, seed)
        + random(low_precision / 2, seed + 1);
    stat2 = projectile2->query_hit() + random(high_precision / 10, seed + 27)
        + random(low_precision / 2, seed + 28);

    if (stat1 > stat2)
    {
        stat1 = (100 - ((80 * stat2) / stat1));
        print1 = str1;
        print2 = str2;
    }
    else
    {
        stat1 = (100 - ((80 * stat1) / stat2));
        print1 = str2;
        print2 = str1;
    }

    stat1 = ((stat1 * sizeof(compare_strings[6])) / 100);
    write("Hitting someone with the " + print1 + " is " +
        compare_strings[6][((stat1 > 3) ? 3 : stat1)] + " the " +
        print2 + " and ");

    /* Find the average of the query_pen up to their skill */
    high_precision = (projectile1->query_pen() > skill
        ? skill : projectile1->query_pen());
    high_precision += (projectile2->query_pen() > skill
        ? skill : projectile2->query_pen());
    high_precision /= 2;

    /* Find the average of the query_pen above their skill */
    low_precision = (projectile1->query_pen() > skill
        ? projectile1->query_pen() - skill : 0);
    low_precision += (projectile2->query_pen() > skill
        ? projectile2->query_pen() - skill : 0);
    low_precision /= 2;

    /* Compare the penetration values. */
    stat1 = projectile1->query_pen() + random(high_precision / 10, seed)
        + random(low_precision / 2, seed + 1);
    stat2 = projectile2->query_pen() + random(high_precision / 10, seed + 27)
        + random(low_precision / 2, seed + 28);

    if (stat1 > stat2)
    {
        stat1 = (100 - ((80 * stat2) / stat1));
        print1 = str1;
        print2 = str2;
    }
    else
    {
        stat1 = (100 - ((80 * stat1) / stat2));
        print1 = str2;
        print2 = str1;
    }

    stat1 = ((stat1 * sizeof(compare_strings[7])) / 100);
    write("damage inflicted by the " + print1 + " is " +
        compare_strings[7][((stat1 > 3) ? 3 : stat1)] + " the " +
        print2 + ".\n");
} /* compare_projectiles */

int
compare(string str)
{
    string str1;
    string str2;
    object *oblist;
    object obj1;
    object obj2;
    int str1_is_fist, str1_is_foot, str1_is_unarmed;

    if (!strlen(str) ||
        sscanf(str, "%s with %s", str1, str2) != 2)
    {
        notify_fail("Compare <whom/what> with <whom/what>?\n");
        return 0;
    }

    str1_is_fist = str1 == "fists" || str1 == "fist";
    str1_is_foot = str1 == "foot" || str1 == "feet";
    str1_is_unarmed = str1_is_fist || str1_is_foot;
        ;
    /* If we compare 'stats' or 'fists/feet', then we are the left
     * party.
     */
    if (str1 == "stats" || str1_is_unarmed)
    {
        obj1 = this_player();
    }
    else
    /* Else, see the left hand side of whom/what we want to compare. */
    {
        oblist = parse_this(str1, "[the] %i");
        if (sizeof(oblist) != 1)
        {
            notify_fail("Compare <whom/what> with <whom/what>?\n");
            return 0;
        }
        obj1 = oblist[0];
    }

    if (str2 == "enemy" && !str1_is_unarmed)
    {
        if (!objectp(obj1->query_attack()))
        {
            notify_fail(((obj1 == this_player()) ? "You are" :
                (obj1->query_The_name(this_player()) + " is")) +
                " not fighting anyone.\n");
            return 0;
        }

        if (!CAN_SEE_IN_ROOM(this_player()) ||
            !CAN_SEE(this_player(), obj1->query_attack()))
        {
            notify_fail("You cannot see " + ((obj1 == this_player()) ? "your" :
                (obj1->query_the_name(this_player()) + "'s")) + " enemy.\n");
            return 0;
        }

        oblist = ({ obj1->query_attack() });
    }
    else
    /* Get the right hand side of what we want to compare. */
    {
        oblist = parse_this(str2, "[the] %i");
        if (sizeof(oblist) != 1)
        {
            notify_fail("Compare <whom/what> with <whom/what>?\n");
            return 0;
        }
    }
    obj2 = oblist[0];

    /* Yes, people will want to do the obvious. */
    if (obj1 == obj2)
    {
        notify_fail("It is pointless to compare something to itself.\n");
        return 0;
    }

    /* Compare the stats of two livings or
     * fists or feet against unarmed enhancers.
     */
    if (living(obj1))
    {
        if (str1_is_unarmed)
        {
            if (IS_UNARMED_ENH_OBJECT(obj2))
            {
                if (str1_is_fist)
                {
                    if (obj1 == obj2->query_worn())
                    {
                        notify_fail("It is pointless to compare something " +
                            "to fists wearing itself.\n");
                        return 0;
                    }
                    if (obj1->query_weapon(W_RIGHT) ||
                        obj1->query_weapon(W_LEFT) ||
                        obj1->query_weapon(W_BOTH))
                    {
                        notify_fail("The " + obj2->short(this_player()) +
                            " cannot be compared to fists " +
                            "while a weapon is wielded.\n");
                        return 0;
                    }
                    if ((obj2->query_at() & A_HANDS) == 0)
                    {
                        notify_fail("The " + obj2->short(this_player()) +
                            " cannot be compared to fists.\n");
                        return 0;
                    }
                }
                else if (str1_is_foot)
                {
                    if (obj1 == obj2->query_worn())
                    {
                        notify_fail("It is pointless to compare something " +
                            "to feet wearing itself.\n");
                        return 0;
                    }
                    if (obj1->query_weapon(W_FOOTR) ||
                        obj1->query_weapon(W_FOOTL))
                    {
                        notify_fail("The " + obj2->short(this_player()) +
                            " cannot be compared to feet " +
                            "while a weapon is wielded.\n");
                        return 0;
                    }
                    if ((obj2->query_at() & A_FEET) == 0)
                    {
                        notify_fail("The " + obj2->short(this_player()) +
                            " cannot be compared to feet.\n");
                        return 0;
                    }
                }
            }
            else
            {
                notify_fail("The " + obj2->short(this_player()) +
                    " does not enhance unarmed combat.\n");
                return 0;
            }
            
            compare_living_to_unarmed_enhancer(obj1, obj2);
            return 1;
        }

        if (!living(obj2))
        {
            if (obj1 == this_player())
            {
                notify_fail("You can only compare your stats to those of " +
                    "another living.\n");
                return 0;
            }
            notify_fail("The stats of " +
                obj1->query_the_name(this_player()) +
                " can only be compared to those of another living.\n" );
            return 0;
        }

        compare_living(obj1, obj2);
        return 1;
    }

    str1 = function_exists("create_object", obj1);
    str2 = function_exists("create_object", obj2);

    /* Compare two weapons. */
    if (str1 == WEAPON_OBJECT)
    {
        if (str2 != WEAPON_OBJECT)
        {
            notify_fail("The " + obj1->short(this_player()) +
                " can only be compared to another weapon.\n");
            return 0;
        }

        if (obj1->query_wt() != obj2->query_wt())
        {
            notify_fail("The " + obj1->short(this_player()) +
                " can only be compared to another weapon of the same " +
                "type.\n");
            return 0;
        }

        compare_weapon(obj1, obj2);
        return 1;
    }

    /* Compare two armours. */
    if (str1 == ARMOUR_OBJECT)
    {
        if (str2 != ARMOUR_OBJECT)
        {
            notify_fail("The " + obj1->short(this_player()) +
                " can only be compared to another armour.\n");
            return 0;
        }

        if ((obj1->query_at() & obj2->query_at()) == 0)
        {
            notify_fail("The " + obj1->short(this_player()) +
                " can only be compared to another armour of similar " +
                "type.\n");
            return 0;
        }

        compare_armour(obj1, obj2);
        return 1;
    }

    /* Compare two projectiles. */
    if (IS_PROJECTILE_OBJECT(obj1))
    {
        if (function_exists("create_projectile", obj2) !=
            function_exists("create_projectile", obj1))
        {
            notify_fail("The " + obj1->short(this_player()) + " can only be "
            + "compared to another projectile of the same type.\n");
            return 0;
        }

        compare_projectiles(obj1, obj2);
        return 1;
    }

    notify_fail("It does not seem possible to compare " +
        LANG_THESHORT(obj1) + " with " + LANG_THESHORT(obj2) + ".\n");
    return 0;
}


/*
 * email - Display/change your email address.
 */
int
email(string str)
{
    if (!stringp(str))
    {
        write("Your current email address is: " +
            this_player()->query_mailaddr() + "\n");
        return 1;
    }

    if (this_player()->set_mailaddr(str)) {
        write("Changed your email address.\n");
    }

    return 1;
}

/*
 * health - Display your health or that of someone else.
 */
int
health(string str)
{
    object *oblist = ({ });
    int display_self;

    str = (stringp(str) ? str : "");

    switch(str)
    {
    case "":
        display_self = 1;
        break;

    case "enemy":
        if (!objectp(this_player()->query_attack()))
        {
            notify_fail("You are not fighting anyone.\n");
            return 0;
        }

        if (!CAN_SEE_IN_ROOM(this_player()) ||
            !CAN_SEE(this_player(), this_player()->query_attack()))
        {
            notify_fail("You cannot see your enemy.\n");
            return 0;
        }

        oblist = ({ this_player()->query_attack() });
        break;

    case "team":
        oblist = this_player()->query_team_others();
        if (!sizeof(oblist))
        {
            str = "noteam";
            write("You are not in a team with anyone.\n");
        }

        if (!CAN_SEE_IN_ROOM(this_player()))
        {
            notify_fail("It is too dark to see.\n");
            return 0;
        }

        /* Only the team members in the environment of the player. */
        oblist &= all_inventory(environment(this_player()));

        oblist = FILTER_CAN_SEE(oblist, this_player());
        if (!sizeof(oblist) &&
            (str != "noteam"))
        {
            write("You cannot determine the health of any team member.\n");
        }

        display_self = 1;
        break;

    case "all":
        display_self = 1;
        /* Intentionally no "break". We need to catch "default" too. */

    default:
        oblist = parse_this(str, "[the] %l") - all_inventory(this_player());
        if (!display_self &&
            !sizeof(oblist))
        {
            notify_fail("Determine whose health?\n");
            return 0;
        }
    }

    if (display_self)
    {
        if (this_player()->query_ghost())
        {
            write("You would be feeling much better with an actual body.\n");
        }
        else
        {
            write("You are physically " +
                GET_NUM_DESC(this_player()->query_hp(), this_player()->query_max_hp(), health_state) +
                " and mentally " +
                GET_NUM_DESC(this_player()->query_mana(), this_player()->query_max_mana(), mana_state) +
                ".\n");
        }
    }
    else if (!sizeof(oblist))
    {
        write("You can only determine the health of those you can see.\n");
        return 1;
    }

    foreach(object obj: oblist)
    {
        write(obj->query_The_name(this_player()) + " is " +
            show_subloc_health(obj, this_player()) + ".\n");
    }
    return 1;
}

/*
 * levels - Print a summary of different levels of different things
 */
public int
levels(string str)
{
    string *ix, *levs;

    ix = sort_array(m_indices(lev_map));

    if (!str)
    {
        notify_fail("Available level descriptions:\n" +
            break_string(COMPOSITE_WORDS(ix) + ".", 77, 3) + "\n");
        return 0;
    }

    /* A nice gesture of backward compatibility for lazy mortals and their scripts. */
    if (str == "improve") str = "progress";

    levs = lev_map[str];
    if (!sizeof(levs))
    {
        notify_fail("No such level descriptions. Available:\n" +
            break_string(COMPOSITE_WORDS(ix) + ".", 77, 3) + "\n");
        return 0;
    }
    if (str == "intox" && this_player()->query_prop(LIVE_S_EXTENDED_INTOX))
    {
        levs += ({ this_player()->query_prop(LIVE_S_EXTENDED_INTOX) });
    }
    if (str == "stuffed" && this_player()->query_prop(LIVE_S_EXTENDED_STUFF))
    {
        levs += ({ this_player()->query_prop(LIVE_S_EXTENDED_STUFF) });
    }

    write("Level descriptions for: " + capitalize(str) + "\n" +
        break_string(COMPOSITE_WORDS(levs) + ".", 77, 3) + "\n");
    return 1;
}

/* **************************************************************************
 * options - Change/view the options
 */
nomask int
options(string arg)
{
    string *args, rest;
    int     wi, proc;
    int     client;

    if (!stringp(arg))
    {
        /* Please keep this list sorted (on the associated text). */
        options("autowrap");
        options("brief");
        options("echo");
        options("gagmisses");
        options("intimate");
//      options("merciful");
        options("morelen");
        options("giftfilter");
        options("screenwidth");
        options("see");
        options("showunmet");
        options("silentships");
        options("inventory");
        options("unarmed");
        options("web");
        options("wimpy");
	if (this_player()->query_wiz_level())
	{
	    write("\n");
	    options("autoline");
	    options("autopwd");
	    options("alwaysknown");
	    options("allcommune");
	    options("timestamp");
	}
        return 1;
    }

    args = explode(arg, " ");
    if (sizeof(args) == 1)
    {
        switch(arg)
        {
        case "morelen":
        case "more":
	    client = this_player()->query_prop(OPT_MORE_LEN);
            write("More length         <morelen>: " +
                this_player()->query_option(OPT_MORE_LEN, 1) +
		(client ? "  (client = " + client + ")" : "") + "\n");
            break;

        case "screenwidth":
        case "sw":
	    client = this_player()->query_prop(OPT_SCREEN_WIDTH);
            wi = this_player()->query_option(OPT_SCREEN_WIDTH, 1);
            write("Screen width    <screenwidth>: " +
	        ((wi > -1) ? ("" + wi) : "Off") +
		(client ? "  (client = " + client + ")" : "") + "\n");
            break;

        case "brief":
            write("Brief display         <brief>: " +
                (this_player()->query_option(OPT_BRIEF) ? "On" : "Off") + "\n");
            break;

        case "echo":
            write("Echo commands          <echo>: " +
                (this_player()->query_option(OPT_ECHO) ? "On" : "Off") + "\n");
            break;

        case "wimpy":
            wi = this_player()->query_whimpy();
            write("Wimp from combat at   <wimpy>: ");
            if (wi)
            {
                wi = wi * sizeof(health_state) / 100;
                write(capitalize(health_state[wi]) + "\n");
            }
            else
                write("Brave (do not wimp)\n");
            break;

        case "see":
        case "fights":
            write("See other fights     <fights>: " +
                (this_player()->query_option(OPT_NO_FIGHTS) ? "Off" : "On") + "\n");
            break;

        case "unarmed":
            write("Unarmed combat      <unarmed>: " +
                (this_player()->query_option(OPT_UNARMED_OFF) ? "Off" : "On") + "\n");
            break;

        case "gagmisses":
            write("Gag your misses   <gagmisses>: " +
                (this_player()->query_option(OPT_GAG_MISSES) ? "On" : "Off") + "\n");
            break;

	case "gift":
        case "giftfilter":
            write("Refuse gifts     <giftfilter>: " +
                (this_player()->query_option(OPT_GIFT_FILTER) ? "On" : "Off") + "\n");
            break;

        case "intimate":
            write("Intimate behaviour <intimate>: " +
                (this_player()->query_option(OPT_BLOCK_INTIMATE) ? "Off" : "On") + "\n");
            break;

        case "merciful":
            write("Merciful combat    <mercifil>: " +
                (this_player()->query_option(OPT_MERCIFUL_COMBAT) ? "On" : "Off") + "\n");
            break;

        case "showunmet":
            write("Show unmet descs  <showunmet>: " +
                (this_player()->query_option(OPT_SHOW_UNMET) ? "On" : "Off") + "\n");
            break;

        case "silentships":
            write("Silent ships    <silentships>: " +
                (this_player()->query_option(OPT_SILENT_SHIPS) ? "On" : "Off") + "\n");
            break;

        case "autowrap":
            write("Auto wrapping      <autowrap>: " +
                (this_player()->query_option(OPT_AUTOWRAP) ? "On" : "Off") + "\n");
            break;

        case "web":
            write("Web rankings/stats      <web>: " +
                (this_player()->query_option(OPT_WEB_PERMISSION) ? "On" : "Off") + "\n");
            break;

        case "inventory":
        case "table":
            write("Table inventory       <table>: " +
                (this_player()->query_option(OPT_TABLE_INVENTORY) ? "On" : "Off") + "\n");
            break;

	case "autoline":
	case "cmd":
	    if (this_player()->query_wiz_level())
	    {
		write("Auto line cmds     <autoline>: " +
		    (this_player()->query_option(OPT_AUTOLINECMD) ? "On" : "Off") + "\n");
		break;
	    }
	    /* Intentional fallthrough to default if not a wizard. */

        case "autopwd":
	case "pwd":
	    if (this_player()->query_wiz_level())
	    {
	        write("Auto pwd on cd      <autopwd>: " +
		    (this_player()->query_option(OPT_AUTO_PWD) ? "On" : "Off") + "\n");
		break;
	    }
	    /* Intentional fallthrough to default if not a wizard. */

	case "allcommune":
	    if (this_player()->query_wiz_level())
	    {
		write("See all communes <allcommune>: " +
		    (this_player()->query_option(OPT_ALL_COMMUNE) ? "On" : "Off") + "\n");
		break;
	    }
	    /* Intentional fallthrough to default if not a wizard. */

	case "alwaysknown":
	    if (this_player()->query_wiz_level())
	    {
		write("Be known to all <alwaysknown>: " +
		    (this_player()->query_option(OPT_ALWAYS_KNOWN) ? "On" : "Off") + "\n");
		break;
	    }
	    /* Intentional fallthrough to default if not a wizard. */

	case "timestamp":
	    if (this_player()->query_wiz_level())
	    {
		write("Timestamp lines   <timestamp>: " +
		    (this_player()->query_option(OPT_TIMESTAMP) ? "On" : "Off") + "\n");
		break;
	    }
	    /* Intentional fallthrough to default if not a wizard. */

	default:
            return notify_fail("Syntax error: No such option.\n");
            break;
        }
        return 1;
    }

    rest = implode(args[1..], " ");

    switch(args[0])
    {
    case "morelen":
    case "more":
        if (!this_player()->set_option(OPT_MORE_LEN, atoi(args[1])))
        {
            notify_fail("Syntax error: More length must be in the range of " +
                "1 - 100.\n");
            return 0;
        }
        options("morelen");
        break;

    case "screenwidth":
    case "sw":
        if (args[1] == "off")
        {
            if (!this_player()->set_option(OPT_SCREEN_WIDTH, -1))
            {
                notify_fail("Failed to reset screen width setting. Please " +
                    "make a sysbug report.\n");
                return 0;
            }
        }
        else if (!this_player()->set_option(OPT_SCREEN_WIDTH, atoi(args[1])))
        {
            notify_fail("Syntax error: Screen width must be in the range " +
                "40 - 200 or 'off' to disable wrapping by the game.\n");
            return 0;
        }
        options("screenwidth");
        break;

    case "brief":
        this_player()->set_option(OPT_BRIEF, (args[1] == "on"));
        options("brief");
        break;

    case "echo":
        this_player()->set_option(OPT_ECHO, (args[1] == "on"));
        options("echo");
        break;

    case "wimpy":
        if (lower_case(args[1]) == "brave")
        {
            this_player()->set_whimpy(0);
        }
        else if (args[1] == "?")
            write("brave, " + implode(health_state, ", ") + "\n");
        else
        {
            wi = member_array(rest, health_state);
            if (wi < 0)
            {
                notify_fail("No such health descriptions (" + rest +
                    ") Available:\n" +
                    break_string(COMPOSITE_WORDS(health_state) + ".", 70, 3) +
                    "\n");
                return 0;
            }

            proc = min(99, (100 * (wi + 1)) / sizeof(health_state));
	    /* Verify the reverse calculation to avoid rounding issues. */
	    if ((proc * sizeof(health_state) / 100) != wi)
	    {
		proc--;
	    }

            this_player()->set_whimpy(proc);
        }
        options("wimpy");
        break;

    case "see":
        /* This to accomodate people typing "options see fights" */
        args -= ({ "fights" });
        /* Intentional fallthrough */
    case "fights":
        if (sizeof(args) == 2)
        {
            this_player()->set_option(OPT_NO_FIGHTS, (args[1] != "on"));
        }
        options("see");
        break;

    case "unarmed":
        /* This to accomodate people typing "options unarmed combat" */
        args -= ({ "combat" });
        if (sizeof(args) == 2)
        {
            this_player()->set_option(OPT_UNARMED_OFF, (args[1] != "on"));
            this_player()->update_procuse();
        }
        options("unarmed");
        break;

    case "gagmisses":
        this_player()->set_option(OPT_GAG_MISSES, (args[1] == "on"));
        options("gagmisses");
        break;

    case "gift":
        /* This to accomodate people typing "options gift filter" */
        args -= ({ "filter" });
        /* Intentional fallthrough */
    case "giftfilter":
        if (sizeof(args) == 2)
        {
            this_player()->set_option(OPT_GIFT_FILTER, (args[1] == "on"));
        }
        options("giftfilter");
        break;

    case "intimate":
        if (sizeof(args) == 2)
        {
            this_player()->set_option(OPT_BLOCK_INTIMATE, (args[1] != "on"));
        }
        options("intimate");
        break;

    case "merciful":
        this_player()->set_option(OPT_MERCIFUL_COMBAT, (args[1] == "on"));
        options("merciful");
        break;

    case "showunmet":
        this_player()->set_option(OPT_SHOW_UNMET, (args[1] == "on"));
        options("showunmet");
        break;

    case "silentships":
        this_player()->set_option(OPT_SILENT_SHIPS, (args[1] == "on"));
        options("silentships");
        break;

    case "autowrap":
        this_player()->set_option(OPT_AUTOWRAP, (args[1] == "on"));
        options("autowrap");
        break;

    case "web":
        this_player()->set_option(OPT_WEB_PERMISSION, (args[1] == "on"));
        options("web");
        break;

    case "inventory":
    case "table":
        this_player()->set_option(OPT_TABLE_INVENTORY, (args[1] == "on"));
        options("inventory");
        break;

    case "allcommune":
        if (this_player()->query_wiz_level())
	{
            this_player()->set_option(OPT_ALL_COMMUNE, (args[1] == "on"));
            options("allcommune");
	    break;
	}
        /* Intentional fallthrough to default if not a wizard. */

    case "alwaysknown":
        if (this_player()->query_wiz_level())
	{
            this_player()->set_option(OPT_ALWAYS_KNOWN, (args[1] == "on"));
            options("alwaysknown");
	    break;
	}
        /* Intentional fallthrough to default if not a wizard. */

    case "autopwd":
    case "pwd":
        if (this_player()->query_wiz_level())
	{
            this_player()->set_option(OPT_AUTO_PWD, (args[1] == "on"));
            options("autopwd");
	    break;
	}
        /* Intentional fallthrough to default if not a wizard. */

    case "autoline":
    case "cmd":
        if (this_player()->query_wiz_level())
	{
            this_player()->set_option(OPT_AUTOLINECMD,(args[1] == "on"));
	    // Make sure the line command set gets updated.
	    WIZ_CMD_APPRENTICE->update_commands();
            options("autoline");
	    break;
	}
        /* Intentional fallthrough to default if not a wizard. */

    case "timestamp":
        if (this_player()->query_wiz_level())
	{
            this_player()->set_option(OPT_TIMESTAMP, (args[1] == "on"));
            options("timestamp");
	    break;
	}
        /* Intentional fallthrough to default if not a wizard. */

    default:
        return notify_fail("Syntax error: No such option.\n");
        break;
    }
    return 1;
}

/* **************************************************************************
 * second - note someone as second.
 */

/*
 * Function name: second_password
 * Description  : For security reasons, player has to enter the password of
 *                the second to verify that it is really his.
 * Arguments    : string password - the command-line input of the player.
 *                string name - the name of the second to add.
 * Returns      : int 1/0 - success/failure.
 */
static void
second_password(string password, string name)
{
    if (!SECURITY->register_second(name, password))
    {
        /* Error message would be printed within the call. */
        return;
    }

    second("list");
}

public int
second(string str)
{
    string *args;

    if (!stringp(str))
    {
        str = "list";
    }

    args = explode(lower_case(str), " ");
    switch (args[0])
    {
    case "a":
    case "add":
        if (sizeof(args) != 2)
        {
            notify_fail("Syntax: second add <player>\n");
            return 0;
        }
        write("Please enter the password of " + capitalize(args[1]) + ": ");
        input_to(&second_password(, args[1]), 1);
        return 1;
        /* Not reached. */

    case "list":
        args = SECURITY->query_player_seconds();
        if (!sizeof(args))
        {
            write("No seconds listed.\n");
            return 1;
        }
        write("Currently listed seconds: " +
            COMPOSITE_WORDS(map(args, capitalize)) + ".\n");
        return 1;
        /* Not reached. */

    default:
        notify_fail("Invalid subcommand \"" + args[0] + "\".\n");
        return 0;
    }

    write("This should never happen. Please report.\n");
    return 1;
}

/*
 * vitals - Give vital state information about the living.
 */
varargs int
vitals(string str, object target = this_player())
{
    string name;
    string hyped;
    int self;
    int value1;
    int value2;

    if (!strlen(str))
    {
        str = "all";
    }

    self = (target == this_player());
    name = capitalize(target->query_real_name());
    switch(str)
    {
    case "age":
        write((self ? "You are" : (name + " is")) + " " +
            CONVTIME(target->query_age() * 2) + " of age.\n");
        return 1;

    case "align":
    case "alignment":
        write((self ? "You are" : (name + " is")) + " " +
            target->query_align_text() + ".\n");
        return 1;

    case "all":
        vitals("health", this_player());
        vitals("panic", this_player());
        vitals("stuffed", this_player());
        vitals("intox", this_player());
        vitals("alignment", this_player());
        vitals("encumbrance", this_player());
        vitals("quickness", this_player());
        vitals("age", this_player());
        return 1;

    case "encumbrance":
        write((self ? "You are" : (name + " is")) + " " +
            GET_PROC_DESC(target->query_encumberance_weight(), enc_weight) + ".\n");
        return 1;

    case "health":
    case "mana":
        if (this_player()->query_ghost())
        {
            write((self ? "You" : name) +
                " would be feeling much better with an actual body.\n");
            return 1;
        }
        write((self ? "You are" : (name + " is")) + " physically " +
            GET_NUM_DESC(target->query_hp(), target->query_max_hp(), health_state) +
            " and mentally " +
            GET_NUM_DESC(target->query_mana(), target->query_max_mana(), mana_state) +
            ".\n");
        return 1;

    case "intox":
    case "intoxication":
        if (target->query_intoxicated() > target->query_prop(LIVE_I_MAX_INTOX) &&
            target->query_prop(LIVE_S_EXTENDED_INTOX))
        {
            write((self ? "You are" : (name + " is")) + " " +
                target->query_prop(LIVE_S_EXTENDED_INTOX) + ".\n");
        }
        else if (target->query_intoxicated())
        {
            write((self ? "You are" : (name + " is")) + " " +
                GET_NUM_DESC_SUB(target->query_intoxicated(),
                    target->query_prop(LIVE_I_MAX_INTOX), intox_state,
                    SD_STAT_DENOM, 0) + ".\n");
        }
        else
        {
            write((self ? "You are" : (name + " is")) + " sober.\n");
        }
        return 1;

    case "mail":
        value1 = MAIL_CHECKER->query_mail(target);
        write((self ? "You have " : (name + " has ")) + MAIL_FLAGS[value1] + ".\n");
        return 1;

    case "panic":
    case "fatigue":
        hyped = (target->query_relaxed_from_combat() ? "" : " and full of adrenaline");
        /* Current fatigue really is an "energy left" value that counts down. */
        value1 = target->query_max_fatigue() - target->query_fatigue();
        write((self ? "You feel" : (name + " feels")) + " " +
            GET_NUM_DESC_SUB(target->query_panic(), F_PANIC_WIMP_LEVEL(target->query_stat(SS_DIS)), panic_state, SD_STAT_DENOM, 2) +
            (strlen(hyped) ? ", " : " and ") +
            GET_NUM_DESC_SUB(value1, target->query_max_fatigue(), fatigue_state, SD_STAT_DENOM, 1) +
            hyped + ".\n");
        return 1;

    case "stuffed":
    case "soaked":
        string stuffed_desc = 
            (target->query_stuffed() > target->query_prop(LIVE_I_MAX_EAT) &&
            target->query_prop(LIVE_S_EXTENDED_STUFF)) ? target->query_prop(LIVE_S_EXTENDED_STUFF)
            : GET_NUM_DESC(target->query_stuffed(), target->query_prop(LIVE_I_MAX_EAT), stuff_state);
        write((self ? "You can" : (name + " can")) + " " + stuffed_desc +
	    " and " +
            GET_NUM_DESC(target->query_soaked(), target->query_prop(LIVE_I_MAX_DRINK), soak_state) + ".\n");
        return 1;
    case "quickness":
    case "haste":
        int value = ftoi((1.0 - target->query_speed(1.0)) * 100.0);
        string desc = GET_NUM_DESC_CENTER(value, 60, 0, 1, SD_QUICKNESS);
        write((self ? "You are" : (name + " is")) + " moving at " + LANG_ADDART(desc) + " pace.\n");
        return 1;

    default:
        if (!this_player()->query_wiz_level())
        {
            notify_fail("The argument " + str + " is not valid for vitals.\n");
            return 0;
        }
        if (!objectp(target = find_player(lower_case(str))))
        {
            notify_fail("There is no player " + capitalize(str) +
                " in the realms at present.\n");
            return 0;
        }
        vitals("health", target);
        vitals("panic", target);
        vitals("stuffed", target);
        vitals("intox", target);
        vitals("alignment", target);
        vitals("encumbrance", target);
        vitals("quickness", target);
        vitals("age", target);
        return 1;
    }

    write("Impossible end of vitals. Please make a sysbug report of this.\n");
    return 1;
}

/*
 * Function name: show_stats
 * Description:   Gives information on the stats
 */
varargs int
show_stats(string str)
{
    int limit, index, stat;
    object ob;
    string start_be, start_have, text, *stats;
    string *immortal = ({ }), *epic = ({ }), *supreme = ({ }),
        *extraordinary = ({ }), *incredible = ({ }), *unbelievable = ({ }),
        *miraculous = ({ }), *impossible = ({ });

    string orig_brute, actual_brute;

    if (str == "reset")
    {
	this_player()->reset_exp_gain_desc();
        write("Resetting your progress counter.\n");
        return show_stats(0);
    }

    if (!strlen(str))
    {
        ob = this_player();
        str = "You";
        start_be = "You are ";
        start_have = "You have ";
    }
    else
    {
        if(!((ob = find_player(str)) && this_player()->query_wiz_level()))
        {
            notify_fail("Curious aren't we?\n");
            return 0;
        }
        str = capitalize(str);
        start_be = str + " is ";
        start_have = str + " has ";
    }

    stats = ({ });
    for (index = 0; index < SS_NO_EXP_STATS; index++)
    {
        stat = ob->query_stat(index);
        if (stat >= SD_STATLEVEL_IMP) impossible += ({ SD_LONG_STAT_DESC[index] });
        else if (stat >= SD_STATLEVEL_MIR) miraculous += ({ SD_LONG_STAT_DESC[index] });
        else if (stat >= SD_STATLEVEL_UNB) unbelievable += ({ SD_LONG_STAT_DESC[index] });
        else if (stat >= SD_STATLEVEL_INC) incredible += ({ SD_LONG_STAT_DESC[index] });
        else if (stat >= SD_STATLEVEL_EXT) extraordinary += ({ SD_LONG_STAT_DESC[index] });
        else if (stat >= SD_STATLEVEL_SUP) supreme += ({ SD_LONG_STAT_DESC[index] });
        else if (stat >= SD_STATLEVEL_IMM) immortal += ({ SD_LONG_STAT_DESC[index] });
        else if (stat >= SD_STATLEVEL_EPIC) epic += ({ SD_LONG_STAT_DESC[index] });
        else stats += ({ GET_STAT_LEVEL_DESC(index, stat) });
    }
    text = start_be + LANG_ADDART(sizeof(stats) ? COMPOSITE_WORDS(stats) + " " : "") + ob->query_nonmet_name();

    stats = ({ });
    if (sizeof(epic)) stats += ({ SD_STATLEV_EPIC + " " + COMPOSITE_WORDS(epic) });
    if (sizeof(immortal)) stats += ({ SD_STATLEV_IMM + " " + COMPOSITE_WORDS(immortal) });
    if (sizeof(supreme)) stats += ({ SD_STATLEV_SUP + " " + COMPOSITE_WORDS(supreme) });
    if (sizeof(extraordinary)) stats += ({ SD_STATLEV_EXT + " " + COMPOSITE_WORDS(extraordinary) });
    if (sizeof(incredible)) stats += ({ SD_STATLEV_INC + " " + COMPOSITE_WORDS(incredible) });
    if (sizeof(unbelievable)) stats += ({ SD_STATLEV_UNB + " " + COMPOSITE_WORDS(unbelievable) });
    if (sizeof(miraculous)) stats += ({ SD_STATLEV_MIR + " " + COMPOSITE_WORDS(miraculous) });
    if (sizeof(impossible)) stats += ({ SD_STATLEV_IMP + " " + COMPOSITE_WORDS(impossible) });
    if (sizeof(stats)) text += " with " + COMPOSITE_WORDS(stats);
    write(text + ".\n");

    /* brutalfactor */
    actual_brute = GET_NUM_DESC_SUB(ftoi(ob->query_brute_factor(0) * 1000.0), 1000, brute_fact, SD_STAT_DENOM, 2);
    if (ob->query_exp_quest() > F_QUEST_EXP_MAX_BRUTE)
    {
        actual_brute += ", helped maximally by your quest experience";
    }
    write(start_be + actual_brute + ".\n");

    /* If we're recovering from death, we may have a lower brute than actually
     * should have had. But only print the message if the brute level would be
     * different. */
    if (ob->query_exp() < ob->query_max_exp())
    {
        orig_brute = GET_NUM_DESC_SUB(ftoi(ob->query_brute_factor(1) * 1000.0), 1000, brute_fact, SD_STAT_DENOM, 2);
        if (actual_brute != orig_brute)
        {
            write("Without death recovery assistance, " + lower_case(str) +
                " would have been " + orig_brute + ".\n");
        }
    }

    /* Progress on combat and general experience. */
    if (text = ob->query_exp_gain_desc())
    {
        write(start_have + "made " + text + " progress since you logged in.\n");
    }
    else
    {
        write(start_have + "made no measurable progress since you logged in.\n");
    }

    /* Progress on quest experience. */
    if (text = ob->query_exp_quest_gain_desc())
    {
        write(start_have + "received " + text + " quest experience since you logged in.\n");
    }

    write("As an explorer, " + lower_case(start_have) + "done " +
        GET_NUM_DESC(ob->query_exp_quest(),
            F_QUEST_EXP_MAX_BRUTE * sizeof(SD_LEVEL_MAP["quest-progress"]) / (sizeof(SD_LEVEL_MAP["quest-progress"]) - 1),
            SD_LEVEL_MAP["quest-progress"]) + ".\n");

    return 1;
}

/*
 * skills - Give information on a livings skills
 */
/*
 * Function name: show_skills
 * Description:   Gives information on the stats
 */
varargs int
show_skills(string str)
{
    int num, wrap;
    object player = this_player();
    int *skills, *gr_skills;
    string *words;
    string extra, group = "";
    string *groups = SS_SKILL_GROUPS;

    words = str ? explode(str, " ") : ({ });
    switch(sizeof(words))
    {
    case 0:
        /* Default situation. Player wants to see all his stats. */
        break;
    case 1:
        /* Person wants to see a group of himself, or see someone elses stats. */
        if (!this_player()->query_wiz_level() || !(player = find_player(words[0])))
        {
            player = this_player();
            groups = ({ capitalize(words[0]) });
        }
        break;
    case 2:
        /* Player specifies both the person to see and the group to see. */
        if (this_player()->query_wiz_level())
            player = find_player(words[0]);
        groups = ({ capitalize(words[1]) });
        break;
    default:
        notify_fail("Too many arguments. Syntax: stats [player] [skill group]\n");
        return 0;
    }

    if (!objectp(player))
    {
        notify_fail("No player " + words[0] + " found.\n");
        return 0;
    }

    skills = sort_array(player->query_all_skill_types());
    SKILL_LIBRARY->sk_init();

    foreach(string group: groups)
    {
        if (!pointerp(SS_SKILL_GROUP_LIMIT[group]))
        {
            write("-- " + group + " is not a valid skills group.\n");
            continue;
        }
        gr_skills = filter(skills, &operator(>=)(,SS_SKILL_GROUP_LIMIT[group][0]));
        gr_skills = filter(gr_skills, &operator(<=)(,SS_SKILL_GROUP_LIMIT[group][1]));
        wrap = 0;
        write("-- " + group + " skills --\n");
        foreach(int skill: gr_skills)
        {
            if (!(num = player->query_skill(skill)))
            {
                player->remove_skill(skill);
                continue;
            }
            if (pointerp(skillmap[skill]))
                str = skillmap[skill][0];
            else if (!strlen(str = player->query_skill_name(skill)))
                continue;
            extra = (player->query_skill_extra(skill) ? "*" : "");

            /* Print the text in two columns. */
            if (++wrap % 2)
            {
                write(sprintf("%-16s %-20s     ", capitalize(str) + ":",
                    SKILL_LIBRARY->sk_rank(num) + extra)[..41]);
            }
            else
            {
                write(sprintf("%-16s %s\n", capitalize(str) + ":",
                    SKILL_LIBRARY->sk_rank(num) + extra)[..39]);
            }
        }
        if (!wrap)
            write("no skills\n");
        else if (wrap % 2)
            write("\n");
    }

    return 1;
}

/*
 * Function name: gmcp_char_skills_get
 * Description  : Implementation of the "char.skills.get" command for GMCP.
 * Arguments    : object player - the player who wants to know.
 *                mixed data - the paramters.
 */
void
gmcp_char_skills_get(object player, mixed data)
{
    int *skills;
    int value;
    string group;
    string name;

    /* Send the Groups. */
    if (!mappingp(data) || !data[GMCP_GROUP])
    {
        /* We don't filter this for actual skills the player has in this group. */
        player->catch_gmcp(GMCP_CHAR_SKILLS_GROUPS, SS_SKILL_GROUPS);
        return;
    }

    /* List the skills within a group. */
    if (!data[GMCP_NAME])
    {
        /* Not a valid skill group. */
        if (!pointerp(SS_SKILL_GROUP_LIMIT[data[GMCP_GROUP]])) return;

        group = data[GMCP_GROUP];
        /* This will filter all skills in a particular group that the player has. */
        skills = sort_array(player->query_all_skill_types());
        skills = filter(skills, &operator(>=)(,SS_SKILL_GROUP_LIMIT[group][0]));
        skills = filter(skills, &operator(<=)(,SS_SKILL_GROUP_LIMIT[group][1]));

        data = ([ ]);
        foreach(int skill: skills)
        {
            if (pointerp(skillmap[skill]))
                name = skillmap[skill][0];
            else if (!strlen(name = player->query_skill_name(skill)))
                continue;

            data[name] = player->query_skill(skill);
        }
        player->catch_gmcp(GMCP_CHAR_SKILLS_LIST, ([ GMCP_GROUP : group, GMCP_LIST : data ]) );
        return;
    }

    /* Lists the data for one skill. Since we don't have any actual info, we echo the name. */
    player->catch_gmcp(GMCP_CHAR_SKILLS_INFO, ([ GMCP_GROUP : data[GMCP_GROUP],
        GMCP_NAME : data[GMCP_NAME], GMCP_INFO : data[GMCP_NAME],
        GMCP_VALUE : player->query_skill(SS_SKILL_LOOKUP[name] ) ]) );
}

/*
 * Function name: gmcp_char_vitals_get
 * Description  : Implementation of the "char.vitals.get" command for GMCP.
 * Arguments    : object player - the player who wants to know.
 *                mixed data - the paramters.
 */
void
gmcp_char_vitals_get(object player, mixed data)
{
    mapping result = ([ ]);

    /* Treat individual request too. */
    if (stringp(data))
    {
	if (lower_case(data) == "all")
	    data = ({ GMCP_HEALTH, GMCP_MANA, GMCP_FATIGUE, GMCP_ENCUMBER,
                GMCP_PANIC, GMCP_PROGRESS, GMCP_FOOD, GMCP_DRINK, GMCP_INTOX });
        else
            data = ({ data });
    }
    /* Send the Lists. */
    if (!pointerp(data))
    {
        /* We don't filter this for actual skills the player has in this group. */
        player->catch_gmcp(GMCP_CHAR_VITALS_LISTS,
	    ({ GMCP_HEALTH, GMCP_MANA, GMCP_FATIGUE, GMCP_PROGRESS }) );
        return;
    }

    foreach(string list: data)
    {
	switch(list)
	{
	case GMCP_HEALTH:
	case GMCP_MANA:
	case GMCP_PROGRESS:
	case GMCP_ENCUMBER:
	case GMCP_PANIC:
	    result += ([ list : lev_map[list] ]);
	    break;
	case GMCP_DRINK:
	    result += ([ list : lev_map["soaked"] ]);
	    break;
	case GMCP_FOOD:
	    result += ([ list : lev_map["stuffed"] ]);
	    break;
	case GMCP_INTOX:
	    result += ([ list : ({ SD_INTOX_SOBER }) + lev_map["intox"] ]);
	    break;
	case GMCP_FATIGUE:
	    result += ([ list : GET_NUM_DESC_COMBOS(lev_map[list], SD_STAT_DENOM, 1) ]);
	    break;
	}
    }

    /* Lists the levels available for improvement. */
    player->catch_gmcp(GMCP_CHAR_VITALS_LEVELS, result);
}
