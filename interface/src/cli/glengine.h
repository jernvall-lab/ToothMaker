#ifndef GLEngine_H
#define GLEngine_H

#if defined(__linux__)
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#if defined(__APPLE__)
#include <OpenGL/OpenGL.h>
#endif

#include <QtGui>
#include <QGLContext>
#include <QtGui>
#include <QtOpenGL>
#include <QSizePolicy>
#include <QGLFormat>
#include <readdata.h>
#include <toothlife.h>
#include <model.h>
#include <renderer/glcore.h>

#if defined(__linux__)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>
#endif



#define DEFAULT_TOOTH_COL 0.5   // 0.5 means middle gray.
#define WIDGET_TOKEN 0          // Widget identifier for screenshot.


class GLEngine
{

    public:
        GLEngine();
        ~GLEngine();

        void initializeGL();
        void togglePolygonFill();
        void resizeGL(int, int);
        void setVisualData(ToothLife *, int, Model *);
        void clearScreen();
        void setViewMode(int);
        int getViewMode();
        void showConnections(int);
        void setViewOrientation(float, float);
        void setViewThreshold(double);

        QImage screenshotGL();
        void setImageSize(int, int);
        void setRenderMode(int);
        int createGLContext();
        void setScreenResolution(int, int);

    signals:
        void msgStatusBar(std::string);

    private:
        GLObject *obj;
};


#endif
