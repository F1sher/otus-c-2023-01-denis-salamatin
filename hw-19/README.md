Otus homework #19 (daemon)

**To compile**
```
meson builddir
ninja -C builddir
```

**To run CLI program as daemon**
```
./builddir/daemon-filesize -c CONFILE -d
```

The config file in glib .ini format