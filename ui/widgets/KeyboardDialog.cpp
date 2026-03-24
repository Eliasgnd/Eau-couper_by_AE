#include "KeyboardDialog.h"
#include "Inventory.h"
#include <QTimer>
#include <QDebug>
#include <QRegularExpression>

KeyboardDialog::KeyboardDialog(QWidget *parent) : QDialog(parent), majusculeActive(true)
{
    setWindowTitle("KeyboardDialog Virtuel AZERTY");
    setFixedSize(650, 540);  // Augmenter la taille pour un meilleur affichage
    isSymbolMode = false;  // Démarrer en mode clavier principal

    QVBoxLayout *layout = new QVBoxLayout(this);
    lineEdit = new QLineEdit(this);
    suggestionList = new QListWidget(this);
    suggestionList->setVisible(false);  // caché par défaut
    suggestionList->setMaximumHeight(120);
    suggestionList->setStyleSheet("font-size: 14px; background: white; border: 1px solid gray;");
    lineEdit->setFixedHeight(50);  // Agrandir la zone de texte
    layout->addWidget(lineEdit);
    layout->addWidget(suggestionList);

    // Lorsqu'on clique sur une suggestion
    connect(suggestionList, &QListWidget::itemClicked, this, [=](QListWidgetItem *item){
        QString text = item->text();
        lineEdit->setText(text);
        suggestionList->hide();
        emit textChangedExternally(text);

        // 🔁 Mise à jour de l’historique
        usageHistory.removeAll(text);  // Évite les doublons
        usageHistory.prepend(text);    // Ajoute en tête
    });


    gridLayout = new QGridLayout(this);  // ✅ Variable membre, accessible partout

    // Ligne des chiffres (placés en haut)
    QStringList chiffres = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
    int row = 0, col = 0;
    for (const QString &chiffre : chiffres) {
        QPushButton *button = new QPushButton(chiffre, this);
        button->setFixedSize(60, 60);
        connect(button, &QPushButton::clicked, this, &KeyboardDialog::handleButton);
        gridLayout->addWidget(button, row, col++);;
    }

    // Déclaration des listes pour organiser les touches
    QStringList row1 = {"A", "Z", "E", "R", "T", "Y", "U", "I", "O", "P"};
    QStringList row2 = {"Q", "S", "D", "F", "G", "H", "J", "K", "L", "M"};
    QStringList row3 = {"W", "X", "C", "V", "B", "N"};

    row = 1; col = 0;  // ✅ Correction de l'indexation des lignes

    // ✅ Organisation des touches de la première ligne
    for (const QString &key : row1) {
        QPushButton *button = new QPushButton(key, this);
        button->setFixedSize(60, 60);
        allButtons.append(button);
        row1Buttons.append(button);
        connect(button, &QPushButton::pressed, this, &KeyboardDialog::startLongPress);
        connect(button, &QPushButton::released, this, &KeyboardDialog::stopLongPress);
        connect(button, &QPushButton::clicked, this, &KeyboardDialog::handleButton);
        gridLayout->addWidget(button, row, col++);
    }

    row++; col = 0;  // ✅ Correction du passage à la ligne suivante

    // ✅ Organisation des touches de la deuxième ligne
    for (const QString &key : row2) {
        QPushButton *button = new QPushButton(key, this);
        button->setFixedSize(60, 60);
        allButtons.append(button);
        row2Buttons.append(button);
        connect(button, &QPushButton::pressed, this, &KeyboardDialog::startLongPress);
        connect(button, &QPushButton::released, this, &KeyboardDialog::stopLongPress);
        connect(button, &QPushButton::clicked, this, &KeyboardDialog::handleButton);
        gridLayout->addWidget(button, row, col++);
    }

    row++; col = 1;  // ✅ Correction du décalage pour ressembler à un vrai clavier

    for (const QString &key : row3) {
        QPushButton *button = new QPushButton(key, this);
        button->setFixedSize(60, 60);
        allButtons.append(button);
        row3Buttons.append(button);
        connect(button, &QPushButton::pressed, this, &KeyboardDialog::startLongPress);
        connect(button, &QPushButton::released, this, &KeyboardDialog::stopLongPress);
        connect(button, &QPushButton::clicked, this, &KeyboardDialog::handleButton);
        gridLayout->addWidget(button, row, col++);

        //qDebug() << "Création du bouton" << key << "en position" << row << "," << col-1;
    }


    // Ajout du bouton Apostrophe (')
    // Création du bouton apostrophe qui change selon le mode clavier
    apostropheButton = new QPushButton("'", this);
    apostropheButton->setFixedSize(80, 60);
    allButtons.append(apostropheButton);
    connect(apostropheButton, &QPushButton::clicked, this, &KeyboardDialog::handleButton);
    gridLayout->addWidget(apostropheButton, row, 8);  // Placé juste à droite de la barre espace


    // ✅ Déplacement correct du bouton de changement de langue
    row++; col = 0;  // ✅ Assurer qu’il est bien sur la dernière ligne
    langueButton = new QPushButton("🌍 AZERTY", this);
    langueButton->setFixedSize(100, 60);
    connect(langueButton, &QPushButton::clicked, this, &KeyboardDialog::switchLayout);
    gridLayout->addWidget(langueButton, row, col);

    // Bouton Shift (⇧) et Effacer (⌫) alignés à droite
    shiftButton = new QPushButton("⇧", this);
    shiftButton->setFixedSize(80, 60);
    allButtons.append(shiftButton);
    connect(shiftButton, &QPushButton::clicked, this, &KeyboardDialog::toggleShift);
    gridLayout->addWidget(shiftButton, row, 9);  // À droite

    QPushButton *deleteButton = new QPushButton("⌫", this);
    deleteButton->setFixedSize(80, 60);
    // Quand on APPUIE sur "⌫", on lance le délai avant l'effacement rapide
    deleteTimer = new QTimer(this);
    deleteDelayTimer = new QTimer(this);
    deleteDelayTimer->setSingleShot(true);  // Assurer que le délai n'est lancé qu'une seule fois

    // Connexions des timers
    connect(deleteDelayTimer, &QTimer::timeout, this, &KeyboardDialog::startDelete);
    connect(deleteTimer, &QTimer::timeout, this, &KeyboardDialog::deleteChar);
    connect(deleteButton, &QPushButton::pressed, this, &KeyboardDialog::startDeleteDelay);
    gridLayout->addWidget(deleteButton, 0, 10);  // Aligné sous Shift
    connect(deleteButton, &QPushButton::released, this, &KeyboardDialog::stopDelete);

    // Ajout du bouton Espace (␣) centré en bas
    row++;
    QPushButton *spaceButton = new QPushButton("␣", this);
    spaceButton->setFixedSize(250, 60);
    connect(spaceButton, &QPushButton::clicked, this, &KeyboardDialog::addSpace);
    gridLayout->addWidget(spaceButton, row, 4, 1, 3);  // Largeur sur 4 colonnes

    // Ajout du bouton Underscore (_)
    QPushButton *underscoreButton = new QPushButton("_", this);
    underscoreButton->setFixedSize(80, 60);
    connect(underscoreButton, &QPushButton::clicked, this, &KeyboardDialog::addUnderscore);
    gridLayout->addWidget(underscoreButton, row, 2);

    // Ajout du bouton Valider (✔) en bas à droite
    QPushButton *validateButton = new QPushButton("✔", this);
    validateButton->setFixedSize(80, 60);
    connect(validateButton, &QPushButton::clicked, this, &KeyboardDialog::validateText);
    gridLayout->addWidget(validateButton, row, 9);

    layout->addLayout(gridLayout);
    setLayout(layout);

    // Ajout du bouton "123?" pour passer aux symboles
    switchButton = new QPushButton("123?", this);
    switchButton->setFixedSize(80, 60);
    connect(switchButton, &QPushButton::clicked, this, &KeyboardDialog::switchKeyboard);
    gridLayout->addWidget(switchButton, row, 0);  // Mettre "123?" en bas à gauche

    // Ajouter les touches de symboles (Cachées au début)
    QStringList symbols1 = {"@", "#", "$", "%", "&", "*", "(", ")", "-", "+"};
    row = 1; col = 0;
    for (const QString &symbol : symbols1) {
        QPushButton *button = new QPushButton(symbol, this);
        button->setFixedSize(60, 60);
        connect(button, &QPushButton::clicked, this, &KeyboardDialog::handleButton);
        gridLayout->addWidget(button, row, col++);
        symbolButtons.append(button);
        button->setVisible(false);  // Cacher les symboles au début
    }

    row++;
    QStringList symbols2 = {"=", "/", "\\", "{", "}", "[", "]", ";", ":", "\""};
    col = 0;
    for (const QString &symbol : symbols2) {
        QPushButton *button = new QPushButton(symbol, this);
        button->setFixedSize(60, 60);
        connect(button, &QPushButton::clicked, this, &KeyboardDialog::handleButton);
        gridLayout->addWidget(button, row, col++);
        symbolButtons.append(button);
        button->setVisible(false);  // Cacher les symboles au début
    }

    QPushButton* apostropheSymbolButton = nullptr;
    row++;
    QStringList symbols3 = {"<", ">", ",", ".", "?", "!", "|", "'", "^", "~"}; // L'apostrophe est toujours là
    col = 0;
    for (const QString &symbol : symbols3) {
        QPushButton *button = new QPushButton(symbol, this); // ✅ Crée le bouton avant de l'utiliser
        button->setFixedSize(60, 60);
        connect(button, &QPushButton::clicked, this, &KeyboardDialog::handleButton);
        gridLayout->addWidget(button, row, col++);
        symbolButtons.append(button);
        button->setVisible(false);  // Cacher les symboles au début

        if (symbol == "'") { // ✅ Vérifie et stocke le bouton après sa création
            apostropheSymbolButton = button;
        }
    }

    longPressTimer = new QTimer(this);
    longPressTimer->setSingleShot(true);
    connect(longPressTimer, &QTimer::timeout, this, &KeyboardDialog::handleLongPress);
    accentPopup = nullptr;

    // Mapping des touches avec leurs variantes accentuées
    accentMap.insert("e", {"é", "è", "ê", "ë"});
    accentMap.insert("i", {"î", "ï"});
    accentMap.insert("o", {"ô", "ö", "œ"});
    accentMap.insert("u", {"û", "ü"});
    accentMap.insert("a", {"à", "â", "æ"});
    accentMap.insert("c", {"ç"});
    accentMap.insert("n", {"ñ"});
    accentMap.insert("E", {"É", "È", "Ê", "Ë"});
    accentMap.insert("I", {"Î", "Ï"});
    accentMap.insert("O", {"Ô", "Ö", "Œ"});
    accentMap.insert("U", {"Û", "Ü"});
    accentMap.insert("A", {"À", "Â", "Æ"});
    accentMap.insert("C", {"Ç"});
    accentMap.insert("N", {"Ñ"});
    currentLayout = QWERTY;  // ✅ QWERTY sélectionné dès le lancement
    updateKeyboardLayout();  // ✅ Applique immédiatement le layout QWERTY
    loadUsageHistory();

}

