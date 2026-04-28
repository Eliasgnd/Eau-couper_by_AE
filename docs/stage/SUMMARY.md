# Resume De Travail

Ce fichier reste court et a jour. Il donne le contexte global sans obliger Codex a relire tous les journaux hebdomadaires.

## Etat Actuel

- Le depot utilise `AGENTS.md` comme carte d'entree pour Codex.
- La documentation technique stable est rangee dans `docs/codebase/`.
- Les traces detaillees de travail sont limitees a un fichier par semaine dans `docs/stage/`.
- Les changements mineurs ne sont pas journalises seuls ; ils sont regroupes avec une tache significative quand ils servent une decision ou une verification.

## Decisions Importantes

- Garder les instructions agent courtes pour ne pas saturer le contexte.
- Separer documentation technique stable, resume de stage et journaux hebdomadaires.
- Conserver l'integration OpenAI produit tout en supprimant l'ancien outillage IA de developpement.
- Prioriser le refactor MVVM sur la vitesse d'ajout de fonctionnalites.

## Travail Realise

- 2026-04-28 : optimisation documentaire du depot pour Codex, suppression de l'ancien contexte IA de developpement, creation du journal de travail.
- 2026-04-28 : passage a une strategie de journalisation hebdomadaire avec resume permanent pour limiter la consommation de contexte.

## Points A Suivre

- Maintenir `SUMMARY.md` concis, idealement sous 100 a 200 lignes.
- Creer un nouveau `WORK_LOG_YYYY-Www.md` au changement de semaine.
- Ajouter des tests ou scripts quand une regle devient recurrente ou critique.
