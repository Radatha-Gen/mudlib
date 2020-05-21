/*
 * /std/player/more.c
 *
 * You can call the function more() in two ways, depending on whether you
 * want the player to read in file or in whether you want him/her to read a
 * text you have stored in a variable. Note that if you want the player to
 * read directly from a file, the player must be able to read that file.
 * This may sound a bit silly, but it means that the player must have read
 * rights to that file. It is therefore advisable to always read the text
 * and pass a single string to the player. It is very unlikely that you will
 * want to make a player read something longer than 50KB in the first place.
 *
 * Reading text from a file:
 *     player->more(string filename, int start);
 *
 *     filename: the filename of the file to read
 *     start   : the first line to read in the file (starting with 1)
 *               WARNING: If start_line == 0, then only the filename
 *               will be printed to the player.
 *
 * Reading text from a variable:
 *     player->more(string *lines);
 *
 *     lines   : An array with the lines of text to print. The lines should
 *               NOT contain newlines, example: ({ line1, line2, line3 })
 * Or:
 *     player->more(string text);
 *
 *     text    : the text to read, separated by \n (newlines)
 *               NOTE that text is a single string with all the lines to read
 *               concatenated, separated by \n, example: line1\nline2\nline3
 *
 * It is possible to let the more function generate a call to the object
 * that initially created us. Therefore, you have to add a THIRD argument
 * to the call to more(). If you read text from a variable, the second
 * argument must be 0. The third argument has the function-pointer to the
 * function you want to have called after more, making the call:
 *     player->more(arg1, arg2, function return_function);
 */

#include <stdproperties.h>
#include <options.h>

#define MORE_PAGESIZE (20)
#define MORE_PROMPT   write("-- More -- " + lineno +             \
		      ((numlines > 1) ? ("/" + numlines) : "") + \
                      " -- (<cr> t b r n a <num> q x !<cmd> h ?) -- ")
#define MORE_DONE     if (functionp(ret_func)) { ret_func(); }
#define MORE_INPUT    input_to(&input_to_more(, lines, filename, first, lineno, ret_func))

#define TAIL_MAX_READ 48000
#define TAIL_DONE     if (functionp(ret_func)) { ret_func(); }
#define TAIL_PROMPT   write("-- Tail -- " + lastline +             \
		      ((numlines > 1) ? ("/" + numlines) : "") + \
                      " -- (<cr> e d r q x !<cmd> h ?) -- ")
#define TAIL_INPUT    input_to(&input_to_tail(, lines, lastline, ret_func))

/*
 * Observe that the variable lineno will contain the number to the last
 * printed line in the terminology of the player. For the player, the first
 * line will be line 1, whereas in the text from memory, the first line is
 * line 0 and in the file the first line is line 1, though internally the
 * file line number starts at 0 too. Confusing neh?
 */

/*
 * Function name: input_to_more
 * Description  : After the some part of the text has been printed, the
 *                next command of the player is fed into this function.
 *                It controls what the player wants to read next.
 * Arguments    : string answer     - the command by the player.
 *                string *lines     - the lines of the text (if in text).
 *                string filename   - the file to print (if any).
 *                function ret_func - the function to return to (if any).
 *                int first         - the first line to print.
 *                int lineno        - the last line printed.
 */
static nomask void
input_to_more(string answer, string *lines, string filename, int first,
    int lineno, function ret_func)
{
    int    index;
    int    pagesize;
    string error;
    int    numlines = sizeof(lines);

    pagesize = (((pagesize = query_option(OPT_MORE_LEN)) < 5) ?
    	MORE_PAGESIZE : pagesize);
    answer = lower_case(answer);

    if (stringp(answer) &&
	(sscanf(answer, "%d", index) == 1))
    {
	lineno = index + pagesize - 1;
	answer = "Goto";
    }

    switch (answer)
    {
    case "":
    case "n":
	lineno += pagesize;
	break;

    /* Since we use the lower case of answer, this can never be triggered
     * by the command of a player, so we can use it as a flag to check on
     * numerical input.
     */
    case "Goto":
	break;

    case "q":
    case "quit":
    case "x":
    case "exit":
	MORE_DONE;
	return;

    case "?":
    case "h":
    case "help":
	write(
"\nAvailable commands:\n\n" +
"   <return>     press <return> to display next page\n" +
"   t(op)        go to top of document\n" +
"   b(ack)       go back one page\n" +
"   n(ext)       display the next page (same as pressing <cr>)\n" +
"   r(edisplay)  display the same page again\n" +
"   <number>     go to line <number>\n" +
"   a(ll)        display all the text remaining if it isn't too much\n" +
"   h(elp) or ?  display this help message\n" +
"   !<command>   escape and execute <command>, then continue reading\n" +
"   q(uit)       quit reading document\n" +
"(e)x(it)        exit reading document\n\n");
	MORE_PROMPT;
	MORE_INPUT;
	return;

    case "t":
    case "top":
	lineno = first;
	break;

    case "r":
    case "redisplay":
	break;

    case "b":
    case "back":
    case "p":
    case "previous":
	lineno -= pagesize;
	break;

    case "a":
    case "all":
	if (stringp(filename))
	{
	    if (strlen(error = catch(cat(filename, lineno + 1))))
	    {
		write("Cannot print remainder: " + error);
		MORE_PROMPT;
	    	MORE_INPUT;
	    	return;
	    }
	    write("END\n");
	}
	else
	{
	    if (strlen(error = catch(write(implode(lines[lineno..], "\n")))))
	    {
	    	write("Cannot print remainder: " + error);
	    	MORE_PROMPT;
	    	MORE_INPUT;
	    	return;
	    }
	}
	MORE_DONE;
	return;

    default:
	MORE_PROMPT;
	MORE_INPUT;
	return;
    }	

    if (lineno < (pagesize + first))
    {
	lineno = pagesize + first;
    }

    if (stringp(filename))
    {
	/* Cat will return the number of lines that are actually read.
	 * If you read beyond the EOF, the reading will stop. This means
	 * that if the last chunk is exactly one page, you still have to
	 * read the EOF sign.
	 */
	if (cat(filename, (lineno - pagesize + 1), pagesize) < pagesize)
	{
	    write("END\n");
	    MORE_DONE;
	    return;
	}
    }
    else
    {
	for (index = (lineno - pagesize);
	    ((index < lineno) && (index < numlines)); index++)
	{
	    write(lines[index] + "\n");
	}

	if (lineno >= numlines)
	{
	    MORE_DONE;
	    return;
	}
    }

    MORE_PROMPT;
    MORE_INPUT;
    return;
}

