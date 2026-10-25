# ShadowMount HTTP/JSON API v1

The machine-readable OpenAPI 3.1 description is available in
[`openapi.yaml`](openapi.yaml).

ShadowMount exposes its public API over HTTP/TCP. By default it listens on
`0.0.0.0:10101`, allowing clients on the local network to connect through the
PS5's IPv4 address. Set `api_bind_address=127.0.0.1` for PS5-only access, or
select a specific PS5 IPv4 address. Existing explicit settings are preserved.
`api_port` accepts values from 1 through 65535.
These settings are applied on runtime config reload. `api_enabled` defaults to
`1`; setting it to `0` stops the listener and joins the libmicrohttpd service
and worker threads. Re-enabling it restarts the HTTP service without restarting
the payload. An already-running storage job is allowed to finish, but no new
jobs can be submitted while the API is disabled.

`/data/shadowmount/index.html` overrides the web interface bundled into the
payload. If the file is absent, the same `web/index.html` built into the ELF is
served automatically, so enabling the API always provides a landing page.

The API has no authentication. Bind it to a network-facing address only on a
trusted network.

The internal `/system_tmp/shadowmount.sock` Unix socket remains private to the
SceShellCore bridge and must not be used by third-party applications.

The HTTP service uses libmicrohttpd with eight fixed polling workers. It accepts
up to 32 concurrent connections, uses a listen backlog of 32 and closes idle
connections after 15 seconds. Slow clients therefore do not serialize unrelated
requests or create an unbounded number of threads. If daemon startup fails, the
service retries with an interruptible 250 ms to 5 s backoff. Suspend, shutdown
and config reload stop the daemon cleanly; resume recreates it.

Every JSON API operation uses `POST`, includes `Content-Type: application/json`
and contains one JSON object. `GET /` returns the optional web interface and the
icon endpoint returns PNG bytes. Connections are closed after one response;
chunked request bodies are not supported. The maximum request body is 4096 bytes.
The parsed request-header limit remains 8192 bytes.

All responses include `Access-Control-Allow-Origin: *`. Browser preflight
requests using `OPTIONS` are supported and allow `GET`, `POST`, the `Content-Type`
header and Private Network Access. This permits calls from any browser origin;
the listener still needs `api_bind_address=0.0.0.0` for access from another
device.

## Routes

| Route | Request | Purpose |
| --- | --- | --- |
| `/` | GET | Serve `/data/shadowmount/index.html`, or the embedded page when it is absent |
| `/api/v1/version` | `{}` | API and ShadowMount versions plus capabilities |
| `/api/v1/storage` | `{}` | Mounted storage filesystems with total, free and available space |
| `/api/v1/images` | `{}` | Complete image snapshot |
| `/api/v1/scan` | `{"reset_attempts":false}` | Queue an immediate full rescan; optionally reset title and image retry counters |
| `/api/v1/manual/list` | `{}` | List direct paths to individual game folders or images from `manual.lst` |
| `/api/v1/manual/add` | `{"path":"/mnt/usb0/games/PPSA12345"}` | Idempotently add one game source to `manual.lst` |
| `/api/v1/manual/remove` | `{"path":"/mnt/usb0/games/PPSA12345"}` | Remove matching source lines from `manual.lst` |
| `/api/v1/settings` | `{}` | Read web-managed runtime settings and scan paths |
| `/api/v1/settings/update` | `{"debug":true,"quiet_mode":false,"update_emulators":true,"fakelib_default_mode":"full","auto_update_ampr":false,"auto_remove_missing_games":false,"auto_remove_missing_delay_seconds":300,"allow_lan_access":true,"fan_target_temperature":0,"scan_paths":["/mnt/usb0/games"]}` | Atomically update the web-managed part of `config.ini` |
| `/api/v1/debug-log` | `{"max_bytes":131072}` | Read a bounded tail of `debug.log` for the web log dialog |
| `/api/v1/kernel-log` | `{"max_bytes":131072}` | Read a bounded tail of the SDK kernel-log stream used by crash detection |
| `/api/v1/games` | `{"include_size":false}` | Detailed game snapshot; optional physical source-size calculation |
| `/api/v1/games/info` | `{"title_id":"PPSA12345"}` | Detailed app.db and source information for one game; size is always calculated |
| `/api/v1/games/fakelib` | `{"title_id":"PPSA12345","mode":"emulators"}` | Persist the PS5 game's fakelib policy for its next launch |
| `/api/v1/games/fakelib` | `{"title_id":"PPSA12345","ignore_global_fakelib":true}` | Ignore the global fakelib overlay for this PS5 game on its next launch |
| `/api/v1/games/icon?title_id=PPSA12345[&size=thumb\|card\|cover]` | GET | Stream the original PNG or a cached icon variant |
| `/api/v1/games/mount` | `{"title_id":"PPSA12345","mode":"ro"}` | Mount a managed game, optionally overriding its image mode with `ro`/`rw` |
| `/api/v1/games/unmount` | `{"title_id":"PPSA12345"}` | Unmount a managed game |
| `/api/v1/games/uninstall` | `{"title_id":"PPSA12345"}` | Request uninstallation through AppInstUtil |
| `/api/v1/games/content/uninstall` | `{"title_id":"PPSA12345","kind":"patch"}` or `{"title_id":"PPSA12345","kind":"dlc","content_id":"…"}` | Remove only the patch or the specified installed DLC through AppInstUtil |
| `/api/v1/games/move` | `{"title_id":"PPSA12345","destination_dir":"/mnt/usb1/games"}` | Start an asynchronous move job |
| `/api/v1/games/copy` | `{"title_id":"PPSA12345","destination_dir":"/mnt/usb1/games"}` | Start an asynchronous copy job |
| `/api/v1/games/unpack` | `{"title_id":"PPSA12345","destination_dir":"/mnt/usb1/games","delete_source":false}` | Mount an image read-only and asynchronously copy the game into a folder |
| `/api/v1/games/storage/status` | `{"job_id":1}` | Get current or last storage job status; `job_id` is optional |
| `/api/v1/games/storage/cancel` | `{"job_id":1}` | Request cancellation while the active job is still cancellable |
| `/api/v1/games/delete` | `{"title_id":"PPSA12345","confirm":true}` | Start asynchronous permanent deletion of the physical source |

