// ClavierNumerique.h
#ifndef CLAVIERNUMERIQUE_H
#define CLAVIERNUMERIQUE_H

#include <QDialog>
#include <QPushButton>
#include <QGridLayout>
#include <QLineEdit>
#include <QList>
//test brache rudy
class ClavierNumerique : public QDialog
{
    Q_OBJECT

public:
    explicit ClavierNumerique(QWidget *parent = nullptr);  // Constructeur du pavé numérique
    QString getText() const;  // Récupère le texte saisi par l'utilisateur

    /// Ouvre modally le pavé numérique, renvoie true si validé et place la valeur dans 'out'
    static bool ouvrirClavierNum(QWidget *parent, int &out);

private slots:
    void handleButton();      // Insère un chiffre ou un point décimal
    void deleteChar();        // Supprime le dernier caractère
    void validateText();      // Valide l'entrée et ferme le pavé numérique

private:
    QLineEdit *lineEdit;                   // Champ d'affichage de la saisie
    QList<QPushButton*> digitButtons;      // Boutons pour les chiffres 0-9
    QPushButton *decimalButton;            // Bouton pour le séparateur décimal
    QPushButton *deleteButton;             // Bouton pour effacer
    QPushButton *validateButton;           // Bouton pour valider la saisie
    QGridLayout *gridLayout;               // Layout pour disposer les boutons
    bool hasDecimal;                       // Indique si le séparateur décimal a déjà été ajouté
};

#endif // CLAVIERNUMERIQUE_H
