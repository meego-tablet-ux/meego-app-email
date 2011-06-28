/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <QtNetwork/QNetworkConfigurationManager>
#include <QTimer>

#include "emailaccount.h"

#include <gconf/gconf-client.h>
#include "e-gdbus-emailstore-proxy.h"

#define CAMEL_COMPILATION 1
#include <camel/camel-url.h>

EmailAccount::EmailAccount()
:   mErrorCode(0),
    mUpdateIntervalConf(new MGConfItem("/apps/meego-app-email/updateinterval")),
    mSignatureConf(new MGConfItem("/apps/meego-app-email/signature"))
{
    mAccount = NULL;
    GConfClient *client = gconf_client_get_default();
    mAccountList = e_account_list_new(client);
    g_object_unref(client);

    init();
}

EmailAccount::~EmailAccount()
{
    g_object_unref(mAccount);
    g_object_unref(mAccountList);
    delete session;
}

void EmailAccount::init()
{
    mPreset = 0;
    mRecvType = 0;
    mRecvServer.clear();
    mRecvPort = 0;
    mRecvSecurity = 0;
    mRecvUsername.clear();
    mRecvPassword.clear();
    mSendServer.clear();
    mSendPort = 0;
    mSendSecurity = 0;
    mSendAuth = 0;
    mSendUsername.clear();
    mSendPassword.clear();
    mPassword.clear();

    EAccount *acc = e_account_new();
    if (mAccount) {
        g_free (acc->uid);
        acc->uid = g_strdup(mAccount->uid);
        g_object_unref(mAccount);
    }
    mAccount = acc;

    mModified = true;
}

void EmailAccount::clear()
{
    init();
}

