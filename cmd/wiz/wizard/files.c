/*
 * /cmd/wiz/normal/files.c
 *
 * This is a subpart of /cmd/wiz/normal.c
 *
 * The commands in this sub-part all have to do with the manipulation of
 * files and objects.
 *
 * Commands in this file:
 * - aft
 * - clone
 * - cp
 * - destruct
 * - distrust
 * - ed
 * - load
 * - merge
 * - mkdir
 * - mv
 * - remake [now an alias for update -r]
 * - rm
 * - rmdir
 * - trust
 * - update
 */

#include <cmdparse.h>
#include <files.h>
#include <filter_funs.h>
#include <language.h>
#include <options.h>

/*
 * Global variable.
 */
static private object  loadmany_wizard;
static private string *loadmany_files;
static private string *loadmany_going = ({ });

static private mapping aft_tracked;
static private mapping aft_current = ([ ]);
static private string *aft_sorted = ({ });

/*
 * Prototypes.
 */
nomask int update_ob(string str, int recurse);
nomask int update(string str);

/*
 * Maxinum number of files that can be copied/moved/removed with one command
 * Protects from errors like 'cp /std/* .'
 */
#define MAXFILES 50

/*
 * Function name: copy
 * Description  : Copy a (text) file. Limited to about 50kB files by the GD.
 *                It could be circumvented but very few files are larger
 *                than that. Maybe I should make this into an simul-efun?
 * Arguments    : string path1 - source path.
 *                string path2 - destination path (including filename).
 * Returns      : int 1/0 - success/failure.
 */
private nomask int
copy(string path1, string path2)
{
    string buf;

    /* Read the source file and test for success. */
    buf = read_file(path1);
    if (!strlen(buf))
    {
        return 0;
    }

    switch(file_size(path2))
    {
        /* Target is a directory. Impossible. */
        case -2:
            return 0;

        /* Target file does not exist or is empty. Proceed. */
        case -1:
        case  0:
            break;

        /* Existing target file. Try to remove it first. */
        default:
            if (!rm(path2))
            {
                return 0;
            }
    }

    /* Write the buffer and return the return value of the efun. */
    return write_file(path2, buf);
}

#define MULTI_CP 0
#define MULTI_MV 1
#define MULTI_RM 2
#define MULTI_OPERATION ({ "Copied", "Moved", "Removed" })

/*
 * Function name: multi_file
 * Description  :
 * Arguments    : string str - the command line argument.
 *                int operation - the operation type.
 * Returns      : int 1/0 - success/failure.
 */
private nomask int
multi_file(string str, int operation)
{
    string *files, *parts, *source, *cont, target;
    int    index, size, done;

    CHECK_SO_WIZ;

    if (!stringp(str))
    {
        notify_fail("No argument given to " + query_verb() + ".\n");
        return 0;
    }

    /* Explode the argument and solve the tilde-path notation. */
    files = explode(str, " ") - ({ "" });
    str = this_player()->query_path();
    index = -1;
    size = sizeof(files);
    while(++index < size)
    {
        files[index] = FTPATH(str, files[index]);
    }

    /* For cp and mv we need at least two arguments, mark the target. */
    if (operation != MULTI_RM)
    {
        if (size == 1)
        {
            notify_fail("Syntax: " + query_verb() + " <source> <target>\n");
            return 0;
        }

        target = files[--size];
        files -= ({ target });
    }

    /* Expand the wildcards. */
    index = -1;
    source = ({ });
    while(++index < size)
    {
        parts = explode(files[index], "/");
        str = implode(parts[..(sizeof(parts) - 2)], "/") + "/";

        if (pointerp((cont = get_dir(files[index]))))
            source += map(cont - ({ ".", ".." }), &operator(+)(str));
        else
            return notify_fail(query_verb() + ": " + files[index] +
                ": No such file or directory.\n");
    }

    /* If too many files are selected, we still process the first batch. */
    if ((size = sizeof(source)) == 0)
        return notify_fail(query_verb() + ": " + files[0] +
            ": No such file or directory.\n");

    if (size > MAXFILES)
    {
        write("Selected " + size + " files. Only the first " + MAXFILES +
            " will be processed.\n");
        source = source[..(MAXFILES - 1)];
        size = sizeof(source);
    }

    if (size > 1)
    {
        if ((operation != MULTI_RM) &&
            (file_size(target) != -2))
        {
            notify_fail("When selecting multiple files, the destination " +
                "must be a directory.\n");
            return 0;
        }

        write("Selected " + size + " files.\n");
    }

    index = -1;
    while(++index < size)
    {
        switch(operation)
        {
        case MULTI_CP:
            if (file_size(source[index]) == -2)
            {
                write("Directory " + source[index] + " cannot be copied.\n");
                break;
            }

            str = target;
            if (file_size(target) == -2)
            {
                parts = explode(source[index], "/");
                str += "/" + parts[sizeof(parts) - 1];
            }

            if (copy(source[index], str))
            {
                done++;
            }
            else
            {
                write("Failed at " + source[index] + "\n");
            }
            break;

        case MULTI_MV:
            if ((file_size(source[index]) == -2) &&
                (size > 1))
            {
                write("When moving directory " + source[index] +
                    " the directory must be the only argument.\n");
                break;
            }

            str = target;
            if (file_size(target) == -2)
            {
                parts = explode(source[index], "/");
                str += "/" + parts[sizeof(parts) - 1];
            }

            if (rename(source[index], str))
            {
                SECURITY->remove_binary(str);
                done++;
            }
            else
            {
                write("Failed at " + source[index] + "\n");
            }
            break;

        case MULTI_RM:
            if (file_size(source[index]) == -2)
            {
                write("Directory " + source[index] +
                    " cannot be removed using rm.\n");
                break;
            }

            if (rm(source[index]))
            {
                done++;
            }
            else
            {
                write("Failed at " + source[index] + "\n");
            }
            break;
        }
    }

    write(MULTI_OPERATION[operation] + " " + done + " file" +
        ((done != 1) ? "s" : "") + ".\n");

    return 1;
}