Example:

```sh
curl -sS http://127.0.0.1:10101/api/v1/version \
  -H 'Content-Type: application/json' \
  -d '{}'

curl -sS http://127.0.0.1:10101/api/v1/games \
  -H 'Content-Type: application/json' \
  -d '{}'

curl -sS http://127.0.0.1:10101/api/v1/scan \
  -H 'Content-Type: application/json' \
  -d '{}'

curl -sS http://127.0.0.1:10101/api/v1/games/mount \
  -H 'Content-Type: application/json' \
  -d '{"title_id":"PPSA12345","mode":"rw"}'
```

An autonomous example library UI is provided as `web/index.html`. Copy it to
`/data/shadowmount/index.html` on the PS5 and open the configured API listener
in a browser, for example `http://192.168.1.50:10101/`. The file is read for
each request, so it can be replaced without restarting ShadowMount. If it is
missing, `GET /` returns HTTP 404 with the normal JSON error response.
The bundled interface covers all 31 console locales, selects the closest match
from the browser locale, and keeps a manually selected locale in local storage.

## Responses

Every response contains numeric `status`. It is zero on success, or a positive
errno value on failure. Error responses also contain `error` and use an
appropriate HTTP status such as 400, 404, 409, 413 or 415.

A successful version response has this shape:

```json
{
  "status": 0,
  "api_version": 1,
  "shadowmount_version": "1.7",
  "capabilities": ["web_ui", "storage_space", "list_images", "list_games", "game_info", "game_data_sizes", "game_icon", "mount_game", "unmount_game", "uninstall_game", "uninstall_game_content", "move_game_source", "copy_game_source", "delete_game_source", "unpack_game_image", "storage_job_status", "storage_job_cancel", "list_manual_sources", "add_manual_source", "remove_manual_source", "manage_settings", "read_debug_log", "read_kernel_log", "rescan"]
}
```

List responses contain `count` and the complete `images` or `games` array.
Confirmed running `FAKE*` and `LAPY*` applications do not defer scans or block
source copy/move/delete operations and uninstalls for unrelated titles. This
applies to folder, image and installed-PKG applications. Ordinary PS4/PS5 games
allow the same work while AppFocus confirms they are in the background. The
live application's own title, source, and backing image chain remain protected;
operations affecting them return `game_active`. Returning to an ordinary game,
unknown focus, and pending launches/exits defer new background work. Focus
changes wake the scanner; unblocking makes the next full resync due immediately.

