/*
 * /secure/report_central.c
 *
 * This object handles the reports files against domains, or root.
 */

#pragma strict_types
#pragma no_clone
#pragma no_inherit
#pragma no_shadow

#include <composite.h>
#include <files.h>
#include <filter_funs.h>
#include <macros.h>
#include <std.h>
#include <time.h>

/*
 * The owner (wizard, domain, root) is always in lower case.
 *
 * m_report_owners = ([ (string)owner : (int)last_report ])
 * m_report_wizards = ([ (string)wizard : ([ (string)owner : (int)last_report ]) ])
 *
 * m_reports = ([ (string)owner :
 *     ({ ({ (string)Author, (int)type, (int)status, (int)time, (string)file }), ... }) ])
 *
 * m_users = ([ (object)wizard : ({ gCurrent, gPrevious, gOwner, gType }) ])
 */
mapping m_report_owners;
mapping m_report_wizards;
static mapping m_reports;
static mapping m_users = ([ ]);

static int    gCurrent;  /* Index of the current report (starting with 1). */
static int    gPrevious; /* The previous gCurrent. */
static string gOwner;    /* Name of the (lower case) owner that is opened. */
static string gType;     /* Type of report that is opened. */

/*
 * Prototypes
 */
static void get_cmd(string str);
static void header(string str);

/*
 * Index to the report index.
 */
#define REPORT_AUTHOR   0
#define REPORT_TYPE     1
#define REPORT_STATUS   2
#define REPORT_TIME     3
#define REPORT_PATH     4

#define REPORT_BUG      1
#define REPORT_TYPO     2
#define REPORT_IDEA     3
#define REPORT_PRAISE   4

#define REPORT_DONE     1
#define REPORT_DELETED  2

#define REPORT_NAME_ALL  "all"
#define REPORT_NAME_BUG  "bug"
#define REPORT_NAME_TYPO "typo"
#define REPORT_NAME_IDEA "idea"
#define REPORT_NAME_PRAISE "praise"
#define REPORT_NAMES_SHORT ({ "-", "B", "T", "I", "P" })
#define REPORT_NAMES_LONG  ({ "all", "bug", "typo", "idea", "praise" })
#define REPORT_NAMES_MAP  ([ "bug" : REPORT_BUG, "typo" : REPORT_TYPO, "idea" : REPORT_IDEA, "praise": REPORT_PRAISE ])
#define REPORT_STATUS_SHORT ([ 0 : "-", REPORT_DONE: "D", REPORT_DELETED: "X", (REPORT_DONE | REPORT_DELETED) : "X" ])
#define REPORT_STATUS_LONG  ([ 0 : "-", REPORT_DONE: "Done", REPORT_DELETED: "Deleted" ])

#define REPORT_FILENAME(num) sprintf("/syslog/reports/d%d/r%d", ((num) % 100), (num))

#define REPORT_SAVE_WIZARDS  "/syslog/reports/report_save"
#define REPORT_SAVE_HEADERS  "/syslog/reports/report_headers"

#define STRING(arr)   (map(arr, &operator(+)("")))

#define AUTO_MORE_LEN 2000
#define SCROLL_SIZE     20
#define NO_LOOP       "_no_loop_"

#define NO_READ_ACCESS(o)  (!check_rw_access((o), 0))
#define NO_WRITE_ACCESS(o) (!check_rw_access((o), 1))

/*
 * Function name: create
 * Description  : Constructor - load the files and process them.
 */
void
create()
{
    string owner, type;

    setuid();
    seteuid(getuid());

    restore_object(REPORT_SAVE_WIZARDS);
    if (!mappingp(m_report_owners)) m_report_owners = ([ ]);
    if (!mappingp(m_report_wizards)) m_report_wizards = ([ ]);

    m_reports = restore_map(REPORT_SAVE_HEADERS);
    if (!mappingp(m_reports)) m_reports = ([ ]);

    foreach(string owner: m_indices(m_reports))
    {
	if (!sizeof(m_reports[owner]))
	{
	    m_report_owners[owner] = time();
	}
	else
	{
            m_report_owners[owner] = max(m_report_owners[owner],
                applyv(max, map(m_reports[owner],
			&operator([])(, REPORT_TIME))));
	}
    }
}

/*
 * Function name: save_reports
 * Description  : Called to save the report index when something has
 *                changed. This should be done somewhat sparingly as the
 *                index can be large ...
 */
static void
save_reports()
{
    save_map(m_reports, REPORT_SAVE_HEADERS);
}

/*
 * Function name: save_wizards
 * Description  : Called to save the wizard indices whenever a wizard does
 *                an action.
 */
static void
save_wizards()
{
//  save_map(m_report_wizards, REPORT_SAVE_WIZARDS);
    save_object(REPORT_SAVE_WIZARDS);
}

/*
 * Function name: check_rw_access
 * Description  : Find out if the actor has read/write access.
 * Arguments    : string owner - the owner to verify.
 *                int rw - if true, wants to write.
 * Returns      : int 1/0 - if true, it's allowed.
 */
int
check_rw_access(string owner, int rw)
{
    string path;

    /* Everybody may read the sys-log reports. */
    if ((owner == ROOT_UID) && !rw)
    {
        return 1;
    }

    path = SECURITY->query_wiz_path(owner) + "/log/some_file";

    if (rw)
    {
        return SECURITY->valid_write(path, this_player());
    }
    return SECURITY->valid_read(path, this_player());
}

