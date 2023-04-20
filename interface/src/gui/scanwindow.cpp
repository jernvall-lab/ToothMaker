/**
 * @class ScanWindow
 * @brief GUI for parameter scanning.
 */

#include <QLineEdit>
#include "gui/scanwindow.h"



/**
 * @brief ScanWindow constructor.
 * @param parent
 */
ScanWindow::ScanWindow(QWidget *parent) : QDialog(parent)
{
    scanList = new ScanList();

    setMinimumSize(620, 400);
    setMaximumSize(620, 400);
    setWindowTitle("Scan parameters");

    QLabel *lab2 = new QLabel("Select parameters & scanning range:", this);
    lab2->move(10, 10);
    lab2->setMinimumWidth(200);

    // Parameters table.
    table = new QTableWidget(0, 5, this);
    //connect(table, SIGNAL(itemEntered(QTableWidgetItem*)), this, SLOT(cellCheckboxToggle(QTableWidgetItem*)));
    QStringList headers;
    headers << "Parameter" << "Scan" << "From" << "Step" << "To";
    table->setHorizontalHeaderLabels(headers);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    table->setColumnWidth(0, 80);
    table->setColumnWidth(1, 35);
    table->setColumnWidth(2, 70);
    table->setColumnWidth(3, 70);
    table->setColumnWidth(4, 70);
    table->move(10, 35);

    table->setMinimumWidth(ROW_WIDTH+25);
    table->setMinimumHeight(MAX_TABLE_HEIGHT);
    table->setMaximumWidth(ROW_WIDTH+25);
    table->setMaximumHeight(MAX_TABLE_HEIGHT);
    connect(table, SIGNAL(cellChanged(int, int)), this, SLOT(cellValueChanged(int, int)));

    // Additional settings.
    exportCheckbox = new QCheckBox("Export vertices/cell data", this);
    exportCheckbox->move(400, 40);
    exportCheckbox->setMinimumWidth(100);
    exportCheckbox->setChecked(true);
    connect(exportCheckbox, SIGNAL(stateChanged(int)), this, SLOT(exportModelData()));

    combCheckbox = new QCheckBox("Scan combinations", this);
    combCheckbox->setCheckState(Qt::Unchecked);
    combScanning=0;
    combCheckbox->move(400, 70);
    combCheckbox->setMinimumWidth(100);
    connect(combCheckbox, SIGNAL(stateChanged(int)), this, SLOT(scanCombinations()));

    stepsCheckbox = new QCheckBox("Store intermediate steps", this);
    stepsCheckbox->move(400, 100);
    stepsCheckbox->setMinimumWidth(100);
    stepsCheckbox->setChecked(false);
    connect(stepsCheckbox, SIGNAL(stateChanged(int)), this, SLOT(exportIntSteps()));

    orientCheckbox = new QCheckBox("Store all orientations", this);
    orientCheckbox->move(400, 130);
    orientCheckbox->setMinimumWidth(100);
    orientCheckbox->setChecked(false);
    //connect(stepsCheckbox, SIGNAL(stateChanged(int)), this, SLOT(exportIntSteps()));

    QLabel *ncomb = new QLabel("Number of jobs:", this);
    ncomb->move(NJOBS_X-108, NJOBS_Y-50);

    QLabel *labelThresh = new QLabel("Time limit per simulation\n(seconds; -1 = no limit):", this);
    labelThresh->move(400,210);
    timeLimitBox = new QLineEdit("-1", this);
    timeLimitBox->move(400,248);
    timeLimitBox->setFixedWidth(67);
    timeLimitBox->setMaxLength(7);
    QValidator *validator = new QIntValidator(-999999, 999999, this);
    validator->setLocale(QLocale("C"));
    timeLimitBox->setValidator(validator);

    createButton(393, 300, "Save to..", SLOT(selectStorageFolder()));
    int frameStyle = QFrame::Sunken | QFrame::Panel;
    resLabel = new QLabel(this);
    resLabel->setFrameStyle(frameStyle);
    resLabel->move(400, 335);
    resLabel->setFixedWidth(200);

    scanStatus = "Start";
    startButton = createButton(5, 365, scanStatus, SLOT(handleStartButton()));
}



