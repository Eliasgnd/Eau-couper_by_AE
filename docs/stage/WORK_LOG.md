# Index Des Journaux De Travail

Ce fichier garde seulement l'index et les regles des journaux de travail. Ne pas y ajouter directement les entrees longues.

Lire d'abord `docs/stage/SUMMARY.md` pour le contexte general, puis ouvrir uniquement le journal hebdomadaire utile.

## Journaux Hebdomadaires

- `docs/stage/WORK_LOG_2026-W18.md` : semaine du 2026-04-27 au 2026-05-03.

## Regles De Journalisation

- Maximum un fichier de journal par semaine : `WORK_LOG_YYYY-Www.md`.
- Ajouter une entree seulement pour un changement significatif : fonctionnalite, refactor, correction de bug, decision d'architecture, test important, dette technique traitee.
- Regrouper les petites modifications liees a une meme tache dans une seule entree.
- Ne pas detailler les micro-ajustements isoles qui n'apportent pas d'explication utile au rapport.
- Mettre a jour `SUMMARY.md` quand une entree change la comprehension globale du projet, l'architecture, les fonctionnalites ou les choix techniques.

## Modele D'entree

```markdown
## YYYY-MM-DD - Titre court

Objectif :
- Ce que la mise a jour devait permettre.

Probleme ou besoin :
- Pourquoi le changement etait necessaire.

Solution mise en place :
- Ce qui a ete change.

Fichiers ou zones concernes :
- `chemin/important`

Raisons du choix :
- Pourquoi cette solution a ete retenue.

Verifications :
- Commandes lancees, tests, observations ou limites.

Limites et prochaines etapes :
- Ce qui reste a verifier ou ameliorer.
```
