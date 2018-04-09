/**
 * @class ParameterWindow
 * @brief Constructs model GUIs.
 *
 * - Constructs the parameter window, enables/disables control panel features.
 */

#include "gui/parameterwindow.h"
#include "utils/readxml.h"


/**
 * @brief ParameterWindow constructor.
 * @param parent
 */
ParameterWindow::ParameterWindow(QWidget *parent)
{
    (void)parent;

    model = NULL;

    setMinimumSize(SQUARE_WIN_SIZE,SQUARE_WIN_SIZE);
    setMaximumSize(SQUARE_WIN_SIZE,SQUARE_WIN_SIZE);
}



/**
 * @brief ParameterWindow destructor.
 */
ParameterWindow::~ParameterWindow()
{
}



/**
 * @brief Prepares model interface.
 * @param m     Model object.
 */
void ParameterWindow::set_model(Model *m)
{
    model = m;
}



/**
 * @brief Places parameter buttons & value fields to the current parameter window.
 * @param par       Parameters object.
 */
void ParameterWindow::set_parameter_buttons( Parameters &par )
{
    pvector& buttons = par.getButtonLocations();
    std::vector<int>& widths = par.getButtonWidths();
    std::vector<std::string>* names = par.getParNames();
    std::vector<std::string>* notes = par.getParNotes();

    if (buttons.size()!=names->size() || buttons.size()!=notes->size() ||
        buttons.size()!=widths.size()) {
        // TODO: Add some warning message.
        return;
    }

    for (uint32_t i=0; i<names->size(); i++) {
        auto name = names->at(i);
        auto note = notes->at(i);
        buttonNames.push_back( name.c_str() );
        buttonNotes.push_back( note.c_str() );
        bool hidden = par.isParameterHidden(name);

        auto x = buttons.at(i).first;
        auto y = buttons.at(i).second;
        createButton( x, y, buttonNames.size()-1, widths.at(i), !hidden );

        // Set button field at a fixed position to the right of the button.
        x = x + widths.at(i);
        addValueField( x, y, !hidden );
    }
}



void ParameterWindow::add_file_dialog( QString& name, int buttonx, int buttony )
{
    QPushButton *button = new QPushButton(name, this);
    connect(button, SIGNAL(clicked()), this, SLOT(importFile()));
    button->move(buttonx, buttony);
    button->setFixedWidth(80);

    QLabel *prepat = new QLabel(this);
    fileLabels.push_back(prepat);
    int frameStyle = 0;
    prepat->setFrameStyle(frameStyle);
    prepat->move(buttonx+10, buttony+30);
    prepat->setFixedWidth(145);
}



/**
 * @brief Updates parameter values in value fields.
 */
void ParameterWindow::updateButtonValues()
{
    char val[64];
    int pos;

    if (buttonNames.size()>0) {
        Parameters *par = model->getParameters();
        for (int i=0; i<buttonNames.size(); i++) {
            // TODO: .12 should be taken from PARAM_PREC. How?
            sprintf( val, "%.12lf",
                     par->getParameter(buttonNames.at(i).toStdString()) );

            // Remove trailing zeros:
            pos = strlen(val)-1;
            while (val[pos] == '0') pos--;
            if (val[pos]=='.') val[pos+2] = '\0';
            else val[pos+1] = '\0';

            valueFields.at(i)->setText(val);
            valueFields.at(i)->setCursorPosition(0);
        }
    }
    else {
        std::vector<double> *values = model->getParameters()->getParValues();
        for (int i=0; i<(int)valueFields.size(); i++) {
            // TODO: .12 should be taken from PARAM_PREC. How?
            sprintf(val, "%.12lf", values->at(i));

            // Remove trailing zeros:
            pos = strlen(val)-1;
            while (val[pos] == '0') pos--;
            if (val[pos]=='.') val[pos+2] = '\0';
            else val[pos+1] = '\0';

            valueFields.at(i)->setText(val);
            valueFields.at(i)->setCursorPosition(0);
        }
    }
}



