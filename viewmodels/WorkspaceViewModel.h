#pragma once

#include <QObject>
#include "Language.h"

class WorkspaceViewModel : public QObject
{
    Q_OBJECT

public:
    explicit WorkspaceViewModel(QObject *parent = nullptr)
        : QObject(parent) {}

    int largeur() const { return m_largeur; }
    int longueur() const { return m_longueur; }
    int shapeCount() const { return m_shapeCount; }
    int spacing() const { return m_spacing; }
    Language language() const { return m_language; }

    void setLargeur(int v)   { if (m_largeur != v)   { m_largeur = v;   emit largeurChanged(v); emit dimensionsChanged(m_largeur, m_longueur); } }
    void setLongueur(int v)  { if (m_longueur != v)   { m_longueur = v;  emit longueurChanged(v); emit dimensionsChanged(m_largeur, m_longueur); } }
    void setShapeCount(int v){ if (m_shapeCount != v) { m_shapeCount = v; emit shapeCountChanged(v); } }
    void setSpacing(int v)   { if (m_spacing != v)    { m_spacing = v;   emit spacingChanged(v); } }
    void setLanguage(Language l) { if (m_language != l) { m_language = l; emit languageChanged(l); } }

    void setDimensions(int largeur, int longueur) {
        bool changed = false;
        if (m_largeur != largeur)  { m_largeur = largeur;  emit largeurChanged(largeur);  changed = true; }
        if (m_longueur != longueur){ m_longueur = longueur; emit longueurChanged(longueur); changed = true; }
        if (changed) emit dimensionsChanged(m_largeur, m_longueur);
    }

signals:
    void largeurChanged(int value);
    void longueurChanged(int value);
    void dimensionsChanged(int largeur, int longueur);
    void shapeCountChanged(int count);
    void spacingChanged(int spacing);
    void languageChanged(Language lang);

private:
    int m_largeur   = 100;
    int m_longueur  = 100;
    int m_shapeCount = 1;
    int m_spacing    = 0;
    Language m_language = Language::French;
};
