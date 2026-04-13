#pragma once

class Inventory;

// Fabrique d'objets applicatifs.
// Construit et câble les dépendances sans que MainWindow ait besoin de les connaître.
class AppFactory
{
public:
    // Crée la pile Inventory complète (InventoryModel → InventoryController →
    // InventoryViewModel → Inventory) et retourne le widget racine.
    // InventoryModel et InventoryController sont créés avec durée de vie processus
    // (identique à l'ancien comportement — pas de parent Qt disponible pour eux).
    static Inventory *createInventory();
};