Image items expose `path`, `mount_point`, `size`, modification time, `unit_id`,
`backend`, `complete`, `source_available`, `mapped` and `mounted`. Game items
combine ShadowMount's game cache with `tbl_contentinfo` from app.db. They expose
the physical `path`, `runtime_path`, `source_type` (`folder` or `image`), image
filesystem type, PS4/PS5 platform, title/content IDs, name, last-launch and
install timestamps, game `version`, relative `icon_url`, app.db size, and runtime state.
`backport_path` contains the directory found by the shared backport resolver,
or an empty string when absent. It respects configured roots, the internal
fallback and title-prefix matching. This reports presence independently of
fakelib policy or whether an overlay is currently mounted. The library shows
a Backport badge and details display the resolved path.
Icon requests without `size` return the original PNG. `size=thumb` retains the
legacy PNG variant capped at 128 pixels. `size=card` and `size=cover` are capped
at 256 and 512 pixels: opaque icons use JPEG quality 90 with 4:4:4 sampling;
icons containing alpha remain PNG. If JPEG would exceed the source PNG size,
a PNG candidate is compared and retained when smaller (the original bytes
are reused when no resizing is needed). The response Content-Type is `image/jpeg`
or `image/png` as appropriate. Aspect ratio is preserved and small sources
are never enlarged. Resizing averages source areas with premultiplied alpha.
The web UI selects card/cover using responsive image sizes and screen density,
loads covers lazily, and falls back to the original if a cached request fails.

Variants are generated only on demand and published through unique temporary
files plus atomic rename. Cache v3 keys include the variant size and a source
fingerprint (path, device/inode, size and nanosecond modification time); the
same fingerprint and cache version also version the public icon URL. A source
changed during encoding is not published under its old key. On a cache miss,
obsolete generations and abandoned temporary files older than five minutes
for that title are removed, while its current variants remain. No worker or
periodic cache scan is added. Generation stops on shutdown/rest requests, and
an unavailable cache falls back to serving the original PNG.

`version` comes from `AppInfoJson` (`CONTENT_VERSION`, falling back to `APP_VER`)
and is an empty string when unavailable.
The timestamp values from `AppInfoJson` (`#_last_access_time` and
`#_install_time`) take precedence over the stale top-level columns when they
are present.

`POST /api/v1/games` does not walk game directories by default. Set
`include_size=true` to add `size_status` and, on success, `size_bytes` to every
item. `POST /api/v1/games/info` always performs this calculation. An image uses
its file size; a folder uses the sum of its regular-file sizes. Details measure
the physical base `app.pkg` for installed packages. The library list can use
the recorded installed total for those titles, while `app_db_size_bytes`
remains the system reference in both responses.

`games/info` also returns `save_data_size_status`, `download_size_status`,
`patch_size_status` and `dlc_size_status`.
On success (status `0`), the matching `save_data_size_bytes`,
`download_size_bytes`, `patch_size_bytes` or `dlc_size_bytes` field contains
the sum of regular-file lengths. Missing
directories are successful zero sizes; errors omit the corresponding byte count.
PS5 saves are read from `/user/home/<8-hex-user>/savedata_prospero/<title_id>`;
other supported titles use `savedata/<title_id>`. Saves are summed across local
profiles and include backups within the title directory, but exclude cloud,
metadata and profile-level backup folders. Download data uses
`/user/download/<title_id>`. Symlink directories are not followed. Traversal
cooperatively stops on payload shutdown or rest mode. These counts run only for
the selected title in `games/info`, never during library listing or scanning.
Patch and DLC totals walk only the selected title directories under
`/user/patch` and `/user/addcont`, plus matching paths beneath `/mnt/ext0`,
`/mnt/ext0/ps5`, `/mnt/ext1` and `/mnt/ext1/ps5`. The same directory identity
(device/inode) is counted once across these roots. Missing directories return
zero; any access, cancellation or measurement failure returns that category's
errno-style status and omits its bytes, without publishing a partial total.
Symlink directories are not followed. Separate patch files are measured;
updates merged into the base package remain part of the game's main size.

The web detail dialog shows a compact colored bar and a labeled legend for
the source, patches, DLC, saves and downloads. Their sum is shown only when
all five measurements succeed. The app.db figure is displayed separately as
a system reference and is excluded from this total.

Details also return `patch_info_status`, a `patch` object (`installed`,
`version`, `size_bytes`), `dlc_info_status`, a `dlc` array and
`dlc_info_truncated`. Patch metadata uses `GetPatchInstallStatus`; DLC records
use `GetAddcontInfoList2` with 16-entry pages and a 256-record cap. The native
list's returned count is the total matching count, not the page length. DLC
records contain `content_id`, `title_name`, `version`, `size_status` and, when
valid, `size_bytes`. Native errors leave filesystem size results intact.
Library entries include `patch_info_status` and `dlc_info_status`, with
`has_patch` and `dlc_count` present only when their respective query succeeds.
These use GetPatchInstallStatus and AppGetAddcontNum, without file size scans
or DLC enumeration. Since patch download history may be absent for an installed
patch, presence also checks regular, nonempty `patch.pkg` files in the same
five internal/ext0/ext1 roots used for patch sizes. This rule is shared by
the library version marker and detail card; installation progress still uses native
download status alone. In the filesystem fallback, the existing game version
and measured patch size supply detail metadata.
The library shows a localized patch marker beside the version and a DLC badge
with the count; failed queries do not display a badge.

