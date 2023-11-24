#ifndef XDR_RDS_UTILS_H_
#define XDR_RDS_UTILS_H_
#include "librds.h"

const gchar* rds_utils_pty_to_string(gboolean, gint);
gchar* rds_utils_text(const librds_string_t*);
gchar* rds_utils_text_markup(const librds_string_t*, gboolean);

#endif

