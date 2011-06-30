/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */



#define CAMEL_COMPILATION 1
#include <camel/camel-mime-message.h>
#include <camel/camel-multipart.h>
#include <camel/camel-stream-mem.h>
#include <camel/camel-stream-fs.h>
#include <camel/camel-stream-null.h>
#include <camel/camel-mime-utils.h>
#include <camel/camel-iconv.h>
#include <camel/camel-mime-filter-basic.h>
#include <camel/camel-mime-filter-canon.h>
#include <camel/camel-mime-filter-charset.h>
#include <camel/camel-stream-filter.h>

#include "folderlistmodel.h"
#include <QDateTime>
#include <QTimer>
#include <QProcess>
#include <libedataserver/e-account-list.h>
#include <libedataserver/e-data-server-util.h>
#include <libedataserver/e-list.h>
#include <gconf/gconf-client.h>
#include <glib.h>
#include <errno.h>

FolderListModel::FolderListModel(QObject *parent) :
    QAbstractListModel(parent)
{
    QHash<int, QByteArray> roles;
    roles.insert(FolderName, "folderName");
    roles.insert(FolderId, "folderId");
    roles.insert(FolderUnreadCount, "folderUnreadCount");
    roles.insert(FolderServerCount, "folderServerCount");
    setRoleNames(roles);
    m_account = NULL;
    m_store_proxy = NULL;
    m_lstore_proxy = NULL;
    m_outbox_proxy = NULL;
    m_sent_proxy = NULL;
    m_drafts_proxy = NULL;
    pop_foldername = NULL;
    session_instance = new OrgGnomeEvolutionDataserverMailSessionInterface (QString ("org.gnome.evolution.dataserver.Mail"),
                                                               QString ("/org/gnome/evolution/dataserver/Mail/Session"),
                                                               QDBusConnection::sessionBus(), parent);

}

FolderListModel::~FolderListModel()
{
    delete session_instance;
}

int FolderListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_folderlist.count();
}

QVariant FolderListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() > m_folderlist.count())
        return QVariant();

    
    CamelFolderInfoVariant  folder(m_folderlist[index.row()]);
    if (role == FolderName)
    {
	QString displayName;
	
        if (folder.full_name == ".#evolution/Junk")
                displayName = QString ("Junk");
        else if (folder.full_name == ".#evolution/Trash")
                displayName = QString ("Trash");
        else {
		const char *url = e_account_get_string (m_account, E_ACCOUNT_SOURCE_URL);
		if (strncmp(url, "pop:", 4) == 0)
	                displayName = QString (folder.folder_name);
		else
	                displayName = QString (folder.full_name);


		if (strncmp(url, "pop:", 4) == 0 &&
			displayName.endsWith ("INBOX", Qt::CaseInsensitive))
			displayName = QString ("Inbox");
		
                displayName.replace (QString("/"), QString(" / "));
        }

        return QVariant(displayName);
    }
    else if (role == FolderId)
    {
        return QVariant(folder.uri);
    } 
    else if (role == FolderUnreadCount)
    {
        return QVariant ((folder.unread_count == -1) ? 0 : folder.unread_count );
    }
    else if (role == FolderServerCount)
    {
        return QVariant ((folder.total_mail_count == -1) ? 0 : folder.total_mail_count);
    }

    return QVariant();
}

EAccount * FolderListModel::getAccountById(EAccountList *account_list, char *id)
{
    EIterator *iter;
    EAccount *account = NULL;

    iter = e_list_get_iterator (E_LIST (account_list));
    while (e_iterator_is_valid (iter)) {
        account = (EAccount *) e_iterator_get (iter);
        if (strcmp (id, account->uid) == 0)
             return account;
        e_iterator_next (iter);
    }

    g_object_unref (iter);

    return NULL;
}

int FolderListModel::getFolderMailCount()
{
	int old = m_folderlist.count();
	int count = 0;

	QDBusPendingReply<CamelFolderInfoArrayVariant> reply = m_store_proxy->getFolderInfo (QString(pop_foldername ? pop_foldername : ""), 
								CAMEL_STORE_FOLDER_INFO_RECURSIVE|CAMEL_STORE_FOLDER_INFO_FAST | CAMEL_STORE_FOLDER_INFO_SUBSCRIBED);
	reply.waitForFinished();
	m_folderlist = reply.value ();
	m_folderlist.removeLast();

	CamelFolderInfoArrayVariant ouboxlist;
	reply = m_lstore_proxy->getFolderInfo ("Outbox", CAMEL_STORE_FOLDER_INFO_FAST);
	reply.waitForFinished();
	ouboxlist = reply.value();
	ouboxlist.removeLast();
	m_folderlist.append (ouboxlist);
	
	if (m_folderlist.count() == old) {
		QModelIndex s_idx = createIndex (0, 0);
		QModelIndex e_idx = createIndex (m_folderlist.count() - 1, 0);

		emit dataChanged(s_idx, e_idx);
	} else if (m_folderlist.count() > old) {
		QModelIndex s_idx = createIndex (0, 0);
		QModelIndex e_idx = createIndex (old - 1, 0);

		emit dataChanged(s_idx, e_idx);
		beginInsertRows (QModelIndex(), old-1, m_folderlist.count()-1);
		endInsertRows();
	} else if (m_folderlist.count() < old) {
		QModelIndex s_idx = createIndex (0, 0);
		QModelIndex e_idx = createIndex (m_folderlist.count() - 1, 0);

		emit dataChanged(s_idx, e_idx);

		beginRemoveRows (QModelIndex(), m_folderlist.count()-1, old-1);
		endRemoveRows();
	
	}

    	foreach (CamelFolderInfoVariant fInfo, m_folderlist)
    	{
		if (fInfo.unread_count > 0)
			count+= fInfo.unread_count;
	}

	return count;
}

