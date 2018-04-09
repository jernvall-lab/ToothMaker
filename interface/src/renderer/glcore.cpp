/**
 *  @class GLCore
 *  @brief Common OpenGL core functionality.
 *
 *  Called by GLEngine for batch mode & GLWidget for GUI implementations.
 *
 *  NOTES:
 *  - Current rendering scheme requires two sets of buffers to be allocated, one for
 *    rendering & other for screenshot taking. This is because multisampled buffers
 *    can't be directly read with glReadPixels(), rather the multisampled data must be
 *    read into an non-multisampled buffers. The problem is, this requires double the
 *    amount of graphics memory.
 *  - The code is a mix of old-style immediate mode/fixed function and vbo/shaders.
 *    Some clean-up should be done, but unlikely as things work fairly well as they are...
 */

#include <iostream>
#include <stdlib.h>
#include <stdio.h>

#include <morphomaker.h>
#include "glcore.h"
#include "gl_modern.h"
#include "gl_legacy.h"



/**
 * @brief Initializes a GLObject.
 * @param obj       Pointer to a GLObject.
 * @return
 */
int glcore::initGLObject(GLObject **obj)
{
    if (DEBUG_MODE) fprintf(stderr, "%s():\n", __FUNCTION__);

    *obj = (GLObject*)malloc(sizeof(GLObject));
    if (*obj==NULL) {
        fprintf(stderr, "Error: Can't allocate memory for GLObject (%s()).\n",
                __FUNCTION__);
        return -1;
    }
    (*obj)->texName = 0;
    (*obj)->framebuffer = 0;
    (*obj)->renderbuffer[0] = 0;
    (*obj)->renderbuffer[1] = 0;

    (*obj)->renderMode = 0;
    (*obj)->viewMode = 0;
    (*obj)->pixelDataHeight = 0;
    (*obj)->pixelDataWidth = 0;
    (*obj)->zoomMultip = 0;
    (*obj)->fbo_dim[0] = 0;
    (*obj)->fbo_dim[1] = 0;

    (*obj)->rtriX = 0;
    (*obj)->rtriY = 0;
    (*obj)->deltaX = 0;
    (*obj)->deltaY = 0;
    (*obj)->zoomMultip = 1.0;
    (*obj)->mouse1Down = 0;
    (*obj)->mouse2Down = 0;

    (*obj)->img = NULL;
    (*obj)->scrimg = NULL;
    (*obj)->polygonFill = 0;

    (*obj)->mesh = NULL;
    (*obj)->cell_data = NULL;

    return 0;
}



/**
 * @brief Checks for GL errors.
 *        Call check_gl_error() rather than this function directly.
 *
 * @param file      Filename
 * @param line      Line
 */
void glcore::gl_error( std::string file, int line ) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::string err_str;

        switch (err) {
            case GL_INVALID_OPERATION:
                err_str = "INVALID_OPERATION";
                break;
            case GL_INVALID_ENUM:
                err_str = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                err_str = "INVALID_VALUE";
                break;
            case GL_OUT_OF_MEMORY:
                err_str = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                err_str = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }

        std::cerr << "OpenGL error (" << file << ":" << line << "): "
                  << err_str << std::endl;
    }
}



/**
 * @brief Uploads mesh data to the GPU.
 * @param obj       GLObject.
 * @param datatype  What to update; VERTICES and/or TEXTURES.
 */
