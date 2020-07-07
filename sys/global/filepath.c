/*
 * /sys/global/filepath.c
 *
 * Useful filepath manipulation.
 */

#pragma save_binary
#pragma strict_types

#include <config.h>
#include <macros.h>
#include <std.h>

#define DEF_START_FILE  "/sys/data/DEFAULT_START"
#define TEMP_START_FILE "/sys/data/TEMP_START"

/*
 * def_start_locations - array of default start locations (mostly guild starts)
 * temp_start_locations - array of temporary start locations (inns etc)
 */
static string *def_start_locations;
static string *temp_start_locations;

/*
 * Function name: create
 * Description  : Called upon initialization to read the adverbs into
 *                memory.
 */
nomask void
create()
{
    setuid();
    seteuid(getuid());

    /* Read the adverbs-file if possible. */
    if (file_size(DEF_START_FILE) > 0)
    {
	def_start_locations = sort_array(explode(read_file(DEF_START_FILE), "\n")  - ({ "" }) );
    }
    if (!sizeof(def_start_locations))
    {
	def_start_locations = DEF_STARTING_PLACES;
    }
    if (file_size(TEMP_START_FILE) > 0)
    {
	temp_start_locations = sort_array(explode(read_file(TEMP_START_FILE), "\n") - ({ "" }) );
    }
    if (!sizeof(temp_start_locations))
    {
	temp_start_locations = TEMP_STARTING_PLACES;
    }
}

/*
 * Function name: fix_path
 * Description  : Fixes the pathname of files to a full path.
 * Arguments    : string path - the static path to resolve against.
 *                string name - a filename with possibly a relative path.
 * Returns      : string - the full path filename.
 */
string
fix_path(string path, string name)
{
    string *parts;
    int index;
    int size;

    /* If the name is already an absolute path, don't prepend the path. */
    parts = explode(((name[0] != '/') ? path : "") + "/" + name + "/", "/");

    size = 0;
    for (index = 0; index < sizeof(parts); index++)
    {
        if (parts[index] == "..")
        {
            size -= (size) ? 1 : 0;
        }
        else if ((parts[index] != ".") && (parts[index] != ""))
        {
            parts[size++] = parts[index];
        }
    }

    if (size > 0)
    {
        return "/" + implode(parts[0..(size - 1)], "/");
    }

    return "/";
}

/*
 * Function name: get_tilde_path
 * Description  : Gets the default path for a wizard or domain.
 * Arguments    : string name - the name of the default wizard or domain. The
 *                    name of a domain must be capitalised.
 *                string tilde - a path that starts with a ~, i.e. "~",
 *                    "~wizname" or  "~Domainname".
 * Returns      : string - the full path.
 */
string
get_tilde_path(string name, string tilde)
{
    string *parts;
    string wizpath;

    /* Path doesn't start with a tilde, immediately return. */
    if (tilde[0] != '~')
    {
        return tilde;
    }

    parts = explode(tilde, "/");

    /* Look at the name of another wizard/domain. */
    if (parts[0] != "~")
    {
        name = extract(parts[0], 1);
    }

    /* Check against a wizard name if the name is in lower case. */
    if (name != capitalize(name))
    {
        if (!strlen(wizpath = SECURITY->query_wiz_path(name)))
        {
            return tilde;
        }

        return wizpath + "/" + implode(parts[1..], "/");
    }

    /* Treat it as domain name as it was capitalised. */
    return "/d/" + name + "/" + implode(parts[1..], "/");
}

/*
 * Function name: reduce_to_tilde_path
 * Description  : Take a path and reduce it to its tilde equivalent, i.e. the
 *                file "/d/Genesis/file.c" becomes "~Genesis/file.c".
 * Arguments    : string path - the path to transform.
 * Returns      : string - the tilde path representation of the path.
 */
string
reduce_to_tilde_path(string path)
{
    string *parts;

    parts = explode(path, "/") - ({ "" });

    /* Must have at least two parts to be able to shorten. */
    if (sizeof(parts) < 2)
    {
        return path;
    }

    if (parts[0] == "d")
    {
        /* Not a domain, or the independant wizard domain. */
        if ((SECURITY->query_domain_number(parts[1]) < 0) ||
            (parts[1] == WIZARD_DOMAIN))
        {
            return path;
        }

        return "~" + implode(parts[1..], "/");
    }

    if (parts[0] == "w")
    {
        /* Only for domain wizards we allow the tilde notation. */
        if (strlen(SECURITY->query_wiz_dom(parts[1])))
        {
            return "~" + implode(parts[1..], "/");
        }
    }

    return path;
}

/*
 * Function name: valid_def_start_location
 * Description  : Find out if a startloc is validated as default starting
 *                location.
 * Arguments    : string startloc - the startloc to test (without .c)
 * Returns      : int 1/0 - if true, it's validated.
 */
public int
valid_def_start_location(string startloc)
{
    return IN_ARRAY(startloc, def_start_locations);
}

/*
 * Function name: valid_temp_start_location
 * Description  : Find out if a startloc is validated as temporary starting
 *                location.
 * Arguments    : string startloc - the startloc to test (without .c)
 * Returns      : int 1/0 - if true, it's validated.
 */
public int
valid_temp_start_location(string startloc)
{
    return IN_ARRAY(startloc, temp_start_locations);
}

/*
 * Function name: query_def_start_locations
 * Description  : Returns a string array of default start locations.
 * Returns      : string array
 */
public string *
query_def_start_locations()
{
    return secure_var(def_start_locations);
}

/*
 * Function name: query_temp_start_locations
 * Description  : Returns a string array of temp start locations.
 * Returns      : string array
 */
public string *
query_temp_start_locations()
{
    return secure_var(temp_start_locations);
}
