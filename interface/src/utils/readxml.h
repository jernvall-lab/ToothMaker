#ifndef READXML_H
#define READXML_H

#include <QXmlStreamReader>

#include <parameters.h>
#include <gui/parameterwindow.h>

#define MAX_XML_COUNT   10000   // Line number limit for XML reader.


namespace morphomaker {

int Read_binary_definitions( const QString, Model& );

int Read_GUI_definitions( const QString, Model&, ParameterWindow& );

}


#endif // READXML_H
