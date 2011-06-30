/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "emailaccountsettingsmodel.h"

#include <gconf/gconf-client.h>
#include "e-gdbus-emailsession-proxy.h"

#define CAMEL_COMPILATION 1
#include <camel/camel-url.h>

#include <QDebug>

static void
account_changed (EAccountList *eal, EAccount *ea, gpointer dummy)
{
    // TODO: need to find out what this routine suppose to do
    Q_UNUSED(dummy);
    EIterator *iter = e_list_get_iterator(E_LIST(eal));
    while (e_iterator_is_valid(iter)) {
        EAccount *acc = (EAccount *)e_iterator_get(iter);
        //qDebug() << e_account_to_xml(acc);
        e_iterator_next(iter);
    }

}

EmailAccountSettingsModel::EmailAccountSettingsModel(QObject *parent)
    : QAbstractListModel(parent)
{
    roles[DescriptionRole] = "description";
    roles[EnabledRole] = "enabled";
    roles[NameRole] = "name";
    roles[AddressRole] = "address";
    roles[PasswordRole] = "password";

    roles[RecvTypeRole] = "recvType";
    roles[RecvServerRole] = "recvServer";
    roles[RecvPortRole] = "recvPort";
    roles[RecvSecurityRole] = "recvSecurity";
    roles[RecvUsernameRole] = "recvUsername";
    roles[RecvPasswordRole] = "recvPassword";

    roles[SendServerRole] = "sendServer";
    roles[SendPortRole] = "sendPort";
    roles[SendAuthRole] = "sendAuth";
    roles[SendSecurityRole] = "sendSecurity";
    roles[SendUsernameRole] = "sendUsername";
    roles[SendPasswordRole] = "sendPassword";

    roles[PresetRole] = "preset";
    setRoleNames(roles);

    mUpdateIntervalConf = new MGConfItem("/apps/meego-app-email/updateinterval");
    mSignatureConf = new MGConfItem("/apps/meego-app-email/signature");
    mNewMailNotificationConf = new MGConfItem("/apps/meego-app-email/newmailnotifications");
    mConfirmDeleteMailConf = new MGConfItem("/apps/meego-app-email/confirmdeletemail");

    /* Init the Glib system */
    g_type_init ();

    GConfClient *client = gconf_client_get_default();
    mAccountList = e_account_list_new(client);
    g_signal_connect (mAccountList, "account-changed", G_CALLBACK(account_changed), mAccountList);
    g_object_unref(client);
    session = new OrgGnomeEvolutionDataserverMailSessionInterface("org.gnome.evolution.dataserver.Mail",
                                                                       "/org/gnome/evolution/dataserver/Mail/Session",
                                                                       QDBusConnection::sessionBus(), this);

    init();
}

EmailAccountSettingsModel::~EmailAccountSettingsModel()
{
    delete session;
    g_object_unref(mAccountList);
}

EAccount * EmailAccountSettingsModel::getAccountByIndex(int idx) const
{
    EIterator *iter;
    int i=-1;
    EAccount *account = NULL;

    iter = e_list_get_iterator (E_LIST (mAccountList));
    for (; e_iterator_is_valid (iter); e_iterator_next (iter)) {

	account = (EAccount *) e_iterator_get (iter);
	        i++;
	
	if (i == idx)
		break;
    }

    if (!e_iterator_is_valid (iter))
	account = NULL;
	
    g_object_unref (iter);

    return account;
}

void EmailAccountSettingsModel::init()
{
    EIterator *iter;
    int i=0;

    iter = e_list_get_iterator (E_LIST (mAccountList));
    while (e_iterator_is_valid (iter)) {
	EAccount *acc;

	acc = (EAccount *) e_iterator_get (iter);
	
	i++;

        e_iterator_next (iter);
	
    }

    mLength = i;
    // initialize global settings from gconf
    mUpdateInterval = mUpdateIntervalConf->value().toInt();
    mSignature = mSignatureConf->value().toString();
    mNewMailNotification = mNewMailNotificationConf->value().toBool();
    mConfirmDeleteMail = mConfirmDeleteMailConf->value().toBool();
}

void EmailAccountSettingsModel::reload()
{
    beginResetModel();
    init();
    endResetModel();
}

int EmailAccountSettingsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return mLength;
}

