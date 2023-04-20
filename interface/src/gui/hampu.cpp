/**
 *  @class Hampu
 *  @brief Main GUI application.
 *
 *  Model progress monitoring:
 *  - A running model is given a pointer to an object (ToothLife) owned by Hampu where
 *    it stores the model output at its own pace.
 *  - Hampu queries the running model at fixed intervals for progress, checks for new
 *    content in the data object and updates the model visuals as needed.
 *
 *  TODO: Calling maxima and baseA computations should be moved somewhere in model
 *  specific files.
 */

#include <algorithm>
#include <numeric>
#include <ctime>

#include "gui/hampu.h"
#include "utils/writeparameters.h"
#include "utils/readparameters.h"
#include "utils/writedata.h"
#include "utils/readxml.h"
#include "model.h"
#include "misc/loader.h"



/**
 * @brief Hampu desctructor.
 */
Hampu::~Hampu()
{
    // Stop all models.
    for (uint32_t i=0; i<models.size(); i++) {
        models.at(i)->stop_model();
        delete models.at(i);
    }

    // Delete the temp. folder. It should be empty by now.
    QDir qdir;
    if (qdir.rmdir((QString)tempPathMorpho.c_str())) {
        if (DEBUG_MODE) fprintf(stderr, "Removed '%s'\n", tempPathMorpho.c_str());
    }
    else {
        if (DEBUG_MODE) fprintf(stderr, "Couldn't remove' '%s'\n", tempPathMorpho.c_str());
    }
}



/**
 * @brief Initialises the GUI.
 * @return      0 if success, else -1.
 */
int Hampu::init_GUI()
{
    if (DEBUG_MODE) fprintf(stderr, "*** %s:%s() START\n", __FILE__, __FUNCTION__);

    currentModel = -1;
    currentHistory = 0;
    viewIntStep = 0;
    scanning = 0;
    screenshotCounter = 0;
    runCounter = 0;
    timeLimit = -1;     // -1 = no time limit
    parwidget = NULL;

    // Load all available models, creater parameter windows.
    morphomaker::Load_models(models);
    for (uint32_t i=0; i<models.size(); i++) {
        Model* model = models.at(i);
        ParameterWindow* p = new ParameterWindow();
        morphomaker::Read_GUI_definitions( model->getInterfaceXML(), *model, *p );
        p->setModel(model);
        parameterWindows.push_back(p);
    }

    // Create control panel, add the models to models menu.
    controlPanel = new ControlPanel(this, &models);
    controlPanel->setSliderMinMax(0, 1);
    followDevelopment = FOLLOW_DEFAULT;

    // Create rendering window.
    QGLFormat glformat;
    glformat.setVersion( 2, 1 );
    glformat.setProfile( QGLFormat::CompatibilityProfile );
    // glformat.setProfile( QGLFormat::CoreProfile );
    glformat.setDoubleBuffer(true);     // Double-buffering. Must be false for PPC Macs.
    glformat.setSwapInterval(0);        // Vertical sync. Must be disabled to prevent lag.
    glformat.setDirectRendering(true);  // Hardware rendering.

    glwidget = new GLWidget(glformat, this, 0);
    if (!glwidget->context()->isValid()) {
        char msg[] = {"Cannot create OpenGL context. System does not provide the required graphics capabilities."};
        QMessageBox::critical(this, "Fatal error", msg);
        qDebug() << "Fatal error:" << msg;
        return -1;
    }
    glwidget->makeCurrent();

    if (DEBUG_MODE) {
        std::cout << "GLWidget creation: " << glwidget->context()->isValid() << std::endl;
        // std::cout << "Current OpenGL profile: " << glformat.profile() << std::endl;
        std::cout << "OpenGL version: " << glformat.majorVersion() << "."
                  << glformat.minorVersion() << std::endl;
        std::cout << "Profile: " << glformat.profile() << std::endl;
    }

    // Create parameter scanning window. Hidden by default.
    scanWindow = new ScanWindow();

    // Add GUI components to the main window.
    QWidget *mainWidget = new QWidget();
    setCentralWidget(mainWidget);
    setMinimumSize(MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT);
    mainLayout = new QGridLayout();
    mainLayout->addWidget(glwidget, 0, 0);
    mainLayout->setVerticalSpacing(0);
    mainLayout->addWidget(controlPanel, 1, 0, 2, 0);
    centralWidget()->setLayout(mainLayout);

    // Checking if required GL extensions available
    // NOTE: QOpenGLContext requires >= Qt 5.0. As of MorphoMaker 0.5.3 this
    // piece breaks the compatibility with Qt 4.8.
    QOpenGLContext* context = glwidget->context()->contextHandle();
    if ( context == NULL || !context->isValid() ) {
        qDebug() << "Fatal error: Cannot create OpenGL context.";
        return -1;
    }

    if (DEBUG_MODE) {
        QList<QByteArray> exts = context->extensions().toList();
        std::cerr << "QOpenGLContext creation: "
                  << glwidget->context()->contextHandle()->isValid() << std::endl;
        std::cout << "Number of extensions retrieved: " << exts.size() << std::endl;
        for (int i=0; i<exts.size(); ++i) {
            QString str(exts[i].constData());
            std::cout << str.toStdString() << std::endl;
        }
    }

    // Testing for the presence of some crucial extensions.
    // NOTE: This check fails if core profile requested in OS X, as the
    // extensions are then part of the core and not listed as extensions!
    std::vector<std::string> extensions = {"GL_EXT_framebuffer_multisample",
                                           "GL_EXT_framebuffer_blit"};
    std::vector<std::string> ext_missing;
    for (auto ext : extensions) {
        if (!context->hasExtension( QByteArray(ext.c_str()) )) {
            ext_missing.push_back(ext);
        }
    }
    if (ext_missing.size() > 0) {
        std::string err = "System graphics capabilities are not sufficient to run this program.\n\n"
                          "Missing required extension(s): ";
        for (auto ext : ext_missing) {
            err.append("\n");
            err.append(ext);
        }
        qDebug() << "Fatal error:" << err.c_str();
        QMessageBox::critical(this, "Fatal error", err.c_str());
        return -1;
    }

    // Create menu bar items.
    setMenuBar_();

    // Signals for communication between Hampu and the rest of the program.
    setSignals_();

    // Setting the default model with which the program starts.
    setModelSettings(DEFAULT_MODEL, 1);

    // Create a temporary folder for running the models.
    int pid = (int)QCoreApplication::applicationPid();
    QDir qdir;
    QString tempPath = qdir.tempPath();
    std::stringstream str;
    str << tempPath.toStdString() << "/" << PROGRAM_NAME << "_" << pid;
    str >> tempPathMorpho;
    if (!qdir.exists(QString(tempPathMorpho.c_str()))) {
        qdir.mkdir(QString(tempPathMorpho.c_str()));
    }
    std::cout << "Temp. folder: " << tempPathMorpho << std::endl;

    // Model progress polling.
    progressTimer = new QTimer(this);
    progressTimer->setInterval(UPDATE_INTERVAL);
    connect(progressTimer, SIGNAL(timeout()), this, SLOT(updateProgress()));

    // Enable drag & drop for parameters.
    setAcceptDrops(true);

    // Print program version to the status bar.
    char msg[1024];
    sprintf(msg, "%s v%s", PROGRAM_NAME, MMAKER_VERSION);
    writeStatusBar(msg);

    if (DEBUG_MODE) fprintf(stderr, "*** %s:%s() END\n\n", __FILE__, __FUNCTION__);

    return 0;
}



