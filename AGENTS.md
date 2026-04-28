# AGENTS.md

Ce fichier est la carte d'entree pour Codex. Il doit rester court. Pour les details, lire les documents dans `docs/codebase/`.

## Projet

Application Qt6/C++17 pour piloter une machine de decoupe a eau sur Raspberry Pi avec ecran tactile.

Le projet est industriel et touche au controle machine. La stabilite, la securite et la lisibilite priment sur les refactors larges ou rapides.

## Sources De Verite

- Architecture et limites de couches : `docs/codebase/ARCHITECTURE.md`
- Structure du depot : `docs/codebase/STRUCTURE.md`
- Conventions C++/Qt : `docs/codebase/CONVENTIONS.md`
- Tests et validation : `docs/codebase/TESTING.md`
- Dette technique et zones fragiles : `docs/codebase/CONCERNS.md`
- Stack et integrations : `docs/codebase/STACK.md`, `docs/codebase/INTEGRATIONS.md`
- Resume de travail utile au rapport de stage : `docs/stage/SUMMARY.md`
- Journaux hebdomadaires : `docs/stage/WORK_LOG.md`

## Regles Critiques

- Respecter MVVM : `ui/` contient la presentation, `viewmodels/` porte l'etat, `application/` orchestre les cas d'usage, `domain/` porte la logique metier.
- Ne pas ajouter de logique metier dans les widgets Qt.
- Ne pas faire dependre `infrastructure/` de `ui/`.
- Ne pas modifier le controle moteur ou les trajectoires sans verification explicite.
- Faire des changements petits, incrementaux et faciles a relire.
- Mettre a jour `machineDecoupeIHM.pro` quand un fichier `.h`, `.cpp` ou `.ui` est ajoute ou retire.
- Ne pas toucher aux bibliotheques dans `external/` sauf demande explicite.

## Fichiers A Eviter

Ne pas lire ni modifier pour le travail courant, sauf besoin precis :

- `build/`
- `debug/`
- `release/`
- `Makefile*`
- `moc_*.cpp`, `qrc_*.cpp`, `ui_*.h`
- `.qtc_clangd/`
- `.git/`

## Workflow Attendu

1. Lire le contexte utile avant de modifier.
2. Identifier la couche concernee et respecter ses dependances.
3. Implementer le plus petit changement coherent.
4. Ajouter ou adapter les tests quand le changement touche le domaine, la geometrie, l'inventaire, le moteur ou une regression connue.
5. Mettre a jour la documentation de stage seulement pour les changements significatifs.
   - Toujours tenir `docs/stage/SUMMARY.md` a jour avec les decisions et evolutions importantes.
   - Ajouter le detail dans le journal hebdomadaire indique par `docs/stage/WORK_LOG.md`.
   - Ne pas journaliser les micro-ajustements isoles, sauf s'ils expliquent une decision utile. Exemple de changement mineur a ne pas detailler seul : rendre visuellement clair qu'un bouton reste active.
6. Quand l'environnement le permet, verifier avec :

```bash
qmake6 machineDecoupeIHM.pro
make -j$(nproc)
cd tests && qmake6 tests.pro && make -j$(nproc) && ./tests
```

Sous Windows/Qt Creator, utiliser les commandes equivalentes du kit Qt configure.

## Integration OpenAI Produit

L'integration `OpenAIService` fait partie du produit pour la generation d'images. Ne pas la supprimer lors du nettoyage des anciens outils de developpement IA.
