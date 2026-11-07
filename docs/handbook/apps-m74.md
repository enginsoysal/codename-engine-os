# M7.4 application guide

The first M7.4 app delivery is qualified within M74-VM-v1. This guide describes
its implemented functions and limits. [Downloads](../releases/m74-v0.7.4.md) ·
[Exact acceptance](../../evidence/M7.4/acceptance-results.json).

## Open and manage applications

Choose **Start**, then an installed application. Use its title bar to move it,
the lower-right corner to resize it, and the upper-right cross to close it.
Applications with unsaved text ask before closing. Essential controls have
minimum window dimensions rather than disappearing under other controls.

**Software Manager** lists installed and available offline applications. Choose
**Open**, **Remove** or **Install**. Removal shows the exact application and
keeps personal documents and private app data. Close its windows before removing
it. Files, Settings and shared system services remain protected. If Software
Manager itself is removed, open **Settings → Manage applications** to reinstall
it. Package changes replace/remove the app-owned package file through the native
copy-on-write volume; they do not run arbitrary shell hooks.

Editor, Calculator, Calendar, Help, Browser and the ZIP decoder have separate CPL3 event
programs. Their executable pages load on demand and are shared between same-app
windows; last close releases their code and instance pages. Lighter fixed
system frontends use a validated declarative manifest and the protected broker,
without starting an otherwise unnecessary process. Trusted
compositor/file/device broker code remains part of the static kernel; it is
not counted as removable app RSS. Removing a frontend's manifest disables its
launcher/integration, without deleting the shared service. These are fixed
built-in providers, not an arbitrary extension language. Framebuffer memory is
separate as well. [Implementation choices](../architecture/applications-m74.md).

## Standard application reference

| Application | Ordinary task | Current boundary |
|---|---|---|
| Files | Browse Home, create folders, open documents/media | Native installed volume; extra partitions are not auto-mounted |
| Settings | Appearance, scaling, saved preferences, app management | Protected fallback provider |
| Text Editor | Open, edit, Save/Save as, clipboard, cursor movement | 8,191-byte documents; richer editing/encodings remain future |
| Calculator | Decimal, scientific functions, conversions | Six decimal places; approximate scientific results |
| Terminal | Optional advanced native commands | Engine commands; no DOS/Windows shell compatibility |
| Clock | UTC offsets, timer, stopwatch, daily alarm | Session state; fixed offsets; visual alarm |
| Calendar | Browse Gregorian months and save day notes | Local notes; reminders/accounts remain future |
| System Monitor | Inspect managed memory and native system details | Allocator figures exclude static kernel/framebuffer/firmware |
| Disk Utility | Inspect disks; manage another supported SATA disk | Current system disk remains read-only here |
| Software Manager | Remove/reinstall fixed offline packages | No arbitrary third-party package repositories |
| Update & Recovery | Verified boot repair and previous-slot rollback | No automatic online system updates |
| Archive Manager | List and extract ZIP into a new folder | Stored/Deflate, 32 members, 64 MiB expanded; no archive creation |
| Image Viewer | Open a BMP and fit it to the window | Uncompressed 24/32-bit BMP |
| Screenshot | Save native-resolution framebuffer | BMP; streaming capture |
| Media Player | Open WAV, Play/Pause/Resume/Stop, volume | PCM 16-bit WAV and supported analog HDA/MSI |
| Browser | Local HTML/text and checked HTTPS, Back/Home/Links | Basic static text rendering |
| Help | Read the built-in guide and turn pages | Offline; independent of Text Editor |
| Downloads | Save a file from a typed HTTPS URL | One shared checked TLS connection; 64 MiB maximum |

The first two providers are required. The other sixteen are optional. The
existing [desktop controls](desktop-m73.md) explain Files, Editor, clipboard,
Terminal, Downloads and appearance. New ordinary tasks are described below.

## Calculator, Clock and Calendar

Calculator offers decimal arithmetic, a Scientific tab and common length,
mass and temperature conversions. Precision is six decimal places, rounded
towards zero. Trigonometry uses degrees and scientific results are approximate.
Invalid operations show a recoverable error; Clear resets the calculation.

Clock offers fixed UTC offsets, a minute timer, a stopwatch and a daily UTC
alarm. Timer/stopwatch durations use the monotonic HPET clock. The once-per-second
RTC interrupt wakes the desktop without a continuous high-rate polling loop.
Timer and alarm messages continue with the window closed. Restarting cancels
this session state; alarms do not currently play sound or persist after restart.
Fixed offsets do not automatically follow daylight-saving rules.

In Calendar, choose a day, type a private note and choose **Save note**, or
press Ctrl+S. Each day's note is stored locally under the owner's AppData.
Save before changing dates. Calendar reminders and synchronized accounts are
not implemented. Removing Calendar keeps its notes.

