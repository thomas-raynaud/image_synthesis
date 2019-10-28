# TPs de synthèse d'images

Thomas RAYNAUD 11402517

# Shadowmap



# Lancer de rayons

```
make tp2 config=release64 -j4 && ./bin/tp2 && ./bin/image_viewer tps/tp2/render.hdr
```

Le programme lance par défaut 64 rayons pour chaque pixel de l'image. Si le rayon touche un triangle du BVH, nous allons calculer la lumière directe et indirecte pour ce rayon. Le rayon peut effectuer jusqu'à 4 rebonds dans la scène.

Pour la lumière directe, on calcule la couleur d'un point p en fonction des rayons qui peuvent atteindre un sample de chaque source de lumière à partir de ce point.

Pour la lumière indirecte, on génère une direction aléatoire v à partir de p. Un rayon est envoyé dans cette direction, et atteint un point p s'il intersecte avec un objet de la scène. On évalue la lumière directe provenant du point q, ainsi que sa lumière indirecte s'il reste encore des rebonds à effectuer.