QString KeyboardDialog::getText() const
{
    return lineEdit->text();
}

// Fonction pour ajouter un texte dans le champ
void KeyboardDialog::handleButton()
{
    QPushButton *button = qobject_cast<QPushButton *>(sender());
    if (!button) return;

    QString text = button->text();
    lineEdit->insert(text);

    emit textChangedExternally(lineEdit->text());

    if (majusculeActive && !shiftLock) {
        majusculeActive = false;
        updateKeys();
    }

    updateSuggestions();
}


// Fonction pour supprimer un caractère
void KeyboardDialog::deleteChar()
{
    if (!lineEdit->text().isEmpty())
    {
        QString text = lineEdit->text();
        text.chop(1);
        lineEdit->setText(text);
        emit textChangedExternally(lineEdit->text());
        updateSuggestions();
    }
}

// Fonction pour valider l'entrée et fermer la fenêtre
void KeyboardDialog::validateText()
{
    accept();
    QString current = lineEdit->text();
    usageHistory.removeAll(current);
    usageHistory.prepend(current);
    saveUsageHistory();

}

void KeyboardDialog::toggleShift()
{
    if (!majusculeActive)
    {
        // 🔹 Première pression : Active la majuscule automatique (Majuscule sur une lettre)
        majusculeActive = true;
        shiftLock = false;
    }
    else if (!shiftLock)
    {
        // 🔹 Deuxième pression : Active le verrouillage des majuscules (Caps Lock)
        shiftLock = true;
    }
    else
    {
        // 🔹 Troisième pression : Désactive complètement Shift (Retour en minuscule)
        majusculeActive = false;
        shiftLock = false;
    }

    //qDebug() << "Shift changé -> Majuscule Active:" << majusculeActive << " | Verrouillage:" << shiftLock;
    updateKeys();  // ✅ Met à jour l'affichage des touches AZERTY & QWERTY
}