/*
 * Function name: remember_wizard
 * Description  : Remember the settings of a wizard for the next incarnation
 *                of the command.
 */
static void
remember_wizard()
{
    m_users[this_player()] = ({ gCurrent, gPrevious, gOwner, gType });
}

/*
 * Function name: restore_wizard
 * Description  : Restore the settings of a wizard for the next incarnation
 *                of the command.
 */
static void
restore_wizard()
{
    string dname;
    string wname = this_player()->query_real_name();

    if (pointerp(m_users[this_player()]))
    {
        gCurrent = m_users[this_player()][0];
        gPrevious = m_users[this_player()][1];
        gOwner = m_users[this_player()][2];
        gType = m_users[this_player()][3];
    }
    else
    {
        dname = SECURITY->query_wiz_dom(wname);
        if (strlen(dname) && (dname != WIZARD_DOMAIN))
        {
            gOwner = lower_case(dname);
        }
        else
        {
            gOwner = wname;
        }
        gType = REPORT_NAME_ALL;
        gCurrent = sizeof(m_reports[gOwner]);
        gPrevious = 0;
    }
}

/*
 * Function name: init_wizard
 * Description  : When the wizard first uses the report central, set up the
 *                default owners to follow (wizard and domain).
 */
static void
init_wizard()
{
    string wname = this_player()->query_real_name();
    string dname;

    if (!mappingp(m_report_wizards[wname]))
    {
        m_report_wizards[wname] = ([ wname : 1 ]);

        dname = SECURITY->query_wiz_dom(wname);
        if (strlen(dname) && (dname != WIZARD_DOMAIN))
        {
            write("Initializing report central. Subscribing to your domain: " + dname + ".\n");
            m_report_wizards[wname][lower_case(dname)] = 1;
        }
    }
}

/*
 * Function name: query_next_report_time
 * Description  : Determins the next time-stamp for a new report. It ensures
 *                there are no clashes if two reports are filed at exactly
 *                the same time.
 * Returns      : int - the current time (without clashes).
 */
static int
query_next_report_time()
{
    int rtime = time();

    while(file_size(REPORT_FILENAME(rtime)) > 0)
    {
        rtime++;
    }
    return rtime;
}

/*
 * Function name: notify_subscribers
 * Description  : Sends notification messages to the wizards subscribing
 *                to a log when a new report is written.
 * Returns      : void
 */
static void
notify_subscribers(string owner, int type)
{
    object *wizards = FILTER_IS_WIZARD(users());

    foreach (object wizard: wizards)
    {
        if (this_player() == wizard)
            continue;

        string name = wizard->query_real_name();
        if (m_report_wizards[name] && m_report_wizards[name][owner])
        {
            wizard->catch_tell("There is a new " + REPORT_NAMES_LONG[type] +
                " report in the " + capitalize(owner) + " log.\n");
        }
    }
}

/*
 * Function name: add_report
 * Description  : Add a new report to the database.
 * Arguments    : string author - the (capitalized) author of the report.
 *                string file - the location the report was filed in.
 *                int type - the report type
 *                int root - if true, indicate a global report.
 *                string text - the text of the report.
 */
public void
add_report(string author, string file, int type, int root, string text)
{
    string owner = lower_case(SECURITY->creator_file(file));
    int rtime = query_next_report_time();

    author = capitalize(author);
    if (!owner || root)
    {
        owner = ROOT_UID;
    }

    if (!pointerp(m_reports[owner]))
    {
        m_reports[owner] = ({ });
    }

    m_reports[owner] += ({ ({ author, type, 0, rtime, file }) });
    write_file(REPORT_FILENAME(rtime), "Author: " + author + " (" +
        REPORT_NAMES_LONG[type] + ")\nDate  : " + ctime(rtime) +
        "\nFile  : " + file + "\n\n" + text + "\n");

    /* Remember the newest report of the owner. */
    m_report_owners[owner] = rtime;
    save_reports();

    notify_subscribers(owner, type);
}

/*
 * Function name: remove_report
 * Description  : This physically removes a report from the system.
 * Arguments    : string owner - the owner (key) into the report mapping.
 *                int index - the index of the report in the array.
 */
static void
remove_report(string owner, int index)
{
    int rtime;

    /* Access failure. */
    if (!pointerp(m_reports[owner]) || (index >= sizeof(m_reports[owner])))
    {
        return;
    }

    rtime = m_reports[owner][index][REPORT_TIME];
    rm(REPORT_FILENAME(rtime));
    m_reports[owner] = exclude_array(m_reports[owner], index, index);
    save_reports();
    /* No need to update the last report time as new reports aren't created
     * in the past.
     */
}

/*
 * Function name: check_valid_msg_num
 * Description  : Check if the number is a valid message number, i.e. a
 *                number in the range of 1 to the number of messages.
 * Arguments    : num: the number to check
 * Returns      : int 1/0 - True if valid
 */
static int
check_valid_msg_num(int num)
{
    return ((num > 0) && (num <= sizeof(m_reports[gOwner])));
}

/*
 * Function name: invalid_gCurrent
 * Description  : Verify whether the gCurrent has a proper value. If invalid,
 *                resets gCurrent to gPrevious.
 * Arguments    : string owner - the owner. If omitted, it uses gOwner.
 * Returns      : int - if true, it was invalid.
 */
