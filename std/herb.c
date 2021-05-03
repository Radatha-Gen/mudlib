/*
 *  The standard herb.
 *
 *  The original made by Elessar Telcontar of Gondor,
 *		Genesis, April to July 1992.
 */

#pragma save_binary

inherit "/std/heap";
inherit "/lib/herb_support";

#include <cmdparse.h>
#include <composite.h>
#include <files.h>
#include <formulas.h>
#include <language.h>
#include <macros.h>
#include <ss_types.h>
#include <stdproperties.h>

#define EAT_VERB     ("eat")   /* the common ingestion verb of herbs */

/*
 * Variables
 */
static int     find_diff,      /* the difficulty of finding the herb */
               id_diff,        /* the difficulty of indentifications */
               food_amount,    /* the food amount in the herb object */
               herb_value,     /* the cost of herb in a copper coins */
               dried,          /* whether the herb is dried or not */
               dryable;        /* whether the herb can be dried */
static string  herb_name,      /* the unique herb name of the object */
               herb_id_name,   /* the additional name for the herb */
               herb_id_pname,  /* the plural form of the added name */
               *herb_id_adjs,  /* additional adjs for identified herbs */
               id_long,        /* the description of identified herb */
               unid_long,      /* the description of unidentif. herb */
               ingest_verb;    /* the verb used for a herb injesting */

/*
 * Function name: can_id_herb
 * Description  : This little function is called each time the herb is referred
 *                to by a player, to check if (s)he identifies it or not.
 * Arguments    : object player - The player, defaults to this_player()
 * Returns      : int - 1 for identification, else 0.
 */
varargs int
can_id_herb(object player = this_player())
{
    return F_CAN_ID_HERB(id_diff, player->query_skill(SS_HERBALISM));
}

varargs int
do_id_check(object player = this_player())
{
    return F_CAN_ID_HERB(id_diff, player->query_skill(SS_HERBALISM));
}

/*
 * Function name: long_description
 * Description  : This is an VBFC function for the set_long in the herb, which
 *                tests if the player examining it can identify it, before
 *                returning the appropriate long-description. To make this
 *                work, you must be sure to do set_id_long(str) and
 *                set_unid_long(str) from the create_herb() function.
 * Returns      : string - the long description.
 */
nomask string
long_description()
{
    return can_id_herb() ? id_long : unid_long;
}

/*
 * Function Name: parse_command_id_list
 * Description  : Used by parse_command to find the names of this item. It
 *                verifies whether the player can identify this herb.
 * Returns      : string * - An array of the names for this herb.
 */
public string *
parse_command_id_list()
{
    if (can_id_herb())
        return ::parse_command_id_list() + ({ herb_id_name });
    return ::parse_command_id_list();
}

/*
 * Function Name: parse_command_plural_id_list
 * Description  : Used by parse_command to find the names of this item. It
 *                verifies whether the player can identify this herb.
 * Returns      : string * - An array of the names for this herb.
 */
public string *
parse_command_plural_id_list()
{
    if (can_id_herb())
        return ::parse_command_plural_id_list() + ({ herb_id_pname });
    return ::parse_command_plural_id_list();
}

/*
 * Function Name: parse_command_adjectiv_id_list
 * Description  : Used by parse_command to find the adjectives of this item. It
 *                verifies whether the player can identify this herb.
 * Returns      : string * - An array of the adjectives for this herb.
 */
public string *
parse_command_adjectiv_id_list()
{
    if (can_id_herb())
        return ::parse_command_adjectiv_id_list() + herb_id_adjs;
    return ::parse_command_adjectiv_id_list();
}

/*
 * Function name: set_ingest_verb
 * Description  : Set what verb the player should type to be able to ingest.
 * 		  the herb. Default is "eat".
 * Arguments    : string str - The verb
 */
void
set_ingest_verb(string str)
{
    ingest_verb = str;
}

/*
 * Function name: query_ingest_verb
 * Description  : What verb is required to ingest this herb?
 * Returns      : string - The verb;

 */
string
query_ingest_verb()
{
    return ingest_verb;
}

