#ifndef XDR_LOG_H_
#define XDR_LOG_H_

void log_cleanup();
void log_pi(gint, gboolean);
void log_af(const gchar*);
void log_ps(const gchar*, const guchar*);
void log_rt(guint8, const gchar*);
void log_pty(const gchar*);
void log_ecc(const gchar* ecc, guint);
gchar* replace_spaces(const gchar*);

#endif