void glcore::uploadData( GLObject *obj, int datatype )
{
    if ( obj->mesh == NULL ) return;

    if (datatype & VERTICES) {
        // Get polygon vertex data with normals and the corresponding vertex
        // indices for polygons.
        std::vector<GLfloat> tri_data;
        std::vector<GLuint> tri_indices;
        Set_vertex_data( obj->mesh, tri_data, tri_indices );

        GLint vertex_attrib = glGetAttribLocation( obj->shader_program, "vertex" );
        GLint normal_attrib = glGetAttribLocation( obj->shader_program, "normal" );

        // Triangles VAO.
        glBindVertexArray(obj->vao);

        glBindBuffer( GL_ARRAY_BUFFER, obj->vbo );
        glBufferData( GL_ARRAY_BUFFER, tri_data.size()*sizeof(GLfloat),
                      &tri_data.front(), GL_DYNAMIC_DRAW );

        glEnableVertexAttribArray( vertex_attrib );
        glVertexAttribPointer( vertex_attrib, 3, GL_FLOAT, GL_FALSE,
                               6*sizeof(GLfloat), 0 );

        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, obj->ebo_tri );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, tri_indices.size()*sizeof(GLuint),
                      &tri_indices.front(), GL_DYNAMIC_DRAW );

        glEnableVertexAttribArray( normal_attrib );
        glVertexAttribPointer( normal_attrib, 3, GL_FLOAT, GL_TRUE,
                               6*sizeof(GLfloat), (const GLvoid*)(3*sizeof(GLfloat)) );
    }

    if (datatype & TEXTURES) {
        std::vector<vertex_color> tri_color_data;
        Set_color_data( obj->mesh, tri_color_data );

        glBindVertexArray(obj->vao);
        glBindBuffer( GL_ARRAY_BUFFER, obj->cbo );
        glBufferData( GL_ARRAY_BUFFER, tri_color_data.size()*sizeof(vertex_color),
                      &tri_color_data.front(), GL_DYNAMIC_DRAW );

        GLint col_attrib = glGetAttribLocation( obj->shader_program, "color" );
        glEnableVertexAttribArray( col_attrib );
        glVertexAttribPointer( col_attrib, 4, GL_FLOAT, GL_FALSE, 0, 0 );
    }
}



/**
 * @brief Updates GL view.
 * @param obj       GLObject
 * @param type      PAINT_SCREEN or 0 to paint off-screen only
 */
