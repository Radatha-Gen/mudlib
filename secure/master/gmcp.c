/*
 * gmcp.c
 *
 * Implementation of GMCP (Generic MUD Communication Protocol).
 */

#include "/sys/files.h"
#include "/sys/gmcp.h"
#include "/sys/ss_types.h"

static int     gmcp_alarm;
static mapping gmcp_tokens = ([ ]);

/*
 * Function name: gmcp_refresh
 * Description  : This routine is called from the GMCP alarm in order to cause
 *                the GMCP values to be updated.
 */
public void
gmcp_refresh()
{
    users()->gmcp_refresh();
}

/* 
 * Function name: init_gmcp
 * Description  : Called when the gamedriver is loaded.
 */
static void
init_gmcp()
{
    /* The alarm needed to cause the GMCP indicators of players to go up. */
    gmcp_alarm = set_alarm(GMCP_INTERVAL, GMCP_INTERVAL, gmcp_refresh);
}

/*
 * Function name: query_gmcp_token_user
 * Description  : Find out the player that belongs to a certain GMCP token.
 * Arguments    : string token - the token to verify.
 * Returns      : string - the name of the player, or "unknown".
 */
public string
query_gmcp_token_user(string token)
{
    string name;

    if (gmcp_tokens[token])
    {
        return gmcp_tokens[token];
    }
    foreach(object user: users())
    {
        name = user->query_real_name();
        if (GMCP_PLAYER_TOKEN(name, user->query_login_time()) == token)
        {
            gmcp_tokens[token] = name;
            return name;
        }        
    }
    return "unknown";
}

/*
 * Function name: query_gmcp_token
 * Description  : Find out the GMCP token for a player session. it buffers the
 *                tokens for use after the player quit.
 * Arguments    : object player - the player object.
 * Returns      : string - the token.
 */
static string
query_gmcp_token(object player)
{
    string token = GMCP_PLAYER_TOKEN(player->query_real_name(), player->query_login_time());

    gmcp_tokens[token] = player->query_real_name();
    
    return token;
}

/*
 * Function name: gmcp_external_discord_hello
 * Description  : Players client request discord information
 * Arguments    : object player - the player reading the file.
 *                mixed data - The directory to read. May have ~ notation.
 */
static void
gmcp_external_discord_get(object player, mixed data)
{
    mapping stat = ([
            "game": "Genesis",
            "state": "Playing",
            "partymax": 0,
            "largeimage": ({ "1024" })
    ]);

    player->catch_gmcp(GMCP_EXTERNAL_DISCORD_STATUS, stat);
}



/*
 * Function name: gmcp_external_discord_hello
 * Description  : Players client request discord information
 * Arguments    : object player - the player reading the file.
 *                mixed data - The directory to read. May have ~ notation.
 */
static void
gmcp_external_discord_hello(object player, mixed data)
{
    mapping info = ([
        "applicationid": "492768060130721792",
        "inviteurl": "https://discord.gg/V9djUuH"
    ]);
    player->catch_gmcp(GMCP_EXTERNAL_DISCORD_INFO, info);
}


/*
 * Function name: gmcp_get_dir
 * Description  : Reads a directory from the GMCP client.
 * Arguments    : object player - the player reading the file.
 *                mixed data - The directory to read. May have ~ notation.
 */
static void
gmcp_get_dir(object player, mixed data)
{
    string wname = player->query_real_name();
    string path;
    string *dirs;
    string *files;
    mapping result;

    if (!stringp(data))
    {
	return;
    }
    set_auth(this_object(), "root:" + wname);
    /* Ensure path is just a straight path. */
    path = FTPATH((string)player->query_path(), data);
    if (path[-1..] != "/")
	path += "/";

    /* Only allow wizards to proceed as we don't want to expose open dirs to
     * players. */
    if (!query_wiz_rank(wname) ||
        !valid_read(path, player, "read"))
    {
        player->catch_gmcp(GMCP_FILES_DIR_LIST, 0);
	return;
    }
    files = get_dir(path + "*");
    if (!sizeof(files))
    {
        player->catch_gmcp(GMCP_FILES_DIR_LIST, ([ GMCP_PATH : path ]) );
	return;
    }
    files -= ({ ".", ".." });
    dirs = filter(files, &operator(==)(, -2) @ &file_size() @ &operator(+)(path, ));
    files -= dirs;
    result = ([ GMCP_PATH : path, GMCP_DIRS : dirs, GMCP_FILES : ([ ]) ]);
    foreach(string file: files)
    {
	result[GMCP_FILES][file] = ([ GMCP_SIZE : file_size(path + file),
	    GMCP_TIME : file_time(path + file) ]);
    }
    player->catch_gmcp(GMCP_FILES_DIR_LIST, result);
}

/*
 * Function name: gmcp_read_file
 * Description  : Reads a file from the GMCP client.
 * Arguments    : object player - the player reading the file.
 *                mixed data - the filename to read. May have ~ notation.
 */
static void
gmcp_read_file(object player, mixed data)
{
    string wname = player->query_real_name();
    string path;

    if (!stringp(data))
    {
	return;
    }
    set_auth(this_object(), "root:" + wname);
    /* Ensure path is just a straight path. */
    path = FTPATH((string)player->query_path(), data);

    /* Only allow wizards to proceed as we don't want to expose open dirs to
     * players. */
    if (!query_wiz_rank(wname) ||
        !valid_read(path, player, "read"))
    {
	return;
    }
    player->catch_gmcp(GMCP_FILES_READ, ([ GMCP_PATH : path, GMCP_TEXT : read_file(path) ]) );
}