varargs int
invalid_gCurrent(string owner)
{
    if (!strlen(owner))
    {
        owner = gOwner;
    }

    if (gCurrent < 1 || gCurrent > sizeof(m_reports[owner]))
    {
        write("Invalid report number " + gCurrent + ".\n");
        gCurrent = gPrevious;
        return 1;
    }
    return 0;
}

/*
 * Function name: parse_range
 * Description  : Try to compile a list of message numbers from the string.
 *                Elements may be space or comma separated, and ranges are
 *                handled, too. The list of numbers is sorted and unique.
 *                All elements are checked for validity.
 * Arguments    : string str - the string that is going to be parsed
 *                             Typical accepted string: 10,1-4,2
 *                             Would return: ({ 1, 2, 3, 4, 10 });
 * Returns      : int - 0 if the string could not be parsed
 *                      otherwise an array of message numbers
 */
static int *
parse_range(string str)
{
    int    *range = ({ });
    int    *subrange;
    string *parts;
    int     left;
    int     right;
    int     index;

    /* Use gCurrent if no input is given. */
    if (!strlen(str))
    {
        if (check_valid_msg_num(gCurrent))
            return ({ gCurrent });
        else
            return 0;
    }

    /* Change all spaces to commas. */
    str = implode(explode(lower_case(str), " "), ",");
    /* Remove all empty spaces from the string. */
    parts = (explode(str, ",") - ({ "", 0 }) );

    foreach(string part: parts)
    {
        /* The parts can either consist of a single number of a range. */
        if (sscanf(part, "%d-%d", left, right) == 2)
        {
            /* Too large or too small numbers. */
            if ((!check_valid_msg_num(left)) ||
                (!check_valid_msg_num(right)))
                return 0;

            /* A descending range is a range too. */
            if (right < left)
            {
                index = left;
                left  = right;
                right = index;
            }

            /* Add all elements from the selected subrange to the range. */
            index = left - 1;
            while(++index <= right)
            {
                range |= ({ index });
            }
        }
        else
        {
            index = atoi(part);
            /* A string will be 0 and thus invalid. */
            if (!check_valid_msg_num(index))
            {
                return 0;
            }
            /* Add the element to the range; eliminate duplicates. */
            range |= ({ index });
        }
    }

    /* Sort the array before returning it. There really is not big deal in
     * this since the range is already unique.
     */
    return sort_array(range);
}

/*
 * Function name: loop
 * Description  : Called at the end of each command to loop the report reader
 *                functionality and pass control to the command handler.
 */
static void
loop()
{
    if (gCurrent < 1 || gCurrent > sizeof(m_reports[gOwner]))
    {
        gCurrent = sizeof(m_reports[gOwner]);
    }

    write("Log: " + capitalize(gOwner) + " [" + capitalize(gType) + "] [1-" +
        sizeof(m_reports[gOwner]) + "] cdDMeEfhlLqrnpsux.-+!? (current: " +
        gCurrent + ") -- ");

    /* Remember the variables for this wizard. */
    remember_wizard();
    input_to(get_cmd);
}

/*
 * Function name: comment
 * Description  : Add some comments to a report.
 * Arguments    : string str - the command line arguments.
 */
static void
comment(string str)
{
    string wname = this_player()->query_real_name();

    if (!strlen(str))
    {
        write("Comment has to specify the comment to add.\n" +
           "Syntax: c <comment>\n");
        loop();
	return;
    }

    if (NO_WRITE_ACCESS(gOwner))
    {
        write("No write access to add comments to reports of " +
            capitalize(gOwner) + ".\n");
        loop();
        return;
    }

    if (invalid_gCurrent(gOwner))
    {
        loop();
        return;
    }

    str = ctime(time()) + " " + capitalize(wname) + ": " + str;
    write_file(REPORT_FILENAME(m_reports[gOwner][gCurrent - 1][REPORT_TIME]),
        HANGING_INDENT(str, 2, 78));
    write("Comments added to report " + gCurrent + ".\n");

    loop();
}

/*
 * Function name: delete
 * Description  : Mark a report for deletion.
 * Arguments    : int *numbers - the message to mark for deletion.
 */
static void
delete(int *numbers)
{
    string wname = this_player()->query_real_name();

    if (WIZ_CHECK < WIZ_NORMAL)
    {
        write("Marking something for deletion may only be done by full wizards.\n");
        loop();
        return;
    }

    if (!pointerp(numbers) || !sizeof(numbers))
    {
        write("Mark which report for deletion?\nSyntax: D [<number/range>][,<...>]\n");
        loop();
        return;
    }

    if (NO_WRITE_ACCESS(gOwner))
    {
        write("No write access to delete reports of " + capitalize(gOwner) + ".\n");
        loop();
        return;
    }

    foreach(int index: numbers)
    {
        index--;
        if (m_reports[gOwner][index][REPORT_STATUS] & REPORT_DELETED)
        {
            m_reports[gOwner][index][REPORT_STATUS] &= ~REPORT_DELETED;
            write("Unmarked report " + (index + 1) + " for deletion.\n");
            write_file(REPORT_FILENAME(m_reports[gOwner][index][REPORT_TIME]),
                ctime(time()) + " Unmarked for deletion by " + capitalize(wname) + ".\n");
        }
        else
        {
            m_reports[gOwner][index][REPORT_STATUS] |= REPORT_DELETED;
            write("Marked report " + (index + 1) + " for deletion.\n");
            write_file(REPORT_FILENAME(m_reports[gOwner][index][REPORT_TIME]),
                ctime(time()) + " Marked for deletion " + capitalize(wname) + ".\n");
        }
    }
    save_reports();
    loop();
}