/**
 * @brief Slot, connects value fields to parameter values.
 * @param i     parameter index.
 */
void ParameterWindow::setParValue(int i)
{
    model->getParameters()->setParameter(buttonNames.at(i).toStdString(),
                                         valueFields.at(i)->text().toDouble());
}



/**
 * @brief Slot, message box for parameter buttons.
 * @param i     parameter index.
 */
void ParameterWindow::infoBox(int i)
{
    QMessageBox msg;

    if (buttonNames.size()>0) {
        msg.setText(buttonNames.at(i));
        msg.setInformativeText(buttonNotes.at(i));
        msg.exec();
    }
    else {
        std::vector<std::string> *names = model->getParameters()->getParNames();
        std::vector<std::string> *notes = model->getParameters()->getParNotes();

        msg.setText(QString(names->at(i).c_str()));
        msg.setInformativeText(QString(notes->at(i).c_str()));
        msg.exec();
    }
}



/**
 * @brief Slot for parameter window file import events.
 * TODO: Currently only supports single file label.
 */
void ParameterWindow::importFile()
{
    QString fname = QFileDialog::getOpenFileName( this,
                                                  tr("Import prepattern image (.bmp)."),
                                                  QDir::homePath(),
                                                  tr("Text Files (*.bmp)") );

    if (!fname.isEmpty()) {
        modelFiles.push_back( fname.toStdString() );
        model->getParameters()->addModelFile( fname.toStdString() );
        QFileInfo fileInfo( fname );
        fileLabels.at(0)->setText( fileInfo.fileName() );
    }
}



/**
 * @brief Adds a parameter value field to (x,y).
 * @param x         x loc.
 * @param y         y loc.
 * @param show      show/hide value field.
 */
void ParameterWindow::addValueField(int x, int y, bool show)
{
    QLineEdit *field = new QLineEdit("0.0", this);
    field->setStyleSheet("QLineEdit{background-color: rgba(100%, 100%, 100%, 0%); border: 1px solid black}");
    field->move(x, y+FIELD_PADDING);
    field->setFixedWidth(77);

    QValidator *validator = new QDoubleValidator(0.0, PARAM_MAX, PARAM_PREC, this);
    validator->setLocale(QLocale("C"));
    field->setValidator(validator);

    valueFields.push_back(field);

    QSignalMapper *signalMapper = new QSignalMapper(this);
    signalMapper->setMapping(field, valueFields.size()-1);
    connect(field, SIGNAL(textChanged(const QString &)), signalMapper, SLOT(map()));
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(setParValue(int)));

    if (!show) field->hide();
}



/**
 * @brief Creates a parameter button at (x,y).
 * @param x         x position
 * @param y         y position
 * @param i         parameter index.
 * @param width     button width.
 * @param show      show/hide button.
 */
void ParameterWindow::createButton(int x, int y, int i, int width, bool show)
{
    QString name;
    if (buttonNames.size()>i) {
        name = buttonNames.at(i);
    }
    else {
        name = QString( model->getParameters()->getParNames()->at(i).c_str() );
    }

    QPushButton *button;
    if (show)
        button  = new QPushButton(name, this);
    else
        button = new QPushButton(name);

    button->move(x,y);
    button->setFixedWidth(width);
    buttons.push_back(button);

    // Mapping parameter buttons to corresponding value slots.
    QSignalMapper *signalMapper = new QSignalMapper(this);
    signalMapper->setMapping(button, i);
    connect(button, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(infoBox(int)));
}



/**
 * @brief Paints the inteface background.
 */
void ParameterWindow::paintEvent(QPaintEvent*)
{
    // Background image under ../Resources/ relative to the app. dir.
    QDir *dir = new QDir(QCoreApplication::applicationDirPath());
    dir->cd(RESOURCES);

    QString source = dir->path() + "/" + model->getBackgroundImage();
    QPixmap pixmap;
    pixmap.load(source);
    QPainter p(this);
    p.drawPixmap(0,0,pixmap);
}
