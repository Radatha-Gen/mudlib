/*
 * /std/living/inventory.c
 *
 * This is a subpart of living.c containing routines related to inventory,
 * including wielding, wearing, holding, etc.
 */

#include <macros.h>
#include <stdproperties.h>
#include <ss_types.h>
#include <wa_types.h>
#include <composite.h>

/*
 * Global variables.
 */
static mapping tool_slots = ([]);  /* The object occupying a certain slot */ 
static private mapping m_worn = ([]);

/*
 * Function name:   max_weight
 * Description:	    Calculate the maximum weight this living can budge.
 * Returns:         The weight.
 */
int
max_weight()
{
    return query_prop(CONT_I_WEIGHT) + (query_stat(SS_STR) + 10) * 1000;
}

/*
 * Function name:   max_volume
 * Description:	    Calculate the maximum volume this living can budge.
 * Returns:         The volume.
 */
int
max_volume()
{
    return query_prop(CONT_I_VOLUME) + (query_stat(SS_STR) + 10) * 1000;
}

/*
 * Function name: carry_reset
 * Description  : This function is called to reset the properties that are
 *                related to carrying or being carried.
 */
static nomask void
carry_reset()
{
    add_prop(OBJ_I_NO_GET, 1);	/* Lifeforms can't be picked up */
    add_prop(CONT_I_MAX_WEIGHT, max_weight);
    add_prop(CONT_I_MAX_VOLUME, max_volume);
}

/*
 * Function name:   query_encumberance_weight
 * Description:     Calculate how encumbered we are in % in weight
 * Returns:         The number in %
 */
int
query_encumberance_weight()
{
    int cont;

    return (query_prop(OBJ_I_WEIGHT) - (cont = query_prop(CONT_I_WEIGHT))) *
		100 / (query_prop(CONT_I_MAX_WEIGHT) - cont);
}

/*
 * Function name:   query_encumberance_volume
 * Description:     Calculate how encumbered we are in % in volume
 * Returns:         The number in %
 */
int
query_encumberance_volume()
{
    int cont;

    return (query_prop(OBJ_I_VOLUME) - (cont = query_prop(CONT_I_VOLUME))) *
                100 / (query_prop(CONT_I_MAX_VOLUME) - cont);
}

/*
 * Function name: clear_tool_slots
 * Description:   remove all tool slots occupied by the given object
 * Arguments:     the object to look for
 * Returns:       1/0 - slots removed/no slots removed
 */
static nomask int
clear_tool_slots(object tool)
{
    int size = m_sizeof(tool_slots);
    tool_slots = filter(tool_slots, &operator(!=)(, tool));
    return (m_sizeof(tool_slots) != size);
}

/*
 * Function name: occupy_slot
 * Description:   Try to put the specified object in its slot(s)
 * Arguments:     The object to be put in the slot(s)
 * Returns:       1 - slots successfully occupied
 *                string - An error message
 */
nomask mixed
occupy_slot(object tool)
{
    int i, size, *slots;
    string name;

    slots = tool->query_slots(); 
    size = sizeof(slots);

    /*
     * Are all the slots it needs free?
     */
    i = -1;
    while (++i < size)
    {
        if (tool_slots[slots[i]])
	{
            name = tool_slots[slots[i]]->short();
            name = (strlen(name) ? "The " + name : "Something");
            return name + " is in the way.\n";
	}
    }

    i = -1;
    while(++i < size)
    {
        tool_slots[slots[i]] = tool;
    }
    
    return 1;
}

/*
 * Function name: empty_slot
 * Description:   Clear a given item's slots and move it out of its subloc
 * Arguments:     object tool - the object to look for
 * Returns:       1/0 - slots remove/slots not removed
 */
nomask int
empty_slot(object tool)
{
    if (!clear_tool_slots(tool))
    {
       return 0;
    }

    /* Move the tool from its subloc */
    if (environment(tool) == this_object())
    {
        tool->move(this_object());
    }

    return 1;
}

