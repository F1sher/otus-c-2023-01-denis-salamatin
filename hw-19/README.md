Otus homework #19 (daemon)

**To compile**
```
meson builddir
ninja -C builddir
```

**To run CLI program as daemon**
```
./builddir/daemon-filesize -c PATH-TO-CONFIG-FILE -d
```

**Get help**
```
./builddir/daemon-filesize -h
```

The config file in glib .ini format: https://developer-old.gnome.org/glib/unstable/glib-Key-value-file-parser.html