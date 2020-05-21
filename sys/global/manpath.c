/*
 * manpath.c
 *
 * Version 2.0
 *
 * Handle manaul search requests in the /doc/man section of the 
 * documentation.
 */

#pragma save_binary

#include <macros.h>
#include <std.h>

#define MANDIR "/doc/man"

/* The chapters under MANDIR */
static string	*chapters;
/* Array of "chapterized" permutated indexes on the form:
 * ([ "chaptname" : ({ "index1", "index2", .. }) ])
 */
static mapping	chapt_index;

int
is_dir(string fname)
{
    return ((file_size(MANDIR + "/" + fname) == -2) &&
	    (member_array(fname, ({".",".."}) ) < 0));
}

int
is_file(string fname, string chapt)
{
    return (file_size(MANDIR + "/" + chapt + "/" + fname) > 0);
}

/*
 * Function name:   init_man
 * Description:     Initialize the index arrays.
 */
static void
init_man()
{
    int i;
    string *files, subjects;

    chapt_index = ([]);

    chapters = filter(get_dir(MANDIR + "/*"), is_dir);

    for (i = 0; i < sizeof(chapters); i++)
    {
	files = filter(get_dir(MANDIR + "/" + chapters[i] + "/*"),
		       &is_file(, chapters[i]));
	chapt_index[chapters[i]] = files;
    }
}

/*
 * Function name:   get_index
 * Description:     Return all subjects in a given chapter
 * Arguments:	    chapter - The chapter to search in.
 * Returns:         An array containing the list of found subjects in the
 *                  chapter, or empty array.
 */
public mixed *
get_index(string chapt)
{
    if (member_array(chapt, chapters) >= 0)
    {
	return chapt_index[chapt] + ({ });
    }
    return ({ });
}

/*
 * Function name:   get_keywords
 * Description:     Return all possible subject name array matches of a 
 *		    given keyword in one or all chapters.
 * Arguments:	    chapter - The chapter to search in.
 *		    keyword - The keyword to seach for.
 * Returns:         An array containing the list of found subjects
  ({ "subject", "subject" ... })
 */
public mixed *
get_keywords(string chapt, string keyword)
{
    mixed *topics;

    if (!pointerp(chapt_index[chapt]))
    {
        return ({ });
    }

    return filter(chapt_index[chapt], &wildmatch(keyword, ));
}

/*
 * Function name:   fix_subjlist
 * Description:     Extract the subject names from a subjectlist split
 *		    on a keyword. Typical example: "hubba%%hubbi%%bibi"
 *		    split on 'bb' will give:
 *			({ "%%hu", "a%%hu", "i%%bibi%%" })
 *		    The result should be:
 *			({ "hubba", "hubbi" })   
 * Arguments:	    split: The subject list split on a keyword
 *		    keyw:  The keyword used to split the list.
 *		    okbef: True if letters before keyword is acceptable
 *		    okaft: True if letters after keyword is acceptable
 * Returns:         An array containing the matching subjects.
 *			
 */
string *
fix_subjlist(string *split, string keyw, int okbef, int okaft)
{
    int il, fb, fa;
    string t, *bef, bstr, fstr;
    string *res;

    res = ({});

    for (il = 1; il < sizeof(split);)
    {
	fa = 0; fb = 0;
	bef = explode(split[il-1] + "&&", "%%");
	t = bef[sizeof(bef)-1];
	if (t != "&&")
	{
	    sscanf(t,"%s&&", bstr);
	    fb = 1;
	}
	else
	    bstr = "";

	t = "&&";
	while (t == "&&" && il < sizeof(split))
	{
	    fstr = "";
	    bstr += keyw;
	    sscanf(split[il] + "%%&&", "%s%%%s", fstr, t);
	    il++;
	    bstr += fstr;
	    if (fstr != "")
		fa = 1;
	}

	if (fb && !okbef)
	    continue;

	if (fa && !okaft)
	    continue;

	res += ({ bstr });
    }
    return res;
}

public string *
get_chapters()
{
    if (!chapters)
	init_man();

    return chapters;
}

void
remove_object()
{
    destruct();
}