//
// *** PRIVATE SLOTS ***
//



/**
 * @brief Sets current view mode.
 * - Triggered by ControlPanel.
 * - Notifies both the model and the renderer.
 *
 * @param mode      View mode.
 */
void Hampu::Panel_ViewMode(int mode)
{
    if (DEBUG_MODE) fprintf(stderr, "%s:%s(%d)\n", __FILE__, __FUNCTION__, mode);

    char tmp[256];
    sprintf(tmp, "%d", mode);

    if (toothHistory.size()>currentHistory) {
        ToothLife *t = toothHistory.at(currentHistory);
        t->getParameters()->setKey(PARKEY_VIEWMODE, tmp);
    }
    models.at(currentModel)->getParameters()->setKey(PARKEY_VIEWMODE, tmp);

    Tooth *tooth=NULL;
    if (toothHistory.size()>currentHistory) {
        tooth = toothHistory.at(currentHistory)->getTooth(viewIntStep);
    }
    glwidget->setViewMode(mode, tooth, models.at(currentModel));
}



/**
 * @brief Sets current view threshold.
 * - Triggered by ControlPanel.
 * - Notifies both the model and the renderer.
 *
 * @param val       Threshold value.
 */
void Hampu::Panel_ViewThreshold(double val)
{
    if (DEBUG_MODE) fprintf(stderr, "%s:%s(%lf)\n", __FILE__, __FUNCTION__, val);

    char tmp[256];
    sprintf(tmp, "%lf", val);

    if (toothHistory.size()>currentHistory) {
        ToothLife *t = toothHistory.at(currentHistory);
        t->getParameters()->setKey(PARKEY_VIEWTHRESH, tmp);
    }
    models.at(currentModel)->getParameters()->setKey(PARKEY_VIEWTHRESH, tmp);

    Tooth *tooth=NULL;
    if (toothHistory.size()>currentHistory) {
        tooth = toothHistory.at(currentHistory)->getTooth(viewIntStep);
    }
    glwidget->setViewThreshold(val, tooth, models.at(currentModel));
}



/**
 * @brief Sets current view orientation.
 *
 * - Triggered by ControlPanel. Note that the index sent by ControlPanel is
 *   i+1, and 0 is reserved for free rotation.
 *
 * @param i     View orientation index
 */
void Hampu::Panel_Orientation(int i)
{
    auto orient = models.at(currentModel)->getOrientations();
    if (i < 1 || (int)orient.size() <= i-1) {
        return;
    }

    glwidget->setViewOrientation( orient.at(i-1).rotx, orient.at(i-1).roty );
}



/**
 * @brief Toggles cell connections/grid.
 * - Triggered by ControlPanel.
 * - Notifies the renderer only.
 *
 * @param mode      1=true, 0=false.
 */
void Hampu::Panel_CellConnections(int mode)
{
    models.at(currentModel)->setShowMesh(mode);
    glwidget->showMesh(mode);
}



/** Changes the current model.
 * - Triggered by ControlPanel.
 *
 *  @param i        Model widget index in 'parameterWindows' vector.
 */