/*
 * Function name: gmcp_write_file
 * Description  : Writes a file from the GMCP client.
 * Arguments    : object player - the player reading the file.
 *                mixed data - the paramters.
 */
static void
gmcp_write_file(object player, mixed data)
{
    string wname = player->query_real_name();
    string path;
    int index = 1;

    /* Access failure. */
    if (!mappingp(data) ||
        !strlen(data[GMCP_PATH]) ||
	!strlen(data[GMCP_TEXT]))
    {
	return;
    }
    set_auth(this_object(), "root:" + wname);
    /* Ensure path is just a straight path. */
    path = FTPATH((string)player->query_path(), data[GMCP_PATH]);

    /* Only allow wizards to proceed, just for good measure. */
    if (!query_wiz_rank(wname) ||
        !valid_write(path, player, "write"))
    {
	return;
    }

    switch(file_size(path))
    {
    case -2:
        /* Don't overwrite a directory. */
        return;
    case -1:
        /* New file, this is good. */
        break;
    case 0:
        /* Overwrite empty file. */
        rm(path);
    default:
        /* Just for good measure, keep a backup of the file. */
        while(file_size(path + index) > 0)
        {
	    index++;
        }
        rename(path, path + index);
    }
    write_file(path, data[GMCP_TEXT]);
}

/*
 * Function name: process_gmcp
 * Description  : Used to process the actual gmcp message.
 * Arguments    : see incoming_gmcp().
 */
static void
process_gmcp(object player, string package, mixed data)
{
    switch(package)
    {
    case GMCP_CORE_HELLO:
        player->gmcp_hello(data);
        return;

    case GMCP_CORE_PING:
        player->catch_gmcp(GMCP_CORE_PING, 0);
        return;

    case GMCP_CORE_TOKEN:
        player->catch_gmcp(GMCP_CORE_TOKEN, query_gmcp_token(player));
        return;

    case GMCP_CORE_SUPPORTS_ADD:
    case GMCP_CORE_SUPPORTS_REMOVE:
    case GMCP_CORE_SUPPORTS_SET:
        player->set_gmcp(package, data);
        return;

    case GMCP_CORE_CLIENT:
        player->set_gmcp_client(data);
        return;

    case GMCP_CORE_OPTIONS:
        player->set_gmcp_options(data);
        return;

    case GMCP_CHAR_SKILLS_GET:
        CMD_LIVE_STATE->gmcp_char_skills_get(player, data);
        return;

    case GMCP_CHAR_VITALS_GET:
        CMD_LIVE_STATE->gmcp_char_vitals_get(player, data);
        return;

    case GMCP_FILES_DIR_GET:
        gmcp_get_dir(player, data);
        return;
    case GMCP_FILES_READ:
        gmcp_read_file(player, data);
	return;
    case GMCP_FILES_WRITE:
        gmcp_write_file(player, data);
	return;

    case GMCP_EXTERNAL_DISCORD_HELLO:
        gmcp_external_discord_hello(player, data);
        gmcp_external_discord_get(player, data);
        return;
    case GMCP_EXTERNAL_DISCORD_GET:
        gmcp_external_discord_get(player, data);
        return;
    }
}

/*
 * Function name: incoming_gmcp
 * Description  : This routine is called from the gamedriver upon receipt of
 *                an incoming gmcp request.
 * Arguments    : object player - the player who sent the gmcp command.
 *                string package - the message identifier / command.
 *                mixed data - the data (optional). Should be a mapping.
 */
static void
incoming_gmcp(object player, string package, mixed data)
{
    if (!objectp(player))
    {
        log_file(LOG_GMCP, ctime(time()) + " No player object.\n", LOG_SIZE_1M);
	return;
    }
    if (!strlen(package))
    {
        log_file(LOG_GMCP, ctime(time()) + " No string package.\n", LOG_SIZE_1M);
	return;
    }
    /* Package always has to be lower case. */
    package = lower_case(package);


    /* During login, all GMCP is buffered in the login object. */
    if (MASTER_OB(player) == LOGIN_OBJECT)
    {
        player->incoming_gmcp(package, data);
        return;
    }

    /* Log all incoming GMCP, but don't log the common used by GWC. */
    if (!IN_ARRAY(package, ({ GMCP_CORE_HELLO, GMCP_CORE_CLIENT, GMCP_CORE_OPTIONS, 
        GMCP_CHAR_VITALS_GET, GMCP_CORE_TOKEN, GMCP_CORE_SUPPORTS_SET }) ))
    {
        log_file(LOG_GMCP, ctime(time()) + " " + (interactive(player) 
            ? capitalize(player->query_real_name()) : "<none>") +
	    ": " + package + "\n", LOG_SIZE_1M);
    }

    /* Give wizards the debug information if they want it. */
    if (player->query_gmcp(GMCP_CORE_DEBUG))
    {
        set_this_player(player);
        write("Incoming GMCP: " + package + "\n");
        dump_array(data);
    }

    process_gmcp(player, package, data);
}

/*
 * Function name: buffered_gmcp
 * Description  : When GMCP is received before the player is logged in, it is
 *                buffered in the login player object. Once the login is done,
 *                it unbuffers via this routine to the static incoming_gmcp().
 * Arguments    : see incoming_gmcp()
 */
public void
buffered_gmcp(object player, string package, mixed data)
{
    string mfile = MASTER_OB(previous_object());

    /* Only accept calls from the login object. */
    if (IN_ARRAY(mfile, ({ LOGIN_OBJECT, LOGIN_NEW_PLAYER }) ))
    {
        incoming_gmcp(player, package, data);
    }
}