/* **************************************************************************
 * aft - [Avernir's] file tracker
 */

/*
 * AFT uses disk-caching and the files are stored in the wizards personal
 * directory. The data type of the save-file is as follows.
 *
 * aft_tracked = ([ (string) path : ({
 *                       (string) private name,
 *                       (int) last modification time,
 *                       (int) last size,
 *                  }), ])
 *
 * aft_sorted  = ({ sorted list of paths })
 *
 * The current file (last tracked) for each wizard is stored as follows.
 * This structure is not saved.
 *
 * aft_current = ([ (string) name : (int) index of last tracked file ])
 */

#define AFT_PRIVATE_NAME (0)
#define AFT_FILE_TIME    (1)
#define AFT_FILE_SIZE    (2)
#define AFT_TAIL_DIFF    (20000)
#define AFT_FILE         ("/.aft")

/*
 * Function name: read_aft_file
 * Description  : This function reads the aft-file of a particular wizard.
 *                If such a file does not exist, the mapping is cleared.
 * Arguments    : string name - the wizard name.
 */
private void
read_aft_file(string name)
{
    aft_tracked = read_cache(SECURITY->query_wiz_path(name) + AFT_FILE);

    /* No such file, set to defaults. */
    if (!m_sizeof(aft_tracked))
    {
        aft_tracked = ([ ]);
        aft_sorted = ({ });
    }
    else
    /* Sort the names in the mapping. */
    {
        aft_sorted = sort_array(m_indices(aft_tracked));
    }
}

/*
 * Function name: save_aft_file
 * Description  : This function saves the aft-file of a particular wizard.
 *                When empty, the save-file is deleted.
 * Arguments    : string name - the wizard name.
 */
private void
save_aft_file(string name)
{
    /* No file being tracked, remove the file completely. */
    if (!m_sizeof(aft_tracked))
    {
        /* We do not need to append the .o as that is done in rm_cache(). */
        rm_cache(SECURITY->query_wiz_path(name) + AFT_FILE);
        return;
    }

    save_cache(aft_tracked, (SECURITY->query_wiz_path(name) + AFT_FILE));
}

/*
 * Function name: aft_find_file
 * Description  : This function will return the full path of the file the
 *                wizard wants to handle. It accepts the path (including
 *                tilde, the private name of the file and the number in the
 *                list of files.
 * Arguments    : string file - the file to find.
 * Returns      : string - path of the file found.
 */
private string
aft_find_file(string file)
{
    int index;
    mapping tmp;

    /* May be the index number in the list of files. */
    if (index = atoi(file))
    {
        if ((index > 0) &&
            (index <= sizeof(aft_sorted)))
        {
            return aft_sorted[index - 1];
        }

        /* No such index. Maybe it is a name, who knows ;-) */
    }

    /* May be a private name the wizard assigned to the file. This filter
     * statement will return a mapping with only one element if the private
     * name was indeed found.
     */
    tmp = filter(aft_tracked,
        &operator(==)(file, ) @ &operator([])(, AFT_PRIVATE_NAME));
    if (m_sizeof(tmp))
    {
        return m_indices(tmp)[0];
    }

    /* Could be the path itself. */
    file = FTPATH(this_interactive()->query_path(), file);
    if (pointerp(aft_tracked[file]))
    {
        return file;
    }

    /* File not found. */
    return 0;
}

/*
 * Function name: aft_catchup_file
 * Description  : Map-function called to update the time a (log) file was
 *                last accessed. For convenience we use time() here and not
 *                the actual file time. This is good enough anyway, because
 *                if the file is going to be changed, it is going to be
 *                changed after the current time().
 * Arguments    : string path - the path to the file.
 */
private void
aft_catchup_file(string path)
{
    aft_tracked[path][AFT_FILE_TIME] = time();
    aft_tracked[path][AFT_FILE_SIZE] = file_size(path);
}