void FolderListModel::setAccountKey(QVariant id)
{
    GConfClient *client;
    EAccountList *account_list;
    QString quid;
    const char *url;
    char *acc_id;
    
   

    if (m_account && m_account->uid && strcmp (m_account->uid, (const char *)id.toString().toLocal8Bit().constData()) == 0) {
	return;
    }

    if (pop_foldername) {
	g_free (pop_foldername);
	pop_foldername = NULL;
    }
    client = gconf_client_get_default ();
    account_list = e_account_list_new (client);
    g_object_unref (client);

    quid = id.value<QString>();    
    acc_id = g_strdup (quid.toLocal8Bit().constData());
    m_account = getAccountById (account_list, acc_id);
    g_free (acc_id);
    g_object_ref (m_account);
    g_object_unref (account_list);
    url = e_account_get_string (m_account, E_ACCOUNT_SOURCE_URL);

    if (!m_lstore_proxy) {
	QDBusPendingReply<QDBusObjectPath> reply;
	reply = session_instance->getLocalStore();
        m_lstore_proxy_id = reply.value();

        m_lstore_proxy = new OrgGnomeEvolutionDataserverMailStoreInterface (QString ("org.gnome.evolution.dataserver.Mail"),
									m_lstore_proxy_id.path(),
									QDBusConnection::sessionBus(), this);

    }

    g_print ("fetching store: %s\n", url);
    if (session_instance && session_instance->isValid()) {
	QDBusPendingReply<QDBusObjectPath> reply;
	const char *email = NULL;

	if (strncmp (url, "pop:", 4) == 0)
		reply = session_instance->getLocalStore();
	else 
		reply = session_instance->getStore (QString(url));
        reply.waitForFinished();
        m_store_proxy_id = reply.value();

	if (strncmp (url, "pop:", 4) == 0) {

		email = e_account_get_string(m_account, E_ACCOUNT_ID_ADDRESS);
		pop_foldername = g_strdup(email);
	}

	g_print ("Store PATH: %s\n", (char *) m_store_proxy_id.path().toLocal8Bit().constData());
        m_store_proxy = new OrgGnomeEvolutionDataserverMailStoreInterface (QString ("org.gnome.evolution.dataserver.Mail"),
									m_store_proxy_id.path(),
									QDBusConnection::sessionBus(), this);
	
	if (m_store_proxy && m_store_proxy->isValid()) {
		QDBusPendingReply<CamelFolderInfoArrayVariant> reply = m_store_proxy->getFolderInfo (QString(pop_foldername ? pop_foldername : ""), 
									CAMEL_STORE_FOLDER_INFO_RECURSIVE|CAMEL_STORE_FOLDER_INFO_FAST | CAMEL_STORE_FOLDER_INFO_SUBSCRIBED);
		reply.waitForFinished();
		m_folderlist = reply.value ();	
		if (reply.isError() && strncmp (url, "pop:", 4) == 0) {
			QDBusPendingReply<CamelFolderInfoArrayVariant> reply2;
			char *folder_name;

			folder_name = g_strdup_printf ("%s/Inbox", email);
			reply2 = m_store_proxy->createFolder ("", folder_name);
			reply2.waitForFinished();
			g_free (folder_name);		
			folder_name = g_strdup_printf ("%s/Drafts", email);
			reply2 = m_store_proxy->createFolder ("", folder_name);
			reply2.waitForFinished();
	
			g_free (folder_name);		
			folder_name = g_strdup_printf ("%s/Sent", email);
			reply2 = m_store_proxy->createFolder ("", folder_name);
			reply2.waitForFinished();
			g_free (folder_name);

			reply2 = m_store_proxy->getFolderInfo (QString(email), 
							CAMEL_STORE_FOLDER_INFO_RECURSIVE|CAMEL_STORE_FOLDER_INFO_FAST | CAMEL_STORE_FOLDER_INFO_SUBSCRIBED);

			reply2.waitForFinished();
			m_folderlist = reply2.value ();	
			m_folderlist.removeFirst();
			m_folderlist.removeLast();
		} else {

			printf("Got %s: %d\n", url, m_folderlist.length());
			if (strncmp (url, "pop:", 4) == 0) {
				m_folderlist.removeFirst();
				m_folderlist.removeLast();
			}  else {
				m_folderlist.removeLast();
			}
		}

		CamelFolderInfoArrayVariant ouboxlist;
		reply = m_lstore_proxy->getFolderInfo ("Outbox", CAMEL_STORE_FOLDER_INFO_FAST);
		reply.waitForFinished();
		ouboxlist = reply.value();
		ouboxlist.removeLast();
		m_folderlist.append (ouboxlist);
		qDebug() << "Appending Outbox";

		foreach (CamelFolderInfoVariant fInfo, m_folderlist)
			qDebug () << fInfo.full_name;
	}

	if (!m_outbox_proxy) {
		reply = session_instance->getLocalFolder (QString("Outbox"));
		reply.waitForFinished();
        	m_outbox_proxy_id = reply.value();

		m_outbox_proxy = new OrgGnomeEvolutionDataserverMailFolderInterface (QString ("org.gnome.evolution.dataserver.Mail"),
										     m_outbox_proxy_id.path(),
										     QDBusConnection::sessionBus(), this);
	}
	
	reply = session_instance->getFolderFromUri (QString(m_account->sent_folder_uri));
        reply.waitForFinished();
        m_sent_proxy_id = reply.value();
	m_sent_proxy = new OrgGnomeEvolutionDataserverMailFolderInterface (QString ("org.gnome.evolution.dataserver.Mail"),
									     m_sent_proxy_id.path(),
									     QDBusConnection::sessionBus(), this);

	reply = session_instance->getFolderFromUri (QString(m_account->drafts_folder_uri));
        reply.waitForFinished();
        m_drafts_proxy_id = reply.value();
	m_drafts_proxy = new OrgGnomeEvolutionDataserverMailFolderInterface (QString ("org.gnome.evolution.dataserver.Mail"),
									     m_drafts_proxy_id.path(),
									     QDBusConnection::sessionBus(), this);

	qDebug() << "Setup Outbox/Draft/Sent";
    }
}

