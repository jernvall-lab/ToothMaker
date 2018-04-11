/**
 * @class ParameterWindow
 * @brief Constructs model GUIs.
 *
 * Constructs the parameter window, enables/disables control panel features.
 *
 */

#include <iostream>

#include "gui/parameterwindow.h"
#include "parameters.h"
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
void ParameterWindow::setModel(Model *m)
{
    model = m;
}



/**
 * @brief Places parameter buttons/value fields and checkboxes to the current
 *        parameter window.
 * @param par       Parameters object.
 */
void ParameterWindow::setParameters( Parameters& par )
{
    auto& parameters = par.getParameters();

    size_t n = parameters.size();
    fileLabels_.resize(n);
    valueFields_.resize(n);
    checkboxes_.resize(n);
    buttons_.resize(n);

    for (size_t i=0; i<parameters.size(); i++) {
        auto& p = parameters.at(i);

        names_.push_back( p.name.c_str() );
        notes_.push_back( p.description.c_str() );
        int x = p.position.first;
        int y = p.position.second;

        if (p.type == PARTYPE_FIELD) {
            // Button widths based on text length.
            // TODO: These should be defined in the header or so.
            int button_width = 65;
            if (p.name.length() > 5)
                button_width = 6 * p.name.length() + 35;

            createButton_( x, y + BUTTON_V_PADDING, i, button_width, !p.hidden );
            x = x + button_width + FIELD_H_PADDING;
            addValueField_( x, y + FIELD_V_PADDING, i, !p.hidden );
        }
        if (p.type == PARTYPE_CHECKBOX)
            addCheckbox_( QString::fromStdString(p.name), x, y, i );
    }
}



/**
 * @brief Creates a file import dialog.
 * @param name  plain text name
 * @param x     position x
 * @param y     position y
 * @param i     parameter index
 */
void ParameterWindow::addFileDialog( QString& name, int x, int y )
{
    QPushButton *button = new QPushButton(name, this);
    connect(button, SIGNAL(clicked()), this, SLOT(importFile()));
    button->move(x, y);
    button->setFixedWidth(80);

    QLabel *prepat = new QLabel(this);
    fileLabels_.push_back(prepat);
    prepat->setFrameStyle(0);
    prepat->move(x+10, y+30);
    prepat->setFixedWidth(145);
}



/**
 * @brief Adds a checkbox to position (x,y), connects the signals.
 * @param name  plain text name
 * @param x     position x
 * @param y     position y
 * @param i     parameter index
 */
void ParameterWindow::addCheckbox_( QString text, int x, int y, int i )
{
    QCheckBox* checkbox = new QCheckBox(text, this);
    checkbox->move(x, y);
    checkboxes_[i] = checkbox;

    QSignalMapper *signalMapper = new QSignalMapper(this);
    signalMapper->setMapping( checkbox, i );
    connect( checkbox, SIGNAL(stateChanged(int)), signalMapper, SLOT(map()) );
    connect( signalMapper, SIGNAL(mapped(int)), this, SLOT(checkbox_state(int)) );
}



/**
 * @brief Updates parameter value fields and checkbox states.
 */
void ParameterWindow::updateButtonValues()
{
    auto& parameters = model->getParameters()->getParameters();

    for (size_t i=0; i<parameters.size(); i++) {
        auto& p = parameters.at(i);

        if (p.type == PARTYPE_FIELD) {
            // TODO: .12 should be taken from PARAM_PREC
            char val[64];
            sprintf( val, "%.12lf", p.value );

            // Remove trailing zeros:
            int pos = strlen(val)-1;
            while (val[pos] == '0') pos--;
            if (val[pos]=='.') val[pos+2] = '\0';
            else val[pos+1] = '\0';

            valueFields_.at(i)->setText(val);
            valueFields_.at(i)->setCursorPosition(0);
        }

        if (p.type == PARTYPE_CHECKBOX) {
            if (p.value > 0.5)
                checkboxes_.at(i)->setCheckState( Qt::Checked );
            else
                checkboxes_.at(i)->setCheckState( Qt::Unchecked );
        }
    }
}



/**
 * @brief Slot, connects value fields to parameter values.
 * @param i     Parameter index.
 */
void ParameterWindow::setParValue(int i)
{
    auto name = names_.at(i).toStdString();
    double value = valueFields_.at(i)->text().toDouble();
    model->getParameters()->setParameterValue( name, value );

    emit parameter_changed();
}



/**
 * @brief Slot, toggle checkbox state.
 * @param i     Checkbox vector index
 */
void ParameterWindow::checkbox_state( int i )
{
    auto name = names_.at(i).toStdString();
    double value = checkboxes_.at(i)->isChecked() ? 1.0 : 0.0;
    model->getParameters()->setParameterValue( name, value );

    emit parameter_changed();
}



/**
 * @brief Slot, message box for parameter buttons.
 * @param i     parameter index.
 */
void ParameterWindow::infoBox(int i)
{
    QMessageBox msg;

    if (buttonNames.size() > 0) {
        msg.setText(buttonNames.at(i));
        msg.setInformativeText(buttonNotes.at(i));
        msg.exec();
        return;
    }

    auto& p = model->getParameters()->getParameters();
    auto name = p.at(i).name.c_str();
    auto notes = p.at(i).description.c_str();

    msg.setText( QString(name) );
    msg.setInformativeText( QString(notes) );
    msg.exec();
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
        fileLabels_.at(0)->setText( fileInfo.fileName() );
    }
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



/**
 * @brief Adds a parameter value field to position (x,y).
 * @param x         position x
 * @param y         position y
 * @param i         parameter index
 * @param show      show/hide value field.
 */
void ParameterWindow::addValueField_(int x, int y, int i, bool show)
{
    QLineEdit *field = new QLineEdit("0.0", this);
    field->setStyleSheet("QLineEdit{background-color: rgba(100%, 100%, 100%, 0%); "
                         "border: 1px solid black}");
    field->move(x, y);
    field->setFixedWidth(77);

    QValidator *validator = new QDoubleValidator(0.0, PARAM_MAX, PARAM_PREC, this);
    validator->setLocale(QLocale("C"));
    field->setValidator(validator);
    valueFields_[i] = field;

    QSignalMapper *signalMapper = new QSignalMapper(this);
    signalMapper->setMapping(field, i);
    connect(field, SIGNAL(textChanged(const QString &)), signalMapper, SLOT(map()));
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(setParValue(int)));

    if (!show)
        field->hide();
}



/**
 * @brief Creates a parameter button at (x,y).
 * @param x         x position
 * @param y         y position
 * @param i         parameter index.
 * @param width     button width.
 * @param show      show/hide button.
 */
void ParameterWindow::createButton_(int x, int y, int i, int width, bool show)
{
    QString name = names_.at(i);

    QPushButton *button;
    if (show)
        button  = new QPushButton(name, this);
    else
        button = new QPushButton(name);

    button->move(x,y);
    button->setFixedWidth(width);
    buttons_[i] = button;

    // Mapping parameter buttons to corresponding value slots.
    QSignalMapper *signalMapper = new QSignalMapper(this);
    signalMapper->setMapping(button, i);
    connect(button, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(infoBox(int)));
}