/*
 * Function name: more
 * Description  : Call this function in a player like explained in the header
 *                of this file in order to let him/her read a text with more.
 *                There are three options:
 *                  player->more(string filename, int start)
 *                  player->more(string *lines)
 *                  player->more(string text)
 *
 * Arguments    : mixed arg - the filename of the file to read or the text to
 *                    read itself. If it's the text, it may be an array of
 *                    lines or a single string concatenated with newlines (\n).
 *                int start - for files this must be the index to the first
 *                    line to read (first line == 1). When <arg> contains the
 *                    text itself, this variable should be 0 or omitted.
 *                function func - the function to call when the player is
 *                    done reading the text.
 * Returns      : int 1 - always.
 */
public nomask varargs int
more(mixed arg, int start, function func)
{
    string *lines;
    string filename = 0;

    if (pointerp(arg))
    {
        lines = arg;
        start = 0;
    }
    else if (start)
    {
    	filename = arg;
	lines = 0;
	start--;
    }
    else
    {
	lines = explode(arg, "\n");
    }

    input_to_more("", lines, filename, start, start, func);
    return 1;
}

/*
 * Function name: input_to_tail
 * Description  : After the some part of the text has been printed, the
 *                next command of the player is fed into this function.
 *                It controls what the player wants to read next.
 * Arguments    : string answer     - the command by the player.
 *                string *lines     - the lines of the text.
 *                int lastline      - the last line printed.
 *                function ret_func - the function to return to (if any).
 */
static nomask void
input_to_tail(string answer, string *lines, int lastline, function ret_func)
{
    int pagesize;
    int index, startline;
    int numlines = sizeof(lines);

    pagesize = (((pagesize = query_option(OPT_MORE_LEN)) < 5) ?
    	MORE_PAGESIZE : pagesize);
    answer = lower_case(answer);

    /* Initialize. */
    if (!lastline)
    {
	lastline = numlines + pagesize;
    }

    switch(answer)
    {
    case "":
    case "n":
	lastline = max(1, lastline - pagesize);
	break;

    case "q":
    case "quit":
    case "x":
    case "exit":
	TAIL_DONE;
	return;

    case "?":
    case "h":
    case "help":
	write(
"\nAvailable commands:\n\n" +
"   <return>     press <return> to display next page (same as 'n')\n" +
"   e(nd)        go to bottom of document\n" +
"   d(own)       go back one page\n" +
"   r(edisplay)  display the same page again\n" +
"   h(elp) or ?  display this help message\n" +
"   !<command>   escape and execute <command>, then continue reading\n" +
"   q(uit) or x  quit reading document (also works with 'x')\n\n");
	TAIL_PROMPT;
	TAIL_INPUT;
	return;

    case "e":
    case "end":
	lastline = numlines;
	break;

    case "r":
    case "redisplay":
	break;

    case "d":
    case "down":
    case "p":
    case "previous":
	lastline = min(numlines, lastline + pagesize);
	break;

    default:
	TAIL_PROMPT;
	TAIL_INPUT;
	return;
    }

    startline = max(0, lastline - pagesize);
    for (index = startline; index < lastline; index++)
    {
	write(lines[index] + "\n");
    }

    if (startline <= 1)
    {
	TAIL_DONE;
	return;
    }

    TAIL_PROMPT;
    TAIL_INPUT;
}

/*
 * Function name: tail
 * Description  : Call this function in a player like explained in the header
 *                of this file in order to let him/her read a text with tail.
 *                There are three options:
 *                  player->tail(string filename)
 *                  player->tail(string *lines)
 *                  player->tail(string text)
 *
 * Arguments    : mixed arg - the filename of the file to read or the text to
 *                    read itself. If it's the text, it may be an array of
 *                    lines or a single string concatenated with newlines (\n).
 *                function func - the function to call when the player is
 *                    done reading the text.
 * Returns      : int 1 - always.
 */
public nomask varargs int
tail(mixed arg, function func)
{
    string *lines;
    int start;

    if (pointerp(arg))
    {
        lines = arg;
    }
    else if ((start = file_size(arg)) > 0)
    {
	start = max(0, start - TAIL_MAX_READ);
    	arg = read_bytes(arg, start, TAIL_MAX_READ+1);
	lines = explode(arg, "\n");
    }
    else
    {
	lines = explode(arg, "\n");
    }

    input_to_tail("n", lines, 0, func);
    return 1;
}
