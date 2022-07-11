#ifndef UTILS_H___
#define UTILS_H___

#include <stdint.h>
#include <plist/plist.h>

int extract_memory(const char *ipsw, const char* internal_path, unsigned char **buf, uint64_t *len);

int extract_file(const char* ipsw, const char* internal_path, const char* out_path);


/////////////////////////////////////////////////////////
/// Helpers
plist_t extract_buildmanifest_plist(const char* ipsw);
plist_t find_buildid(plist_t buildmanifest, const char* hardware_model, bool isErase);
int     find_component_path(plist_t build_id, const char* comp, const char** internal_path);
/////////////////////////////////////////////////////////

#endif
