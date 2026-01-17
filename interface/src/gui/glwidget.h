#pragma once

#include <QtGui>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QSizePolicy>
#include <QSurfaceFormat>
#include "tooth.h"
#include "toothlife.h"
#include "model.h"
#include "renderer/glcore.h"


// Allocates space for framebuffers as screen resolution times FBO_MULTIPLIER in
// each dimension.
#define FBO_MULTIPLIER 2


class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

    public:
        GLWidget(QWidget *parent=0);
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

        // Deferred state for thread-safe updates (applied in paintGL).
        // This avoids makeCurrent() calls outside Qt callbacks, preventing deadlocks.
        bool m_renderModeChanged = false;
        int m_pendingRenderMode = 0;

        bool m_visualDataChanged = false;
        ToothLife* m_pendingToothLife = nullptr;
        int m_pendingStep = 0;
        Model* m_pendingModel = nullptr;

        bool m_texturesChanged = false;
        Tooth* m_pendingTooth = nullptr;
        // m_pendingModel reused for textures

        void applyVisualData_();            // Apply deferred visual data (called from paintGL)
};