The web dialog appends the localized original version in parentheses beside
the current version when a patch is installed. `base_version` is read from the
base PKG only for game details, with title ID and base-PKG classification
checks. Patch detection and original-version annotations apply only to PKG games. Empty means the original version
could not be read; the UI displays unavailable rather than assuming 1.00.
A button with an SVG
disclosure arrow expands DLC names, content IDs, versions and system-recorded sizes.
Games without listed DLC do not show an empty disclosure button. Expanded
lists may scroll within the detail body; the bottom action row stays visible.

Patch removal is available in the bottom action row; each expanded DLC row has its
own delete button. Both actions require confirmation and reload the card's
metadata and sizes. The content uninstall route checks runtime/scanner guards
and serializes against storage submissions. DLC deletion passes the content ID
suffix in a zero-padded 32-byte directory-name buffer to AppUnInstallAddcont;
patch deletion calls AppUnInstallPat. Neither requests whole-game removal.
The `uninstall_game_content` capability advertises the route.

`system_content_size_status` and, on success, `system_content_size_bytes`
expose `AppGetSize` for comparison. This is a system-accounted installed total,
not the physical source size.

`GetContentSize` with a base-content record returned the system-accounted
total too; `GetContentSize2` returned `0x80F50005` on the tested titles.
For DEATHLOOP's DLC the native record reported 1,245,184 bytes while its files
totalled 1,246,202 bytes. These different accounting scopes are why the size
bar retains filesystem counts; native totals never replace folder/image sizes
or get added to the bar a second time. These calls add no background polling.

The `game_data_sizes` capability advertises these fields.

`POST /api/v1/storage` returns capacity-bearing mounted filesystems and omits
virtual mounts such as `devfs`, `nullfs` and `tmpfs`. Each item contains the
device `source`, `mount_point`, `filesystem`, `total_bytes`, `free_bytes`,
`available_bytes`, `used_bytes` and `read_only`. `available_bytes` is the space
available for new files; `used_bytes` is calculated against that value. The
`destinations` array contains only writable, currently available managed
`/data/homebrew`, `/mnt/ext*/homebrew` and `/mnt/usb*/homebrew` paths. Scanned
`/mnt/ext*` and `/mnt/usb*` roots are included as additional destinations. Each
item adds the selectable `path` to the same filesystem metadata. Writable nullfs
aliases such as `/data/homebrew` are valid destinations but remain omitted from
the mount overview to avoid duplicates. `/mnt/ext1/homebrew` is also advertised
when its writable ext1 mount is available and only the homebrew folder is absent;
listing storage does not create it. The folder is created on an explicit
copy/move/unpack request. Other missing destination directories remain invalid.
For USB game sources, the web picker places `/data/homebrew` and an available
`/mnt/ext1/homebrew` before other choices.

The web storage overview uses `mounts` to show the internal `/user` filesystem
and mounted `/mnt/ext0..1` and `/mnt/usb0..7` disks once per filesystem/source.
System partitions and game mounts are omitted from that overview. The separate
`destinations` list supplies folder choices for copying, moving and unpacking.

`copy` and `move` preserve the source basename and require an existing
destination under a configured non-runtime scan root (with the ext1 homebrew
creation exception above). They and `delete` return
HTTP 202 with a `job_id` and continue in one background worker;
only one of these jobs can be active. A second start returns HTTP 409/`EBUSY`.
`delete` still requires `confirm=true`. If a cached source is already absent
(`ENOENT` or `ENOTDIR`), it instead returns HTTP 200 with `source_missing=true`
and `scan_queued=true`, without creating a job or directly uninstalling the title.
The scanner reconciles stale cache and links when runtime is safe. An absent
source can also mean disconnected storage; permission and I/O errors remain
failures. Unknown titles still return HTTP 404.
The status response contains the phase,
byte-based percentage, processed/total bytes,
processed/total files, average processed-byte rate, elapsed time and final
errno-style result. The initial `measuring` phase discovers the totals and has
zero percent until they are known. Only the active or most recently finished
job is retained; an older explicit `job_id` returns HTTP 404.

