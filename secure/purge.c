/*
 * /secure/purge.c
 *
 * After some time there may be a lot of idle characters around, wasting
 * the space and also taking up names that other people might want to
 * use.
 *
 * Wizards will not be purged. They have to be demoted first. Also, wizards
 * may be absent for a while. From each purge, a log is made. It is
 * recommented to check this log and to see which wizards are idle so long,
 * so they may be manually removed. Also, it lists files that are not normal
 * player characters.
 *
 * Some of these functions may seem a little robust and there indeed are a
 * lot of checks in this object, but then again, purging is serious business.
 */

#pragma no_clone
#pragma no_include
#pragma strict_types

inherit "/std/object";

#include <formulas.h>
#include <macros.h>
#include <mail.h>
#include <options.h>
#include <std.h>
#include <time.h>

#define MAX_PURGE       (50)
#define PURGE_LOG       ("/syslog/log/purge/PURGE")
#define PLAYER_FILES(c) (PLAYER_FILE_DIR + (c) + "/*")
/* Two years of idleness for each day of playing age. */
#define AGE_PLAY_FACTOR (365 * 2)

#define LOGIN_365_DAYS  (31536000) /* seconds */
#define LOGIN_180_DAYS  (15552000) /* seconds */
#define LOGIN_90_DAYS   (7776000)  /* seconds */
#define LOGIN_60_DAYS   (5184000)  /* seconds */
#define LOGIN_30_DAYS   (2592000)  /* seconds */
#define SECS_PER_DAY    (86400)    /* seconds */
#define AGE_TEN_DAYS    (432000)   /* heartbeats */
#define AGE_ONE_DAY     (43200)    /* heartbeats */
#define AGE_SIX_HOURS   (10800)    /* heartbeats */
#define AGE_ONE_HOUR    (1800)     /* heartbeats */
#define AGE_TEN_MINUTES (300)      /* heartbeats */
#define AGE_ONE_MINUTE  (30)       /* heartbeats */

/*
 * Static global variables.
 *
 * These global variables are private to this object. They have nothing to
 * do with the playerfiles that are being checked.
 */
private static object  purger;
private static string *purge_files;
private static string  purged_wizards;
private static string  notpurged;
private static string  purged_mortals;
private static string  strange_files;
private static string  purge_log;
private static int     num_wizards;
private static int     num_notpurged;
private static int     num_mortals;
private static int     num_strange;
private static int     num_deleted;
private static int     purge_index;
private static int     tested_files;

/*
 * Non-static global variables.
 *
 * These global variables are a part of the playerfiles that are being
 * checked.
 */
private string  name;       /* the name of the player             */
private int     login_time; /* the last time the player logged in */
private int     age_heart;  /* the of the player in heartbeats    */
private int    *acc_exp;    /* the acc-experience of the player   */
private mapping m_vars;     /* suspended or self-restricted       */
private mapping m_seconds;  /* registered seconds of the player   */

/*
 * Function name: create_object
 * Description  : Called when the object is created.
 */
public nomask void
create_object()
{
    set_short("another hole in the donut");

    purge_log = PURGE_LOG + "-" + TIME2FORMAT(time(), "yyyymmdd");

    setuid();
    seteuid(getuid());
}

/*
 * Function name: report_purge_done
 * Description  : When the purging is over, we make the final report and
 *                write that to disk. Naturally we also notify the purger
 *                if (s)he is still arround.
 */
static nomask void
report_purge_done()
{
    if (strlen(purged_wizards))
    {
        write_file(purge_log, "Found " + num_wizards + " wizard(s) that " +
            "have been idle too long.\nThey have not been purged. You shall " +
            "have to demote them manually:\n\n" + purged_wizards + "\n");
    }
    else
    {
        write_file(purge_log, "No overly idle wizards found this time.\n\n");
    }

    if (strlen(notpurged))
    {
        write_file(purge_log, "Found " + num_notpurged + " experienced/aged " +
	    "mortal(s) that have have been idle too long.\nThey have NOT " +
	    "been purged:\n\n" +
            notpurged + "\n");
    }
    else
    {
        write_file(purge_log,
            "No overly idle experienced/aged mortal players found this time.\n\n");
    }

    if (strlen(purged_mortals))
    {
        write_file(purge_log, "Found " + num_mortals + " mortal(s) that " +
            "have have been idle too long.\nThey have been purged:\n\n" +
            purged_mortals + "\n");
    }
    else
    {
        write_file(purge_log,
            "No overly idle mortal players found this time.\n\n");
    }

    if (num_deleted)
    {
        write_file(purge_log, "Cleaned up " + num_deleted +
            " old deleted player files.\n\n");
    }

    if (strlen(strange_files))
    {
        write_file(purge_log, "In addition, we have found " + num_strange +
            " files that are no normal playerfiles:\n\n" + strange_files +
            "\n");
    }

    if (objectp(purger))
    {
        tell_object(purger, "Purge done. Tested " + tested_files +
	    " files and purged " + num_mortals + " mortal " +
            "player(s).\nNot purged " + num_notpurged +
	    " idle aged/experienced player(s).\nFound " + num_wizards +
	    " overly idle wizard(s).\nThe wizards have not been purged. " +
            "Demote them first.\nCleaned up " + num_deleted +
            " deleted players.\n");
    }

    remove_object();
}

