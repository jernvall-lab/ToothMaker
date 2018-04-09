/**
 *  @class ControlPanel
 *  @brief Interface control panel.
 *
 *  Consists of all the controls & buttons below the model view and the parameters window.
 *
 *  TODO: The naming of things has become convoluted and confusing over time. A little
 *  cleaning would not be totally unwarrented.
 */

#include <gui/controlpanel.h>
#include <model.h>


/**
 * @brief ControlPanel constructor.
 * @param parent
 * @param m         Vector of models to be shown in 'Model' menu.
 */
ControlPanel::ControlPanel( QWidget *, std::vector<Model*> *models )
{
    setMinimumSize( CONTROLPANEL_WIDTH, CONTROLPANEL_HEIGHT );

    // View mode menu:
    QLabel *labelView = new QLabel("View mode:", this);
    labelView->move(5,10);
    viewModeBox(102, 5);
    labelView->setBuddy(viewMode);

    // Orientation menu:
    QLabel *orientationLabel = new QLabel("Orientation:", this);
    orientationLabel->move(302,10);
    orientations = orientationBox(379,5);

    // Text-field view threshold.
    QLabel *labelThresh = new QLabel("View threshold:", this);
    labelThresh->move(5,40);
    threshold = new QLineEdit("0.0", this);
    threshold->move(107,37);
    threshold->setFixedWidth(67);
    threshold->setMaxLength(7);
    QValidator *validator = new QDoubleValidator(0.0, 99999, 5, this);
    validator->setLocale(QLocale("C"));
    threshold->setValidator(validator);
    connect( threshold, SIGNAL(textChanged(const QString &)), this,
             SLOT(viewThreshold(const QString &)) );

    // View checkboxes:
    showGrid = createCheckBox(302, 40, "Show mesh", SLOT(cellConnections(int)));
    showGrid->setChecked(SHOW_MESH);

    // Model menu
    QLabel *labelMorpho = new QLabel("Model:", this);
    labelMorpho->move(513, 10);
    model = modelBox( models, 564, 5 );
    model->setFixedWidth(COMBO_WIDTH);
    labelMorpho->setBuddy(model);

    // Import/Export parameters:
    QLabel *labelFiles = new QLabel("Parameters:", this);
    labelFiles->move(764, 10);
    createButton(847, 5, 68, "Import", SLOT(readParameters()));
    createButton(916, 5, 68, "Export", SLOT(saveParameters()));

    // Model history menu:
    QLabel *labelHistory = new QLabel("History:", this);
    labelHistory->move(513, 40);
    history = new QComboBox(this);
    history->move(564, 35);
    history->setFixedWidth(COMBO_WIDTH);
    labelHistory->setBuddy(history);
    connect( history, SIGNAL(currentIndexChanged(int)), this,
             SLOT(changeHistory(int)) );

    // Development slider:
    QLabel *labelDevel = new QLabel("Development:", this);
    labelDevel->move(5,87);
    develSlider = new QSlider(Qt::Horizontal, this);
    develSlider->move(98,82);
    develSlider->setTickPosition(QSlider::TicksBelow);
    develSlider->setMinimum(0);
    develSlider->setMaximum(40);
    develSlider->setMinimumWidth(DEV_SLIDER_WIDTH);
    develSlider->setTickInterval(1);

    // Check devel. slider value at fixed intervals. This is done because
    // connecting valueChanged() to the slot directly floods the event queue
    // on graphically demanding models, making the interface crawl.
    sliderTimer = new QTimer(this);
    sliderTimer->setInterval(UPDATE_INTERVAL);
    connect(sliderTimer, SIGNAL(timeout()), this, SLOT(slider_step_view()));
    sliderTimer->start();
    // Emit slider signal to Hampy only if user inacting with the slider.
    connect(develSlider, SIGNAL(sliderPressed()), this, SLOT(slider_active()));
    connect(develSlider, SIGNAL(sliderReleased()), this, SLOT(slider_inactive()));
    sliderUpdate = false;

    // Iterations & Run button
    QLabel *labelRun = new QLabel("Iterations:", this);
    labelRun->move(513, 87);
    iterations = createSpinBox(580, 84, SLOT(readLineValue(int)));
    iterations->setSingleStep(1000);
    iterations->setMinimum(0);
    iterations->setMaximum(MAX_ITER);
    iterations->setFixedWidth(80);
    iterations->installEventFilter(this);
    connect( iterations, SIGNAL(valueChanged(int)), this,
             SLOT(changeIterations(int)) );

    runStatus = "Run";
    runButton = createButton(673, 82, 68, runStatus, SLOT(handleRunButton()));

    // Checkbox for following development: //
    QCheckBox *follow_devel = createCheckBox( 758, 87, "Follow development",
                                              SLOT(follow_development(int)) );
    follow_devel->setChecked(FOLLOW_DEFAULT);

    currentRunIndex = 0;
    nIter = 0;
}



