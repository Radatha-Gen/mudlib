/*
 * A standard object that adds resistance to its carrier
 */
 
#pragma save_binary
#pragma strict_types

inherit "/std/object";

#include <macros.h>
#include <ss_types.h>
#include <stdproperties.h>

//	Prototypes
public void stop_res();
public string query_res_type();
public string query_subloc_description(object on, object for_obj);

//	Global variables
mixed	Res_Type;	/* The type of resistance */
int	Alarm_Id,	/* The alarm id of the alarm to end the effect */
        Strength;	/* How strong it will be */
float   Time;	/* How long time it will be in effect, if 0.0 always. */

/*
 * Function name: create_resistance
 * Description:   Create the resistance
 */
void
create_resistance()
{
    set_name("Resistance_Object");
    add_prop(OBJ_M_NO_DROP, "Drop what?\n");
    add_prop(OBJ_I_WEIGHT, 0);
    add_prop(OBJ_I_VOLUME, 0);
    add_prop(OBJ_I_VALUE, 0);
    set_no_show();
}

string
query_subloc_name() 
{
    return "_subloc_resistance" + query_res_type();
}

/*
 * Function name: create_object
 * Description:   The standard create
 */
nomask void
create_object()
{
    Strength = 5;
    Res_Type = MAGIC_I_RES_MAGIC;
    Time = 0.0;
    create_resistance();
}

/*
 * Function name: enter_env
 * Description:   Called each time this object enters another environment
 * Arguments:     to   - The object this object enters
 *		  from - The object this object leaves
 */
void
enter_env(object to, object from)
{
    if (objectp(to) && living(to))
    {
        to->add_magic_effect(this_object());
        if (Time && (!Alarm_Id || !sizeof(get_alarm(Alarm_Id))))
            Alarm_Id = set_alarm(Time, 0.0, stop_res);
    }

    ::enter_env(to, from);

    if (interactive(to))
    {
        to->add_subloc(query_subloc_name(), this_object());        
    }
}

public void
move_subloc_owner(object player, object current_subloc_owner)
{
    object * others = filter(all_inventory(player), &operator(!=)(0) @ &->query_magic_protection(query_res_type(), player)) - ({ current_subloc_owner });
    int bMovedSublocOwner = 0;
    if (sizeof(others) > 0) 
    {
        foreach (object other : others)
        {
            if (strlen(other->show_subloc(query_subloc_name(), player, player)))
            {
                // We re-assign the subloc to another resistance object of the same type
                player->add_subloc(query_subloc_name(), other);
                bMovedSublocOwner = 1;
                break;
            }
        }
    } 
    if (!bMovedSublocOwner)
    {
        player->remove_subloc(query_subloc_name());
    }
}

/*
 * Function name: leave_env
 * Description:   Called when this object is moved from another object
 * Arguments:     from - The object this object is moved from
 *		  to   - The object to which this object is being moved
 */
void
leave_env(object from, object to)
{
    if (objectp(from) && living(from))
      	from->remove_magic_effect(this_object());
    
    ::leave_env(from, to);

    if (interactive(from))
    {
        object subloc_obj = from->query_subloc_obj(query_subloc_name());
        if (subloc_obj == this_object())
        {
            move_subloc_owner(from, subloc_obj);            
        }
    }
}

/*
 * Function name: stop_res
 * Description:   If the resistance is time dependent this function will
 *		  called when it's time to remove this resistance
 */
public void
stop_res()
{
    object ob;

    if (objectp(ob = environment(this_object())) && living(ob))
        tell_object(ob, "You feel less resistant.\n");

    remove_alarm(Alarm_Id);
    remove_object();
}

/*
 * Function name: set_res_type
 * Description:   Set the type of resistance of this object
 */
void set_res_type(string str) { Res_Type = str; }

/*
 * Function name: query_res_type
 * Description:   Returns what type of resistance this object helps
 */
string query_res_type() { return Res_Type; }

/*
 * Function name: set_strength
 * Description:   Set how strong this resistance is
 */
