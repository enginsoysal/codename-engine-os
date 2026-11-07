# Your Engine OS desktop and folders

After installation choose **Open desktop**. On later disk boots the desktop opens automatically. Files opens Home, showing Desktop, Documents, Downloads, Pictures and AppData from your real installed user volume.

## Find your way

- Click a folder row to open it. The location above the list shows the real path.
- Use the sidebar to open Home, Desktop, Documents, Downloads or Pictures directly.
- The upper **< / >** buttons go Back and Forward through the last eight locations in this Files window. **Up** opens the parent folder.
- The lower **< / >** buttons page through long lists; the item count describes the current folder. Up/Down arrow keys also scroll the list.
- **System drive** browses the installed native volume's root: System, Applications, Users, Data, Cache, Logs, Temporary and Volumes. System items are protected from modification through Files. Additional disks/partitions are not automatically mounted here.

## Create or remove a folder

Open a personal folder, choose **New folder**, type a name and choose **Create folder** or press Enter. Use 1–63 letters, digits, spaces, dots, dashes or underscores; the final character cannot be a dot or space. Existing names are refused. Escape or Cancel keeps your files unchanged. New folders are saved on disk.

Choose **Delete** beside a personal item. **Keep item** is the safe default; deletion requires a separate Delete item choice. Nonempty folders cannot be deleted. Files does not offer system-item deletion or creation in system folders. This UI guard is not a multiuser permission/ACL system. Advanced trusted components have different access.

## Arrange your workspace

Drag a window by its title bar. Resize using the lower-right corner. Use **_** to minimize, and click its taskbar button to restore it. **x** closes it. My files and Documents on the desktop open Files windows; Appearance opens Settings. The Start menu contains Files, Settings and optional technical tools. Calculator, Text Editor and Downloads do not clutter the default launcher; existing native document opening remains available for personal files.

Appearance opens the actual saved accent and light/dark settings. Choose **Save preferences** to retain your changes. These fixed native choices are not yet a wallpaper-image editor, general theme store or replaceable desktop-shell manager. Distribution authors can change the validated profile's default accent, window appearance, wallpaper colour, tagline and content. [Distribution guide](distributions.md).

Tab and Shift+Tab move focus through visible controls. Enter activates the focused control; F10 opens Start; Alt+Tab changes windows. Up to eight windows can be open. The renderer repaints damaged regions and halts at input idle; folder navigation does not allocate one full framebuffer per window.

The desktop and Files are original native Engine OS code. They are not a browser preview, Windows compatibility layer or Linux desktop. DOS and Windows application support remains later work. [Installation](install-m72.md) · [Implementation choices](../architecture/desktop-m72.md).