/*
 * Function name: set_decay_time
 * Description:	  Set how long time it takes for the herb to decay.
 *                OBSOLETE! Retained only for backward compatibility.
 * Argumetns:	  i - the time (in seconds)
 */
void
set_decay_time(int i) { }

/*
 * Function name: set_dryable
 * Description:   Calling this function makes the herb dryable. Only works
 *                if the herb is not dried already.
 */
void
set_dryable()
{
    if (!dried) dryable = 1;
}

/*
 * Function name: query_dryable
 * Description  : Find out if this herb is dryable.
 * Returns      : int - 1 if it is dryable, else 0.
 */
int
query_dryable()
{
    return dryable;
}

/*
 * Function name: dry
 * Description:   This function is called when a herb dries, to allow
 *                different effects for dried herbs.
 */
void
dry() {}

/*
 * Function name: force_dry
 * Description:   Call this function if you want to make the herb
 *                dry after creation. E.g. if you have some tool that
 *                makes a herb dry. If you want to set the herb
 *                to dried at creation, use set_dried.
 */
void
force_dry()
{
    string pshort;

    if (dried || !dryable) return;

    dryable = 0;
    dried = 1;
    dry();
}

/*
 * Function name: set_dried
 * Description:   Set the herb to dried. Use this function in create_herb to
 *                generate a dried herb from the start. If you want to make
 *                the herb dry after creation, use force_dry.
 */
void
set_dried()
{
    if (!dried)
    {
	dried = 1;
	dryable = 0;
    }
}

/*
 * Function name: query_dried
 * Description  : Find out if the herb is dried.
 * Returns      : int - 1 if dried, else 0.
 */
int
query_dried()
{
    return dried;
}

/*
 * Function name: singular_short
 * Description  : This function will return the singular short descritpion
 *                for this heap.
 * Arguments    : object for_obj - who wants to know.
 * Returns      : string - the singular short description.
 */
public varargs string
singular_short(object for_obj)
{
    if (query_dried())
    {
        return "dried " + ::singular_short(for_obj);
    }
    return ::singular_short(for_obj);
}

/*
 * Function name: pluarl_short
 * Description  : This function will return the plural short descritpion
 *                for this heap.
 * Arguments    : object for_obj - who wants to know.
 * Returns      : string - the plural short description.
 */
public varargs string
plural_short(object for_obj)
{
    if (query_dried())
    {
        return "dried " + ::plural_short(for_obj);
    }
    return ::plural_short(for_obj);
}

/*
 * Function name: set_herb_name
 * Description:   Set the true name of the herb
 * Arguments:	  string str - The name
 */
void
set_herb_name(string str)
{
    mixed *parts = explode(str, " ");

    herb_name = str;
    herb_id_name = parts[-1];
    herb_id_pname = LANG_IS_PLURAL(herb_id_name) ? herb_id_name
        : LANG_PWORD(herb_id_name);
    herb_id_adjs = parts[..-2];
}

/*
 * Function name: query_herb_name
 * Description:   What is the true name of the herb
 * Returns:	  string - The name of the herb
 */
string
query_herb_name()
{
    return herb_name;
}

/*
 * Function name: set_id_long
 * Description:   Set the long description you see for identified herbs.
 * Arguments:     string str - The description
 */
void
set_id_long(string str)
{
    id_long = str;
}

/*
 * Function name: query_id_long
 * Description:   The long description if you can id the herb.
 * Returns:       string - The long description
 */
string
query_id_long()
{
    return id_long;
}

/*
 * Function name: set_unid_long
 * Description:   Set the long description you see if you cannot identify the
 *		  herb.
 * Arguments:     string str - The long description
 */
void
set_unid_long(string str)
{
    unid_long = str;
}

/*
 * Function name: query_unid_long
 * Description:   Query the long description you get if you cannot identify
 *		  the herb.
 * Returns:   	  string - The unidentified long description
 */
string
query_unid_long()
{
    return unid_long;
}