QStringList FolderListModel::folderNames()
{
    QStringList folderNames;

    foreach (CamelFolderInfoVariant fInfo, m_folderlist)
    {
        QString displayName;

	if (fInfo.full_name == ".#evolution/Junk")
		displayName = QString ("Junk");
	else if (fInfo.full_name == ".#evolution/Trash")
		displayName = QString ("Trash");
	else {
		displayName = QString (fInfo.full_name);
		displayName.replace (QString("/"), QString(" / "));
	}
	
        if (fInfo.unread_count > 0)
        {
            displayName = displayName + " (" + QString::number(fInfo.unread_count) + ")";
        }
	
        folderNames << displayName;
	g_print("FOLDER: |-%s-|\n", (char *)displayName.toLocal8Bit().constData());
    }
    return folderNames;
}

QVariant FolderListModel::folderId(int index)
{
    if (index < 0 || index >= m_folderlist.count())
        return QVariant();
   
    return m_folderlist[index].uri;
}

int FolderListModel::indexFromFolderId(QVariant vFolderId)
{   
    
    QString uri = vFolderId.toString();
    g_print ("Entering FolderListModel::indexFromFolderId: %s\n", (char *)uri.toLocal8Bit().constData());
    for (int i = 0; i < m_folderlist.size(); i ++)
    {
        if (vFolderId == 0) {
	        CamelFolderInfoVariant folder(m_folderlist[i]);
        	if (QString::compare(folder.full_name, "INBOX", Qt::CaseInsensitive) == 0)
			return i;

	} else {
        	if (uri == m_folderlist[i].uri)
            		return i;
	}
    }
    return -1;
}

QVariant FolderListModel::folderServerCount(QVariant vFolderId)
{
    QString uri = vFolderId.toString();

    for (int i = 0; i < m_folderlist.size(); i ++)
    {
        if (uri == m_folderlist[i].uri)
            return QVariant(m_folderlist[i].total_mail_count);
    }
    return QVariant(-1);
}

QVariant FolderListModel::inboxFolderId()
{
    for (int i = 0; i < m_folderlist.size(); i++)
    {
        CamelFolderInfoVariant folder(m_folderlist[i]);
        if (QString::compare(folder.full_name, "INBOX", Qt::CaseInsensitive) == 0) {
	    g_print ("Returning INBOX URI: %s\n", (char *)folder.uri.toLocal8Bit().constData());
            return QVariant(folder.uri);
	}
    }

    /* Check if anything ends with INBOX. POP has it like that */
    for (int i = 0; i < m_folderlist.size(); i++)
    {
        CamelFolderInfoVariant folder(m_folderlist[i]);
        if (folder.full_name.endsWith("INBOX", Qt::CaseInsensitive)) {
	    g_print ("Returning INBOX ENDS WITH URI: %s\n", (char *)folder.uri.toLocal8Bit().constData());
            return QVariant(folder.uri);
	}
    }
   
    return QVariant();
}

