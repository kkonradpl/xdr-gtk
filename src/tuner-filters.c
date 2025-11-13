#include <glib.h>
#include <stdlib.h>
#include "tuner.h"
#include "conf.h"

static const gint filters_legacy[] =
{
    15,
    14,
    13,
    12,
    11,
    10,
    9,
    8,
    7,
    6,
    5,
    4,
    3,
    29,
    2,
    28,
    1,
    26,
    0,
    24,
    23,
    22,
    21,
    20,
    19,
    18,
    17,
    16,
    31,
    -1
};

static const gint filters_bw_fm_xdr[] =
{
    /* FM */
    309000,
    298000,
    281000,
    263000,
    246000,
    229000,
    211000,
    194000,
    177000,
    159000,
    142000,
    125000,
    108000,
    95000,
    90000,
    83000,
    73000,
    63000,
    55000,
    48000,
    42000,
    36000,
    32000,
    27000,
    24000,
    20000,
    17000,
    15000,
    9000,
    0
};

static const gint filters_bw_am_xdr[] =
{
    38600,
    37300,
    35100,
    32900,
    30800,
    28600,
    26400,
    24300,
    22100,
    19900,
    17800,
    15600,
    13500,
    11800,
    11300,
    10400,
    9100,
    7900,
    6900,
    6000,
    5200,
    4600,
    3900,
    3400,
    2900,
    2500,
    2200,
    1900,
    1100,
    0
};

static const gint filters_bw_fm_tef[] =
{
    311000,
    287000,
    254000,
    236000,
    217000,
    200000,
    184000,
    168000,
    151000,
    133000,
    114000,
    97000,
    84000,
    72000,
    64000,
    56000,
    0
};

static const gint filters_bw_am_tef[] =
{
    8000,
    6000,
    4000,
    3000,
    0
};

#define FILTERS_LEGACY_COUNT (sizeof(filters_legacy) / sizeof(filters_legacy[0]))
G_STATIC_ASSERT(sizeof(filters_legacy) == sizeof(filters_bw_fm_xdr));
G_STATIC_ASSERT(sizeof(filters_legacy) == sizeof(filters_bw_am_xdr));


static const gint*
tuner_filter_get_array(gint *len_out)
{
    if (tuner.mode == MODE_FM)
    {
        if (conf.tef668x_mode)
        {
            *len_out = sizeof(filters_bw_fm_tef) / sizeof(filters_bw_fm_tef[0]);
            return filters_bw_fm_tef;
        }
        else
        {
            *len_out = sizeof(filters_bw_fm_xdr) / sizeof(filters_bw_fm_xdr[0]);
            return filters_bw_fm_xdr;
        }
    }
    else if (tuner.mode == MODE_AM)
    {
        if (conf.tef668x_mode)
        {
            *len_out = sizeof(filters_bw_am_tef) / sizeof(filters_bw_am_tef[0]);
            return filters_bw_am_tef;
        }
        else
        {
            *len_out = sizeof(filters_bw_am_xdr) / sizeof(filters_bw_am_xdr[0]);
            return filters_bw_am_xdr;
        }
    }

    *len_out = 0;
    return NULL;
}

gint
tuner_filter_from_index(gint index)
{
    /* For legacy 'F' message */
    if(index >= 0 && index < FILTERS_LEGACY_COUNT)
        return filters_legacy[index];

    return -1; /* unknown -> adaptive */
}

gint
tuner_filter_bw(gint filter)
{
    /* From legacy 'F' message */
    /* Always use the legacy table with XDR bandwidths */
    for (gint i = 0; i < FILTERS_LEGACY_COUNT; i++)
    {
        if (filters_legacy[i] == filter)
        {
            if (tuner.mode == MODE_FM)
                return filters_bw_fm_xdr[i];
            else if (tuner.mode == MODE_AM)
                return filters_bw_am_xdr[i];
            break;
        }
    }

    return 0; /* unknown bandwidth */
}

gint
tuner_filter_bw_from_index(gint index)
{
    gint count;
    const gint *array = tuner_filter_get_array(&count);

    if (index < 0 ||
        index >= count)
    {
        return 0;
    }

    return array[index];
}

gint
tuner_filter_index_from_bw(gint bw)
{
    gint count;
    const gint *array = tuner_filter_get_array(&count);

    gint index = count - 1;
    gint min = G_MAXINT;
    gint diff, i;

    if (bw > 0)
    {
        for (i = 0; i < count - 1; i++)
        {
            diff = abs(array[i] - bw);
            if (min > diff)
            {
                min = diff;
                index = i;
            }
        }
    }

    return index;
}

gint
tuner_filter_count()
{
    gint count;
    tuner_filter_get_array(&count);

    return count;
}