void Hampu::Panel_Model(int i)
{
    if (DEBUG_MODE) fprintf(stderr, "%s:%s(%d)\n", __FILE__, __FUNCTION__, i);
    // NOTE: A new (empty) history entry must be created before changing the model.
    // Otherwise some settings from the new entry will be incorrectly transferred to
    // the previous
    currentHistory = controlPanel->addHistory(0);
    controlPanel->setSliderMinMax(0, 1);
    setModelSettings(i,1);

    char msg[256];
    Parameters* parameters = models.at(currentModel)->getParameters();
    sprintf(msg, "Model: %s", parameters->getKey(PARKEY_MODEL).c_str());
    writeStatusBar(msg);
}



/**
 * @brief Changes current history entry.
 * - Triggered by ControlPanel when changing history or model.
 *
 * @param mode      Current history index.
 */
void Hampu::Panel_History(int mode)
{
    if (DEBUG_MODE) fprintf(stderr, "%s(): %d\n", __FUNCTION__, mode);

    currentHistory = mode;
    if (currentHistory >= toothHistory.size()) {
        return;
    }

    ToothLife* toothLife = toothHistory.at(currentHistory);
    Model* model = models.at( toothLife->getCurrentModel() );

    model->setParameters( toothLife->getParameters() );
    int niter = atoi( model->getParameters()->getKey(PARKEY_ITER).c_str() );
    int stepsize = model->getStepSize();
    controlPanel->setSliderMinMax(0, (niter/stepsize));

    if ( followDevelopment || toothLife->getLifeSize()-1 < viewIntStep ) {
        viewIntStep = toothLife->getLifeSize()-1;
        if ( viewIntStep < 0 ) {
            viewIntStep = 0;
        }
    }
    controlPanel->setSliderValue(viewIntStep);

    setModelSettings( toothLife->getCurrentModel(), 0 );
    updateCurrentStepView_(STATUSBAR_VERBOSE);
}



/**
 * @brief Reads parameters file imported by user.
 * - Triggered by user selecting 'Import'.
 *
 * @param file      File name.
 */
void Hampu::Panel_Import(std::string file)
{
    // Find the model matching the one given in the parameters file.
    // NOTE: Running Import_parameters() on an empty Parameters object only
    // reads the keys words and values! The actual parameters are read later
    // once we know the target model.
    Parameters par;
    morphomaker::Import_parameters(file, &par);
    std::string model = par.getKey(PARKEY_MODEL);
    if (!model.compare("")) {
        char msg[256];
        sprintf(msg, "Error: Can't find tag 'model' in the parameter file.");
        writeStatusBar(msg);
        return;
    }
    int modelFound = -1;
    for (uint32_t i=0; i<parameterWindows.size(); i++) {
        if (!model.compare( models.at(i)->getModelName()) ) {
            modelFound = i;
            break;
        }
    }
    if (modelFound < 0) {
        char msg[256];
        sprintf(msg, "Error: Unknown model name '%s' in the parameter file.",
                     model.c_str());
        writeStatusBar(msg);
        return;
    }

    // Now read the model parameters:
    morphomaker::Import_parameters( file,
                                    models.at(currentModel)->getParameters() );
    currentHistory = controlPanel->addHistory(0);
    controlPanel->setSliderMinMax(0, 1);
    setModelSettings(modelFound, 0);

    std::stringstream ss;
    ss << "Read: '" << file << "'";
    writeStatusBar(ss.str());
}



/**
 * @brief Exports parameters.
 * - Triggered by user selecting 'Export'.
 *
 * @param file      File name.
 */
void Hampu::Panel_Export(std::string file)
{
    morphomaker::Export_parameters( models.at(currentModel)->getParameters(),
                                    file, PROGRAM_NAME );
}



/**
 * @brief Requests updating the model view according to the current development
 *        stage.
 *
 * - Called from ControlPanel when user moves the development slider.
 *
 * @param step      Development stage to view.
 */
void Hampu::Panel_Development(int step)
{    
    if ( viewIntStep == step || currentHistory>=toothHistory.size() ) {
        return;
    }

    ToothLife *toothLife = toothHistory.at(currentHistory);

    if ( step < toothLife->getLifeSize() ) {
        viewIntStep = step;
    }
    if ( step >= toothLife->getLifeSize() ) {
        // This is to prevent the slider from going beyond the position corresponding
        // to the last entry in toothLife. Not the nicest solution, but works.
        viewIntStep = toothLife->getLifeSize()-1;
        controlPanel->setSliderValue( toothLife->getLifeSize()-1 );
    }

    updateCurrentStepView_(STATUSBAR_VERBOSE);
    QApplication::processEvents();
}



/**
 * @brief Sets current number of iterations.
 * - Triggered by ControlPanel.
 * - Effect takes place at the next run.
 *
 * @param val       Number of iterations.
 */
void Hampu::Panel_Iterations(int val) {
    char tmp[256];
    sprintf(tmp, "%d", val);
    models.at(currentModel)->getParameters()->setKey(PARKEY_ITER, tmp);
}



/**
 * @brief Launches the currently selected model.
 * - Called either ControlPanel when user hits 'Run', or from Hampu when
 *   scanning parameters.
 *
 * @param nIter      Number of iterations.
 */