public int
aft(string str)
{
    string *args;
    int    size, len, ftime;
    int    index = -1;
    int    flag = 0;
    int    changed;
    string name = this_interactive()->query_real_name();
    string fname;
    string *files;
    mapping tmp;

    CHECK_SO_WIZ;

    /* Set to default when there is no argument. */
    if (!strlen(str))
    {
        str = "lu";
    }

    args = explode(str, " ");
    size = sizeof(args);

    /* Read the wizards aft-file. */
    read_aft_file(name);

    /* Wizard is not tracking any files and does not want to select a file
     * to track either.
     */
    if (!m_sizeof(aft_tracked) &&
        args[0] != "s")
    {
        write("You are not tracking any files.\n");
        return 1;
    }

    switch(args[0])
    {
    case "c":
        if (size != 2)
        {
            notify_fail("Syntax: aft c <file>\n");
            return 0;
        }

        str = aft_find_file(args[1]);
        if (!stringp(str))
        {
            notify_fail("You are not tracking a file \"" + args[1] + "\".\n");
            return 0;
        }

        /* Mark the file as being up to date and make it current. */
        aft_catchup_file(str);
        aft_current[name] = member_array(str, aft_sorted);
        save_aft_file(name);

        write("Caught up on " + str + ".\n");
        return 1;

    case "C":
        if (size != 1)
        {
            notify_fail("Syntax: aft C\n");
            return 0;
        }

        /* Mark all files as being up to date. */
        map(m_indices(aft_tracked), aft_catchup_file);
        save_aft_file(name);

        write("Caught up on all files.\n");
        return 1;

    case "l":
        flag = 1;
        /* Continue at "lu". */

    case "lu":
        if (size != 1)
        {
            notify_fail("Syntax: aft " + args[0] + "\n");
            return 0;
        }

        /* Loop over all files being tracked. */
        size = sizeof(aft_sorted);
        while(++index < size)
        {
            ftime = file_time(aft_sorted[index]);
            changed = (ftime > aft_tracked[aft_sorted[index]][AFT_FILE_TIME]);
            /* Only print if the file actually changed, or if the wizard
             * signalled that he wanted all files.
             */
            if (flag || changed)
            {
                fname = aft_sorted[index];
                len = strlen(fname);
                write(sprintf("%2d %-8s%-1s%-1s %-50s %s\n",
                    (index + 1),
                    aft_tracked[aft_sorted[index]][AFT_PRIVATE_NAME],
                    (changed ? "*" : " "),
                    ((index == aft_current[name]) ? ">" : ":"),
                    ((len > 50) ? (fname[..23] + "..." + fname[-23..]) : fname),
                    TIME2FORMAT(ftime, "-d mmm yyyy")));

                args[0] = "oke";
            }
        }

        /* No output of any files. Give him a "fail" message. */
        if (args[0] != "oke")
        {
            write("No changes in any of the tracked files.\n");
        }

        return 1;

    case "n":
    case "r":
    case "rr":
        switch(size)
        {
        case 1:
            /* user wants to see next changed file. Loop over all files,
             * starting at the current file.
             */
            index = aft_current[name] - 1;
            size = sizeof(aft_sorted);
            while(++index < (size + aft_current[name]))
            {
                /* If there is a change, break. */
                if (file_time(aft_sorted[index % size]) >
                    aft_tracked[aft_sorted[index % size]][AFT_FILE_TIME])
                {
                    index %= size;
                    args += ({ aft_sorted[index] });
                    break;
                }
            }

            /* No change to any files. Give him a "fail" message. */
            if (sizeof(args) == 1)
            {
                write("No changes in any of the tracked files.\n");
                return 1;
            }

            /* Found file added to the list of arguments. */
            break;

        case 2:
            /* user specified a file to read. */
            str = aft_find_file(args[1]);
            if (!stringp(str))
            {
                notify_fail("You are not tracking a file \"" + args[1] + "\".\n");
                return 0;
            }

            args[1] = str;
            break;

        default:
            notify_fail("Syntax: aft " + args[0] + " [<file>]\n");
            return 0;
        }

        /* Access failure. */
        if (!(SECURITY->valid_read(args[1], geteuid(), "tail")))
        {
            notify_fail("You have no read access to: " + args[1] + "\n");
            return 0;
        }

        /* Mark as read and file to current. Then save. */
        aft_current[name] = member_array(args[1], aft_sorted);
        size = aft_tracked[args[1]][AFT_FILE_SIZE];
        ftime = aft_tracked[args[1]][AFT_FILE_TIME];
        aft_catchup_file(args[1]);
        save_aft_file(name);

        write("AFT on " + args[1] + "\n");
        if (args[0] == "n")
        {
            /* If there is a newer '.old' then the file cycled. */
            if (file_time(args[1] + ".old") >= ftime)
            {
                size = 0;
            }
            if (file_size(args[1]) <= size)
            {
                write("No new entries. File may have been edited.\n");
                return 1;
            }

            /* Read at most the last AFT_TAIL_DIFF bytes. */
            size = max(size, (file_size(args[1]) - AFT_TAIL_DIFF));

            this_player()->tail(read_bytes(args[1], size, AFT_TAIL_DIFF));
            return 1;
        }

        /* Force the wizard to use the tail command. We can force since we
         * have his/her euid.
         */
        return this_interactive()->command("tail -r " + args[1]);
        /* not reached */

    case "s":
        switch(size)
        {
        case 2:
            /* User does not want a private name. */
            args += ({ "" });
            break;

        case 3:
            /* Specified a private name. See whether it is not a duplicate. */
            tmp = filter(aft_tracked,
                &operator(==)(args[2], ) @ &operator([])(, AFT_PRIVATE_NAME));
            if (m_sizeof(tmp))
            {
                notify_fail("Name \"" + args[2] + "\" already used for " +
                    m_indices(tmp)[0] + ".\n");
                return 0;
            }

            break;

        default:
            notify_fail("Syntax: aft s <path> [<name>]\n");
            return 0;
        }

        args[1] = FTPATH(this_interactive()->query_path(), args[1]);
        if (aft_tracked[args[1]])
        {
            notify_fail("You are already tracking " + args[1] + ".\n");
            return 0;
        }

        if (file_size(args[1]) < 0)
        {
            notify_fail("There is no file " + args[1] + ".\n");
            return 0;
        }

        /* Add the file, and mark as unread. Then save. */
        aft_tracked[args[1]] = ({ args[2], 0, 0 });
        aft_sorted = sort_array(m_indices(aft_tracked));
        save_aft_file(name);

        write("Started tracking on " + args[1] + ".\n");
        return 1;

    case "u":
        if (size != 2)
        {
            notify_fail("Syntax: aft u <file>\n");
            return 0;
        }

        str = aft_find_file(args[1]);
        if (!stringp(str))
        {
            notify_fail("You are not tracking a file \"" + args[1] + "\".\n");
            return 0;
        }

        if (member_array(str, aft_sorted) >= aft_current[name])
        {
            aft_current[name] -= 1;
        }

        m_delkey(aft_tracked, str);
        aft_sorted -= ({ str });
        save_aft_file(name);

        write("Unselected file " + str + ".\n");
        return 1;

    case "U":
        aft_tracked = ([ ]);
        m_delkey(aft_current, name);
        aft_sorted = ({ });
        save_aft_file(name);

        write("Unselected all files. Stopped all tracking.\n");
        return 1;

    default:
        notify_fail("No subcommand \"" + args[0] + "\" to aft.\n");
        return 0;
    }

    write("Impossible end of aft switch. Please report to an archwizard!\n");
    return 1;
}