QVariant EmailAccountSettingsModel::data(const QModelIndex &index, int role) const
{
    QVariant result;
    if (index.isValid() && index.row() < rowCount()) {
        EAccount *acc = getAccountByIndex(index.row());
        QString sourceUrl = e_account_get_string(acc, E_ACCOUNT_SOURCE_URL);
        CamelURL *sourceCamelUrl = camel_url_new(sourceUrl.toUtf8(), NULL);
        int recvType;
        QString protocol = sourceCamelUrl->protocol;
        if (protocol == "pop") {
            recvType = 0;
        } else if (protocol == "imapx") {
            recvType = 1;
        } else {
            qWarning("EmailAccountSettingsModel::data: No IMAP or POP service found for account");
            camel_url_free(sourceCamelUrl);
            return QVariant();
        }

        QString transportUrl = e_account_get_string(acc, E_ACCOUNT_TRANSPORT_URL);
        CamelURL *transportCamelUrl = camel_url_new(transportUrl.toUtf8(), NULL);

        switch (role) {
            case DescriptionRole:
                result = e_account_get_string(acc, E_ACCOUNT_NAME);
                break;
            case EnabledRole:
                result = acc->enabled;
                break;
            case NameRole:
                result = e_account_get_string(acc, E_ACCOUNT_ID_NAME);
                break;
            case AddressRole:
                result = e_account_get_string(acc, E_ACCOUNT_ID_ADDRESS);
                break;
            case PasswordRole: {
               char *key = camel_url_to_string(sourceCamelUrl, CAMEL_URL_HIDE_PASSWORD | CAMEL_URL_HIDE_PARAMS);
            
                QString recvpass = session->findPassword (QString(key));
                g_free (key);
                key = camel_url_to_string(transportCamelUrl, CAMEL_URL_HIDE_PASSWORD | CAMEL_URL_HIDE_PARAMS);
                QString sendpass = session->findPassword (QString(key));
                g_free (key);
                 
                if (recvpass == sendpass)
                    result = recvpass;

                break;
            }
            case RecvTypeRole:
                result = recvType;
                break;
            case RecvServerRole:
                result = sourceCamelUrl->host;
                break;
            case RecvPortRole:
                result = sourceCamelUrl->port;
                break;
            case RecvSecurityRole: {
                QString recvSecurity = camel_url_get_param(sourceCamelUrl, "use_ssl");
                if (recvSecurity == "never")
                    result = 0;
                else if (recvSecurity == "always")
                    result = 1;
                else if (recvSecurity == "when-possible")
                    result = 2;
                break;
            }
            case RecvUsernameRole:
                result = sourceCamelUrl->user;
                break;
            case RecvPasswordRole: {
                char *key = camel_url_to_string(sourceCamelUrl, CAMEL_URL_HIDE_PASSWORD | CAMEL_URL_HIDE_PARAMS);
                result = QVariant(session->findPassword (QString(key)));
                g_free (key);
                break;
            }
            case SendServerRole:
                result = transportCamelUrl->host;
                break;
            case SendPortRole:
                result = transportCamelUrl->port;
                break;
            case SendAuthRole: {
                QString sendAuth = camel_url_get_param(transportCamelUrl, "auth");
                if (sendAuth == "LOGIN")
                    result = 1;
                else if (sendAuth == "PLAIN")
                    result = 2;
                else if (sendAuth == "CRAM-MD5")
                    result = 3;
                else
                    result = 0;
                break;
            }
            case SendSecurityRole: {
                QString recvSecurity = camel_url_get_param(transportCamelUrl, "use_ssl");
                if (recvSecurity == "never")
                    result = 0;
                else if (recvSecurity == "always")
                    result = 1;
                else if (recvSecurity == "when-possible")
                    result = 2;
                break;
            }
            case SendUsernameRole:
                result = transportCamelUrl->user;
                break;
            case SendPasswordRole: {
                char *key = camel_url_to_string(transportCamelUrl, CAMEL_URL_HIDE_PASSWORD | CAMEL_URL_HIDE_PARAMS);
                result = QVariant(session->findPassword (QString(key)));
                g_free (key);
                break;
            }
            case PresetRole:
                // FIXME is it better to rely on CamelUrl->host?
                QString domain = e_account_get_string(acc, E_ACCOUNT_ID_ADDRESS);
                domain.remove(QRegExp("^.*@"));
                if (domain == "me.com")
                    result = 1;
                else if (domain == "gmail.com" || domain == "googlemail.com")
                    result = 2;
                else if (domain == "yahoo.com")
                    result = 3;
                else if (domain == "aol.com")
                    result = 4;
                else if (domain == "live.com")
                    result = 5;
                else
                    result = 0;
                break;
        }
        camel_url_free(transportCamelUrl);
        camel_url_free(sourceCamelUrl);
    }

    return result;
}