void Hampu::Panel_Run(int nIter)
{
    Model* model = models.at(currentModel);
    if (model == NULL) {
        return;
    }
    glwidget->setRenderMode( model->getRenderMode() );
    glwidget->clearScreen();

    // Disable the model menu while running model:
    controlPanel->enableModelList(0);

    // NOTE: Model/run ID is set as time(NULL), meaning that if two consequtive
    // model runs occur within one second they are assigned the same ID, leading
    // to undefined behaviour!
    int run_id = time(NULL);
    toothLifeWork = new ToothLife( currentModel, run_id );
    toothLifeWork->setParameters( model->getParameters() );

    // Clean the history if needed, push the current work into history.
    while (toothHistory.size() > getMaxHistorySize_()) {
        delete toothHistory.at(0);
        toothHistory.erase(toothHistory.begin());
        controlPanel->removeHistory(0);
    }
    toothHistory.push_back(toothLifeWork);
    currentHistory = controlPanel->addHistory(1);

    int stepsize = model->getStepSize();
    int rv = model->init_model( QString(tempPathMorpho.c_str()), 2,
                                *toothLifeWork, nIter, stepsize, run_id, timeLimit );
    if (rv<0) {
        // Model initialization failed.
        ToothLife *toothLife = toothHistory.at(toothHistory.size()-1);
        controlPanel->endHistory( (toothLife->getLifeSize()-1)*stepsize );
        controlPanel->enableModelList(1);
        controlPanel->updateRunStatus("Run");
        return;
    }

    progressTimer->start();
    timeStart = model->start_model();
    runCounter++;
    controlPanel->setSliderMinMax(0, (nIter/stepsize));
}



/**
 *  @brief Called when the model is killed by the user.
 */
void Hampu::Panel_Stop()
{
    for (uint32_t i=0; i<models.size(); i++) {
        models.at(i)->stop_model();
    }
}




/**
 * @brief Sets development auto-follow.
 * - Called from ControlPanel when the user toggles 'Follow development'.
 *
 * @param mode      1=true, 0=false.
 */
void Hampu::Panel_FollowDevelopment(int mode)
{
    if (DEBUG_MODE) fprintf(stderr, "Follow development: %d\n", mode);

    if (mode) {
        followDevelopment = 1;
        if (toothHistory.size() == 0) return;

        ToothLife *toothLife=toothHistory.at(currentHistory);
        viewIntStep = toothLife->getLifeSize()-1;
        if ( viewIntStep < 0 ) {
            viewIntStep = 0;
        }
        controlPanel->setSliderValue(viewIntStep);
        updateCurrentStepView_(STATUSBAR_VERBOSE);
    }
    else {
        followDevelopment = 0;
    }
}



/**
 * @brief Exits MorphoMaker.
 * - Called when user select File->Exit.
 */
void Hampu::File_Exit()
{
    QApplication::exit();
}



/**
 * @brief File dialog for exporting model output files.
 */
void Hampu::Tools_ExportObjects()
{
    if (DEBUG_MODE) fprintf(stderr, "%s():", __FUNCTION__);  // DEBUG

    QString folder = QFileDialog::getExistingDirectory(this,
                                                       "Select data storage folder",
                                                       QDir::homePath());
    if (!folder.isEmpty() && toothHistory.size()>0) {
        exportModelData(-1, EXPORT_DATA, folder);
        char msg[256];
        sprintf(msg, "Data export complete.");
        writeStatusBar(msg);
    }
}



/**
 * @brief Folder dialog for exporting screenshots.
 */
void Hampu::Tools_ExportImages()
{
    QString folder = QFileDialog::getExistingDirectory(this,
                                                       "Select image storage folder",
                                                       QDir::homePath());

    if (!folder.isEmpty() && toothHistory.size()>0) {
        int i = exportModelData(-1, EXPORT_SCREENSHOTS, folder);
        char msg[256];
        sprintf(msg, "Exported %d steps to %s.", i, folder.toStdString().c_str());
        writeStatusBar(msg);
    }
}



/**
 * @brief Manages Options->Scan parameters window.
 */
void Hampu::Tools_ScanParameters()
{
    scanWindow->show();
}




/**
 * @brief Cleans history, frees all allocated memory.
 */
void Hampu::Options_PurgeHistory()
{
    while (toothHistory.size() > 1) {
        delete toothHistory.at(0);
        toothHistory.erase(toothHistory.begin());
        controlPanel->removeHistory(0);
    }
}



/**
 * @brief Manages Options->Preferences window.
 */
/*
void Hampu::Options_Preferences()
{
    preferences->show();
    preferences->raise();
    preferences->activateWindow();
}
*/



/**
 * @brief Sets the interface into parameter scanning mode.
 * - Called when the user starts the scanning in the interface.
 * - Calls scanParameters().
 */
void Hampu::startParameterScan()
{
    if (DEBUG_MODE) fprintf(stderr, "%s:%s()\n", __FILE__, __FUNCTION__);

    // Storing the base parameters before varying anything.
    baseParameters = new Parameters(models.at(currentModel)->getParameters());
    scanning = 1;

    QString resfolder = scanWindow->getResultsFolder();
    std::string parfile = resfolder.toStdString() + "/parameters_base.txt";
    morphomaker::Export_parameters(baseParameters, parfile, PROGRAM_NAME);


    std::string joblist = resfolder.toStdString() + "/" + SCAN_LIST;
    scanList = scanWindow->getScanList();
    scanList->resetScanQueue();
    scanList->setBaseParameters(baseParameters);
    scanList->populateScanQueue(joblist, scanWindow->calcPermutations());

    controlPanel->enableRunButton(0);
    controlPanel->enableHistory(0);
    scanParameters_();
}



/**
 * @brief Stop parameter scanning.
 * - Called when user requests stoppping parameter scanning.
 */