QVariant FolderListModel::inboxFolderName()
{
    for (int i = 0; i < m_folderlist.size(); i++)
    {
        CamelFolderInfoVariant folder(m_folderlist[i]);

       if (QString::compare(folder.full_name, "INBOX", Qt::CaseInsensitive) == 0) {
            return QVariant(folder.folder_name);
        }
    }

   /* Check if anything ends with INBOX. POP has it like that */
    for (int i = 0; i < m_folderlist.size(); i++)
    {
        CamelFolderInfoVariant folder(m_folderlist[i]);
        if (folder.full_name.endsWith("INBOX", Qt::CaseInsensitive)) {
            return QVariant("Inbox");
	}
    }
    return QVariant("");
}

int FolderListModel::totalNumberOfFolders()
{
    return m_folderlist.size();
}


EAccount * getAccountByAddress(EAccountList *account_list, char *address)
{
    EIterator *iter;
    EAccount *account = NULL;

    iter = e_list_get_iterator (E_LIST (account_list));
    while (e_iterator_is_valid (iter)) {
        account = (EAccount *) e_iterator_get (iter);
        if (strcmp (address, account->id->address) == 0)
             return account;
        e_iterator_next (iter);
    }

    g_object_unref (iter);

    return NULL;
}

#define LINE_LEN 72

static gboolean
text_requires_quoted_printable (const gchar *text, gsize len)
{
	const gchar *p;
	gsize pos;

	if (!text)
		return FALSE;

	if (len == (gsize) -1)
		len = strlen (text);

	if (len >= 5 && strncmp (text, "From ", 5) == 0)
		return TRUE;

	for (p = text, pos = 0; pos + 6 <= len; pos++, p++) {
		if (*p == '\n' && strncmp (p + 1, "From ", 5) == 0)
			return TRUE;
	}

	return FALSE;
}

static CamelTransferEncoding
best_encoding (const char *buf, int buflen, const gchar *charset)
{
	gchar *in, *out, outbuf[256], *ch;
	gsize inlen, outlen;
	gint status, count = 0;
	iconv_t cd;

	if (!charset)
		return (CamelTransferEncoding)-1;

	cd = camel_iconv_open (charset, "utf-8");
	if (cd == (iconv_t) -1)
		return (CamelTransferEncoding) -1;

	in = (gchar *) buf;
	inlen = buflen;
	do {
		out = outbuf;
		outlen = sizeof (outbuf);
		status = camel_iconv (cd, (const gchar **) &in, &inlen, &out, &outlen);
		for (ch = out - 1; ch >= outbuf; ch--) {
			if ((guchar) *ch > 127)
				count++;
		}
	} while (status == -1 && errno == E2BIG);
	camel_iconv_close (cd);

	if (status == -1 || status > 0)
		return (CamelTransferEncoding)-1;

	if ((count == 0) && (buflen < LINE_LEN) &&
		!text_requires_quoted_printable (
		(const gchar *) buf, buflen))
		return CAMEL_TRANSFER_ENCODING_7BIT;
	else if (count <= buflen * 0.17)
		return CAMEL_TRANSFER_ENCODING_QUOTEDPRINTABLE;
	else
		return CAMEL_TRANSFER_ENCODING_BASE64;
}

static gchar *
best_charset (const char *buf, int buflen,
              /* const gchar *default_charset, */
              CamelTransferEncoding *encoding)
{
	gchar *charset;

	/* First try US-ASCII */
	*encoding = best_encoding (buf, buflen, "US-ASCII");
	if (*encoding == CAMEL_TRANSFER_ENCODING_7BIT)
		return NULL;
#if 0
	/* Next try the user-specified charset for this message */
	*encoding = best_encoding (buf, default_charset);
	if (*encoding != -1)
		return g_strdup (default_charset);

	/* Now try the user's default charset from the mail config */
	charset = e_composer_get_default_charset ();
	*encoding = best_encoding (buf, charset);
	if (*encoding != -1)
		return charset;
#endif
	/* Try to find something that will work */
	if (!(charset = (gchar *) camel_charset_best ((const gchar *)buf, buflen))) {
		*encoding = CAMEL_TRANSFER_ENCODING_7BIT;
		return NULL;
	}

	*encoding = best_encoding (buf, buflen, charset);

	return g_strdup (charset);
}

#define ATTACHMENT_QUERY "standard::*,preview::*,thumbnail::*"

