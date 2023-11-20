#ifndef XDR_RDS_UTILS_H_
#define XDR_RDS_UTILS_H_

const gchar* rds_utils_pty_to_string(gboolean, gint);
gchar* rds_utils_text_markup(const gchar*, const librds_string_error_t*, gboolean);

#endif

