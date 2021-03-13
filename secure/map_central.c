/*
 * map_central.c
 *
 * This object contains the maps of the realms. Management is via the 'map'
 * command.
 */

#pragma no_clone
#pragma no_inherit
#pragma no_shadow
#pragma strict_types

#include <files.h>
#include <macros.h>

#define MAP_MAPLINKS "/data/maplinks"
#define MAP_MAPFILES "/data/maps"
#define MAP_ID       "_map_"

/*
 * maplinks = ([ (string)path : (string)mapfile ])
 * maps = ([ (string)mapfile :
 *           ([ (string)section:
 *              ([ "_map_" : (string)maptext,
 *                 (string)filename : (string)coords ]) ]) ])
 *
 * Note: path is without .c
 */
mapping maplinks;
mapping maps;
int     alarm_id = 0;

/*
 * Function name: create
 * Description  : Constructor.
 */
void
create()
{
    setuid();
    seteuid(getuid());

    maplinks = restore_map(MAP_MAPLINKS);
    if (!mappingp(maplinks))
	maplinks = ([ ]);
    maps = restore_map(MAP_MAPFILES);
    if (!mappingp(maps))
	maps = ([ ]);
}

/*
 * Function name: short
 * Description  : Identify the object by a nice short description.
 * Returns      : string - the short description.
 */
string
short()
{
    return "the archive of maps";
}

/*
 * Function name: save_mapdata
 * Description  : Save the map files. Called from an alarm usually.
 */
void
save_mapdata()
{
    save_map(maplinks, MAP_MAPLINKS);
    save_map(maps, MAP_MAPFILES);

    alarm_id = 0;
}

/*
 * Function name: add_maplink
 * Description  : Register a room with its map. Both path and mapfile need
 *                to exist for this to work.
 * Arguments    : string path - the full path to the room (excluding .c).
 *                string mapfile - the mapfile with fully qualified path.
 */
void
add_maplink(string path, string mapfile)
{
    if (!maps[mapfile] || ((file_size(path + ".c") < 1) && !find_object(path)))
    {
	return;
    }

    maplinks[path] = mapfile;

    /* Use a small alarm, so that multiple actions are saved in one go. */
    if (!alarm_id)
    {
	alarm_id = set_alarm(300.0, 0.0, save_mapdata);
    }
}

/*
 * Function name: remove_maplink
 * Description  : Remove a room (and its mapfile association).
 * Arguments    : string path - the full path to the room (excluding .c).
 */
void
remove_maplink(string path)
{
    m_delkey(maplinks, path);

    /* Use a small alarm, so that multiple actions are saved in one go. */
    if (!alarm_id)
    {
	alarm_id = set_alarm(300.0, 0.0, save_mapdata);
    }
}

/*
 * Function name: query_maplink
 * Description  : Find out the mapfile associated with a room.
 * Arguments    : string path - the full path to the room (including .c).
 * Returns      : string - the mapfile.
 */
public string
query_maplink(string path)
{
    return maplinks[path];
}

/*
 * Function name: query_maplink
 * Description  : Find out the rooms associated with a mapfile.
 * Arguments    : string mapfile - the mapfile with fully qualified path.
 * Returns      : string * - the rooms that link to it.
 */
public string *
query_maplinks(string mapfile)
{
    return m_indices(filter(maplinks, &operator(==)(, mapfile)));
}

/*
 * Function name: query_room_map_data
 * Description  : Find out the map information associated with a room.
 * Arguments    : string path - the full path to the room (including .c).
 * Returns      : mixed - array or 0 if no such data.
 *                ({ (string)mapfile, (string)section,
 *                   (int)x, (int)y, (int)zoomx, (int)zoomy })
 */
