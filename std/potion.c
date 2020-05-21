/*
 * /std/potion.c
 *
 * This is the standard object used for any form of potion.
 *
 * It works much the same way as herb.c.
 * Potions are quaffed btw ;-)
 *
 *      Nota bene:
 *      Potions do define a global variable alco_amount. Quaffing a
 *      potion, however, does not intoxicate. The variable alco_amount
 *      is kept for backwards compatibility. It can be queried using
 *      function query_alco_strength().
 */
#pragma save_binary
#pragma strict_types

inherit "/std/heap";
inherit "/lib/herb_support";

#include <cmdparse.h>
#include <composite.h>
#include <files.h>
#include <formulas.h>
#include <macros.h>
#include <ss_types.h>
#include <stdproperties.h>

#define QUAFF_VERB     ("quaff")   /* the common ingestion verb of potions */

int	id_diff,	  /* How difficult is it to id the potion? */
        magic_resistance, /* How strong is the resistance against magic */
        soft_amount,	  /* the soft amount of the potion */
        alco_amount,	  /* the alco value of the potion */
        potion_value,	  /* the cost of potion in a copper coins */
        identified = 0;
object *gFail;		  /* needed for a backwards compability */
string	quaff_verb,	  /* the verb used for potion ingesting */
        potion_name,	  /* the unique name of the potion */
        potion_pname,     /* the plural name of the potion */
        id_long,	  /* the description of identified potion */
        unid_long,	  /* the description of unidentified potion */
        id_smell,	  /* the smell of the identified potion */
        unid_smell,	  /* the smell of the unidentified potion */
        id_taste,	  /* the taste of the identified potion */
        unid_taste;	  /* the taste of the unidentified potion */

/*
 * Function name: can_id_potion
 * Description:   This little function is called each time the potion is
 *                referred to by a player, to check if (s)he identifies it
 *                or not.
 * Arguments:	  object player - The player
 * Returns:       int - 1 for identification, else 0.
 */
varargs int
can_id_potion(object player = this_player())
{
    if (F_CAN_ID_POTION(id_diff, player->query_skill(SS_ALCHEMY)))
        return 1;

    string *id_names = player->query_prop(LIVE_AS_POTION_ID);

    return IN_ARRAY(potion_name, id_names);
}

/*
 * Function name: long_description
 * Description:   This is an VBFC function for the set_long in the
 *                potion, which tests if the player examining it can
 *                identify it.
 *                Make sure to set_id_long(str) and set_unid_long(str)
 *                from the create_potion() function.
 */
nomask string
long_description()
{
    if (can_id_potion(this_player()))
        return id_long;
    else
        return unid_long;
}

/*
 * Function name: set_quaff_verb
 * Description:   Set what verb the player should type to be able to
 * 		  quaff the potion. Default is "quaff".
 *                "drink" is not possible.
 * Arguments:     string str - The verb
 */
void
set_quaff_verb(string str)
{
    if (str != "drink")
        quaff_verb = str;
}

/*
 * Function name: query_quaff_verb
 * Description:   What verb is required to quaff this potion?
 * Returns:	  string - The verb;
 */
string
query_quaff_verb() { return quaff_verb; }

/*
 * Function name: set_potion_name
 * Description:   Set the true name of the potion
 * Arguments:	  string str - The name
 */
void
set_potion_name(string str)
{
    potion_name = str;
    potion_pname = LANG_PWORD(str);
}

/*
 * Function name: query_potion_name
 * Description:   What is the true name of the potion
 * Returns:	  string - The name of the potion
 */
string
query_potion_name() { return potion_name; }

/*
 * Function name: set_id_long
 * Description:   Set the long description you see if you know
 *                what potion it is.
 * Arguments:     string str - The description
 */
void
set_id_long(string str) { id_long = str; }

/*
 * Function name: query_id_long
 * Description:   The long description if you can id the potion
 * Returns:       string - The long description
 */
string
query_id_long() { return id_long; }