void KeyboardDialog::updateKeys()
{
    for (QPushButton *button : allButtons) {
        QString text = button->text();

        if (majusculeActive || shiftLock)
            button->setText(text.toUpper());  // 🔹 Majuscules si activé
        else
            button->setText(text.toLower());  // 🔹 Minuscules sinon
    }

    // ✅ Assurer que le bouton Shift change visuellement (optionnel)
    if (shiftLock)
        shiftButton->setStyleSheet("background-color: lightblue;");  // Caps Lock activé
    else if (majusculeActive)
        shiftButton->setStyleSheet("background-color: lightgray;");  // Shift activé
    else
        shiftButton->setStyleSheet("");  // Mode normal
}


// Ajout de l'espace
void KeyboardDialog::addSpace()
{
    lineEdit->insert(" ");
    emit textChangedExternally(lineEdit->text());
    updateSuggestions();
}

// Ajout de l'underscore
void KeyboardDialog::addUnderscore()
{
    lineEdit->insert("_");
    emit textChangedExternally(lineEdit->text());
    updateSuggestions();
}

void KeyboardDialog::updateKeyboard()
{
    if (isSymbolMode) {
        // Masquer les touches principales et afficher les symboles
        for (QPushButton *button : allButtons) {
            button->setVisible(false);
        }
        for (QPushButton *button : symbolButtons) {
            button->setVisible(true);
        }
        switchButton->setText("ABC");  // Changer le texte du bouton de bascule
        switchButton->update();  // Force Qt à rafraîchir le texte du bouton

        // ✅ Mode symboles : le bouton affiche " et devient plus petit
        apostropheButton->setText("'");
        apostropheButton->setFixedSize(60, 60);
    }
    else {
        // Afficher les touches principales et masquer les symboles
        for (QPushButton *button : allButtons) {
            button->setVisible(true);
        }
        for (QPushButton *button : symbolButtons) {
            button->setVisible(false);
        }
        switchButton->setText("123?");  // Changer le texte du bouton de bascule
        switchButton->update();  // Force Qt à rafraîchir le texte du bouton

        // ✅ Mode AZERTY : le bouton affiche ' et devient plus large
        apostropheButton->setText("'");
        apostropheButton->setFixedSize(80, 60);
    }
}


