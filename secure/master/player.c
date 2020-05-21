/*
 * /secure/master/player.c
 *
 * This file is a sub-part of SECURITY related to player matters, most notably
 * registration of seconds and characters marked for deletion due to
 * inappropriate names.
 */

#include "/sys/log.h"

#define PREDEATH_CLEANUP 31536000 /* one year */
#define BAD_NAME_CLEANUP  1209600 /* two weeks */
#define NEW_CHAR_CLEANUP  1209600 /* two weeks */
#define NEW_CHAR_MINAGE      3600 /* two hours in heartbeats */

/* Indices to the m_seconds mapping. */
#define SNDS_REPORTER 0
#define SNDS_TIME     1

/*
 * Global variable in the seconds save-file:
 *
 * The record consists of a first player with a number of associated seconds.
 * This prevents a double/duplex bookkeeping. For each second, we remember who
 * reported the second, and when.
 *
 * m_seconds = ([ (string) player :
 *                ([ (string) second :
 *                   ({ (string) reporter, (int) time }) ]) ])
 */
static private mapping m_seconds = ([ ]);

/*
 * At run-time we maintain a simple index from each second to the first.
 *
 * m_firsts = ([ (string) player : (string) first ])
 */
static private mapping m_firsts = ([ ]);

/*
 * Global variable in KEEPERSAVE with the names of players marked for deletion
 * due to inappropriate names.
 *
 * m_badnames = ([ (string) player : ({ (string) wizard, (int) time }) ])
 */
private mapping m_badnames = ([ ]);

/*
 * Global variable in KEEPERSAVE with the names of recently created players.
 *
 * m_newchars = ([ (string) player : (int) time ])
 */
private mapping m_newchars = ([ ]);

/*
 * Function name: save_seconds
 * Description  : Since the seconds information is stored in a separate
 *                place, we have a separate routine for it.
 */
static void
save_seconds()
{
    set_auth(this_object(), "root:root");
    save_map(m_seconds, SECONDS_SAVE);
}

/*
 * Function name: query_find_first
 * Description  : Find out who is the first of the second.
 * Arguments    : string name - the lower case name of the second.
 * Returns      : string - the first associated with the second. Note this
 *                    may be the person itself if it really is the first.
 */
public string
query_find_first(string name)
{
    string first = (m_firsts[name] ? m_firsts[name] : name);

    if (!valid_player_info(previous_object(), first, "second"))
    {
        return 0;
    }

    return (m_seconds[first] ? first : 0);
}

/*
 * Function name: query_is_second
 * Description  : Find out whether two players are seconds. Note that the
 *                order of the arguments is irrelevant.
 * Arguments    : string first - the lower case name of the first player.
 *                string second - the lower case name of the second player.
 * Returns      : int 1/0 - if true, they are seconds.
 */
public int
query_is_second(string first, string second)
{
    if (!valid_player_info(previous_object(), first, "second"))
    {
        return 0;
    }

    /* The order of the arguments doesn't matter. */
    return (m_firsts[second] == first) || (m_firsts[first] == second);
}

/*
 * Function name: query_seconds
 * Description  : Find out the names of the seconds of a certain player.
 * Arguments    : string name - the lower case name of the player.
 * Returns      : string *names - the names of the seconds, 0 means no access.
 */
public string *
query_seconds(string name)
{
    string first = (m_firsts[name] ? m_firsts[name] : name);
    string *names;

    if (!valid_player_info(previous_object(), first, "second"))
    {
        return 0;
    }

    /* Seconds found. */
    if (sizeof(names = m_indices(m_seconds[first])))
    {
        /* Add the first to the seconds. */
        names = ({ first }) + names;
        /* But don't return the queried player. */
        return names - ({ name });
    }
    return ({ });
}

/*
 * Function name: query_player_seconds
 * Description  : Find out the names of the seconds of a player that were
 *                registered by that player. This routine operates on
 *                this_interactive().
 * Returns      : string *names - the names of the seconds.
 */
public string *
query_player_seconds()
{
    string name = this_interactive()->query_real_name();
    string first = (m_firsts[name] ? m_firsts[name] : name);
    string *seconds = m_indices(m_seconds[first]);

    /* Only return those seconds listed by the player. */
    foreach(string second: seconds)
    {
        if (m_seconds[first][second][SNDS_REPORTER] != first)
        {
            seconds -= ({ second });
        }
    }

    if (sizeof(seconds))
    {
        /* Add the first to the seconds. */
        seconds = ({ first }) + seconds;
        /* But don't return the querying player. */
        return seconds - ({ name });
    }
    return ({ });
}

