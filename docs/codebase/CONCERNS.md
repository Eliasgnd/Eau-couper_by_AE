# Concerns Et Dette Technique

Derniere mise a jour : 2026-04-28

## Priorite Haute

- Couplage entre controle machine et UI : isoler progressivement `TrajetMotor` des widgets.
- Tests insuffisants sur les trajectoires et conversions physiques.
- `CustomDrawArea` reste volumineux et doit etre refactore par petites extractions.
- Persistance inventaire : renforcer les tests de sauvegarde, corruption et recuperation.

## Priorite Moyenne

- Supprimer ou encadrer les `qDebug()` dans les chemins de production.
- Remplacer les recherches globales de widgets par injection explicite.
- Reduire les `QMetaObject::invokeMethod` complexes quand un signal/slot type suffit.
- Ajouter des tests d'integration sur les transformations geometriques.

## Zones A Modifier Avec Prudence

- `infrastructure/hardware/`
- `domain/geometry/`
- `ui/canvas/`
- `application/services/CuttingService.*`
- persistance inventaire

## Principes De Remediation

- Refactorer en petites etapes.
- Ajouter un test ou une verification avant de changer un comportement critique.
- Capturer les decisions dans `docs/stage/WORK_LOG.md`.
- Promouvoir une regle recurrente en documentation ou test mecanique.
