#ifndef INCLUDED_UNICLOGS_API_H
#define INCLUDED_UNICLOGS_API_H

#include <gnuradio/attributes.h>

#ifdef gnuradio_uniclogs_EXPORTS
#define UNICLOGS_API __GR_ATTR_EXPORT
#else
#define UNICLOGS_API __GR_ATTR_IMPORT
#endif

#endif /* INCLUDED_UNICLOGS_API_H */