/**
 * @brief Paints the horizontal line dividing the control panel.
 */
void ControlPanel::paintEvent(QPaintEvent*)
{
    QColor color(210, 210, 210);
    QPainter painter(this);
    painter.fillRect(0, 70, 1000, 2, color);
}



/**
 * @brief Adds a drop-down menu for models.
 * @param models    Vector of models.
 * @param x         Loc. x
 * @param y         Loc. y
 * @return
 */
QComboBox *ControlPanel::modelBox( std::vector<Model*> *models, int x, int y )
{
    QComboBox *morpho = new QComboBox(this);

    if (models != NULL) {
        for (uint16_t i=0; i<models->size(); i++) {
            auto name = models->at(i)->getModelName();
            morpho->addItem( name.c_str() );
        }
    }
    morpho->move(x,y);
    connect( morpho, SIGNAL(currentIndexChanged(int)), this,
             SLOT(modelIndex(int)) );

    return morpho;
}



/**
 * @brief Adds a drop-down menu for view modes.
 * @param x     Loc. x
 * @param y     Loc. y
 */
void ControlPanel::viewModeBox(int x, int y)
{
    viewMode = new QComboBox(this);
    viewMode->move(x,y);
    connect( viewMode, SIGNAL(currentIndexChanged(int)), this,
             SLOT(changeViewMode(int)) );
    viewMode->setFixedWidth(COMBO_WIDTH);
}



/**
 * @brief Adds a drop-down menu for view orientations.
 * @param x     Loc. x
 * @param y     Loc. y
 */
QComboBox *ControlPanel::orientationBox(int x, int y)
{
    QComboBox *orientations = new QComboBox(this);

    orientations->move(x,y);
    orientations->setFixedWidth(ORIENT_WIDTH);
    connect( orientations, SIGNAL(currentIndexChanged(int)), this,
             SLOT(changeOrientation(int)) );

    return orientations;
}



/**
 * @brief Creates a generic push button.
 * @param x         Loc. x
 * @param y         Loc. y
 * @param width     Width of the button
 * @param text      Button text
 * @param member    Signal connected to the button.
 * @return
 */
QPushButton *ControlPanel::createButton( int x, int y, int width,
                                         const QString &text, const char *member )
{
    QPushButton *button = new QPushButton(text, this);
    button->move(x,y);
    connect(button, SIGNAL(clicked()), this, member);
    button->setFixedWidth(width);
    return button;
}



/**
 * @brief Creates a generic spinbox.
 * @param x         Loc. x
 * @param y         Loc. y
 * @param member    Signal connected to the spinbox.
 * @return
 */
QSpinBox *ControlPanel::createSpinBox(int x, int y, const char *member)
{
    QSpinBox *iter = new QSpinBox(this);
    iter->move(x,y);
    connect(iter, SIGNAL(valueChanged(int)), this, member);
    return iter;
}



/**
 * @brief Creates a generic checkbox.
 * @param x         Loc. x
 * @param y         Loc. y
 * @param text      Checkbox text
 * @param member    Signal connected to the checkbox.
 * @return
 */
QCheckBox *ControlPanel::createCheckBox( int x, int y, const QString &text,
                                         const char *member )
{
    QCheckBox *checkbox = new QCheckBox(text, this);
    checkbox->move(x, y);
    connect(checkbox, SIGNAL(stateChanged(int)), this, member);
    return checkbox;
}



/**
 * @brief Emits signal connected to history drop-down.
 * @param mode
 */
void ControlPanel::changeHistory(int mode)
{
    emit historyIndex(mode);
}



/**
 * @brief Emits signal connected to "cell connections" checkbox.
 * @param mode
 */
void ControlPanel::cellConnections(int mode)
{
    emit showMesh(mode);
}



/**
 * @brief Emits signal connected to the view mode.
 * @param mode      View mode.
 */
void ControlPanel::changeViewMode(int mode)
{
    if (DEBUG_MODE) fprintf(stderr, "%s:%s(%d)\n", __FILE__, __FUNCTION__, mode);
    emit viewmode(mode);
}



/**
 * @brief Emits signal connected to the view threshold.
 * @param str       Values entered in the box.
 */
void ControlPanel::viewThreshold(const QString &str)
{
    double val = atof(str.toStdString().c_str());
    emit thresholdChange(val);
}



/**
 * @brief Emits signal connected to iterations.
 * @param val       Number of iterations.
 */
void ControlPanel::changeIterations(int val)
{
    emit setIterations(val);
}



/**
 * @brief Emits signal connected to the model menu.
 * @param i
 */