`scan`, successful `uninstall`, and missing-source delete responses include
`scan_queued`, `scan_deferred`, and `scan_deferred_reason`. The reason is
`game_active`, `runtime_prepared`, `rest_mode`, or an empty string. These fields
describe the runtime at response time; queue acceptance is not scan completion.
A successful uninstall request queues a full synchronization only when its
known source was already absent before uninstall. Retained sources do not
trigger immediate rediscovery; `scan_queued=false` reports this case. Uninstall
does not exclude or delete the source, so a later scan can still discover it.
Busy uninstall/storage responses add a stable `error_reason` code; storage jobs
expose the corresponding `result_error_reason`. The English `error` and
`result_error` remain available for clients without localized catalogs. Reasons distinguish
game activity, pending installation, mutation-gate contention, prepared runtime,
and busy mount release. Close a game fully before retrying; returning to the
home screen alone does not terminate it.
If the source root disappears after a delete job is accepted, an absent root is
treated as already deleted. A missing child in an existing tree, I/O errors,
permission errors, and cancellation remain failures.

Cancellation is cooperative during `preparing`, `measuring` and `transferring`.
A partial copy destination is removed. A delete job becomes non-cancellable
when it enters `deleting`, before its first physical removal. Likewise, once a
cross-filesystem move enters
`finalizing` and starts deleting the complete source, cancellation is rejected
to avoid leaving the only source tree partially removed. The scanner and
ShellCore mutation gates remain held for the whole job and a rescan is queued
after success, cancellation or failure. `move` uses `rename()` on one
filesystem and exact copy-plus-delete across filesystems. A backing image
shared by several titles is handled as one physical source.
`unpack` uses the same worker and progress endpoint. It mounts the requested
image read-only, copies the mounted game tree to `<destination_dir>/<TITLE_ID>`,
and always releases the runtime mount afterward. `delete_source=true` removes
the image only after a complete copy and successful unmount; this irreversible
final phase is non-cancellable. Source deletion is rejected when one image is
shared by several titles.
Changing the API bind address or port restarts only the HTTP listener; an
active storage job continues and remains available through the new listener.

The game list combines discovered folder/image sources with installed PS4 and
PS5 PKGs from `app.db`, identified by an existing `app.pkg`. Title IDs appear
once; managed folder/image sources retain their normal actions. Installed PKGs
have `source_type: "pkg"`, `installed_pkg: true`, `managed: false` and
`mounted: false`. Their `path` points to `app.pkg`; `runtime_path` is empty.
They support information, icons and uninstall, plus fakelib configuration for
PS5. Mount/unmount and source copy/move/delete/unpack reject PKGs with HTTP 403
and `EPERM`. Library PKG size uses the installed size in `app.db`, falling back to the
package file size; folder/image measurement remains unchanged.

Game responses also provide `can_uninstall`, `can_manage_source`,
`can_toggle_fakelib`, `fakelib_enabled` and `fakelib_effective_enabled`.
`fakelib_mode` is the resolved mode (`full`, `emulators`, `disabled`),
`fakelib_mode_override` is the title's explicit mode or `default`, and
`fakelib_default_mode` is the current global default.
`ignore_global_fakelib` reports the title's `global_fakelib_exclude` setting.
Post `title_id` and a boolean `ignore_global_fakelib` to update only that setting;
do not combine it with `mode` or `enabled`. False removes the exclusion, including
legacy `global_fakelib_exclude_title` entries. The game's own fakelib mode stays
unchanged. Changes apply on the next launch and preserve active mounts.
The fakelib route accepts only known PS5 games, including PKGs, images and
folders. `PPSA` identifies PS5; `LAPY`/`FAKE` homebrew uses its `app.db` platform.
Send `mode: "full"`, `"emulators"` or `"disabled"` to persist an explicit override,
or `mode: "default"` to remove it and inherit `fakelib_default_mode`.
Legacy `enabled: true|false` requests remain supported and select explicit
`full` or `disabled`; `mode` takes precedence when both fields are present.
Unrelated config lines, other title overrides and global settings are preserved.
The response includes the three mode fields, `saved: true` and
`applies_on_next_launch: true`. Existing mounts stay until game exit.
`fakelib_enabled` is false when the resolved mode is `disabled`;
`fakelib_effective_enabled` also respects the global `backport_fakelib` switch.
Reaching the 128-title override limit fails without modifying the file;
removing an override remains possible at the limit.

