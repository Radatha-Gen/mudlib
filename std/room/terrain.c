/*
 * /std/room/terrain.c
 *
 * File containing terrain and map related functions.
 */

#include <terrain.h>

static int room_terrain;

/* The map coordinates of this room. */
static string map_mapfile,
              map_section;
static int    map_x, map_y,
              map_zoomx, map_zoomy;

/*
 * Function name: init_map_data
 * Description  : Obtain the map information from the map central.
 */
void
init_map_data()
{
    mixed mapdata = MAP_CENTRAL->query_room_map_data(MASTER);

    if (pointerp(mapdata) && (sizeof(mapdata) == 6))
    {
	map_mapfile = mapdata[0];
	map_section = mapdata[1];
	map_x = mapdata[2];
	map_y = mapdata[3];
	map_zoomx = mapdata[4];
	map_zoomy = mapdata[5];
    }
}

/*
 * Function name: terrain_includes_all
 * Description  : Checks if the room contains terrain of a 
 *                certain type (or combination of types)
 * Arguments    : int terrain - the terrain (or combination of terrains) 
 *                to check for
 * Returns      : in - 1 if terrain matches the room, 0 otherwise
 */
int
terrain_includes_all(int terrain)
{
  return ((room_terrain & terrain) == terrain);
}

/*
 * Function name: terrain_includes_any
 * Description  : Checks if the terrain of the room includes a
 *                certain type
 * Arguments    : int terrain - the terrain (or combination of terrains) 
 *                to check for
 * Returns      : int - 1 if terrain found in the room, 0 otherwise
 */
int
terrain_includes_any(int terrain)
{
  return ((room_terrain & terrain) != 0);
}

/*
 * Function name: terrain_exact
 * Description  : Checks if the terrain of the room exactly matches a 
 *                certain type
 * Arguments    : int terrain - the terrain (or combination of terrains) 
 *                to check for
 * Returns      : int - 1 if terrain matches the room, 0 otherwise
 */
int
terrain_exact(int terrain)
{
  return ((room_terrain ^ terrain) == 0);
}

/*
 * Function name: set_terrain
 * Description  : Set the terrain proerties for the room. This is
 *                normally called from create_room()
 * Arguments    : int terrain - the terrain value to set (see /sys/terrain.h)
 */
void
set_terrain(int terrain)
{
  room_terrain = terrain;
}

/*
 * Function name: query_terrain
 * Description  : Query the room for its terrain properties
 * Returns      : int - terrain value
 */
int
query_terrain()
{
  return room_terrain;
}
