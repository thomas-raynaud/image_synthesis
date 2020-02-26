# Synthèse d'images

Pour générer les makefiles des différents projets :

```
premake4 --file=premake4_tp_synthese.lua gmake
```

# TP 1 :

Compilation :
```
make tp1
```

Exécution : 
```
bin/tp1
```

# TP 2 :

Compilation :
```
make tp2 config=release64 -j4 && make image_viewer
```

Exécution : 
```
bin/tp2 && bin/image_viewer tps/tp2/render.hdr
```

# TP 3 :

Compilation :
```
make tp3
```

Exécution : 
```
bin/tp3
```