void ControlPanel::modelIndex(int i)
{
    if (DEBUG_MODE) fprintf(stderr, "%s:%s(%d)\n", __FILE__, __FUNCTION__, i);
    emit changeModel(i);
}



/**
 * @brief Opens a read file dialog for importing parameters.
 */
void ControlPanel::readParameters()
{
    QString filename = QFileDialog::getOpenFileName( this, tr("Read parameters"),
                                                     QDir::homePath(),
                                                     tr("Text Files (*.txt)") );

    if (!filename.isEmpty()) {
        // Send import file name to Hampu:
        emit importFile(filename.toStdString());
    }
}



/**
 * @brief Opens a file write dialog for exporting parameters.
 */
void ControlPanel::saveParameters()
{
    QString filename = QFileDialog::getSaveFileName( this, tr("Save parameters"),
                                                     QDir::homePath(),
                                                     tr("Text Files (*.txt)") );

    if (!filename.isEmpty()) {
        // Send import file name to Hampu:
        emit exportFile(filename.toStdString());
        std::stringstream ss;
        ss << "Save: '" << filename.toStdString() << "'";
        emit msgStatusBar(ss.str());
    }
}



/**
 * @brief Emits signal connected to "Follows development".
 * @param mode      1 true, 0 false.
 */
void ControlPanel::follow_development(int mode)
{
    emit followDevel(mode);
}



/**
 * @brief Sets minimum & maximum values for the development slider.
 * @param min       Min. value.
 * @param max       Max. value.
 */
void ControlPanel::setSliderMinMax(int min, int max)
{
    develSlider->setMinimum(min);
    develSlider->setMaximum(max);
}



/**
 * @brief Emits the signal connected to the devel. slider.
 */
void ControlPanel::slider_step_view()
{
    if (sliderUpdate) {
        emit changeStepView( develSlider->value() );
    }
}



/**
 * @brief Called if devel. slider pressed.
 */
void ControlPanel::slider_active()
{
    sliderUpdate = true;
}



/**
 * @brief Called if devel. slider released.
 */
void ControlPanel::slider_inactive()
{
    sliderUpdate = false;
    emit changeStepView( develSlider->value() );
}



/**
 * @brief Sets the current location of the devel. slider.
 * NOTE: This is the slider numerical value, not the tick position.
 *
 * @param val       Position value.
 */
void ControlPanel::setSliderValue(int val)
{
    develSlider->setValue(val);
}



/**
 * @brief Returns the current location of the devel. slider.
 * NOTE: This is the slider numerical value, not the tick position.
 */
int ControlPanel::getSliderValue()
{
    return develSlider->value();
}



/**
 * @brief Handles run button, emits startModel() or killModel().
 */
void ControlPanel::handleRunButton()
{
    if (!QString::compare(runStatus, "Run")) {
        if (nIter>0) {  // Nothing to do if nIter=0.
            runStatus="Halt";
            if (DEBUG_MODE) fprintf(stderr, "emit startModel()\n");
            emit startModel(nIter);
        }
    }
    else if (!QString::compare(runStatus, "Halt")) {
        runStatus="Run";
        emit killModel();
    }
    else {}

    updateRunStatus(runStatus);
}



/**
 * @brief Updates status bar info.
 * @param status        Message string.
 */
void ControlPanel::updateRunStatus(QString status)
{
    runStatus=status;
    runButton->setText(runStatus);
}



/**
 * @brief Slot called by spinbox for iterations.
 * @param val       Number of iterations.
 */
void ControlPanel::readLineValue(int val)
{
    nIter = val;
}



/**
 * @brief Sets current view mode.
 * @param val       View mode.
 */
void ControlPanel::setViewMode(int val)
{
    viewMode->setCurrentIndex(val);
}



/**
 * @brief Sets view threshold.
 * @param val       View threshold value.
 */
void ControlPanel::setViewThreshold(double val)
{
    if (DEBUG_MODE) fprintf(stderr, "%s:%s(%lf)\n", __FILE__, __FUNCTION__, val);

    threshold->setText(QString::number(val));
}



/**
 * @brief Called from external objects for iterations.
 * @param val       Number of iterations.
 * @param step
 */
void ControlPanel::setnIter(int val)
{
    nIter=val;
    iterations->setValue(nIter);
}



/**
 * @brief Emits signal connected to the view orientation menu.
 * @param val
 */
void ControlPanel::changeOrientation(int val)
{
    emit viewOrientation(val);
}



/** Adds or changes the tooth history.
 *  @param val = '0' adds a dummy entry, '1' adds a proper entry/updates a dummy.
 *  @return     Current history index.
 */
