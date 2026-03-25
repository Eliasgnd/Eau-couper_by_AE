# Vérification MVC — Inventory

Date: 2026-03-25

## Verdict
`Inventory` est **partiellement** conforme MVC, mais **pas encore complètement**.

## Ce qui est déjà en place (OK)
- Le contrôleur sait construire un `InventoryViewState` pour la racine et pour un dossier.
- La vue `Inventory` délègue la construction de l'état (root/folder) au contrôleur.
- La vue dispose d'un renderer dédié `renderState(...)`.
- Les DTO de présentation (`InventoryViewState`, `InventoryViewItem`) existent.
- La persistance est encapsulée derrière `InventoryRepository` via `InventoryModel`.

## Ce qui n'est PAS encore MVC (reste à faire)
1. **Mutations métier encore dans la vue**
   - `Inventory.cpp` modifie encore directement `m_customShapes`, `m_folders`, `m_baseShapeFolders`.
   - Exemples: renommage/suppression forme, assignation dossier, création dossier depuis menus contextuels.

2. **Persistance déclenchée par la vue**
   - La vue appelle encore `saveCustomShapes()` dans de nombreuses actions UI.
   - Objectif MVC: toutes les commandes de mutation + save doivent passer par `InventoryController`.

3. **Règles métier dans le code de rendu des cards**
   - `createBaseShapeCard(...)`, `createFolderCard(...)`, `addCustomShapeToGrid(...)` contiennent encore des décisions métier.
   - À déplacer dans des commandes contrôleur/services.

4. **Navigation encore pilotée depuis la vue**
   - `goToMainWindow()` et certains flux retour restent dans `Inventory`.
   - Préférable: signal d'intention + orchestration contrôleur/coordinator.

5. **Dépendance domaine vers la vue**
   - Le contrôleur utilise encore `Inventory::baseShapeName(...)` (méthode statique de classe UI).
   - À extraire dans un service domaine dédié (ex: `BaseShapeNamingService`).

## Plan court recommandé
### Étape 1 — Commandes contrôleur
Créer des méthodes contrôleur pour:
- rename/delete custom shape,
- move shape to folder,
- create/rename/delete folder,
- remove from folder,
- save layout commands.

Puis remplacer dans la vue les mutations directes par des appels contrôleur.

### Étape 2 — Vue strictement passive
- Garder seulement: émission de signaux d'intention + `renderState(state)`.
- Supprimer de `Inventory.cpp` toute écriture sur le modèle et tout `saveCustomShapes()`.

### Étape 3 — Domaine pur
- Extraire `baseShapeName(...)` vers un service domaine.
- Garder `InventoryController` sans dépendance aux classes UI.

### Étape 4 — Tests
- Tests unitaires de commandes contrôleur.
- Tests de projection `buildRootState` / `buildFolderState`.
- Tests de non-régression UI minimaux (wiring signaux -> contrôleur).

## Critère final de conformité MVC
- `Inventory` ne fait plus que du rendu + émission d'intentions.
- Aucune mutation métier/persistance dans la vue.
- `InventoryController` orchestre toutes les commandes et fournit le `ViewState`.
- Les services domaine et repository sont testés unitairement.