/* **************************************************************************
 * clone - clone an object
 */

/*
 * Function name: clone_message
 * Description  : This function returns the proper message to be displayed
 *                to people watching the cloning. Wizards get a message,
 *                mortal players see 'something'.
 * Arguments    : mixed cloned - the file_name with object number of the
 *                               object cloned.
 * Returns      : string - the description.
 */
public nomask string
clone_message(mixed cloned)
{
    object proj = previous_object(-1);

    if (!(proj->query_wiz_level()))
        return "something";

    if ((!stringp(cloned)) || (!strlen(cloned)))
        return "something";
    if (!objectp(cloned = find_object(cloned)))
        return "something";

    if (living(cloned))
        return (string)cloned->query_art_name(cloned);

    return (strlen(cloned->short(cloned)) ?
        LANG_ASHORT(cloned) : file_name(cloned));
}

/*
 * Function name: clone_ob
 * Description  : This function actually clones the object a wizard wants
 *                to clone.
 * Arguments    : string what - the filename of the object to clone.
 * Returns      : object - the object cloned.
 */
static nomask object
clone_ob(string what)
{
    string file, argument;

    if (sscanf(what, "%s:%s", file, argument) != 2)
    {
        file = what;
        argument = 0;
    }

    file = FTPATH(this_interactive()->query_path(), file);

    if (!strlen(file))
    {
        notify_fail("Invalid file.\n");
        return 0;
    }

    if (file_size(file + ".c") < 0 && file_size(file) < 0)
    {
        notify_fail("No such file: " + file + "\n");
        return 0;
    }

    object ob = clone_object(file);
    if (!ob)
    {
        notify_fail("You can not clone: " + file + "\n");
        return 0;
    }

    function init = &->init_recover();
    if (function_exists("query_auto_load", ob)) {
        init = &->init_arg();
    }

    if (argument) {
        mixed ret = init(ob, argument);

        if (ret) {
            ob->remove_object();

            if (objectp(ret)) {
                ob = ret;
            } else {
                notify_fail("Object removed by init_recover\n");
                return 0;
            }
        }
    }

    object *watchers = FILTER_IS_SEEN(this_player(), FILTER_OTHER_LIVE(
                        all_inventory(environment(this_player()))));
    watchers->catch_msg(QCTNAME(this_interactive()) +
         " fetches @@clone_message:" + file_name(this_object()) + "|" +
         file_name(ob) + "@@ from another dimension.\n");
    return ob;
}

nomask int
clone(string str)
{
    object ob;
    int num, argc;
    string *argv;
    string desc;

    CHECK_SO_WIZ;

    if (!stringp(str))
    {
                notify_fail("Clone what object ?\n");
                return 0;
    }

    argv = explode(str, " ");
    argc = sizeof(argc);

    switch (argv[0])
    {
    case "-i":
        ob = clone_ob(implode(argv[1..], " "));
        if (!ob)
            return 0;
        ob->move(this_interactive(), 1);
        if (this_interactive()->query_option(OPT_ECHO))
        {
            desc = (living(ob) ? ob->query_art_name(this_interactive()) : LANG_ASHORT(ob));
            write("You clone " + desc + " into your inventory.\n");
        }
        else
        {
            write("Ok.\n");
        }
        break;

    case "-e":
        ob = clone_ob(implode(argv[1..], " "));
        if (!ob)
            return 0;
        ob->move(environment(this_interactive()), 1);
        /* We need to do this instead of write() because cloning a living
         * will alter this_player().
         */
        if (this_interactive()->query_option(OPT_ECHO))
        {
            desc = (living(ob) ? ob->query_art_name(this_interactive()) : LANG_ASHORT(ob));
            this_interactive()->catch_tell("You clone " + desc +
                " into your environment.\n");
        }
        else
        {
            this_interactive()->catch_tell("Ok.\n");
        }
        break;

    default:
        ob = clone_ob(implode(argv[0..], " "));
        if (!ob)
            return 0;

        num = (int)ob->move(this_interactive());
        switch (num)
        {
        case 0:
            if (this_interactive()->query_option(OPT_ECHO))
            {
                desc = (living(ob) ? ob->query_art_name(this_interactive()) : LANG_ASHORT(ob));
                write("You clone " + desc + " into your inventory.\n");
            }
            else
            {
                write("Ok.\n");
            }
            break;

        case 1:
            write("Too heavy for destination.\n");
            break;

        case 2:
            write("Can't be dropped.\n");
            break;

        case 3:
            write("Can't take it out of it's container.\n");
            break;

        case 4:
            write("The object can't be inserted into bags etc.\n");
            break;

        case 5:
            write("The destination doesn't allow insertions of objects.\n");
            break;

        case 6:
            write("The object can't be picked up.\n");
            break;

        case 7:
            write("Other (Error message printed inside move() function).\n");
            break;

        case 8:
            write("Too big volume for destination.\n");
            break;

        default:
            write("Strange, very strange error in move: " + num + "\n");
            break;
        }
        if (num)
        {
            num = (int)ob->move(environment(this_interactive()));
            if (this_interactive()->query_option(OPT_ECHO))
            {
                desc = (living(ob) ? ob->query_art_name(this_interactive()) : LANG_ASHORT(ob));
                write("You clone " + desc + " into your environment.\n");
            }
            else
            {
                write("Ok.\n");
            }
        }
        break;
    }
    return 1;
}