void glcore::paintGL(GLObject *obj, int type)
{
    // Draw into the off-screen framebuffer.
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, obj->framebuffer);
    glClearColor(0.0,0.0,0.0,1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLint view[4];
    glGetIntegerv(GL_VIEWPORT, view);

    if (obj->renderMode == RENDER_PIXEL) {
        PaintGL_2D(obj, (GLdouble)view[2]/view[3]);
    }
    if (obj->renderMode == RENDER_HUMPPA) {
        PaintGL_RENDER_HUMPPA(obj, view[2], view[3]);
    }
    if (obj->renderMode == RENDER_MESH) {
        Draw_mesh( obj, view[2], view[3] );
    }
    glFlush();

    if (type & PAINT_SCREEN) {
        // Switch back to screen fb for drawing, read from off-screen fb
        glBindFramebuffer(GL_READ_FRAMEBUFFER, obj->framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, view[2], view[3], 0, 0, view[2], view[3],
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
}



/**
 * @brief Resize GL viewport.
 * @param w         Width.
 * @param h         Height.
 */
void glcore::resizeGL(GLObject* obj, int w, int h)
{
    if (w > obj->fbo_dim[0]) {
        w = obj->fbo_dim[0];
    }
    if (h > obj->fbo_dim[1]) {
        h = obj->fbo_dim[1];
    }

    glViewport( 0, 0, (GLsizei)w, (GLsizei)h );
}


/**
 * @brief Set vertex data.
 * @param visualData        Vertex coordinates.
 * @param celldata          Cell values (concentrations etc.).
 * @param faces             Number of faces.
 * @param obj               GLObject.
 */
void glcore::setVisualData( std::vector<std::vector<float>>* cell_data,
                            GLObject* obj, Mesh* m)
{
    obj->cell_data = cell_data;
    obj->mesh = m;
}


/**
 * @brief Set pixel data.
 * @param data          Pixel data.
 * @param height        Height of the image.
 * @param width         Width of the image.
 * @param obj           GLObject.
 */
void glcore::setVisualData2D(int height, int width, GLObject *obj)
{
    obj->pixelDataHeight = height;
    obj->pixelDataWidth = width;
}



/**
 * @brief Creates GL context using OSMesa. Full off-screen sw rendering. DISABLED.
 * @return
 */
#if defined(__linux__)
int glcore::createGLContext_OSMesa()
{
    if (DEBUG_MODE) fprintf(stderr, "%s():\n", __FUNCTION__);
/*
    OSMesaContext ctx;
    ctx = OSMesaCreateContextExt(OSMESA_RGBA, 24, 0, 0, NULL);
    if (!ctx) {
        printf("Error: OSMesaCreateContext failed. Aborting.\n");
        return -1;
    }
    void *buffer = malloc(SQUARE_WIN_SIZE * SQUARE_WIN_SIZE * 4 * sizeof(GLubyte));
    if (!buffer) {
        printf("Error: Alloc image buffer failed. Aborting.\n");
        return -1;
    }

    if (!OSMesaMakeCurrent(ctx, buffer, GL_UNSIGNED_BYTE, SQUARE_WIN_SIZE,
                           SQUARE_WIN_SIZE)) {
        printf("Error: OSMesaMakeCurrent failed. Aborting.\n");
        return -1;
    }

    fprintf(stderr, "System OpenGL version: %s\n", (char*)glGetString(GL_VERSION));
*/
    return 0;
}
#endif



#if defined(__linux__)
/**
 * @brief Create GL context for off-screen rendering in Linux.
 * - Requires GL3 features & doesn't really work with Mesa.
 *
 * @return      0 OK
 * @return      -1 Error
 */
int glcore::createGLContext()
{
    if (DEBUG_MODE) fprintf(stderr, "%s():\n", __FUNCTION__);

    Display* display = XOpenDisplay(0);
    if (display==NULL) {
        fprintf(stderr, "Error: Can't open display. Aborting. \n");
        fprintf(stderr, "*** Please check that DISPLAY environment variable is set correctly (usually DISPLAY=\":0\").\n");
        return -1;
    }

    typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig,
                                                         GLXContext, Bool, const int*);
    typedef Bool (*glXMakeContextCurrentARBProc)(Display*, GLXDrawable, GLXDrawable,
                                                 GLXContext);
    static glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
    static glXMakeContextCurrentARBProc glXMakeContextCurrentARB = 0;

    static int visualAttribs[] = {None};
    int fbcount=0;
    GLXFBConfig* fbConfigs = glXChooseFBConfig(display, DefaultScreen(display),
                                               visualAttribs, &fbcount);
    if (fbConfigs == NULL) {
        fprintf(stderr, "Error: glXChooseFBConfig() failed.\n");
        return -1;
    }

    // Request OpenGL version 3.0:
    int context_attribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 0,
        None
    };

    glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB((const GLubyte *) "glXCreateContextAttribsARB");
    glXMakeContextCurrentARB = (glXMakeContextCurrentARBProc)glXGetProcAddressARB((const GLubyte *) "glXMakeContextCurrent");
    if (!(glXCreateContextAttribsARB && glXMakeContextCurrentARB)) {
        fprintf(stderr, "Error: Missing support for GLX_ARB_create_context.\n");
        fprintf(stderr, "*** This feature requires GLX >=1.4 and OpenGL >=3.0.\n");
        return -1;
    }

    GLXContext openGLContext = glXCreateContextAttribsARB(display, fbConfigs[0], 0, True,
                                                          context_attribs);
    if (openGLContext == NULL) {
        fprintf(stderr, "Error: glXCreateContextAttribsARB() failed.\n");
        return -1;
    }

    // Apparently some fake pbuffer is needed for glxMakeContextCurrent:
    int pbufferAttribs[] = {
        GLX_PBUFFER_WIDTH,  32,
        GLX_PBUFFER_HEIGHT, 32,
        None
    };
    GLXPbuffer pbuffer = glXCreatePbuffer(display, fbConfigs[0], pbufferAttribs);

    // Clean up:
    XFree(fbConfigs);
    XSync(display, False);

    if (!glXMakeContextCurrent(display, pbuffer, pbuffer, openGLContext)) {
        fprintf(stderr, "Error: glXMakeContextCurrent() failed.\n");
        return -1;
    }

    if (DEBUG_MODE) fprintf(stderr, "System OpenGL version: %s\n",
                            (char*)glGetString(GL_VERSION));
    if (DEBUG_MODE) fprintf(stderr, "OpenGL context creation OK.\n");

    return 0;
}


#elif defined(__APPLE__)
/**
 * @brief Create GL context for off-screen rendering in OS X.
 * @return
 */
int glcore::createGLContext()
{
    if (DEBUG_MODE) fprintf(stderr, "%s():\n", __FUNCTION__);

    CGLContextObj context;
    CGLPixelFormatAttribute attributes[1] = {
        // NOTE: kCGLOGLPVversion_Legacy is not supported in OS X 10.6.
        // kCGLPFAOpenGLProfile, (CGLPixelFormatAttribute) kCGLOGLPVersion_Legacy,
        // In OS X 'legacy' refers to v2.1 + Apple extras.
        (CGLPixelFormatAttribute) 0
    };

    CGLPixelFormatObj pix;
    CGLError errorCode;
    GLint num; // Number of possible pixel formats.
    errorCode = CGLChoosePixelFormat(attributes, &pix, &num);
    // TODO: add error checking here
    // Second parameter could be another context for object sharing.
    errorCode = CGLCreateContext(pix, NULL, &context);
    // TODO: add error checking here
    CGLDestroyPixelFormat(pix);
    errorCode = CGLSetCurrentContext(context);

    if (DEBUG_MODE) {
        fprintf(stderr, "System OpenGL version: %s\n",
                (char*)glGetString(GL_VERSION));
        fprintf(stderr, "OpenGL context creation OK.\n");
        if (errorCode) {
            fprintf(stderr, "Error code: %d\n", errorCode);
        }
    }

    return 0;
}