/*
 * Function name: set_unid_long
 * Description:   Set the long description you see if you cannot identify the
 *		  potion.
 * Arguments:     string str - The long description
 */
void
set_unid_long(string str) { unid_long = str; }

/*
 * Function name: query_unid_long
 * Description:   Query the long description you get if you cannot identify
 *		  the potion.
 * Returns:   	  string - The unidentified long description
 */
string
query_unid_long() { return unid_long; }

/*
 * Function name: set_id_taste
 * Description:   Set the message when you taste the identified potion
 * Arguments:     string str - The message
 */
void
set_id_taste(string str) { id_taste = str; }

/*
 * Function name: query_id_taste
 * Description:   Query the message you get if you taste and identify
 *                the potion.
 * Returns:   	  string - The taste of the identified potion
 */
string
query_id_taste() { return id_taste; }

/*
 * Function name: set_id_smell
 * Description:   Set the message when you smell the identified potion
 * Arguments:     string str - The message
 */
void
set_id_smell(string str) { id_smell = str; }

/*
 * Function name: query_id_smell
 * Description:   Query the message you get if you smell and identify
 *                the potion.
 * Returns:   	  string - The smell of the identified potion
 */
string
query_id_smell() { return id_smell; }

/*
 * Function name: set_unid_taste
 * Description:   Set the message when you taste the unidentified potion
 * Arguments:     string str - The message
 */
void
set_unid_taste(string str) { unid_taste = str; }

/*
 * Function name: query_unid_taste
 * Description:   Query the message you get if you taste and do not
 *                identify the potion.
 * Returns:   	  string - The taste of the unidentified potion
 */
string
query_unid_taste() { return unid_taste; }

/*
 * Function name: set_unid_smell
 * Description:   Set the message when you smell the unidentified potion
 * Arguments:     string str - The message
 */
void
set_unid_smell(string str) { unid_smell = str; }

/*
 * Function name: query_unid_smell
 * Description:   Query the message you get if you smell and do not
 *                identify the potion.
 * Returns:   	  string - The smell of the unidentified potion
 */
string
query_unid_smell() { return unid_smell; }

/*
 * Function name: set_id_diff
 * Description:   Set how hard it is to identify a potion
 * Arguments:     int i - The skill needed to know the potion
 */
void
set_id_diff(int i) { id_diff = i; }

/*
 * Function name: query_id_diff
 * Description:   How hard is it to identify this potion
 * Returns:  	  int - The difficulty
 */
int
query_id_diff() { return id_diff; }

/*
/*
 * Function name: set_soft_amount
 * Description:   Set the soft amount of the potion
 * Argument:      int i - the soft amount
 */
void
set_soft_amount(int i)
{
    mark_state();
    soft_amount = max(i, 10);

    add_prop(OBJ_I_VOLUME, soft_amount);
    add_prop(OBJ_I_WEIGHT, soft_amount);
    update_state();
}

/*
 * Function name: query_soft_amount
 * Description:   What is the soft amount of the potion
 * Returns:       int - The soft amount
 */
int
query_soft_amount() { return soft_amount; }

/*
 * Function name: set_alco_amount
 * Description:   Set the alco amount of the potion
 * Argument:      int i - the alco amount
 */
void
set_alco_amount(int i)
{
    alco_amount = i;
    /* Somehow various potions lose their strength.
     * Let's see if keeping heaps separated helps. */
    add_prop(HEAP_S_UNIQUE_ID, MASTER + "_" + potion_name + "_" + i);
}

/*
 * Function name: query_alco_amount
 * Description:   Quaffing potions does not intoxicate.
 *                This is achieved by making this function return 0.
 *                To query the alcohol amount in the potion use
 *                query_alco_strength().
 * Returns:       int - 0
 */
nomask int
query_alco_amount() { return 0; }

/*
 * Function name: query_alco_strength
 * Description:   What is the alco amount of the potion?
 * Returns:       int - The alco amount
 */
int
query_alco_strength() { return alco_amount; }

/*
 * Function name: set_potion_value
 * Description:   Set the value of the potion when dealing with
 *                a potion specialist
 * Arguments:     int i - The value
 */
