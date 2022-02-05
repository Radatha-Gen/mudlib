/*
 * /sys/global/formulas.c
 *
 * Formulas which have become too large for formulas.h
 */
#pragma strict_types

#include <formulas.h>

static mapping at_weight_factor = ([
    -A_SHIELD & ~W_RIGHT: 20,
    A_BODY: 45,
    A_LEGS: 20,
    A_HEAD: 15,
    A_R_FOOT: 5,
    A_L_FOOT: 5,
    A_R_ARM: 10,
    A_L_ARM: 10,
    A_R_HAND: 3,
    A_L_HAND: 3,
    A_ROBE: 20
]);

public nomask int
weight_default_armour(int ac, int at)
{
    /* Use on of the components for the any-cases */
    if (at < 0) {
        at = -at & ~(W_RIGHT|A_R_FOOT|A_R_ARM|A_R_HAND);
    }

    int factor = 0;
    foreach (int bit, int weight: at_weight_factor) {
        if (at & bit) {
            factor += weight;
        }
    }

    return factor * (428 * max(ac - 1, 1) + (((ac) > 14) ? 10000 : 0)) / 100;
}

/*
 * Function name: delta_align_on_kill
 * Called from  : F_KILL_ADJUST_ALIGN(k_al, v_al) in <formulas.h>
 * Description  : This function returns the adjustment of the alignment of the
 *                killer when he has killed something.
 * Arguments    : int k_align - the current alignment of the killer.
 *                int v_align - the alignment of the victim.
 * Returns      : int - the alignment adjustment, either positive or negative.
 */
int
delta_align_on_kill(int k_align, int v_align)
{
    if (v_align > 0)
    {
        return ((k_align > 0) ? (v_align / -5) :
            -((v_align * 4000) / ((k_align * k_align) + 20000)));
    }

    if (k_align >= 400)
    {
        return (v_align - 600) / -k_align;
    }

    if (k_align <= -400)
    {
        return v_align * 8 / k_align;
    }

    return v_align / -50;
}

/*
 * Function name: exp_on_kill
 * Called from  : F_EXP_ON_KILL(k_av, v_av) in <formulas.h>
 * Description  : This is the amount of experience a person gets when he kills
 *                another living. The amount is dependant on both the size of
 *                the killer and that of the target.
 * Arguments    : int ka - the average stat of the killer.
 *                int va - the average stat of the victim.
 * Returns      : int - the experience.
 */
int
exp_on_kill(int ka, int va)
{
    float fka;
    float fva;

    /* New formulae, rewritten for optimal calculation. We need to convert to
     * floats to avoid MAXINT limitations.
     * Respectively for va > ka and ka >= va.
     * 400 * (va * va * va) / (((va + ka * 11) / 12) * (va + 50))
     * 400 * (va * va * va) / (((va + ka) / 2) * (ka + 50))
     * Original formula:
     * 400 * (va * va) / (va + 50)
     */
    fva = itof(va);
    fka = itof(ka);

    if (va > ka)
    {
        return ftoi(4800.0 * (fva * fva * fva) / (fva + fka * 11.0) /
            (fva + 50.0));
    }
    else
    {
        return ftoi(800.0 * (fva * fva * fva) / (fva + fka) / (fka + 50.0));
    }
}