void KeyboardDialog::switchKeyboard()
{
    isSymbolMode = !isSymbolMode;  // Inverser l'état du clavier
    updateKeyboard();
}

void KeyboardDialog::startDeleteDelay()
{
    deleteChar();
    if (deleteDelayTimer->isActive())
    {
        deleteDelayTimer->stop();  // Stopper le timer s'il tourne déjà
    }
    deleteDelayTimer->start(500);  // Démarrer un délai de 500ms avant l’effacement rapide
}

void KeyboardDialog::startDelete()
{
    if (!deleteTimer->isActive()) {
        deleteTimer->start(75);  // Démarrer l'effacement toutes les 100ms
    }
}

void KeyboardDialog::stopDelete()
{
    deleteTimer->stop();  // Stopper l’effacement rapide
    deleteDelayTimer->stop();  // Stopper aussi le délai s'il n’est pas fini
}

void KeyboardDialog::handleAccent() {
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        QString text = button->text();

        // Si un popup d'accentuation est ouvert, on ne traite pas un nouveau bouton
        if (accentPopup)
        {
            return;
        }
        // Si la touche possède des accents, on démarre un timer pour détecter un appui long
        if (accentMap.contains(text))  // ✅ Vérifie via le texte du bouton
        {
            currentLongPressButton = button;
            longPressTimer->start(500); // 500ms pour déclencher l'affichage des accents
        }
        else
        {
            lineEdit->insert(text);
            if (majusculeActive && !shiftLock)
            {
                majusculeActive = false;
                updateKeys();
            }
        }
    }
}