bool EmailAccount::save()
{
    session = new OrgGnomeEvolutionDataserverMailSessionInterface("org.gnome.evolution.dataserver.Mail",
                                                                  "/org/gnome/evolution/dataserver/Mail/Session",
                                                                  QDBusConnection::sessionBus(), this);

    QString descr = e_account_get_string(mAccount, E_ACCOUNT_NAME);
    QString newDescr = name() + " - " + description();
    if (descr != newDescr) {
        e_account_set_string(mAccount, E_ACCOUNT_NAME, newDescr.toUtf8());
        mModified = true;
    }

    QString sig = e_account_get_string(mAccount, E_ACCOUNT_ID_SIGNATURE);
    QString newSig = mSignatureConf->value().toString();
    if (sig != newSig) {
        e_account_set_string(mAccount, E_ACCOUNT_ID_SIGNATURE, newSig.toUtf8());
        mModified = true;
    }

    bool autoCheck =  e_account_get_bool(mAccount, E_ACCOUNT_SOURCE_AUTO_CHECK);
    bool newAutoCheck = true;
    if (autoCheck != newAutoCheck) {
        e_account_set_bool(mAccount, E_ACCOUNT_SOURCE_AUTO_CHECK, newAutoCheck);
        mModified = true;
    }

    int update = e_account_get_int(mAccount, E_ACCOUNT_SOURCE_AUTO_CHECK_TIME);
    int newUpdate = mUpdateIntervalConf->value().toInt();
    if (update != newUpdate) {
        e_account_set_int(mAccount, E_ACCOUNT_SOURCE_AUTO_CHECK_TIME, newUpdate);
        mModified = true;
    }

    QString recvSecurStr;
    switch (recvSecurity().toInt()) {
        case 0: recvSecurStr = "never"; break;
        case 1: recvSecurStr = "always"; break;
        case 2: recvSecurStr = "when-possible";
    }

    QString sourceUrl;
    if (recvType() == "0") {
        QString draft = e_account_get_string(mAccount, E_ACCOUNT_DRAFTS_FOLDER_URI);
        QString newDraft = "mbox:/home/meego/.local/share/evolution/mail/local#Drafts";
        if (draft != newDraft) {
            e_account_set_string(mAccount, E_ACCOUNT_DRAFTS_FOLDER_URI, newDraft.toUtf8());
            mModified = true;
        }

        QString sent = e_account_get_string(mAccount, E_ACCOUNT_DRAFTS_FOLDER_URI);
        QString newSent = "mbox:/home/meego/.local/share/evolution/mail/local#Sent";
        if (sent != newSent) {
            e_account_set_string(mAccount, E_ACCOUNT_SENT_FOLDER_URI, newSent.toUtf8());
            mModified = true;
        }
	
	if (mModified)
	        e_account_set_bool(mAccount, E_ACCOUNT_SOURCE_KEEP_ON_SERVER, TRUE);

        sourceUrl = "pop://" + recvUsername().replace("@", "%40") + "@" + recvServer() + ":" + recvPort() + "/;" +
                    "keep_on_server=true;mobile;disable_autofetch;" +
                    "command=ssh%20-C%20-l%20%25u%20%25h%20exec%20/usr/sbin/dovecot%20--exec-mail%20imap;";
    } else if (recvType() == "1") {
        sourceUrl = "imapx://" + recvUsername().replace("@", "%40") + "@" + recvServer() + ":" + recvPort() + "/;" +
                    "mobile;fetch-order=descending;" +
                    "command=ssh%20-C%20-l%20%25u%20%25h%20exec%20/usr/sbin/dovecot%20--exec-mail%20imap;";
    }
    sourceUrl.append("use_ssl=" + recvSecurStr + ";use_lsub;cachedconn=5;check_all=1;use_idle;use_qresync;sync_offline=1");

    QString sourceUrlOld = e_account_get_string(mAccount, E_ACCOUNT_SOURCE_URL);
    if (sourceUrl != sourceUrlOld) {
        e_account_set_string(mAccount, E_ACCOUNT_SOURCE_URL, sourceUrl.toUtf8());
        mModified = true;
    }

    CamelURL *sourceCamelUrl = camel_url_new(sourceUrl.toUtf8(), NULL);
    char *sourceKey = camel_url_to_string(sourceCamelUrl, CAMEL_URL_HIDE_PASSWORD | CAMEL_URL_HIDE_PARAMS);

    session->addPassword(sourceKey, recvPassword(), TRUE);
    camel_url_free(sourceCamelUrl);
    g_free(sourceKey);

    QString sendSecurStr;
    switch (sendSecurity().toInt()) {
        case 0: sendSecurStr = "never"; break;
        case 1: sendSecurStr = "always"; break;
        case 2: sendSecurStr = "when-possible";
    }
    QString transportUrl = "smtp://" + sendUsername().replace("@", "%40") + ";";
    if (sendAuth() != 0) {
        QString authType;
        switch (sendAuth().toInt()) {
            case 1: authType = "LOGIN"; break;
            case 2: authType = "PLAIN"; break;
            case 3: authType = "CRAM-MD5";
        }
        transportUrl.append("auth=" + authType + "@" + sendServer() + ":" + sendPort() + "/;");
    }
    transportUrl.append("use_ssl=" + sendSecurStr);

    QString transportUrlOld = e_account_get_string(mAccount, E_ACCOUNT_TRANSPORT_URL);
    if (transportUrl != transportUrlOld) {
        e_account_set_string(mAccount, E_ACCOUNT_TRANSPORT_URL, transportUrl.toUtf8());
        mModified = true;
    }

    CamelURL *transportCamelUrl = camel_url_new(transportUrl.toUtf8(), NULL);
    char *transportKey = camel_url_to_string(transportCamelUrl, CAMEL_URL_HIDE_PASSWORD | CAMEL_URL_HIDE_PARAMS);
    session->addPassword(transportKey, sendPassword(), TRUE);
    camel_url_free(transportCamelUrl);
    g_free(transportKey);

    setEnabled(true);

    bool found = false;
    EIterator *iter = e_list_get_iterator(E_LIST(mAccountList));
    while (e_iterator_is_valid(iter)) {
        EAccount *acc = (EAccount *)e_iterator_get(iter);
        if (strcmp(mAccount->uid, acc->uid) == 0) {
            found = true;
            if (acc != mAccount)
                e_account_import(acc, mAccount);
            acc->enabled = TRUE;
            e_account_list_change(mAccountList, acc);
            break;
        }
        e_iterator_next(iter);
    }
    g_object_unref(iter);
    if (!found) {
        e_account_list_add(mAccountList, mAccount);
        mModified = true;
    }

    if (mModified) {
        e_account_list_save(mAccountList);
        mModified = false;
    }

    return true;
}

