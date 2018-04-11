/**
 *   @class GLEngine
 *   @brief Command-line rendering engine.
 *
 *   Wrapper around GLCore for batch mode. Called by CMDAppCore.
 *
 */

#include "cli/glengine.h"

using namespace glcore;

GLEngine::GLEngine()
{
    initGLObject(obj);
    obj.polygonFill = SHOW_MESH;
    obj.viewPosX = 0.0;
    obj.viewPosY = 0.0;
    obj.viewMode = 0;
    obj.viewThreshold = DEFAULT_VIEW_THRESH;
}



GLEngine::~GLEngine()
{
}



int GLEngine::createGLContext()
{
    if (DEBUG_MODE) fprintf(stderr, "%s():\n", __FUNCTION__);
    int rv = glcore::createGLContext();

    return rv;
}



void GLEngine::initializeGL()
{
    if (DEBUG_MODE) fprintf(stderr, "%s():\n", __FUNCTION__);

    QDir resources( QCoreApplication::applicationDirPath() );
    resources.cd( RESOURCES );
    glcore::initializeGL( obj, resources.path().toStdString() );
}



/**
 * @brief Set rendering resolution.
 * - Call this after setScreenResolution() and initializeGL().
 *
 * @param w     Width
 * @param h     Height
 */
void GLEngine::resizeGL(int w, int h)
{
    glcore::resizeGL(obj, w, h);
}



/**
 * @brief Sets visual data for rendering.
 * @param tooth     Pointer to a tooth object.
 */
void GLEngine::setVisualData(ToothLife *toothlife, int step, Model *model)
{
    if (DEBUG_MODE) fprintf(stderr, "%s():\n", __FUNCTION__);

    Tooth *tooth = toothlife->getTooth(step-1);

    if (tooth->get_tooth_type() == RENDER_HUMPPA) {
        glcore::setVisualData( &(tooth->get_cell_data()), obj,
                               &(tooth->get_mesh()) );
    }
    if (tooth->get_tooth_type() == RENDER_PIXEL) {
        auto dim = tooth->get_domain_dim();
        glcore::setImageSize(dim.first*dim.second, obj);
        model->fill_image(tooth, obj.img);
        glcore::setVisualData2D(dim.first, dim.second, obj);
    }
}



void GLEngine::clearScreen()
{
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}



void GLEngine::setViewMode(int mode)
{
    fprintf(stderr, "Rendering mode: %d\n", mode);
    obj.viewMode = mode;
}



int GLEngine::getViewMode()
{
    return obj.viewMode;
}



void GLEngine::showConnections(int mode)
{
    obj.polygonFill = mode;
}



void GLEngine::setViewOrientation( float rotx, float roty )
{
    obj.rtriX = rotx;
    obj.rtriY = roty;
}



void GLEngine::setViewThreshold(double val)
{
    obj.viewThreshold = val;
}



QImage GLEngine::screenshotGL()
{
    glcore::screenshotGL( obj, obj.fbo_dim[0], obj.fbo_dim[1] );
    QImage qimg = QImage( obj.scrimg, obj.fbo_dim[0], obj.fbo_dim[1],
                          QImage::Format_RGB32 );
    return qimg.mirrored(false, true);
}



void GLEngine::setImageSize(int height, int width)
{
    glcore::setImageSize(height*width, obj);
}



void GLEngine::setRenderMode(int mode)
{
    if (DEBUG_MODE) fprintf(stderr, "%s(): Setting render mode: %d.\n",
                            __FUNCTION__, mode);
    glcore::setRenderMode(mode, obj);
}



/**
 * @brief Set image resolution, or fbo dimensions in this case.
 * @param w     Width
 * @param h     Height
 */
void GLEngine::setScreenResolution(int w, int h)
{
    obj.fbo_dim[0] = w;
    obj.fbo_dim[1] = h;
}
