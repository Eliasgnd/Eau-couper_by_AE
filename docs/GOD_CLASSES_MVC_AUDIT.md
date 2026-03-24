# Audit rapide – God classes et écarts MVC

Date : 2026-03-24.

## Méthode utilisée

- Tri des fichiers C++ par taille (nombre de lignes) pour repérer les hotspots.
- Lecture des headers/sources des plus gros widgets/controllers.
- Signalement des classes qui cumulent **au moins 3 responsabilités** (UI + logique métier + accès système/persistance/navigation), ce qui viole le découpage MVC.

## Classes candidates (priorité haute)

### 1) `Inventory` (`models/Inventory.h/.cpp`)

**Pourquoi c’est une god class**
- La classe est un `QWidget` (vue) **et** un singleton global (gestion d’état applicatif).
- Elle porte à la fois la logique de données (`CustomShapeData`, `LayoutData`, stats d’usage), la persistance JSON (`loadCustomShapes`, `saveCustomShapes`) et la construction complète de l’UI (`displayShapes`, `addCustomShapeToGrid`, `createFolderCard`, `createBaseShapeCard`).
- Elle gère aussi navigation/comportement applicatif (`resolveMainWindow`, `goToMainWindow`) et tri/filtrage/recherche.

**Symptômes MVC**
- Le modèle (`Inventory`) connaît des détails de rendu Qt (`QGraphicsScene`, `QFrame`, `QListWidget`, styles CSS) au lieu d’exposer seulement des données.
- Mélange de responsabilités Model + View + Controller dans une seule classe.

**Refactor recommandé**
- Extraire `InventoryRepository` (I/O JSON).
- Extraire `InventoryService` (règles métier: tri, filtre, stats, layouts).
- Garder `InventoryWidget` pour la vue Qt, pilotée par un controller/presenter.

### 2) `MainWindow` (`MainWindow.h/.cpp`)

**Pourquoi c’est une god class**
- Point d’orchestration de presque toute l’application : setup UI, menus, styles, navigation, AI, import image, découpe machine, traduction, et binding massif des signaux/slots.
- Contient plusieurs “mini-contrôleurs” internes (`setupShapeConnections`, `setupNavigationConnections`, `setupSystemConnections`) + logique applicative (chargement layout, ouverture dialogs, génération IA).

**Symptômes MVC**
- La vue principale contient de la logique de coordination métier et navigation.
- Couplage fort à `Inventory`, `NavigationController`, `AIServiceManager`, `ShapeController`, `ImageImportService`.

**Refactor recommandé**
- Introduire un `MainWindowPresenter`/`AppCoordinator` qui prend les décisions métier/navigation.
- Limiter `MainWindow` à l’affichage + forwarding d’événements utilisateur.

### 3) `WifiConfigDialog` (`ui/dialogs/WifiConfigDialog.h/.cpp`)

**Pourquoi c’est une god class**
- UI complète + interaction système directe (`QProcess`/`nmcli`) + parsing texte + persistance credentials + timers d’orchestration + diagnostics.
- Le même objet gère affichage, règles de connexion Wi‑Fi et accès OS.

**Symptômes MVC**
- La vue connaît des commandes système et format de parsing bas niveau.
- Impossible de tester la logique réseau sans lancer le widget.

**Refactor recommandé**
- Extraire `WifiService` (exécution nmcli), `WifiParser`, `WifiCredentialsStore`.
- Dialog = pure vue + binding sur un contrôleur/presenter.

### 4) `ShapeVisualization` (`ui/widgets/ShapeVisualization.h/.cpp`)

**Pourquoi c’est une god class**
- Widget Qt qui gère aussi placement algorithmique, validation, transformations géométriques, état d’optimisation, capture/application de layout.
- Beaucoup de logique non-UI intégrée (calcul de placement, adaptation dimensions, interaction état métier).

**Symptômes MVC**
- La vue encapsule une part importante de la logique métier de placement.
- Couplage direct entre rendu graphique et décisions d’algorithme.

**Refactor recommandé**
- Déporter calcul/validation dans services (`PlacementService`, `LayoutService`, `ValidationService`) injectés.
- Garder `ShapeVisualization` comme adaptateur de rendu.

### 5) `CustomEditor` (`ui/widgets/CustomEditor.h/.cpp`)

**Pourquoi c’est une god class**
- Gère composition UI complexe (splitters/panneaux), import logo/image, interactions de dessin, navigation, sauvegarde vers inventory, gestion police/clavier, etc.
- De nombreux rôles d’orchestration et état de workflow dans la même classe.

**Symptômes MVC**
- Mélange View + Controller + une partie de la logique métier de dessin/import.

**Refactor recommandé**
- Extraire un `CustomEditorController` (workflow), `ImageImportFacade`, et services de commande pour les actions de dessin.

## Ordre de refactor conseillé (impact/risque)

1. `WifiConfigDialog` (gain testabilité immédiat, frontières claires I/O système).
2. `Inventory` (réduction forte du couplage global).
3. `MainWindow` (stabiliser architecture générale et navigation).
4. `CustomEditor`.
5. `ShapeVisualization` (à faire après extraction des services de placement).

## Hotspots volumétriques observés

Top fichiers en lignes (indicatif) :
- `models/Inventory.cpp` ~1406
- `ui/dialogs/WifiConfigDialog.cpp` ~869
- `ui/widgets/CustomEditor.cpp` ~851
- `ui/widgets/ShapeVisualization.cpp` ~788
- `MainWindow.cpp` ~552

Ces volumes confirment les hotspots où se concentre la dette de séparation des responsabilités.