void KeyboardDialog::handleLongPress()
{
    if (currentLongPressButton) {
        QString buttonText = currentLongPressButton->text();
        //qDebug() << "handleLongPress() appelé pour : " << buttonText;

        if (accentMap.contains(buttonText)) {
            //qDebug() << "Affichage des accents pour : " << buttonText;
            showAccentPopup(currentLongPressButton);
        } else {
            //qDebug() << "Erreur : Aucun accent trouvé pour " << buttonText;
        }
    } else {
        //qDebug() << "Erreur : handleLongPress() appelé mais aucun bouton valide.";
    }
}


void KeyboardDialog::showAccentPopup(QPushButton* button) {
    if (accentPopup) return;

    // ✅ Création de l'overlay semi-transparent
    overlay = new QWidget(this);
    overlay->setStyleSheet("background-color: rgba(0, 0, 0, 100);"); // Fond noir semi-transparent
    overlay->setGeometry(0, 0, this->width(), this->height());
    overlay->show();

    // ✅ Création du popup des accents
    accentPopup = new QWidget(this);
    accentPopup->setWindowFlags(Qt::Popup);
    accentPopup->setAttribute(Qt::WA_DeleteOnClose);
    connect(accentPopup, &QWidget::destroyed, this, &KeyboardDialog::hideAccentPopup); // Supprime overlay à la fermeture
    accentLayout = new QGridLayout(accentPopup);

    QStringList accents = accentMap.value(button->text());
    int col = 0;
    for (const QString &accent : accents) {
        QPushButton* accentButton = new QPushButton(accent, accentPopup);
        accentButton->setFixedSize(50, 50);
        connect(accentButton, &QPushButton::clicked, this, &KeyboardDialog::insertAccent);
        accentLayout->addWidget(accentButton, 0, col++);
    }

    // ✅ Positionnement du popup
    QPoint buttonPos = button->mapToGlobal(QPoint(0, -button->height()));
    accentPopup->setGeometry(buttonPos.x(), buttonPos.y(), col * 50, 50);
    accentPopup->show();
}



void KeyboardDialog::insertAccent() {
    QPushButton* accentButton = qobject_cast<QPushButton*>(sender());
    if (accentButton) {
        lineEdit->insert(accentButton->text());
        emit textChangedExternally(lineEdit->text());
        updateSuggestions();
    }
    hideAccentPopup();
}

void KeyboardDialog::hideAccentPopup() {
    if (accentPopup) {
        accentPopup->close();
        accentPopup = nullptr;
        currentLongPressButton = nullptr;
    }

    // ✅ Supprime l'overlay en même temps que le popup
    if (overlay) {
        overlay->deleteLater();
        overlay = nullptr;
    }
}

void KeyboardDialog::stopTimer() {
    deleteTimer->stop();
    deleteDelayTimer->stop();
    hideAccentPopup(); // Cache les accents si le doigt est levé
}

void KeyboardDialog::startLongPress()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        QString buttonText = button->text();  // Défini avant de l'utiliser

        if (accentMap.contains(buttonText)) {  // Vérifie si un accent est disponible
            //qDebug() << "Appui long détecté sur : " << buttonText;
            //qDebug() << "Appui long détecté sur : " << buttonText;

            if (accentMap.contains(buttonText)) {  // Vérifie avec le texte du bouton
                currentLongPressButton = button;
                longPressTimer->start(500);
                //qDebug() << "Timer démarré pour : " << buttonText;
            } else {
                //qDebug() << "Aucun accent pour ce bouton.";
            }
        } else {
            //qDebug() << "startLongPress() appelé mais aucun bouton détecté.";
        }
    }
}

void KeyboardDialog::stopLongPress()
{
    longPressTimer->stop();  // Annule l'appui long si la touche est relâchée
}

void KeyboardDialog::switchLayout()
{
    // ✅ Alterner immédiatement entre AZERTY et QWERTY
    if (currentLayout == AZERTY)
    {
        currentLayout = QWERTY;
        langueButton->setText("🌍 QWERTY");  // ✅ Mise à jour du bouton langue
    }
    else
    {
        currentLayout = AZERTY;
        langueButton->setText("🌍 AZERTY");
    }

    updateKeyboardLayout();  // ✅ Appliquer immédiatement le changement
}

