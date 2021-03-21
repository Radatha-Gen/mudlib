/*
 * /secure/srcman.c
 *
 * Handle search requests in source documentation.
 */
#pragma strict_types
#pragma save_binary

#include <macros.h>
#include <std.h>

#define SMANDIR "/doc/sman"
#define MAXLOCAL_DIRS 10
/*
 * DOCMARK
 *
 * If defined, this file must exist within a directory for the directory
 * to be accepted as a documentation directory
 *
 */
#undef DOCMARK /* "/DOCDIR" */

/*
 * docdirs contains the indices to the manual.
 * Each entry on the form:
 *
 * ([ "/dir/name/" : ({ subdirs_sorted, ([ subdir : funcs ]) }) ])
 *
 * There can be many subdir:funcs for one main docdir.
 * The funcs is a string of the form "function1%%function2%%function3"
 */
static  mapping docdirs;

/*
 * Function name:   valid_docdir
 * Description:     Check if this is a valid documentation directory
 */
int
valid_docdir(string ddir)
{
#ifdef DOCMARK
    if (file_size(ddir + DOCMARK) < 0)
        return 0;
    else
#endif
        return 1;
}


/*
 * Function name:   read_index
 * Description:     Read the contents of a docdir and return a mapping
 *                  containing all the ([ subdir:info ]) information
 *
 */
static mapping
read_index(string mdir)
{
    string *files, *sfiles, sdname, *funcs;
    mapping res, sdir;
    int i, j;

    seteuid(geteuid(this_player()));

    files = get_dir(mdir + "/*");
    if (!sizeof(files))
        return ([]);

    files -= ({ ".", ".." });

    funcs = ({ });
    res = ([]);

    for (i = 0; i < sizeof(files); i++)
    {
        sdname = mdir + "/" + files[i];
        if (file_size(sdname) == -2 && (files[i] != ".obsolete"))
        {
            if (mappingp(docdirs[sdname]))
                sdir = docdirs[sdname];
            else
                sdir = read_index(sdname);
            sfiles = m_indexes(sdir);
            for (j = 0; j < sizeof(sfiles); j++)
            {
                res["/" + files[i] +
                    (sfiles[j] == "/" ? "" : sfiles[j])] = sdir[sfiles[j]];
            }
        }
        else
        {
            funcs += ({ files[i] });
        }
    }
    if (sizeof(funcs))
    {
        res["/"] = implode(funcs, "%%");
    }

    return res;
}

/*
 * This mapping is used to sort the matches to provide a better first match
 * in most cases.
 */
static mapping priorities = ([
    "/secure/simul_efun": 100,
    "/secure/master":  95,
    "/std/object": 90,
    "/std/container": 80,
    "/std/monster": 70,
    "/std": 50,
    "/lib": 40,
    "/cmd": 30,
    "/obj": 20,
    "/secure": 10,
    "/d": -5
]);

/*
 * Function name: path_priorty
 * Description  : Calculate a priority for a path.
 */
 static int
 path_priority(string path)
 {
     return match_path(priorities, path);
 }

 /*
  * Function name: sort_path
  * Description  : Sort paths based on priority
  */
 static int
 compare_paths(mapping priority, string a, string b)
 {
     if (priority[a] != priority[b])
        return priority[b] - priority[a];

    if (a > b)
        return 1;
    if (a < b)
        return -1;
    return 0;
 }

/*
 * Function name:   init_man
 * Description:     Initialize the docdir info for a specific docdir
 */
static void
init_docdir(string mdir)
{
    if (!mappingp(docdirs))
    {
        docdirs = ([]);
        if (mdir != SMANDIR)
            init_docdir(SMANDIR);
    }

    if (!valid_docdir(mdir))
        return;

    if (m_sizeof(docdirs) >= MAXLOCAL_DIRS)
    {
        string dir = m_indexes(docdirs)[random(m_sizeof(docdirs) - 1) + 1];
        m_delkey(docdirs, dir);
    }

    mapping sdirs = read_index(mdir);
    string *paths = m_indexes(sdirs);

    /* Sort paths to put the most important files first */
    mapping lookup = mkmapping(paths, map(paths, &match_path(priorities, )));
    sort_array(paths, &compare_paths(lookup, , ));

    if (m_sizeof(sdirs))
        docdirs[mdir] = ({ paths, sdirs });
}

