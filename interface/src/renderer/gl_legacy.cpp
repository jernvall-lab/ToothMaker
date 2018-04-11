/**
 *  @file gl_legacy.cpp
 *  @brief Functionality for 2D and Humppa drawing.
 *
 *  This is all legacy code, using fixed functionality and immediate mode stuff
 *  that requires OpenGL compatibility profile. New code should not call these
 *  methods!
 */

#include <string>
#include <math.h>

#include "gl_legacy.h"
#include "morphomaker.h"        // SQUARE_WIN_SIZE


namespace {

/**
 * @brief Calculates unit normal to given two vector.
 *        Legacy function.
 * @param a         Vector a.
 * @param b         Vector b.
 * @param c         Unit normal.
 */
void get_surface_normal_(GLdouble *a, GLdouble *b, GLdouble *c)
{
    c[0] = a[1]*b[2] - a[2]*b[1];
    c[1] = a[2]*b[0] - a[0]*b[2];
    c[2] = a[0]*b[1] - a[1]*b[0];

    GLdouble norm = sqrt( c[0]*c[0] + c[1]*c[1] + c[2]*c[2] );

    if (norm != 0.0) {
        c[0] = c[0]/norm;
        c[1] = c[1]/norm;
        c[2] = c[2]/norm;
    }
}

}   // END namespace


/**
 * @brief Vertex coloring for RENDER_HUMPPA.
 *        Legacy function.
 * @param cell          Cell index.
 * @param obj           GLObject.
 */
void glcore::Vertex_color_RENDER_HUMPPA(int cell, GLObject& obj)
{
    GLfloat colorArr[4];
    float color;

    if (obj.cell_data == NULL || obj.viewMode == 0) { // Mode: Shape only
        colorArr[0] = DEFAULT_TOOTH_COL;
        colorArr[1] = DEFAULT_TOOTH_COL;
        colorArr[2] = DEFAULT_TOOTH_COL;
        colorArr[3] = DEFAULT_TOOTH_COL;
    }

    else if (obj.viewMode==1) {                        // Mode: Diff & knots.
        auto& colors = obj.mesh->get_vertex_colors();
        colorArr[0] = colors.at(cell).r;
        colorArr[1] = colors.at(cell).g;
        colorArr[2] = colors.at(cell).b;
        colorArr[3] = colors.at(cell).a;
    }
    else {    // For all other view modes use red above threshold concentration.
        color = 0.0;
        auto data = obj.cell_data->at(cell);

        if ( data.size() > (uint16_t)(obj.viewMode-2) ) {
            color = data.at( obj.viewMode-2 );
        }

        if ( color > obj.viewThreshold ) {
            colorArr[0]=1.0;
            colorArr[1]=0.0;
            colorArr[2]=0.0;
            colorArr[3]=0.0;
        }
        else {
            colorArr[0] = DEFAULT_TOOTH_COL;
            colorArr[1] = DEFAULT_TOOTH_COL;
            colorArr[2] = DEFAULT_TOOTH_COL;
            colorArr[3] = DEFAULT_TOOTH_COL;
        }
    }

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, colorArr);
}






/**
 * @brief Immediate mode vertex data renderer for RENDER_HUMPPA.
 * @param obj       GLObject.
 * @param x         Rendering field width.
 * @param y         Rendering field height.
 */
