# Tests Et Validation

Derniere mise a jour : 2026-04-28

## Framework

Les tests utilisent Qt Test via `tests/tests.pro`.

Commandes Linux/Raspberry Pi :

```bash
cd tests
qmake6 tests.pro
make -j$(nproc)
./tests
```

Build principal :

```bash
qmake6 machineDecoupeIHM.pro
make -j$(nproc)
```

Sous Windows, utiliser le kit Qt Creator configure ou les equivalents du terminal Qt.

## Quand Ajouter Des Tests

Ajouter ou adapter des tests si le changement touche :

- geometrie, placement, chemins de coupe ;
- inventaire ou persistance ;
- controle moteur, trajectoires, conversion mm/steps ;
- refactor MVVM avec deplacement de logique ;
- correction de bug reproductible.

## Types De Tests Actuels

- Tests unitaires Qt Test dans `tests/`.
- Donnees de test creees en code, sans fixtures externes lourdes.
- Tests surtout orientes geometrie, inventaire et ShapeManager.

## Verification Manuelle

Pour les zones UI ou hardware difficiles a automatiser, documenter dans `docs/stage/WORK_LOG.md` :

- l'ecran ou le workflow teste ;
- les actions realisees ;
- le resultat observe ;
- les limites de verification.

## Priorites De Couverture

1. Controle moteur et trajectoires.
2. Persistance inventaire et recuperation apres erreur.
3. Transformations geometriques de bout en bout.
4. Workflows UI critiques sur ecran tactile.