void Hampu::stopParameterScan()
{
    scanning = 0;
    timeLimit = -1;
    models.at(currentModel)->setParameters(baseParameters);
    // TODO: Do not call signals directly from the code.
    Panel_Stop();
    controlPanel->enableRunButton(1);
    controlPanel->enableHistory(1);
}



/**
 * @brief Resets view orientation menu.
 * - Called from GLWidget when user moves/rotates the object in model view.
 *
 * @param val       1=true, 0=false.
 */
void Hampu::resetOrientation(int val)
{
    controlPanel->resetOrientation(val);
}



/**
 * @brief Updates the GUI with proper model settings.
 * - Optionally loads the default/example parameters.
 *
 * @param id                Model ID.
 * @param useDefault        1=load default parameters, 0=don't (use existing).
 */
void Hampu::setModelSettings(int id, int useDefault)
{
    if (DEBUG_MODE) fprintf(stderr, "%s:%s()\n", __FILE__, __FUNCTION__);

    if (id<0 || (uint)id>=models.size()) {
        return;
    }

    // Only reset the parameter scanner if the model truly changes.
    // For example, if the history is browsed and the model does not change,
    // retains the previously entered scanning ranges etc.
    if (currentModel != id) {
        scanWindow->resetScanList();
        scanWindow->setParameters( models.at(id)->getParameters() );
    }
    currentModel = id;
    Model* model = models.at(currentModel);

    if (AUTO_IMPORT_EXAMPLES && useDefault) {
        importExampleParameters_();
    }

    // Set the rendering mode for the model.
    glwidget->setRenderMode( model->getRenderMode() );
    setVisualData();

    // Update parameter window.
    if (parwidget != NULL) {
        parwidget->hide();
        mainLayout->removeWidget(parwidget);
    }
    parwidget = parameterWindows.at(currentModel);
    mainLayout->addWidget(parwidget, 0, 1, Qt::AlignVCenter);
    parwidget->show();
    parwidget->updateButtonValues();

    // Set control panel:
    auto orientations = model->getOrientations();
    if (orientations.size() == 0) {
        glwidget->setRotations(false);
    }
    else {
        glwidget->setRotations(true);
    }
    controlPanel->setOrientations( orientations );
    controlPanel->showCellConnections( model->getShowMeshAccess(),
                                       model->getShowMesh() );

    Parameters* par = model->getParameters();
    int viewmode = atoi( par->getKey(PARKEY_VIEWMODE).c_str() );
    controlPanel->setViewModeBox( model->getViewModes(), viewmode );
    controlPanel->setModelIndex(currentModel);

    int niter = atoi( par->getKey(PARKEY_ITER).c_str() );
    controlPanel->setnIter(niter);
    double viewthresh = atof( par->getKey(PARKEY_VIEWTHRESH).c_str() );
    controlPanel->setViewThreshold(viewthresh);
    viewmode = atoi( par->getKey(PARKEY_VIEWMODE).c_str() );
    controlPanel->setViewMode(viewmode);
}



/**
 * @brief Writes model data (objects, images,...) into files.
 * @param step          Export step, or -1 for all steps.
 * @param datatype      What to export; EXPORT_SCREENSHOTS, EXPORT_DATA.
 * @param export_folder Folder for storing the data.
 * @return              Number of exported steps.
 */
int Hampu::exportModelData( int step, int datatype, QString export_folder )
{
    if (export_folder == "") {
        return 0;
    }

    Model* model = models.at( currentModel );
    ToothLife* toothLife = toothHistory.at(currentHistory);
    Parameters* parameters = model->getParameters();
    QString par_id = QString::fromStdString( parameters->getID() );
    if (par_id == "" || !scanning) {
        par_id = QString::number(runCounter);
    }
    QString run_id = QString::number( toothLife->getID() );
    int counter = 0;

    // Take screenshots of the model view, loop over steps if all steps requested.
    if (datatype & EXPORT_SCREENSHOTS) {
        std::vector<int> steps = {step};
        if (step == -1) {
            steps.resize( toothLife->getLifeSize() );
            std::iota( steps.begin(), steps.end(), 0 );
        }

        for (auto& i : steps) {
            viewIntStep = i;
            updateCurrentStepView_(STATUSBAR_QUIET);
            controlPanel->setSliderValue(viewIntStep);

            // This keeps the interface alive while exporting images.
            // TODO: Lock the main interface features during the export,
            // lest the user mess things up.
            QApplication::processEvents();

            QString folder = export_folder + "/" + SSHOT_SAVE_DIR;
            QDir qdir;
            if (!qdir.exists(folder)) {
                qdir.mkdir(folder);
            }

            std::vector<model::orientation> orientations;
            if (scanWindow->storeOrientations()) {
                orientations = model->getOrientations();
            }

            // Take screenshot at current orientation if no orientations given:
            if (orientations.size() == 0) {
                QImage img = glwidget->screenshotGL();
                char iter[11];
                sprintf( iter, "%.10d", viewIntStep*model->getStepSize() );
                QString target = folder + "/" + PROGRAM_NAME + "_" + par_id
                                 + "_" + QString(iter) + ".png";
                img.save(target);
            }

            // Take screenshots in predefined orientations:
            for (auto orient : orientations) {
                glwidget->setViewOrientation( orient.rotx, orient.roty );
                QImage img = glwidget->screenshotGL();
                char iter[11];
                sprintf( iter, "%.10d", viewIntStep*model->getStepSize() );
                QString target = folder + "/" + PROGRAM_NAME + "_" + par_id
                                 + "_" + QString::fromStdString(orient.name)
                                 + "_" + QString(iter) + ".png";
                img.save(target);
            }

            char msg[256];
            uint32_t steps = toothLife->getLifeSize();
            sprintf(msg, "Taking screenshot %u/%u.", counter, steps);
            writeStatusBar(msg);
            counter++;
        }
    }

    // All data files are always exported, regardless of whether a single or all
    // steps are requested.
    if (datatype & EXPORT_DATA) {
        char msg[256];
        sprintf(msg, "Exporting model data...");
        writeStatusBar(msg);

        QString folder = export_folder + "/" + DATA_SAVE_DIR;
        QDir qdir;
        if (!qdir.exists(folder)) {
            qdir.mkdir(folder);
        }

        if (scanning) {
            // For parameter scanning create additional subfolder to distiguish
            // between different runs by parameter ID.
            QDir qdir(folder);
            qdir.mkdir(par_id);
            folder = folder + "/" + par_id;
        }

        // Copy simulation output files to the target folder.
        model->exportData( run_id, folder );

        if (model->getRenderMode() == RENDER_HUMPPA) {
            // TODO: Model specific stuff like the following belongs to
            // result parsers, not here.
            Tooth* tooth = toothLife->getTooth( viewIntStep );

            QString file = export_folder + "/local_maxima.txt";
            morphomaker::Export_local_maxima( *tooth, file.toStdString(),
                                              par_id.toStdString() );
            file = export_folder + "/cuspA_baseline.txt";
            morphomaker::Export_main_cusp_baseline( *tooth, file.toStdString(),
                                                    par_id.toStdString() );
        }

        // Apply result parsers on the output files at the export folder.
        model->runResultParsers( export_folder );
    }

    return counter;
}



