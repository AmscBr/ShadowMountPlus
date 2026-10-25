#ifndef SM_PKG_NATIVE_H
#define SM_PKG_NATIVE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include "sm_storage.h"

// AppInstUtil output sizes verified against the console's 9.40 library.
typedef struct { char content_id[48]; int type, platform; } sm_pkg_native_info_t;
typedef struct {
  int32_t code, version;
  char description[512], type[9];
} sm_pkg_native_error_t;
typedef struct {
  char status[16], source[8];
  float remaining_seconds;
  uint64_t downloaded, initial_chunk, total;
  uint32_t promotion;
  unsigned char reserved[100];
  sm_pkg_native_error_t error;
  char patch_version[12];
  int32_t local_copy_percent;
  bool copy_only;
} sm_pkg_native_status_t;

typedef struct {
  const char *uri, *ex_uri, *scenario, *content_id, *name, *icon;
  uint64_t reserved[8];
} sm_pkg_native_metadata_t;
typedef struct {
  char languages[30][8], scenarios[64][3], content_ids[64][48];
  unsigned char reserved[6488];
} sm_pkg_native_playgo_t;
_Static_assert(sizeof(sm_pkg_native_metadata_t) == 0x70, "AppInstUtil metadata ABI");
_Static_assert(sizeof(sm_pkg_native_info_t) == 0x38, "AppInstUtil info ABI");
_Static_assert(sizeof(sm_pkg_native_playgo_t) == 0x2708, "AppInstUtil PlayGo ABI");
_Static_assert(sizeof(sm_pkg_native_status_t) == 0x2c8, "AppInstUtil status ABI");
_Static_assert(offsetof(sm_pkg_native_status_t, downloaded) == 0x20, "AppInstUtil download offset");
_Static_assert(offsetof(sm_pkg_native_status_t, error) == 0xa0, "AppInstUtil error offset");
_Static_assert(offsetof(sm_pkg_native_status_t, local_copy_percent) == 0x2c0, "AppInstUtil copy offset");
int sceAppInstUtilInstallByPackage(sm_pkg_native_metadata_t *metadata,
    sm_pkg_native_info_t *info, sm_pkg_native_playgo_t *playgo);
int sceAppInstUtilGetInstallStatus(const char *content_id, sm_pkg_native_status_t *status);
int sceAppInstUtilCancelInstall(const char *content_id);
#endif