/*
 * Function name: done
 * Description  : Mark a report as being done or not done.
 * Arguments    : int *numbers - the numbers to mark as done.
 */
static void
done(int *numbers)
{
    string wname = this_player()->query_real_name();

    if (WIZ_CHECK < WIZ_NORMAL)
    {
        write("Marking something as done may only be done by full wizards.\n");
        loop();
        return;
    }

    if (NO_WRITE_ACCESS(gOwner))
    {
        write("No write access to mark reports of " + capitalize(gOwner) +
            " as done.\n");
        loop();
        return;
    }

    if (!pointerp(numbers) || !sizeof(numbers))
    {
        write("Mark which report as done?\nSyntax: d [<number/range>][,<...>]\n");
        loop();
        return;
    }

    foreach(int index: numbers)
    {
        index--;
        if (m_reports[gOwner][index][REPORT_STATUS] & REPORT_DONE)
        {
            m_reports[gOwner][index][REPORT_STATUS] &= ~REPORT_DONE;
            write("Marked report " + (index + 1) + " as not done.\n");
            write_file(REPORT_FILENAME(m_reports[gOwner][index][REPORT_TIME]),
                ctime(time()) + " Marked NOT done by " + capitalize(wname) + ".\n");
        }
        else
        {
            m_reports[gOwner][index][REPORT_STATUS] |= REPORT_DONE;
            write("Marked report " + (index + 1) + " as done.\n");
            write_file(REPORT_FILENAME(m_reports[gOwner][index][REPORT_TIME]),
                ctime(time()) + " Marked done by " + capitalize(wname) + ".\n");
        }
    }
    save_reports();
    loop();
}

/*
 * Function name: erase
 * Description  : Remove all messages marked for deletion within the current
 *                owner.
 */
static void
erase(int done)
{
    string cOwner = capitalize(gOwner);

    if (!sizeof(m_reports[gOwner]))
    {
        write("No messages available for owner " + cOwner + ".\n");
        loop();
        return;
    }

    int rank = WIZ_CHECK;
    string wname = this_player()->query_real_name();
    /*
     * Owners should be able to delete their own log entries.
     * Stewards of a domain should likewise be permitted.
     */
    if ((rank < WIZ_LORD) && (wname != gOwner))
    {
	string steward = SECURITY->query_domain_steward(gOwner);

	if (steward != wname)
	{
	    write("Deleting reports may only be done by Lieges++" +
		(SECURITY->query_domain_number(gOwner) >= 0 ?
		    " or the Steward of " + cOwner : "") + ".\n");
	    loop();
	    return;
	}
    }

    if (NO_WRITE_ACCESS(gOwner))
    {
        write("No write access to erase reports of " + cOwner + ".\n");
        loop();
        return;
    }

    int *deleted = ({ });
    int index = sizeof(m_reports[gOwner]);
    int flag = (done ? REPORT_DONE : REPORT_DELETED);

    while(--index >= 0)
    {
        if (m_reports[gOwner][index][REPORT_STATUS] & flag)
        {
            deleted = ({ index + 1 }) + deleted;
            if ((index + 1) < gCurrent)
            {
                gCurrent--;
            }
            remove_report(gOwner, index);
        }
    }

    if (sizeof(deleted))
    {
        write(HANGING_INDENT("Deleting: " +
            COMPOSITE_WORDS(STRING(deleted)) + ".", 4, 0));
    }
    else
    {
        write("No reports marked " + (done ? "as done" : "for deletion") +
	    " for owner " + cOwner + ".\n");
    }

    /* Ensure gCurrent is valid after erasing. */
    gCurrent = min(gCurrent, sizeof(m_reports[gOwner]));

    loop();
}

/*
 * Function name: forward
 * Description  : Re-assign a report to another owner or report type.
 * Arguments    : string str - the command line arguments.
 */
