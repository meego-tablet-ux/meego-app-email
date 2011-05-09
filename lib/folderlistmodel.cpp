/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */



#define CAMEL_COMPILATION 1
#include <camel/camel-mime-message.h>
#include <camel/camel-stream-mem.h>
#include <camel/camel-mime-utils.h>
#include <camel/camel-iconv.h>
#include <camel/camel-mime-filter-basic.h>
#include <camel/camel-mime-filter-canon.h>
#include <camel/camel-mime-filter-charset.h>
#include <camel/camel-stream-filter.h>

#include "folderlistmodel.h"
#include <QMailAccount>
#include <QMailFolder>
#include <QMailMessage>
#include <QMailMessageKey>
#include <QMailStore>
#include <QDateTime>
#include <QTimer>
#include <QProcess>
#include <qmailnamespace.h>
#include <libedataserver/e-account-list.h>
#include <libedataserver/e-list.h>
#include <gconf/gconf-client.h>
#include <glib.h>

FolderListModel::FolderListModel(QObject *parent) :
    QAbstractListModel(parent)
{
    QHash<int, QByteArray> roles;
    roles.insert(FolderName, "folderName");
    roles.insert(FolderId, "folderId");
    roles.insert(FolderUnreadCount, "folderUnreadCount");
    roles.insert(FolderServerCount, "folderServerCount");
    setRoleNames(roles);
    m_outbox_proxy = NULL;
    m_sent_proxy = NULL;
    m_drafts_proxy = NULL;
}

