/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.7
 Written in Qt

                        main.cpp  -  description
                             -------------------
    copyright         : (C) 2005-2020 by Dimitris B. Kalamaras
    project site      : https://socnetv.org

 ***************************************************************************/

/*******************************************************************************
*     This program is free software: you can redistribute it and/or modify     *
*     it under the terms of the GNU General Public License as published by     *
*     the Free Software Foundation, either version 3 of the License, or        *
*     (at your option) any later version.                                      *
*                                                                              *
*     This program is distributed in the hope that it will be useful,          *
*     but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*     GNU General Public License for more details.                             *
*                                                                              *
*     You should have received a copy of the GNU General Public License        *
*     along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
********************************************************************************/

#include <QApplication>		//core Qt functionality
#include <QScreen>
#include <QtDebug>
#include <QFile>
#include <QTranslator>		//for text translations
#include <QLocale>
#include <iostream>			//used for cout
#include "mainwindow.h"		//main application window

using namespace std;

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(src);

    // Todo: debug highdpiscaling (Windows
//    QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
//    QApplication::setAttribute(Qt::AA_Use96Dpi);

//    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
//    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);

    qDebug() << QApplication::primaryScreen()->geometry();
    qDebug() << QApplication::primaryScreen()->size();
    qDebug() << QApplication::primaryScreen()->availableSize();
     qDebug() << QApplication::primaryScreen()->devicePixelRatio(); //the scale factor applied by the OS/Windowing system (e.g. 1 or 2)
     qDebug() << QApplication::primaryScreen()->logicalDotsPerInch(); // the logical DPI of the screen (e.g. 144 on Windows "Make everything bigger" 150%)

    //QScreen::logicalBaseDpi() // [new] : the base DPI on the system (e.g. 96 on Windows, 72 on macOS)

     // Todo update/remove translations
    QTranslator tor( 0 );
    QLocale locale;

    // set the location where .qm files are in load() below as the last parameter instead of "."
    // for development, use "/" to use the english original as
    // .qm files are stored in the base project directory.

    tor.load( QString("socnetv.") + locale.name(), "." );
    app.installTranslator( &tor );

    //Check if a filename is passed when this program is called.
    QString option;
    if ( argc > 1 )     {
        option = argv[1];
        if (option=="--help" || option=="-h" || option=="--h" || option=="-help" ) {
            cout<<"\nSocial Network Visualizer v." << qPrintable(VERSION)<< "\n"
               <<"\nUsage: socnetv [flags] [file]\n"
              <<"-h, --help 	Displays this help message\n"
             <<"-V, --version	Displays version number\n\n"
            <<"You can load a network from a file using \n"
            <<"socnetv file.net \n"
            <<"where file.net/csv/dot/graphml must be of valid format. See README\n\n"

            <<"Please send any bug reports to dimitris.kalamaras@gmail.com.\n\n";
            return -1;
        }
        else if (option=="-V" || option=="--version") {
            cout<<"\nSocial Network Visualizer v." << qPrintable(VERSION)
               << "\nCopyright Dimitris V. Kalamaras, \nLicense: GPL3\n\n";
            return -1;
        }
        else  {
            cout<<"\nSocial Network Visualizer v." << qPrintable(VERSION);
            cout<<"\nLoading file: " << qPrintable(option) << "\n\n";
        }

    }

    // Create our MainWindow
    MainWindow *socnetv=new MainWindow(option);

    // Load our default stylesheet
    QString sheetName = "default.qss";
    QFile file(":/qss/" + sheetName );
    file.open(QFile::ReadOnly);
    QString styleSheet = QString::fromLatin1(file.readAll());
    file.close();

    // Apply our default stylesheet
    qApp->setStyleSheet(styleSheet);

    // Show the application
    socnetv->show();

    return app.exec();
}