void set_strength(int i) { Strength = i; }

/*
 * Function name: query_strength
 * Description:   Query how strong this resistance is
 */
int query_strength() { return Strength; }

/*
 * Function name: set_time
 * Description:   How long this will hold
 */
void set_time(int i) { Time = itof(i); }

/*
 * Function name: query_time
 * Description:   How long will we be resistive
 */
int query_time() { return ftoi(Time); }

/*
 * Function name: stat_object
 * Description:   Called when wizard stats the object
 * Returns:       A string describing the object.
 */
public string
stat_object()
{
    mixed   a_info;
    string  desc = ::stat_object();

    desc += "Resistance: " + Res_Type + "\n"
         +  "Strength:   " + Strength + "\n"
         +  "Time:       " + ftoa(Time) + "\n";

    if (Alarm_Id && sizeof(a_info = get_alarm(Alarm_Id)))
        desc += "Time left:  " + ftoa(a_info[2]) + "\n";

    return desc;
}

/*
 * Function name: query_magic_protection
 * Description:   the resistance effect
 * Arguments:     prop - the resistance property
 *                what - the protected object
 * Returns:       an array of integer with the resistance:
 *                ({ strength, additive })
 *                Strength is always less or equal 40, and
 *                the resistance effect is always additive
 */
public mixed
query_magic_protection(string prop, object what)
{
    if ((what == environment(this_object())) && (prop == Res_Type))
        return ({ Strength, 1 });

    return 0;
}

public string
show_subloc(string subloc, object on, object for_obj)
{
    string data;

    if (on->query_prop(TEMP_SUBLOC_SHOW_ONLY_THINGS))
        return "";

    if (subloc != query_subloc_name())
    {
        return "";
    }
    
    return query_subloc_description(on, for_obj);
}

/*
 * Function name: query_subloc_description
 * Description:   Each resistance type is associated with a descriptive
 *                effect. This function captures just the strings
 *                descriptions. It can be used by the actual subloc
 *                function.
 */
public string
query_resistance_description(string resistance_type)
{
    string subloc;
    switch(resistance_type)
    {
    case MAGIC_I_RES_ACID:
        subloc = "skin is covered with a glossy substance.\n";
        break;
    case MAGIC_I_RES_AIR:
        subloc = "aura is of calm and stillness.\n";
        break;
    case MAGIC_I_RES_COLD:
        subloc = "flesh is radiating a strange heat.\n";
        break;
    case MAGIC_I_RES_DEATH:
        subloc = "countenance exhudes a mystical sense of well-being.\n";
        break;
    case MAGIC_I_RES_EARTH:
        subloc = "body is unnaturally stiff and rigid.\n";
        break;
    case MAGIC_I_RES_ELECTRICITY:
        subloc = "skin is coated with a smooth springy substance.\n";
        break;
    case MAGIC_I_RES_FIRE:
        subloc = "flesh is covered with tiny ice crystals.\n";
        break;
    case MAGIC_I_RES_ILLUSION:
        subloc = "eyes vibrate strangely.\n";
        break;
    case MAGIC_I_RES_LIFE:
        subloc = "body is cast in a dark pall of gloom.\n";
        break;
    case MAGIC_I_RES_MAGIC:
        subloc = "aura is utterly silent.\n";
        break;
    case MAGIC_I_RES_POISON:
        subloc = "veins pulse with unnatural verve.\n";
        break;
    case MAGIC_I_RES_WATER:
        subloc = "skin has taken on an unusual texture.\n";
        break;
    default:
        subloc = "appearance seems somehow strange.\n";
    }
    return subloc;
}

/*
 * Function:    query_subloc_description
 * Description: When someone examines a person with this spell effect,
 *              they will see whatever is returned by this function.
 */
public string
query_subloc_description(object on, object for_obj)
{
    string subloc = query_resistance_description(query_res_type());
    if (for_obj == on) {
        subloc = "Your " + subloc;
    } else {
        subloc = capitalize(on->query_possessive()) + " " + subloc;
    }
    return subloc;
}