bool EmailAccountSettingsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    bool modified = false;
    bool result = false;
    if (index.isValid() && index.row() < rowCount()) {
        EAccount *acc = getAccountByIndex(index.row());
        QString sourceUrl = e_account_get_string(acc, E_ACCOUNT_SOURCE_URL);
        CamelURL *sourceCamelUrl = camel_url_new(sourceUrl.toUtf8(), NULL);
        int recvType;
        QString protocol = sourceCamelUrl->protocol;
        if (protocol == "pop") {
            recvType = 0;
        } else if (protocol == "imapx") {
            recvType = 1;
        } else {
            qWarning("EmailAccountSettingsModel::data: No IMAP or POP service found for account");
            camel_url_free(sourceCamelUrl);
            return result;
        }

        QString transportUrl = e_account_get_string(acc, E_ACCOUNT_TRANSPORT_URL);
        CamelURL *transportCamelUrl = camel_url_new(transportUrl.toUtf8(), NULL);

        switch (role) {
            case DescriptionRole: {
                QString descr = e_account_get_string(acc, E_ACCOUNT_NAME);
                QString newDescr = value.toString();
                if (descr != newDescr) {
                    e_account_set_string(acc, E_ACCOUNT_NAME, newDescr.toUtf8());
                    modified = true;
                }
                result = true;
                break;
            }
            case EnabledRole:
                if (acc->enabled != value.toBool()) {
                    acc->enabled = value.toBool();
                    modified = true;
                }
                result = true;
                break;
            case NameRole: {
                QString name = e_account_get_string(acc, E_ACCOUNT_ID_NAME);
                QString newName = value.toString();
                if (name != newName) {
                    e_account_set_string(acc, E_ACCOUNT_ID_NAME, newName.toUtf8());
                    modified = true;
                }
                result = true;
                break;
            }
            case AddressRole: {
                QString address = e_account_get_string(acc, E_ACCOUNT_ID_ADDRESS);
                QString newAddress = value.toString();
                if (address != newAddress) {
                    e_account_set_string(acc, E_ACCOUNT_ID_ADDRESS, newAddress.toAscii());
                    modified = true;
                }
                result = true;
                break;
            }
            case PasswordRole:
                if (!value.toString().isEmpty()) {
                    // FIXME check if passsword changed
                    char *skey = camel_url_to_string(sourceCamelUrl, CAMEL_URL_HIDE_PASSWORD | CAMEL_URL_HIDE_PARAMS);
                    char *tkey = camel_url_to_string(transportCamelUrl, CAMEL_URL_HIDE_PASSWORD | CAMEL_URL_HIDE_PARAMS);

                    QString recvpass = session->findPassword (QString(skey));
                    QString sendpass = session->findPassword (QString(tkey));

                    if (recvpass != value.toString())
			session->addPassword (skey, value.toString(), TRUE);
                    if (sendpass != value.toString())
			session->addPassword (tkey, value.toString(), TRUE);

                    g_free (skey);
                    g_free (tkey);

                }
                result = true;
                break;
            case RecvTypeRole: {
                int newRecvType = value.toInt();
                if (newRecvType != 0 && newRecvType != 1) {
                    result = false;
                } else if (newRecvType == recvType) {
                    result = true;
                } else {
                    camel_url_set_protocol(sourceCamelUrl, newRecvType == 0 ? "pop" : "imapx");
                    // FIXME what about 'keep_on_server', 'disable_autofetch', 'fetch-order'?
                    // automatically clear the recv fields in the UI
                    emit dataChanged(index, index);
                    result = true;
                }
                break;
            }
            case RecvServerRole: {
                QString name = sourceCamelUrl->host;
                if (name != value.toString()) {
                    camel_url_set_host(sourceCamelUrl, value.toString().toUtf8());
                    result = true;
                    modified = true;
                }
                break;}
            case RecvPortRole:
                if (value.toInt() != sourceCamelUrl->port) {
                    camel_url_set_port(sourceCamelUrl, value.toInt());
                    result = true;
                    modified = true;
                }
                break;
            case RecvSecurityRole: {
                QString encryption;
                switch (value.toInt()) {
                    case 0: encryption = "never";
                    case 1: encryption = "always";
                    case 2: encryption = "when-possible";
                }
                QString name = camel_url_get_param (sourceCamelUrl, "use_ssl");
                if (name != encryption) {
                    camel_url_set_param(sourceCamelUrl, "use_ssl", encryption.toUtf8());
                    result = true;
                    modified = true;
                }
                break;
            }
            case RecvUsernameRole: {
                QString name = sourceCamelUrl->user;
                if (name != value.toString()) {
                    camel_url_set_user(sourceCamelUrl, value.toString().toUtf8());
                    result = true;
                    modified = true;
                }
                break;
            }
            case RecvPasswordRole: {
                char *skey = camel_url_to_string(sourceCamelUrl, CAMEL_URL_HIDE_PASSWORD | CAMEL_URL_HIDE_PARAMS);
                QString recvpass = session->findPassword (QString(skey));

                if (recvpass != value.toString())
                    session->addPassword (skey, value.toString(), TRUE);
                g_free (skey);

                result = true;
                break;
            }
            case SendServerRole: {
                QString name = transportCamelUrl->host;
                if (name != value.toString()) {
                    camel_url_set_host(transportCamelUrl, value.toString().toUtf8());
                    result = true;
                    modified = true;
                }
                break;
            }
            case SendPortRole:
                if (transportCamelUrl->port != value.toInt()) {
                    camel_url_set_port(transportCamelUrl, value.toInt());
                    result = true;
                    modified = true;
                }
                break;
            case SendAuthRole: {
                QString authentication;
                switch (value.toInt()) {
                    case 0: authentication = "NONE";
                    case 1: authentication = "LOGIN";
                    case 2: authentication = "PLAIN";
                    case 3: authentication = "CRAM-MD5";
                }
                QString name = camel_url_get_param(transportCamelUrl, "auth");
                if (name != authentication) {
                    camel_url_set_param(transportCamelUrl, "auth", authentication.toUtf8());
                    result = true;
                    modified = true;
                }
                break;
            }
            case SendSecurityRole: {
                QString encryption;
                switch (value.toInt()) {
                    case 0: encryption = "never";
                    case 1: encryption = "always";
                    case 2: encryption = "when-possible";
                }
                QString name = camel_url_get_param(transportCamelUrl, "use_ssl");
                if (name != encryption) {
                    camel_url_set_param(transportCamelUrl, "use_ssl", encryption.toUtf8());
                    result = true;
                    modified = true;
                }
                break;
            }
            case SendUsernameRole:{
                QString name = transportCamelUrl->user;
                if (name != value.toString()) {
                    camel_url_set_user(transportCamelUrl, value.toString().toUtf8());
                    result = true;
                    modified = true;
                }       
                break;
            }
            case SendPasswordRole: {
                char *tkey = camel_url_to_string(transportCamelUrl, CAMEL_URL_HIDE_PASSWORD | CAMEL_URL_HIDE_PARAMS);

                QString sendpass = session->findPassword (QString(tkey));

                if (sendpass != value.toString())
                    session->addPassword (tkey, value.toString(), TRUE);

                g_free (tkey);

                result = true;
                break;
            }
            case PresetRole:
                // setting preset not implemented here
            default:
                result = false;
        }
        QString newSourseUrl = camel_url_to_string(sourceCamelUrl, CAMEL_URL_HIDE_PASSWORD);
        if (newSourseUrl != sourceUrl && modified) {
            e_account_set_string(acc, E_ACCOUNT_SOURCE_URL, newSourseUrl.toUtf8());
            modified = true;
        }

        QString newTransportUrl = camel_url_to_string(transportCamelUrl, CAMEL_URL_HIDE_PASSWORD);
        if (transportUrl != newTransportUrl && modified ) {
            e_account_set_string(acc, E_ACCOUNT_TRANSPORT_URL, newTransportUrl.toUtf8());
            modified = true;
        }

        if (modified) {
            EIterator *iter = e_list_get_iterator(E_LIST(mAccountList));
            while (e_iterator_is_valid(iter)) {
                EAccount *acc2 = (EAccount *)e_iterator_get(iter);
                if (strcmp(acc->uid, acc2->uid) == 0) {
                    if (acc != acc2)
                        e_account_import(acc2, acc);
                    e_account_list_change(mAccountList, acc2);
                    if (modified)
                        e_account_list_save(mAccountList);
                    break;
                }
                e_iterator_next(iter);
            }
            g_object_unref(iter);
        }

        camel_url_free(transportCamelUrl);
        camel_url_free(sourceCamelUrl);
    }

    return result;
}