#else
/**
 * @brief Dummy context creation for Windows.
 * - TODO: This needs to be implemented if batch mode is to be used in Windows.
 *
 * @return
 */
int glcore::createGLContext()
{
    return 0;
}
#endif



/**
 * @brief Initializes OpenGL. Sets lightning, creates fbos etc.
 * @param obj           GLObject
 * @param shaders_path  Path to the shader files.
 * @return              0 if success, else -1.
 */
int glcore::initializeGL( GLObject *obj, std::string shaders_path )
{
    if (DEBUG_MODE) fprintf(stderr, "%s():\n", __FUNCTION__);

    // Setting the texture for 2D models:
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &obj->texName);
    glBindTexture(GL_TEXTURE_2D, obj->texName);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    check_gl_error();

    // Framebuffer & associated renderbuffers for off-screen rendering.
    // Max. 4 samples for multisampling:
    int nSamples;
    glGetIntegerv(GL_MAX_SAMPLES, &nSamples);
    if (DEBUG_MODE) fprintf(stderr, "Max. samples: %d\n", nSamples);
    if (nSamples > 4) {
        nSamples = 4;
    }

    // NOTE: gl*Framebuffer* require OpenGL 3.0.
    glGenRenderbuffers(2, obj->renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, obj->renderbuffer[0]);  // For color.
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, nSamples, GL_RGBA,
                                     obj->fbo_dim[0], obj->fbo_dim[1]);
    glBindRenderbuffer(GL_RENDERBUFFER, obj->renderbuffer[1]);  // For depth.
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, nSamples, GL_DEPTH_COMPONENT24,
                                     obj->fbo_dim[0], obj->fbo_dim[1]);
    glGenFramebuffers(1, &obj->framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, obj->framebuffer);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
                              obj->renderbuffer[0]);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                              obj->renderbuffer[1]);
    check_gl_error();

    // fbo & render buffers for screen captures.
    // Separate buffers required due to multisampling not being supported:
    glGenRenderbuffers(2, obj->scrrender);
    glBindRenderbuffer(GL_RENDERBUFFER, obj->scrrender[0]);  // For color.
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 0, GL_RGBA,
                                     obj->fbo_dim[0], obj->fbo_dim[1]);
    glBindRenderbuffer(GL_RENDERBUFFER, obj->scrrender[1]);  // For depth.
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 0, GL_DEPTH_COMPONENT24,
                                     obj->fbo_dim[0], obj->fbo_dim[1]);
    glGenFramebuffers(1, &obj->scrfbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, obj->scrfbo);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
                              obj->scrrender[0]);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                              obj->scrrender[1]);
    check_gl_error();

    // Buffers for 3D stuff.
    glGenBuffers(1, &obj->cbo);
    glGenBuffers(1, &obj->ebo_tri);
    glGenBuffers(1, &obj->vbo);
    glGenVertexArrays(1, &obj->vao);
    check_gl_error();

    // Create and compile the vertex shader.
    GLuint vertex_shader = glCreateShader( GL_VERTEX_SHADER );
    std::string shader_file = shaders_path + "/vertex.glsl";
    shader_file = Read_shader_file( shader_file );
    const GLchar* source = shader_file.c_str();
    glShaderSource( vertex_shader, 1, &source, NULL );
    glCompileShader( vertex_shader );
    Shader_log( "Vertex", vertex_shader );
    check_gl_error();

    // Create and compile the fragment shader.
    GLuint fragment_shader = glCreateShader( GL_FRAGMENT_SHADER );
    shader_file = shaders_path + "/fragment.glsl";
    shader_file = Read_shader_file( shader_file );
    source = shader_file.c_str();
    glShaderSource( fragment_shader, 1, &source, NULL );
    glCompileShader( fragment_shader );
    Shader_log( "Fragment", fragment_shader );
    check_gl_error();

    // Link the vertex and fragment shaders into a shader program.
    obj->shader_program = glCreateProgram();
    glAttachShader( obj->shader_program, vertex_shader );
    glAttachShader( obj->shader_program, fragment_shader );
    // Only one output from framgent shader, so shouldn't need this:
    // glBindFragDataLocation(shaderProgram, 1, "gl_FragColor");
    glLinkProgram( obj->shader_program );
    check_gl_error();

    // Install the shader program as part of current rendering state.
    // NOTE: We will be swithing between the programmable and legacy fixed
    // function pipelines in setRenderMode() as needed.
    glUseProgram( obj->shader_program );
    check_gl_error();

    return 0;
}



