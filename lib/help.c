/*
 * /lib/help.c
 *
 * Support for a unified help system. It supports two basic methods:
 *
 * 1) a topic + help text or help file.
 * 2) a help directory, giving access to all *.help files in that folder.
 *
 * Optionally, one or more categories may be given. When the player mentions
 * our category, we know they mean our help topic. Also used to list all
 * topics that we have available in our directory.
 *
 * For more info, see 'man help'.
 */

static string  help_dir;
static string  help_default;
static string *help_categories = ({ });
static mapping help_topics = ([ ]);

#include <macros.h>
#include <options.h>

#define NO_DISPLAY_DONE 2

/*
 * Function name: hook_display_help_topics
 * Description  : Display the available topics for a given category. It makes
 *                a little table based on the screen width of the player.
 * Arguments    : string category - the category.
 *                string *topics - the topics within that category.
 */
public void
hook_display_help_topics(string category, string *topics)
{
    int scrw = this_player()->query_option(OPT_SCREEN_WIDTH);

    scrw = ((scrw >= 40) ? (scrw - 2) : 78);

    write("Topics for: " + category + "\n" +
        sprintf("%-*#s\n", scrw, implode(topics, "\n")));
}

/*
 * Function name: hook_no_help_topic
 * Description  : Display a message if a player specifies a category and a
 *                topic, but the topic isn't found.
 * Arguments    : string category - the category.
 *                string topic - the topic the player looked for.
 */
public void
hook_no_help_topic(string category, string topic)
{
    write("The topic \"" + topic + "\" is not found in category \"" +
        category + "\".\n");
}

/*
 * Function name: query_display_no_help
 * Description  : Called to verify if there is a reason to not display the
 *                help, even if the topic exists.
 * Nota Bene    : Normally this should be a silent check, but if you really
 *                must (preferably when a valid category is presented), then
 *                you may display a message and terminate the search by
 *                returning 2 instead of 1. But really only do this with a
 *                good reason.
 * Arguments    : string category - the (optional) category.
 *                string topic - the topic.
 * Returns      : int 1/0 - if true, don't display the help. 
 */
public int
query_display_no_help(string category, string topic)
{
    return 0;
}

public int
process_help(string category, string topic)
{
    string *topics;
    string text;
    int result;
    int showindex;

    /* A category is mentioned, but it isn't ours, so don't touch. */
    if (category && !IN_ARRAY(category, help_categories))
    {
        return 0;
    }

    /* If the player enters only the name of the category, display the index
     * of available topics, or a default topic, if set. */
    if (IN_ARRAY(topic, help_categories))
    {
        if (help_default)
        {
            topic = help_default;
        }
        else
        {
            showindex = 1;
        }
    }
    /* Player wants to see an index of the available topics. */
    if (topic == "index")
    {
        showindex = 1;
    }

    /* We need an euid to check the folder. */
    setuid();
    seteuid(getuid());

    /* Player wants to see an index of the available help topics. */
    if (showindex)
    {
        /* Make sure we have a category. */
        if (!category)
        {
            category = ((sizeof(help_categories) > 0) ? help_categories[0] : "unknown");
        }

        /* Maybe there is a reason not to display this? */
        if (result = query_display_no_help(category, topic))
        {
            return (result == NO_DISPLAY_DONE);
        }

        topics = get_dir(help_dir + "/*.help");
        topics = map(topics, &extract(, 0, -6));
        topics |= m_indices(help_topics);

        hook_display_help_topics(category, topics);
        return 1;
    }

    if (help_topics[topic])
    {
        /* Maybe there is a reason not to display this? */
        if (result = query_display_no_help(category, topic))
        {
            return (result == NO_DISPLAY_DONE);
        }

        /* If it starts with a / then treat it as a path to the help file. */
        text = (wildmatch("/*", help_topics[topic]) ?
            read_file(help_topics[topic]) : help_topics[topic]);
        if (strlen(text))
        {
            this_player()->more(text);
            return 1;
        }
    }
    if (help_dir && (file_size(help_dir + "/" + topic + ".help") > 0))
    {
        /* Maybe there is a reason not to display this? */
        if (result = query_display_no_help(category, topic))
        {
            return (result == NO_DISPLAY_DONE);
        }

        text = read_file(help_dir + "/" + topic + ".help");
        this_player()->more(text);
        return 1;
    }

    /* This is potentially dangerous if two places have the same category as
     * it would block access to the 2nd one that may have different topics.
     * However, as a design rule we'll assume they are unique ...
     */
    if (category)
    {
        hook_no_help_topic(category, topic);
        return 1;
    }

    /* Not found. Silently pass control back. */
    return 0;
}

/*
 * Function name: add_help_topic
 * Description  : Adds a single topic to get help on. It can take either a
 *                text or the path to a help file. Note that if you use the
 *                path, then the object should have the rights to read it.
 * Arguments    : string topic - the topic to add.
 *                string text - this can take two forms:
 *                    1) a text to display.
 *                    2) the path to a file to display.
 */
public void
add_help_topic(string topic, string text)
{
    help_topics[topic] = text;
}

/*
 * Function name: remove_help_topic
 * Description  : Removes a help topic from the list of topics.
 * Arguments    : string topic - the topic to remove.
 */
public void
remove_help_topic(string topic)
{
    m_delkey(help_topics, topic);
}

/*
 * Function name: query_help_topic
 * Description  : Find out the text or path to a help file associated with a
 *                topic.
 * Arguments    : string topic - the topic.
 * Returns      : string - either the text of the help, or a filename.
 */
public string
query_help_topic(string topic)
{
    return help_topics[topic];
}

/*
 * Function name: query_help_topics
 * Description  : Find out the topics that have been added to this module.
 *                Note it doesn't give the topics that are in the directory,
 *                if a directory has been set.
 * Returns      : string * - the topics.
 */
public string *
query_help_topics()
{
    return m_indices(help_topics);
}

/*
 * Function name: set_help_category
 * Description  : Sets one or more categories that we especially recognize.
 * Arguments    : mixed category - a single string or an array of string
 *                    with the category/ies that we recognize. As convention
 *                    it should have the most prominent category name first.
 */
public void
set_help_category(mixed category)
{
    if (pointerp(category))
    {
        help_categories += category;
    }
    if (stringp(category))
    {
        help_categories += ({ category });
    }
}

/*
 * Function name: query_help_categories
 * Description  : Returns a list of the help categories we recognize.
 * Returns      : string * - an array of the categories (may be empty).
 */
public string *
query_help_categories()
{
    return help_categories + ({ });
}

/*
 * Function name: set_help_dir
 * Description  : Set a directory to look at for help files. All files in the
 *                directory with xxx.help will be considered help files. Note,
 *                the object into which this is inhrited must have the rights
 *                to read in this directory.
 * Arguments    : string dir - the directory to look at.
 */
public void
set_help_dir(string dir)
{
    help_dir = dir;
}

/*
 * Function name: query_help_dir
 * Description  : Returns the path to the directory with the help files.
 * Returns      : string - the directory.
 */
public string
query_help_dir()
{
    return help_dir;
}

/*
 * Function name: set_help_default
 * Description  : Set a default topic to be displayed when a user does 
 *                "help <category>" rather than displaying an index.
 * Arguments    : string topic - the default topic to display.
 */
public void
set_help_default(string topic)
{
    help_default = topic;
}

/*
 * Function name: query_help_default
 * Description  : Returns the default topic
 * Returns      : string - the default topic.
 */
public string
query_help_default()
{
    return help_default;
}