FolderListModel::~FolderListModel()
{
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
                displayName = QString (folder.full_name);
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

void FolderListModel::setAccountKey(QVariant id)
{
    GConfClient *client;
    EAccountList *account_list;
    QString quid;
    const char *url;

    if (m_account && m_account->uid && strcmp (m_account->uid, (const char *)id.toString().toLocal8Bit().constData()) == 0) {
	return;
    }

    client = gconf_client_get_default ();
    account_list = e_account_list_new (client);
    g_object_unref (client);

    quid = id.value<QString>();    
    m_account = getAccountById (account_list, (char *)quid.toLocal8Bit().constData());
    g_object_ref (m_account);
    g_object_unref (account_list);
    url = e_account_get_string (m_account, E_ACCOUNT_SOURCE_URL);

    g_print ("fetching store: %s\n", url);
    OrgGnomeEvolutionDataserverMailSessionInterface *instance = OrgGnomeEvolutionDataserverMailSessionInterface::instance(this);
    if (instance && instance->isValid()) {
	QDBusPendingReply<QDBusObjectPath> reply = instance->getStore (QString(url));
        reply.waitForFinished();
        m_store_proxy_id = reply.value();
	g_print ("Store PATH: %s\n", (char *) m_store_proxy_id.path().toLocal8Bit().constData());
        m_store_proxy = new OrgGnomeEvolutionDataserverMailStoreInterface (QString ("org.gnome.evolution.dataserver.Mail"),
									m_store_proxy_id.path(),
									QDBusConnection::sessionBus(), this);
	
	if (m_store_proxy && m_store_proxy->isValid()) {
		QDBusPendingReply<CamelFolderInfoArrayVariant> reply = m_store_proxy->getFolderInfo (QString(""), 
									CAMEL_STORE_FOLDER_INFO_RECURSIVE|CAMEL_STORE_FOLDER_INFO_FAST | CAMEL_STORE_FOLDER_INFO_SUBSCRIBED);
		reply.waitForFinished();
		m_folderlist = reply.value ();	
	g_print ("Got folder list....\n");

	}
	
	if (!m_outbox_proxy) {
		reply = instance->getLocalFolder (QString("Outbox"));
		reply.waitForFinished();
        	m_outbox_proxy_id = reply.value();

		m_outbox_proxy = new OrgGnomeEvolutionDataserverMailFolderInterface (QString ("org.gnome.evolution.dataserver.Mail"),
										     m_outbox_proxy_id.path(),
										     QDBusConnection::sessionBus(), this);
	}
	
	reply = instance->getFolderFromUri (QString(m_account->sent_folder_uri));
        reply.waitForFinished();
        m_sent_proxy_id = reply.value();
	m_sent_proxy = new OrgGnomeEvolutionDataserverMailFolderInterface (QString ("org.gnome.evolution.dataserver.Mail"),
									     m_sent_proxy_id.path(),
									     QDBusConnection::sessionBus(), this);

	reply = instance->getFolderFromUri (QString(m_account->drafts_folder_uri));
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
	g_print("FOLDER: %s\n", (char *)displayName.toLocal8Bit().constData());
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

    return QVariant();
}

QVariant FolderListModel::inboxFolderName()
{
    for (int i = 0; i < m_folderlist.size(); i++)
    {
        CamelFolderInfoVariant folder(m_folderlist[i]);

       if (QString::compare(folder.full_name, "INBOX", Qt::CaseInsensitive) == 0) {
            g_print ("Returning INBOX URI: %s\n", (char *)folder.uri.toLocal8Bit().constData());
            return QVariant(folder.folder_name);
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

	if (len == -1)
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
	} while (status == (gsize) -1);// && errno == E2BIG);
	camel_iconv_close (cd);

	if (status == (gsize) -1 || status > 0)
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

CamelMimeMessage * createMessage (const QString &from, const QStringList &to, const QStringList &cc, const QStringList &bcc, const QString &subject, const QString &body, const QStringList &attachment_uris, int priority)
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
	const gchar *iconv_charset = NULL;
	CamelTransferEncoding plain_encoding;

	client = gconf_client_get_default ();
	account_list = e_account_list_new (client);
	g_object_unref (client);

	msg = camel_mime_message_new ();
	addr = camel_internet_address_new ();
    	selected = getAccountByAddress (account_list, (char *)from.toLocal8Bit().constData());
	g_print("PRINT: %p %s %s\n", selected, selected->id->name, selected->id->address);
	camel_internet_address_add (addr, selected->id->name, selected->id->address);
	camel_mime_message_set_from (msg, addr);
	g_object_unref (addr);

	camel_medium_set_header (CAMEL_MEDIUM (msg), "X-Evolution-Account", selected->uid);
	camel_medium_set_header (CAMEL_MEDIUM (msg), "X-Evolution-Transport", selected->transport->url);
	camel_medium_set_header (CAMEL_MEDIUM (msg), "X-Evolution-Fcc",  selected->sent_folder_uri);

	g_object_unref (account_list);


	camel_mime_message_set_subject (msg, subject.toLocal8Bit().constData());

	addr = camel_internet_address_new ();
	foreach (QString str, to) {
		const char *email = str.toLocal8Bit().constData();

		if (camel_address_decode (CAMEL_ADDRESS (addr), email) <= 0)
			camel_internet_address_add (addr, "", email);
	}
	if (camel_address_length (CAMEL_ADDRESS (addr)) > 0) 
		camel_mime_message_set_recipients (msg, CAMEL_RECIPIENT_TYPE_TO, addr);
	g_object_unref (addr);


	addr = camel_internet_address_new ();
	foreach (QString str, cc) {
		const char *email = str.toLocal8Bit().constData();

		if (camel_address_decode (CAMEL_ADDRESS (addr), email) <= 0)
			camel_internet_address_add (addr, "", email);
	}
	if (camel_address_length (CAMEL_ADDRESS (addr)) > 0) 
		camel_mime_message_set_recipients (msg, CAMEL_RECIPIENT_TYPE_CC, addr);
	g_object_unref (addr);


	addr = camel_internet_address_new ();
	foreach (QString str, bcc) {
		const char *email = str.toLocal8Bit().constData();

		if (camel_address_decode (CAMEL_ADDRESS (addr), email) <= 0)
			camel_internet_address_add (addr, "", email);
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

	type = camel_content_type_new ("text", "plain");
	if ((charset = best_charset (body.toLocal8Bit().constData(), body.length(), /*p->charset, */ &plain_encoding))) {
		camel_content_type_set_param (type, "charset", charset);
		iconv_charset = camel_iconv_charset_name (charset);
		g_free (charset);
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

	if (attachment_uris.length() == 0) {
		camel_medium_set_content (CAMEL_MEDIUM (msg), plain);
		camel_mime_part_set_encoding (CAMEL_MIME_PART (msg), plain_encoding);
	} else {
		/*CamelMultipart *body = NULL;
		CamelMimePart *part;

		body = camel_multipart_new ();
		camel_multipart_set_boundary (body, NULL);

		part = camel_mime_part_new ();
		camel_medium_set_content (CAMEL_MEDIUM (part), plain);
		camel_multipart_add_part (multipart, part);
		g_object_unref (part);
		
		foreach (QString uri, attachment_uris) {
			CamelDataWrapper *wrapper;
			CamelMimePart *apart;
			CamelStream *astream;
			GFileInfo *file_info;
			
			wrapper = camel_data_wrapper_new ();
					
		}
*/

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

	/* We don't have to form refs & other headers while appending for send. Its not used anyways */

	return;
}

int FolderListModel::saveDraft(const QString &from, const QStringList &to, const QStringList &cc, const QStringList &bcc, const QString &subject, const QString &body, const QStringList &attachment_uris, int priority)
{
	CamelMimeMessage *msg;
	CamelStream *stream;
        GByteArray *array;
	CamelMessageInfoVariant info;

	msg = createMessage (from, to, cc, bcc, subject, body, attachment_uris, priority);
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

int FolderListModel::sendMessage(const QString &from, const QStringList &to, const QStringList &cc, const QStringList &bcc, const QString &subject, const QString &body, const QStringList &attachment_uris, int priority)
{
	CamelMimeMessage *msg;
	CamelStream *stream;
        GByteArray *array;
	CamelMessageInfoVariant info;

	msg = createMessage (from, to, cc, bcc, subject, body, attachment_uris, priority);
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




