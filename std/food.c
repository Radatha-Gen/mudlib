/*
 * /std/food.c
 * 
 * This is the standard object used for any form of edible things
 *
 * Typical usage of /std/food.c
 *
 *      inherit "/std/food";
 *
 *      void
 *      create_food()
 *      {
 *          set_amount(amount_in_gram);
 *          set_name("name of food");
 *          set_short(...);
 *          set_long(....);
 *      }
 *
 *  If you want any special effect in your food you can define
 *  special_effect() which is called when eating it.
 *  The number of items you ate is passed as argument to special_effect();
 *
 *  Food may recover and recovery is added by default, but only if the
 *  the food has been coded in a particular object and not cloned from
 *  this file directly and externally set up.
 */

#pragma save_binary
#pragma strict_types

inherit "/std/heap";

#include <cmdparse.h>
#include <composite.h>
#include <files.h>
#include <macros.h>
#include <ss_types.h>
#include <stdproperties.h>

#define EAT_VERB     ("eat")   /* the common ingestion verb of food */

/*
 * Global variables. They are not saved.
 */
static  int     food_amount;

/*
 * Function name: create_food
 * Description  : This function is the constructor. You should redefine
 *                it to create your own food object.
 */
public void
create_food()
{
}

/*
 * Function name: create_heap
 * Description  : The heap constructor. You should not redefine this
 *                function, but use create_food() instead. It makes the
 *                food object have a default size of 1 item.
 */
public nomask void
create_heap()
{
    set_heap_size(1);
    add_prop(OBJ_M_NO_SELL, "Food is not to be resold.\n");

    set_name("food");
    set_pname("foods");

    create_food();

    if (!query_prop(HEAP_S_UNIQUE_ID))
    {
        add_prop(HEAP_S_UNIQUE_ID,
            MASTER + ":" + singular_short(this_object()));
    }
}

/*
 * Function name: set_short
 * Description  : When the short is set/changed, also make sure that we do set
 *                the unique identifier for the heap.
 * Arguments    : mixed new_short - the short description.
 */
void
set_short(mixed new_short)
{
    ::set_short(new_short);

    add_prop(HEAP_S_UNIQUE_ID,
        MASTER + ":" + singular_short(this_object()));
}

/*
 * Function name:       set_amount
 * Description:         sets the amount of food in this food (in grams)
 * Arguments:           a: The amount of food
 */
public void
set_amount(int a) 
{ 
    mark_state();
    food_amount = a; 
    add_prop(HEAP_I_UNIT_VOLUME, a / 10);
    add_prop(HEAP_I_UNIT_WEIGHT, a);
    update_state();
}

/*
 * Function name:       query_amount
 * Description:         Gives the amount of food in this food
 * Returns:             Amount as int (in grams)
 */
public int
query_amount() { return food_amount; }

/*
 * Function name: command_eat
 * Description:   Eat this food.
 * Returns:       string - an error message (failure)
 *                1 - food successfully eaten.
 *                0 - access failure; default error message.
 */
public mixed
command_eat()
{
    mixed err;
    int am, num, index;

    /* Protect against redefined eat verbs for herbs. */
    if (query_verb() != EAT_VERB)
    {
        return 0;
    }

    if (err = this_player()->query_prop(LIVE_M_NO_INGEST))
    {
        if (strlen(err))
            return err;
        else
            return "You are unable to " + query_verb() + " anything.\n";
    }

    am = query_amount();
    num = num_heap();

    for (index = 0; index < num; index++)
    {
        /* See if we are too full */
        if (!this_player()->eat_food(am))
        {
            /* We couldn't eat all of the food in this heap, so split
             * it and eat what we can.
             */
            if (index == 0)
            {
                return "The " + singular_short() + " is too much for you.\n";
            }

            split_heap(index);            
            this_object()->special_effect(index);
            return 1;
        }
    }

    this_object()->special_effect(num);

    return 1;
}

/*
 * Function name: remove_food
 * Description:   Reduce the number of items in this heap after a split_heap()
 *                or remove the heap if there are no items left.
 */
public void
remove_food()
{
    if (leave_behind > 0)
    {
        set_heap_size(leave_behind);
    }
    else
    {
        remove_object();
    }
}

void
config_split(int new_num, object orig)
{
    ::config_split(new_num, orig);
    set_amount(orig->query_amount());
}

void
config_merge(object child)
{
    int total, count;

    ::config_merge(child);

    /* If there is a weight difference, average it. */
    if (query_amount() != child->query_amount())
    {
        total = (query_amount() * num_heap()) + (child->query_amount() * child->num_heap());
        count = num_heap() + child->num_heap();
        set_amount(total / count);
    }
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
    return ::stat_object() +
        "Food: " + food_amount + " (grams).\n";
}

/*
 * Function name: appraise_object
 * Description  : Called to appraise the object using the command thereto.
 * Arguments    : int num - optional - if given, use this number. Otherwise
 *                    use the appraise object skill of the player.
 */
void
appraise_object(int num)
{
    int value, skill;
    string nutrition;

    ::appraise_object(num);

    skill = (num ? num : this_player()->query_skill(SS_APPR_OBJ));
    skill = random((1000 / (skill + 1)), atoi(OB_NUM(this_object())));
    value = cut_sig_fig(food_amount + (skill % 2 ? -skill % 70 : skill) * food_amount / 100);

    /* Display whole kilograms, fractions of kilograms or grams. */
    if ((value > 10000) || (!(value % 1000)))
        nutrition = (value / 1000) + " kilograms";
    else if (value > 1000)
        nutrition = sprintf("%3.1f", itof(value) / 1000.0) + " kilograms";
    else
        nutrition = value + " grams";

    write("The nutritional value of each " + singular_short() + " is " +
        nutrition + "\n");
}

/*
 * Function name: query_recover
 * Description  : This function is called to see whether this object may
 *                recover. It will only function for food that has a
 *                real file rather than being cloned from /std/food.c
 *                since only the amount of food on the heap is saved.
 */
public string
query_recover()
{
    string file = MASTER;

    /* Don't recover bare /std/drinks since we only recover the amount of
     * drinks and no descriptions.
     */
    if (file == FOOD_OBJECT)
    {
        return 0;
    }

    return file + ":heap#" + num_heap() + "#";
}

/*
 * Function name: init_recover
 * Description  : This function is called when the food recovers.
 * Arguments    : string str - the recover argument.
 */
public void
init_recover(string str)
{
    string foobar;
    int    num;

    if (sscanf(str, "%sheap#%d#%s", foobar, num, foobar) == 3)
    {
        set_heap_size(num);
    }
}

/*      
 * Function name: special_effect
 * Description  : Define this routine if you want to do some special effect
 *                if a player consumes this food.
 * Arguments    : int amount - the number of foods consumed.
 */             
public void
special_effect(int amount)
{
}
