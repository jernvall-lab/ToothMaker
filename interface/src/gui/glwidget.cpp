/**
 *  @class GLWidget
 *  @brief Model viewer widget for the GUI.
 *
 *  Takes care of creating OpenGL context, event handling for mouse and keyboard, and
 *  other tasks such as initiating screenshot taking. The actual rendering takes place
 *  in GLCore, which is a non-QT module.
 *
 */

#include <iostream>
#include <sstream>
#include <gui/glwidget.h>
#include <gui/controlpanel.h>   // Control panel dimensions for fbo allocation.


namespace {

/**
 * @brief Updates texturing/vertex colors.
 *        Called whenever new data available (setVisualData), or when the user
 *        requests view threshold change, view mode change.
 * @param tooth     Current tooth object.
 * @param model     Current model object.
 * @param obj       GLObject.
 */
void _update_textures( Tooth* tooth, Model* model, GLObject* obj )
{
    if (tooth->get_tooth_type() == RENDER_PIXEL) {
        auto dim = tooth->get_domain_dim();
        glcore::setImageSize( dim.first*dim.second, obj);
        model->fill_image(tooth, obj->img);
        glcore::setVisualData2D( dim.first, dim.second, obj );
    }
    else {
        obj->mesh = &(model->fill_mesh( *tooth ));
        glcore::uploadData( obj, TEXTURES );
    }
}

}   // END namespace



/**
 * @brief Class constructor.
 * @param format        Context options.
 * @param parent
 * @param shareWidget
 */
GLWidget::GLWidget(const QGLFormat &format, QWidget *parent, QGLWidget *shareWidget) :
                   QGLWidget(format, parent, shareWidget)
{
    setMinimumSize(SQUARE_WIN_SIZE,SQUARE_WIN_SIZE);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    if (DEBUG_MODE) fprintf(stderr, "Double buffering: %d\n", doubleBuffer());
    setAutoBufferSwap(true);

    glcore::initGLObject(&obj);
    obj->polygonFill = SHOW_MESH;
    obj->viewPosX = 0.0;
    obj->viewPosY = 0.0;
    obj->viewMode = 0;
    obj->viewThreshold = DEFAULT_VIEW_THRESH;
}



/**
 * @brief Class destructor.
 */
GLWidget::~GLWidget()
{
}



/**
 * @brief Paints the model view.
 */
void GLWidget::paintGL()
{
    glcore::paintGL(obj, PAINT_SCREEN);
}



/**
 * @brief Initialize model viewer.
 *
 * - The maximum fbo size is computed from the reduced screen resolution under
 *   the assumption that the parameter window occupies SQUARE_WIN_SIZE in width,
 *   and the control panel occupies CONTROLPANEL_HEIGHT from the bottom of
 *   the screen.
 *
 *   Issues:
 *   - The viewer behaviour is undefined if the users scales the window such
 *     that model viewer occupies more than the screen dimensions.
 */
void GLWidget::initializeGL()
{
    if (DEBUG_MODE) fprintf(stderr, "%s():\n", __FUNCTION__);

    QRect geom = QApplication::desktop()->screenGeometry();
    obj->fbo_dim[0] = (geom.width() - SQUARE_WIN_SIZE) * FBO_MULTIPLIER;
    obj->fbo_dim[1] = (geom.height() - CONTROLPANEL_HEIGHT) * FBO_MULTIPLIER;

    std::cout << "Framebuffer size: " << obj->fbo_dim[0] << "x"
              << obj->fbo_dim[1] << " (device-independent pixels " << geom.width()
              << "x" << geom.height() << ", multiplier " << FBO_MULTIPLIER
              << ", parameter window width " << SQUARE_WIN_SIZE << ")."
              << std::endl;

    QDir resources( QCoreApplication::applicationDirPath() );
    resources.cd( RESOURCES );
    glcore::initializeGL( obj, resources.path().toStdString() );
}



/**
 * @brief Called when model viewer resized.
 * @param w     Widget width.
 * @param h     Widget height.
 */
void GLWidget::resizeGL(int w, int h)
{
    if (DEBUG_MODE) fprintf(stderr, "w: %d, h: %d\n", w,h);
    glcore::resizeGL(obj, w, h);
}



/**
 * @brief Called when mouse button pressed.
 * @param event     QMouseEvent object
 */
void GLWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button()==1) obj->mouse1Down=1;
    if (event->button()==2) obj->mouse2Down=1;
    obj->startX=event->x();
    obj->startY=event->y();
    obj->deltaX=0;
    obj->deltaY=0;

    setFocus();
}