/* **************************************************************************
 * cp - copy multiple files
 */
nomask int
cp_cmd(string str)
{
    return multi_file(str, MULTI_CP);
}

/* **************************************************************************
 * destruct - destruct an object
 */
nomask int
destruct_ob(string str)
{
    object *oblist;
    int    dflag;

    CHECK_SO_WIZ;

    if (!stringp(str))
    {
        notify_fail("Destruct what?\n");
        return 0;
    }

    if (sscanf(str, "-D %s", str) == 1)
    {
        dflag = 1;
    }

    if (!parse_command(str, environment(this_interactive()), "[the] %i",
        oblist))
    {
        notify_fail("Destruct what?\n");
        return 0;
    }

    oblist = NORMAL_ACCESS(oblist, 0, 0);
    if (!sizeof(oblist))
    {
        notify_fail("Destruct what?\n");
        return 0;
    }

    if (sizeof(oblist) > 1)
    {
        notify_fail("You can destruct only one object at a time.\n");
        return 0;
    }

    if (living(oblist[0]))
    {
        say(QCTNAME(oblist[0]) + " is disintegrated by " +
            QTNAME(this_interactive()) + ".\n");
        if (this_player()->query_option(OPT_ECHO))
            write("Destructed " + oblist[0]->query_the_name(this_player()) +
                  " (" + RPATH(MASTER_OB(oblist[0])) + ").\n");
        else
            write("Ok.\n");
    }
    else
    {
        say(QCTNAME(this_interactive()) + " disintegrates " +
            LANG_ASHORT(oblist[0]) + ".\n");
        if (this_player()->query_option(OPT_ECHO))
            write("Destructed " + LANG_THESHORT(oblist[0]) + " (" +
                  RPATH(MASTER_OB(oblist[0])) + ").\n");
        else
            write("Ok.\n");
    }

    if (dflag)
    {
        SECURITY->do_debug("destroy", oblist[0]);
    }
    else
    {
        oblist[0]->remove_object();
    }

    return 1;
}

/* **************************************************************************
 * distrust - distrust an object
 */
nomask int
distrust(string str)
{
    object ob;

    CHECK_SO_WIZ;

    if (!str)
    {
        notify_fail("Distrust what object?\n");
        return 0;
    }

    ob = present(str, this_interactive());

    if (!ob)
        ob = present(str, environment(this_interactive()));

    if (!ob)
        ob = parse_list(str);

    if (!ob)
    {
        notify_fail("Object not found: " + str + "\n");
        return 0;
    }

    if (geteuid(ob) != geteuid(this_object()))
    {
        notify_fail("Object not trusted by you.\n");
        return 0;
    }

    /* Remove the previous euid */
    ob->set_trusted(0);

    write("Ok.\n");
    return 1;
}

/* **************************************************************************
 * du - calculate disk usage
 */
static nomask int xdu(string p, int af);

nomask int
du(string str)
{
    int aflag;
    string p, flag, path;

    if (!str)
    {
        path = ".";
    }
    else
    {
        /* There is no getopt in CDlib... or? */
        if (sscanf(str, "%s %s", flag, path) == 2)
        {
            if (flag != "-a")
            {
                notify_fail("usage: du [-a] [path]\n");
                return 0;
            }
            else
                aflag = 1;
        }
        else
        {
            if (str == "-a")
            {
                aflag = 1;
                path = ".";
            }
            else
                path = str;
        }
    }
    p = FTPATH(this_interactive()->query_path(), path);

    if (p == "/")
        p = "";

    xdu(p, aflag);

        return 1;
}

static nomask int
xdu(string path, int aflag)
{
    int sum, i;
    string *files, output;

    files = get_dir(path + "/*");

    sum = 0;

    for (i = 0; i < sizeof(files); i++)
    {
        if (files[i] == "." || files[i] == "..")
            continue;

        if (aflag && file_size(path + "/" + files[i]) > -1)
        {
            write(file_size(path + "/" + files[i]) / 1024 + "\t" +
                  path + "/" + files[i] + "\n");
        }

        if (file_size(path + "/" + files[i]) == -2)
            sum += xdu(path + "/" + files[i], aflag);
        else
            sum += file_size(path + "/" + files[i]);
    }

    write(sum / 1024 + "\t" + path + "\n");
    return sum;
}

/* **************************************************************************
 * ed - edit a file
 */
nomask int
ed_file(string file)
{
    CHECK_SO_WIZ;

    if (!stringp(file))
    {
        ed();
        return 1;
    }
    file = FTPATH((string)this_interactive()->query_path(), file);
#ifdef LOG_ED_EDIT
    SECURITY->log_syslog(LOG_ED_EDIT, sprintf("%s %-11s %s\n", ctime(time()),
        capitalize(this_player()->query_real_name()), file));
#endif LOG_ED_EDIT
    ed(file);
    return 1;
}

