# Audit rapide — God classes et écarts MVC

Date: 2026-03-25

## Méthode
- Repérage des classes volumineuses (taille fichier + nombre d'API publiques/slots).
- Vérification de mélange de responsabilités (UI + orchestration + logique métier + accès données) contraire à une séparation MVC stricte.

## Classes candidates prioritaires

### 1) `Inventory` (très prioritaire)
**Pourquoi c'est une god class :**
- Classe énorme (`models/Inventory.cpp` > 1000 lignes) et API large (`models/Inventory.h` expose beaucoup de responsabilités).
- Mélange dans une seule classe de :
  - vue Qt (widgets, cartes, menus, interactions utilisateur),
  - logique métier (tri/filtre, incrément d'usage, gestion dossiers/layouts),
  - persistance (chargement/sauvegarde),
  - navigation applicative (retour vers `MainWindow`),
  - gestion singleton globale.

**Indices MVC violés :**
- Le widget de vue gère directement mutation/persistance des données.
- Le contrôleur existe (`InventoryController`) mais la vue garde encore une grande partie de l'orchestration.

### 2) `ShapeVisualization` (prioritaire)
**Pourquoi c'est une god class :**
- Classe volumineuse (`ui/widgets/ShapeVisualization.cpp` ~ 788 lignes, header avec beaucoup d'API/signaux/slots).
- Mélange de :
  - rendu (QGraphicsScene/View),
  - logique de placement/calcul géométrique,
  - édition interactive (sélection, déplacement, rotation, suppression),
  - validation métier (collisions / hors-bord),
  - état d'optimisation et layout capture/apply.

**Indices MVC violés :**
- La vue n'est pas passive : elle contient aussi des décisions métiers et des règles de validation.

### 3) `MainWindow` (prioritaire)
**Pourquoi c'est une god class :**
- Classe centrale assez large (`MainWindow.cpp` ~ 500 lignes, beaucoup de slots/connexions).
- Mélange de :
  - composition UI,
  - wiring de navigation,
  - orchestration AI/import image,
  - gestion langue/traductions,
  - contrôle de flux métier (découpe, layouts).

**Indices MVC violés :**
- Le rôle de contrôleur d'application est en partie déjà externalisé (`MainWindowCoordinator`), mais `MainWindow` garde trop de logique de coordination.

### 4) `CustomEditor` (candidat secondaire)
**Pourquoi c'est une god class :**
- Fichier très volumineux (`ui/widgets/CustomEditor.cpp` ~ 850 lignes).
- Agrège : gestion complète UI + import d'images/logos + logique d'édition + navigation + sauvegarde dans l'inventaire.

**Indices MVC violés :**
- Beaucoup de logique de workflow et de traitement est directement dans la classe de vue.

## Recommandation d'ordre de refactor
1. `Inventory`
2. `ShapeVisualization`
3. `MainWindow`
4. `CustomEditor`

---

## Plan détaillé — rendre `Inventory` conforme MVC

### 1) Cible architecturale

**Vue (`InventoryView`)**
- Rôle: afficher l'état + remonter des intentions utilisateur.
- Conserve uniquement:
  - rendu des cards/items,
  - signaux UI (click shape, click folder, rename, delete, search, sort, filter),
  - application d'un `InventoryViewState` prêt à afficher.
- Interdit en vue:
  - accès disque,
  - mutation directe du modèle,
  - règles métier (tri, droits, validation, fallback de noms, etc.).

**Contrôleur (`InventoryController`)**
- Rôle: orchestrer les intentions utilisateur.
- Traduit les actions UI en commandes vers services.
- Récupère l'état métier et pousse un `InventoryViewState` vers la vue.

**Modèle / Domaine (`InventoryModel` + services métier)**
- `InventoryModel`: état en mémoire strict (shapes, folders, layouts, stats, language).
- Services domaine (sans widgets Qt):
  - `InventoryQueryService`: lectures et projections,
  - `InventoryMutationService`: mutations validées,
  - `InventorySortFilterService` (à créer): tri/filtre unifiés,
  - `InventoryUsageService` (optionnel): logique usage/récence.

**Infrastructure (`InventoryRepository`)**
- Encapsule `InventoryStorage`.
- API explicite: `load()`, `save(const InventorySnapshot&)`.
- Aucune logique d'affichage.

### 2) Découpage concret du fichier `Inventory.cpp`

**À extraire immédiatement depuis la vue:**
1. Persistance:
   - `loadCustomShapes()`, `saveCustomShapes()`, `customShapesFilePath()` vers repository/service I/O.
2. Tri/filtre:
   - logique de `displayShapes()` et `displayShapesInFolder()` (construction + tri des items) vers un service de projection.
3. Navigation applicative:
   - `goToMainWindow()` via callback injecté ou signal `requestBackToMain`.
4. Règles métier de dossiers/layouts:
   - validations de nom, renommage/suppression dossiers, affectation shape<->folder vers services contrôleur.
5. Statistiques usage/récence:
   - centraliser timestamp + incréments hors vue.

**À laisser temporairement dans la vue (phase 1):**
- `createBaseShapeCard`, `createFolderCard`, `addCustomShapeToGrid` mais **sans** décision métier.

### 3) Contrat de données recommandé

Créer un DTO de présentation:

- `InventoryViewItem`
  - `enum class Kind { Folder, BaseShape, CustomShape }`
  - `id`, `displayName`, `iconKey`, `usageCount`, `lastUsed`, `isInFolder`, `folderName`.
- `InventoryViewState`
  - `QList<InventoryViewItem> items`
  - `bool inFolderView`
  - `QString currentFolder`
  - `QString searchText`
  - `SortMode sortMode`
  - `FilterMode filterMode`

La vue devient un renderer de `InventoryViewState`.

### 4) Flux MVC cible (exemple)

1. L'utilisateur clique « supprimer dossier ».
2. La vue émet: `deleteFolderRequested(folderId)`.
3. Le contrôleur appelle `InventoryMutationService::deleteFolder(...)`.
4. Si succès, le contrôleur persiste via `InventoryRepository::save(...)`.
5. Le contrôleur recalcule `InventoryViewState` via Query + SortFilter service.
6. La vue reçoit l'état et rerender.

### 5) Plan de migration par petites PRs

**PR 1 — Séparer l'I/O**
- Introduire `InventoryRepository`.
- Remplacer dans `Inventory` tout accès direct stockage par repository.

**PR 2 — Sort/Filter hors vue**
- Extraire la logique de tri/filtre en `InventorySortFilterService`.
- La vue ne manipule plus d'algorithmes de tri.

**PR 3 — Presenter/ViewState**
- Introduire `InventoryViewState`.
- `InventoryController` construit l'état, la vue l'affiche.

**PR 4 — Commandes contrôleur**
- Migrer rename/delete/create folder, assignation aux dossiers, usage counters vers contrôleur/services.

**PR 5 — Nettoyage singleton + navigation**
- Réduire l'usage global de `Inventory::getInstance()`.
- Passer par injection (`MainWindowCoordinator`/factory) + signaux navigation.

### 6) Règles de qualité (Definition of Done)

- La vue `Inventory` n'appelle plus `save/load`.
- La vue ne modifie plus directement `m_customShapes`, `m_folders`, `m_baseShapeLayouts`.
- Toutes les mutations passent par contrôleur/services.
- Tri/filtre testés unitairement hors UI.
- Le nombre de méthodes privées de la vue diminue significativement.
- Les tests d'intégration UI vérifient uniquement le wiring (pas les règles métier).

### 7) Risques et mitigation

- **Risque:** régression UX pendant extraction.
  - **Mitigation:** PRs courtes + snapshots visuels + tests de non-régression sur workflows clés.
- **Risque:** duplication temporaire (ancienne/nouvelle logique).
  - **Mitigation:** feature flag local ou suppression immédiate de l'ancien chemin dès validation.
- **Risque:** dépendances circulaires entre contrôleur et vue.
  - **Mitigation:** contrat strict `signals/slots` + DTO immuables de présentation.

### 8) Priorités des tests à ajouter

- Unit tests:
  - tri par nom/utilisation/date,
  - filtres (tout / dossiers / formes),
  - mutations dossier (rename/delete/create),
  - robustesse sur collisions de noms.
- Integration tests (sans rendu complexe):
  - événements vue -> contrôleur,
  - contrôleur -> état vue.

## Stratégie concrète (itérative)
- Étape A: extraire des services purs (sans Qt Widgets) pour tri/filtre/règles/layouts.
- Étape B: déplacer les commandes utilisateur dans des contrôleurs dédiés (actions nommées, testables).
- Étape C: rendre les vues passives (elles émettent des intentions, affichent un ViewModel).
- Étape D: garder persistance et I/O dans des repositories/services dédiés.