/**
 * @brief Called when mouse button released.
 * @param event     QMouseEvent object
 */
void GLWidget::mouseReleaseEvent(QMouseEvent *event)
{
   if (event->button()==1) obj->mouse1Down=0;
   if (event->button()==2) obj->mouse2Down=0;
}



/**
 * @brief Called when mouse button pressed & mouse moved.
 * @param event     QMouseEvent object.
 */
void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    // Disable all mouse controls in 2D model view:
    if (obj->renderMode == RENDER_PIXEL) return;

    // To have the object move at correct speed on hidpi displays, multiply the
    // deltas by the ratio between physical pixels and device-independent points.
    obj->deltaX = (obj->startX - event->x()) * devicePixelRatio();
    obj->deltaY = (obj->startY - event->y()) * devicePixelRatio();

    // Mouse button 1 rotates the object.
    if (obj->mouse1Down) {
        if (allowRotations == false) {
            obj->deltaX = 0;
            obj->deltaY = 0;
        }
        emit resetOrientation(0);
        updateGL();
    }

    // Mouse button 2 pans the object.
    if (obj->mouse2Down) {
        std::stringstream ss;
        ss << "Position: (" << obj->viewPosX << ", " << obj->viewPosY << ")";
        emit msgStatusBar(ss.str());
        updateGL();
    }

    obj->startX = event->x();
    obj->startY = event->y();
    obj->deltaX = 0;
    obj->deltaY = 0;
}



/**
 * @brief Called when key event detected.
 * @param event
 */
void GLWidget::keyPressEvent(QKeyEvent *event)
{
    if (obj->renderMode == RENDER_PIXEL) {
        return;   // Disable all orientation etc. controls in 2D model view.
    }

    std::stringstream ss;

    // View orientations:
    if (event->key()==49) {
        obj->zoomMultip=0.2;
        ss << "Distance: " << obj->zoomMultip;
        emit msgStatusBar(ss.str());
    }
    if (event->key()==50) {
        obj->zoomMultip=0.3;
        ss << "Distance: " << obj->zoomMultip;
        emit msgStatusBar(ss.str());
    }
    if (event->key()==51) {
        obj->zoomMultip=0.4;
        ss << "Distance: " << obj->zoomMultip;
        emit msgStatusBar(ss.str());
    }
    if (event->key()==52) {
        obj->zoomMultip=50.5;
        ss << "Distance: " << obj->zoomMultip;
        emit msgStatusBar(ss.str());
    }
    if (event->key()==53) {
        obj->zoomMultip=100.6;
        ss << "Distance: " << obj->zoomMultip;
        emit msgStatusBar(ss.str());
    }
    if (event->key()==67) {  // Center object
        obj->viewPosX=0.0;
        obj->viewPosY=0.0;
        ss << "Position: (" << obj->viewPosX << ", " << obj->viewPosY << ")";
        emit msgStatusBar(ss.str());
    }
    if (event->key()==16777234) {  // Left
        emit changeStepView(-1);
    }
    if (event->key()==16777236) {  // Right
        emit changeStepView(1);
    }
    if (event->key()==16777235) { // Up
        obj->zoomMultip = obj->zoomMultip+0.01;
        ss << "Distance: " << obj->zoomMultip;
        emit msgStatusBar(ss.str());
    }
    if (event->key()==16777237) { // Down
        obj->zoomMultip = obj->zoomMultip-0.01;
        ss << "Distance: " << obj->zoomMultip;
        emit msgStatusBar(ss.str());
    }

    updateGL();
}



/**
 * @brief Called when mouse wheel event detected.
 * @param event
 */
void GLWidget::wheelEvent(QWheelEvent *event)
{
    if (obj->renderMode==RENDER_PIXEL) return;   // Disable mouse scroll for 2D model view.

    obj->zoomMultip = obj->zoomMultip + (float)event->delta()/WHEEL_SENSITIVITY;
    if (obj->zoomMultip<ZOOM_MIN_MULTIP) obj->zoomMultip=ZOOM_MIN_MULTIP;
    if (obj->zoomMultip>ZOOM_MAX_MULTIP) obj->zoomMultip=ZOOM_MAX_MULTIP;

    std::stringstream ss;
    ss << "Distance: " << obj->zoomMultip;
    emit msgStatusBar(ss.str());

    updateGL();
}



/**
 * @brief Model view sizing.
 * @return
 */