Mount mutations remain conservative. They return HTTP 409 with `status` set to
`EBUSY` while a game is active, while ShellCore owns another prepared title,
during suspend, or while a conflicting mount/release is in progress. Clients
must not retry this response in a tight loop. HTTP 404 with `ENOENT` means the
title has no ShadowMount `mount.lnk` tracker.
The optional mount `mode` is request-scoped and does not change `config.ini`.
Accepted values are `ro`, `rw`, `r/o` and `r/w`; responses normalize them to
`ro` or `rw`. It applies to every image layer needed for that title. A direct
directory source or an image profile that is inherently read-only rejects an
explicit mode it cannot support with HTTP 501 and `ENOTSUP`. An already mounted
image in the other mode returns HTTP 409 and `EBUSY`.
With `persistent_image_mounts=1`, unmounting a game releases its per-title
runtime layers but keeps the backing image mounted.

The scan command queues the existing full-scan path and wakes the scanner. If a
game runtime mount is active, the request remains pending until the lifecycle
watcher reports that scanning is safe; it does not create a parallel scan.
By default retry counters are preserved. With `reset_attempts=true`, immediately
before the requested full scan starts it clears all title registration and
install/remount counters plus all path-owned image-mount counters. If several
scan requests merge while one is pending, one `true` request keeps the reset
enabled for that scan.

Manual entries are direct paths to individual game directories or image files;
they are not recursive scan roots. Updates atomically replace `manual.lst` and preserve comments and
unrelated lines. Adding an existing normalized path and removing a missing path
both succeed with `changed: false`. A real change queues a scan. Removing a
manual source does not uninstall its game; use the uninstall route separately
when both operations are desired.
The list route returns `{status, count, paths}`. Paths use the same trimming and
trailing-slash normalization as the scanner; comments and empty lines are not
included. A missing `manual.lst` is reported as an empty list.

The settings update owns only `debug`, `quiet_mode`, `update_emulators`,
`fakelib_default_mode`, `auto_update_ampr`, `auto_remove_missing_games`,
`auto_remove_missing_delay_seconds`,
`api_bind_address`, `fan_target_temperature`, and repeated `scanpath` keys. The
`allow_lan_access` field maps to `0.0.0.0` when enabled and `127.0.0.1` when
disabled. It atomically rewrites those keys while preserving every unrelated
`config.ini` line and comment, then wakes the scanner so its existing
runtime-reload path applies the values and rebuilds scan watches. `api_enabled`
is deliberately config-only, so the web interface cannot disable the service it
needs to restore access. A fan target of `0` means system control; explicit
values must be 50 through 91 degrees Celsius. Unlike `manual.lst`, `scan_paths`
are recursive library roots. The settings response contains only explicit
custom roots, never compile-time defaults or internal image-mount roots. Saving
an empty array removes the custom override and restores compile-time defaults.

`GET /api/v1/settings` returns `fakelib_default_mode` (`full`, `emulators`, or
`disabled`; initial default `full`). The settings update accepts the same
optional field; omission preserves its current value for older clients.
Per-title overrides take priority, and mode changes apply on the next launch.

The debug-log route returns at most 256 KiB and never modifies or rotates the log.
The kernel-log route reads a `sceKernelDebugGetSdkLogText` snapshot.

Uninstall returns success after `sceAppInstUtilAppUnInstall` accepts the
request; the on-disk removal itself is asynchronous. It returns `EBUSY` while a
game is active, another title owns a prepared runtime mount, or batch install
work is pending. An unchanged discoverable source may be installed again by a
later scan, so remove its manual entry or source first when that is not wanted.

## Test client

Run the host-side Python client while ShadowMount is active:

```sh
python3 tools/api_test.py 192.168.1.50 10101
```

It validates CORS preflight, the version response and complete image/game
snapshots. It also runs 16 parallel version requests while one client holds an
incomplete request open, then selects the first safe managed game for a
mount/unmount cycle. The mutation test is skipped if there is no candidate or
the runtime is busy.


### PKG installation and browser streaming

`pkg_install` and `pkg_browser_stream` capabilities advertise the package tab.
`POST /api/v1/packages` accepts `{"path":"/data/pkgs","offset":0}` and returns
`path`, `parent`, available `roots`, `entries`, `offset` and `truncated`.
An empty path lists available roots. Each page contains up to 256 folders/PKGs;
advance `offset` by 256 when `truncated` is true. Browsing is non-recursive and
restricted to internal `/data`, mounted `/mnt/usb0..7` and `/mnt/ext0..1`.
`/data/pkgs` remains a shortcut. Symlinks and installed content trees are excluded. Valid package entries expose
`title_id`, `title_name`, `content_id`, `version`, `kind` (base/patch/dlc) and
`size_bytes`. Invalid packages remain visible with `installable=false`.
Valid entries also include `installed` (base packages installed as native PKGs), `base_installed`,
`base_queued`, `has_icon` and `icon_revision`. A folder/image registration counts
as `base_installed` but not `installed`: its base PKG can be installed or queued,
and the UI labels that action Replace. Native PKG bases are labelled Installed,
patches with an installed base as Update, and missing dependencies
as No base game. Direct patch/DLC installation requires the base to be installed.
`GET /api/v1/packages/icon?path=...&v=...` extracts the embedded icon0.png from a
permitted PKG. The image is limited to 4 MiB, with file bounds and PNG signature
checks; packages without artwork use a placeholder.

