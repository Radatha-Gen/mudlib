/*
 * /cmd/wiz/wizard/communication.c
 *
 * This is a sub-part of /cmd/wiz/wizard.c
 *
 * This file contains the commands that allow wizards to communicate.
 *
 * Commands currently included:
 * - dtell
 * - dtelle
 * - echo
 * - echoto
 * - tellall
 */

/* **************************************************************************
 * dtell  - actually an alias for 'line <my domain>'
 * dtelle - actually an alias for 'linee <my domain>'
 */
nomask int
dtell(string str)
{
    string domain;

    if (!stringp(str))
    {
        notify_fail(capitalize(query_verb()) + " what?\n");
        return 0;
    }

    if (!strlen(domain =
        SECURITY->query_wiz_dom(this_interactive()->query_real_name())))
    {
        notify_fail("You are not a member of any domain.\n");
        return 0;
    }

    return WIZ_CMD_APPRENTICE->line((domain + " " + str),
        (query_verb() == "dtelle"));
}

/* **************************************************************************
 * echo - echo something in a room
 */
nomask int
echo(string str)
{
    CHECK_SO_WIZ;

    if (!stringp(str))
    {
        notify_fail("Echo what?\n");
        return 0;
    }

    say(str + "\n");

#ifdef LOG_ECHO
    if (SECURITY->query_wiz_rank(this_player()->query_real_name()) < WIZ_ARCH)
    {
        SECURITY->log_syslog(LOG_ECHO, sprintf("%s %-11s: %s\n", ctime(time()),
            capitalize(this_interactive()->query_real_name()), str));
    }
#endif LOG_ECHO

    if (this_player()->query_option(OPT_ECHO))
    {
        write("You echo: " + str + "\n");
    }
    else
    {
        write("Ok.\n");
    }
    return 1;
}

/* **************************************************************************
 * echoto - echo something to someone
 */
nomask int
echo_to(string str)
{
    object ob;
    string who, msg;

    CHECK_SO_WIZ;

    if (!stringp(str) ||
        (sscanf(str, "%s %s", who, msg) != 2))
    {
        notify_fail("Syntax: echoto <who> <what> ?\n");
        return 0;
    }

    ob = find_player(lower_case(who));
    if (!objectp(ob))
    {
        notify_fail("No player " + capitalize(who) + " in the game.\n");
        return 0;
    }

#ifdef LOG_ECHO
    if (SECURITY->query_wiz_rank(this_player()->query_real_name()) < WIZ_ARCH)
    {
        SECURITY->log_syslog(LOG_ECHO, sprintf("%s %-11s to %-11s: %s\n",
            ctime(time()), capitalize(this_interactive()->query_real_name()),
            capitalize(who), msg));
    }
#endif LOG_ECHO

    tell_object(ob, msg + "\n");
    if (this_player()->query_option(OPT_ECHO))
    {
        write("You echo to " + capitalize(who) + ": " + msg + "\n");
    }
    else
    {
        write("Ok.\n");
    }

    return 1;
}

/* **************************************************************************
 * tellall - tell something to everyone
 */
nomask int
tellall(string str)
{
    string name = this_player()->query_real_name();
    string tell_text, tell_who;
    int index = -1;
    int size;
    object *players;
    string *names;

    /* We do NOT add the wizard-check here since Armageddon should be
     * able to use the function.
     */

    if (!stringp(str))
    {
        notify_fail("Tellall what?\n");
        return 0;
    }

    players = users() - ({ this_player(), 0 });
    players -= (object *)QUEUE->queue_list(0);
    foreach(object player: players)
    {
        if (player->query_wiz_level())
        {
	    tell_text = capitalize(name) + " tells everyone: " + str;
            tell_object(player, tell_text + "\n");
            player->gmcp_comms("tellall", capitalize(name), tell_text);
        }
        else
        {
	    tell_who = "An apparition of " + this_player()->query_art_name(player);
            tell_object(player, tell_who + " appears before you.\n" +
                capitalize(this_player()->query_pronoun()) +
                " tells everyone: " + str +
                "\nThe figure then disappears.\n");
            player->gmcp_comms("tellall", tell_who,
	        tell_who + " tells everyone: " + str);
        }

        names = player->query_prop(PLAYER_AS_REPLY_WIZARD);
        if (pointerp(names))
        {
            names = ({ name }) + (names - ({ name }) );
        }
        else
        {
            names = ({ name });
        }
        player->add_prop(PLAYER_AS_REPLY_WIZARD, names);
    }

    tell_text = "You tell everyone: " + str;
    if (this_player()->query_option(OPT_ECHO))
        write(tell_text + "\n");
    else
        write("Ok.\n");
    this_player()->gmcp_comms("tellall", 0, tell_text);
    return 1;
}

