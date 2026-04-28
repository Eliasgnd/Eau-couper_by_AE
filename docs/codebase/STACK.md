# Stack Technique

Derniere mise a jour : 2026-04-28

## Langages Et Build

- C++17 pour l'application.
- Qt6 avec qmake comme systeme de build principal.
- Bash pour les scripts Linux.

## Frameworks Et Modules

- Qt Widgets pour l'interface.
- Qt SVG, Network, Bluetooth, HttpServer, OpenGL Widgets selon `machineDecoupeIHM.pro`.
- Qt Test pour les tests.

## Dependances

- OpenCV 4 pour import et traitement d'image.
- libgpiod pour GPIO Raspberry Pi.
- Bibliotheques embarquees dans `external/` :
  - Clipper2 pour la geometrie ;
  - LEMON pour l'optimisation graphe/TSP ;
  - qrcodegen pour les QR codes.

## Plateformes

- Cible principale : Raspberry Pi Linux avec ecran tactile.
- Developpement possible sous Linux/Debian/Ubuntu.
- Developpement Windows via Qt Creator et configuration OpenCV locale.

## Configuration Importante

- `OPENAI_API_KEY` : utilisee par l'integration produit OpenAI pour la generation d'images.
- `machineDecoupeIHM.pro` : source de verite pour les fichiers compiles.
- `resources.qrc` : source de verite des ressources embarquees.
