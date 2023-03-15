/*
 * /lib/guild_support.c
 *
 * Some support for meditating code.
 *
 * To use this code, take the following steps:
 *
 * - Inherit it into your room code.
 * - Place a call to the function init_guild_support() in your init() function.
 * - Place a call to the function gs_leave_inv(object ob, object to) in your
 *   leave_inv(object ob, object to)  function.
 * - You may redefine the gs_hook*() functions to create your own messages.
 */

#pragma save_binary
#pragma strict_types

#include <composite.h>
#include <files.h>
#include <macros.h>
#include <language.h>
#include <living_desc.h>
#include <login.h>
#include <options.h>
#include <ss_types.h>
#include <state_desc.h>
#include <std.h>
#include <stdproperties.h>

/* Local var for changes in looks.
 * newlooks = ([ "name" : ([ "var" : "value" ]) ])
 */
static private mapping newlooks = ([ ]);

/*
 * Prototypes.
 */
int gs_catch_all(string arg);

/*
 * Function name: create_guild_support
 * Description  : Obsolete function. Kept for backward compatibility so far.
 */
void
create_guild_support()
{
}

/*
 * Function name: get_prefs
 * Description  : When the player uses the "set" command, this function is used
 *                to parse the input.
 * Arguments    : string str - input text.
 */
void
get_prefs(string str)
{
    string *words;
    int    *prefs = allocate(SS_NO_EXP_STATS);
    int     index;
    int     primary;

    /* Start initialize variable pref. */
    index = -1;
    while(++index < SS_NO_EXP_STATS)
    {
        prefs[index] = GS_DEFAULT_FOCUS;
    }

    if (!strlen(str))
    {
        write("Please use \"<stat> [and] <stat>\", \"evenly\", "+
	    "\"physical\", \"mental\" or \"abort\".\n" +
            "Example: \"dexterity and wisdom\"\nPlease try again: ");
        input_to(get_prefs);
        return;
    }

    if (str == "abort")
    {
        write("Leaving your preferences unchanged.\n");
        return;
    }
    if (str == "evenly")
    {
        this_player()->set_learn_pref(prefs);
        write("Distributing your focus evenly over all stats.\n");
        return;
    }
    if (str == "mental")
    {
        prefs[SS_WIS] = GS_SECONDARY_FOCUS;
        prefs[SS_INT] = GS_SECONDARY_FOCUS;
        prefs[SS_DIS] = GS_SECONDARY_FOCUS;
        this_player()->set_learn_pref(prefs);
        write("You focus on your mental abilities.\n");
        return;
    }
    if (str == "physical")
    {
        prefs[SS_STR] = GS_SECONDARY_FOCUS;
        prefs[SS_DEX] = GS_SECONDARY_FOCUS;
        prefs[SS_CON] = GS_SECONDARY_FOCUS;
        this_player()->set_learn_pref(prefs);
        write("You focus on your physical abilities.\n");
        return;
    }

    words = explode(lower_case(str), " ") - ({ "" });

    /* Is it the correct number of arguments? */
    switch (sizeof(words))
    {
    case 2:
        break;

    case 3:
        if (words[1] == "and")
        {
            words[1] = words[2];
            break;
        }
        /* Intentional fallthrough. */

    default:
        write("Use \"<stat> and <stat>\", \"evenly\", \"mental\", " +
            "\"physical\" or \"abort\".\nPlease try again: ");
        input_to(get_prefs);
        return;
    }

    if ((index = member_array(words[0], SD_LONG_STAT_DESC)) == -1)
    {
        index = member_array(words[0], SD_STAT_DESC);
    }
    if ((index < 0) || (index >= SS_NO_EXP_STATS))
    {
        write("Unknown stat " + words[0] + ". Use either " +
            COMPOSITE_WORDS(SD_LONG_STAT_DESC[..(SS_NO_EXP_STATS-1)]) +
            " (or their known abbreviations).\nPlease try again: ");
        input_to(get_prefs);
        return;
    }
    prefs[index] = GS_PRIMARY_FOCUS;
    write("You put your primary focus on " + SD_LONG_STAT_DESC[index] + ".\n");
    primary = index;

    if ((index = member_array(words[1], SD_LONG_STAT_DESC)) == -1)
    {
        index = member_array(words[1], SD_STAT_DESC);
    }
    if ((index < 0) || (index >= SS_NO_EXP_STATS))
    {
        write("Unknown stat " + words[0] + ". Use either " +
            COMPOSITE_WORDS(SD_LONG_STAT_DESC[..(SS_NO_EXP_STATS-1)]) +
            " (or their known abbreviations).\nPlease try again: ");
        input_to(get_prefs);
        return;
    }
    if (index == primary)
    {
        write("Select two different stats to focus on.\nPlease try again: ");
        input_to(get_prefs);
        return;
    }
    prefs[index] = GS_SECONDARY_FOCUS;
    write("You put your secondary focus on " + SD_LONG_STAT_DESC[index] + ".\n");

    this_player()->set_learn_pref(prefs);
}