/**
 * @brief Sends visual data to the renderer.
 * -Does not check the validity of the requested step.
 */
void Hampu::setVisualData()
{
    if (toothHistory.size() < currentHistory+1) {
        glwidget->setVisualData(NULL, 0, 0);
        return;
    }

    ToothLife *toothLife = toothHistory.at(currentHistory);
    glwidget->clearScreen();
    glwidget->setVisualData(toothLife, viewIntStep, models.at(currentModel));
}



/**
 * @brief Writes stuff to status bar, or to the command line in batch mode.
 * @param msg   Message
 */
void Hampu::writeStatusBar(std::string msg="")
{
    if (msg.empty()) return;
    statusBar()->showMessage(msg.c_str());
}



/**
 * @brief Takes screenshot of the current model view.
 */
void Hampu::screenshotWidget()
{
    QImage img = glwidget->screenshotGL();

    char fname[2048], msg[4096];
    sprintf( fname, "%s/Desktop/%s_proj_%d.png",
             (QDir::homePath()).toStdString().c_str(), PROGRAM_NAME,
             screenshotCounter );
    img.save( QString(fname) );
    screenshotCounter++;

    sprintf(msg, "Screenshot saved: %s", fname);
    writeStatusBar(msg);
}



/**
 * @brief Updates model view window, status bar & development slider position.
 * - Called by a QTimer set in the Hampu constructor.
 */
void Hampu::updateProgress()
{
    char msg[256];
    int model_idx = toothLifeWork->getCurrentModel();

    // Update the development position only if viewing the currently running
    // model and 'Follows development' is checked.
    if (currentHistory==toothHistory.size()-1 && followDevelopment) {
        viewIntStep = toothLifeWork->getLifeSize()-1;
        if ( viewIntStep < 0 ) {
            viewIntStep = 0;
        }

        int stepsize = models.at(model_idx)->getStepSize();
        if (stepsize <= 0) {
            sprintf(msg, "Error: Model step size must be a positive integer (current step size %d).",
                    stepsize);
            writeStatusBar(msg);
            return;
        }

        int n_iter = controlPanel->getnIter();
        // Number of elements in toothLifeWork that is represented by one slider tick.
        double ticks = std::max( double(n_iter/stepsize)/DEV_SLIDER_WIDTH, 1.0 );

        // Call the slider position update only when the position change is more than
        // one tick. QSlider->setValue() is terribly inefficient, hence must avoid calling
        // it unnecessarily.
        if ( (int)(controlPanel->getSliderValue()/ticks) != (int)(viewIntStep/ticks) ) {
            controlPanel->setSliderValue(viewIntStep);
        }

        setVisualData();
    }

    float prog = models.at(model_idx)->getProgress();
    if (!scanning) {
        sprintf(msg, "Running... %.1f%% complete.", prog);
        writeStatusBar(msg);
    }
    else {
        int n = scanList->getScanQueueSize();
        int i = scanList->getCurrentScanItem();
        sprintf(msg, "Scanning item %d/%d,  %.1f%% complete. To abort scanning, go Tools -> Scan parameters.",
                i, n, prog);
        writeStatusBar(msg);
    }
}



/**
 *  @brief Called whenever model has finished/exited.
 *  - Updating status bar, takes screenshots if scanning etc.
 */