/*
 * Function name: query_second_info
 * Description  : Obtain more information about a second, namely the person
 *                who registered it, and when.
 * Arguments    : string first - the lower case name of the first player.
 *                string second - the lower case name of the second player.
 * Returns      : mixed - ({ string name, int time })
 */
public mixed
query_second_info(string first, string second)
{
    string wname = geteuid(previous_object());

    if (!valid_player_info(wname, first, "second"))
    {
        return 0;
    }

    if (!m_seconds[first])
    {
        return 0;
    }

    return secure_var(m_seconds[first][second]);
}

/*
 * Function name: add_second
 * Description  : Called (from the wizard soul) to add a second to a certain
 *                first. Note that the order is relevant.
 * Arguments    : string first - the lower case name of the first player.
 *                string second - the lower case name of the second player.
 * Returns      : int 1/0 - success/failure.
 */
public int
add_second(string first, string second)
{
    string wname = geteuid(previous_object());

    first = lower_case(first);
    second = lower_case(second);
    /* If the first is a second, find the real first. */
    first = (m_firsts[first] ? m_firsts[first] : first);

    if (!valid_player_info(wname, first, "second"))
    {
        return 0;
    }

    /* Second is really a first. */
    if (m_seconds[second])
    {
        notify_fail("The player " + capitalize(second) +
            " already has seconds registered.\n");
        return 0;
    }

    /* Second already has a first. */
    if (m_firsts[second])
    {
        notify_fail("The player " + capitalize(second) +
            " is already a second of " + capitalize(m_firsts[second]) + ".\n");
        return 0;
    }

    /* The actual addition. */
    if (!mappingp(m_seconds[first]))
    {
        m_seconds[first] = ([ ]);
    }
    m_seconds[first] += ([ second : ({ wname, time() }) ]);
    m_firsts[second] = first;
    save_seconds();
#ifdef LOG_SECONDS
    log_file(LOG_SECONDS, (ctime(time()) + " " +
        capitalize(wname) + " added " + capitalize(second) +
        " to " + capitalize(first) + ".\n"));
#endif LOG_SECONDS
    return 1;
}

/*
 * Function name: remove_second
 * Description  : Called (from the wizard soul) to remove a second from a
 *                certain first. Note that the order is relevant.
 * Arguments    : string first - the lower case name of the first player.
 *                string second - the lower case name of the second player.
 * Returns      : int 1/0 - success/failure.
 */
public int
remove_second(string first, string second)
{
    string wname = geteuid(previous_object());

    first = lower_case(first);
    second = lower_case(second);
    /* If the first is a second, find the real first. */
    first = (m_firsts[first] ? m_firsts[first] : first);

    if (!valid_player_info(wname, first, "second"))
    {
        return 0;
    }

    if (m_firsts[second] != first)
    {
        notify_fail("The player " + capitalize(second) + " is not a second of " +
            capitalize(first) +
            (m_firsts[second] ? " (but of " + capitalize(m_firsts[second]) + ") ": "") +
            ".\n");
        return 0;
    }

    /* The actual removal. If there is no second left, remove the first too. */
    m_delkey(m_seconds[first], second);
    if (!m_sizeof(m_seconds[first]))
    {
        m_delkey(m_seconds, first);
    }
    m_delkey(m_firsts, second);
    save_seconds();
#ifdef LOG_SECONDS
    log_file(LOG_SECONDS, (ctime(time()) + " " +
        capitalize(wname) + " removed " + capitalize(second) +
        " from " + capitalize(first) + ".\n"));
#endif LOG_SECONDS
    return 1;
}

/*
 * Function name: set_second_first
 * Description  : Called (from the wizard soul) to mark a second as first.
 *                It only changes the order of the data structure.
 * Arguments    : string newfirst - the lower case name of the second player.
 * Returns      : int 1/0 - success/failure.
 */