/*
 * Function name: set_prefs
 * Description  : When the player wants to change the learn preferences, this
 *                function prompts the player for the new values.
 * Returns      : int 1 - always.
 */
int
set_prefs()
{
    write("Deep in trance you can select to concentrate on improving your " +
        "different stats. Simply type \"abort\" to keep your preferences " +
        "unchanged.\n");
    write("The syntax is \"<stat> [and] <stat>\" using the stats " +
        COMPOSITE_WORDS(SD_LONG_STAT_DESC[..(SS_NO_EXP_STATS-1)]) +
        " (or their known abbreviations).\nYou may also enter \"evenly\", " +
        "\"mental\" or \"physical\" to direct your focus.\nOn which stats " +
        "do you wish to focus? ");

    input_to(get_prefs);
    return 1;
}

/*
 * Function name: gs_leave_inv
 * Description  : Should be called if someone leaves the room. if that person
 *                was meditating, better do something. You should call this
 *                function from leave_inv(ob, to) in your room.
 * Arguments    : object ob - the object that is leaving.
 *                object to - the new destination of the object.
 */
void
gs_leave_inv(object ob, object to)
{
    if (ob->query_prop(LIVE_I_MEDITATES))
    {
        ob->remove_prop(LIVE_I_MEDITATES);
        ob->remove_prop(LIVE_S_EXTRA_SHORT);
    }
}

/*
 * Function name: gs_list
 * Description  : With this command, players can get information on their
 *                guilds.
 * Arguments    : string str - the command line argument.
 * Returns      : int 1/0 - success/failure.
 */
nomask int
gs_list(string str)
{
    string long_name;
    string short_name;
    int    index = -1;
    int    total_tax = 0;
    string *not_found = SD_GUILD_FULL_NAMES;
    string *masters;

    /* Access failure. The player probably wanted a different list. */
    if (str != "guilds")
    {
        notify_fail("List what? Here you can only list your \"guilds\".\n");
        return 0;
    }

    /* Loop over all guilds to try and find information about them. */
    while(++index < SD_NUM_GUILDS)
    {
        if (!strlen(long_name = call_other(this_player(), "query_guild_name_" +
            SD_GUILD_EXTENSIONS[index])))
        {
            continue;
        }

        not_found -= ({ SD_GUILD_FULL_NAMES[index] });
        total_tax += this_player()->query_learn_pref(SS_NO_EXP_STATS + index);

        short_name = SECURITY->query_guild_short_name(long_name);
        if (!strlen(short_name))
        {
            write("Your " + SD_GUILD_FULL_NAMES[index] + " guild named \"" +
                long_name + "\" has not been registered! This is not good! " +
                "Please report this to the playerarch team and inform your " +
                "guildmaster so he/she can correct this.\n\n");
            short_name = "unknown";
        }

        write(capitalize(SD_GUILD_FULL_NAMES[index]) + " guild: " +
            long_name + " (short: " + capitalize(short_name) + ").\n");

        masters = SECURITY->query_guild_masters(short_name);
        if (!sizeof(masters))
        {
            write("No guildmasters are registered for the " +
                capitalize(short_name) + " guild. This is not good!\n");
        }
        else
        {
            write("Guildmaster" + (sizeof(masters) > 1 ? "s" : "") + ": " +
                COMPOSITE_WORDS(map(masters, capitalize)) + ".\n");
        }
        write("\n");
    }

    switch(sizeof(not_found))
    {
    case 0:
        break;

    case SD_NUM_GUILDS:
        write("You are not a member of any guild, be it racial, layman or " +
            "occupational.\n");
        break;

    default:
        write("You are not a member of any " +
	    COMPOSITE_WORDS_WITH(not_found, "or") + " guild.\n");
        break;
    }

    /* See whether the total tax payed is the tax we expect for the guilds the
     * player is in.
     */
    while(--index >= 0)
    {
        total_tax -= this_player()->query_learn_pref(SS_NO_EXP_STATS + index);
    }

    if (total_tax)
    {
        write("The total tax you currently pay is not the amount you " +
            "should be paying. It seems that a guild you left did not " +
            "clean up after itself properly. Please contact the playerarch " +
            "team to have this corrected.\n");
    }

    return 1;
}

