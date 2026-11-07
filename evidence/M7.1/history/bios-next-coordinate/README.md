# BIOS fixture coordinate correction

The BIOS fixture uses a 1024-pixel framebuffer. The first mouse adapter clicked Next at a coordinate for the 1280-pixel UEFI screen, so it never left Welcome. The saved screenshot demonstrates this harness error; no partition operation was reached. The Next coordinate now follows the actual firmware framebuffer profile.