static void
forward(string str)
{
    string wname = this_player()->query_real_name();
    string *words;
    string newtype, newowner, filename, text;
    mixed  report;
    int    index;

    if (!strlen(str))
    {
        write("Forward has to specify new Owner, new type, or both.\n" +
            "Syntax: f [number] [<owner>] [<type>]\n");
        loop();
        return;
    }

    if (WIZ_CHECK < WIZ_NORMAL)
    {
        write("Forwarding a report may only be done by full wizards.\n");
        loop();
        return;
    }

    if (NO_WRITE_ACCESS(gOwner))
    {
        write("No write access to forward reports of " + capitalize(gOwner) + ".\n");
        loop();
        return;
    }

    words = explode(lower_case(str), " ");
    if ((index = atoi(words[0])) > 0)
    {
        if (index > sizeof(m_reports[gOwner]))
        {
           write("Number " + index + " does not refer to a valid report number.\n");
           loop();
           return;
        }
        gCurrent = index;
        words = words[1..];
        if (!sizeof(words))
        {
            write("Forward has to specify new Owner, new type, or both.\n" +
                "Syntax: f [number] [<owner>] [<type>]\n");
            loop();
            return;
        }
    }

    if (IN_ARRAY(words[-1], REPORT_NAMES_LONG))
    {
        newtype = words[-1];
        words -= ({ newtype });
    }
    else
    {
        newtype = gType;
    }

    if (sizeof(words))
    {
        newowner = words[0];
        if ((newowner != ROOT_UID) &&
            (SECURITY->query_wiz_level(newowner) < WIZ_NORMAL) &&
            (SECURITY->query_domain_number(newowner) == -1))
        {
            write("The report cannot be assigned to owner " +
                capitalize(newowner) + ". It must be a domain, wizard or Root.\n");
            loop();
            return;
        }
    }
    else
    {
        newowner = gOwner;
    }

    if ((newowner == gOwner) && (newtype == gType))
    {
        write("The report is already classified as type " + gType +
            " for owner " + capitalize(gOwner) + ".\n");
        loop();
        return;
    }

    gCurrent--;
    report = m_reports[gOwner][gCurrent];

    /* If it's moved to another owner, update the timestamp so that the new
     * owner recognises it as a new message. Move the file to be in the
     * location of the new timestamp. */
    if (newowner != gOwner)
    {
        filename = REPORT_FILENAME(report[REPORT_TIME]);
        report[REPORT_TIME] = query_next_report_time();
        rename(filename, REPORT_FILENAME(report[REPORT_TIME]));
    }

    m_reports[gOwner] = exclude_array(m_reports[gOwner], gCurrent, gCurrent);
    if (!pointerp(m_reports[newowner]))
    {
        m_reports[newowner] = ({ report });
        m_report_owners[newowner] = report[REPORT_TIME];
    }
    else
    {
        index = sizeof(m_reports[newowner]);
        while(--index > 0)
        {
            if (m_reports[newowner][index][REPORT_TIME] < report[REPORT_TIME])
            {
                index++;
                break;
            }
        }
        m_reports[newowner] = include_array(m_reports[newowner], ({ report }), index);
        m_report_owners[newowner] = max(m_report_owners[newowner], report[REPORT_TIME]);
    }
    save_reports();

    if (newowner != gOwner)
    {
        text = " Forwarded to " + capitalize(newowner) + " (as " + newtype + ")";
    }
    else
    {
        text = " Reclassified to " + newtype + " (for " + capitalize(newowner) + ")";
    }

    write_file(REPORT_FILENAME(report[REPORT_TIME]), ctime(time()) +
        text + " by " + capitalize(wname) + ".\n");
    write("Fowarded report to owner " + capitalize(newowner) + " as type " +
        newtype + ".\n");

    /* Header will call loop(). */
    header("");
}

/*
 * Function name: header
 * Description  : Display the headers of a certain owner / report type.
 * Arguments    : string str - the command line arguments.
 */
static void
header(string str)
{
    string wname = this_player()->query_real_name();
    int index, size;
    string pattern;
    mixed report;
    string flag;
    int *indices = ({ });

    if (NO_READ_ACCESS(gOwner))
    {
        write("No read access for reports of " + capitalize(gOwner) + ".\n");
        loop();
        return;
    }

    if (REPORT_NAMES_MAP[str])
    {
        gType = str;
        report = REPORT_NAMES_MAP[str];
        size = sizeof(m_reports[gOwner]);
        for (index = 0; index < size; index++)
        {
            if (report == m_reports[gOwner][index][REPORT_TYPE])
            {
                indices += ({ index });
            }
        }
    }
    else if (strlen(str))
    {
        pattern = "*" + lower_case(str) + "*";
        str = capitalize(str);
        size = sizeof(m_reports[gOwner]);
        for (index = 0; index < size; index++)
        {
            if ((str == m_reports[gOwner][index][REPORT_AUTHOR]) ||
                wildmatch(pattern, lower_case(m_reports[gOwner][index][REPORT_PATH])))
            {
                indices += ({ index });
            }
        }
    }
    else
    {
        /* Display previous SCROLL_SIZE reports. */
        for (index = max(0, gCurrent - SCROLL_SIZE); index < gCurrent; index++)
        {
            indices += ({ index });
        }
    }

    if (!sizeof(indices))
    {
        write("No " + (strlen(str) ? "" : gType + " ") +
            "reports to display for " +
            (strlen(str) ? lower_case(str) : capitalize(gOwner)) + ".\n");
        loop();
        return;
    }

    foreach(int index: indices)
    {
        report = m_reports[gOwner][index];

        flag = ((report[REPORT_TIME] == m_report_wizards[wname][gOwner]) ? "R" :
            REPORT_STATUS_SHORT[report[REPORT_STATUS]]);
        write(sprintf("%2d: %-11s %1s %-46s %1s %11s\n", index+1,
            report[REPORT_AUTHOR], REPORT_NAMES_SHORT[report[REPORT_TYPE]],
            report[REPORT_PATH][..45], flag,
            TIME2FORMAT(report[REPORT_TIME], "mmm dd yyyy")));
    }

    loop();
}

/*
 * Function name: iterate
 * Description  : Returns gCurrent modified by count while taking
 *                gType filters into account
 */
int
iterate(int count)
{
    int type = REPORT_NAMES_MAP[gType];
    if (!type)
        return gCurrent + count;

    int current = gCurrent - 1;
    int last = current;
    int remaining = abs(count);

    while (remaining != 0) {
        current += (count > 0 ? 1 : -1);

        if (current < 0 || current >= sizeof(m_reports[gOwner]))
            return last + 1;

        if (m_reports[gOwner][current][REPORT_TYPE] == type) {
            last = current;
            remaining--;
        }
    }

    return current + 1;
}

/*
 * Function name: list
 * Description  : Display statistics on reports.
 * Arguments    : string str - the command line arguments.
 *                int all - list all reports for those owners.
 */