/*
 * Function name: query_tool
 * Description:   Get the item occupying a given slot or all items in tool slots
 * Arguments:     int slot - the slot to check or -1 for all items
 * Returns:       If a slot is given as an argument, the object occupying 
 *                the slot or 0.  If -1 is given as an argument, an array
 *                of objects.
 */
public mixed
query_tool(int slot)
{
    object *tools, *values, tool, tmp;
    int i;

    if (slot >= 0)
    {
        for (i = 1; i <= slot; i *= 2)
	{
            if (i & slot)
	    {
                tmp = tool_slots[i];
                if (!tmp || (tool && (tmp != tool)))
		{
                    return 0;
		}

                tool = tmp;
	    }
	}

        return tool;
    }

    tools = ({});
    values = m_values(tool_slots);
    for (i = 0; i < sizeof(values); i++)
    {
        tools |= ({ values[i] });
    }

    return tools;
}

/*
 * Function name: query_tool_map
 * Description:   Get the tool mapping, which maps each occupied slot to the
 *                item it contains
 * Returns:       The tool mapping
 */
public mapping
query_tool_map()
{
    return tool_slots + ([]);
}

/*
 * Function name: hold_reset
 * Description:   initialize holding routines
 */
nomask void
hold_reset()
{
    add_subloc(SUBLOC_HELD, this_object());
}

/*
 * Function name: hold
 * Description:   Hold an item
 * Arguments:     object tool - the item to hold
 * Returns:       1 - item successfully held
 *                string - An error message (item not held)
 */
public int
hold(object tool)
{
    mixed res;

    res = occupy_slot(tool);
  
    if (!stringp(res))
    {
        query_combat_object()->cb_modify_procuse();
        tool->move(this_object(), SUBLOC_HELD);
        return 1;
    }

    return res;
}

/*
 * Function name: release
 * Description:   Release a held item
 * Arguments:     object tool - the object to release
 */
public void
release(object tool)
{
    empty_slot(tool);
    query_combat_object()->cb_modify_procuse();
}

/*
 * Function name: show_held
 * Description:   Describe the items that are currently held
 * Arguments:     object ob - the onlooker
 * Returns:       The (string) description
 */
public string
show_held(object for_obj)
{
    mixed *a;
    int il, size;
    string str, p, pr;

    a = (object *)this_object()->query_tool(-1) - 
        (object *)this_object()->query_weapon(-1) -
        (object *)this_object()->query_armour(-1);

    if (!sizeof(a = filter(a, objectp)))
    {
        return "";
    }
    
    if (for_obj != this_object())
    {
        p = query_possessive();
        pr = capitalize(query_pronoun()) + " is";
    }
    else
    {
        p = "your";
        pr = "You are";
    }

    a = filter(map(a, &->query_hold_desc(for_obj)), stringp);
    /* Only use table form when displaying inventory. */
    if (for_obj->query_option(OPT_TABLE_INVENTORY) &&
        for_obj->query_prop(TEMP_SUBLOC_SHOW_ONLY_THINGS))
    {
        return HANGING_INDENT("Held    : " + COMPOSITE_WORDS(a), 10, 0);
    }
    else
    {
        str = pr + " holding " + COMPOSITE_WORDS(a) + ".";
        return HANGING_INDENT(str, 2, 0);
    }
}

/*
 * Function name: wear_reset
 * Description:   initialize the wear routines
 */
static nomask void
wear_reset()
{
    add_subloc(SUBLOC_WORNA, this_object());
}

/*
 * Function name: query_clothing
 * Description:   See what is worn on a particular location or get
 *                all items worn.
 * Arguments:     int slot - the location to check for clothing or -1
 *                           to get all clothing worn.
 * Returns:       (object *) all items worn on the specified location
 */
public object *
query_clothing(int slot)
{
    mixed *values;
    object *all_worn;
    int i;

    if (slot == -1)
    {
        all_worn = ({});
        values = m_values(m_worn);

        for (i = 0; i < sizeof(values); i++)
            all_worn |= values[i];

        return all_worn;
    }

    return (m_worn[slot] || ({}));
}

/*
 * Function name: show_worn
 * Description:   Display the items that are worn
 * Arguments:     object for_obj - the onlooker
 * Returns:       A string describing the worn items
 */