bool EmailAccount::remove()
{
    bool result = false;
    EIterator *iter = e_list_get_iterator(E_LIST(mAccountList));
    while (e_iterator_is_valid(iter)) {
        EAccount *acc = (EAccount *)e_iterator_get(iter);
        if (strcmp(mAccount->uid, acc->uid) == 0) {
            e_account_list_remove(mAccountList, acc);
            e_account_list_save(mAccountList);
            result = true;
            break;
        }
        e_iterator_next(iter);
    }
    g_object_unref(iter);

    return result;
}

void EmailAccount::test()
{
    QNetworkConfigurationManager networkManager;
    if (networkManager.isOnline()) {
        QTimer::singleShot(5000, this, SLOT(testConfiguration()));
    } else {
        // skip test if not connected to a network
        emit testSucceeded();
    }
}

void EmailAccount::testConfiguration()
{
    QString sourceUrl = e_account_get_string(mAccount, E_ACCOUNT_SOURCE_URL);
    if (sourceUrl.isEmpty()) {
        emit testFailed();
        return;
    }

    if (sourceUrl.startsWith("pop:")) {
        emit testSucceeded();
        return;
    }

    QDBusPendingReply<QDBusObjectPath> storeReply = session->getStore(sourceUrl);
    storeReply.waitForFinished();
    QDBusObjectPath store_id = storeReply.value();

    OrgGnomeEvolutionDataserverMailStoreInterface *proxy;
    proxy = new OrgGnomeEvolutionDataserverMailStoreInterface(QString("org.gnome.evolution.dataserver.Mail"),
                                                              store_id.path(), QDBusConnection::sessionBus(), this);
    QDBusPendingReply<CamelFolderInfoArrayVariant> infoArrayReply;
    infoArrayReply = proxy->getFolderInfo("",
                                          CAMEL_STORE_FOLDER_INFO_RECURSIVE |
                                          CAMEL_STORE_FOLDER_INFO_FAST |
                                          CAMEL_STORE_FOLDER_INFO_SUBSCRIBED);
    infoArrayReply.waitForFinished();
    CamelFolderInfoArrayVariant infoArray = infoArrayReply.value();

    if (infoArray.length() == 0) {
        emit testFailed();
        return;
    }

    delete proxy;

    QString draftUri;
    QString sentUri;
    foreach (CamelFolderInfoVariant info, infoArray) {
        if (info.folder_name.contains("draft", Qt::CaseInsensitive))
            draftUri = info.uri;
        if (info.folder_name.contains("sent", Qt::CaseInsensitive))
            sentUri = info.uri;
    }

    QString draftUriOld = e_account_get_string(mAccount, E_ACCOUNT_DRAFTS_FOLDER_URI);
    if (!draftUri.isEmpty()) {
        if (draftUriOld != draftUri) {
            mModified = true;
            e_account_set_string(mAccount, E_ACCOUNT_DRAFTS_FOLDER_URI, draftUri.toUtf8());
        }
    } else {
        QString draftUriAlt = "mbox:/home/meego/.local/share/evolution/mail/local#Drafts";
        if (draftUriOld != draftUriAlt) {
            mModified = true;
            e_account_set_string(mAccount, E_ACCOUNT_DRAFTS_FOLDER_URI, draftUriAlt.toUtf8());
        }
    }

    QString sentUriOld = e_account_get_string(mAccount, E_ACCOUNT_SENT_FOLDER_URI);
    if (!sentUri.isEmpty()) {
        if (sentUriOld != sentUri) {
            mModified = true;
            e_account_set_string(mAccount, E_ACCOUNT_SENT_FOLDER_URI, sentUri.toUtf8());
        }
    } else {
        QString sentUriAlt = "mbox:/home/meego/.local/share/evolution/mail/local#Sent";
        if (sentUriOld != sentUriAlt) {
            mModified = true;
            e_account_set_string(mAccount, E_ACCOUNT_SENT_FOLDER_URI, sentUriAlt.toUtf8());
        }
    }

    bool found = false;
    EIterator *iter = e_list_get_iterator(E_LIST(mAccountList));
    while (e_iterator_is_valid(iter) && mModified) {
        EAccount *acc = (EAccount *)e_iterator_get(iter);
        if (strcmp(mAccount->uid, acc->uid) == 0) {
            found = true;
            qDebug() << e_account_to_xml(acc);
            if (acc != mAccount)
                e_account_import(acc, mAccount);
            e_account_list_change(mAccountList, acc);
            break;
        }
        e_iterator_next(iter);
    }
    g_object_unref(iter);
    if (!found) {
        emit testFailed();
        return;
    }

    if (mModified) {
        e_account_list_save(mAccountList);
        mModified = false;
    }

    emit testSucceeded();
}