public int
set_second_first(string newfirst)
{
    string wname = geteuid(previous_object());
    string oldfirst;

    newfirst = lower_case(newfirst);
    if (!valid_player_info(wname, newfirst, "second"))
    {
        return 0;
    }

    if (m_seconds[newfirst])
    {
        notify_fail(capitalize(newfirst) + " is already marked as the first character.\n");
        return 0;
    }
    oldfirst = m_firsts[newfirst];
    if (!oldfirst)
    {
        notify_fail(capitalize(newfirst) + " is not marked as second of anyone.\n");
        return 0;
    }

    /* Copy seconds from oldfirst. */
    m_seconds[newfirst] = m_seconds[oldfirst];
    /* The old first becomes the new second. Copy assignment details. */
    m_seconds[newfirst][oldfirst] = m_seconds[oldfirst][newfirst];
    /* New first is not a second of itself. */
    m_delkey(m_seconds[newfirst], newfirst);
    /* Old first a not first anymore, and new first isn't a second. */
    m_delkey(m_seconds, oldfirst);
    m_delkey(m_firsts, newfirst);

    /* Relist seconds to new first. */
    foreach(string second: m_indices(m_seconds[newfirst]))
    {
        m_firsts[second] = newfirst;
        if (m_seconds[newfirst][second][SNDS_REPORTER] == oldfirst)
        {
            m_seconds[newfirst][second][SNDS_REPORTER] = newfirst;
        }
    }
    save_seconds();
#ifdef LOG_SECONDS
    log_file(LOG_SECONDS, ctime(time()) + " " + capitalize(wname) + " sets " +
        capitalize(newfirst) + " as first over " + capitalize(oldfirst) + ".\n");
#endif LOG_SECONDS
    return 1;
}

/*
 * Function name: remove_player_seconds
 * Description  : Remove a player from the seconds administration. Generally
 *                this is done when the player is deleted.
 * Arguments    : string pname - the name of the player.
 */
static void
remove_player_seconds(string pname)
{
    string first;
    string *seconds;

    /* Person is a second to a first. */
    if (first = m_firsts[pname])
    {
        m_delkey(m_firsts, pname);
        m_delkey(m_seconds[first], pname);
        /* No seconds left, so he's not a first anymore either. */
        if (!m_sizeof(m_seconds[first]))
        {
            m_delkey(m_seconds, first);
        }
        save_seconds();
#ifdef LOG_SECONDS
        log_file(LOG_SECONDS, (ctime(time()) + " " + capitalize(pname) +
            " deleted from " + capitalize(first) + ".\n"));
#endif LOG_SECONDS
        return;
    }

    /* Get the seconds associated with the first. */
    seconds = m_indices(m_seconds[pname]);
    /* Person has no seconds, so he's not really a first. */
    if (!sizeof(seconds))
    {
        m_delkey(m_seconds, pname);
        return;
    }

    /* First had only one second, release the bond. */
    if (sizeof(seconds) == 1)
    {
        m_delkey(m_seconds, pname);
        save_seconds();
        m_delkey(m_firsts, seconds[0]);
#ifdef LOG_SECONDS
        log_file(LOG_SECONDS, (ctime(time()) + " " + capitalize(pname) +
            " deleted, freeing " + capitalize(seconds[0]) + ".\n"));
#endif LOG_SECONDS
        return;
    }

    /* The first second becomes the first, reassign the others. */
    first = seconds[0];
    seconds = seconds[1..];
    /* Take over the seconds from the old first. */
    m_seconds[first] = secure_var(m_seconds[pname]);
    m_delkey(m_seconds, pname);
    /* New first isn't a second of itself. */
    m_delkey(m_seconds[first], first);
    m_delkey(m_firsts, first);

    save_seconds();

    foreach(string second: seconds)
    {
        m_firsts[second] = first;
#ifdef LOG_SECONDS
        log_file(LOG_SECONDS, (ctime(time()) + " " + capitalize(second) +
            " relisted to " + capitalize(first) + " from " +
            capitalize(pname) + ".\n"));
#endif LOG_SECONDS
    }
}

/*
 * Function name: register_second
 * Description  : Called (from the player soul) for a player to register a
 *                second player. Note we don't do notify_fail() in this
 *                routine since it isn't called directly from the command.
 * Arguments    : string second - the lower case name of the second to add.
 *                string password - the password of teh second to add.
 * Returns      : int 1/0 - success/failure.
 */