void
set_potion_value(int i) { potion_value = i; }

/*
 * Function name: query_potion_value
 * Description:   The value of the potion when dealing with a specialist
 * Returns:	  int - The value
 */
int
query_potion_value() { return potion_value; }

/*
 * Function name: query_identified
 * Description:   Did you identify the potion?
 * Returns:       int - True if identified
 */
int
query_identified() { return identified; }

/*
 * Function name: set_identified
 * Description:   Did you identify the potion?
 * Returns:       int - True if identified
 */
varargs void
set_identified(int i = 1) { identified = i; }

/*
 * Function name: create_potion
 * Description:   This is the create-function of the potion, which you should
 *                redefine and setup the herb from.
 */
public void
create_potion()
{
}

/*
 * Function name: create_heap
 * Description:   Constructor. Do not redefine this routine to make your own
 *                potion. Instead define create_potion().
 */
public nomask void
create_heap()
{
    set_name("potion");
    set_short("unknown potion");
    set_id_long("This potion has not been described by the wizard.\n");
    set_unid_long("This is an undescribed, unknown potion.\n");
    set_unid_smell("The potion seems to be without smell.\n");
    set_id_smell("The potion is without smell.\n");
    set_unid_taste("The potion seems to be tasteless.\n");
    set_id_taste("The potion is tasteless.\n");
    set_potion_name("xortion");
    set_soft_amount(50);
    set_alco_amount(0);
    set_id_diff(20);
    set_potion_value(10);
    set_quaff_verb(QUAFF_VERB);

    add_prop(OBJ_I_VALUE, 0);

    set_heap_size(1);
    create_potion();
    set_long(long_description);

    if (!query_prop(HEAP_S_UNIQUE_ID))
        add_prop(HEAP_S_UNIQUE_ID, MASTER + "_" + potion_name);
    add_prop(HEAP_I_UNIT_VALUE, 0);
}

/*
 * Function name: reset_potion
 * Description  : Reset routine. To reset this potion, define reset_potion()
 *                and call enable_reset() to start the reset functionality.
 */
public nomask void
reset_heap()
{
    this_object()->reset_potion();
}

/*
 * Function name: consume_text
 * Description  : Called when the player eats one or more herbs.
 * Arguments    : object *arr - the herbs eaten.
 *                string verb - the verb used for eating.
 */
void
consume_text(object *arr, string verb)
{
    string str;

    write("You " + verb + " " + (str = COMPOSITE_DEAD(arr)) + ".\n");
    say(QCTNAME(this_player()) + " " + verb + "s " + str + ".\n");
}

/*
 * Function name: quaff_it
 * Description:	  Quaffs the objects described as parameter to the quaff_verb.
 *                It uses command parsing to find which objects to quaff.
 * Arguments:	  string str: The trailing command after 'quaff_verb ...'
 * Returns:	  int - True if command successful
 */
public int
quaff_it(string str)
{
    object *potions;
    string  verb = query_verb();;

    if (this_player()->query_prop(TEMP_STDPOTION_CHECKED) ||
	query_prop(TEMP_OBJ_ABOUT_TO_DESTRUCT))
    {
        return 0;
    }

    /* It's impossible to quaff all potions at once. */
    if (str == "all")
    {
        notify_fail("You must specify which potion to " + verb + ".\n");
        return 0;
    }

    if (!strlen(str))
    {
        notify_fail(capitalize(verb) + " what?\n", 0);
        return 0;
    }

    gFail = ({ });
    potions = CMDPARSE_ONE_ITEM(str, "quaff_one_thing", "quaff_access");
    if (sizeof(potions) > 0)
    {
        consume_text(potions, verb);
        potions->destruct_object();
        return 1;
    }
    else
    {
	    set_alarm(1.0, 0.0, &(this_player())->remove_prop(TEMP_STDPOTION_CHECKED));
        this_player()->add_prop(TEMP_STDPOTION_CHECKED, 1);
        if (sizeof(gFail))
            notify_fail("@@quaff_fail:" + file_name(this_object()));
        return 0;
    }
}