QVariant EmailAccountSettingsModel::dataWrapper(int row, int role)
{
    return data(index(row, 0), role);
}

bool EmailAccountSettingsModel::setDataWrapper(int row, const QVariant &value, int role)
{
    return setData(index(row, 0), value, role);
}

int EmailAccountSettingsModel::updateInterval()
{
    return mUpdateInterval;
}

void EmailAccountSettingsModel::setUpdateInterval(int interval)
{
    mUpdateInterval = interval;
}

QString EmailAccountSettingsModel::signature()
{
    return mSignature;
}

void EmailAccountSettingsModel::setSignature(QString signature)
{
    mSignature = signature;
}

bool EmailAccountSettingsModel::newMailNotifications()
{
    return mNewMailNotification;
}

void EmailAccountSettingsModel::setNewMailNotifications(bool val)
{
    mNewMailNotification = val;
}

bool EmailAccountSettingsModel::confirmDeleteMail()
{
    return mConfirmDeleteMail;
}

void EmailAccountSettingsModel::setConfirmDeleteMail(bool val)
{
    mConfirmDeleteMail = val;
}

void EmailAccountSettingsModel::saveChanges()
{
    bool modified = false;
    bool save = false;
    mUpdateIntervalConf->set(mUpdateInterval);
    mSignatureConf->set(mSignature);
    mNewMailNotificationConf->set(mNewMailNotification);
    mConfirmDeleteMailConf->set(mConfirmDeleteMail);

        EIterator *iter = e_list_get_iterator(E_LIST(mAccountList));
        while (e_iterator_is_valid(iter)) {
            EAccount *acc = (EAccount *)e_iterator_get(iter);
            QString sig = e_account_get_string(acc, E_ACCOUNT_ID_SIGNATURE);
            QString newSig = mSignatureConf->value().toString();
            modified = false;
            if (sig != newSig) {
                e_account_set_string(acc, E_ACCOUNT_ID_SIGNATURE, newSig.toUtf8());
                modified = true;
            }

            int update = e_account_get_int(acc, E_ACCOUNT_SOURCE_AUTO_CHECK_TIME);
            int newUpdate = mUpdateIntervalConf->value().toInt();
            if (update != newUpdate) {
                e_account_set_int(acc, E_ACCOUNT_SOURCE_AUTO_CHECK_TIME, newUpdate);
                e_account_set_bool (acc, E_ACCOUNT_SOURCE_AUTO_CHECK, newUpdate != 0);
                modified = true;
            }
	    if (modified) {
	        e_account_list_change(mAccountList, acc);
                save = true;
                modified = false;
            }
            e_iterator_next(iter);
        }
        g_object_unref(iter);

    if (save)
        e_account_list_save(mAccountList);
}

void EmailAccountSettingsModel::deleteRow(int row)
{
    // deletes a row immediately, rather than when saveChanges is called
    if (row >= 0 && row < rowCount()) {
        beginResetModel();
	EAccount *account = getAccountByIndex(row);

        EIterator *iter = e_list_get_iterator(E_LIST(mAccountList));
        while (e_iterator_is_valid(iter)) {
            EAccount *acc = (EAccount *)e_iterator_get(iter);
            if (strcmp(account->uid, acc->uid) == 0) {
                e_account_list_remove(mAccountList, acc);
                e_account_list_save(mAccountList);
                break;
            }
            e_iterator_next(iter);
        }
        g_object_unref(iter);

        init();
        endResetModel();
    }
}