public int
register_second(string second, string password)
{
    string pname = this_interactive()->query_real_name();
    string first = (m_firsts[pname] ? m_firsts[pname] : pname);
    string tmp;
    object player;
    int match;

    second = lower_case(second);

    /* Verify whether the first knows the password of the second. */
    player = finger_player(second);
    match = player->match_password(password);
    player->remove_object();
    if (!match)
    {
        write("The password of " + capitalize(second) + " is not correct.\n");
        return 0;
    }

    /* In principle the wizard is the first, unless he's a second. */
    if (query_wiz_level(second) && !m_firsts[second])
    {
        tmp = first; first = second; second = tmp;
    }

    /* Second is already a first. */
    if (m_seconds[second])
    {
        /* Both are firsts. This won't work. */
        if (m_seconds[first])
        {
#ifdef LOG_SECONDS
            log_file(LOG_SECONDS, (ctime(time()) + " " +
                capitalize(pname) + " tries to merge " +
                capitalize(second) + " and " + capitalize(first) + ").\n"));
#endif LOG_SECONDS
            write("Second " + capitalize(second) +
                " appears to have seconds as well. Please consult the AoP.\n");
            return 0;
        }
        /* Swap the first and the second. */
        tmp = first; first = second; second = tmp;
    }

    /* The second is already registered. */
    if (m_firsts[second])
    {
        /* Second is registered to someone else. */
        if (m_firsts[second] != first)
        {
#ifdef LOG_SECONDS
            log_file(LOG_SECONDS, (ctime(time()) + " " +
                capitalize(pname) + " tries to register " +
                capitalize(second) + " (first is " +
                capitalize(m_firsts[second]) + ").\n"));
#endif LOG_SECONDS
            write(capitalize(second) + " appears to be registered already. " +
                "Register a new second from an existing character. When in " +
                "doubt, please consult the AoP.\n");
            return 0;
        }

        /* Player has already registered this second. */
        if (m_seconds[first][second][SNDS_REPORTER] == first)
        {
            write("You already registered " + capitalize(second) + ".\n");
            return 0;
        }

        /* Player registers a second already listed by wizard. */
#ifdef LOG_SECONDS
        log_file(LOG_SECONDS, (ctime(time()) + " " +
            capitalize(pname) + " registers " + capitalize(second) +
            " (previously by " +
            capitalize(m_seconds[first][second][SNDS_REPORTER]) + ").\n"));
#endif LOG_SECONDS
        m_seconds[first][second] = ({ first, time() });
        save_seconds();
        write("You registered " + capitalize(second) + " as second.\n");
        return 1;
    }

    /* New registration. */
    if (!mappingp(m_seconds[first]))
    {
        m_seconds[first] = ([ ]);
    }
    m_seconds[first] += ([ second : ({ first, time() }) ]);
    m_firsts[second] = first;
    save_seconds();
    /* If it's the second doing the logging, correct the logging. */
    if (pname == second)
    {
        pname = first;
    }
#ifdef LOG_SECONDS
    log_file(LOG_SECONDS, (ctime(time()) + " " +
        capitalize(pname) + " registers " + capitalize(second) + ".\n"));
#endif LOG_SECONDS
    write("You registered " + capitalize(second) + " as second.\n");
    return 1;
}

/*
 * Function name: init_player_info
 * Description  : Fill the m_firsts mapping with a combination of all seconds
 *                pointing to the firsts they are a second to.
 */
static void
init_player_info()
{
    string *seconds;

    m_seconds = restore_map(SECONDS_SAVE);
    if (!mappingp(m_seconds))
    {
        m_seconds = ([ ]);
    }

    foreach(string first: m_indices(m_seconds))
    {
        /* Guard against file errors. */
        if (!mappingp(m_seconds[first]))
        {
            m_delkey(m_seconds, first);
            continue;
        }

        seconds = m_indices(m_seconds[first]);
        /* Guard against file errors. */
        if (!pointerp(seconds))
        {
            m_delkey(m_seconds, first);
            continue;
        }

	/* Check if the seconds exist. */
        foreach(string second: seconds)
        {
	    if (!exist_player(second))
	    {
                m_seconds[first] = m_delete(m_seconds[first], second);
	    }
        }

        seconds = m_indices(m_seconds[first]);
        /* No seconds (left), remove the first. */
        if (!sizeof(seconds))
        {
            m_delkey(m_seconds, first);
	    continue;
        }

        /* If the first doesn't exist, juggle a second into first. */
        if (!exist_player(first))
        {
            /* Just one second and no first really isn't a second. */
            if (sizeof(seconds) == 1)
            {
                m_delkey(m_seconds, first);
                continue;
            }
            m_seconds[seconds[0]] = m_delete(m_seconds[first], seconds[0]);
            m_delkey(m_seconds, first);
            first = seconds[0];
            seconds = seconds[1..];
        }

	/* Prepare the reverse mapping of seconds to first. */
        foreach(string second: seconds)
	{
            m_firsts[second] = first;
        }
    }
}

/*
 * Function name: is_bad_name
 * Description  : Find out if a certain name is registered as a bad name
 *                marked for deletion.
 * Arguments    : string name - the (lower case) name of the player.
 * Returns      : int 1/0 - if true, is marked.
 */
public int
is_bad_name(string name)
{
    return pointerp(m_badnames[name]);
}