CamelMimeMessage * createMessage (const QString &from, const QStringList &to, const QStringList &cc, const QStringList &bcc, const QString &subject, const QString &body, bool html, const QStringList &attachment_uris, int priority)
{
	CamelMimeMessage *msg;
	GConfClient *client;
	EAccountList *account_list;
	EAccount *selected;
	CamelInternetAddress *addr;
	CamelStream *stream;
	CamelDataWrapper *plain;
	CamelContentType *type;
	gchar *charset;
	char *cfrom, *csubject;
	const gchar *iconv_charset = NULL;
	CamelTransferEncoding plain_encoding;

	client = gconf_client_get_default ();
	account_list = e_account_list_new (client);
	g_object_unref (client);

	msg = camel_mime_message_new ();
	addr = camel_internet_address_new ();
	cfrom = g_strdup (from.toLocal8Bit().constData());
    	selected = getAccountByAddress (account_list, cfrom);
	g_free (cfrom);
	g_print("PRINT: %p %s %s\n", selected, selected->id->name, selected->id->address);
	camel_internet_address_add (addr, selected->id->name, selected->id->address);
	camel_mime_message_set_from (msg, addr);
	g_object_unref (addr);

	camel_medium_set_header (CAMEL_MEDIUM (msg), "X-Evolution-Account", selected->uid);
	camel_medium_set_header (CAMEL_MEDIUM (msg), "X-Evolution-Transport", selected->transport->url);
	camel_medium_set_header (CAMEL_MEDIUM (msg), "X-Evolution-Fcc",  selected->sent_folder_uri);

	g_object_unref (account_list);

	csubject = g_strdup (subject.toLocal8Bit().constData());
	camel_mime_message_set_subject (msg, csubject);
	g_free (csubject);

	addr = camel_internet_address_new ();
	foreach (QString str, to) {
		char *email = g_strdup(str.toLocal8Bit().constData());

		if (camel_address_decode (CAMEL_ADDRESS (addr), email) <= 0)
			camel_internet_address_add (addr, "", email);

		g_free (email);
	}
	if (camel_address_length (CAMEL_ADDRESS (addr)) > 0) 
		camel_mime_message_set_recipients (msg, CAMEL_RECIPIENT_TYPE_TO, addr);
	g_object_unref (addr);


	addr = camel_internet_address_new ();
	foreach (QString str, cc) {
		char *email = g_strdup(str.toLocal8Bit().constData());

		if (camel_address_decode (CAMEL_ADDRESS (addr), email) <= 0)
			camel_internet_address_add (addr, "", email);
		g_free (email);
	}
	if (camel_address_length (CAMEL_ADDRESS (addr)) > 0) 
		camel_mime_message_set_recipients (msg, CAMEL_RECIPIENT_TYPE_CC, addr);
	g_object_unref (addr);


	addr = camel_internet_address_new ();
	foreach (QString str, bcc) {
		char *email = g_strdup(str.toLocal8Bit().constData());

		if (camel_address_decode (CAMEL_ADDRESS (addr), email) <= 0)
			camel_internet_address_add (addr, "", email);
		g_free (email);
	}
	if (camel_address_length (CAMEL_ADDRESS (addr)) > 0) 
		camel_mime_message_set_recipients (msg, CAMEL_RECIPIENT_TYPE_BCC, addr);
	g_object_unref (addr);

	//Not sure if Priority is to be set like this, but for now following the existing client code.
	if (priority == 2) /* High priority*/ {
		camel_medium_add_header (CAMEL_MEDIUM (msg), "X-Priority", "1");
		camel_medium_add_header (CAMEL_MEDIUM (msg), "X-MSMail-Priority", "High");
	} else if (priority == 0) /* Low Priority */ {
		camel_medium_add_header (CAMEL_MEDIUM (msg), "X-Priority", "5");
		camel_medium_add_header (CAMEL_MEDIUM (msg), "X-MSMail-Priority", "Low");
	} else
		camel_medium_add_header (CAMEL_MEDIUM (msg), "X-MSMail-Priority", "Normal");

	stream = camel_stream_mem_new_with_buffer (body.toLocal8Bit().constData(), body.length());
	type = camel_content_type_new ("text", html ? "html" : "plain");

	if (html) {
		/* possible that we have non-utf8. lets filter that out first */
		static const char *gcharset = NULL;
		CamelStream *filter_stream = NULL;
		CamelStream *newstream;
		CamelMimeFilter *charenc = NULL;

		if (!gcharset)  {	
			GConfClient *gconf = gconf_client_get_default ();
			gcharset = gconf_client_get_string (gconf, "/apps/evolution/mail/display/charset",NULL);
			if (!gcharset || !*gcharset) {
				gcharset = gconf_client_get_string (gconf, "/apps/evolution/mail/composer/charset",NULL);
				if (!gcharset || !*gcharset) {
					bool ret = g_get_charset (&gcharset);
					if (!ret || !gcharset || !*gcharset)
						gcharset = g_strdup ("us-ascii");
				}
			}

			g_object_unref (gconf);
		}
		newstream = camel_stream_mem_new ();
		filter_stream = camel_stream_filter_new (stream);
		charenc = camel_mime_filter_charset_new (gcharset, "UTF-8");
		camel_stream_filter_add (CAMEL_STREAM_FILTER (filter_stream), charenc);
		g_object_unref (charenc);
		g_object_unref (stream);
		camel_stream_write_to_stream (filter_stream, newstream, NULL);
		
		stream = newstream;	
        	GByteArray *array;
	        array = camel_stream_mem_get_byte_array ((CamelStreamMem *)stream);
		array->data[array->len-1] = 0;

		if ((charset = best_charset ((const char *)array->data, array->len, /*p->charset, */ &plain_encoding))) {
			camel_content_type_set_param (type, "charset", charset);
			iconv_charset = camel_iconv_charset_name (charset);
			g_free (charset);
		}
	} else {
		if ((charset = best_charset (body.toLocal8Bit().constData(), body.length(), /*p->charset, */ &plain_encoding))) {
			camel_content_type_set_param (type, "charset", charset);
			iconv_charset = camel_iconv_charset_name (charset);
			g_free (charset);
		}
	}

	/* convert the stream to the appropriate charset */
	if (iconv_charset && g_ascii_strcasecmp (iconv_charset, "UTF-8") != 0) {
		CamelStream *filter_stream;
		CamelMimeFilter *filter;

		filter_stream = camel_stream_filter_new (stream);
		g_object_unref (stream);

		stream = filter_stream;
		filter = camel_mime_filter_charset_new ("UTF-8", iconv_charset);
		camel_stream_filter_add (
			CAMEL_STREAM_FILTER (filter_stream), filter);
		g_object_unref (filter);
	}

	if (plain_encoding == CAMEL_TRANSFER_ENCODING_QUOTEDPRINTABLE) {
		/* encode to quoted-printable by ourself, together with
		 * taking care of "\nFrom " text */
		CamelStream *filter_stream;
		CamelMimeFilter *mf, *qp;

		if (!CAMEL_IS_STREAM_FILTER (stream)) {
			filter_stream = camel_stream_filter_new (stream);
			g_object_unref (stream);

			stream = filter_stream;
		}

		qp = camel_mime_filter_basic_new (
			CAMEL_MIME_FILTER_BASIC_QP_ENC);
		camel_stream_filter_add (CAMEL_STREAM_FILTER (stream), qp);
		g_object_unref (qp);

		mf = camel_mime_filter_canon_new (CAMEL_MIME_FILTER_CANON_FROM);
		camel_stream_filter_add (CAMEL_STREAM_FILTER (stream), mf);
		g_object_unref (mf);
	}

	plain = camel_data_wrapper_new ();
	camel_data_wrapper_construct_from_stream (plain, stream, NULL);
	g_object_unref (stream);

	if (plain_encoding == CAMEL_TRANSFER_ENCODING_QUOTEDPRINTABLE) {
		/* to not re-encode the data when pushing it to a part */
		plain->encoding = plain_encoding;
	}

	camel_data_wrapper_set_mime_type_field (plain, type);
	camel_content_type_unref (type);

	foreach (QString uri, attachment_uris)
		qDebug() << uri;

	if (attachment_uris.length() == 0) {
		camel_medium_set_content (CAMEL_MEDIUM (msg), plain);
		camel_mime_part_set_encoding (CAMEL_MIME_PART (msg), plain_encoding);
	} else {
		CamelMultipart *body = NULL;
		CamelMimePart *part;

		body = camel_multipart_new ();
		camel_multipart_set_boundary (body, NULL);

		part = camel_mime_part_new ();
		camel_medium_set_content (CAMEL_MEDIUM (part), plain);
		camel_mime_part_set_encoding (CAMEL_MIME_PART (part), plain_encoding);
		camel_multipart_add_part (body, part);
		g_object_unref (part);
		
		foreach (QString uri, attachment_uris) {
			CamelDataWrapper *wrapper;
			CamelMimePart *apart;
			CamelStream *astream;
			GFile *file;
			GFileInfo *finfo;
			const gchar *ctype, *display_name, *description;
			gchar *mime_type, *fpath;
			CamelContentType *content_type;
			GError *error;
			char *newpath = NULL;
			
			if (uri.indexOf("file://") == -1) {
				if (!g_file_test(uri.toLocal8Bit().constData(),G_FILE_TEST_EXISTS)) {
					//File doesn't exist, so check tmp
					char *oldpath = g_strdup (uri.toLocal8Bit().constData());
					if (oldpath && *oldpath && oldpath[0] != '/') {
						/* Not an absolute path. lets check tmp */
						newpath = g_build_filename (e_get_user_cache_dir(), "tmp", oldpath, NULL);
						uri = QString("file://")+QString (newpath);
						qDebug() << "Built new path: "+ uri;
					} else { /* Ignore and move on to the next attachment. */ 
						g_free (oldpath);
						continue;
					}
					g_free (oldpath);
				} else {
					uri = QString("file://")+QString (newpath);
				}
			}

			qDebug() << "About to save "+ uri;

			file = g_file_new_for_uri (uri.toLocal8Bit().constData());
			finfo = g_file_query_info (file, ATTACHMENT_QUERY, G_FILE_QUERY_INFO_NONE, NULL, NULL);
			ctype = g_file_info_get_content_type (finfo);
			mime_type = g_content_type_get_mime_type (ctype);
			
			fpath = g_file_get_path (file);
			astream = camel_stream_fs_new_with_name (fpath, O_RDONLY, 0, &error);
			if (!astream)
				g_print ("Error: %s: %s\n", fpath, error->message);
			else
				g_print("Successfully opened the attachment\n");
			g_free (fpath);

			wrapper = camel_data_wrapper_new ();
			camel_data_wrapper_set_mime_type (wrapper, mime_type);
			g_print("CONSTRUCT: %d\n", camel_data_wrapper_construct_from_stream (wrapper, astream, NULL));
			g_object_unref (astream);

			apart = camel_mime_part_new ();
			camel_medium_set_content (CAMEL_MEDIUM (apart), wrapper);

			display_name = g_file_info_get_display_name (finfo);
			if (display_name != NULL)
				camel_mime_part_set_filename (apart, display_name);

			description = g_file_info_get_attribute_string (finfo, G_FILE_ATTRIBUTE_STANDARD_DESCRIPTION);
			if (description != NULL)
				camel_mime_part_set_description (apart, description);
			camel_mime_part_set_disposition (apart, "attachment");

			content_type = camel_mime_part_get_content_type (apart);

			/* For text content, determine the best encoding and character set. */
			if (camel_content_type_is (content_type, "text", "*")) {
				CamelTransferEncoding encoding;
				CamelStream *filtered_stream;
				CamelMimeFilter *filter;
				CamelStream *stream;
				const gchar *charset;
				gchar *type;

				charset = camel_content_type_param (content_type, "charset");

				/* Determine the best encoding by writing the MIME
				 * part to a NULL stream with a "bestenc" filter. */
				stream = camel_stream_null_new ();
				filtered_stream = camel_stream_filter_new (stream);
				filter = camel_mime_filter_bestenc_new (
					CAMEL_BESTENC_GET_ENCODING);
				camel_stream_filter_add (
					CAMEL_STREAM_FILTER (filtered_stream),
					CAMEL_MIME_FILTER (filter));
				camel_data_wrapper_decode_to_stream (
					wrapper, filtered_stream, NULL);
				g_object_unref (filtered_stream);
				g_object_unref (stream);

				/* Retrieve the best encoding from the filter. */
				encoding = camel_mime_filter_bestenc_get_best_encoding (
					CAMEL_MIME_FILTER_BESTENC (filter),
					CAMEL_BESTENC_8BIT);
				camel_mime_part_set_encoding (apart, encoding);
				g_object_unref (filter);

				if (encoding == CAMEL_TRANSFER_ENCODING_7BIT) {
					/* The text fits within us-ascii, so this is safe.
					 * FIXME Check that this isn't iso-2022-jp? */
					charset = "us-ascii";
				} else if (charset == NULL) {
					charset = g_strdup (camel_iconv_locale_charset ());
					/* FIXME Check that this fits within the
					 *       default_charset and if not, find one
					 *       that does and/or allow the user to
					 *       specify. */
				}

				camel_content_type_set_param (
					content_type, "charset", charset);
				type = camel_content_type_format (content_type);
				camel_mime_part_set_content_type (apart, type);
				g_free (type);

			} else if (!CAMEL_IS_MIME_MESSAGE (wrapper))
				camel_mime_part_set_encoding (
					apart, CAMEL_TRANSFER_ENCODING_BASE64);


			camel_multipart_add_part (body, apart);


			g_object_unref (wrapper);
			g_free (mime_type);
			g_object_unref (apart);
			if (newpath) {
				/* This is a tmp file. Remove it.*/
				g_file_delete (file, NULL, NULL);
				g_free (newpath);
			}
			g_object_unref (file);
			g_object_unref (finfo);
				
		}

		camel_medium_set_content (CAMEL_MEDIUM (msg), (CamelDataWrapper *)body);
		


	}

	g_object_unref (plain);
	return msg;
}

