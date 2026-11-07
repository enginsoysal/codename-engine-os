#ifndef ENGINE_CATALOG74_H
#define ENGINE_CATALOG74_H

/* Stable package identities. Files, Settings and the service broker are base. */
#define APP74_CATALOG(X) \
    X(editor,2,TextEditor,"Text Editor",1,1) \
    X(calculator,4,Calculator,"Calculator",1,1) \
    X(downloads,5,Downloads,"Downloads",0,1) \
    X(monitor,6,SystemMonitor,"System Monitor",0,1) \
    X(terminal,7,Terminal,"Terminal",0,1) \
    X(clock,8,Clock,"Clock",0,1) \
    X(calendar,9,Calendar,"Calendar",1,1) \
    X(disks,10,DiskUtility,"Disk Utility",0,1) \
    X(software,11,SoftwareManager,"Software Manager",0,1) \
    X(recovery,12,UpdateRecovery,"Update & Recovery",0,1) \
    X(archives,13,ArchiveManager,"Archive Manager",1,1) \
    X(images,14,ImageViewer,"Image Viewer",0,1) \
    X(screenshot,15,Screenshot,"Screenshot",0,1) \
    X(media,16,MediaPlayer,"Media Player",0,1) \
    X(browser,17,Browser,"Browser",1,1) \
    X(help,18,Help,"Help",1,1)

#define APP74_KINDS 19u
#ifndef APP7_USER
const char *app74_title(unsigned);
unsigned app74_required(unsigned);
unsigned app74_ready(unsigned);
unsigned app74_worker(unsigned);
#endif
#endif
