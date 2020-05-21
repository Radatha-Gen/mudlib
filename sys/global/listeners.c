/*
 * /sys/global/listeners.c
 *
 * This item registry serves as a way for items to notify listeners
 * that they are available. When an object is cloned, it will notify
 * this registry. Any listener that is also registered with this
 * registry will be notified and passed the newly cloned object.
 *
 * This can be used to track magic items for the AoB database, as
 * well as modify recovery and imbue magical effects as deemed
 * necessary by the game.
 *
 * Files that define themselves as listened will be called with the
 * following routine for each newly cloned object.
 *
 *    (void) notify_new_object(object obj)
 */

#pragma strict_types
#pragma no_clone
#pragma no_inherit

#include <macros.h>

// Global Variables
public function *       listeners = ({ });
public object *         waiting_objects = ({ });
public int              process_alarm = 0;

// Prototypes
public void             process_objects();

/*
 * Function:    create
 * Description: Standard create function for any object
 */
public void
create()
{
    setuid();
    seteuid(getuid());
}

/*
 * Function Name: register_listener
 * Description  : A listener who wants to be notified whenever a new
 *                object is cloned will register themselves here.
 * Macro call   : LISTENER_ADD(obj) in <files.h>
 */
public int
register_listener(mixed listener)
{
    object listener_obj;
    function listener_fun;

    if (objectp(listener))
    {
        listener_obj = listener;
    }
    else if (stringp(listener) && objectp(find_object(listener)))
    {
        listener_obj = find_object(listener);
    }

    if (!objectp(listener_obj))
    {
        return 0;
    }

    listener_fun = listener_obj->notify_new_object;
    listeners -= ({ listener_fun, 0 });
    listeners += ({ listener_fun });
    return 1;
}

/*
 * Function Name: unregister_listener
 * Description  : It removes a listener from the list of listeners.
 * Macro call   : LISTENER_REMOVE(obj) in <files.h>
 */
public int
unregister_listener(mixed listener)
{
    object listener_obj;
    function listener_fun;

    if (objectp(listener))
    {
        listener_obj = listener;
    }
    else if (stringp(listener))
    {
        listener_obj = find_object(listener);
    }

    if (!objectp(listener_obj))
    {
        return 0;
    }

    listener_fun = listener_obj->notify_new_object;
    listeners -= ({ listener_fun, 0 });
    return 1;    
}

/*
 * Function name: process_objects
 * Description:   This gets called every second to process all of
 *                the waiting objects. It loops through the listeners
 *                and processes them all.
 */
public void
process_objects()
{
    // Since this function is called in an alarm, we first copy
    // all the objects to be processed to a local variable. That
    // way there should be no race conditions.
    object * local_waiting_objects = waiting_objects + ({ });
    
    waiting_objects = ({ });
    process_alarm = 0;

    // Validate the listeners first
    listeners -= ({ 0 }); // remove invalid listeners
    // Validate the waiting objects
    local_waiting_objects -= ({ 0 }); // remove empty/invalid objects

    int nNumObjects = sizeof(local_waiting_objects);
    int nNumListeners = sizeof(listeners);
    for (int nCurrentListenerIndex = 0; nCurrentListenerIndex < nNumListeners; ++nCurrentListenerIndex)
    {
        function fListener = listeners[nCurrentListenerIndex];
        for (int nCurrentObjIndex = 0; nCurrentObjIndex < nNumObjects; ++nCurrentObjIndex)
        {
            catch(fListener(local_waiting_objects[nCurrentObjIndex]));
        }
    }
/*
    // Removed foreach because there is a gamedriver bug with function refcounts
    foreach (object obj: local_waiting_objects)
    {
        foreach (function listener_func : listeners)
        {
            // We use catch te prevent runtimes from stopping the process.
            catch(listener_func(obj));
        }
    }  
*/  
}

/*
 * Function Name: register_new_object
 * Description:   This function gets called by every object as it is being
 *                cloned. It adds the object to a list of waiting objects,
 *                which get processed every minute.
 * Macro call   : LISTENER_NOTIFY(obj) in <files.h>
 */
public void
register_new_object(object obj)
{
    waiting_objects += ({ obj });

    // If the size of objects is greater than 1000, we go ahead and
    // process it immediately.
    if (sizeof(waiting_objects) >= 1000)
    {
        remove_alarm(process_alarm);
        process_alarm = 0;
        process_objects();
        return;
    }
    
    if (!process_alarm)
    {
        process_alarm = set_alarm(1.0, 0.0, process_objects);
    }
}