static varargs void
list(string str, int all)
{
    string wname = this_player()->query_real_name();
    string *owners = sort_array(m_indices(m_report_wizards[wname]));
    mixed reports;
    int found;
    int rtime, otime;
    int bugs, ideas, typos;
    string stime;
    int noloop = (str == NO_LOOP);

    if (noloop)
    {
        str = "";
    }

    str = lower_case(str);
    if (strlen(str))
    {
        if ((sscanf(str, "%d", rtime) == 1) ||
            (sscanf(str, "%d %s", rtime, str) == 2))
        {
            rtime = time() - (rtime * 86400);
        }
        if (m_report_owners[str])
        {
            owners = ({ str });
        }
    }

    /* User wants to list counts history. */
    if (all)
    {
        rtime = 1;
    }

    foreach(string owner: owners)
    {
        if (all || (m_report_wizards[wname][owner] < m_report_owners[owner]))
        {
            found = 1;
            otime = (rtime ? rtime : m_report_wizards[wname][owner]);
            stime = ((otime > 1) ? TIME2FORMAT(otime, "   (since dd mmm yyyy)") : "");

            reports = filter(m_reports[owner],
                &operator(>)(, otime) @ &operator([])(, REPORT_TIME) );
            bugs  = sizeof(filter(reports,
                &operator(==)(, REPORT_BUG) @ &operator([])(, REPORT_TYPE) ));
            ideas  = sizeof(filter(reports,
                &operator(==)(, REPORT_IDEA) @ &operator([])(, REPORT_TYPE) ));
            typos  = sizeof(filter(reports,
                &operator(==)(, REPORT_TYPO) @ &operator([])(, REPORT_TYPE) ));

            write(sprintf("%-11s :  %3d bugs   %3d ideas   %3d typos%s\n",
                capitalize(owner), bugs, ideas, typos, stime));
        }
    }

    if (!found)
    {
        write("No new reports found.\n");
    }

    if (!noloop)
    {
        loop();
    }
}

/*
 * Function name: merge
 * Description  : Merge multiple entries together
 * Arguments    : int *numbers - the numbers to consolidate
 */
static void
merge(int *numbers)
{
    if (WIZ_CHECK < WIZ_NORMAL)
    {
        write("Merging reports may only be done by full wizards.\n");
        loop();
        return;
    }

    if (NO_WRITE_ACCESS(gOwner))
    {
        write("No write access to merge reports of " +
	    capitalize(gOwner) + ".\n");
        loop();
        return;
    }

    int size = (pointerp(numbers) ? sizeof(numbers) : 0);

    if (size <= 1)
    {
        write("Merge which reports?\nSyntax: M <number/range>[,<...>]\n");
        loop();
        return;
    }

    if (size < 2)
    {
	write("Merging requires at least two reports be specified.\n");
	loop();
	return;
    }

    int first = numbers[0];
    string wname = this_player()->query_real_name();
    string file = REPORT_FILENAME(m_reports[gOwner][first - 1][REPORT_TIME]);

    write_file(file, ctime(time()) + " " + " Reports (" + (size - 1) +
	") merged by " + capitalize(wname) + ".\n\n");

    foreach(int num: numbers[1..])
    {
	mixed report = m_reports[gOwner][num - 1];
	write("Merging report " + num + " into " + first + ".\n");
        write_file(file, read_file(REPORT_FILENAME(report[REPORT_TIME])));
    }
    /*
     * Merged reports must be deleted in reverse order.
     */
    while(size-- > 1)
    {
	int index = numbers[size];
	write("Removing report " + index  + ".\n");
	remove_report(gOwner, index - 1);
    }
    loop();
}

/*
 * Function name: open
 * Description  : Activate the reports of an owner / report type.
 * Arguments    : string str - the command line arguments.
 */
static void
open(string str)
{
    string wname = this_player()->query_real_name();
    string *words;
    string oldtype = gType;
    string oldowner = gOwner;
    int size;

    if (!strlen(str))
    {
        str = wname;
        words = ({ wname, REPORT_NAME_BUG });
    }
    else
    {
        words = explode(lower_case(str), " ");
    }

    if (sizeof(words) == 1)
    {
        /* If specifying a type, open your own report, or the type you ask
         * for on the owner you're reading. */
        if (IN_ARRAY(words[0], REPORT_NAMES_LONG))
        {
            gType = words[0];
            words = ({ strlen(gOwner) ? gOwner : wname });
        }
        else if (!gType)
        {
            gType = REPORT_NAME_BUG;
        }
    }
    else
    {
        gType = words[1];
        if (!IN_ARRAY(gType, REPORT_NAMES_LONG))
        {
            write("Not a valid report category: " + gType + ".\n");
            loop();
            return;
        }
    }

    gOwner = words[0];
    if (!(size = sizeof(m_reports[gOwner])))
    {
        write("No reports found for owner " + capitalize(gOwner) + ".\n");
        gType = oldtype;
        gOwner = oldowner;
        loop();
        return;
    }

    if (NO_READ_ACCESS(gOwner))
    {
        write("No read access for report of " + capitalize(gOwner) + ".\n");
        loop();
        return;
    }

    gCurrent = size;
    header("");
    /* loop() called from header(). */;
}

/*
 * Function name: quit
 * Description  : Stop using the report reader.
 * Arguments    : string str - the command line arguments.
 */