/*
 * Function name: purge_deleted_files
 * Description  : Removes all files of deleted players a while after their
 *                deletion.
 */
nomask void
purge_deleted_files()
{
    string *letters = explode(ALPHABET, "");
    string *files;

    foreach(string letter: letters)
    {
        files = get_dir(DELETED_FILE(letter) + "*.o");
        foreach(string filename: files)
        {
            if (file_time(DELETED_FILE(filename)) + LOGIN_365_DAYS < time())
            {
                rm(DELETED_FILE(filename));
                num_deleted++;
            }
        }
    }
    report_purge_done();
}

/*
 * Function name: last_date
 * Description  : Returns the date the player was last logged in. It only
 *                returns month, day and year.
 * Arguments    : int    - the time.
 * Returns      : string - the time.
 */
static nomask string
last_date(int last_time)
{
    string tmp = ctime(last_time);

    return extract(tmp, 4, 10) + extract(tmp, 20, 23);
}

/*
 * Function name: player_age
 * Description  : Returns the age of the player in the largest denomination.
 * Returns      : string - the age.
 */
static nomask string
player_age()
{
    return TIME2STR(age_heart * F_SECONDS_PER_BEAT, 1);
}

/*
 * Function name: player_average
 * Description  : Returns the average stat of the player.
 * Returns      : int - the average stat.
 */
static nomask int
player_average()
{
    return ((F_EXP_TO_STAT(acc_exp[0]) + F_EXP_TO_STAT(acc_exp[1]) +
             F_EXP_TO_STAT(acc_exp[2]) + F_EXP_TO_STAT(acc_exp[3]) +
             F_EXP_TO_STAT(acc_exp[4]) + F_EXP_TO_STAT(acc_exp[5])) / 6);
}

/*
 * Function name: purge_one
 * Description  : This function actually tests a file and purges it if
 *                necessary.
 * Arguments    : string filename - the name to test and possibly purge.
 */