/*
 * Function name: gs_looks
 * Description  : With this command, young players can modify their looks.
 * Arguments    : string str - the command line argument.
 * Returns      : int 1/0 - success/failure.
 */
nomask int
gs_looks(string str)
{
    string *args;
    string *attributes;
    string *adjs = this_player()->query_adjs();
    string name = this_player()->query_real_name();
    string race = this_player()->query_race();
    string result, guild;
    string gender = LD_GENDER_MAP[this_player()->query_gender()];
    int    scrw = this_player()->query_option(OPT_SCREEN_WIDTH);

    if (!strlen(str))
    {
	str = "list";
    }
    if (!mappingp(newlooks[name]))
    {
	newlooks[name] = ([ ]);
    }

    args = explode(str, " ");
    str = implode(args[1..], " ");
    switch(args[0])
    {
    case "adj1":
    case "adj2":
        if (!strlen(str))
	{
	    write(HANGING_INDENT("Possible attribute categories: " +
	        COMPOSITE_WORDS(LD_ATTRIB_CATEGORIES) + ".", 4, 0));
	    write("See the attributes per category with: looks <category>\n");
	    return 1;
	}
	if (!LD_IS_ATTRIBUTE(str))
	{
	    write("Not a valid attribute: " + str + ".\n");
	    return 1;
	}
	newlooks[name][args[0]] = str;
	return gs_looks("new");

    case "gender":
        if (!strlen(str))
	{
	    write("Possible genders: male and female.\n");
	    return 1;
	}
	if (!IN_ARRAY(str, m_indices(LD_GENDER_REVERSE_MAP)))
	{
	    write("Not a valid gender: " + str + ".\n");
	    return 1;
	}
	newlooks[name]["gender"] = str;
	return gs_looks("new");

    case "race":
        if (!strlen(str))
	{
	    write("Possible races: " + COMPOSITE_WORDS(RACES) + ".\n");
	    return 1;
	}
	if (!IN_ARRAY(str, RACES))
	{
	    write("Not a valid race: " + str + ".\n");
	    return 1;
	}
	newlooks[name]["race"] = str;
	guild = this_player()->query_guild_name_race();
        write("Warning: you are changing your race from " + race +
	    " to " + newlooks[name]["race"] + ". This will not come into " +
	    "effect until you quit and log in again." +
	    (strlen(guild) ? " It will cause you to be expelled from the " + guild + "." : "") +
	    "\n\n");
	return gs_looks("new");

    case "height":
        if (!strlen(str))
	{
	    write("Possible heights: " + COMPOSITE_WORDS(HEIGHTDESC) + ".\n");
	    return 1;
	}
	if (!IN_ARRAY(str, HEIGHTDESC))
	{
	    write("Not a valid height: " + str + ".\n");
	    return gs_looks("height");
	}
	newlooks[name]["height"] = str;
	return gs_looks("new");

    case "width":
        if (!strlen(str))
	{
	    write("Possible widths: " + COMPOSITE_WORDS(WIDTHDESC) + ".\n");
	    return 1;
	}
	if (!IN_ARRAY(str, WIDTHDESC))
	{
	    write("Not a valid width: " + str + ".\n");
	    return gs_looks("width");
	}
	newlooks[name]["width"] = str;
	return gs_looks("new");

    case "list":
        write("You are " + LANG_ADDART(this_player()->query_nonmet_name()) +
	    "; you are " + this_player()->query_height_desc() + " and " +
	    this_player()->query_width_desc() + " for " + LANG_ADDART(race) + ".\n");
	write("  adj1   : " + adjs[0] + "\n");
	write("  adj2   : " + adjs[1] + "\n");
	write("  gender : " + gender + "\n");
	write("  race   : " + this_player()->query_race_name() + "\n");
	write("  height : " + this_player()->query_height_desc() + "\n");
	write("  width  : " + this_player()->query_width_desc() + "\n");
	return gs_looks("new");

    case "new":
    	result = "";
	if (newlooks[name]["adj1"]) result += "  adj1   : " + newlooks[name]["adj1"] + "\n";
	if (newlooks[name]["adj2"]) result += "  adj2   : " + newlooks[name]["adj2"] + "\n";
	if (newlooks[name]["gender"]) result += "  gender : " + newlooks[name]["gender"] + "\n";
	if (newlooks[name]["race"]) result += "  race   : " + newlooks[name]["race"] + "\n";
	if (newlooks[name]["height"]) result += "  height : " + newlooks[name]["height"] + "\n";
	if (newlooks[name]["width"]) result += "  width  : " + newlooks[name]["width"] + "\n";
	if (strlen(result))
	{
	    write("You prepared the following new looks:\n" + result +
	        "\nIt would make you " +
		LANG_ADDART(newlooks[name]["adj1"] ? newlooks[name]["adj1"] : adjs[0]) +
		" " + (newlooks[name]["adj2"] ? newlooks[name]["adj2"] : adjs[1]) +
		" " + (newlooks[name]["gender"] ? newlooks[name]["gender"] : gender) +
		" " + (newlooks[name]["race"] ? newlooks[name]["race"] : race) +
		".\nEnter \"looks confirm\" when you are done to confirm all changes.\n");
	}
        return 1;

    case "confirm":
        if (!wildmatch("*jr", name))
	{
	    write("Only juniors may undergo this experimental treatment.\n");
	    return 1;
	}
	this_player()->remove_adjs(adjs);
	if (newlooks[name]["adj1"])
	    adjs = newlooks[name]["adj1"] + adjs[1..];
	if (newlooks[name]["adj2"])
	    adjs = adjs[..0] + newlooks[name]["adj2"];
	this_player()->set_adjs(adjs);
	if (newlooks[name]["gender"])
	    this_player()->set_gender(LD_GENDER_REVERSE_MAP[newlooks[name]["gender"]]);
	if (newlooks[name]["race"])
	{
	    this_player()->set_player_file(RACEMAP[newlooks[name]["race"]]);
	    write("Warning: you are changing your race from " + race +
	        " to " + newlooks[name]["race"] + ". This will not come " +
		"into effect until you quit and log in again.\nAlso note " +
		"that you may need to reset your height and width of your " +
		"new body upon login (as they are based on your current " +
		"race).\n");
	}
	if (newlooks[name]["height"])
	    this_player()->set_height_desc(newlooks[name]["height"]);
	if (newlooks[name]["width"])
	    this_player()->set_width_desc(newlooks[name]["width"]);
	newlooks[name] = ([ ]);
        write("You are now " + LANG_ADDART(this_player()->query_nonmet_name()) +
	    "; you are " + this_player()->query_height_desc() + " and " +
	    this_player()->query_width_desc() + " for " + LANG_ADDART(race) + ".\n");
	return 1;

    default:
        attributes = LD_ATTRIBS_BY_CAT(args[0]);
        if (sizeof(attributes))
	{
	    write("Attributes for category " + args[0] + ":\n");
	    scrw = ((scrw >= 40) ? (scrw - 3) : 79);
	    write(sprintf("%-*#s\n", scrw, implode(attributes, "\n")));
	    return 1;
	}

        write("Unknown instruction. Please see \"help looks\"for details.\n");
	return 1;
    }
}