void createInfo (CamelMessageInfoVariant &info, const QString &from, const QStringList &to, const QStringList &cc, QString subject, CamelMimeMessage *msg)
{
	struct _camel_header_raw *header;
	char *date;


	header = ((CamelMimePart *)msg)->headers;

	info.uid = "";
	info.subject = subject;
	if (from.indexOf(QString("<"), 0) == -1) {
		info.from = QString("<");
		info.from.append (from);
		info.from.append (QString(">"));
	} else
		info.from = from;

	info.to = QString("");
	foreach (QString to_email, to) {
		if (to_email.indexOf(QString("<"), 0) == -1) {
			info.to.append(QString("<"));
			info.to.append (to_email);
			info.to.append (QString(">"));
		} else
			info.to.append (to_email);
		
		info.to.append (QString(","));
	}
	if (info.to.length() > 1) //Truncate the last comma
		info.to.truncate (info.to.length() - 1);


	info.cc = QString("");
	foreach (QString cc_email, cc) {
		if (cc_email.indexOf(QString("<"), 0) == -1) {
			info.cc.append(QString("<"));
			info.cc.append (cc_email);
			info.cc.append (QString(">"));
		} else
			info.cc.append (cc_email);
		
		info.cc.append (QString(","));
	}
	if (info.cc.length() > 1) //Truncate the last comma
		info.cc.truncate (info.cc.length() - 1);


	info.mlist = QString (camel_header_raw_check_mailing_list(&header));
	info.flags = 0;

	if ((date = (char *)camel_header_raw_find (&header, "date", NULL)))
		info.date_sent= (qulonglong)camel_header_decode_date (date, NULL);
	else
		info.date_sent = 0;

	date = (char *)camel_header_raw_find(&header, "received", NULL);
	if (date)
		date = strrchr(date, ';');
	if (date)
		info.date_received = (qulonglong)camel_header_decode_date(date + 1, NULL);
	else
		info.date_received = 0;
	
	info.references_size = 0;
	/* We don't have to form refs & other headers while appending for send. Its not used anyways */

	return;
}

