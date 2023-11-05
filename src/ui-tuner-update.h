#ifndef XDR_UI_TUNER_UPDATE_H_
#define XDR_UI_TUNER_UPDATE_H_
#include "tuner.h"
#include "tuner-scan.h"

void ui_update_freq();
void ui_update_mode();
void ui_update_stereo_flag();
void ui_update_rds_flag();
void ui_update_signal();
void ui_update_cci();
void ui_update_cci_peak();
void ui_update_aci();
void ui_update_aci_peak();

void ui_update_rds_init();

void ui_update_pi();
void ui_update_tp();
void ui_update_ta();
void ui_update_ms();
void ui_update_pty();
void ui_update_country();
void ui_update_ps();
void ui_update_rt(gboolean);
void ui_update_af(gint);

void ui_update_bandwidth();
void ui_update_rotator();
void ui_update_forced_mono();
void ui_update_scan(tuner_scan_t*);
void ui_update_disconnected();
void ui_update_pilot();
void ui_action();
void ui_unauthorized();
void ui_unauthorized();
void ui_clear_af();

void ui_update_service();

#endif
