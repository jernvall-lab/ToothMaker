#include <cmath>
#include <iostream>
#include <exception>
#include <tuple>
#include <QProcess>
#include <QTimer>
#include <QReadWriteLock>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

#include "model.h"
#include "colormap.h"



Model::Model() : m_exampleParameters(""),
                 m_modelName(""),
                 m_enableShowMesh(true),
                 m_showMesh(false)
{
    // protected members
    modelBin = "";
    renderMode = -1;
    systemTempPath = "";
    retval = 0;
    nIter = 0;
    currentIter = 0;
    stepSize = 1;

    parameters = new Parameters();
}



Model::~Model()
{
    if (systemTempPath.compare("")) {
        workDirCleanUp();
    }
}



/**
 * @brief Default RGBA image filler.
 *        Serves the purpose of fill_mesh() for 2D/image models.
 *
 * @param tooth     Tooth object.
 * @param img       Preallocated float array pointer.
 */
void Model::fill_image( Tooth *tooth, float *img )
{
    int viewMode = atof(parameters->getKey(PARKEY_VIEWMODE).c_str());
    double viewThresh = atof(parameters->getKey(PARKEY_VIEWTHRESH).c_str());
    int idx = floor( viewMode / 2.0 );

    try {
        auto data = tooth->get_cell_data().at( idx );

        for (uint32_t i=0; i<data.size(); i++) {
            std::string type;
            if (viewMode==0 || viewMode==2 || viewMode==5) {
                type = "heatmap";
            }
            if (viewMode==1 || viewMode==3 || viewMode==4 || viewMode==6) {
                type = "BW";
            }

            colormap::Color color;
            color.r = 0;
            color.g = 0;
            color.b = 0;
            colormap::Map_value( data.at(i), viewThresh, &color, type );

            img[4*i + 0] = ((color.r)/255.0);
            img[4*i + 1] = ((color.g)/255.0);
            img[4*i + 2] = ((color.b)/255.0);
            img[4*i + 3] = (1.0);
        }
    }
    catch (const std::out_of_range& oor)  {
        std::cerr << "Error: requesting invalid view mode " << idx << " ("
                  << oor.what() << ")." << std::endl;
    }
}



/**
 * @brief Sets new parameters for the model.
 *
 * Note: It is not safe to delete the old parameters and then use the parameters
 * copy-constructor, since the information about the order of the parameters is lost.
 * The parameter order is given in the model class file, and is crucial for e.g.
 * the tooth model.
 *
 * @param par
 */
void Model::setParameters(Parameters *par)
{
    if (par == nullptr)
        return;

    for (auto& p : par->getParameters())
        parameters->setParameterValue( p.name, p.value );

     for (size_t i=0; i<par->getKeywords()->size(); i++) {
        std::string key = par->getKeywords()->at(i);
        std::string value = par->getKey(key);
        parameters->setKey(key, value);
    }
    parameters->setID(par->getID());
}



/**
 * @brief Deletes the temporary folder and everything in it.
 *
 * Note: For performance reasons a better solution would be to remove the old
 * data files as soon as the model has finished.
 *
 */
void Model::workDirCleanUp()
{
    if (!PRESERVE_MODEL_TEMP) {
        QDir *qdir = new QDir(systemTempPath);
        qdir->removeRecursively();                  // Requires Qt 5.0 or later.
    }
}



/**
 * @brief Returns current model progress percentage.
 * @return      Percent complete.
 */
float Model::getProgress()
{
    if (nIter == 0)
        return 100.0;
    return (100.0*currentIter/nIter);
}



/**
 * @brief Copies model output files to user-specified data export folder.
 * @param ID                Model run ID (i.e., run folder name).
 * @param export_folder     Target path.
 * @return                  0 if success, else -1.
 */
int Model::exportData( const QString run_id, const QString export_folder )
{
    if (!export_folder.compare("")) {
        return -1;
    }

    QString run_path = systemTempPath + "/" + run_id + "/";
    QDir qdir(run_path);
    QFileInfoList files = qdir.entryInfoList( QDir::Files );

    // Removes the target files if they already exist before copying new ones.
    for (int i=0; i<files.size(); i++) {
        QString file = files.at(i).fileName();
        QString target = export_folder + "/" + file;
        QFile::remove(target);
        QFile::copy(file, target);
    }

    return 0;
}



/**
 * @brief Executes result parsers on model output at the data export folder.
 * @return      0 if success, else -1.
 */
int Model::runResultParsers( const QString export_folder )
{
    if (!export_folder.compare("")) {
        return -1;
    }

    QDir resources( QCoreApplication::applicationDirPath() );
    resources.cd(RESOURCES);
    resources.cd("bin");

    QString old_path = QDir::currentPath();
    QDir::setCurrent( export_folder );
    QProcess process;

    for (auto& parser : m_resultParsers) {
        QString cmd = "";
        QStringList blist = parser.split(".");
        if (blist.size() > 1 && blist.at(1) == "py") {
            cmd = "python ";
        }

        cmd = cmd + resources.path() + "/" + parser;
        process.start(cmd);
        if(!process.waitForFinished( PARSER_TIMEOUT )) {
            // TODO: Add checks for other errors, e.g., does the parser exist.
           qDebug() << "Error: Parser" << parser << "failed to finish in"
                    << PARSER_TIMEOUT << "msecs. SKipping.";
           continue;
        }

        std::cout << "Results parser: " << cmd.toStdString() << std::endl;
    }

    QDir::setCurrent( old_path );

    return 0;
}



/**
 * @brief Sets binary information: Binary file names and input/output formats.
 * @param bin           Binary name.
 * @param in_style      Input file format.
 * @param out_style     Output file format.
 * @param parsers       List of parsers to be applied to model output.
 */
void Model::setBinaryInfo( const QString& bin, const QString& in_style,
                           const QString& out_style,
                           const std::vector<QString>& output_parsers,
                           const std::vector<QString>& result_parsers )
{
    modelBin =  bin.toStdString();
    inputStyle = in_style;
    outputStyle = out_style;
    outputParsers = output_parsers;
    m_resultParsers = result_parsers;

    // Set the render mode according to the given output style.
    // Defaults to RENDER_MESH for PLY and Hexa output styles.
    renderMode = RENDER_MESH;           // 3D vertex data.

    if (outputStyle == "Matrix") {    // 2D pixel data.
        renderMode = RENDER_PIXEL;
    }
    if (outputStyle == "Humppa") {    // Legacy mode; don't use!
        renderMode = RENDER_HUMPPA;
    }
}
