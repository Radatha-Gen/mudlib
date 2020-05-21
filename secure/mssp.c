/* 
 * Gathers the data required for the MSSP protocol.
 *
 * Refer to: http://tintin.sourceforge.net/mssp/ for details.
 */
#include <files.h>

static mapping mssp = ([ ]);

#define START   "MSSP-REPLY-START"
#define END     "MSSP-REPLY-END"

void
create()
{
    setuid();
    seteuid(getuid());

    mssp["NAME"] = "Genesis";
    mssp["HOSTNAME"] = "mud.genesismud.org";
    mssp["PORT"] = "3011";
    mssp["CRAWL DELAY"] = "-1";
    mssp["CREATED"] = "1989";
    mssp["LANGUAGE"] = "English";
    mssp["WEBSITE"] = "https://www.genesismud.org/";
    mssp["MINIMUM AGE"] = "0";
    mssp["LOCATION"] = "Sweden";
    mssp["CONTACT"] = "info@genesismud.org";
    mssp["CODEBASE"] = SECURITY->do_debug("version")[0..7]; 

    mssp["FAMILY"] = "LPMud";
    mssp["GENRE"] = "Fantasy";
    mssp["GAMEPLAY"] = ({ "Roleplaying", "Adventure", "Hack and Slash", "Player versus Player" });
    mssp["STATUS"] = "Live";
    mssp["GMCP"] = "1";
    mssp["UPTIME"] = SECURITY->query_start_time();
}

mapping
mssp_data(object ob)
{
    mssp["PLAYERS"] = sizeof(users());
    return mssp;
}


string
mssp_request(object ob)
{
    string text = "\n";
    text += START + "\n";
    foreach (string key, mixed values: mssp_data(ob)) 
    {
        if (!pointerp(values)) {
            values = ({ values });
        }

        foreach (string value: values) 
        {
            text += key + "\t" + value + "\n";
        }
    }
    text += END + "\n\n";
    return text;
}