/*
 * Function name: ingest_fail
 * Description  : Composes the failure message when you fail to eat something.
 * Returns      : string - the composite failure message.
 */
string
quaff_fail()
{
    string str = "You try to " + quaff_verb + " " + COMPOSITE_DEAD(gFail) +
        " but fail.\n";
    say(QCTNAME(this_player()) + " tries to " + quaff_verb + " " +
        QCOMPDEAD + " but fails.\n");
    this_player()->remove_prop(TEMP_STDPOTION_CHECKED);
    return str;
}

/*
 * Function name: quaff_access
 * Description  : Find out whether you can (and want to) quaff this.
 * Arguments    : object ob - the object to try.
 * Returns      : int 1/0 - if true, you can (and want to) quaff this.
 */
int
quaff_access(object ob)
{
    string verb = query_verb();

    /* Return true if all this is true. */
    return (environment(ob) == this_player()) &&
        IS_POTION_OBJECT(ob) &&
        (verb == ob->query_quaff_verb() || verb == QUAFF_VERB) &&
        !ob->query_prop(TEMP_OBJ_ABOUT_TO_DESTRUCT);
}

/*
 * Function name: quaff_one_thing
 * Description  : Actually try to quaff this one herb.
 * Arguments    : object ob - the object to try.
 * Returns      : int 1/0 - if true, you can actually ate it.
 */
int
quaff_one_thing(object ob)
{
    int    index;
    int    soft = ob->query_soft_amount();
    int    alco = ob->query_alco_amount();
    int    size = ob->num_heap();
    string verb = query_verb();

    if (verb == QUAFF_VERB)
    {
        /* We "drink" one potion at a time to allow for non-linear formulae. */
        for (index = 1; index <= size; index++)
        {
            if (!this_player()->drink_soft(soft))
            {
                /* If we fail on the first, you cannot drink it. */
                if (index == 1)
                {
                    ob->split_heap(1);
	                write(capitalize(ob->short()) + " is too much for you.\n");
                    gFail += ({ ob });
	                return 0;
                }
                ob->split_heap(index - 1);
                break;
	        }
            if (!this_player()->drink_alco(alco))
            {
                this_player()->drink_soft(-soft);
                /* If we fail on the first, you cannot stomach it. */
                if (index == 1)
                {
                    ob->split_heap(1);
	                write(capitalize(ob->short()) + " is too strong for you.\n");
                    gFail += ({ ob });
	                return 0;
                }
                ob->split_heap(index - 1);
                break;
    	    }
       }
       ob->force_heap_split();
    }
    return 1;
}

/*
 * Function name: destruct_object
 * Description  : Invokes the herb effects, clones an empty vial and removes
 *                the potion if nothing is left behind.
 */
void
destruct_object()
{
    do_herb_effects(num_heap());

    seteuid(getuid());
    clone_object(POTION_VIAL_OBJECT)->move(environment(this_object()), 1);
    remove_object();
}

/*
 * Function name: set_magic_res
 * Description:   Set how resistance this potion is agains magic / how easy it
 *		  is to dispel it.
 * Arguments:     int resistance - the resistance
 */
void set_magic_res(int resistance) { magic_resistance = resistance; }

/*
 * Function name: query_magic_resistance
 * Description:   Query the magic resistance
 * Returns:       int - How resistive the potion is
 */
int
query_magic_res(string prop)
{
    if (prop == MAGIC_I_RES_MAGIC)
        return magic_resistance;
    else
        return 0;
}

/*
 * Function name: dispel_magic
 * Description:   Function called by a dispel spell
 * Argument:	  int magic - How strong the dispel is
 * Returns:	  int 0 - No dispelling, 1 - Object dispelled
 */