/* **************************************************************************
 * load - load a file
 */

#define LOADMANY_MAX   (10)
#define LOADMANY_DELAY (5.0)

/*
 * Function name: load_many_delayed
 * Description  : When a wizard wants to test many files in one turn, this
 *                function is called by the alarm to prevent the system
 *                from slowing down too much and to prevent 'evaluation too
 *                long' types of errors.
 * Arguments    : object wizard - the wizard handling the object.
 *                string *files - the files to load still.
 */
static nomask void
load_many_delayed(object wizard, string *files)
{
    loadmany_wizard = wizard;

    if (!objectp(loadmany_wizard) ||
        (member_array(loadmany_wizard->query_real_name(),
            loadmany_going) == -1) ||
        (!interactive(loadmany_wizard)))
    {
        return;
    }

    loadmany_files = files;
    wizard->reopen_soul();
}

/*
 * Function name: load_many
 * Description  : When a wizard wants to test many files in one turn, this
 *                function tests only a few (LOADMANY_MAX to be exact) and
 *                then calls an alarm to test the other files.
 */
static nomask void
load_many()
{
    int    index = -1;
    int    size;
    object obj;
    string error;

    size = (sizeof(loadmany_files) > LOADMANY_MAX) ? LOADMANY_MAX :
        sizeof(loadmany_files);
    while(++index < size)
    {
        if (objectp(find_object(loadmany_files[index])) &&
            loadmany_wizard->query_option(OPT_ECHO))
        {
            tell_object(loadmany_wizard, "Already loaded:  " +
                loadmany_files[index] + "\n");

            /* Already loaded.. Readjust. */
            loadmany_files = exclude_array(loadmany_files, index, index);
            size = (sizeof(loadmany_files) > LOADMANY_MAX) ? LOADMANY_MAX :
                sizeof(loadmany_files);
            index--;

            continue;
        }

        if (error = catch(call_other(loadmany_files[index],
            "teleledningsanka")))
        {
            tell_object(loadmany_wizard, "Error loading:   " +
                loadmany_files[index] + "\nMessage      :   " + error + "\n");
            continue;
        }

        if (loadmany_wizard->query_option(OPT_ECHO))
            tell_object(loadmany_wizard, "Loaded:          " +
                loadmany_files[index] + "\n");

        /* Try to remove the object from memory the easy way. */
        if (catch(call_other(loadmany_files[index], "remove_object")))
        {
            tell_object(loadmany_wizard, "Cannot destruct: " +
                loadmany_files[index] + "\n");
        }

        /* Hammer hard if the object doesn't go away that easily. */
        if (objectp(obj = find_object(loadmany_files[index])))
        {
            SECURITY->do_debug("destroy", obj);
        }
    }

    if (sizeof(loadmany_files) > size)
    {
        set_alarm(LOADMANY_DELAY, 0.0, &load_many_delayed(loadmany_wizard,
            loadmany_files[size..]));
    }
    else
    {
        tell_object(loadmany_wizard, "Loading completed.\n");
        loadmany_going -= ({ loadmany_wizard->query_real_name() });
    }

    loadmany_files = 0;
    loadmany_wizard = 0;
}

/*
 * Function name: load_many_delayed_reloaded
 * Description  : After an alarm this object looses its euid, so we have
 *                to reopen the soul. This function could be integrated
 *                with load_many() itself, but I decided to separate them
 *                in order to make load_many() a static function.
 */
public nomask void
load_many_delayed_reloaded()
{
    if ((geteuid(previous_object()) != geteuid()) ||
        (!interactive(previous_object())) ||
        (calling_function() != REOPEN_SOUL))
    {
        loadmany_files = 0;
        loadmany_going -= ({ loadmany_wizard->query_real_name() });
        loadmany_wizard = 0;
        return;
    }

    load_many();
}