/**
 * @brief Populates the parameters table for the currrent model.
 * - Called whenever model is changed in the interface.
 *
 * @param names     Parameter names.
 * @param values    Parameter base values.
 */
void ScanWindow::setParameters(Parameters *par)
{
    tableSet = false;
    if (table!=NULL) {
        while (table->rowCount() > 0) {
            table->removeRow(0);
        }
        table->clearContents();
    }

    for (auto& p : par->getParameters()) {
        if (!p.hidden) {
            addParameterRow( p.name.c_str() );
        }
    }

    tableSet = true;
    printNofJobs(0);
}



/**
 * @brief Resets current scan list.
 */
void ScanWindow::resetScanList()
{
    scanList->reset();
}



/**
 * @brief Gets current scanlist.
 * @return
 */
ScanList *ScanWindow::getScanList()
{
    return scanList;
}



/**
 * @brief Gets results folder set by user.
 * @return
 */
QString ScanWindow::getResultsFolder()
{
    return resultsFolder;
}



/**
 * @brief Sets scanning status.
 * @param status
 */
void ScanWindow::updateScanStatus(QString status)
{
    scanStatus=status;
    startButton->setText(scanStatus);
}



/**
 * @brief Returns the scan combinations checkbox state.
 * @return  1 if scan combinations checked, else 0.
 */
int ScanWindow::calcPermutations()
{
    return combScanning;
}



/**
 * @brief Returns a truth value of exporting intermediate steps.
 * @return  1 if export step, else 0.
 */
int ScanWindow::storeModelSteps()
{
    return (stepsCheckbox->checkState()==Qt::Checked);
}



/**
 * @brief Returns a truth value of exporting all model orientations.
 * @return  1 if export orient., else 0.
 */
int ScanWindow::storeOrientations()
{
    return (orientCheckbox->checkState()==Qt::Checked);
}



/**
 * @brief Signal slot for selecting scan output folder.
 */
void ScanWindow::selectStorageFolder()
{
    QDir *qdir = new QDir();

    // Selects results folder for scanning output.
    resultsFolder = QFileDialog::getExistingDirectory(this, "Select folder for scan results", qdir->homePath());
    std::string msg;
    msg = "Store results to: " + resultsFolder.toStdString();
    writeStatusBar(msg);
    resLabel->setText(resultsFolder);

    // Add required sub-folders.
    char tmp[1024];
    sprintf(tmp, "%s/%s", resultsFolder.toStdString().c_str(), SSHOT_SAVE_DIR);
    if (!qdir->exists(QString(tmp))) {
        qdir->mkdir(QString(tmp));
    }
    sprintf(tmp, "%s/%s", resultsFolder.toStdString().c_str(), DATA_SAVE_DIR);
    if (!qdir->exists(QString(tmp))) {
        qdir->mkdir(QString(tmp));
    }
}



/**
 * @brief Signal slot for handling Start button.
 */
void ScanWindow::handleStartButton()
{
    if (!resultsFolder.compare("")) {
        std::string msg = "Select results folder first!";
        writeStatusBar(msg);
        return;
    }

    if (!QString::compare(scanStatus, "Start")) {
        scanStatus="Stop";
        emit startScan();
    }
    else if (!QString::compare(scanStatus, "Stop")) {
        scanStatus="Start";
        emit stopScan();
        // currentItem=0;
        // currentStep=0;
    }
    else {}

    startButton->setText(scanStatus);
}



/**
 * @brief Signal slot for scanning combinations.
 */
void ScanWindow::scanCombinations()
{
    if (combCheckbox->checkState()==Qt::Checked) {
        combScanning=1;
    }
    else {
        combScanning=0;
    }
    printNofJobs(scanList->getNofJobs(combScanning));
}



/**
 * @brief Signal slot for exporting model data during scan.
 */
