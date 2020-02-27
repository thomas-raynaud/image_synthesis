# Image synthesis

*More info on this page: http://thomas-raynaud.space/index.html#/projects/3D-image-synthesis*

Generate the makefiles of the different image synthesis projects:

```
premake4 --file=premake4_tp_synthese.lua gmake
```

# TP1 - OpenGL pipeline

Compilation and execution:
```
make tp1 && bin/tp1
```

# TP2 - Raytracing

Compilation:
```
make tp2 config=release64 -j4 && make image_viewer
```

Execution: 
```
bin/tp2 && bin/image_viewer tps/tp2/render.hdr
```

# TP3 - Raytracing, compute shaders and BVH

Compilation and execution:
```
make tp3 && bin/tp3
```