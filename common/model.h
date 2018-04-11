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


namespace model {

struct view_mode {
    std::string name;                           // view mode plain text name
    std::vector<std::pair<int,int>> shapes;     // pairs [shape index, data index]
};

struct orientation {
    std::string name;                           // plain text name (e.g. "Buccal")
    float rotx;                                 // rotation along x axis
    float roty;                                 // rotation along y axis
};

}   // END namespace


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
    virtual Mesh& fill_mesh( Tooth& tooth ) { return tooth.get_mesh(); }

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
    void setShowMeshAccess( bool state )    { m_enableShowMesh = state; }
    bool getShowMeshAccess()                { return m_enableShowMesh; }

    // Set the status of 'Show mesh' in the GUI.
    void setShowMesh( bool state )          { m_showMesh = state; }
    bool getShowMesh()                      { return m_showMesh; }

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
    void setInterfaceXML( QString s )       { m_interfaceXML = s; }
    QString getInterfaceXML()               { return m_interfaceXML; }

    // Set model plain text name
    void setModelName( std::string s )      { m_modelName = s;
                                              parameters->setModelName(s); }
    std::string getModelName()              { return m_modelName; }

    // Returns model binary name for the current OS/platform, if applicable.
    QString getBinaryName()                 { return QString(modelBin.c_str()); }

    // Set parameter window background image file name.
    void setBackgroundImage( const QString& f )         { m_backgroundImage = f; }
    QString getBackgroundImage()                        { return m_backgroundImage; }

    // Add a predefined orientation for model viewing.
    void addOrientation( const model::orientation& o )  { m_orientations.push_back(o); }
    std::vector<model::orientation>& getOrientations()  { return m_orientations; }

    // Set default parameters file name.
    void setExampleParameters( const std::string& s )   { m_exampleParameters = s; }
    std::string getExampleParameters()                  { return m_exampleParameters; }

    // Sets binary information: Binary file names and input/output formats.
    void setBinaryInfo( const QString&, const QString&, const QString&,
                        const std::vector<QString>&,
                        const std::vector<QString>& );

    // Adds a view mode to the model.
    void addViewMode( const model::view_mode& mode )    { m_viewModes.push_back(mode); }
    const std::vector<model::view_mode>& getViewModes() { return m_viewModes; }



private:
    // Interface XML file.
    QString m_interfaceXML;

    // Parameter window background image file name.
    QString m_backgroundImage;

    // List of predefined orientations.
    std::vector<model::orientation> m_orientations;

    // Default parameters. XML key: <DefaultParameters>
    std::string m_exampleParameters;

    // List of view modes. XML key: <ViewMode>
    std::vector<model::view_mode> m_viewModes;

    // Model plain text name. XML key: <Name>
    std::string m_modelName;

    // Parsers executed at the user-defined export folder. No input arguments.
    // XML key: <ResultParser>
    std::vector<QString> m_resultParsers;

    // Control panel settings.
    bool m_enableShowMesh;         // true if 'Show mesh' accessible.
    bool m_showMesh;                // 'Show mesh' status, if applicable.



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