void ScanWindow::exportModelData()
{
    if (exportCheckbox->checkState()==Qt::Checked) {
        exportData=1;
    }
    else {
        exportData=0;
    }
}



/**
 * @brief Signal slots for exporinting intermediate steps.
 */
void ScanWindow::exportIntSteps()
{
}



/**
 * @brief Signal slot fired whenever a cell value changed.
 * @param row       Cell row
 * @param column    Cell column
 */
void ScanWindow::cellValueChanged(int row, int column)
{
    if (!tableSet) {
        return;
    }

    if (column==1) {
        std::string parName = table->item(row,0)->text().toStdString();

        if (table->item(row,column)->checkState()==Qt::Checked) {
            scanList->addScanItem(createScanItem(row));
        }
        else {
            scanList->removeScanItem(parName);
        }
        printNofJobs(scanList->getNofJobs(combScanning));
        return;
    }

    // Changing parameter value. Replace the old scanItem.
    scanList->addScanItem(createScanItem(row));
    if (table->item(row,1)->checkState()==Qt::Checked) {
        printNofJobs(scanList->getNofJobs(combScanning));
    }
}



/**
 * @brief Creates a scan item for a parameter.
 * @param i     Parameter index.
 * @return      Scan item.
 */
ScanItem *ScanWindow::createScanItem(int i)
{
    double val;
    std::string parName = table->item(i,0)->text().toStdString();
    ScanItem *scanItem = new ScanItem();

    scanItem->setParName(parName);
    val = table->item(i,2)->text().toDouble();
    scanItem->setMinValue(val);
    val = table->item(i,3)->text().toDouble();
    scanItem->setStep(val);
    val = table->item(i,4)->text().toDouble();
    scanItem->setMaxValue(val);

    return scanItem;
}



/**
 * @brief Prints text to the widget status bar.
 * @param s     Text to print.
 */
void ScanWindow::writeStatusBar(std::string s)
{
    printTextMsg = s;
    update();
}



/**
 * @brief Prints the current number of jobs.
 * @param n     Number of jobs.
 */
void ScanWindow::printNofJobs(int n)
{
    std::stringstream s;
    s << n;
    njobs_msg = s.str();
    update();
}



/**
 * @brief Adds a table rows for a given parameter by name.
 * @param parname       Parameter name.
 */
void ScanWindow::addParameterRow(QString parname)
{
    int i = table->rowCount();
    table->setRowCount(i+1);

    QTableWidgetItem *item = new QTableWidgetItem();
    item->setFlags(Qt::ItemFlags(Qt::ItemIsEnabled));
    item->setText(parname);
    table->setItem(i, 0, item);

    QTableWidgetItem *item2 = new QTableWidgetItem();
    item2->setCheckState(Qt::Unchecked);
    item2->setFlags(Qt::ItemFlags(Qt::ItemIsEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsSelectable));
    table->setItem(i, 1, item2);

    QTableWidgetItem *item3 = new QTableWidgetItem();
    item3->setText("0.0");
    table->setItem(i, 2, item3);

    QTableWidgetItem *item4 = new QTableWidgetItem();
    item4->setText("0.0");
    table->setItem(i, 3, item4);

    QTableWidgetItem *item5 = new QTableWidgetItem();
    item5->setText("0.0");
    table->setItem(i, 4, item5);

}



/**
 * @brief Prints text to the widget.
 * - Assumes global variables printTextX, printTextY and printTextMsg.
 */
void ScanWindow::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.drawText(STATUS_BAR_X, STATUS_BAR_Y, printTextMsg.c_str());
    painter.drawText(NJOBS_X, NJOBS_Y-37, njobs_msg.c_str());
}



/**
 * @brief Creates a generic push button.
 * @param x         Location x.
 * @param y         Location y.
 * @param text      Button text.
 * @param member    Button signal slot.
 * @return          Created button.
 */
QPushButton *ScanWindow::createButton(int x, int y, const QString &text, const char *member)
{
    QPushButton *button = new QPushButton(text, this);
    button->move(x,y);
    connect(button, SIGNAL(clicked()), this, member);
    return button;
}
