/*
 * /std/player/stats_sec.c
 *
 * This is a subpart of player_sec.c
 *
 * All the stat related code that is exclusive to players is here.
 */

#include <ss_types.h>
#include <state_desc.h>
#include <stdproperties.h>

/* Remember the last stats. */
static private int *last_stats = ({ });
static private int  last_xp = 0;
static private int  last_qxp = 0;

/*
 * Function name: reset_exp_gain_desc
 * Description  : Cause the exp-gain descriptions to reset.
 */
public void
reset_exp_gain_desc()
{
    last_xp = query_exp_combat() + query_exp_general();
    last_qxp = query_exp_quest();
}

/*
 * Function name: query_exp_gain_desc
 * Description  : Get the description of the exp gain since last login.
 * Returns      : string - the description, or 0 if no gain.
 */
public varargs string
query_exp_gain_desc()
{
    int gain, limit;

    gain = query_exp_combat() + query_exp_general() - last_xp;
    if (gain <= 0)
    {
	return 0;
    }

    /* The progress is measured relatively to your current exp. If you gained
     * more than 6.6% of the exp you had when you logged in, you get the
     * maximum progress measure. */
    limit = minmax((last_xp / 15), SD_IMPROVE_MIN, SD_IMPROVE_MAX);

    return GET_NUM_DESC(gain, limit, SD_PROGRESS);
}

/*
 * Function name: query_exp_quest_gain_desc
 * Description  : Get the description of the exp gain since last login.
 * Returns      : string - the description, or 0 if no gain.
 */
public string
query_exp_quest_gain_desc()
{
    int gain, limit;

    gain = query_exp_quest() - last_qxp;
    if (gain <= 0)
    {
	return 0;
    }

    /* The progress is measured relatively to your current exp. If you gained
     * more than 2.5% of the exp you had when you logged in, you get the
     * maximum progress measure. */
    limit = minmax((last_qxp / 40), (SD_IMPROVE_MIN / 5), (SD_IMPROVE_MAX / 12));

    return GET_NUM_DESC(gain, limit, SD_PROGRESS);
}

/*
 * Function name: update_last_stats
 * Description  : Copies the current stats into the PLAYER_AI_LAST_STATS
 *                property for later reference.
 */
public void
update_last_stats()
{
    int index;

    last_stats = allocate(SS_NO_EXP_STATS + 1);
    for (index = 0; index < SS_NO_EXP_STATS; index++)
    {
        last_stats[index] = query_base_stat(index);
    }
    last_stats[SS_NO_EXP_STATS] = query_average_stat();
}

/*
 * Function name: check_last_stats
 * Description  : After experience has been added, check whether the stats
 *                of the player have changed. If so, inform the player of
 *                his stat increase.
 * Arguments    : int quest - if true, quest exp was added.
 */
