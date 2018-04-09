#pragma once

/**
 *  @class Model
 *  @brief Model object base class.
 *
 *  Provides an interface between MorphoMaker and models. All model objects
 *  must inherit this class.
 *
 *  A class inhitering this class must implement the following virtual methods:
 *
 *  start_model()   - Starts the model main loop.
 *  stop_model()    - Halts the model main loop.
 *  init_model()    - Initialises the model; called before start_model().
 *
 *  and at least one of
 *
 *  fill_mesh()     - Fills a mesh object for vertex models (RENDER_MESH).
 *  fill_image()    - Fills an image object for pixel models (RENDER_PIXEL).
 *
 *  For pixel models one may choose to call the predefined coloring methods
 *  from fill_image():
 *
 *  The rest of the methods are for internal use by MorphoMaker.
 */

#include <QThread>

#include "parameters.h"
#include "toothlife.h"
#include "tooth.h"
#include "morphomaker.h"

#define PARSER_TIMEOUT 30000    // Timeout in msecs for output & result parsers.


// Model view mode; plain-text name & ID.
typedef std::tuple<std::string, int> view_mode;


// Predefined 3D model orientation
struct orientation {
    std::string name;       // Plain text name (e.g. "Buccal")
    float rotx;             // Rotation along x axis
    float roty;             // Rotation along y axis
};



class Model : public QThread
{
Q_OBJECT

public:
    Model();
    virtual ~Model();

    //
    // Classes inheriting Model must implement the following three methods:
    //

    // Call to start the simulation; returns start time.
    virtual int start_model()           { return 0; }

    // Call to stop the simulation.
    virtual void stop_model()           {}

    // Initialize the model with parameters:
    // niter     - total number of iterations.
    // par       - model parameters object.
    // tlife     - toothLife object to store the model output.
    // ncores    - max. number of CPU cores to use.
    // Returns model start time.
    virtual int init_model( const QString& temp_path, const int max_cores, ToothLife& tlife,
                            const int num_iter, const int step_size, const int id )
    {
        stepSize = step_size;
        nIter = num_iter;
        systemTempPath = temp_path;
        (void)max_cores;
        (void)tlife;
        (void)id;

        return 0;
    }

    //
    // Also one of the following two must be implemented:
    //

    // Returns a modified version of the current mesh (e.g., colors added) for RENDER_MESH.
    virtual Mesh& fill_mesh( Tooth& tooth )         { return tooth.get_mesh(); }

    // Default RGBA image filler for RENDER_PIXEL.
    virtual void fill_image( Tooth *tooth, float *img );


    //
    // Hampu-Model interface methods (called from Hampu).
    //

    // Set model parameters.
    void setParameters(Parameters *);
    Parameters *getParameters()             { return parameters; }

    // Get model render mode (RENDER_MESH, RENDER_PIXEL or RENDER_HUMPPA)
    int getRenderMode()                     { return renderMode; }

    // Returns '0' if last model run was successfull, else '1' for error.
    int getReturnValue()                   { return retval; }

    // Set model step size for reading/storing results every i iterations.
    void setStepSize( int i )               { stepSize = i; }
    int getStepSize()                       { return stepSize; }

    // Enable/disable 'Show mesh' checkbox in the GUI.
    void setShowMeshAccess( bool state )    { _enable_show_mesh = state; }
    bool getShowMeshAccess()                { return _enable_show_mesh; }

    // Set the status of 'Show mesh' in the GUI.
    void setShowMesh( bool state )          { _show_mesh = state; }
    bool getShowMesh()                      { return _show_mesh; }

    // Deletes the temporary folder and everything in it.
    void workDirCleanUp();

    // Returns current model progress percentage.
    float getProgress();

    // Copies model output files to user-specified data export folder.
    int exportData( const QString, const QString );

    // Executes result parsers on model output at the data export folder.
    int runResultParsers( const QString );


    //
    // Interface initialization methods; called from Hampu and ReadXML.
    //

    // Set interface XML file name.
    void setInterfaceXML( QString s )       { _interface_xml = s; }
    QString getInterfaceXML()               { return _interface_xml; }

    // Set model plain text name
    void setModelName( std::string s )      { _model_name = s;
                                              parameters->setModelName(s); }
    std::string getModelName()              { return _model_name; }

    // Returns model binary name for the current OS/platform, if applicable.
    QString getBinaryName()                 { return QString(modelBin.c_str()); }

    // Set parameter window background image file name.
    void setBackgroundImage( const QString& f )         { _background_image = f; }
    QString getBackgroundImage()                        { return _background_image; }

    // Add a predefined orientation for model viewing.
    void addOrientation( const orientation& o )         { _orientations.push_back(o); }
    std::vector<orientation>& getOrientations()         { return _orientations; }

    // Set default parameters file name.
    void setExampleParameters( const std::string& s )   { _example_parameters = s; }
    std::string getExampleParameters()                  { return _example_parameters; }

    // Sets binary information: Binary file names and input/output formats.
    void setBinaryInfo( const QString&, const QString&, const QString&,
                        const std::vector<QString>&,
                        const std::vector<QString>& );

    // Adds a view mode to the model.
    void addViewMode( std::pair<std::string,std::string>& mode );
    std::vector<std::string> *getViewModes()            { return &_view_modes; }



private:
    // Interface XML file.
    QString _interface_xml;

    // Parameter window background image file name.
    QString _background_image;

    // List of predefined orientations.
    std::vector<orientation> _orientations;

    // Default parameters. XML key: <DefaultParameters>
    std::string _example_parameters;

    // List of view modes. XML key: <ViewMode>
    std::vector<std::string> _view_modes;

    // Model plain text name. XML key: <Name>
    std::string _model_name;

    // Parsers executed at the user-defined export folder. No input arguments.
    // XML key: <ResultParser>
    std::vector<QString> _result_parsers;

    // Control panel settings.
    bool _enable_show_mesh;         // true if 'Show mesh' accessible.
    bool _show_mesh;                // 'Show mesh' status, if applicable.



protected:
    // Parsers applied to all output files at the original output location.
    // File names as input arguments. XML key: <OutputParser>
    std::vector<QString> outputParsers;

    Parameters *parameters;             // Current model parameters.

    int nIter;                          // Max. number of iterations.
    int currentIter;                    // Current model iteration.
    int stepSize;                       // Step size for storing results.

    int renderMode;                     // RENDER_MESH or RENDER_PIXEL.
    QString systemTempPath;             // Full temporary files path.
    int retval;                         // Return value of the last model run.
    std::string modelBin;               // Model binary name.

    std::string domainTemplate;         // Domain shape template file.
    std::string sourceTemplate;         // Morphogen source template file.
    std::string prepatTemplate;         // Prepattern template file.

    QString inputStyle;                 // Input file style: 'MorphoMaker'
    QString outputStyle;                // Output style: 'Matrix' or 'PLY'



signals:
    void msgStatusBar(std::string);     // message to be shown at GUI status bar
    void finished();                    // emitted when the model has exited

};
