#ifndef CUSTOMDRAWAREA_H
#define CUSTOMDRAWAREA_H

#include <QSvgRenderer>
#include "touchgesturereader.h"
#include <QWidget>
#include <QPointF>
#include <QList>
#include <QPolygonF>
#include <QMouseEvent>
#include <QPainterPath>
#include <QImage>
#include <QWheelEvent>
#include <QTimer>
#include <QFont>
#include <QRectF>
#include <QGestureEvent>
#include <QPinchGesture>
#include <QElapsedTimer>

class CustomDrawArea : public QWidget
{
    Q_OBJECT
public:
    // Enumération des différents modes de dessin disponibles
    enum class DrawMode {
        Freehand,       // Dessin à main levée
        PointParPoint,  // Dessin point par point
        Line,           // Dessin de lignes droites
        Circle,         // Dessin de cercles
        Rectangle,      // Dessin de rectangles
        Supprimer,      // Suppression d'une forme
        Gomme,          // Gomme avec découpe en petits segments
        Deplacer,       // Déplacement d'une forme sélectionnée
        Pan,            // Déplacement de la vue globale (zoom/pan)
        Text,           // Ajout de texte plein
        ThinText        // Ajout de texte fin
    };


    // Structure représentant une forme dessinée
    struct Shape {
        QPainterPath path;
        int originalId;
        qreal rotationAngle = 0.0;

        bool operator==(const Shape &other) const {
            return path == other.path && originalId == other.originalId;
        }
    };

    struct HandlePosition {
        qreal radius;
        qreal angleOffset;  // angle relatif initial au centre
    };
    HandlePosition m_rotationHandlePos;



    explicit CustomDrawArea(QWidget *parent = nullptr); // Constructeur
    ~CustomDrawArea(); // Destructeur

    // Gestion des modes de dessin
    void setDrawMode(DrawMode mode);
    DrawMode getDrawMode() const;
    DrawMode lastPrimaryMode() const { return m_lastPrimaryMode; }

    // Accès aux formes et gestion du dessin
    QList<QPolygonF> getCustomShapes() const;
    void clearDrawing(); // Efface toutes les formes
    void setEraserRadius(qreal radius); // Définit la taille de la gomme

    // Ajout de logos importés
    void addImportedLogo(const QPainterPath &logoPath);
    void addImportedLogoSubpath(const QPainterPath &subpath);

    // --- Nouvelles méthodes pour la gestion du texte ---
    // Setter et getter pour la police utilisée dans l'ajout de texte
    void setTextFont(const QFont &font);
    QFont getTextFont() const;

    // va relier les deux extrémités les plus proches
    void connectNearestEndpoints(int idx1, int idx2);
    // Démarre le mode sélection de deux formes
    void startShapeSelection();           // sélection pour relier deux formes
    void cancelSelection();               // annule la sélection en cours
    void toggleMultiSelectMode();         // active/désactive la sélection multiple
    bool hasSelection() const { return !m_selectedShapes.isEmpty(); }
    void deleteSelectedShapes();          // supprime toutes les formes sélectionnées
    void copySelectedShapes();            // copie les formes sélectionnées
    void enablePasteMode();               // active le mode collage
    void pasteCopiedShapes(const QPointF &dest); // colle à la position donnée

    // Nouvelle méthode pour appliquer un delta pan avec clamp
    void applyPanDelta(const QPointF &delta);


    void setSnapToGridEnabled(bool enabled) { m_snapToGrid = enabled; update(); }
    bool isSnapToGridEnabled()      const  { return m_snapToGrid;      }

    void setGridVisible(bool visible) { m_showGrid = visible; update(); }
    bool isGridVisible() const { return m_showGrid; }

    static QList<QPainterPath> separateIntoSubpaths(const QPainterPath &path);

    bool isDeplacerMode() const { return m_deplacerMode; }
    bool isSupprimerMode() const { return m_supprimerMode; }
    bool isGommeMode() const { return m_gommeMode; }
    bool isConnectMode() const { return m_connectSelectionMode; }

public slots:
    void undoLastAction(); // Annule la dernière action
    void mergeShapesAndConnector(int idx1, int idx2);
    /// Ferme la forme dont l’index est m_selectedShapeIndex
    void closeCurrentShape();
    void startCloseMode();
    void cancelCloseMode();
    void startDeplacerMode();
    void cancelDeplacerMode();
    void startSupprimerMode();
    void cancelSupprimerMode();
    void startGommeMode();
    void cancelGommeMode();


protected:
    // Gestion des événements de la souris et du clavier
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void clampOffsetToCanvas();
    QPointF clampToCanvas(const QPointF &p) const;


private:
    QList<QPointF> m_freehandPoints; // Liste des points du dessin à main levée
    QList<Shape> m_shapes; // Liste des formes finalisées

    // Pile d'annulation
    QList< QList<Shape> > m_undoStack;

    // États du dessin
    bool m_drawing;
    int m_smoothingLevel;
    int m_savedSmoothingLevel;   // sauvegarde du niveau pour le mode Freehand
    bool m_lowPassFilterEnabled = true;
    DrawMode m_drawMode = DrawMode::Freehand;
    DrawMode m_lastPrimaryMode = DrawMode::Freehand;

    // Variables pour le dessin de formes géométriques
    QPointF m_startPoint;
    QPointF m_currentPoint;
    bool m_drawingLineOrCircle = false;