/**
 * @brief Sets current render mode (RENDER_PIXEL or RENDER_MESH).
 * @param mode          Render mode.
 * @param obj           GLObject.
 */
void glcore::setRenderMode(int mode, GLObject *obj)
{
    if (DEBUG_MODE) fprintf(stderr, "%s:%s(%d, ...)\n", __FILE__, __FUNCTION__, mode);

    if (mode == RENDER_PIXEL || mode == RENDER_HUMPPA) {
        // Enter fixed function pipeline.
        // NOTE: In 3.1+ core profile glUseProgram(0) is not allowed!
        glUseProgram(0);

        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

        // Lightning:
        glShadeModel(GL_SMOOTH);
        GLfloat light_diffuse[] = {1.0, 1.0, 1.0, 1.0};
        GLfloat light_ambient[] = {0.5, 0.5, 0.5, 1.0};
        glLightfv(GL_LIGHT0, GL_SPECULAR, light_diffuse);
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light_ambient);
    }

    if (mode == RENDER_PIXEL) {
        glDisable(GL_CULL_FACE);
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        glDisable(GL_LIGHT0);
    }

    if (mode == RENDER_HUMPPA) {
        // Humppa gives incorrect surface normals, need some culling to
        // circumvent the problem. Cannot distinguish 'up' and 'down' after.
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_DEPTH_TEST);

        glDisable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
    }

    if (mode == RENDER_MESH) {
        // Enter programmable pipeline.
        glUseProgram( obj->shader_program );

        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        glDisable(GL_LIGHT0);
    }

    obj->renderMode = mode;
}



/**
 * @brief Allocates memory for pixel data image/texture.
 * @param n             Number of pixels.
 * @param obj           GLObject.
 */
void glcore::setImageSize(int n, GLObject *obj)
{
    if (obj->img!=NULL) free(obj->img);

    obj->img = (GLfloat*)malloc(n*4*sizeof(GLfloat));
    if (obj->img==NULL) {
        fprintf(stderr, "Error: memory allocation failed (%s()).\n", __FUNCTION__);
        return;
    }
}



/**
 * @brief Takes a screenshot of the current model view.
 *
 * - Reads from off-screen framebuffer and writes into a dedicated screenshot
 *   buffer of given size.
 *
 * @param obj           GLObject.
 * @param w             Screenshot buffer width.
 * @param h             Screenshot buffer height.
 */
void glcore::screenshotGL(GLObject *obj, int w, int h)
{
    // Store the original view port dimensions.
    GLint view_old[4];
    glGetIntegerv(GL_VIEWPORT, view_old);

    resizeGL(obj, w, h);
    paintGL(obj, 0);    // Update off-screen framebuffer only.
    glFinish();
    check_gl_error();

    // Read from off-screen buffer, write to screenshot buffer.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, obj->framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, obj->scrfbo);
    glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    check_gl_error();

    // Allocate space for 4-component image buffer.
    if (obj->scrimg != NULL) {
        free(obj->scrimg);
    }
    obj->scrimg = (GLubyte*)malloc( w*h*4 );
    if (obj->scrimg == NULL) {
        fprintf(stderr, "Error: memory allocation failed (%s()).\n", __FUNCTION__);
        return;
    }

    // Read from screenshot buffer, write to image buffer.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, obj->scrfbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, w, h, GL_BGRA, GL_UNSIGNED_BYTE, obj->scrimg);
    glFinish();
    check_gl_error();

    // Reset view port size to what it was.
    resizeGL(obj, view_old[2], view_old[3]);

    if (DEBUG_MODE) {
        fprintf(stderr, "glCheckFramebufferStatus(GL_READ_FRAMEBUFFER): %d\n",
                glCheckFramebufferStatus(GL_READ_FRAMEBUFFER));
    }
}