/*
 * Function name: gs_hook_already_meditate
 * Description  : This hook is called when player is already meditating and
 *                tries to mediate again. You can mask this function to give
 *                a different message.
 * Returns      : int 1 - always.
 */
int
gs_hook_already_meditate()
{
    write("You are already in deep trance. If you wish to finish your " +
        "meditation\nyou can do so by typing \"rise\".\n");
    return 1;
}

/*
 * Function name: gs_hook_start_meditate
 * Description  : This hook is called when player starts to meditate. You can
 *                mask this function to give a different message.
 */
void
gs_hook_start_meditate()
{
    write("Slowly you sit down on the soft carpet and close your eyes. A " +
        "feeling of great ease and self control falls upon you. You block " +
        "off your senses and concentrate solely upon your own mind. You " +
        "find yourself able to <set> your different preferences at your own " +
        "desire. Just <rise> when you are done meditating.\n");
    say(QCTNAME(this_player()) + " sits down on the carpet and starts " +
        "to meditate.\n");
}

/*
 * Function name: gs_hook_rise
 * Description  : This hook is called when player rises from the meditation.
 *                You can mask this function to give a different message.
 */
void
gs_hook_rise()
{
    write("As if ascending from a great depth, you rise to the surface of\n" +
        "your consciousness. You exhale and feel very relaxed as you get\n" +
        "up and leave the carpet.\n");
    say(QCTNAME(this_player()) + " rises from the carpet.\n");
}

