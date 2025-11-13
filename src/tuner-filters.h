#ifndef XDR_TUNER_FILTERS_H_
#define XDR_TUNER_FILTERS_H_

gint tuner_filter_from_index(gint);
gint tuner_filter_bw(gint);

gint tuner_filter_bw_from_index(gint);
gint tuner_filter_index_from_bw(gint);
gint tuner_filter_count();

#endif
