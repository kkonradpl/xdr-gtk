#ifndef XDR_RDS_UTILS_H_
#define XDR_RDS_UTILS_H_
#include <librdsparser.h>

gchar* rds_utils_text(const rdsparser_string_t*);
gchar* rds_utils_text_markup(const rdsparser_string_t*, gboolean);

#endif