void glcore::PaintGL_RENDER_HUMPPA(GLObject& obj, int x, int y)
{
    if (obj.mesh == NULL) return;

    GLdouble aspect = ((GLdouble)x/y);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho((-20.0*aspect+obj.viewPosX)*obj.zoomMultip,
            (20.0*aspect+obj.viewPosX)*obj.zoomMultip,
            (-20.0+obj.viewPosY)*obj.zoomMultip,
            (20.0+obj.viewPosY)*obj.zoomMultip, -2000.0, 2000.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Object panning.
    if (obj.mouse2Down) {
        obj.viewPosY = obj.viewPosY -
                        obj.deltaY/(PAN_SENSITIVITY*(float)y/SQUARE_WIN_SIZE);
        obj.viewPosX = obj.viewPosX +
                        obj.deltaX/(PAN_SENSITIVITY*(float)y/SQUARE_WIN_SIZE);
    }

    // Object rotation.
    if (obj.mouse1Down) {
        obj.rtriX += obj.deltaX;
        obj.rtriY -= obj.deltaY;
    }
    glRotated(180.0, 1.0, 0.0, 0.0);
    glRotated((double)obj.rtriY, 1.0, 0.0, 0.0);
    glRotated((double)obj.rtriX, 0.0, 0.0, 1.0);

    auto& polygons = obj.mesh->get_polygons();
    auto& vertices = obj.mesh->get_vertices();

    for ( auto& pol : polygons ) {
        // Note: RENDER_HUMPPA check needs to be inside loop, as it may change
        // while the loop is still rolling.
        if (obj.renderMode != RENDER_HUMPPA) break;

        // Calculate & assign the surface normal of the polygon.
        auto& v1 = vertices.at( pol.at(0) );
        auto& v2 = vertices.at( pol.at(1) );
        auto& v3 = vertices.at( pol.at(2) );
        GLdouble a[3] = { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
        GLdouble b[3] = { v3.x - v2.x, v3.y - v2.y, v3.z - v2.z };
        GLdouble n[3];
        get_surface_normal_(a,b,n);
        glNormal3dv(n);

        // Add an offset to the polygon to make the edges stick out better.
        glPolygonOffset(1.0, 1.0);
        glEnable(GL_POLYGON_OFFSET_FILL);

        // Draw polygons.
        glBegin(GL_POLYGON);
        for ( auto& i : pol ) {
            Vertex_color_RENDER_HUMPPA( i, obj );
            auto p = vertices.at( i );
            glVertex3d( p.x, p.y, p.z );
        }
        glEnd();
        glDisable(GL_POLYGON_OFFSET_FILL);

        // Draw polygon edges if required.
        if ( obj.polygonFill ) {
            GLfloat color[4] = {0.0, 0.0, 0.0, 0.0};  // Black edge color.
            glLineWidth(1.0);
            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);

            glBegin(GL_LINE_LOOP);
            for ( auto& i : pol ) {
                auto p = vertices.at( i );
                glVertex3d( p.x, p.y, p.z );
            }
            glEnd();

        }
    }
}



/**
 * @brief Updates GL view for 2D models.
 */
void glcore::PaintGL_2D(GLObject& obj, GLdouble aspect)
{
    GLfloat divXY;

    if (obj.pixelDataHeight==0 || obj.pixelDataWidth==0) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        return;
    }

    glBindTexture(GL_TEXTURE_2D, obj.texName);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, obj.pixelDataWidth, obj.pixelDataHeight,
                 0, GL_RGBA, GL_FLOAT, obj.img);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 10*0.1, 0.0, 10*0.1, -200.0, 200.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glBindTexture(GL_TEXTURE_2D, obj.texName);

    divXY = (obj.pixelDataWidth) / (float)(obj.pixelDataHeight) / aspect;
    glBegin(GL_QUADS);
        glTexCoord2f(0.0, 1.0);
        glVertex3f(0.5-divXY/2.0, 0.0, 0.0);
        glTexCoord2f(0.0, 0.0);
        glVertex3f(0.5-divXY/2.0, 1.0, 0.0);
        glTexCoord2f(1.0, 0.0);
        glVertex3f(0.5+divXY/2.0, 1.0, 0.0);
        glTexCoord2f(1.0, 1.0);
        glVertex3f(0.5+divXY/2.0, 0.0, 0.0);
        /* Gives MATLAB-style coordinates:
        glTexCoord2f(0.0, 1.0);
        glVertex3f(0.5+divXY/2.0, 0.0, 0.0);
        glTexCoord2f(0.0, 0.0);
        glVertex3f(0.5-divXY/2.0, 0.0, 0.0);
        glTexCoord2f(1.0, 0.0);
        glVertex3f(0.5-divXY/2.0, 1.0, 0.0);
        glTexCoord2f(1.0, 1.0);
        glVertex3f(0.5+divXY/2.0, 1.0, 0.0);
        */
    glEnd();
}
