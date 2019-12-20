# Fork of a fork of fingerterm

# How to build
```
source /usr/local/oecore-x86_64/environment-setup-cortexa9hf-neon-oe-linux-gnueabi
qmake
make
arm-oe-linux-gnueabi-strip fingerterm
scp fingerterm root@remarkable:
```

# Mods
white background

Ctrl-K hides/shows keyboard

Ctrl-Left/Right rotates the screen (bug with the framebuffer)
