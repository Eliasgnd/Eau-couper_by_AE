# Structure Du Depot

Derniere mise a jour : 2026-04-28

## Racine

- `main.cpp` : entree de l'application.
- `machineDecoupeIHM.pro` : configuration qmake principale.
- `resources.qrc` : ressources Qt.
- `style.qss`, `style_light.qss` : themes.
- `AGENTS.md` : carte d'entree pour Codex.
- `docs/` : documentation versionnee du projet.

## Dossiers Applicatifs

- `domain/` : logique metier, formes, geometrie, inventaire, interfaces.
- `application/` : coordinators, services, factory de dependances.
- `infrastructure/` : hardware Raspberry/STM, OpenCV, OpenAI, WiFi, persistance.
- `viewmodels/` : etats et commandes exposes a l'UI.
- `ui/` : MainWindow, widgets, canvas, dialogs, utilitaires UI.
- `shared/` : enums et types transverses.
- `tests/` : tests Qt Test.
- `translations/` : fichiers Qt Linguist.
- `external/` : bibliotheques embarquees, a eviter sauf demande explicite.
- `scripts/` : scripts de setup ou maintenance.

## Ou Ajouter Le Code

- Nouvelle logique metier : `domain/`, avec tests si possible.
- Nouveau workflow applicatif : `application/services/` ou `application/coordinators/`.
- Nouvel etat UI : `viewmodels/`.
- Nouveau widget ou dialog : `ui/widgets/` ou `ui/dialogs/`.
- Nouvelle integration externe : interface dans `domain/interfaces/`, implementation dans `infrastructure/`.
- Nouveau test : `tests/`, puis mise a jour de `tests/tests.pro`.

## Regles qmake

Chaque ajout ou retrait de fichier C++ doit etre reflete dans `machineDecoupeIHM.pro`.

- `.cpp` dans `SOURCES`
- `.h` dans `HEADERS`
- `.ui` dans `FORMS`
- ressources dans `resources.qrc` si elles sont embarquees

## Fichiers Generes

Ne pas modifier les fichiers generes par Qt ou qmake :

- `Makefile*`
- `ui_*.h`
- `moc_*.cpp`
- `qrc_*.cpp`
- `build/`, `debug/`, `release/`