int FolderListModel::saveDraft(const QString &from, const QStringList &to, const QStringList &cc, const QStringList &bcc, const QString &subject, const QString &body, bool html, const QStringList &attachment_uris, int priority)
{
	CamelMimeMessage *msg;
	CamelStream *stream;
        GByteArray *array;
	CamelMessageInfoVariant info;

	msg = createMessage (from, to, cc, bcc, subject, body, html, attachment_uris, priority);
	stream = camel_stream_mem_new ();
        camel_data_wrapper_decode_to_stream ((CamelDataWrapper *)msg, stream, NULL);
        array = camel_stream_mem_get_byte_array ((CamelStreamMem *)stream);
	g_byte_array_append (array, (const guint8 *)"\0", 1);
	g_print ("Draft Message:\n\n%s\n", array->data);

	createInfo (info, from, to, cc, subject, msg);
	m_drafts_proxy->AppendMessage (info, QString((char *)array->data));

        g_object_unref (stream);
        g_object_unref (msg);

	return 0;
	
}

int FolderListModel::sendMessage(const QString &from, const QStringList &to, const QStringList &cc, const QStringList &bcc, const QString &subject, const QString &body, bool html, const QStringList &attachment_uris, int priority)
{
	CamelMimeMessage *msg;
	CamelStream *stream;
        GByteArray *array;
	CamelMessageInfoVariant info;

	msg = createMessage (from, to, cc, bcc, subject, body, html, attachment_uris, priority);
	stream = camel_stream_mem_new ();
        camel_data_wrapper_decode_to_stream ((CamelDataWrapper *)msg, stream, NULL);
        array = camel_stream_mem_get_byte_array ((CamelStreamMem *)stream);
	g_byte_array_append (array, (const guint8 *)"\0", 1);
	g_print ("Draft Message:\n\n%s\n", array->data);
	
	createInfo (info, from, to, cc, subject, msg);

	m_outbox_proxy->AppendMessage (info, QString((char *)array->data));

        g_object_unref (stream);
        g_object_unref (msg);

	return 0;
} 

