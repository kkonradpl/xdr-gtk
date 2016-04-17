#include <glib.h>
#include <stdlib.h>
#include "tuner.h"

static const gint filters[] =
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

#define FILTERS_COUNT (sizeof(filters) / sizeof(gint))
static const gint filters_bw[][FILTERS_COUNT] =
{
    { /* FM */
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
            -1
    },
    { /* AM */
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
    }
};

gint
tuner_filter_from_index(gint index)
{
    if(index >= 0 && index < FILTERS_COUNT)
        return filters[index];
    return -1; /* unknown -> adaptive */
}

gint
tuner_filter_index(gint filter)
{
    gint i;
    for(i=0; i<FILTERS_COUNT; i++)
        if(filters[i] == filter)
            return i;
    return -1; /* unknown -> none set */
}

gint
tuner_filter_bw(gint filter)
{
    gint position = tuner_filter_index(filter);
    if(position >= 0 && (tuner.mode == MODE_FM || tuner.mode == MODE_AM))
        return filters_bw[tuner.mode][position];
    return 0; /* unknown bandwidth */
}

gint
tuner_filter_bw_from_index(gint index)
{
    if(index >= 0 && index < FILTERS_COUNT && (tuner.mode == MODE_FM || tuner.mode == MODE_AM))
        return filters_bw[tuner.mode][index];
    return 0; /* unknown bandwidth */
}

gint
tuner_filter_index_from_bw(gint bw)
{
    gint index = FILTERS_COUNT-1;
    gint min = G_MAXINT;
    gint diff, i;

    if(bw >= 0)
    {
        for(i=0; i<FILTERS_COUNT-1; i++)
        {
            diff = abs(tuner_filter_bw_from_index(i)-bw);
            if(min > diff)
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
    return FILTERS_COUNT;
}