void Hampu::updateModel()
{
    if (DEBUG_MODE) fprintf(stderr, "*** %s:%s()\n", __FILE__, __FUNCTION__);

    progressTimer->stop();

    if (!models.at(currentModel)->getReturnValue()) {
        // Call updateProgress one last time to make the current model view is
        // up-to-date.
        updateProgress();

        // Report total running time.
        int timeDiff = time(NULL)-timeStart;
        char timeMsg[256];
        int hours = timeDiff/(3600);
        int mins = (timeDiff-(hours*3600)) / 60;
        int secs = timeDiff - (hours*3600) - (mins*60);
        sprintf(timeMsg, "Finished after %.2d:%.2d:%.2d.",
                hours, mins, secs);
        writeStatusBar(timeMsg);
    }

    // Renames the current work item in history from "..Running" into number of
    // iterations at finish:
    ToothLife *toothLife = toothHistory.at( toothHistory.size()-1 );
    int stepsize = models.at(toothLife->getCurrentModel())->getStepSize();
    uint32_t last_step = toothLife->getLifeSize()-1;
    controlPanel->endHistory( last_step*stepsize );
    controlPanel->enableModelList(1);
    controlPanel->updateRunStatus("Run");

    if (scanning) {
        QString folder = scanWindow->getResultsFolder();
        if (scanWindow->storeModelSteps()) {
            exportModelData( -1, EXPORT_SCREENSHOTS | EXPORT_DATA, folder );
        }
        else {
            exportModelData( last_step, EXPORT_SCREENSHOTS | EXPORT_DATA, folder );

        }
        char msg[64];
        sprintf(msg, "Data export complete.");
        writeStatusBar(msg);

        // Calls next set of parameters for scanning.
        scanParameters_();
    }
}



/**
 * @brief Requests updating the current development slider position.
 * - Triggered by GLWidget.
 * - Called when using arrow keys to view different development stages.
 *
 * @param dir   Development stage to show.
 */
void Hampu::viewIntSteps(int dir)
{
    if (DEBUG_MODE) fprintf(stderr, "*** %s:%s()\n", __FILE__, __FUNCTION__);

    if (toothHistory.size()==0) {
        return;
    }
    ToothLife *toothLife = toothHistory.at(currentHistory);

    int lastStep = viewIntStep;
    if (dir==-1 && viewIntStep>0) {
        viewIntStep--;
    }
    if (dir==1 && viewIntStep < toothLife->getLifeSize()-1) {
        viewIntStep++;
    }

    if (lastStep != viewIntStep) {
        controlPanel->setSliderValue(viewIntStep);
        updateCurrentStepView_(STATUSBAR_VERBOSE);
    }
}



//
// *** PRIVATE METHODS ***
//



/**
 * @brief Set signaling between different interface components.
 */
void Hampu::setSignals_()
{
    // Needed for threaded signaling to work.
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<int>("int");
    qRegisterMetaType<double>("double");
    qRegisterMetaType<uint>("uint");

    // Signals for control panel actions.
    connect(controlPanel, SIGNAL(viewmode(int)), this,
            SLOT(Panel_ViewMode(int)));
    connect(controlPanel, SIGNAL(thresholdChange(double)), this,
            SLOT(Panel_ViewThreshold(double)));
    connect(controlPanel, SIGNAL(viewOrientation(int)), this,
            SLOT(Panel_Orientation(int)));
    connect(controlPanel, SIGNAL(showMesh(int)), this,
            SLOT(Panel_CellConnections(int)));
    connect(controlPanel, SIGNAL(changeModel(int)), this,
            SLOT(Panel_Model(int)));
    connect(controlPanel, SIGNAL(historyIndex(int)), this,
            SLOT(Panel_History(int)));
    connect(controlPanel, SIGNAL(importFile(std::string)), this,
            SLOT(Panel_Import(std::string)));
    connect(controlPanel, SIGNAL(exportFile(std::string)), this,
            SLOT(Panel_Export(std::string)));
    connect(controlPanel, SIGNAL(changeStepView(int)), this,
            SLOT(Panel_Development(int)));
    connect(controlPanel, SIGNAL(setIterations(int)), this,
            SLOT(Panel_Iterations(int)));
    connect(controlPanel, SIGNAL(startModel(int)), this,
            SLOT(Panel_Run(int)));
    connect(controlPanel, SIGNAL(killModel()), this,
            SLOT(Panel_Stop()));
    connect(controlPanel, SIGNAL(followDevel(int)), this,
            SLOT(Panel_FollowDevelopment(int)));
    connect(controlPanel, SIGNAL(msgStatusBar(std::string)), this,
            SLOT(writeStatusBar(std::string)));

    // Signals with the renderer.
    connect(glwidget, SIGNAL(changeStepView(int)), this,
            SLOT(viewIntSteps(int)));
    connect(glwidget, SIGNAL(resetOrientation(int)), this,
            SLOT(resetOrientation(int)));
    connect(glwidget, SIGNAL(msgStatusBar(std::string)), this,
            SLOT(writeStatusBar(std::string)));

    // Signals with models, progress monitoring.
    for (auto model : models) {
        connect(model, SIGNAL(msgStatusBar(std::string)), this,
                SLOT(writeStatusBar(std::string)));
        connect(model, SIGNAL(finished()), this,
                SLOT(updateModel()));
    }

    // Signals with parameter scanning window.
    connect(scanWindow, SIGNAL(startScan()), this,
            SLOT(startParameterScan()));
    connect(scanWindow, SIGNAL(stopScan()), this,
            SLOT(stopParameterScan()));

    if (DEBUG_MODE) fprintf(stderr, "Signals set.\n");
}



/**
 * @brief Set menu bar.
 */