public void
check_last_stats(int quest)
{
    int index;
    int *new_stats = allocate(SS_NO_EXP_STATS + 1);
    string olddesc, newdesc, gain;

    /* Initialize if needed. */
    if (sizeof(last_stats) != (SS_NO_EXP_STATS + 1))
    {
        update_last_stats();
        return;
    }

    for (index = 0; index < SS_NO_EXP_STATS; index++)
    {
        new_stats[index] = query_base_stat(index);
        if (new_stats[index] != last_stats[index])
        {
            if (new_stats[index] >= SD_STATLEVEL_IMP)
            {
                if (last_stats[index] < SD_STATLEVEL_IMP)
                {
                    tell_object(this_object(), "You have reached impossible " +
                        SD_LONG_STAT_DESC[index] + ".\n");
                    gmcp_char(GMCP_CHAR_STATUS, SD_LONG_STAT_DESC[index], "impossible");
                    ACHIEVEMENTS->trigger_event_stat_level(this_object(), index, "immortal", "impossible");
                }
            }
            if (new_stats[index] >= SD_STATLEVEL_MIR)
            {
                if (last_stats[index] < SD_STATLEVEL_MIR)
                {
                    tell_object(this_object(), "You have reached miraculous " +
                        SD_LONG_STAT_DESC[index] + ".\n");
                    gmcp_char(GMCP_CHAR_STATUS, SD_LONG_STAT_DESC[index], "miraculous");
                    ACHIEVEMENTS->trigger_event_stat_level(this_object(), index, "immortal", "miraculous");
                }
            }
            if (new_stats[index] >= SD_STATLEVEL_UNB)
            {
                if (last_stats[index] < SD_STATLEVEL_UNB)
                {
                    tell_object(this_object(), "You have reached unbelievable " +
                        SD_LONG_STAT_DESC[index] + ".\n");
                    gmcp_char(GMCP_CHAR_STATUS, SD_LONG_STAT_DESC[index], "unbelievable");
                    ACHIEVEMENTS->trigger_event_stat_level(this_object(), index, "immortal", "unbelievable");
                }
            }
            if (new_stats[index] >= SD_STATLEVEL_INC)
            {
                if (last_stats[index] < SD_STATLEVEL_INC)
                {
                    tell_object(this_object(), "You have reached incredible " +
                        SD_LONG_STAT_DESC[index] + ".\n");
                    gmcp_char(GMCP_CHAR_STATUS, SD_LONG_STAT_DESC[index], "incredible");
                    ACHIEVEMENTS->trigger_event_stat_level(this_object(), index, "immortal", "incredible");
                }
            }
            if (new_stats[index] >= SD_STATLEVEL_EXT)
            {
                if (last_stats[index] < SD_STATLEVEL_EXT)
                {
                    tell_object(this_object(), "You have reached extraordinary " +
                        SD_LONG_STAT_DESC[index] + ".\n");
                    gmcp_char(GMCP_CHAR_STATUS, SD_LONG_STAT_DESC[index], "extraordinary");
                    ACHIEVEMENTS->trigger_event_stat_level(this_object(), index, "immortal", "extraordinary");
                }
            }
            if (new_stats[index] >= SD_STATLEVEL_SUP)
            {
                if (last_stats[index] < SD_STATLEVEL_SUP)
                {
                    tell_object(this_object(), "You have reached supreme " +
                        SD_LONG_STAT_DESC[index] + ".\n");
                    gmcp_char(GMCP_CHAR_STATUS, SD_LONG_STAT_DESC[index], "supreme");
                    ACHIEVEMENTS->trigger_event_stat_level(this_object(), index, "immortal", "supreme");
                }
            }
            if (new_stats[index] >= SD_STATLEVEL_IMM)
            {
                if (last_stats[index] < SD_STATLEVEL_IMM)
                {
                    tell_object(this_object(), "You have reached the " +
                        SD_LONG_STAT_DESC[index] + " of an immortal.\n");
                    gmcp_char(GMCP_CHAR_STATUS, SD_LONG_STAT_DESC[index], "immortal");
                    ACHIEVEMENTS->trigger_event_stat_level(this_object(), index, "epic", "immortal");
                }
            }
            if (new_stats[index] >= SD_STATLEVEL_EPIC)
            {
                if (last_stats[index] < SD_STATLEVEL_EPIC)
                {
                    tell_object(this_object(), "You have reached epic " +
                        SD_LONG_STAT_DESC[index] + ".\n");
                    gmcp_char(GMCP_CHAR_STATUS, SD_LONG_STAT_DESC[index], "epic");
                    olddesc = GET_STAT_LEVEL_DESC(index, last_stats[index]);
                    ACHIEVEMENTS->trigger_event_stat_level(this_object(), index, olddesc, "epic");
                }
            }
            else
            {
                olddesc = GET_STAT_LEVEL_DESC(index, last_stats[index]);
                newdesc = GET_STAT_LEVEL_DESC(index, new_stats[index]);
                if (olddesc != newdesc)
                {
                    /* In some cases, this can be a lowering. */
                    tell_object(this_object(), "Your " +
                        SD_LONG_STAT_DESC[index] + " " +
                        ((new_stats[index] > last_stats[index]) ? "in" : "de") +
                        "creases from " + olddesc + " to " + newdesc + ".\n");
                    gmcp_char(GMCP_CHAR_STATUS, SD_LONG_STAT_DESC[index], newdesc);
                    if (new_stats[index] > last_stats[index])
                    {
                        ACHIEVEMENTS->trigger_event_stat_level(this_object(), index, olddesc, newdesc);
                    }
                }
            }
        }
    }

    index = SS_NO_EXP_STATS;
    new_stats[index] = query_average_stat();
    if (new_stats[index] != last_stats[index])
    {
        olddesc = GET_EXP_LEVEL_DESC(last_stats[index], query_gender());
        newdesc = GET_EXP_LEVEL_DESC(new_stats[index], query_gender());
        if (olddesc != newdesc)
        {
            if (new_stats[index] > last_stats[index])
            {
                tell_object(this_object(), "Congratulations. You are now " +
                    "sufficiently experienced to call yourself " + newdesc +
                    ".\n");
                ACHIEVEMENTS->trigger_event_mortal_level(this_object(), olddesc, newdesc);
            }
            else
            {
                tell_object(this_object(), "The loss of experience causes " +
                    "you to drop from " + olddesc + " to " + newdesc + ".\n");
            }
            gmcp_char(GMCP_CHAR_STATUS, GMCP_LEVEL, newdesc);
        }
    }
    last_stats = new_stats;

    /* Update client about the progress indicators. */
    if (quest)
    {
        gain = query_exp_quest_gain_desc();
        gmcp_char(GMCP_CHAR_VITALS, GMCP_QPROGRESS, (gain ? gain : SD_NO_PROGRESS));
    }
    else
    {
        gain = query_exp_gain_desc();
        gmcp_char(GMCP_CHAR_VITALS, GMCP_PROGRESS, (gain ? gain : SD_NO_PROGRESS));
    }
}

