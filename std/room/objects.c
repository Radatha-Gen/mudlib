/*
 * /std/room/objects.c
 *
 * Module of /std/room.c
 *
 * Provides easy methods for adding objects to rooms.
 */

/*
 * Function Name: add_npc
 * Description  : Clone a npc to the room and reset it automatically
 *                when it has been killed.
 * Arguments    : string file - the file to clone
 *                int count   - how many of this item.
 *                function pre  - Optional function which is called before the
 *                                npc is moved into the room.
 *                                Example: &->arm_me()
 *                function post - Optional function which is called after
 *                                the npc has been moved into the room.
 *                                Example: &->command("say Hello!")
 */
varargs void
add_npc(string file, int count = 1, function pre = 0, function post = 0)
{
    add_auto_object(file, count, &objectp(), pre, post);
}

/*
 * Function name: room_add_object
 * Description:   Clone and move an object into the room
 * Arguments:	  file - What file it is we want to clone
 *		  num  - How many clones we want to have, if not set 1 clone
 *		  mess - Message to be written when cloned
 */
varargs void
room_add_object(string file, int num, string mess)
{
    int i;
    object ob;

    if (num < 1)
	num = 1;

    seteuid(getuid());
    for (i = 0; i < num; i++)
    {
	ob = clone_object(file);
	if (stringp(mess))
	{
	    tell_room(this_object(), mess);
	    ob->move(this_object(), 1);
 	}
	else if (living(ob))
	    ob->move_living("xx", this_object());
	else
	    ob->move(this_object(), 1);
    }
}