QSize GLWidget::sizeHint() const
{
    return QSize(SQUARE_WIN_SIZE,SQUARE_WIN_SIZE);
}




/**
 * @brief Sets visual data for rendering.
 * @param tooth     Pointer to a tooth object.
 * @param step      Current step.
 * @param model     Current model object.
 */
void GLWidget::setVisualData(ToothLife *toothlife, int step, Model *model)
{
    if (toothlife == NULL || toothlife->getTooth(step) == NULL) {
        glcore::setVisualData(NULL, obj, NULL);
        obj->img = NULL;
        glcore::setVisualData2D(0, 0, obj);
        updateGL();
        return;
    }

    Tooth *tooth = toothlife->getTooth(step);

    if (tooth->get_tooth_type() == RENDER_HUMPPA) {
        glcore::setVisualData( &(tooth->get_cell_data()), obj, &(tooth->get_mesh()) );
    }
    else if (tooth->get_tooth_type() == RENDER_PIXEL) {
        _update_textures( tooth, model, obj );
    }
    else {
        obj->mesh = &(model->fill_mesh( *tooth ));
        glcore::uploadData(obj, VERTICES);
        glcore::uploadData(obj, TEXTURES);             // Vertex colors.
    }

    updateGL();
}



/**
 * @brief Zeroes all object data.
 */
void GLWidget::clearScreen()
{
    obj->mesh = NULL;
    obj->cell_data = NULL;
    obj->pixelDataHeight = 0;
    obj->pixelDataWidth = 0;
}



/**
 * @brief Sets current view mode, B&W, RGB colors..
 * @param mode      View mode.
 */
void GLWidget::setViewMode(int mode, Tooth* tooth, Model *model)
{
    if (DEBUG_MODE) fprintf(stderr, "%s:%s(%d, ...)\n", __FILE__, __FUNCTION__, mode);

    obj->viewMode = mode;
    if (tooth!=NULL) {
        _update_textures( tooth, model, obj );
        updateGL();
    }
}



/**
 * @brief Sets current view threshold.
 * @param val       View threshold.
 */
void GLWidget::setViewThreshold(double val, Tooth* tooth, Model *model)
{
    if (DEBUG_MODE) fprintf(stderr, "%s:%s(%lf)\n", __FILE__, __FUNCTION__, val);

    obj->viewThreshold = val;
    if (tooth!=NULL) {
        _update_textures( tooth, model, obj );
        updateGL();
    }
}



/**
 * @brief Draw mesh edges.
 * @param mode      Boolean.
 */
void GLWidget::showMesh(int mode)
{
    if (DEBUG_MODE) printf("%s: %d\n", __FUNCTION__, mode);

    obj->polygonFill = mode;
    updateGL();
}



/**
 * @brief View orientation definitions for 3D models.
 * @param i     Current view mode.
 * @param rots  Rotations.
 */
void GLWidget::setViewOrientation( float rotx, float roty )
{
    obj->rtriX = rotx;
    obj->rtriY = roty;
    updateGL();
}



/**
 * @brief Takes a screenshot of the current model view.
 * @return
 */
QImage GLWidget::screenshotGL()
{
    // Screenshot dimensions are set to twice the model view dimensions.
    int w = width() * FBO_MULTIPLIER;
    int h = height() * FBO_MULTIPLIER;

    // Max. resolution limited by fbo dimensions.
    if (w > obj->fbo_dim[0] || h > obj->fbo_dim[1]) {
        w = width();
        h = height();
    }

    glcore::screenshotGL(obj, w, h);
    QImage qimg = QImage(obj->scrimg, w, h, QImage::Format_RGB32);

    return qimg.mirrored(false, true);
}



/**
 * @brief Sets current render mode, e.g. 3D vertex data, 2D hexa data..
 * @param mode      RENDER_MESH, RENDER_PIXEL or RENDER_HUMPPA.
 */
void GLWidget::setRenderMode(int mode)
{
    if (DEBUG_MODE) fprintf(stderr, "%s:%s(%d)\n", __FILE__, __FUNCTION__, mode);

    if (obj->renderMode != mode) {
        clearScreen();
    }
    glcore::setRenderMode(mode, obj);
}



/**
 * @brief Enable/disable object rotations controlled with mouse button 1.
 * @param state     'true' to enable rotations, 'false' to disable.
 */
void GLWidget::setRotations(bool state)
{
    allowRotations = state;

    if (allowRotations == false) {
        obj->rtriX = 0.0;
        obj->rtriY = 0.0;
    }
}
