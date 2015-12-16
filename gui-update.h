#ifndef XDR_GUI_UPDATE_H_
#define XDR_GUI_UPDATE_H_

gboolean gui_update_freq(gpointer data);
gboolean gui_update_signal(gpointer data);
gboolean gui_update_pi(gpointer data);
gboolean gui_update_af(gpointer data);
gboolean gui_update_af_check(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer* newfreq);
gboolean gui_update_ps(gpointer nothing);
gboolean gui_update_rt(gpointer flag);
gboolean gui_update_tp(gpointer data);
gboolean gui_update_ta(gpointer data);
gboolean gui_update_ms(gpointer data);
gboolean gui_update_pty(gpointer data);
gboolean gui_update_ecc(gpointer data);
gboolean gui_update_volume(gpointer data);
gboolean gui_update_squelch(gpointer data);
gboolean gui_update_agc(gpointer data);
gboolean gui_update_deemphasis(gpointer data);
gboolean gui_update_ant(gpointer data);
gboolean gui_update_gain(gpointer data);
gboolean gui_update_filter(gpointer data);
gboolean gui_update_rotator(gpointer data);
gboolean gui_update_alignment(gpointer data);
gboolean gui_clear_power_off();
gboolean gui_update_pilot(gpointer data);
gboolean gui_external_event(gpointer data);

#endif
