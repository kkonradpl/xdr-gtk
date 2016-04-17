#ifndef XDR_UI_SIGNAL_H_
#define XDR_UI_SIGNAL_H_

void signal_init();
void signal_resize();

void signal_push(gfloat, gboolean, gboolean, gboolean);
void signal_separator();
void signal_clear();

gfloat signal_level(gfloat);
const gchar* signal_unit();
void signal_display();

#endif