int
dispel_magic(int magic)
{
    object env = environment(this_object());

    if (magic < query_magic_res(MAGIC_I_RES_MAGIC))
        return 0;

    if (living(env))
    {
        tell_room(environment(env), "The " + QSHORT(this_object()) +
            " held by " + QTNAME(env) + " glows white and explodes!\n", env);
        env->catch_msg("The " + QSHORT(this_object()) + " that you hold " +
            " glows white and explodes!\n");
    }
    else if (env->query_prop(ROOM_I_IS))
        tell_room(env, "The " + QSHORT(this_object()) +
            " glows white and explodes!\n");

    return 1;
}

/*
 * Function name: smell_access
 * Description  : Find out whether you can (and want to) smell this.
 * Arguments    : object ob - the object to try.
 * Returns      : int 1/0 - if true, you can (and want to) smell this.
 */
int
smell_access(object ob)
{
    string verb = query_verb();

    return (environment(ob) == this_player()) && IS_POTION_OBJECT(ob)
        && (verb == "taste" || verb == "smell");
}

/*
 * Function name: smell_one_thing
 * Description  : Actually try to smell this one potion. This handled both
 *                smelling and teasting.
 * Arguments    : object ob - the object to try.
 * Returns      : int 1/0 - if true, you can actually smelled it.
 */
int
smell_one_thing(object ob)
{
    object  pl = this_player();
    string  verb = query_verb();

    if (ob->query_identified() ||
        (ob->query_id_diff() <= (pl->query_skill(SS_ALCHEMY) * 150 +
         pl->query_skill(SS_HERBALISM) * 50) / 100))
    {
        ob->set_identified(1);
        pl->add_prop(LIVE_AS_POTION_ID,
            (pl->query_prop(LIVE_AS_POTION_ID) || ({ })) | ({ potion_name }));

        if (verb == "taste")
            write(ob->query_id_taste());
        else if (verb == "smell")
            write(ob->query_id_smell());
    }
    else
    {
        if (verb == "taste")
            write(ob->query_unid_taste());
        else if (verb == "smell")
            write(ob->query_unid_smell());
    }

    return 1;
}

/*
 * Function name: smell_it
 * Description:   Smell the potion, but do not quaff it
 *                If smelling should call do_herb_effects set
 *                quaff_verb to "smell"
 * Arguments:     str - the argument of the command
 * Returns:       True is successful
 */
int
smell_it(string str)
{
    object *potions;
    string  verb = query_verb();

    if (this_player()->query_prop(TEMP_STDPOTION_CHECKED) ||
	query_prop(TEMP_OBJ_ABOUT_TO_DESTRUCT))
    {
        return 0;
    }

    /* It's impossible to smell all potions at once. */
    if (str == "all")
    {
        notify_fail("You must specify which potion to " + verb + ".\n");
        return 0;
    }

    if (!strlen(str))
    {
        notify_fail(capitalize(verb) + " what?\n");
	return 0;
    }

    potions = CMDPARSE_ONE_ITEM(str, "smell_one_thing", "smell_access");
    if (sizeof(potions) > 0)
    {
        say(QCTNAME(this_player()) + " " + verb + "s " +
            COMPOSITE_DEAD(potions) + ".\n");
        return 1;
    }
    else
    {
	    set_alarm(1.0, 0.0, &(this_player())->remove_prop(TEMP_STDPOTION_CHECKED));
        this_player()->add_prop(TEMP_STDPOTION_CHECKED, 1);
        notify_fail(capitalize(verb) + " what?\n");
        return 0;
    }
}

/*
 * Function name: taste_it
 * Description:   Taste the potion, but do not quaff it
 *                If tasting should call do_herb_effects set
 *                quaff_verb to "taste"
 * Arguments:     str - the argument of the command
 * Returns:       True is successful
 */
int
taste_it(string str)
{
    return smell_it(str);
}

/*
 * Function name: init
 * Description:   adds the quaff-action to the player
 */
void
init()
{
    ::init();

    add_action(quaff_it, quaff_verb);
    if (quaff_verb != QUAFF_VERB)
        add_action(quaff_it, QUAFF_VERB);
    if (quaff_verb != "taste")
        add_action(taste_it, "taste");
    if (quaff_verb != "smell")
        add_action(smell_it, "smell");
}