/*
 * Function name: gs_hook_catch_error
 * Description  : This hook is called when a player tried to do something strange
 *                while meditating like examining things or leave the room. You
 *                can mask this function to give a different message.
 * Arguments    : string str - Argument the player tried to send to his command.
 * Returns      : int 1 - normally.
 */
int
gs_hook_catch_error(string str)
{
    write("You cannot do that while meditating. " +
        "You may <rise> to end your trance.\n");
    return 1;
}

/*
 * Function name: gs_meditate
 * Description  : Player wants to meditate.
 * Arguments    : string str - the command line argument.
 * Returns      : int 1/0 - success/failure.
 */
int
gs_meditate(string str)
{
    string primary_desc;
    string secondary_desc;
    int    index;
    int    stat;
    int    level;
    int    residue;
    int    spread;
    int   *prefs;
    int   *sorted_prefs;
    int    primary;

    this_player()->add_prop(LIVE_S_EXTRA_SHORT, " is meditating");
    if (this_player()->query_prop(LIVE_I_MEDITATES))
    {
        return gs_hook_already_meditate();
    }

    this_player()->add_prop(LIVE_I_MEDITATES, 1);

    this_player()->reveal_me(1);

    gs_hook_start_meditate();
    write("Here you may also <restrict> yourself from playing.\n\n");

    /* Average stat, mortal level. */
    stat = this_player()->query_average_stat();
    if (stat >= SD_AV_LEVELS[SD_NUM_AV_LEVELS-1])
    {
        write("You are " + LANG_ADDART(SD_AV_TITLES[SD_NUM_AV_LEVELS-1]) +
            ", one step from reaching immortality.\n" );
    }
    else
    {
        level = SD_NUM_AV_LEVELS;
        while(--level >= 0)
        {
            if (stat >= SD_AV_LEVELS[level])
            {
                break;
            }
        }
        residue = stat - SD_AV_LEVELS[level];
        spread = SD_AV_LEVELS[level+1] - SD_AV_LEVELS[level];
        write("You are " + GET_NUM_DESC(residue, spread, SD_AV_ADVANCE_DESCS) +
            " " + one_of_list( ({ "advancing", "developing", "progressing", "being promoted", "rising" }) ) +
            " to " + SD_AV_TITLES[level+1] + ".\n");
    }

    /* Individual stats. */
    index = -1;
    while(++index < SS_NO_EXP_STATS)
    {
        stat = this_player()->query_stat(index);
        if (stat >= SD_STATLEVEL_IMP)
        {
            write("You have reached impossible " + SD_LONG_STAT_DESC[index] + ".\n");
            continue;
        }

        level = GET_NUM_LEVEL_INDEX(stat, SD_STATLEVELS);
        residue = stat - SD_STATLEVELS[level];
        spread = SD_STATLEVELS[level+1] - SD_STATLEVELS[level];

        write("You are " + GET_NUM_DESC(residue, spread, SD_ADVANCE_DESCS) +
            " " + one_of_list(SD_PROGRESS_DESCS) +
            " " + GET_STAT_INDEX_DESC(index, level+1) + ".\n");
    }

    add_action(gs_catch_all, "", 1);

    write("\n");
    prefs = this_player()->query_learn_pref(-1)[..(SS_NO_EXP_STATS-1)];
    /* One point difference between first and last stat means equal. */
    sorted_prefs = sort_array( ({ }) + prefs);
    if (abs(sorted_prefs[0] - sorted_prefs[SS_NO_EXP_STATS-1]) <= 1)
    {
        write("Your focus is distributed evenly over your stats.\n");
        return 1;
    }

    /* See if both mental and physical stats are at the same levels ... */
    if ((abs(prefs[SS_STR] - prefs[SS_CON]) <= 1) &&
        (abs(prefs[SS_STR] - prefs[SS_DEX]) <= 1) &&
        (abs(prefs[SS_WIS] - prefs[SS_INT]) <= 1) &&
        (abs(prefs[SS_WIS] - prefs[SS_DIS]) <= 1))
    {
        if (prefs[SS_WIS] - prefs[SS_CON] >= 2)
        {
            write("Your focus is towards the mental stats.\n");
            return 1;
        }

        if (prefs[SS_CON] - prefs[SS_WIS] >= 2)
        {
            write("Your focus is towards the physical stats.\n");
            return 1;
        }
    }

    index = member_array(sorted_prefs[SS_NO_EXP_STATS-1], prefs);
    write("Your primary focus is on " + SD_LONG_STAT_DESC[index] + ".\n");

    if (sorted_prefs[SS_NO_EXP_STATS-2] > sorted_prefs[SS_NO_EXP_STATS-3] + 1)
    {
        index = member_array(sorted_prefs[SS_NO_EXP_STATS-2], prefs);
        write("Your secondary focus is on " + SD_LONG_STAT_DESC[index] + ".\n");
    }
    write("The focus on your other stats is distributed evenly.\n");
    return 1;
}