public mixed
query_room_map_data(string path)
{
    string mapfile = query_maplink(path);
    int ix, iy, size;
    mixed main = ({ 0, 0, 0 });
    mixed zoom = ({ 0, 0 });
    string filename;
    string str;

    if (!strlen(mapfile) || !mappingp(maps[mapfile]))
    {
	return 0;
    }
    filename = explode(path, "/")[-1];

    foreach(string section: m_indices(maps[mapfile]))
    {
	if (!maps[mapfile][section][filename])
	{
	    continue;
	}
	size = sscanf(maps[mapfile][section][filename], "%d %d %s", ix, iy, str);
	/* Only x and y means this is where the file is on the map. */
	if ((size == 2) || (str == section))
	{
	    main = ({ section, ix, iy });
	}
	/* Also a section name means these are the coordinates for the zoom. */
	if ((size == 3) && (str != section))
	{
	    zoom = ({ ix, iy });
	}
    }
    return ({ mapfile }) + main + zoom;
}

/*
 * Function name: query_map
 * Description  : Get the map of a certain mapfile/section.
 * Arguments    : string mapfile - the mapfile to read.
 *                string section - the section of the map (optional)
 * Returns      : string - the map itself, or 0.
 */
public varargs string
query_map(string mapfile, string section = "main")
{
    if (!mappingp(maps[mapfile]))
    {
	return 0;
    }
    if (!maps[mapfile][section])
    {
	return 0;
    }
    return maps[mapfile][section][MAP_ID];
}

/*
 * Function name: query_map_with_coords
 * Description  : Get the map of a certain mapfile/section; X marks the spot.
 * Arguments    : string mapfile - the mapfile to read.
 *                string section - the section of the map
 *                int ix, iy - the coordinates of the player.
 * Returns      : string - the map; X marks the spot, or 0.
 */
public string
query_map_with_coords(string mapfile, string section, int ix, int iy)
{
    string maptext = query_map(mapfile, section);
    string *lines;

    if (!maptext)
    {
	return 0;
    }

    lines = explode(maptext, "\n");
    if (iy >= sizeof(lines))
    {
	return maptext + "\n";
    }
    ix--;
    if (ix >= strlen(lines[iy]))
    {
	return maptext + "\n";
    }
    /* Place the X at the coordinates. This relies on ix never being 0. */
    lines[iy] = (lines[iy][..ix]) + "X" + lines[iy][(ix+2)..];

    return (implode(lines, "\n") + "\n");
}

/*
 * Function name: add_map
 * Description  : Reads a mapfile and stores the content into the maps
 *                data structure.
 * Arguments    : string mapfile - the filename of the mapfile.
 * Returns      : int 1/0 - success/failure.
 */
public int
add_map(string mapfile)
{
    string *newmaps;
    string *parts;
    string *lines;
    string filename, args;
    mapping data = ([ ]);

    /* Go through the front end provided by the 'map' command. */
    if (!CALL_BY(WIZ_CMD_WIZARD))
    {
	return 0;
    }

    if (file_size(mapfile) < 1)
    {
	write("Mapfile " + mapfile + " not found.\n");
	return 0;
    }

    newmaps = explode(read_file(mapfile), "::NEWMAP::\n");
    foreach(string newmap: newmaps)
    {
        /* A map should consist of 3 parts: the name, the coordinates and
         * the actual map. */
	parts = explode(newmap, "\n::MAPDATA::\n");
	if (sizeof(parts) != 3)
	{
	    write("Mapfile " + mapfile + " section \"" + newmap[..29] +
	        "\" with " + sizeof(parts) + " part(s) skipped.\n");
	    return 0;
	}
	data += ([ parts[0] : ([ MAP_ID : parts[2] ]) ]);
	lines = explode(parts[1], "\n");
	foreach(string line: lines)
	{
	    /* Strip superfluous spaces from the coordinates. */
	    while(line[-1] == ' ')
	    {
		line = line[..-2];
	    }
	    if (sscanf(line, "%s %s", filename, args) != 2)
	    {
		write("Coordinates \"" + line + "\"of map \"" +
		    parts[0] + "\" not understood.\n");
		return 0;
	    }
	    data[parts[0]] += ([ filename : args ]);
	}
    }

    /* Replace existing info. */
    maps[mapfile] = data;

    /* Use a small alarm, so that multiple actions are saved in one go. */
    if (!alarm_id)
    {
	alarm_id = set_alarm(300.0, 0.0, save_mapdata);
    }
    return 1;
}