/*
 * Function name: config_merge
 * Description  : This is called when a heap merges with another heap. It
 *                is called in the parent heap where the child gets merged
 *                into. It is called before the heap size is adjusted.
 * Arguments    : object child - the child that gets merged into us.
 */
void
config_merge(object child)
{
    int temp;
    int count = num_heap() + child->num_heap();

    ::config_merge(child);

    /* Calculate the average soft amount, with rounding. */
    temp = (soft_amount * num_heap()) + (child->num_heap() * child->query_soft_amount()) + (count / 2);
    set_soft_amount(temp / count);
    /* Calculate the average alco amount, with rounding. */
    temp = (alco_amount * num_heap()) + (child->num_heap() * child->query_alco_strength()) + (count / 2);
    set_alco_amount(temp / count);

    set_identified(identified || child->query_identified());
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

    set_alco_amount(orig->query_alco_strength());
    set_soft_amount(orig->query_soft_amount());
    set_identified(orig->query_identified());
}

/*
 * Function name: stat_object
 * Description:   This function is called when a wizard wants to get more
 *                information about an object.
 * Returns:       str - The string to write..
 */
string
stat_object()
{
    string str = ::stat_object();

    return str + "Soft: " + query_soft_amount() + " \t" + "Alco: " +
        query_alco_strength() + "\n";
}

/*
 * Function name: query_potion_recover
 * Description  : Return the recover strings for changing potion variables.
 * Returns      : string - the potion recovery info.
 */
string
query_potion_recover()
{
    return "#POT#" + num_heap() + "#" + alco_amount + "#" + soft_amount + "#";
}

/*
 * Function name: init_potion_recover
 * Description  : Initialize the potion variables at recover.
 * Arguments    : string arg - the argument
 */
void
init_potion_recover(string arg)
{
    string foobar;
    int soft, alco, num;

    if (sscanf(arg, "%s#POT#%d#%d#%d#%s", foobar, num, alco, soft, foobar) == 5)
    {
        set_heap_size(max(1, num));
        set_soft_amount(soft);
        set_alco_amount(alco);
    }
}

/*
 * Function name: query_recover
 * Description  : Called to check whether this potion is recoverable. If you
 *                have variables you want to recover yourself, you have to
 *                redefine this function, keeping at least the filename and
 *                the potion recovery variables, like they are queried below.
 *                If, for some reason, you do not want your potion to
 *                recover, you should define the function and return 0.
 * Returns      : string - the default recovery string.
 */
string
query_recover()
{
    /* Don't recover if we're about to destruct. */
    if (query_prop(TEMP_OBJ_ABOUT_TO_DESTRUCT))
        return 0;

    return MASTER + ":" + query_potion_recover();
}

/*
 * Function name: init_recover
 * Description  : When the object recovers, this function is called to set
 *                the necessary variables. If you redefine the function,
 *                you must add a call to init_potion_recover with the string
 *                that you got after querying query_potion_recover or simply
 *                call ::init_recover(arg).
 * Arguments    : string arg - the arguments to parse
 */
void
init_recover(string arg)
{
    init_potion_recover(arg);
}

/*
 * Function Name: parse_command_id_list
 * Description  : Used by parse_command to find the names of this item. It
 *                verifies whether the player can identify this potion.
 * Returns      : string * - An array of the names for this potion.
 */
public string *
parse_command_id_list()
{
    if (can_id_potion())
        return ::parse_command_id_list() + ({ potion_name });
    else
        return ::parse_command_id_list();
}

/*
 * Function Name: parse_command_plural_id_list
 * Description  : Used by parse_command to find the names of this item. It
 *                verifies whether the player can identify this potion.
 * Returns      : string * - An array of the names for this potion.
 */
public string *
parse_command_plural_id_list()
{
    if (can_id_potion())
        return ::parse_command_plural_id_list() + ({ potion_pname });
    else
        return ::parse_command_plural_id_list();
}