nomask int
load(string str)
{
    object obj;
    string *parts;
    string error;
    int recurse;
    int self;

    CHECK_SO_WIZ;

    if (!stringp(str))
    {
        notify_fail("Load what?\n");
        return 0;
    }

    parts = explode(str, " ");
    recurse = IN_ARRAY("-r", parts);
    parts -= ({ "-r" });
    str = implode(parts, " ");

    if (str == "stop")
    {
        if (member_array(this_player()->query_real_name(),
            loadmany_going) == -1)
        {
            notify_fail("You are not loading multiple files at the moment.\n");
            return 0;
        }

        loadmany_going -= ({ this_player()->query_real_name() });
        write("Stopped loading multiple files.\n");
        return 1;
    }

    str = FTPATH(this_interactive()->query_path(), str);
    if (!strlen(str))
    {
        notify_fail("Invalid file name.\n");
        return 0;
    }

    /* If wildcards are used, the wizard means to check many files. */
    if (wildmatch("*[\\*\\?]*", str))
    {
        if (member_array(this_player()->query_real_name(),
            loadmany_going) != -1)
        {
            notify_fail("You are already loading multiple files. You have " +
                "to use \"load stop\" first if you want to interrupt that " +
                "sequence and start a new one. Please bear in mind that " +
                "this operation is costly and that you should be careful " +
                "with executing it a lot.\n");
            return 0;
        }

        /* Get the files the wizard wants to load and filter only those
         * that are executable, ergo that end in .c
         */
        loadmany_files = filter(get_dir(str), &wildmatch("*.c"));

        if (!pointerp(loadmany_files) ||
            !sizeof(loadmany_files))
        {
            write("No files found: " + str + "\n");
            return 1;
        }

        write("Loading " + sizeof(loadmany_files) + " file" +
            ((sizeof(loadmany_files) == 1) ? "" : "s") + "." +
            ((sizeof(loadmany_files) > LOADMANY_MAX) ? (" A delay of " +
                ftoi(LOADMANY_DELAY) + " seconds is used each " +
                LOADMANY_MAX + " files.") : "") + "\n");

        /* We have to add the full path to all the files to load. Then */
        parts = explode(str, "/");
        parts[sizeof(parts) - 1] = "";
        loadmany_files = map(loadmany_files,
            &operator(+)(implode(parts, "/"), ));
        loadmany_wizard = this_interactive();
        loadmany_going += ({ this_player()->query_real_name() });

        load_many();
        return 1;
    }

    /* Add ".c" if needed. */
    if (!wildmatch("*.c", str))
    {
        str += ".c";
    }
    /* File does not exists. */
    if (file_size(str) < 0)
    {
        notify_fail("No such file.\n");
        return 0;
    }

    /* If the object is already in memory, destruct it. */
    if (objectp(obj = find_object(str)))
    {
        self = (obj == this_object());

        write("Trying to update: " + str + "\n");

        if (!update_ob(str, recurse))
        {
            write("Updating failed...\n");
            return 1;
        }

        /* Don't try to reload ourselves. Just update ... */
        if (self)
            return 1;
    }

    if (error = catch(str->teleledningsanka()))
    {
        write("Error loading: " + str + "\n");
        write("Message: " + error + "\n");
        return 1;
    }

    if (this_player()->query_option(OPT_ECHO))
        write("Loaded: " + str + "\n");
    else
        write("Ok.\n");
    return 1;
}

/* **************************************************************************
 * merge - merge a file with its inherited code
 */

/*
 * Function name: merge_one
 * Description  : This routine does the actual deep recursive merging.
 * Arguments    : string line - the line to parse.
 *                string dir - the directory of the current context.
 *                string file - the file to merge the code into.
 */
void
merge_one(string line, string dir, string file)
{
    string path, text;

    write_file(file, line + "\n");
    if (sscanf(line, "inherit \"%s\";%s", path, text) == 2)
    {
        path = FPATH(dir, path);
        if (extract(path, -2) != ".c") path += ".c";
        write("inherits " + path + "\n");

        if (wildmatch("/[dw]/*", path))
        {
            text = read_file(path);
            if (strlen(text))
            {
                 map(explode(text, "\n"), &merge_one(, FILE_PATH(path), file));
            }
            write_file(file, "// end of " + line + "\n\n");
            return;
        }
    }
}

int
merge(string file)
{
    string path, text;

    CHECK_SO_WIZ;

    if (!strlen(file))
    {
        notify_fail(capitalize(query_verb() + " what?\n"));
        return 0;
    }
    file = FTPATH((string)this_interactive()->query_path(), file);
    if (file_size(file) <= 0)
    {
        notify_fail("Not a valid file: " + file + "\n");
        return 0;
    }

    text = read_file(file);
    if (!strlen(text))
    {
        write("No content: " + file + "\n");
        return 1;
    }
    file += ".merge";
    if (file_size(file) > 0)
    {
       rm(file);
    }
    map(explode(text, "\n"), &merge_one(, FILE_PATH(file), file));

    write("Merged into: " + file + "\n");
    return 1;
}

/* **************************************************************************
 * mkdir - make a directory
 */
nomask int
makedir(string str)
{
    CHECK_SO_WIZ;

    if (!stringp(str))
    {
        notify_fail("Make what dir?\n");
        return 0;
    }
    if (mkdir(FTPATH((string)this_interactive()->query_path(), str)))
        write("Ok.\n");
    else
        write("Fail.\n");
    return 1;
}


/* **************************************************************************
 * mv - move multiple files or a sigle directory.
 */
nomask int
mv_cmd(string str)
{
    return multi_file(str, MULTI_MV);
}

/* **************************************************************************
 * remake - Remake an object, checks entire dependency of inherited files
 */
nomask int
remake(string str)
{
    return update("-r" + (strlen(str) ? (" " + str) : ""));
}

/* **************************************************************************
 * rm - remove multiple files
 */
nomask int
rm_cmd(string str)
{
    return multi_file(str, MULTI_RM);
}

/* **************************************************************************
 * rmdir - delete a directory
 */
nomask int
removedir(string str)
{
    CHECK_SO_WIZ;

    if (!stringp(str))
    {
        notify_fail("Remove what dir?\n");
        return 0;
    }
    if (rmdir(FTPATH((string)this_interactive()->query_path(), str)))
        write("Ok.\n");
    else
        write("Fail.\n");
    return 1;
}

/* **************************************************************************
 * trust - trust an object.
 */
nomask int
trust_ob(string str)
{
    object ob;

    CHECK_SO_WIZ;

    if (!str)
    {
        notify_fail("Trust what object?\n");
        return 0;
    }

    ob = parse_list(str);

    if (!ob)
    {
        notify_fail("Object not found: " + str + "\n");
        return 0;
    }

    if (geteuid(ob))
    {
        notify_fail("Object already trusted by: " + geteuid(ob) + "\n");
        return 0;
    }

    /* Install the euid of this player as uid in the object */
    export_uid(ob);
    /* Activate the object */
    ob->set_trusted(1);

    write("Beware! You have just trusted: " + str + ".\n");
    return 1;
}

