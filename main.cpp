/*
    Copyright 2011-2012 Heikki Holstila <heikki.holstila@gmail.com>

    This file is part of FingerTerm.

    FingerTerm is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    FingerTerm is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FingerTerm.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "qplatformdefs.h"

#include <QtGui>
#include <QtQml>
#include <QQuickView>
#include <QDir>
#include <QString>

extern "C" {
#include <pty.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
}

#include "ptyiface.h"
#include "terminal.h"
#include "textrender.h"
#include "util.h"
#include "version.h"
#include "keyloader.h"

#ifdef __arm__
#include <QtPlugin>
Q_IMPORT_PLUGIN(QsgEpaperPlugin)
#endif

int main(int argc, char *argv[])
{
#ifdef __arm__
    qputenv("QMLSCENE_DEVICE", "epaper");
    qputenv("QT_QPA_PLATFORM", "epaper:enable_fonts");
    qputenv("QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS", "rotate=180");
#endif
    QCoreApplication::setApplicationName("FingerTerm");
    QDir dir;

    QString settings_path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (!dir.exists(settings_path)) {
        if (!dir.mkdir(settings_path))
            qWarning() << "Could not create fingerterm settings path" << settings_path;
    }


    QSettings *settings = new QSettings(settings_path + "/settings.ini", QSettings::IniFormat);


    // fork the child process before creating QGuiApplication
    int socketM;
    int pid = forkpty(&socketM,NULL,NULL,NULL);
    if( pid==-1 ) {
        qFatal("forkpty failed");
        exit(1);
    } else if( pid==0 ) {
        setenv("TERM", settings->value("terminal/envVarTERM", "xterm").toByteArray(), 1);

        QString execCmd;
        for(int i=0; i<argc-1; i++) {
            if( QString(argv[i]) == "-e" )
                execCmd = QString(argv[i+1]);
        }
        if(execCmd.isEmpty()) {
            execCmd = settings->value("general/execCmd").toString();
        }
        if(execCmd.isEmpty()) {
            // execute the user's default shell
            passwd *pwdstruct = getpwuid(getuid());
            execCmd = QString(pwdstruct->pw_shell);
            execCmd.append(" --login");
        }

        delete settings; // don't need 'em here

        QStringList execParts = execCmd.split(' ', QString::SkipEmptyParts);
        if(execParts.length()==0)
            exit(0);
        char *ptrs[execParts.length()+1];
        for(int i=0; i<execParts.length(); i++) {
            ptrs[i] = new char[execParts.at(i).toLatin1().length()+1];
            memcpy(ptrs[i], execParts.at(i).toLatin1().data(), execParts.at(i).toLatin1().length());
            ptrs[i][execParts.at(i).toLatin1().length()] = 0;
        }
        ptrs[execParts.length()] = 0;

        execvp(execParts.first().toLatin1(), ptrs);
        exit(0);
    }

    QGuiApplication app(argc, argv);

    qmlRegisterType<TextRender>("FingerTerm", 1, 0, "TextRender");
    qmlRegisterUncreatableType<Util>("FingerTerm", 1, 0, "Util", "Util is created by app");
    QQmlApplicationEngine engine;

    Terminal term;
    Util util(settings); // takes ownership
    term.setUtil(&util);
    TextRender::setUtil(&util);
    TextRender::setTerminal(&term);

    QString startupErrorMsg;

    KeyLoader keyLoader;
    keyLoader.setUtil(&util);
    bool ret = keyLoader.loadLayout(util.keyboardLayout());
    if(!ret) {
        // on failure, try to load the default one (english) directly from resources
        startupErrorMsg = "There was an error loading the keyboard layout.<br>\nUsing the default one instead.";
        util.setKeyboardLayout("english");
        ret = keyLoader.loadLayout(":/data/english.layout");
        if(!ret)
            qFatal("failure loading keyboard layout");
    }

    engine.rootContext()->setContextProperty( "term", &term );
    engine.rootContext()->setContextProperty( "util", &util );
    engine.rootContext()->setContextProperty( "keyLoader", &keyLoader );
    engine.rootContext()->setContextProperty( "startupErrorMessage", startupErrorMsg);
    engine.load(QUrl("qrc:/qml/Main.qml"));

    QQuickView *view = (QQuickView*) engine.rootObjects().first();
    term.setWindow(view);
    util.setWindow(view);
    util.setTerm(&term);

    if(engine.rootObjects().isEmpty())
        qFatal("no root object - qml error");

    PtyIFace ptyiface(pid, socketM, &term, util.charset());

    if( ptyiface.failed() )
        qFatal("pty failure");

    return app.exec();
}