/*
 * Function name: remove_map
 * Description  : Obliterate a mapfile (graphics, coordinates and all
 *                room associatations to it).
 * Arguments    : string mapfile - the full path to the mapfile.
 * Returns      : int 1/0 - success/failure.
 */
public int
remove_map(string mapfile)
{
    string *paths;

    /* Go through the front end provided by the 'map' command. */
    if (!CALL_BY(WIZ_CMD_WIZARD))
    {
	return 0;
    }

    /* Remove all rooms that link to this mapfile. */
    paths = m_indices(filter(maplinks, &operator(==)(, mapfile)));
    foreach(string path: paths)
    {
	m_delkey(maplinks, path);
    }

    m_delkey(maps, mapfile);

    /* Use a small alarm, so that multiple actions are saved in one go. */
    if (!alarm_id)
    {
	alarm_id = set_alarm(300.0, 0.0, save_mapdata);
    }
    return 1;
}

/*
 * Function name: link_map
 * Description  : Cross-link a map to a set of files that is contained therein.
 *                It requires the wizard to have write access to the dir/path.
 * Arguments    : string path - the room the wizard is in, to transform into
 *                    the directory.
 *                string mapfile - the filename of the mapfile.
 * Returns      : int 1/0 - success/failure.
 */
public int
link_map(string path, string mapfile)
{
    string *dirfiles, *files, *parts;
    int ix, iy, size, linked;
    object room;

    /* Go through the front end provided by the 'map' command. */
    if (!CALL_BY(WIZ_CMD_WIZARD))
    {
	return 0;
    }
    if (!strlen(mapfile) || !mappingp(maps[mapfile]))
    {
	write("Map not found: " + mapfile + "\n");
	write("Reminder: use 'map add' before you try to link it.\n");
	return 0;
    }
    /* Petros requested that access to the map file should be enough, and
     * not specifically to the path being mapped. */
    if (!SECURITY->valid_write(mapfile, this_interactive(), "map"))
    {
	write("No access to write in: " + path + "\n");
	return 0;
    }

    path = implode(explode(path, "/")[..-2], "/") + "/";
    dirfiles = get_dir(path + "*.c");
    dirfiles = map(dirfiles, &extract(, 0, -3));

    foreach(string section: m_indices(maps[mapfile]))
    {
	files = sort_array(dirfiles & (string *)m_indices(maps[mapfile][section]));
	foreach(string filename : files)
	{
	    if (!maps[mapfile][section][filename])
	    {
	        write("Error: No coordinates for " + filename +
		    " in section " + section + "\n");
	    }
	    /* Only link if the file is mentioned in the details section. This
             * can be idenfitied by the format "x y" rather than "x y foo".
             * If the "foo" part is present, it's a reference to a details
             * section where the map part is located. */
            parts = explode(maps[mapfile][section][filename], " ") - ({ "" });
            if (sizeof(parts) == 2)
	    {
                ix = atoi(parts[0]);
                iy = atoi(parts[1]);
		if (query_maplink(path + filename) == mapfile)
		{
		    write("Already linked " + path + filename + "\n");
		    continue;
		}
		write("Linked " + path + filename +
                    " (" + section + " " + ix + "," + iy + ")\n");
		add_maplink(path + filename, mapfile);
                /* Init the map if the room is loaded. */
                if (objectp(room = find_object(path + filename)))
                {
                    room->init_map_data();
                }
		linked++;
	    }
	}
    }
    if (!linked)
    {
	write("Found no matches in: " + path + "\n");
	return 0;
    }
    return 1;
}