/* **************************************************************************
 * update - update an object
 */

/*
 * Function name: update_startloc_check
 * Description  : Find out whether you're trying to update the start location
 *                of a player ... while he is in the room.
 * Arguments    : object room - the room (object) to check.
 * Returns      : int 1/0 - if true, the player is in his start location.
 */
nomask int
update_startloc_check(object room)
{
    string path = MASTER_OB(room);
    object *players = FILTER_PLAYERS(all_inventory(room));

    foreach(object player: players)
    {
        if (player->query_default_start_location() == path)
        {
            write("Cannot update the start location of " +
                capitalize(player->query_real_name()) + ".\n");
            return 1;
        }
    }
    return 0;
}

/*
 * Function name: update_one_object
 * Description  : Execute the update of an object, including recursion.
 * Arguments    : object ob - the object to update.
 *                int recurse - if true, recurse the whole chain.
 */
nomask void
update_one_object(object ob, int recurse)
{
    string str = MASTER_OB(ob);
    string *files = SECURITY->do_debug("inherit_list", ob);

    ob->remove_object();
    if (ob = find_object(str))
        SECURITY->do_debug("destroy", ob);

    if (recurse)
    {
        /* We don't want to update the AUTO object. */
        files -= ({ SECURE_AUTO + ".c" });

        foreach(string file: files)
        {
            if (ob = find_object(file))
            {
                write("  " + file + "\n");
                ob->remove_object();
            }

            if (ob = find_object(file))
                SECURITY->do_debug("destroy", ob);
        }
    }
}

nomask int
update_ob(string str, int recurse)
{
    object ob, *players;
    int kick_master;
    string error;
    string *files;

    CHECK_SO_WIZ;

    if (!strlen(str))
    {
        if (!objectp(ob = environment(this_player())))
        {
            notify_fail("Update what? The void?\n");
            return 0;
        }

        /* Avoid updating a player's start location. */
        if (update_startloc_check(ob))
        {
            return 1;
        }

        write("Updating environment.\n");
        say(QCTNAME(this_player()) + " updates the surroundings.\n");

        /* Move all players out of the room to their start location. */
        players = FILTER_PLAYERS(all_inventory(ob));
        foreach(object player: players)
        {
            player->move(player->query_default_start_location());
        }

        str = MASTER_OB(ob);
        /* The actual update. */
        update_one_object(ob, recurse);

        if (strlen(error = catch(str->teleledningsanka())))
        {
            write("Error loading: " + str + "\n");
            write("Message: " + error + "\n");
            players->catch_tell("Something went wrong. You find yourself ... home.\n\n");
            foreach(object player: players)
            {
                set_this_player(player);
                player->do_glance(player->query_option(OPT_BRIEF));
            }

            return 1;
        }

        /* And bring the players back. */
        players->move(str);
        return 1;
    }

    str = FTPATH((string)this_interactive()->query_path(), str);
    if (!strlen(str))
    {
        notify_fail("Invalid file name.\n");
        return 0;
    }

    if (!objectp(ob = find_object(str)))
    {
        notify_fail("No such object.\n");
        return 0;
    }

    if (ob == find_object(SECURITY))
        kick_master = 1;

    str = MASTER_OB(ob) + ".c";
    if (ob == this_object())
    {
        write("Updated: " + str + "\n");
        write("Destructing this object.\n");

        /* Remove the binary file too. */
        SECURITY->remove_binary(MASTER);

        destruct();
        return 1;
    }

    /* Remove the binary file too. */
    SECURITY->remove_binary(str);

    /* Avoid updating someone's start location while he is in it. */
    if (update_startloc_check(ob))
    {
        return 1;
    }

    /* The actual update. */
    update_one_object(ob, recurse);

    /* When updating the master object it must be reloaded at once and from
     * within the GD.
     */
    if (kick_master)
    {
        write(SECURITY + " was updated and reloaded.\n");
        SECURITY->teleledningsanka();
        return 1;
    }

    if (objectp(ob) && ob == find_object(str))
    {
        notify_fail("Object could not be updated.\n");
        return 0;
    }

    if (this_player()->query_option(OPT_ECHO))
        write("Updated: " + str + "\n");
    else
        write("Ok.\n");
    return 1;
}

nomask int
update(string str)
{
    string *files, *args, path, fpath;
    int recurse, dirupd;

    /* catch 'update' without args */
    if (!strlen(str))
    {
        return update_ob(str, 0);
    }

    /* check if user specifies the -d or -r options */
    args = explode(str, " ");
    recurse = IN_ARRAY("-r", args);
    dirupd = IN_ARRAY("-d", args);
    args -= ({ "-r", "-d" });
    str = implode(args, " ");

    if (!dirupd)
    {
        return update_ob(str, recurse);
    }

    /* fix path and get files */
    path = FTPATH(this_interactive()->query_path(), str);
    files = get_dir(path);

    /* remove file pattern from end of path to expand path in update loop */
    args = explode(path, "/");
    args = exclude_array(args, sizeof(args) - 1, sizeof(args));
    path = implode(args, "/");

    /* move through all files returned by 'dir' */
    foreach(string file: files)
    {
        /* get full filepath to objects */
        fpath = FTPATH(path, file);

        /* just update existing objects */
        if (find_object(fpath))
        {
            /* Ignore the update success when doing multiple. */
            update_ob(fpath, recurse);
        }
    }
    return 1;
}
