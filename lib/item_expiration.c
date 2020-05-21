/*
 * This function simply keeps track of how much something has been used in
 * combat and its creation date.
 *
 * To use this, you need to find functions in which you wish to call the
 * <add_expiration_combat_hook> and <remove_expiration_combat_hook>. In a 
 * weapon or armour these functions would be <wield/worn> and <wear/remove>.
 * In a standard object, this would likely be <enter_env> and <leave_env>.
 *
 * You surely wish to have the variabled time of combat stored when the item
 * leaves the game, for which you need to add the following to your code.
 *
 * string
 * query_recover()
 * {
 *     return MASTER + ":" + <other recovery> + query_item_expiration_recover();
 * }
 *
 * void
 * init_recover(string arg)
 * {
 *     <other recovery>
 *     init_item_expiration_recover(arg);
 * }
 */
 
#include <hooks.h>
#include <time.h>
#include <files.h>
#include <macros.h>

// Global Variables
static int      gExpiration = 0,    // The timestamp for when the item expires
                gExpireAlarm = 0,   // The alarm for breaking items
                gUptimeLimit = 0;   // The timestamp of the next armageddon

/*
 * Function name:   item_expiration_combat_rate
 * Description:     A maskable function in which we handle the effect that the
 *                  interval has on the item expiration date. In case we want
 *                  varying decay based on imbues or similar factors.
 * Arguments:       (int) interval - The interval since last combat 
 * Returns:         (int) - the amount to remove from the timestamp.
 */
public int
item_expiration_combat_rate(float interval)
{
    // Defaulted to 17, this means that for every second in combat, 18 seconds
    // pass on the combat timer.
    return ftoi(interval * 17.0);
} /* item_expiration_combat_rate */

/*
 * Function name:   item_expiration_break
 * Description:     The function that is called to break the item, it handles
 *                  removing or unwielding the item if remove_broken function
 *                  does not exist. It also handles description of the break.
 * Arguments:       None
 * Returns:         Nothing
 */
static nomask void
item_expiration_break()
{
    object *env, room;
    
    // Cant break broken items..
    if (this_object()->query_prop(OBJ_I_BROKEN))
        return;
    
    // The object doesnt have an environment..
    if (!sizeof(env = all_environment(this_object())))
        return;
    
    room = env[-1];
    // Tell everyone that it broke.
    tell_room(room, "The " + QSHORT(this_object()) + " breaks due to wear "
    + "and tear.\n");
    
    // Break the item.
    if (!function_exists("remove_broken", this_object()))
    {
        if (this_object()->query_wearable_item())
            this_object()->remove_me();
        
        if (this_object()->query_holdable_item())
            this_object()->command_release();
        
        this_object()->add_prop(OBJ_I_BROKEN, 1);
    } else
        this_object()->remove_broken(1);
} /* item_expiration_break */

/*
 * Function name:   update_item_expiration_alarm
 * Description:     This handles the alarm for removing the item when it gets
 *                  too old.
 * Arguments:       None 
 * Returns:         Nothing
 */
static nomask void
update_item_expiration_alarm()
{
    float expire = 0.0;
    
    if (gExpireAlarm)
        remove_alarm(gExpireAlarm);
    
    if (gExpiration > time())
    {
        if (!gUptimeLimit)
            gUptimeLimit = SECURITY->query_uptime_limit() +
                           SECURITY->query_start_time();
        
        if (gExpiration > gUptimeLimit)
            return;
        
        expire = itof(gExpiration - time());
    }
    
    gExpireAlarm = set_alarm(expire, 0.0, &item_expiration_break());
} /* update_item_expiration_alarm */

/*
 * Function name:   query_item_expiration
 * Description:     Returns the timestamp when this item expires.
 * Arguments:       None
 * Returns:         (int) - the timestamp.
 */
public nomask int
query_item_expiration()
{
    return gExpiration;
} /* query_item_expiration */

/*
 * Function name:   set_item_expiration
 * Description:     Sets the amount of real life seconds until the item expires.
 * Arguments:       (int) expire - The amount of time until the item expires.
 * Returns:         Nothing
 */
static nomask varargs void
set_item_expiration(int expire = 7776000)
{
    if (!gExpiration)
        gExpiration = time() + expire;
} /* set_item_expiration */

/*
 * Function name:   remove_item_expiration
 * Description:     This removes the expiration for items, this can be used for
 *                  guild items or other similar situations where the expiration
 *                  has been set incorrectly.
 * Arguments:       None
 * Returns:         Nothing
 */
static void
remove_item_expiration()
{
    gExpiration = 0;
    
    if (gExpireAlarm)
        remove_alarm(gExpireAlarm);
} /* remove_item_expiration */

/*
 * Function name:   update_expiration_combat
 * Description:     This is called to handle the very simplistic calculation
 *                  which determines the amount of time this has been used in
 *                  combat.
 * Arguments:       (int) heartbeat - timestamp from your last heartbeat
 * Returns:         Nothing
 */
public nomask void
update_expiration_combat(object actor, object *enemies, object attack_ob,
    float speed)
{
    if (objectp(attack_ob))
        gExpiration -= max(0, item_expiration_combat_rate(speed));
    
    if (gExpiration <= time() || !objectp(attack_ob))
        update_item_expiration_alarm();
} /* update_expiration_combat */