`POST /api/v1/files` supplies the path picker for scan roots, individual game
sources and storage destinations. It accepts `path` (empty for drives), `offset`
and `include_files` (boolean, default false). The response has the same listing
envelope as `/packages`: `roots`, `path`, `parent`, `offset`, `truncated` and
`entries` containing `name`, `path`, `directory`, `size_bytes` and
`installable=false`. It returns directories only unless files are requested,
never reads package metadata, and uses the same drive restrictions and 256-entry
pagination. Selecting a path only fills the input; it does not save settings,
add a source or start a storage operation.

The Installation view also recognises backport directories named
`^(PPSA|CUSA)[0-9]{5}.*`, containing a real `fakelib` subdirectory and no
`sce_sys/keystone` entry. Such directory rows expose `backport=true`,
`backport_installed` and their nine-character `title_id`. Installed-game metadata
is matched by Title ID from one app.db snapshot per listing: `title_name`,
`version` and the versioned `icon_url`. Missing games return empty metadata;
the UI shows a controller placeholder and a localised Unknown game label.
`sdk_version` comes from the backport's own `eboot.bin` process parameter
(plain ELF or readable, uncompressed SELF), independently of the installed
game. The probe reads bounded headers and 24 parameter bytes, not the full
executable. Missing, malformed, encrypted or compressed data returns zero and
the UI omits the SDK label. PKG SDK parsing still uses param.json/param.sfo.
`POST /api/v1/backports/install` accepts `{"path":"/mnt/usb0/PPSA12345 Game"}`
and copies the whole directory to `/data/homebrew/backports/<original name>`.
It returns HTTP 202 with the shared storage-job response (`operation=copy`,
`source_type=backport`). It uses the existing cancellation, progress and mutation
gates. The list labels these folders as backports and offers Replace backport
when the same destination already exists. A complete copy is staged beside the
destination before replacement; failed/cancelled copies leave the installed
version intact. The short final rename step closes cancellation and rolls back
the previous directory if publishing the new one fails. The source is preserved.
Installing a destination over itself or an ancestor is rejected.
No game registration is required.

`POST /api/v1/backports/delete` accepts the same `path` and removes exactly the
selected backport directory, including its contents. It does not substitute the
corresponding internal installation path. The directory must still satisfy the
backport rules at request time and when the worker starts. The shared job uses
`operation=delete`, `source_type=backport` and an empty destination; progress is
shown in the common background-operation popup and the list refreshes when done.
The existing mutation gates and active-game protections apply. Cancellation is
available during preparation/measurement; recursive deletion is irreversible
once it starts. The UI confirms the full selected path before submitting.

`POST /api/v1/packages/queue` accepts `action` (`status`, `add`, `remove`, `clear`
or `start`), and `path` for add/remove. The payload owns up to 32 pending local
packages. A queued or currently installing base permits enqueueing its patches
and DLC, which still require an installed base when their turn begins. Duplicate
packages and bases already installed as PKGs cannot be queued. Removing a queued base
also removes its dependents if the base is not installed. Clear removes pending
items; cancellation stops the current batch and clears its remaining items.
The same happens on a batch failure. No files are deleted or retried.
Starting returns HTTP 202 with StorageJobResponse; other actions return HTTP 200
with `queue_running`, `queue_completed`, `queue_total` and pending `entries`.
The queue runs sequentially in the existing storage worker, keeps its mutation
gates through the batch, survives browser closure and scans once after any
successful installs. It is kept in memory until payload shutdown.
Storage status adds `queue_count`, `queue_completed`, `queue_total`,
`queue_running` and the current `package_kind`; its job ID covers the batch.

`POST /api/v1/packages/install` accepts `{"path":"/mnt/usb0/packages/game.pkg"}`.
It submits the package to the existing AppInstUtil session over an on-demand
loopback range server. There is one shared job slot for installation and file
operations, with the same scanner/runtime mutation gates. Patches and DLC
require an installed base game. Sources are not deleted or automatically retried.
Local file identity, size and modification times are checked before and during
installation. System errors preserve the raw `native_status` code.