int ControlPanel::addHistory(int val)
{
    char dummy[256], runMsg[256];

    sprintf(dummy, "--");
    sprintf(runMsg, "#%d (Running)", currentRunIndex);

    if (val==0) {
        // Add a dummy history for model change.
        bool state = history->blockSignals(true);
        if (history->count()==0 || (history->itemText(history->count()-1)).compare(dummy)) {
            history->addItem(dummy);
        }
        history->setCurrentIndex(history->count()-1);
        history->blockSignals(state);
        return history->currentIndex();
    }
    // Not adding a new entry but update the old one:
    if (!(history->itemText(history->count()-1)).compare(dummy)) {
        history->setItemText(history->count()-1, runMsg);
    }
    else history->addItem(runMsg);

    history->setCurrentIndex(history->count()-1);
    return history->currentIndex();
}



/** Renames the last piece of history with n.iter. after model exit.
 *  @param niter = number of iterations at model exit.
 */
void ControlPanel::endHistory(int niter)
{
    if (DEBUG_MODE) fprintf(stderr, "%s:%s(%d)\n", __FILE__, __FUNCTION__, niter);

    char msg[256];

    sprintf(msg, "#%d (no.iter. %d)", currentRunIndex, niter);
    history->setItemText(history->count()-1, msg);
    currentRunIndex++;
}



/** Removes the first/last piece of history.
 *  @param i = '0' remove 1st, '1' remove last.
 */
void ControlPanel::removeHistory(int i)
{
    if (i==0) history->removeItem(0);
    if (i==1) history->removeItem(history->count()-1);
}



void ControlPanel::resetOrientation(int val)
{
    orientations->setCurrentIndex(val);
}



void ControlPanel::setModelIndex(int val)
{
    if (DEBUG_MODE) fprintf(stderr, "%s:%s(%d)\n", __FILE__, __FUNCTION__, val);

    bool state = model->blockSignals(true);
    model->setCurrentIndex(val);
    model->blockSignals(state);
}



/**
 * @brief Directs Enter presses on iterations field to the run button.
 * @param event
 * @return
 */
bool ControlPanel::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return) {
            handleRunButton();
            return true;
        }
        else {
            return false;
        }
    }
    return false;
}



/**
 * @brief Enables Orientation drop menu and fills it.
 * @param names     String vector containing the names.
 */
void ControlPanel::setOrientations( const std::vector<orientation>& orient )
{
    orientations->blockSignals(true);   // Block signaling while updating.

    orientations->clear();
    orientations->addItem("");
    if (orient.size()==0) {
        orientations->setEnabled(false);
        return;
    }

    orientations->setEnabled(true);
    for (auto o : orient) {
        orientations->addItem( o.name.c_str() );
    }

    orientations->blockSignals(false);   // Updating done, enable signaling.
}



/**
 * @brief Disable/Enable 'Show mesh' in control panel, set checked state.
 * @param enabled       true if enabled
 * @param checked       true if checked
 */
void ControlPanel::showCellConnections( bool enabled, bool checked )
{
    showGrid->setChecked( checked );
    showGrid->setEnabled( enabled );
}



/**
 * @brief Disable/enbale run button in control panel.
 * @param i     0 to disable, 1 to enable.
 */
void ControlPanel::enableRunButton(int i)
{
    if (i) runButton->setEnabled(true);
    else runButton->setEnabled(false);
}



/**
 * @brief Disable/Enable 'Model' in control panel.
 * @param i     0 to disable, 1 to enable.
 */
void ControlPanel::enableModelList(int i)
{
    if (i) model->setEnabled(true);
    else model->setEnabled(false);
}



/**
 * @brief Disable/Enable 'History' in control panel.
 * @param i     0 to disable, 1 to enable.
 */
void ControlPanel::enableHistory(int i)
{
    if (i) history->setEnabled(true);
    else history->setEnabled(false);
}



/**
 * @brief Fills 'View mode' according to the model type.
 * @param viewModes    Pointer to view mode names string vector.
 * @param viewmode     Value of the menu to be set.
 */
void ControlPanel::setViewModeBox( std::vector<std::string> *viewModes,
                                   int viewmode )
{
    disconnect( viewMode, SIGNAL(currentIndexChanged(int)), this,
                SLOT(changeViewMode(int)) );
    viewMode->clear();
    if (viewModes==NULL) return;

    for (uint16_t i=0; i<viewModes->size(); i++) {
        viewMode->addItem(viewModes->at(i).c_str());
    }

    viewMode->setCurrentIndex(viewmode);
    changeViewMode(viewmode);

    connect( viewMode, SIGNAL(currentIndexChanged(int)), this,
             SLOT(changeViewMode(int)) );
}



/**
 * @brief Returns the current number in the iterations box.
 * @return
 */
int ControlPanel::getnIter()
{
    return nIter;
}
