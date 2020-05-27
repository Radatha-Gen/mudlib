/* std/act/add_things.c
 *
 * This is a file inherited into monster.c that enables the player to
 * add weapons and armour to the monster in a very convinient way. The
 * monster will automaticly wield/wear the things.
 *
 */
#pragma save_binary
#pragma strict_types

#include <files.h>
#include <macros.h>

static int
wield_wear_item(object ob)
{
    if (IS_WEAPON_OBJECT(ob))
    {
        ob->command_wield();
        return 0;
    }

    if (IS_WEARABLE_OBJECT(ob))
    {
        ob->command_wear();
        return 0;
    }

    if (IS_HOLDABLE_OBJECT(ob))
    {
        ob->command_hold();
        return 0;
    }
    return 0;
}

static object
equip_clone(mixed item)
{
    if (stringp(item))
    {
        item = FPATH(FILE_PATH(file_name()), item);
        item = clone_object(item);
    }

    return item;
}

/*
 * Function name:  equip
 * Description:    clones weapons, armour or other objects and makes
 *                 the npc wield and wear all.
 * Arguments:      equipment: string, or array of strings with the
 *                            filenames of the equipment to clone
 *                            Relative paths are accepted
 *                 dont_wield: if this flag is set the npc will
 *                       not attempt to wield or wear any of the
 *                       equipment cloned. [optional]
 *                 init_call: this function will be called with the items
 *                            as argument. [optional]
 * Note:           It doesn't hurt to leave off the dont_wield flag even
 *                 when the things you are cloning are not weapons and
 *                 armour. The npc only attempts to wield or wear legit
 *                 weapons and armour. The only use for the flag is when
 *                 you for some reason want the npc to have some weapon
 *                 or armour in his inv. but not worn/wielded.
 * Returns:
 */
public varargs object *
equip(mixed equiplist, int dont_wield = 0, function init_call = 0)
{
    object otp;

    setuid();
    seteuid(getuid());

    otp = this_player();
    set_this_player(this_object());

    if (!pointerp(equiplist))
        equiplist = ({ equiplist });

    equiplist = map(equiplist, equip_clone) - ({ 0 });
    equiplist->move(this_object(), 1);

    if (!dont_wield)
        filter(equiplist, wield_wear_item);

    if (functionp(init_call))
        map(equiplist, init_call);

    set_this_player(otp);
    return equiplist;
}

static object
equip_one_thing(string file)
{
    object *items = equip(file);
    if (sizeof(items))
        return items[0];

    return 0;
}

/*
 * Function name:  add_weapon
 * Description:    clones a weapon to the monster and makes the monster wield it.
 * Arguments:      filename:  The filename of the weapon. [string]
 * Returns:        the weapon
 */
public object
add_weapon(string file)
{
    return equip_one_thing(file);
}

/*
 * Function name:  add_armour
 * Description:    clones an armour to the monster and makes the monster wear it.
 * Arguments:      filename: The filename of the armour. [string]
 * Returns:        the armour
 */
public object
add_armour(string file)
{
    return equip_one_thing(file);
}