void EmailAccount::applyPreset()
{
    switch(preset()) {
        case mobilemePreset:
            setRecvType("1");                    // imap
            setRecvServer("mail.me.com");
            setRecvPort("993");
            setRecvSecurity("1");                // SSL
            setRecvUsername(username());         // username only
            setRecvPassword(password());
            setSendServer("smtp.me.com");
            setSendPort("587");
            setSendSecurity("1");                // SSL
            setSendAuth("1");                    // Login
            setSendUsername(username());         // username only
            setSendPassword(password());
            break;
        case gmailPreset:
            setRecvType("1");                    // imap
            setRecvServer("imap.googlemail.com");
            setRecvPort("993");
            setRecvSecurity("1");                // SSL
            setRecvUsername(address());          // full email address
            setRecvPassword(password());
            setSendServer("smtp.googlemail.com");
            setSendPort("465");
            setSendSecurity("1");                // SSL
            setSendAuth("1");                    // Login
            setSendUsername(address());          // full email address
            setSendPassword(password());
            break;
        case yahooPreset:
            setRecvType("1");                    // imap
            setRecvServer("imap.mail.yahoo.com");
            setRecvPort("993");
            setRecvSecurity("1");                // SSL
            setRecvUsername(address());          // full email address
            setRecvPassword(password());
            setSendServer("smtp.mail.yahoo.com");
            setSendPort("465");
            setSendSecurity("1");                // SSL
            setSendAuth("1");                    // Login
            setSendUsername(address());          // full email address
            setSendPassword(password());
            break;
        case aolPreset:
            setRecvType("1");                    // imap
            setRecvServer("imap.aol.com");
            setRecvPort("143");
            setRecvSecurity("0");                // none
            setRecvUsername(username());         // username only
            setRecvPassword(password());
            setSendServer("smtp.aol.com");
            setSendPort("587");
            setSendSecurity("0");                // none
            setSendAuth("1");                    // Login
            setSendUsername(username());         // username only
            setSendPassword(password());
            break;
        case mslivePreset:
            setRecvType("0");                    // pop
            setRecvServer("pop3.live.com");
            setRecvPort("995");
            setRecvSecurity("1");                // SSL
            setRecvUsername(address());          // full email address
            setRecvPassword(password());
            setSendServer("smtp.live.com");
            setSendPort("587");
            setSendSecurity("2");                // TLS
            setSendAuth("1");                    // Login
            setSendUsername(address());          // full email address
            setSendPassword(password());
            break;
        case noPreset:
            setRecvType("1");                    // imap
            setRecvPort("993");
            setRecvSecurity("1");                // SSL
            setRecvUsername(username());         // username only
            setRecvPassword(password());
            setSendPort("587");
            setSendSecurity("1");                // SSL
            setSendAuth("1");                    // Login
            setSendUsername(username());         // username only
            setSendPassword(password());
            break;
        default:
            break;
    }
}