/*
 * Function name: set_id_diff
 * Description:   Set how hard it is to identify a herb. Valid range 1-100.
 * Arguments:     int i - The skill needed to know the herb
 */
void
set_id_diff(int i)
{
    id_diff = min(max(i, 1), 100);
}

/*
 * Function name: query_id_diff
 * Description:   How hard is it to identify this herb
 * Returns:  	  int - The difficulty
 */
int
query_id_diff()
{
    return id_diff;
}

/*
 * Function name: set_find_diff
 * Description:   Set how hard it is to find the herb
 * Arguments:     int i - Difficulty (suggested range is 0 - 10)
 */
void
set_find_diff(int i)
{
    find_diff = i;
}

/*
 * Function name: query_find_diff
 * Description:   How hard is it to find this herb
 * Returns: 	  int - The difficulty to find the herb in the nature.
 */
int
query_find_diff()
{
    return find_diff;
}

/*
 * Function name: set_herb_value
 * Description:   Set the value of the herb when dealing with a herb specialist.
 * Arguments:     int i - The value
 */
void
set_herb_value(int i)
{
    herb_value = i;
}

/*
 * Function name: query_herb_value
 * Description:   The value of the herb when dealing with a specialist.
 * Returns:	  The value
 */
int
query_herb_value()
{
    return herb_value;
}

/*
 * Function name: set_amount
 * Description:   Sets the amount of food in this herb (in grams)
 * Arguments:     int a: The amount of food.
 */
public void
set_amount(int a)
{
    food_amount = a;
    add_prop(OBJ_I_VOLUME, max(1, (a / 5)));
    add_prop(OBJ_I_WEIGHT, a);
}

/*
 * Function name: query_amount
 * Description:   Gives the amount of food in this herb
 * Returns:       int - Amount as int (in grams)
 */
public int
query_amount()
{
    return food_amount;
}

/*
 * Function name: ate_non_eat_herb
 * Description:   This function is called in stead of do_herb_effects
 *                if you eat a herb that isn't supposed to be eaten
 */
void
ate_non_eat_herb()
{
    write("You don't feel any effect.\n");
}

/*
 * Function name: create_herb
 * Description:   This is the create-function of the herb, which you should
 *                redefine and setup the herb from.
 */
void
create_herb()
{
}

/*
 * Function name: create_heap
 * Description:   Constructor. Do not redefine this routine to make your own
 *                herb. Instead define create_herb().
 */
nomask void
create_heap()
{
    set_name("herb");
    set_short("unknown herb");
    set_id_long("This herb has not been described by the wizard.\n");
    set_unid_long("This is an undescribed, unknown herb.\n");
    set_herb_name("xortion");
    set_amount(2);
    set_id_diff(20);
    set_find_diff(5);
    set_herb_value(10);
    set_ingest_verb(EAT_VERB);

    add_prop(OBJ_I_VALUE, 0);

    set_heap_size(1);
    create_herb();
    set_long(long_description);

    if (!query_prop(HEAP_S_UNIQUE_ID))
        add_prop(HEAP_S_UNIQUE_ID, MASTER + "_" + herb_name);
    add_prop(HEAP_I_UNIT_VALUE, 0);
}

/*
 * Function name: reset_heap
 * Description  : Reset routine. To reset this herb, define reset_herb() and
 *                call enable_reset() to start the reset functionality.
 */
nomask void
reset_heap()
{
    this_object()->reset_herb();
}

/*
 * Function name: ingest_access
 * Description  : Determine whether we can ingest this herb. Redefine this if
 *                you only want it to trigger on the special ingest verb.
 * Returns      : see command_eat()
 */
public mixed
ingest_access()
{
    return ((query_verb() == EAT_VERB) ||
        (query_verb() == query_ingest_verb()));
}

/*
 * Function name: command_eat
 * Description  : Eat the herb if we can stomach it.
 * Returns      : string - an error message (failure)
 *                1 - food successfully eaten.
 *                0 - access failure, default error message.
 */