void FolderListModel::createFolder(const QString &name, QVariant parentFolderId)
{
	QDBusPendingReply<CamelFolderInfoArrayVariant> reply;
	CamelFolderInfoArrayVariant newlist;

	//See what to do with parentFolderId, do we have to create under Parent?
	reply = m_store_proxy->createFolder ("", name);
	reply.waitForFinished();
	newlist = reply.value ();
	newlist.removeLast();

	/* Add the new folder to the folder list. */
    	beginInsertRows(QModelIndex(), rowCount()-1, rowCount()+newlist.length()-2);
	m_folderlist.append (newlist);
    	endInsertRows();
}

void FolderListModel::deleteFolder(QVariant folderId)
{
    for (int i = 0; i < m_folderlist.size(); i++)
    {
        CamelFolderInfoVariant folder(m_folderlist[i]);

	if (folder.uri == folderId.toString()) {
		/*This is the folder to delete */
		QDBusPendingReply<bool> reply = m_store_proxy->deleteFolder(folder.full_name);
		reply.waitForFinished();
		/* check the bool return to see if it succeeded deleting. */
		break;
	}
    }
	
}

void FolderListModel::renameFolder(QVariant folderId, const QString &name)
{
    for (int i = 0; i < m_folderlist.size(); i++)
    {
        CamelFolderInfoVariant folder(m_folderlist[i]);

	if (folder.uri == folderId.toString()) {
		/*This is the folder to rename */
		QDBusPendingReply<bool> reply = m_store_proxy->renameFolder(folder.full_name, name);
		reply.waitForFinished();
		/* check the bool return to see if it succeeded renaming. */
		break;
	}
    }	
}

bool FolderListModel::canModifyFolders ()
{
    const char *url = e_account_get_string (m_account, E_ACCOUNT_SOURCE_URL);
    if (strncmp (url, "pop:", 4) == 0)
        return false;

    return true;
}