    // Gestion de la gomme
    QImage m_canvas;
    bool m_gommeErasing = false;
    QPointF m_gommeCenter;
    qreal m_gommeRadius = 20.0;

    // Déplacement d'une forme
    int m_selectedShapeIndex = -1;
    bool m_shapeMoving = false;
    QPointF m_lastMovePoint;

    // Gestion du zoom et du déplacement global (pan)
    double m_scale = 1.0;
    QPointF m_offset = QPointF(0, 0);
    bool m_panningActive = false;

    // Variables pour gestion unique des formes
    int m_nextShapeId = 1;

    // --- Variables pour le mode texte ---
    QString m_currentText;    // Contenu par défaut du texte
    QFont   m_textFont;       // Police utilisée pour le texte

    // Fonctions utilitaires pour la génération de formes
    QPainterPath generateBezierPath(const QList<QPointF>& pts);
    QPainterPath generateRawPath(const QList<QPointF>& pts);
    double distance(const QPointF &p1, const QPointF &p2);
    QList<QPointF> applyChaikinAlgorithm(const QList<QPointF>& inputPoints, int iterations);
    int computeSmoothingIterations(const QList<QPointF> &pts) const;
    QList<QPointF> applyLowPassFilter(const QList<QPointF>& points, double alpha) const;
    double smoothingAlpha() const;

    // Gestion du canevas
    void initCanvas();
    void updateCanvas();
    void updateCanvas(const QRectF& logicalDirty);  // ← AJOUTER ceci
    void applyEraserAt(const QPointF &center); // gomme une zone circulaire

    // Gestion des actions annulables
    void pushState();

    // Calcule le rectangle englobant de toutes les formes sélectionnées
    QRectF selectedShapesBounds() const;

    // Recalculates the rotation handle when the selection changes
    void updateRotationHandle();

    bool   m_selectMode      = false;        // vrai si une sélection est active
    bool   m_connectSelectionMode = false;   // sélection utilisée pour la fonction "Relier"
    QVector<int> m_selectedShapes;            // indices des formes sélectionnées
    bool m_closeMode = false;               // vrai si on est en train de fermer
    QList<Shape> m_copiedShapes;            // formes copiées
    QPointF m_copyAnchor;                   // point d'ancrage de la copie
    bool m_pasteMode = false;               // vrai si un collage est en attente
    QPointF m_lastSelectClick;              // dernière position de sélection

    bool gestureEvent(QGestureEvent *event);
    bool pinchTriggered(QPinchGesture *gesture);

    TouchGestureReader* m_touchReader;
    bool m_twoFingersOn = false;

    bool   m_snapToGrid   = false;   // État du “snap”
    bool   m_showGrid     = true;    // Affichage de la grille
    int    m_gridSpacing  = 20;      // Doit rester synchronisé avec celui du paintEvent
    QPointF snapIfNeeded(const QPointF &p) const;

    // Clamp un point en coordonnées logiques à la zone du canevas
    //QPointF clampToCanvas(const QPointF &p) const;

    // Distance minimale entre deux points consécutifs lors du dessin libre
    qreal  m_minPointDistance = 2.0;
    // Seuil de distance moyen en dessous duquel on applique un lissage renforcé
    qreal  m_lowSpeedThreshold = 5.0;


    QPointF m_rotationHandle;      // Position du handle
    QPointF m_rotationCenter;      // Centre de rotation
    bool m_rotating = false;       // Vrai si on est en train de tourner
    qreal m_lastAngle = 0.0;       // Dernier angle mesuré
    qreal m_groupRotationAngle = 0.0; // Rotation cumulée du groupe sélectionné
    QSvgRenderer m_handleRenderer;

    bool m_deplacerMode = false;
    bool m_supprimerMode = false;
    bool m_gommeMode = false;

    // -- Helper to reset everything back to the default drawing mode
    void revertToFreehand();
    void restoreLastPrimaryMode();

    QPointF m_lastEraserPos;

    // Nouveau
    bool m_deferredErase = false; // évite updateCanvas() à chaque micro-coupure

    QElapsedTimer m_eraseTimer;
    qint64        m_lastEraseCommitMs = 0;
    QRect         m_dirty;               // zone à redessiner côté widget

    void eraseAlong(const QPointF& from, const QPointF& to);
    void commitEraseIfNeeded(bool force);

    void drawGrid(QPainter &painter);
signals:
    void zoomChanged(double newScale); // Signal pour informer d'un changement de zoom
    void closeModeChanged(bool enabled);
    void shapeSelection(bool enabled);
    void smoothingLevelChanged(int level); // émis lorsque le niveau est modifié
    void multiSelectionModeChanged(bool enabled);
    void deplacerModeChanged(bool enabled);
    void supprimerModeChanged(bool);
    void gommeModeChanged(bool);
    void drawModeChanged(DrawMode mode);

private slots:
    void onPinchZoom(const QPointF &center, qreal scaleFactor);
    void onTwoFingerPan(const QPointF &delta);

public slots:
    void handlePinchZoom(QPointF center, qreal factor);
    void setTwoFingersOn (bool active);
    void setGridSpacing(int px);     // px ≥ 1
    int  gridSpacing() const { return m_gridSpacing; }
    void setLowPassFilterEnabled(bool enabled) { m_lowPassFilterEnabled = enabled; }
    bool lowPassFilterEnabled() const { return m_lowPassFilterEnabled; }
    void setSmoothingLevel(int level); // Définit le niveau de lissage


};

#endif // CUSTOMDRAWAREA_H