QString EmailAccount::description() const
{
    return mDescription;
}

void EmailAccount::setDescription(QString val)
{
    mDescription = val;
}

bool EmailAccount::enabled() const
{
    return mAccount->enabled;
}

void EmailAccount::setEnabled(bool val)
{
    mAccount->enabled = val;
}

QString EmailAccount::name() const
{
    return QString::fromUtf8(e_account_get_string(mAccount, E_ACCOUNT_ID_NAME));
}

void EmailAccount::setName(QString val)
{
    if (val != name()) {
        e_account_set_string(mAccount, E_ACCOUNT_ID_NAME, val.toUtf8());
        mModified = true;
    }
}

QString EmailAccount::address() const
{
    return e_account_get_string(mAccount, E_ACCOUNT_ID_ADDRESS);
}

void EmailAccount::setAddress(QString val)
{
    if (val != address()) {
        e_account_set_string(mAccount, E_ACCOUNT_ID_ADDRESS, val.toAscii());
        mModified = true;
    }
}

QString EmailAccount::username() const
{
    return address().remove(QRegExp("@.*$"));
}

QString EmailAccount::server() const
{
    return address().remove(QRegExp("^.*@"));
}

QString EmailAccount::password() const
{
    return mPassword;
}

void EmailAccount::setPassword(QString val)
{
    mPassword = val;
}

QString EmailAccount::recvType() const
{
    // 0 == pop3, 1 == imap4
    return QString::number(mRecvType);
}

void EmailAccount::setRecvType(QString val)
{
    int newRecvType = val.toInt();
    if (newRecvType != mRecvType) {
        mRecvType = newRecvType;
    }
}

QString EmailAccount::recvServer() const
{
    return mRecvServer;
}

void EmailAccount::setRecvServer(QString val)
{
    mRecvServer = val;
}

QString EmailAccount::recvPort() const
{
    return QString::number(mRecvPort);
}

void EmailAccount::setRecvPort(QString val)
{
    mRecvPort = val.toInt();
}

QString EmailAccount::recvSecurity() const
{
    return QString::number(mRecvSecurity);
}

void EmailAccount::setRecvSecurity(QString val)
{
    mRecvSecurity = val.toInt();
}

QString EmailAccount::recvUsername() const
{
    return mRecvUsername;
}

void EmailAccount::setRecvUsername(QString val)
{
    mRecvUsername = val;
}

QString EmailAccount::recvPassword() const
{
    return mRecvPassword;
}

void EmailAccount::setRecvPassword(QString val)
{
    mRecvPassword = val;
}

QString EmailAccount::sendServer() const
{
    return mSendServer;
}

void EmailAccount::setSendServer(QString val)
{
    mSendServer = val;
}

QString EmailAccount::sendPort() const
{
    return QString::number(mSendPort);
}

void EmailAccount::setSendPort(QString val)
{
    mSendPort = val.toInt();
}

QString EmailAccount::sendAuth() const
{
    return QString::number(mSendAuth);
}

void EmailAccount::setSendAuth(QString val)
{
    mSendAuth = val.toInt();
}

QString EmailAccount::sendSecurity() const
{
    return QString::number(mSendSecurity);
}

void EmailAccount::setSendSecurity(QString val)
{
    mSendSecurity = val.toInt();
}

QString EmailAccount::sendUsername() const
{
    return mSendUsername;
}

void EmailAccount::setSendUsername(QString val)
{
    mSendUsername = val;
}

QString EmailAccount::sendPassword() const
{
    return mSendPassword;
}

void EmailAccount::setSendPassword(QString val)
{
    mSendPassword = val;
}

int EmailAccount::preset() const
{
    return mPreset;
}

void EmailAccount::setPreset(int val)
{
    mPreset = val;
}

QString EmailAccount::errorMessage() const
{
    return mErrorMessage;
}

int EmailAccount::errorCode() const
{
    return mErrorCode;
}


