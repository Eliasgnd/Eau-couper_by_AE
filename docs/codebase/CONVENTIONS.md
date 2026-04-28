# Conventions

Derniere mise a jour : 2026-04-28

## C++ Et Qt

- Standard : C++17.
- Framework : Qt6 Widgets.
- Headers : `.h`, implementations : `.cpp`.
- Indentation : 4 espaces.
- Classes : `PascalCase`.
- Methodes et variables locales : `camelCase`.
- Membres prives : prefixe `m_`.
- Signaux : noms descriptifs comme `shapeChanged()`.
- Slots : verbes d'action comme `setLanguage()` ou `startCutting()`.

## Includes

Ordre recommande :

1. header local correspondant ;
2. headers projet ;
3. headers Qt ;
4. headers standard.

Eviter les chemins relatifs profonds quand `INCLUDEPATH` permet un include clair.

## Style De Code

- Preferer les guard clauses aux blocs imbriques.
- Garder les classes petites et orientees responsabilite unique.
- Preferer la composition a l'heritage quand le code n'a pas besoin du polymorphisme Qt.
- Utiliser les signaux/slots pour les interactions UI et les transitions asynchrones.
- Documenter le "pourquoi" des algorithmes complexes, pas les operations evidentes.

## Erreurs Et Securite

- Preferer les retours explicites (`bool`, resultats structures, signaux d'erreur) aux exceptions.
- Valider les entrees aux frontieres : fichiers, reseau, materiel, donnees utilisateur.
- Ne pas ignorer les erreurs hardware ou reseau.
- Eviter les logs verbeux dans les chemins de decoupe actifs.

## MVVM

- Les widgets n'appliquent pas les regles metier.
- Les ViewModels exposent l'etat, les commandes et les signaux utiles.
- Les services applicatifs portent les cas d'usage.
- Le domaine reste testable sans UI.

## Documentation De Travail

Apres une modification significative, ajouter une entree dans `docs/stage/WORK_LOG.md` avec l'objectif, le probleme, la solution, les fichiers touches et les verifications.