static void
quit()
{
    gCurrent = 0;
    gPrevious = 0;
    gOwner = 0;
    gType = 0;
    write("Exiting the report reader.\n");
}

/*
 * Function name: done_reading_more
 * Description  : This function is a security function that checks whether
 *                it was indeed the more function that called us. We do
 *                not want anyone else to call loop().
 */
public void
done_reading_more()
{
    if (previous_object() == this_player())
    {
        /* Re-initialize the variables for this wizard before calling loop. */
        restore_wizard();
        loop();
    }
}

/*
 * Function name: read
 * Description  : Display a single report. Operates on gCurrent.
 * Arguments    : int more - use more.
 */
static void
read(int more)
{
    string wname = this_player()->query_real_name();
    string text;
    mixed report;

    if (invalid_gCurrent(gOwner))
    {
        loop();
        return;
    }

    if (NO_READ_ACCESS(gOwner))
    {
        write("No access to read reports of " + capitalize(gOwner) + ".\n");
        loop();
        return;
    }

    report = m_reports[gOwner][gCurrent - 1];
    text = sprintf("%-3d %s [%s] %s\n",
        gCurrent,
        capitalize(gOwner), capitalize(REPORT_NAMES_LONG[report[REPORT_TYPE]]),
        (report[REPORT_STATUS] ?
            "Status: " + REPORT_STATUS_LONG[report[REPORT_STATUS]] : ""));
    text += read_file(REPORT_FILENAME(report[REPORT_TIME]));

    /* Update last read report for this owner. */
    m_report_wizards[wname][gOwner] = max(m_report_wizards[wname][gOwner], report[REPORT_TIME]);
    save_wizards();

    if (strlen(text) > AUTO_MORE_LEN)
    {
        write("Too long report. More automatically invoked.\n");
        more = 1;
    }
    /* Add extra newline if there isn't one in the text. */
    text += ((text[-2..] == "\n\n") ? "" : "\n");
    if (more)
    {
        /* Remember the variables for this wizard before leaving the loop. */
        remember_wizard();
        this_player()->more(text, 0, done_reading_more);
    }
    else
    {
        write(text);
        loop();
    }
}

/*
 * Function name: subscribe
 * Description  : Subscribe to the reports of an owner.
 * Arguments    : string str - the command line arguments.
 */
static void
subscribe(string str)
{
    string wname = this_player()->query_real_name();

    str = lower_case(str);
    if (!strlen(str))
    {
        write("Subscribing to: " +
            COMPOSITE_WORDS(map(m_indices(m_report_wizards[wname]), capitalize)) +
            ".\nSyntax: s <wizard/domain/root>\n");
        loop();
        return;
    }
    if (m_report_wizards[wname][str])
    {
        write("Already subscribing to reports of " + capitalize(str) + ".\n");
        loop();
        return;
    }

    if ((SECURITY->query_wiz_rank(str) < WIZ_NORMAL) &&
        (SECURITY->query_domain_number(str) == -1) &&
        (str != ROOT_UID))
    {
        write("Cannot subscribe to " + capitalize(str) +
            ". Must be a domain, wizard or Root.\n");
        loop();
        return;
    }

    if (NO_READ_ACCESS(str))
    {
        write("No read access to subscribe to " + capitalize(str) + ".\n");
        loop();
        return;
    }

    m_report_wizards[wname] += ([ str : 1 ]);
    save_wizards();
    write("Subscribed to reports of " + capitalize(str) + ".\n");
    list();
}

/*
 * Function name: unread
 * Description  : Mark a log as unread.
 * Arguments    : string str - the command line arguments.
 */
static void
unread(string str)
{
    string wname = this_player()->query_real_name();

    if (!strlen(str))
    {
        str = gOwner;
    }
    str = lower_case(str);

    if (!m_report_wizards[wname][str])
    {
        write("You are not subscribed to the reports of " + capitalize(str) + ".\n");
        loop();
        return;
    }

    m_report_wizards[wname][str] = 1;
    save_wizards();
    write("Marked the reports from " + capitalize(str) + " as unread.\n");
    loop();
}

/*
 * Function name: unsubscribe
 * Description  : Unsubscribe from the reports of an owner.
 * Arguments    : string str - the command line arguments.
 */
static void
unsubscribe(string str)
{
    string wname = this_player()->query_real_name();

    str = lower_case(str);
    if (!strlen(str))
    {
        write("Unsubscribe from which reports? Syntax: u <wizard/domain>\n");
        loop();
        return;
    }
    if ((str == wname) || (str == SECURITY->query_wiz_dom(wname)))
    {
        write("Cannot unsubscribe from your own (domain's) reports.\n");
        loop();
        return;
    }
    if (!m_report_wizards[wname][str])
    {
        write("Not subscribing to reports from " + capitalize(str) + ".\n");
        loop();
        return;
    }
    m_delkey(m_report_wizards[wname], str);
    save_wizards();
    write("Unsubscribed from reports of " + capitalize(str) + ".\n");
    loop();
}

/*
 * Function name: get_cmd
 * Description  : Interal command handler for the report reader.
 * Arguments    : string str - the command line arguments.
 */