/*
 * Function name: gs_rise
 * Description  : Player rises from meditation.
 * Returns      : int 1 - always.
 */
int
gs_rise()
{
    gs_hook_rise();
    this_player()->remove_prop(LIVE_I_MEDITATES);
    return 1;
}

/*
 * Function name: gs_restrict
 * Description  : This allows a player to restrict himself from playing.
 * Arguments    : string str - the command line argument.
 * Returns      : int 1 - always.
 */
static nomask int
gs_restrict(string str)
{
    int number;
    int seconds;
    string *words;

    if (str == "off")
    {
        if (this_player()->query_restricted())
        {
            this_player()->reset_restricted(1);
            write("You lift your self-imposed restriction.\n");
        }
        else
        {
            write("You had not imposed a restriction on yourself.\n");
        }

        return 1;
    }

    if (!strlen(str) ||
        !parse_command(str, ({ }),
             "'myself' 'for' %d [hour] [hours] [day] [days]", number))
    {
        write("Syntax: restrict off\n" +
            "        restrict myself for <number> hour/hours/day/days\n");
        return 1;
    }

    words = explode(str, " ");
    switch(words[sizeof(words) - 1])
    {
    case "hour":
    case "hours":
        if (number > 720)
        {
            write("You may not restrict yourself for more than 720 hours.\n");
            return 1;
        }
        str = "hour";
        seconds = (3600 * number);
        break;

    case "day":
    case "days":
        if (number > 30)
        {
            write("You may not restrict yourself for more than 30 days.\n");
            return 1;
        }
        str = "day";
        seconds = (86400 * number);
        break;

    default:
        write("Syntax: restrict myself for <number> hour/hours/day/days\n");
        return 1;
    }

    write("You have imposed a playing restriction on yourself for " + number +
        " " + str + ((number > 1) ? "s" : "") + ". This period starts now, " +
        "but will not come into effect until you log out. To lift this " +
        "restriction, meditate and type \"restrict off\" before you next " +
        "quit.\n\nNo new login is accepted from you until: " +
        ctime(time() + seconds) + "\n");
    this_player()->set_restricted(seconds, 1);

    return 1;
}

/*
 * Function name: gs_catch_all
 * Description  : Catch all commands the player makes while meditating.
 * Returns      : int 1/0 - success/failure.
 */
int
gs_catch_all(string arg)
{
    if (!this_player()->query_prop(LIVE_I_MEDITATES))
    {
        return 0;
    }

    switch(query_verb())
    {
    case "meditate":
        return gs_meditate("");

    case "set":
        set_prefs();
        return 1;

    case "rise":
        this_player()->remove_prop(LIVE_S_EXTRA_SHORT);
        gs_rise();
        return 1;

    case "restrict":
        gs_restrict(arg);
        return 1;

    case "help":
    case "stats":
    case "quit":
    case "save":
    case "drop": /* For those that quit from meditation */
    case "commune":
    case "reply":
    case "bug":
    case "typo":
    case "idea":
    case "praise":
    case "sysbug":
    case "systypo":
    case "syspraise":
    case "sysidea":
    case "looks":
        return 0;

    default:
        return gs_hook_catch_error(arg);
    }
}

/*
 * Function name: init_guild_support
 * Description  : Add the meditate command to the player. You must call this
 *                function from init() in your room.
 */
void
init_guild_support()
{
    add_action(gs_list, "list");
    add_action(gs_looks, "looks");
    add_action(gs_meditate, "meditate");
}