void KeyboardDialog::updateKeyboardLayout()
{
    // 1) Une seule déclaration, tout en haut
    QStringList newRow1, newRow2, newRow3;

    // 2) Remplir selon le layout
    if (currentLayout == AZERTY) {
        newRow1 << "A" << "Z" << "E" << "R" << "T" << "Y" << "U" << "I" << "O" << "P";
        newRow2 << "Q" << "S" << "D" << "F" << "G" << "H" << "J" << "K" << "L" << "M";
        newRow3 << "W" << "X" << "C" << "V" << "B" << "N";
        langueButton->setText("🌍 AZERTY");
        apostropheButton->setText("'");
    } else { // QWERTY
        newRow1 << "Q" << "W" << "E" << "R" << "T" << "Y" << "U" << "I" << "O" << "P";
        newRow2 << "A" << "S" << "D" << "F" << "G" << "H" << "J" << "K" << "L" << ";";
        newRow3 << "Z" << "X" << "C" << "V" << "B" << "N" << "M";
        langueButton->setText("🌍 QWERTY");
        apostropheButton->setText("'");
    }

    // 3) Ajouter / supprimer le bouton 'M' selon la différence de taille
    int desiredCount = newRow3.size();
    int currentCount = row3Buttons.size();

    if (currentLayout == QWERTY && currentCount < desiredCount) {
        // on ajoute M
        QPushButton *buttonM = new QPushButton("M", this);
        buttonM->setFixedSize(60, 60);
        allButtons.append(buttonM);
        row3Buttons.append(buttonM);
        connect(buttonM, &QPushButton::clicked, this, &KeyboardDialog::handleButton);
        // colonne = index+1 (décalage)
        int column = row3Buttons.size() /*=7*/;
        gridLayout->addWidget(buttonM, 3, column);
    }
    else if (currentLayout == AZERTY && currentCount > desiredCount) {
        // on supprime M
        QPushButton *buttonM = row3Buttons.takeLast();
        gridLayout->removeWidget(buttonM);
        allButtons.removeOne(buttonM);
        buttonM->deleteLater();
    }

    // 4) Appliquer les nouveaux textes
    for (int i = 0; i < row1Buttons.size() && i < newRow1.size(); ++i)
        row1Buttons[i]->setText(newRow1[i]);
    for (int i = 0; i < row2Buttons.size() && i < newRow2.size(); ++i)
        row2Buttons[i]->setText(newRow2[i]);
    for (int i = 0; i < row3Buttons.size() && i < newRow3.size(); ++i)
        row3Buttons[i]->setText(newRow3[i]);

    // 5) Mettre à jour maj/min
    updateKeys();
}

void KeyboardDialog::updateSuggestions()
{
    QString currentText = lineEdit->text().trimmed();
    suggestionList->clear();

    if (currentText.isEmpty()) {
        suggestionList->setVisible(false);
        return;
    }

    QString pattern = "^" + QRegularExpression::escape(currentText);
    QRegularExpression rx(pattern, QRegularExpression::CaseInsensitiveOption);

    QStringList matches = Inventory::getInstance()
                              ->getAllShapeNames()
                              .filter(rx);

    if (!matches.isEmpty()) {
        // Tri par historique : les noms dans usageHistory en premier
        QStringList orderedMatches;
        for (const QString &name : usageHistory) {
            if (matches.contains(name)) {
                orderedMatches << name;
            }
        }
        for (const QString &name : matches) {
            if (!orderedMatches.contains(name))
                orderedMatches << name;
        }

        suggestionList->addItems(orderedMatches);
        suggestionList->setVisible(true);
    } else {
        suggestionList->setVisible(false);
    }
}

void KeyboardDialog::loadUsageHistory()
{
    QSettings settings("AubeElectronique", "EauCouper");
    usageHistory = settings.value("KeyboardDialog/UsageHistory").toStringList();
}

void KeyboardDialog::saveUsageHistory() const
{
    QSettings settings("AubeElectronique", "EauCouper");
    settings.setValue("KeyboardDialog/UsageHistory", usageHistory);
}