/*
 * Function name:   add_expiration_combat_hook
 * Description:     This adds a hook to a living to monitor the amount of time
 *                  spent in combat.
 * Arguments:       (object) living - The living we wish to monitor
 * Returns:         Nothing
 */
public nomask void
add_expiration_combat_hook(object living)
{
    if (gExpiration)
        living->add_hook(HOOK_HEART_BEAT_IN_COMBAT, &update_expiration_combat());
} /* add_expiration_combat_hook */

/*
 * Function name:   remove_expiration_combat_hook
 * Description:     This removes the combat hook from the living whom we are
 *                  monitoring.
 * Arguments:       (object) living - The living we wish to monitor
 * Returns:         Nothing
 */
public nomask void
remove_expiration_combat_hook(object living)
{
    living->remove_hook(HOOK_HEART_BEAT_IN_COMBAT, this_object());
} /* remove_expiration_combat_hook */

/*
 * Function name:   time_to_desc
 * Description:     This converts a timestamp to days and hours.
 * Arguments:       (int) time - the time to convert
 *                  (status) combat - convert up or down
 * Returns:         (string) The formated time.
 */
static string
time_to_desc(int time, status combat)
{
    // Modulus of an hour, minutes and seconds are not of interest.
    time -= time % 3600;
    time = max(0, time);
    
    // I gave up, the switch is ugly..
    switch(time / 3600)
    {
        case 0:
        // Minimum 1 hour
        time = 3600;
            break;
        case 1..3:
        time = 3 * 3600;
        if (combat) time = 3600;
            break;
        case 4..6:
        time = 6 * 3600;
        if (combat) time /= 2;
            break;
        case 7..12:
        time = 12 * 3600;
        if (combat) time /= 2;
            break;
        case 13..24:
        time = 24 * 3600;
        if (combat) time /= 2;
            break;
        default:
        // Round the hours up on days, or remove them
        if ((time % (24 * 3600)) > 0 && !combat)
            time += (24 * 3600) - (time % (24 * 3600));
        else
            time -= time % (24 * 3600);
            break;
    }
    
    return CONVTIME(time);
} /* time_to_desc */ 

/*
 * Function name:   appraise_item_expiration
 * Description:     This function returns the items expiration description.
 * Arguments:       None
 * Returns:         (string) - The expiration description
 */
public nomask string
appraise_item_expiration()
{
    if (gExpiration)
    {
        string  expire_desc, combat_desc;
        int     expiration, combat_expire,
               *time_num;
        
        expiration = max(0, gExpiration - time());
    
        if (expiration > 0)
        {
            // Dont want to show the age desc if it is broken.
            if (this_object()->query_prop(OBJ_I_BROKEN))
                return "";
        
            // Get the actual time that passes every second in combat.
            if (item_expiration_combat_rate(1.0) > 0)
            {
                // Figure out how long the item will last in combat.
                combat_expire = expiration / (item_expiration_combat_rate(1.0) + 1);
                combat_desc = time_to_desc(combat_expire, 1);
                
                if (!combat_expire)
                    combat_expire = -1;
            }
            
            expire_desc = time_to_desc(expiration, 0);
            
            if (combat_expire > 3600)
                return "Judging by its age and how much it appears to have "
                + "been used in combat you determine that it will last a "
                + "minimum of " + combat_desc + " and a maximum of " 
                + expire_desc + " before it succumbs to wear and tear.\n";
/*
                return "You determine that it has between " + combat_desc
                + " and " + expire_desc + " before it succumbs to wear and "
                + "tear, dependent on its age and further use in combat.\n";
*/
            if (combat_expire)
                return "Judging by its age and how much it appears to have "
                + "been used in combat you determine that the "
                + this_object()->short() + " does not have much time left.\n";
            /*
            return "Judging by its age and how much it appears to have been "
            + "used in combat you determine that it will last less than "
            + expire_desc + " before it succumbs to wear and tear.\n";
            */
            
            // this only happens if the item doesnt have combat expire.
            return "You determine it has less than " + expire_desc + " before "
            + "it succumbs to wear and tear.\n";
        }

        return "The " + this_object()->short() + " is long past a usable "
        + "state, having succumbed to wear and tear.\n";
    }
    
    return "";
} /* appraise_item_expiration */

/*
 * Function name:   query_item_expiration_recover
 * Description:     Called to check whether this item has any recoverable
 *                  variables.
 * Arguments:       None
 * Returns:         (string) - the default recovery string.
 */
public nomask string
query_item_expiration_recover()
{
    /* We only need to add a recover string if the variables are set. */
    if (gExpiration)
        return "#ItemExpire#" + gExpiration + "#";

    return "";
} /* query_item_expiration_recover */

/*
 * Function name:   init_item_expiration_recover
 * Description:     When the object recovers, this function is called to set
 *                  the necessary variables.
 * Arguments:       (string) arg - the arguments to parse
 * Returns:         Nothing
 */
public nomask void
init_item_expiration_recover(string arg)
{
    string  foo, bar;
    sscanf(arg, "%s#ItemExpire#%d#%s", foo, gExpiration, bar);
    
    if (gExpiration)
        update_item_expiration_alarm();
    
} /* init_item_expiration_recover */