#pragma once
#include <QWidget>

class AspectRatioWrapper : public QWidget {
    Q_OBJECT
public:
    explicit AspectRatioWrapper(QWidget* child = nullptr,
                                double aspect = 0.0,
                                QWidget* parent = nullptr);

    void   setAspect(double aspect);
    double aspect() const { return m_aspect; }

    // NOUVEAU : adopter l’enfant après l’insertion du wrapper dans le layout
    void setChild(QWidget* child);
    QWidget* child() const { return m_child; }

    bool hasHeightForWidth() const override { return (m_child && m_child->hasHeightForWidth()) || m_aspect > 0.0; }
    int  heightForWidth(int w) const override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void resizeEvent(QResizeEvent *e) override;

private:
    void applyLetterbox();   // calcule et pose la géométrie de l’enfant

    QWidget* m_child  = nullptr;
    double   m_aspect = 0.0; // width / height
};