/*
 * Function name:   update_index
 * Description:     Update the function index
 */
public void
update_index(string mdir)
{
    init_docdir(mdir);
}


/*
 * Function name:   get_subdirs
 * Description:     Return a list of subdirectories for a given doc-dir
 */
string *
get_subdirs(string ddir)
{
    if (!mappingp(docdirs) || !pointerp(docdirs[ddir]))
        init_docdir(ddir);

    if (!sizeof(docdirs[ddir]))
        return ({});

    return docdirs[ddir][0];
}

string *
fix_subjlist(string *split, string keyw, int okbef, int okaft)
{
    return MANCTRL->fix_subjlist(split, keyw, okbef, okaft);
}

/*
 * Function name:   filter_keyword
 * Description:     Return all possible function names that match a
 *                  given keyword in one or all subdirs.
 * Arguments:       mdir    - The main documentation directory
 *                  subdir  - The subdir to search in. (Optional)
 *                  keyword - The keyword to search for.
 * Returns:         An array with the matching keywords
 */
public string *
filter_keyword(string mdir, string sdir, string keyword)
{
    mixed *fun = this_object()->get_index(mdir, sdir);

    if (!sizeof(fun)) {
       return ({ });
    }

    return filter(fun[1], &wildmatch(keyword, ));
}


/*
 * Function name:   get_keywords
 * Description:     Return all possible function names that match a
 *                  given keyword in one or all subdirs.
 * Arguments:       mdir    - The main documentation directory
 *                  subdir  - The subdir to search in. (Optional)
 *                  keyword - The keyword to search for.
 * Returns:         An array containing the list of found names in each
 *                  subdir. Each entry is on the form:
 *                      ({ "subdir", ({ "funcname", "funcname" ... }) })
 */
public mixed *
get_keywords(string mdir, string sdir, string keyword)
{
    mixed *found_arr, *tmp;
    string *sdlist, st;
    int i, okbef, okaft;

    if (!sizeof(sdlist = get_subdirs(mdir)))
        return ({ });

    if (keyword == "" || keyword == "*")
        return this_object()->get_index(mdir, sdir);

    if (stringp(sdir) && member_array(sdir, sdlist) >= 0)
        return ({ sdir, filter_keyword(mdir, sdir, keyword) });
    else if (stringp(sdir)) /* No such subdir */
        return ({});

    found_arr = ({});
    for (i = 0; i < sizeof(sdlist); i++)
    {
        tmp = filter_keyword(mdir, sdir, keyword);

        if (sizeof(tmp))
            found_arr += ({ ({ sdlist[i], tmp }) });
    }

    return found_arr;
}


/*
 * Function name:   get_index
 * Description:     Return all function names in a given subdir (object)
 * Arguments:       mdir - The main documentation directory
 *                  sdir - The subdir to search in. (Optional)
 * Returns:         An array containing the list of found names in each
 *                  subdir. Each entry is on the form:
 *                      ({ "subdir", ({ "funcname", "funcname" ... }) })
 */
public mixed *
get_index(string mdir, string sdir)
{
    mixed *found_arr, *tmp;
    string *sdlist;
    int i;

    if (!sizeof(sdlist = get_subdirs(mdir)))
        return ({ });

    if (stringp(sdir) && member_array(sdir, sdlist) >= 0)
        return ({ sdir, explode(docdirs[mdir][1][sdir], "%%") });

    else if (stringp(sdir)) /* No such subdir */
        return ({});

    found_arr = ({});
    for (i = 0; i < sizeof(sdlist); i++)
    {
        tmp = explode(docdirs[mdir][1][sdlist[i]], "%%");

        if (sizeof(tmp))
            found_arr += ({ ({ sdlist[i], tmp }) });
    }

    return found_arr;
}