static void
get_cmd(string str)
{
    string cmd;
    int number, more;

    /* Re-initialize the variables for this wizard. */
    restore_wizard();

    /* Default command: next report. */
    if (!strlen(str))
        str = "n";

    /* Typing an owner name, or bug/typo/idea opens at log. */
    if ((str == ROOT_UID) ||
        (SECURITY->query_wiz_level(str) >= WIZ_NORMAL) ||
        (SECURITY->query_domain_number(str) > -1) ||
        IN_ARRAY(str, REPORT_NAMES_LONG))
    {
        str = "o " + str;
    }

    /* If gCurrent is changed, we like to remember the previous active
     * message. Also, we check the more-option up front.
     */
    gPrevious = gCurrent;
    more = (extract(str, 0, 0) == "m");

    /* Check for commands we cannot trigger in the switch. */
    if ((sscanf(str, "%d", number) == 1) ||
        (sscanf(str, "m%d", number) == 1))
    {
        /* This is a flag-command to indicate that the player entered a
         * number of a message. If people give the command _read manually,
         * it will probably result in an illegal gCurrent == 0.
         */
        cmd = "_read";
    }
    else
    {
        cmd  = explode(str, " ")[0];
        str  = str[(strlen(cmd) + 1)..];
    }

    switch(cmd)
    {
    case "?":
        write(" <owner> [type]   - open a log of a different owner\n" +
              " <type>           - open the log for <type> within the same owner\n" +
              " c <comment>      - add a comment to the current report\n" +
              " d [number]       - mark a report as done or not done\n" +
              " D [number]       - mark a report for deletion or not (purging is done later)\n" +
              " e                - erase all reports of current owner marked for deletion\n" +
              " E                - erase all reports of current owner marked as done\n" +
              " M <numbers>      - merge multiple reports into a consolidated entry\n" +
              " f [num] [owner] [type] - forward to a new owner, change the type, or both\n" +
              " h [type/auth/pattern] - display headers; filter on type, author or pattern in the path\n" +
              " + or -           - display the next or previous " + SCROLL_SIZE + " headers\n" +
              " l [days] [owner] - list headers [up to x days old] [for an owner]\n" +
              " L [owner]        - list summary of all headers [for an owner]\n" +
              " q or x           - quit the report handler (does not auto-erase)\n" +
              " r or .           - read the current report (mr or m. for more)\n" +
              " [m]<number>      - read the report <number> (use m for more)\n" +
              " n or p           - read the next or previous report (mn or np for more)\n" +
              " s [owner]        - subscribe to reports of a domain/wizard/root, or list subs.\n" +
              " u [owner]        - mark the report [log of an owner] as unread\n" +
              " us <owner>       - unsubscribe from reports on a domain/wizard/root\n");
        loop();
        return;

    /* Read the next report [with more]. */
    case "n":
    case "mn":
        gCurrent = iterate(1);
        read(more);
        return;

    /* Read the current report [with more]. */
    case ".":
    case "m.":
    case "r":
    case "mr":
        read(more);
        return;

    /* Read a numbered report [with more]. */
    case "_read":
        gCurrent = number;
        read(more);
        return;

    /* Read the previous report [with more]. */
    case "p":
    case "mp":
        gCurrent = iterate(-1);
        read(more);
        return;

    /* Display the previoux X headers */
    case "-":
        gCurrent = max(SCROLL_SIZE, gCurrent - SCROLL_SIZE);
        gCurrent = min(sizeof(m_reports[gOwner]), gCurrent);
        header("");
        return;

    /* Display the previoux X headers */
    case "+":
        gCurrent = min(sizeof(m_reports[gOwner]), gCurrent + SCROLL_SIZE);
        header("");
        return;

    /* Add a comment to a report. */
    case "c":
        comment(str);
        return;

    /* Mark a report as done or not done. */
    case "d":
    case "ud":
        done(parse_range(str));
        return;

    /* Mark a report for deletion or not. */
    case "D":
    case "uD":
        delete(parse_range(str));
        return;

    case "e":
    case "E":
        erase(cmd == "E");
        return;

    /* Merge and consolidate reports. */
    case "M":
        merge(parse_range(str));
        return;

    /* Forward report to another owner or another type. */
    case "f":
        forward(str);
        return;

    /* List headers [for a certain path or specific owner]. */
    case "h":
        header(str);
        return;

    /* List [all] reports [of a certain age] [for a specific owner]. */
    case "l":
    case "L":
        list(str, (cmd == "L"));
        return;

    /* Open a report dossier [for a certain owner] [of a certain type] */
    case "o":
        open(str);
        return;

    /* Quit the report reader. */
    case "q":
    case "x":
        quit();
        return;

    /* Subscribe to a certain domain / wizard's reports. */
    case "s":
        subscribe(str);
        return;

    /* Mark a report [dossier of an owner] as unread. */
    case "u":
        unread(str);
        return;

    /* Unsubscribe from a certain domain / wizard's reports. */
    case "us":
        unsubscribe(str);
        return;

    default:
        write("Unrecognized command. Do \"?\" for help, \"q\" to quit.\n");
        loop();
    }
}

/*
 * Function name: report_handler
 * Description  : This routine handled the incoming commands from the
 *                apprentice soul.
 * Arguments    : string str - the command line argument.
 * Returns      : int 1/0 - success/failure.
 */
int
report_handler(string str)
{
    /* Access failure. May only be called from apprentice soul. */
    if (!CALL_BY(WIZ_CMD_APPRENTICE))
    {
        return 0;
    }

    init_wizard();
    if (!strlen(str))
    {
        restore_wizard();
        /* Don't enter into the report handler. */
        list(NO_LOOP);
    }
    else
    {
        get_cmd(str);
    }
    return 1;
}
