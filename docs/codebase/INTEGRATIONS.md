# Integrations Externes

Derniere mise a jour : 2026-04-28

## OpenAI

`infrastructure/network/OpenAIService.*` appelle l'API OpenAI pour generer des images utilisables dans les workflows de decoupe.

- Cle : `OPENAI_API_KEY`.
- Usage : generation d'image depuis un prompt utilisateur.
- Cette integration fait partie du produit et ne doit pas etre supprimee lors du nettoyage des anciens outils de developpement IA.

## Hardware

- Raspberry Pi GPIO via libgpiod.
- STM32 via UART et protocole dedie.
- Controle moteur et jet dans `infrastructure/hardware/`.

Toute modification de ce perimetre doit etre consideree critique.

## Reseau Local

- WiFi via wrappers `nmcli`.
- Serveur HTTP local pour transfert de fichiers.
- Bluetooth via Qt Bluetooth selon les workflows disponibles.

## Imagerie

- OpenCV pour import, detection de contours et skeletonisation.
- Les fichiers image importes doivent etre valides aux frontieres : taille, format, dimensions, echec de lecture.

## Persistance

- Stockage local fichier pour inventaire, formes personnalisees et layouts.
- Pas de base de donnees externe detectee.