public string
show_worn(object for_obj)
{
    object *worn = query_clothing(-1);
    string str;
    
    if (!sizeof(worn))
        return "";

    /* Only use table form when displaying inventory. */
    if (for_obj->query_option(OPT_TABLE_INVENTORY) &&
        for_obj->query_prop(TEMP_SUBLOC_SHOW_ONLY_THINGS))
    {
        return HANGING_INDENT("Worn    : " +
            FO_COMPOSITE_ALL_DEAD(worn, for_obj), 10, 0);
    }
    else
    {
        str = ((for_obj == this_object()) ? "You are" : 
            capitalize(query_pronoun()) + " is") + " wearing " +  
            FO_COMPOSITE_ALL_DEAD(worn, for_obj) + ".";
        return HANGING_INDENT(str, 2, 0);
    }
}

/*
 * Function name: occupy_clothing_slots
 * Description:   Add a piece of clothing to the slots where it is worn
 * Arguments:     The clothing being added
 * Returns:       string - error message (clothing could not be added)
 *                1 - success (clothing added)
 */
public mixed
occupy_clothing_slots(object ob)
{
    int *slots, i;

    slots = ob->query_slots();

    for (i = 0; i < sizeof(slots); i++)
        if (!pointerp(m_worn[slots[i]]))
            m_worn[slots[i]] = ({ ob });
        else
            m_worn[slots[i]] += ({ ob });

    return 1;
}

/* 
 * Function name: clear_clothing_slots
 * Description:   remove a piece of clothing from the slots where it is worn
 * Arguments:     the clothing object being removed
 */
static void
clear_clothing_slots(object ob)
{
    int *slots, i;

    slots = m_indices(m_worn);
    for (i = 0; i < sizeof(slots); i++)
        if (!pointerp(m_worn[slots[i]]) ||
            !sizeof(m_worn[slots[i]] -= ({ ob })))
            m_delkey(m_worn, slots[i]);
}

/*
 * Function name: remove_clothing
 * Description:   Remove an item that has been worn
 * Arguments:     object ob - the item we wish to remove
 */
public void
remove_clothing(object ob)
{
    clear_clothing_slots(ob);

    /* We do this to remove the item from the worn-subloc. */
    if (environment(ob) == this_object())
        ob->move(this_object());
}

/*
 * Function name: wear_clothing
 * Description:   Wear something
 * Arguments:     object ob - the item to be worn
 * Returns:       string - an error message (the item cannot be worn)
 *                1 - item successfully worn
 */
public mixed
wear_clothing(object ob)
{
    int *slots, i;

    if (ob->move(this_object(), SUBLOC_WORN))
    {
        return "You cannot wear the " + ob->short(this_object()) +
            " for some reason.\n";
    }

    return occupy_clothing_slots(ob);
}

/*
 * Function name: wear_arm
 * Description:   wear an armour
 * Arguments:     The armour to be worn
 * Returns:       string - error message (armour could not be worn)
 *                1 - success (armour was worn)
 */
public mixed
wear_arm(object arm)
{
    mixed val;

    if (stringp(val = occupy_slot(arm)))
        return val;

    if (arm->move(this_object(), SUBLOC_WORNA))
    {
        clear_tool_slots(arm);
        return "You cannot wear the " + arm->short(this_object()) +
            " for some reason.\n";
    }

    if (stringp(val = occupy_clothing_slots(arm)))
    {
        arm->move(this_object());
        clear_tool_slots(arm);
        return val;
    }

    combat_reload();

    if (stringp(val = query_combat_object()->cb_wear_arm(arm)))
    {
        arm->move(this_object());
        clear_tool_slots(arm);
        clear_clothing_slots(arm);
        return val;
    }

    return 1;
}

/*
 * Function name:   remove_arm
 * Description:     Remove an armour
 * Arguments:       arm - The armour.
 */
public void
remove_arm(object arm)
{
    if (environment(arm) == this_object())
        arm->move(this_object());

    clear_clothing_slots(arm);
    clear_tool_slots(arm);

    /* No need for a combat reload */

    query_combat_object()->cb_remove_arm(arm);
}