## Pictures and screenshots

Screenshot writes the real framebuffer at its actual resolution into
**Pictures/Screenshots**, using a new BMP filename. Choose **Open picture** to
view it. Capture streams through the volume buffer; it does not allocate a
second full-screen image.

Image Viewer reads uncompressed 24/32-bit BMP files using bounded tiles and fits
them to the window. Choose an image through its folder picker or open a BMP in
Files. The sample **Pictures/ColorStudy.bmp** is an original generated image.
PNG, JPEG, animated formats, rotation and arbitrary zoom are not implemented.

## ZIP archives

Open a ZIP in Files, or choose it from Archive Manager. **Extract all** creates
a fresh **Downloads/ExtractedN** folder and never overwrites an existing file.
**Open output folder** opens Files at the result. The original archive remains
unchanged. The small original **Downloads/ArchiveSample.zip** demonstrates
Deflate extraction with a nested folder and an original two-second PCM chime.
Open the extracted ChimeLong.wav in Files to play it.

Supported formats are non-encrypted ZIP Stored and Deflate, at most 32 members
and 64 MiB total expanded data. ZIP64, links, non-ASCII names, unsupported
compression methods and unsafe paths are rejected. Each regular file's expanded
size and CRC are checked before it becomes visible. If a later member fails,
earlier completed members can remain in the new output folder; check that folder
before retrying. The current slice does not create ZIP files or support TAR/7z.

Deflate executes in a watchdog-bounded CPL3 worker. Its dictionary/decoder pages
and the broker's small scratch pages exist only during extraction and are
released afterwards. The original pinned miniz sources retain their MIT notice
in **System/Licenses/miniz-MIT.txt**. [Upstream](https://github.com/richgel999/miniz),
[pin and file hashes](../../third_party/miniz/provenance.json).

## Disks and recovery

Disk Utility inspects disks and manages another supported SATA disk: initialize
GPT, select unallocated space, create a named partition, choose FAT32 or Engine
quick format, or delete/erase after explicit review and consent. The running
system disk is inspection-only. Use installation media to change that disk's
partitions. Quick format is not a secure erase. Resize, relocation, conversion,
arbitrary hardware drivers and automatic extra-volume mounting remain outside
this implementation.

Update & Recovery repairs verified boot files or selects a complete previous
recovery slot. Review and confirm the operation, keep the disk connected and
restart afterwards. Personal data and the existing GPT are preserved. A fresh
installation usually has no previous slot to roll back to. System updates
still require the appropriate installation ISO; automatic online updates and
signed update repositories are not implemented.

## Browser and audio

Browser opens local HTML/text and certificate-checked HTTPS pages. Use the
address field, Back, Home, Stop and Links. Parsing runs in its bounded CPL3
worker. Source pages are limited to 128 KiB, displayed text to 7,679 bytes and
links to 16. This is a basic text browser: JavaScript, CSS layout, forms,
cookies and image rendering are not supported. It cannot replace a modern
web browser for general websites. Network cancellation preserves the previous
page, and the temporary download is deleted after parsing.

Media Player has native PCM capture qualification. Its decoder
accepts 16-bit PCM WAV, mono/stereo at 44.1/48 kHz, up to 64 MiB. The new
analog HDA/MSI path allocates DMA buffers only when playback starts and must
release them after verified hardware quiescence. Unsupported output hardware
must leave the desktop usable. MP3, AAC, video and broad hardware support are
not implemented. Captured playback qualification uses QEMU HDA; VMware audio
output has not been independently recorded.

## Remaining work

Richer codecs/web rendering, Calendar reminders, advanced editing and additional
desktop integrations remain follow-ups. The first app delivery passes its
removal/omission, actual 4K, resource and VMware lifecycle gates. Existing Windows/MS-DOS applications remain M8
work. [Full contract](../milestones/m7.4.md).

## Build a distribution with a different application set

Schema 3 adds `desktop.applications`, an explicit list of optional package IDs.
The [standard profile](../../distros/engineos-apps.json) selects all implemented
apps. The [minimal example](../../distros/engineos-minimal-apps.json) selects none;
Files, Settings and shared services remain part of the base. Keep `calculator`
consistent with whether `calculator` appears in that list. Invalid or deferred
IDs fail profile validation before compilation. Data-only packages can seed
Documents, Downloads, Pictures and Music; executable installation hooks are
not allowed.

For an isolated example artifact, run:

```powershell
python tools/build/m74.py --profile distros/engineos-minimal-apps.json --revision minimal
```

The result is `EngineOS-M74MINIMAL.iso`, not an overwrite of a historical
release. Omitted packages retain their verified offline reinstall sources.
Open Settings → Manage applications to add them later without needing Software
Manager. Distribution naming, colors and the app set are independent of disk
safety and destructive-operation confirmations.
