#ifndef CLAVIER_H
#define CLAVIER_H

#include <QDialog>
#include <QPushButton>
#include <QGridLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QSettings>


//cration brnache global
class Clavier : public QDialog
{
    Q_OBJECT

public:
    explicit Clavier(QWidget *parent = nullptr);  // Constructeur du clavier
    QString getText() const;  // Récupère le texte saisi

private slots:
    void handleButton();  // Ajoute un caractère au champ de texte
    void deleteChar();    // Supprime le dernier caractère saisi
    void validateText();  // Valide l'entrée et ferme la fenêtre du clavier
    void toggleShift();   // Active/Désactive les majuscules
    void addSpace();  // Ajoute un espace dans le champ de texte
    void addUnderscore(); // Ajoute un "_" dans le champ de texte
    void handleLongPress(); // Gestion d'un appui long sur une touche
    void insertAccent(); // Insère un caractère accentué

private:
    QLineEdit *lineEdit;  // Champ de texte pour l'entrée utilisateur
    QPushButton *shiftButton; // Bouton pour activer Shift (majuscules)
    QPushButton *switchButton;  // Bouton pour basculer entre lettres et symboles
    QPushButton* currentLongPressButton; // Bouton actuellement maintenu enfoncé
    QPushButton* apostropheButton;  // Bouton d'apostrophe (varie selon mode AZERTY/QWERTY)
    QPushButton *langueButton;  // Bouton pour changer la langue du clavier
    QList<QPushButton*> allButtons;  // Liste de toutes les touches principales
    QList<QPushButton*> symbolButtons;  // Liste des touches symboles
    QList<QPushButton*> row1Buttons;  // Rangée supérieure du clavier (lettres)
    QList<QPushButton*> row2Buttons;  // Rangée du milieu
    QList<QPushButton*> row3Buttons;  // Rangée inférieure
    QMap<QString, QStringList> accentMap; // Associe une lettre aux accents disponibles
    QTimer *deleteTimer;  // Timer pour suppression continue après appui long
    QTimer *deleteDelayTimer;  // Timer pour retarder le début de la suppression rapide
    QTimer* longPressTimer; // Timer pour détecter un appui long
    QWidget* accentPopup; // Popup pour afficher les accents disponibles
    QWidget* overlay = nullptr;  // Overlay pour bloquer l'écran lors de la sélection d'accents
    QGridLayout* accentLayout; // Layout pour les boutons d'accents
    QGridLayout *gridLayout; // Layout général du  clavier

    enum KeyboardLayout { AZERTY, QWERTY };  // Types de dispositions clavier supportées
    KeyboardLayout currentLayout;  // Stocke la disposition actuelle

    bool majusculeActive;  // Indique si les majuscules sont activées
    bool isSymbolMode;  // Indique si le clavier est en mode symboles
    bool shiftLock = false;  // False = Maj Auto, True = Maj Verrouillée

    // Fonctions internes pour gérer l'affichage du clavier
    void handleAccent();
    void hideAccentPopup();
    void showAccentPopup(QPushButton* button);
    void startDelete();
    void startDeleteDelay();
    void startLongPress();
    void stopLongPress();
    void stopTimer();
    void stopDelete();
    void switchKeyboard();  // Alterne entre lettres et symboles
    void switchLayout();  // Change la disposition AZERTY/QWERTY
    void updateKeys();  // Met à jour l'affichage des touches selon Majuscule/Minuscule
    void updateKeyboard();
    void updateKeyboardLayout();  // Met à jour la disposition AZERTY/QWERTY
    QListWidget *suggestionList = nullptr; // Suggestion de noms

    void updateSuggestions();
    QStringList usageHistory;
    void loadUsageHistory();
    void saveUsageHistory() const;

signals:
    void textChangedExternally(const QString &text);

};

#endif // CLAVIER_H
