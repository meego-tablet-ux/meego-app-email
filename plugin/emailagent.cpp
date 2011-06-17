/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */


#include "emailagent.h"
#include <QDir>
#include <QUrl>
#include <QFile>
#include <QProcess>


EmailAgent *EmailAgent::m_instance = 0;

EmailAgent *EmailAgent::instance()
{
    if (!m_instance)
        m_instance = new EmailAgent();
    return m_instance;
}

EmailAgent::EmailAgent(QDeclarativeItem *parent)
    : QDeclarativeItem(parent), 
    m_confirmDeleteMail(new MGConfItem("/apps/meego-app-email/confirmdeletemail"))
{
    m_instance = this;
}


EmailAgent::~EmailAgent()
{
    delete m_confirmDeleteMail;
}

QString EmailAgent::getSignatureForAccount(QVariant vMailAccountId)
{
    //TODO:  Need to implement this using libcamel api.
    return (QString (""));
}

bool EmailAgent::confirmDeleteMail()
{
    return m_confirmDeleteMail->value().toBool();
}

bool EmailAgent::openAttachment(const QString & uri)
{
    bool status = true;
    
    // let's determine the file type
    QString filePath = QDir::homePath() + "/Downloads/" + uri;

    QProcess fileProcess;
    fileProcess.start("file", QStringList() << "-b" << filePath);
    if (!fileProcess.waitForFinished())
        return false;

    QString s(fileProcess.readAll());
    QStringList parameters;
    parameters << "--opengl" << "--fullscreen";
    if (s.contains("video", Qt::CaseInsensitive))
    {
        parameters << "--app" << "meego-app-video";
        parameters << "--cmd" << "playVideo";
    }
    else if (s.contains("image", Qt::CaseInsensitive))
    {
        parameters << "--app" << "meego-app-photos";
        parameters << "--cmd" << "showPhoto";
    }
    else if (s.contains("audio", Qt::CaseInsensitive))
    {
        parameters << "--app" << "meego-app-music";
        parameters << "--cmd" << "playSong";
    }
    else if (s.contains("Ogg data", Qt::CaseInsensitive))
    {
        // Fix Me:  need more research on Ogg format. For now, default to video.
        parameters << "--app" << "meego-app-video";
        parameters << "--cmd" << "video";
    }
    else
    {
        // Unsupported file type.
        return false;
    }

    QString executable("meego-qml-launcher");
    filePath.prepend("file://");
    parameters << "--cdata" << filePath;
    QProcess::startDetached(executable, parameters);

    return status;
}

void EmailAgent::openBrowser(const QString & url)
{
    QString executable("xdg-open");
    QStringList parameters;
    parameters << url;
    QProcess::startDetached(executable, parameters);
}


QString EmailAgent::getMessageBodyFromFile (const QString& bodyFilePath)
{
    QFile f(bodyFilePath);
    if(!f.open(QFile::ReadOnly))
        return ("");

    QString data = f.readAll();
    return data;
}