public mixed
command_eat()
{
    mixed result = ingest_access();
    mixed err;

    if (!result || stringp(result))
    {
        return result;
    }

    if (err = this_player()->query_prop(LIVE_M_NO_INGEST))
    {
        if (strlen(err))
            return err;
        else
            return "You are unable to " + query_verb() + " anything.\n";
    }

    if (this_player()->query_prop(LIVE_I_HERB_EFFECT) > time() - F_HERB_INTERVAL)
    {
        return capitalize(LANG_THESHORT(this_object())) +
            " is too much for you.\n";
    }

    /* Eat only a single herb at a time. */
    split_heap(1);
    if (!this_player()->eat_food(query_amount()))
    {
        return capitalize(LANG_THESHORT(this_object())) +
            " is too much for you.\n";
    }

    force_heap_split();
    this_player()->add_prop(LIVE_I_HERB_EFFECT, time());
    return 1;
}

/*
 * Function name: do_ingest
 * Description  : When using a non-standard ingest verb, this routine links to
 *                the items soul for processing.
 * Arguments    : string str - the command line argument.
 * Returns      : int 1/0 - success/failure.
 */
int
do_ingest(string str)
{
    return CMD_LIVE_ITEMS->eat(str);
}

/*
 * Function name: init
 * Description  : Adds the ingest-action to the player.
 */
void
init()
{
    ::init();

    if (ingest_verb != EAT_VERB)
    {
        add_action(do_ingest, ingest_verb);
    }
}

/*
 * Function name: remove_food
 * Description  : Cause the destruction of this herb as part of the ingestion
 *                process.
 */
void
remove_food()
{
    /* Use the right ingest verb to get the desired effect. */
    if (query_verb() == query_ingest_verb())
    {
        do_herb_effects(num_heap());
    }
    else
    {
        ate_non_eat_herb();
    }

    remove_object();
}

/*
 * Function name: config_split
 * Description  : This is called before inserting this heap into the game
 *                It configures the split copy.
 * Arguments    : int new_num - the number to split off.
 *                object orig - the original object.
 */
void
config_split(int new_num, object orig)
{
    if (!new_num)
        return;

    ::config_split(new_num, orig);

    /* Set the long again because it's overwritten in config_split. */
    set_long(long_description);
    set_amount(orig->query_amount());

    if (orig->query_dried())
    {
        force_dry();
    }
    else
    {
        dried = 0;
        dryable = orig->query_dryable();
    }
}

/*
 * Function name: query_herb_recover
 * Description  : Return the recover strings for changing herb variables.
 * Returns      : string - the recover string for herbs.
 */
string
query_herb_recover()
{
    return "#h_dr#" + num_heap() + "#" + dried + "#";
}

/*
 * Function name: init_herb_recover
 * Description  : Initialize the herb variables during recovery.
 * Arguments    : string arg - the arguments.
 */
void
init_herb_recover(string arg)
{
    string foobar;
    int dr, num;

    if (sscanf(arg, "%s#h_dr#%d#%d#%s", foobar, num, dr, foobar) == 4)
    {
        set_heap_size(max(1, num));
        if (dr && !dried)
        {
	    force_dry();
        }
    }
}

/*
 * Function name: query_recover
 * Description  : Called to check whether this herb is recoverable. If you
 *                have variables you want to recover yourself, you have to
 *                redefine this function, keeping at least the filename and
 *                the herb recovery variables, like they are queried below.
 *                If, for some reason, you do not want your herb to
 *                recover, you should define the function and return 0.
 * Returns      : string - the default recovery string.
 */
string
query_recover()
{
    /* Don't recover if we're about to destruct. */
    if (query_prop(TEMP_OBJ_ABOUT_TO_DESTRUCT))
        return 0;

    return MASTER + ":" + query_herb_recover();
}

/*
 * Function name: init_recover
 * Description  : When the object recovers, this function is called to set
 *                the necessary variables. If you redefine the function,
 *                you must add a call to init_herb_recover with the string
 *                that you got after querying query_herb_recover or simply
 *                call ::init_recover(arg).
 * Arguments    : string arg - the arguments to parse
 */
void
init_recover(string arg)
{
    init_herb_recover(arg);
}