For a file on a computer, `POST /api/v1/packages/stream` accepts
`{"name":"game.pkg","size":123456}` and returns the job plus a private `token`.
The response advertises `chunk_bytes` (512 KiB) and `stream_port`. Connect to
`ws://<console>:<stream_port>/<token>/ws` on the existing on-demand package
server. One source connection is accepted per job. Server text messages are
`{"op":"ready","window":2}`, `{"op":"range","id":1,"offset":0,"size":524288}`
and `{"op":"ack","id":1}`. Reply to each range with a binary message containing
a little-endian 64-bit range ID followed by exactly the requested bytes.
Two reads/transfers overlap; an acknowledgement releases its window slot.
Demand notifications wake the connection immediately, without polling.
The server closes the connection when the operation ends; disconnecting the
source fails pending reads immediately with `pkg_stream_lost`. Explicit cancellation
remains a separate cancelled job. Keep the browser page open until completion.

The stream retains up to 64 aligned 512 KiB segments (32 MiB) in RAM and pins
the header. Bulk reads look ahead at most eight segments, with at most sixteen
pending requests before speculation stops. Actual reader demand takes priority.
Speculation can evict only already-consumed segments; demanded reads can reclaim
ready segments without active readers. This prevents competing read-ahead windows
from repeatedly discarding each other's unread data. No staging PKG is saved to
storage. The installer only counts each delivered byte once.

Compatibility clients can use two concurrent
`POST /api/v1/packages/stream/request` clients, with
`{"token":"...","client":1}` or client 2. Each returns an `id`, `offset` and
`size` immediately; id=0 means no pending range. Idle clients wait 16?100 ms
before polling again. Reply with
`POST /api/v1/packages/stream/chunk?token=...&id=...`, Content-Type
`application/octet-stream`, and exactly that range's bytes. Successful range
and chunk responses keep the HTTP connection alive. Stable client IDs reclaim
an unacknowledged range after a lost response. Repeated buffered replies are
accepted only if their bytes match; an evicted range returns HTTP 409.
Only transient HTTP transport failures are recovered, at most three consecutive
failures per client, with 150/300/600 ms delays. The install is never restarted.
Expired sessions return 404/410.

Both modes publish `operation=install` and `state=preparing|installing` through
`/api/v1/games/storage/status` and use its existing cancellation endpoint.
The popup shows the package name, bytes, speed, elapsed time and system state.
Progress uses AppInstUtil downloaded/total counters when available, with HTTP
coverage as a fallback. Patches use `sceAppInstUtilGetPatchInstallStatus`: the
general install status can report the already playable base game with the same
content ID. DLC completion requires state `4` (installed) from
`sceAppInstUtilGetAddcontInstalledStatus`; general status still supplies its
download counters and errors. DLC without a general download record still uses
its own installed state. State `1` is not installed and must not complete the job.
When a general download record exists, it must also be complete, so an installed
DLC cannot prematurely finish an in-progress replacement. `sceAppInstUtilGetInstallProgress` and
`sceAppInstUtilGetInstallProgressInfo` are not used for patch completion: on
the tested 9.40 console both kept reporting 100% for the installed base while
its patch was still transferring. Speed is sampled from byte deltas once per
second and becomes zero when progress stops. System completion does not require fetching
PKG padding or unused ranges. An early playable state only completes the job
after all system bytes and any local copy are finished. Cancellation calls
AppInstUtil CancelInstall and retires the stream and its connections. Sleep
latches cancellation for both disk and PC installations, including coalesced
sleep/resume events. The install worker retires its per-attempt HTTP/WebSocket
listener and any running queue; the interrupted installation must be restarted.
The main API listener is recreated automatically after resume using its existing
bounded bind-recovery backoff. A new installation creates fresh sockets and a
new stream token. Shutdown uses the same cleanup path.
A 15-minute inactivity deadline follows progress,
not the total installation duration.

The PKG parser/range-server foundation was adapted from
[PR #157](https://github.com/drakmor/ShadowMountPlus/pull/157), with additional
range, lifetime and source-validation fixes. The browser streaming approach
was informed by the local ps5-pkg-manager implementation. AppInstUtil uses the
payload's existing initialization and authid; no helper process or elfldr
connection is introduced.

Package list and queue entries also include `sdk_version`, an unsigned packed
SDK value (for example `0x07508000` displays as `SDK 7.50`), or zero if unavailable.
PS5 values come from `param.json.sdkVersion` (the high 32 bits of the standard
64-bit hex string). PS4 values come from `param.sfo`'s `PUBTOOLINFO.sdk_ver`.
Only these metadata fields are used; a zero/missing value remains unknown and
never prevents installation. Minimum firmware requirements are separate fields.
