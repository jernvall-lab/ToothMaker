#pragma once

#include <QtGui>
#include <QGLWidget>
#include <QtGui>
#include <QtOpenGL>
#include <QSizePolicy>
#include <QGLFormat>
#include "tooth.h"
#include "toothlife.h"
#include "model.h"
#include "renderer/glcore.h"


// Allocates space for framebuffers as screen resolution times FBO_MULTIPLIER in
// each dimension.
#define FBO_MULTIPLIER 2


class GLWidget : public QGLWidget
{
    Q_OBJECT

    public:
        GLWidget(const QGLFormat &format, QWidget *parent=0, QGLWidget *shareWidget=0);
        ~GLWidget();

        void paintGL();
        void initializeGL();
        void resizeGL(int, int);
        void mousePressEvent(QMouseEvent *);
        void mouseReleaseEvent(QMouseEvent *);
        void mouseMoveEvent(QMouseEvent *);
        void keyPressEvent(QKeyEvent *);
        void wheelEvent(QWheelEvent *);
        QSize sizeHint() const;

        void setVisualData(ToothLife *, int, Model *);
        void clearScreen();
        void setViewMode(int, Tooth* tooth, Model *model);
        void setViewThreshold(double, Tooth*, Model *);
        void showMesh(int);
        void setViewOrientation(float, float);
        QImage screenshotGL();
        void setRenderMode(int);
        void setRotations(bool);

    signals:
        void changeStepView(int);           // Current view step changed.
        void resetOrientation(int);         // Emitted when object has been rotated.
        void msgStatusBar(std::string);     // Write to status bar.

    private:
        GLObject obj;                       // See glcore.h for definition.
        bool allowRotations;                // If false, only object panning allowed.
};