void Hampu::setMenuBar_()
{
    // File menu.
    QMenu *file = new QMenu("File");
    file->addAction("Exit", this, SLOT(File_Exit()),
                    QKeySequence(Qt::CTRL + Qt::Key_Q));
    menuBar()->addMenu(file);

    // Tools.
    QMenu *tools = new QMenu("Tools");
    tools->addAction("Export data", this, SLOT(Tools_ExportObjects()),
                     QKeySequence(Qt::CTRL + Qt::Key_D));
    tools->addAction("Export images", this, SLOT(Tools_ExportImages()),
                     QKeySequence(Qt::CTRL + Qt::Key_I));
    tools->addAction("Take screenshot", this, SLOT(screenshotWidget()),
                     QKeySequence(Qt::CTRL + Qt::Key_S));
    tools->addAction("Scan parameters", this, SLOT(Tools_ScanParameters()),
                     QKeySequence(Qt::CTRL + Qt::Key_N));
    menuBar()->addMenu(tools);

    // Options.
    QMenu *options = new QMenu("Options");
    options->addAction("Purge history", this, SLOT(Options_PurgeHistory()),
                       QKeySequence(Qt::CTRL + Qt::Key_P));

    // Preferences disabled for now.
    // options->addAction("Preferences", this, SLOT(Options_Preferences()),
    //                    QKeySequence(Qt::CTRL + Qt::Key_R));

    menuBar()->addMenu(options);
}



/**
 * @brief Updates current development stage view.
 * - Called from Hampu when the running model has something new to show.
 *
 * @param quiet     1=Write to status bar, 0=Be quiet.
 */
void Hampu::updateCurrentStepView_(int quiet)
{
    setVisualData();

    // Active progressTimer means a model is running - don't mess with the
    // status bar if that's the case.
    if (quiet || progressTimer->isActive()) {
        return;
    }

    Model* model = models.at(currentModel);
    int stepsize = model->getStepSize();
    ToothLife* toothLife = toothHistory.at(currentHistory);

    uint32_t n_vert = 0;
    uint32_t n_tri = 0;
    Tooth* tooth = toothLife->getTooth(viewIntStep);
    if (tooth != NULL) {
        Mesh& mesh = model->fill_mesh(*tooth);
        n_vert = mesh.get_vertices().size();
        n_tri = mesh.get_polygons().size();
    }

    char msg[256];
    sprintf(msg, "Current step: %d", stepsize*viewIntStep);
    if (n_vert !=0 && n_tri != 0) {
        sprintf( msg, "Current step: %d. Vertices: %d, triangles: %d.",
                 stepsize*viewIntStep, n_vert, n_tri );
    }
    writeStatusBar(msg);
}



/**
 * @brief Gets next scan parameters in GUI scanning.
 * - Initially called by startParameterScan(), then iteratively by
 *   updateModel().
 *
 * TODO: Use the scanning functionality from CmdAppCore.
 */
void Hampu::scanParameters_()
{
    scanning = 1;
    timeLimit = scanWindow->getTimeLimit();

    Parameters *parameters = scanList->getNextScanJob();
    if (parameters==NULL) {
        // Scanning done (or failed for whatever reason).
        scanning = 0;
        timeLimit = -1;
        scanWindow->updateScanStatus("Start");
        controlPanel->enableRunButton(1);
        controlPanel->enableHistory(1);
        return;
    }

    models.at(currentModel)->setParameters(parameters);
    Panel_Run(controlPanel->getnIter());
    parwidget->updateButtonValues();
}



/**
 * @brief Reads default parameter values for the current model.
 */
void Hampu::importExampleParameters_()
{
    QDir qdir(QCoreApplication::applicationDirPath());
    qdir.cd(RESOURCES);
    std::string par = models.at(currentModel)->getExampleParameters();
    QString source = qdir.path() + "/" + QString::fromStdString(par);

    if (par.compare("")) {
        // Stores parameters into the model & update parameter window.
        morphomaker::Import_parameters(source.toStdString(),
                                       models.at(currentModel)->getParameters());
    }
}



/**
 * @brief Returns the maximum number of runs to be stored in the run history.
 * - During parameter scan the history size is fixed to 0, otherwise given by
 *   MAX_HISTORY_SIZE.
 *
 * @return  Max. history size.
 */
unsigned int Hampu::getMaxHistorySize_()
{
    if (!scanning) {
        return (unsigned int)MAX_HISTORY_SIZE;
    }
    return 0;
}



/**
 * @brief Arrow key control for development slider.
 * @param event     Key event.
 */
void Hampu::keyPressEvent(QKeyEvent *event)
{
    if (DEBUG_MODE) fprintf(stderr, "kevent: %d\n", event->key());
    if (event->key()==16777234) {  // Left
        viewIntSteps(-1);
    }
    if (event->key()==16777236) {  // Right
        viewIntSteps(1);
    }
}



/**
 * @brief Called when something is dragged over the window.
 * @param event
 */
void Hampu::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
    }
}



/**
 * @brief Called when a drag object is dropped into the window.
 * - TODO: A clean solution would be based on QMimeDatabase for guessing the
 *   file type, available since Qt 5.0.
 *
 * @param event
 */
void Hampu::dropEvent(QDropEvent *event)
{
    QString parfile = event->mimeData()->text();
    parfile = parfile.trimmed();
    parfile = parfile.remove("file://");
    Panel_Import(parfile.toStdString());
}
