/**
 *  @file colormap.cpp
 *  @brief Color maps for converting a value into an RGB color.
 *
 *  Usage:
 *  - Call Map_value() to convert a float into a Color object (see function notes below).
 *  - Supported modes:
 *    'BW' for white-black
 *    'RGB' for black-blue-green-red,
 *    'heatmap' for blue-green-red.
 */

#include "colormap.h"

using namespace colormap;


namespace {

/**
 * @brief Maps cell data values to heatmap along the given view threshold.
 *        Every value above 'viewThresh' is colored with the max. color
 *        (dark red).
 * @param val       Cell value.
 * @param col       Color object.
 * @param obj       GLObject.
 */
void map_color_heatmap_(double val, double viewThresh, Color *col)
{
    if (val<0.0) {
        val = 0.0;
    }
    if (val>viewThresh) {
        val = viewThresh;
    }
    double k = viewThresh/8.0;

    // The colors are divided into four sections, with five turning points:
    // 1) Dark blue, 2) full blue, 3) blue+green, 4) green+red, 5) dark red.
    if (val<k) {
        col->r = 0;
        col->g = 0;
        col->b = 255 - 255*(k-val)/(2*k);
    }
    else if (val>=k && val<3*k) {
        col->r = 0;
        col->b = 255;
        col->g = 255 - 255*(3*k-val)/(2*k);
    }
    else if (val>=3*k && val<5*k) {
        col->r = 255 - 255*(5*k-val)/(2*k);
        col->g = 255;
        col->b = 255 - 255*(val-3*k)/(2*k);
    }
    else if (val>=5*k && val<7*k){
        col->r = 255;
        col->g = 255 - 255*(val-5*k)/(2*k);
        col->b = 0;
    }
    else {
        col->r = 255 - 255*(val-7*k)/(2*k);
        col->g = 0;
        col->b = 0;
    }
}



/**
 * @brief Maps cell data values to RGB colors along the given view threshold.
 * @param val       Cell value.
 * @param col       Color object.
 * @param obj       GLObject.
 */
void map_color_RGB_(double val, double viewThresh, Color *col)
{
    col->r = 0;
    col->g = 0;
    col->b = 0;

    if (val >= 0.0) {
        if ((val*0.5/viewThresh*255) >= 3*255) {
            col->b = 0;
            col->g = 0;
            col->r = 255;
        }
        else if ((val*0.5/viewThresh*255) >= 2*255) {
            col->r = val*0.5/viewThresh*255 - 2*255;
            col->g = 255 - col->r;
            col->b = 0;
        }
        else if ((val*0.5/viewThresh*255) >= 255) {
            col->g = val*0.5/viewThresh*255 - 255;
            col->b = 255 - col->g;
            col->r = 0;
        }
        else {
            col->b = val*0.5/viewThresh*255;
            col->r = 0;
            col->g = 0;
        }
    }
}



/**
 * @brief Maps cell data values to B&W colors along the given view threshold.
 * @param val       Cell value.
 * @param col       Color object.
 * @param obj       GLObject.
 */
void map_color_BW_(double val, double viewThresh, Color *col)
{
    if (val<0.0) {
        val = 0.0;
    }
    if (val>viewThresh) {
        val = viewThresh;
    }

    col->r = 255 - 255*val/viewThresh;
    col->g = col->r;
    col->b = col->r;
}

}   // END namespace.



/**
 * @brief Maps a value into RGB color.
 * @param val           Value to be mapped.
 * @param viewThresh    View threshold set by the user.
 * @param col           Output color.
 * @param type          Map type, e.g., "heatmap", "RGB, "BW".
 * @return              0 if ok, else -1.
 */
int colormap::Map_value( double val, double viewThresh, Color* col,
                         std::string type )
{
    if (!type.compare("heatmap")) {
        map_color_heatmap_( val, viewThresh, col );
        return 0;
    }
    if (!type.compare("RGB")) {
        map_color_RGB_( val, viewThresh, col );
        return 0;
    }
    if (!type.compare("BW")) {
        map_color_BW_( val, viewThresh, col );
        return 0;
    }

    return -1;
}
