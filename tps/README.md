# TPs de synthèse d'images

Thomas RAYNAUD 11402517

Le contenu de cette archive est à placer dans le répertoire gkit. Pour générer les makefiles des différents projets :

```
premake4 --file=premake4_tp_synthese.lua gmake
```

# Matières, animation et shadowmap

Exécution :
```
make tp1 && bin/tp1 
```

## Matières

3 objets sont affichés dans la scène : 2 quaternius, et une box en guise le sol. On utilise des uniform buffers pour stocker les attributs des matières du modèle quaternius (quaternius.cpp) : les couleurs (diffuse et émission) et exposants des matières correspondant à chaque indice de matière.

Le vertex array object contient les positions des sommets et normales de chaque keyframe (23 en tout), suivi de l'indice de matière de chaque triangle.

## Interpolation

Pour l'interpolation entre deux keyframes de l'animation, nous utilisons le fait que les positions des sommets et des normales des deux keyframes sont côte à côte dans le vertex array object. Pour récupérer les positions de deux keyframes dans le vertex shader, il suffit d'activer les positions et les normales des deux keyframes à l'endroit pointé (glVertexAttribPointer(...) suivi de glEnableVertexAttribArray(...)).

## Shadowmap

On effectue une première passe du rendu des objets avec un framebuffer attribué à la shadowmap. La deuxième passe se fait avec le framebuffer par défaut, en utilisant comme texture de profondeur celle générée par le premier framebuffer avec le programme shadowmap.glsl. On utilise cette texture dans les programmes quaternius.glsl (dédié au modèle quaternius) et object.glsl (autres objets non animés de la scène, comme le sol).

La projection de la shadowmap est orthographique. On utilise la position de la lumière et le point d'origine du monde pour fixer le repère de la caméra (Lookat(point_lumiere, origine)).

# Lancer de rayons

Exécution :
```
make tp2 config=release64 -j4 && bin/tp2 && bin/image_viewer tps/tp2/render.hdr
```

Le programme lance par défaut 64 rayons pour chaque pixel de l'image. Si le rayon touche un triangle du BVH, nous allons calculer la lumière directe et indirecte pour ce rayon. Le rayon peut effectuer jusqu'à 4 rebonds dans la scène.

Pour la lumière directe, on calcule la couleur d'un point p en fonction des rayons qui peuvent atteindre un sample de chaque source de lumière à partir de ce point.

Pour la lumière indirecte, on génère une direction aléatoire v à partir de p. Un rayon est envoyé dans cette direction, et atteint éventuellement un point q s'il y a intersection avec un objet de la scène. On évalue la lumière directe provenant du point q, ainsi que sa lumière indirecte s'il reste encore des rebonds à effectuer dans la scène.