static int
check_armour_object(object arm)
{
    return (arm->check_armour() && (arm->query_worn() == this_object()));
}

/*
 * Function name: query_armour
 * Description  : Returns the armour of a given position or lists all armours
 *                worn when -1 is given as argument.
 * Arguments    : int which - a numeric label describing an armour location.
 *                            On humanoids this is TS_HEAD etc. Give -1 to
 *                            list all.
 * Returns      : object   - the corresponding armour or 0.
 *                object * - all armours when -1 is given.
 */
varargs public mixed
query_armour(int which) 
{
    object arm;

    if (which == -1)
        return filter(query_tool(-1), check_armour_object);

    if ((arm = query_tool(which)) && check_armour_object(arm))
        return arm;

    return 0;
}

/*
 * Function name: wield_reset
 * Description:   initialize the wield routines
 */
static nomask void
wield_reset()
{
    add_subloc(SUBLOC_WIELD, this_object());
}

/*
 * Function name: show_wielded
 * Description:   Display the items that are worn
 * Arguments:     object for_obj - the onlooker
 * Returns:       A string describing the worn items
 */
public string
show_wielded(object for_obj)
{
    mixed *a;
    string str, p, pr;

    a = query_weapon(-1);

    if (!sizeof(a))
    {
        return "";
    }
    
    if (for_obj != this_object())
    {
        p = this_object()->query_possessive();
        pr = capitalize(this_object()->query_pronoun()) + " is";
    }
    else
    {
        p = "your";
        pr = "You are";
    }

    a = map(a, &->query_wield_desc(p)) - ({ 0 });

    /* Only use table form when displaying inventory. */
    if (for_obj->query_option(OPT_TABLE_INVENTORY) &&
        for_obj->query_prop(TEMP_SUBLOC_SHOW_ONLY_THINGS))
    {
        return HANGING_INDENT("Wielded : " + COMPOSITE_WORDS(a), 10, 0);
    }
    else
    {
        str = pr + " wielding " + COMPOSITE_WORDS(a) + ".";
        return HANGING_INDENT(str, 2, 0);
    }
}

/*
 * Function name: wield
 * Description:   Wield a weapon
 * Arguments:     The weapon to wield
 * Returns:       string - error message (weapon not wielded)
 *                1 - successs (weapon wielded)
 */
public mixed
wield(object weapon)
{
    mixed val;

    if (stringp(val = occupy_slot(weapon)))
    {
        return val;
    }

    if (weapon->move(this_object(), SUBLOC_WIELD))
    {
        clear_tool_slots(weapon);
        return "You cannot wield this weapon for some strange reason.\n";
    }

    combat_reload();

    if (stringp(val = query_combat_object()->cb_wield_weapon(weapon)))
    {
        clear_tool_slots(weapon);
        weapon->move(this_object());
        return val;
    }

    return 1;
}

/* 
 * Function name: unwield
 * Description:   Unwield a weapon
 * Arguments:     The weapon to unwield
 */
public void
unwield(object weapon)
{
    /* We do this to remove the weapon from the wielded-subloc. */
    if (environment(weapon) == this_object())
    {
        weapon->move(this_object());
    }

    empty_slot(weapon);

    /* No need for a combat reload */

    query_combat_object()->cb_unwield(weapon);
}

static int
check_weapon_object(object wep)
{
    return (wep->check_weapon());
}

/*
 * Function name: query_weapon
 * Description  : Returns the weapon held in a specified location or all the
 *                weapons the living wields when -1 is given as argument.
 * Arguments    : int which - a numeric label describing a weapon location.
 *                            On humanoids this is W_RIGHT etc. Give -1 to
 *                            list all weapons.
 * Returns      : object   - the corresponding weapon or 0.
 *                object * - all weapons in an array for which == -1.
 */
public mixed
query_weapon(int which)
{
    object weapon;

    if (which == -1)
    {
        return filter(query_tool(-1), check_weapon_object);
    }

    if ((weapon = query_tool(which)) &&
        (weapon->query_attack_id() == which) &&
        check_weapon_object(weapon))
    {
        return weapon;
    }

    return 0;
}