static nomask void
purge_one(string filename)
{
    string  my_name;
    string *seconds;
    int     level;
    int     last_login;
    int     high_limit;
    int     junior;

    /* Reset the information we use. */
    name = 0;
    login_time = 0;

    /* This should be the name of the player. */
    my_name = extract(filename, 0, -3);

    /* If we cannot restore it, it is not a true playerfile. */
    if (!restore_object(PLAYER_FILE(my_name)))
    {
        strange_files += (filename + "\n");
        num_strange++;
        return;
    }

    /* Apparently not a true playerfile. The saved name does not match the
     * filename.
     */
    if (name != my_name)
    {
        strange_files += (filename + "\n");
        num_strange++;
        return;
    }

    if (!login_time)
    {
        rm(PLAYER_FILE(filename));
        purged_mortals += sprintf("%-11s %-13s (unfinished ghost)\n",
            capitalize(name), last_date(login_time));
        num_mortals++;
        return;
    }

    /* How long ago was their last login. */
    last_login = time() - login_time;

    /* Don't hurt players that are suspended by the administration or that have
     * restricted themselves.
     */
    if (mappingp(m_vars) && m_vars[SAVEVAR_RESTRICT])
    {
        return;
    }
    junior = (extract(name, -2) == "jr");

    /* If the player is a wizard, report him but don't hurt him. */
    if ((level = SECURITY->query_wiz_rank(name)) ||
        (junior && (level = SECURITY->query_wiz_rank(extract(name, 0, -3)))))
    {
        if ((last_login > LOGIN_365_DAYS) && !junior)
        {
            purged_wizards += sprintf("%-11s %-13s = %6s | %-10s\n",
                capitalize(name), last_date(login_time), TIME2STR(last_login, 1),
                WIZ_RANK_NAME(level));
            num_wizards++;
        }
        return;
    }

    /* If a player is old or has a lot of experience, he can be idle a bit
     * longer than other people.
     */
    level = player_average();

    /* Age related checks, Play for a day ... idle for a year.  */
    if (((age_heart > AGE_ONE_HOUR) && (last_login < LOGIN_365_DAYS)) ||
        ((age_heart > AGE_TEN_MINUTES) && (last_login < LOGIN_180_DAYS)) ||
        (last_login < (age_heart * F_SECONDS_PER_BEAT * AGE_PLAY_FACTOR)))
    {
        notpurged += sprintf("%-11s %-13s = %6s | %3d stat | %5s age | NoPurge Age\n",
            capitalize(name), last_date(login_time), TIME2STR(last_login, 1),
            level, player_age());
        num_notpurged++;
        return;
    }

    /* Find out if he's a wizard second. */
    seconds = SECURITY->query_seconds(name);
    foreach(string second: seconds)
    {
        /* Advise the wizard, but registered jr's pass unnoticed. */
        if (SECURITY->query_wiz_rank(second) && !junior)
        {
            /* Mail the confirmation. Subject, author, to, cc, body. */
            CREATE_MAIL(("Non-purging of " + capitalize(name)), "root",
                second, "", "Your second " + capitalize(name) +
                " was up for purging. However, since you are a wizard, the " +
                "deities have once again shown their benevolence.\n\n" +
                "Last login: " + last_date(login_time) + "\n");
            purged_mortals += sprintf("%-11s %-13s = %6s | %3d stat | %5s age | NoPurge second of %s\n",
                capitalize(name), last_date(login_time), TIME2STR(last_login, 1),
                level, player_age(), capitalize(second));
            num_mortals++;
            return;
        }
    }

    /* Oke.. Player has been idle too long. Lets purge him/her */
    SECURITY->remove_playerfile(name, "Purge", 1);
    purged_mortals += sprintf("%-11s %-13s = %6s | %3d stat | %5s age\n",
        capitalize(name), last_date(login_time), TIME2STR(last_login, 1),
	level, player_age());
    num_mortals++;
}

/*
 * Function name: delayed_purge
 * Description  : We use alarms to prevent problems with eval-cost when
 *                testing a lot of players.
 */
static nomask void
delayed_purge()
{
    string letter;
    int limit;

    /* No files left to purge. Lets check the next character. */
    if (!sizeof(purge_files))
    {
        if (++purge_index >= strlen(ALPHABET))
        {
            set_alarm(2.0,0.0, purge_deleted_files);
            return;
        }

        letter = extract(ALPHABET, purge_index, purge_index);
        tell_object(purger, "Purge: " + num_mortals + " mortals, " +
            num_wizards + " wizards. Purging letter " + capitalize(letter) + ".\n");
        purge_files = get_dir(PLAYER_FILES(letter) + "*.o");
        /* Don't bother about the predeath files. */
        purge_files = filter(purge_files, &operator(!=)(".predeath.o", ) @ &extract(, -11));
    }

    limit = ((sizeof(purge_files) > MAX_PURGE) ? MAX_PURGE : sizeof(purge_files));
    tested_files += limit;

    foreach(string filename: purge_files[..(limit-1)])
    {
        purge_one(filename);
    }

    purge_files = purge_files[limit..];

    set_alarm(2.0, 0.0, delayed_purge);
}

/*
 * Function name: purge
 * Description  : Clean out old and idle playercharacters. You need to be
 *                an archwizard or keeper to be allowed to execute the
 *                purge command or you may find your own character purged.
 * Arguments    : string str - the command line argument.
 */
public nomask int
purge(string str)
{
    if (!CALL_BY(WIZ_CMD_ARCH))
    {
        write("You are not allowed to use this command.\n");
        return 1;
    }

    if (str != "players")
    {
        write("Syntax: 'purge players'.\n");
        return 1;
    }

    if (file_size(purge_log) > 0)
    {
        rename(purge_log, (purge_log + ".old"));
    }

    purger         = this_player();
    purge_files    = ({ });
    num_wizards    = 0;
    num_mortals    = 0;
    num_notpurged  = 0;
    num_strange    = 0;
    tested_files   = 0;
    purge_index    = -1;
    purged_wizards = "";
    purged_mortals = "";
    notpurged      = "";
    strange_files  = "";

    write("Purge started. You shall be notified when the purge is done.\n");
    write_file(purge_log, "Purge executed by " +
        capitalize(purger->query_real_name()) + ".\nDate: " +
        ctime(time()) + ".\n\n");

    delayed_purge();
    return 1;
}
