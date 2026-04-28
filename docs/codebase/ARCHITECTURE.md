# Architecture

Derniere mise a jour : 2026-04-28

## Vue D'ensemble

Le projet suit une architecture MVVM avec des couches propres et un pattern Coordinator pour orchestrer les interactions complexes.

- `domain/` : logique metier, geometrie, modeles, interfaces abstraites.
- `application/` : services applicatifs, coordinators, factories et workflows.
- `infrastructure/` : materiel, persistance, reseau, imagerie et integrations externes.
- `viewmodels/` : etat exposable a l'UI, commandes et signaux.
- `ui/` : widgets Qt, dialogs, canvas et presentation uniquement.
- `shared/` : types transverses.

## Regles De Dependances

- `ui/` communique avec les ViewModels et Coordinators via signaux/slots.
- `viewmodels/` depend de `application/` et `domain/`, pas de widgets.
- `application/` orchestre `domain/`, `infrastructure/` et `viewmodels/`.
- `domain/` reste aussi pur que possible et ne depend pas de l'UI.
- `infrastructure/` implemente les details externes et ne doit pas connaitre les widgets.
- Les concerns transverses passent par interfaces explicites quand ils franchissent les couches.

## Flux Principaux

- Creation de formes : UI -> Coordinator -> ViewModel -> Domain -> rendu UI.
- Inventaire : InventoryViewModel -> InventoryController -> services query/mutation/storage -> repository.
- Decoupe : MainWindowCoordinator -> CuttingService -> PathPlanner/PathOptimizer -> MachineViewModel/StmUartService.
- Generation image IA : AIDialogCoordinator -> OpenAIService -> ImageImportService -> canvas.

## Zones Fragiles Connues

- `CustomDrawArea` reste une zone volumineuse et sensible avec de la logique a extraire progressivement.
- `TrajetMotor` et le controle machine demandent une verification stricte avant toute modification.
- Certains widgets ont encore des couplages historiques avec services ou autres widgets.
- Les transformations geometriques peuvent produire des erreurs physiques si les conversions mm/pixels/steps divergent.

## Direction D'evolution

Priorite au refactor MVVM progressif :

1. Extraire les dependances UI hors des couches materiel/infrastructure.
2. Centraliser l'orchestration dans `application/`.
3. Rendre les ViewModels testables sans UI.
4. Ajouter des tests avant ou pendant les changements sur les chemins critiques.