/*
 * Function name: query_bad_names
 * Description  : Find out the list of names that are identified as bad names,
 *                marked for deletion.
 * Returns      : string * - the list of lower case names.
 */
public string *
query_bad_names()
{
    return m_indices(m_badnames);
}

/*
 * Function name: query_bad_name_info
 * Description  : Find out who marked the name as bad, and when.
 * Arguments    : string name - the (lower case) name of the player.
 * Returns      : mixed ({ name, time, #logins })
 */
public mixed
query_bad_name_info(string name)
{
    return secure_var(m_badnames[name]);
}

/*
 * Function name: set_bad_name
 * Description  : Identify a name as inappropriate and mark for deletion.
 * Arguments    : string name - the (lower case) name of the player.
 * Returns      : int 1/0 - if true, is marked.
 */
public int
set_bad_name(string name)
{
    string wname = this_interactive()->query_real_name();
    name = lower_case(name);

    if (query_wiz_rank(name))
    {
	return 0;
    }
    if (m_badnames[name])
    {
        return 0;
    }
    m_badnames[name] = ({ wname, time() });
    
    /* Log the action */
    log_file("BADNAME", ctime(time()) + " " + capitalize(name) +
        " -> bad name by " + capitalize(this_interactive()->query_real_name()) +
        ".\n", -1);

    return 1;
}

/*
 * Function name: remove_bad_name
 * Description  : Remove a name from the list of bad names.
 * Arguments    : string name - the (lower case) name of the player.
 */
public void
remove_bad_name(string name)
{
    m_delkey(m_badnames, lower_case(name));
}

/*
 * Function name: add_new_char
 * Description  : Called when a new character is created. Used to do a cleanup
 *                if they haven't been active within a while after creation.
 * Arguments    : string name - the (lower case) name of the new player.
 */
public void
add_new_char(string name)
{
    m_newchars[name] = time();
}

/*
 * Function name: remove_new_char
 * Description  : Remove a name from the list of new characters.
 * Arguments    : string name - the (lower case) name of the player.
 */
public void
remove_new_char(string name)
{
    m_delkey(m_newchars, lower_case(name));
}

/*
 * Function name: purge_new_chars
 * Description  : A little garbage collector that will remove the new players
 *                who haven't been active after the first creation.
 *                We assume that due to the moderate volume, this can be done
 *                in a single call without eval-cost issues.
 */
public void
purge_new_chars()
{
    string *names = m_indices(m_newchars);
    object player;
    int    age;

    set_auth(this_object(), "root:root");

    foreach(string name: names)
    {
        /* Not yet old enough, ignore and revisit later. */
        if ((m_newchars[name] + NEW_CHAR_CLEANUP) > time())
        {
            continue;
        }
        player = finger_player(name);
        age = player->query_age();
        player->remove_object();
        /* Too old, wait for regular purge. */
        if (age > NEW_CHAR_MINAGE)
        {
            remove_new_char(name);
            continue;
        }
        this_object()->log_public("ABANDON_PLAYER",
            sprintf("%s %-11s (%s since %s)\n", ctime(time()),
            capitalize(name), TIME2STR(age * 2, 2),
            TIME2FORMAT(m_newchars[name], "dd mmm yyyy")));
        this_object()->remove_playerfile(name, "Abandoned Newbie", 1);
    }
}

/*
 * Function name: purge_bad_names
 * Description  : A little garbage collector that will remove the players
 *                with inappropriate names that are overdue for a change.
 *                We assume that due to the moderate volume, this can be done
 *                in a single call without eval-cost issues.
 */
public void
purge_bad_names()
{
    string *names = m_indices(m_badnames);

    set_auth(this_object(), "root:root");

    foreach(string name: names)
    {
	if ((file_time(PLAYER_FILE(name) + ".o") + BAD_NAME_CLEANUP) < time())
	{
	    this_object()->remove_playerfile(name, "BadName Purge");
	}
    }
}

/*
 * Function name: purge_predeath
 * Description  : A little garbage collector that will remove the predeath
 *                files after they reach a certain age. We assume that due to
 *                the moderate volume, this can be done in a single call
 *                without eval-cost issues.
 */
public void
purge_predeath()
{
    string *files;

    set_auth(this_object(), "root:root");

    foreach(string letter: explode(ALPHABET, ""))
    {
        files = get_dir(PLAYER_FILE(letter) + "*.predeath.o");
	foreach(string file: files)
	{
	    if ((file_time(PLAYER_FILE(file)) + PREDEATH_CLEANUP) < time())
	    {
	        rm(PLAYER_FILE(file));
	    }
	}
    }
}
