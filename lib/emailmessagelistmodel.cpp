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
#include <camel/camel-stream-fs.h>
#include <camel/camel-multipart.h>
#include <camel/camel-medium.h>
#include <camel/camel-data-wrapper.h>
#include <camel/camel-url.h>
#include <camel/camel-stream-filter.h>
#include <camel/camel-mime-filter-charset.h>
#include "emailmessagelistmodel.h"

#include <QDateTime>
#include <QTimer>
#include <QProcess>
#include <glib.h>
#include <libedataserver/e-account-list.h>
#include <libedataserver/e-data-server-util.h>
#include <gconf/gconf-client.h>
#include "meegolocale.h"

using namespace meego;
static const Locale g_locale;

namespace {
    const int WINDOW_LIMIT = 20;
    const QString strNotDeleted = QString("(match-all (and (not (system-flag \"deleted\")) (not (system-flag \"junk\")))) ");

    const QString sortString(EmailMessageListModel::SortBy sortById)
    {
        switch (sortById)
        {
        case EmailMessageListModel::SortDate:
            return QString ("date");
        case EmailMessageListModel::SortSubject:
            return QString ("subject");
        case EmailMessageListModel::SortSender:
            return QString ("sender");
        default:
            //Q_ASSERT (false);
            return QString ("date");
        }
    }

    bool sortInfoFunction (const CamelMessageInfoVariant &info1, const CamelMessageInfoVariant &info2, int id, bool asc = true)
    {
            bool ret = false;

            if (id == EmailMessageListModel::SortSender) {
                ret = info1.from < info2.from;
            } else if (id == EmailMessageListModel::SortSubject) {
                ret = info1.subject < info2.subject;
            } else if (id == EmailMessageListModel::SortDate) {
                ret = info1.date_received < info2.date_received;
            } else {
                qCritical() << Q_FUNC_INFO << "Unsupported sort mode";
                return ret;
            }
            return (ret == asc);
    }

    bool sortInfoSenderFunction (const CamelMessageInfoVariant &info1, const CamelMessageInfoVariant &info2)
    {
            return sortInfoFunction (info1, info2, EmailMessageListModel::SortSender);
    }

    bool sortInfoSubFunction (const CamelMessageInfoVariant &info1, const CamelMessageInfoVariant &info2)
    {
            return sortInfoFunction (info1, info2, EmailMessageListModel::SortSubject);
    }

    bool sortInfoDateFunction (const CamelMessageInfoVariant &info1, const CamelMessageInfoVariant &info2)
    {
            return sortInfoFunction (info1, info2, EmailMessageListModel::SortDate);
    }

    class LessThan
    {
    public:
        LessThan(const QHash<QString, CamelMessageInfoVariant>& infos, EmailMessageListModel::SortBy sortById) : mInfos(infos), mSortById(sortById)
        {
                Q_ASSERT (mInfos);
        }

        bool operator()(const QString& uid1, const QString& uid2) const
        {
                if (mInfos.contains(uid1) && mInfos.contains(uid2)) {
                        return sortInfoFunction(mInfos.value(uid1), mInfos.value(uid2), mSortById, false);
                } else {
                        qDebug() << Q_FUNC_INFO << "there is no uid in cache" << uid1 << uid2;
                        return false;
                }
        }

    private:
        const QHash<QString, CamelMessageInfoVariant>& mInfos;
        const EmailMessageListModel::SortBy mSortById;
    };

    template<typename T>
    QList<T> reverse(const QList<T> &l)
    {
            QList<T> ret;

            for (int i=0 ; i<l.size(); i++)
                    ret.prepend(l.at(i));
            return ret;
    }

    bool sortInfoSenderFunctionDes (const CamelMessageInfoVariant &info1, const CamelMessageInfoVariant &info2)
    {
            return sortInfoFunction (info1, info2, EmailMessageListModel::SortSender, false);
    }
    bool sortInfoSubFunctionDes (const CamelMessageInfoVariant &info1, const CamelMessageInfoVariant &info2)
    {
            return sortInfoFunction (info1, info2, EmailMessageListModel::SortSubject, false);
    }
    bool sortInfoDateFunctionDes (const CamelMessageInfoVariant &info1, const CamelMessageInfoVariant &info2)
    {
            return sortInfoFunction (info1, info2, EmailMessageListModel::SortDate, false);
    }

    const char* UUID_property = "UUID";
}

typedef enum _CamelMessageFlags {
	CAMEL_MESSAGE_ANSWERED = 1<<0,
	CAMEL_MESSAGE_DELETED = 1<<1,
	CAMEL_MESSAGE_DRAFT = 1<<2,
	CAMEL_MESSAGE_FLAGGED = 1<<3,
	CAMEL_MESSAGE_SEEN = 1<<4,

	/* these aren't really system flag bits, but are convenience flags */
	CAMEL_MESSAGE_ATTACHMENTS = 1<<5,
	CAMEL_MESSAGE_ANSWERED_ALL = 1<<6,
	CAMEL_MESSAGE_JUNK = 1<<7,
	CAMEL_MESSAGE_SECURE = 1<<8,
	CAMEL_MESSAGE_USER_NOT_DELETABLE = 1<<9,
	CAMEL_MESSAGE_HIDDEN = 1<<10,
	CAMEL_MESSAGE_NOTJUNK = 1<<11,
	CAMEL_MESSAGE_FORWARDED = 1<<12,

	/* following flags are for the folder, and are not really permanent flags */
	CAMEL_MESSAGE_FOLDER_FLAGGED = 1<<16, /* for use by the folder implementation */
	/* flags after 1<<16 are used by camel providers,
           if adding non permanent flags, add them to the end  */

	CAMEL_MESSAGE_JUNK_LEARN = 1<<30, /* used when setting CAMEL_MESSAGE_JUNK flag
					     to say that we request junk plugin
					     to learn that message as junk/non junk */
	CAMEL_MESSAGE_USER = 1<<31 /* supports user flags */
} CamelMessageFlags;

void   append_part_to_string (QByteArray &str, CamelMimePart *part)
{
	CamelStream *stream;
	GByteArray *ba;
	CamelStream *filter_stream = NULL;
	CamelMimeFilter *charenc = NULL;
	const char *charset;
 
	stream = camel_stream_mem_new ();
	filter_stream = camel_stream_filter_new (stream);
	charset = camel_content_type_param (((CamelDataWrapper *) part)->mime_type, "charset");
	charenc = camel_mime_filter_charset_new (charset, "UTF-8");
	camel_stream_filter_add (CAMEL_STREAM_FILTER (filter_stream), charenc);
	g_object_unref (charenc);
	g_object_unref (stream);

	camel_data_wrapper_decode_to_stream ((CamelDataWrapper *)part, filter_stream, NULL);
	camel_stream_close(filter_stream, NULL);
	
	ba = camel_stream_mem_get_byte_array ((CamelStreamMem *)stream);
	str.append ((const char *)ba->data, ba->len);
}

void parseMultipartBody (QByteArray &reparray, CamelMimePart *mpart, bool plain)
{
	int parts, i;
	CamelContentType *ct;
	const char *textType = plain ? "plain" : "html";
	CamelDataWrapper *containee;

	containee = camel_medium_get_content (CAMEL_MEDIUM(mpart));
	ct = ((CamelDataWrapper *)containee)->mime_type;
	if (CAMEL_IS_MULTIPART(containee)) {
		parts = camel_multipart_get_number(CAMEL_MULTIPART(containee));
		for (i=0;i<parts;i++) {
			CamelMimePart *part = camel_multipart_get_part(CAMEL_MULTIPART(containee), i);

			//ct = ((CamelDataWrapper *)subc)->mime_type;
			parseMultipartBody (reparray, part, plain);
//	                if (camel_content_type_is(ct, "text", textType) && camel_mime_part_get_filename(part) == NULL) {
//				append_part_to_string (reparray, (CamelMimePart *)subc);
//	                }

		}
	} else  {
		if (camel_content_type_is(ct, "text", textType) && camel_mime_part_get_filename(mpart) == NULL) {
			append_part_to_string (reparray, (CamelMimePart *)containee);
		}


	}
}

QString EmailMessageListModel::mimeMessage (QString &uid)
{
    QString qmsg = (*m_messages)[uid];
	if (qmsg.isEmpty()) {
            QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(m_folder_proxy->getMessage(uid));
            watcher->setProperty(UUID_property, uid);
            connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(onMessageDownloadCompleted(QDBusPendingCallWatcher*)));
            connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), watcher, SLOT(deleteLater()));
        } else {
            qDebug() << "Got message from cache " << uid;
	}

	return qmsg;
}

QString EmailMessageListModel::bodyText(QString &uid, bool plain) const
{
    	QDBusPendingReply<QString> reply;
	QString qmsg;
	QByteArray reparray;
	CamelMimeMessage *message;
	CamelStream *stream;

	qmsg = ((EmailMessageListModel *)this)->mimeMessage (uid);

	message = camel_mime_message_new();
	stream = camel_stream_mem_new_with_buffer (qmsg.toLocal8Bit().constData(), qmsg.length());
	camel_data_wrapper_construct_from_stream ((CamelDataWrapper *) message, stream, NULL);
	camel_stream_reset (stream, NULL);
	g_object_unref(stream);

	parseMultipartBody (reparray, (CamelMimePart *)message, plain);
 	g_object_unref (message);

	return QString::fromUtf8(reparray);
}

void EmailMessageListModel::addPendingFolderOp(AsyncOperation *op)
{
    addPendingOpToList(m_pending_folder_ops, op);
}

void EmailMessageListModel::addPendingOpToList(AsyncOperationList &list, AsyncOperation *op)
{
    Q_ASSERT (op);
    QMutableListIterator<QPointer<AsyncOperation> > i(list);
    while (i.hasNext()) {
        QPointer<AsyncOperation> val = i.next();
        if (val.isNull()) {
            i.remove();
        }
    }

    list.append(QPointer<AsyncOperation>(op));
}

void EmailMessageListModel::createChecksum()
{
	GChecksum *checksum;
	guint8 *digest;
	gsize length;
	char buffer[9];
	gint state = 0, save = 0;

	length = g_checksum_type_get_length (G_CHECKSUM_MD5);
	digest = (guint8 *)g_alloca (length);

	checksum = g_checksum_new (G_CHECKSUM_MD5);
	g_checksum_update  (checksum, (const guchar *)m_current_folder.toLocal8Bit().constData(), -1);

	g_checksum_get_digest (checksum, digest, &length);
	g_checksum_free (checksum);

	g_base64_encode_step (digest, 6, FALSE, buffer, &state, &save);
	g_base64_encode_close (FALSE, buffer, &state, &save);
	buffer[8] = 0;
	
	m_current_hash = QString ((const char *)buffer);
	
    	return;
    
}


void parseMultipartAttachmentName (QStringList &attachments, CamelMimePart *mpart, CamelMimePart *top)
{
	int parts, i;
	CamelDataWrapper *containee;

	containee = camel_medium_get_content (CAMEL_MEDIUM(mpart));
	if (CAMEL_IS_MULTIPART(containee)) {
		parts = camel_multipart_get_number(CAMEL_MULTIPART(containee));
		for (i=0;i<parts;i++) {
			CamelMimePart *part = camel_multipart_get_part(CAMEL_MULTIPART(containee), i);

			parseMultipartAttachmentName (attachments, part, top);
		}
	} else if (camel_mime_part_get_filename(mpart) != NULL) {
		attachments << QString::fromUtf8 (camel_mime_part_get_filename(mpart));
		qDebug() << "Attachment : " << QString (camel_mime_part_get_filename(mpart));
	} else if (mpart != top) {
		/* Check content type see if the attachment is a email */
	}
}

//![0]
EmailMessageListModel::EmailMessageListModel(QObject *parent)
    : QAbstractItemModel (parent), m_pending_account_init(false)
{
    QHash<int, QByteArray> roles;
    roles[MessageAddressTextRole] = "sender";
    roles[MessageSubjectTextRole] = "subject";
    roles[MessageFilterTextRole] = "messageFilter";
    roles[MessageTimeStampTextRole] = "timeStamp";
    roles[MessageSizeTextRole] = "size";
    roles[MessageTypeIconRole] = "icon";
    roles[MessageStatusIconRole] = "statusIcon";
    roles[MessageDirectionIconRole] = "directionIcon";
    roles[MessagePresenceIconRole] = "presenceIcon";
    roles[MessageBodyTextRole] = "body";
    roles[MessageIdRole] = "messageId";
    roles[MessageHighPriorityRole] = "highPriority";
    roles[MessageAttachmentCountRole] = "numberOfAttachments";
    roles[MessageAttachmentsRole] = "listOfAttachments";
    roles[MessageRecipientsRole] = "recipients";
    roles[MessageRecipientsDisplayNameRole] = "recipientsDisplayName";
    roles[MessageReadStatusRole] = "readStatus";
    roles[MessageHtmlBodyRole] = "htmlBody";
    roles[MessageQuotedBodyRole] = "quotedBody";
    roles[MessageUuidRole] = "messageUuid";
    roles[MessageSenderDisplayNameRole] = "senderDisplayName";
    roles[MessageSenderEmailAddressRole] = "senderEmailAddress";
    roles[MessageCcRole] = "cc";
    roles[MessageBccRole] = "bcc";
    roles[MessageTimeStampRole] = "qDateTime";
    roles[MessageSelectModeRole] = "selected";
    roles[MessageMimeId] = "mimeMessageId";
    roles[MessageReferences] = "messageReferences";
    setRoleNames(roles);

    m_folder_proxy = NULL;
    m_account = NULL;
    m_store_proxy = NULL;
    m_lstore_proxy = NULL;

    m_sortById = EmailMessageListModel::SortDate;
    m_sortKey = 1;
    m_messages = new QHash <QString, QString>;
    timer = new QTimer(this);
    search_str = QString();
    connect(timer, SIGNAL(timeout()), this, SLOT(updateSearch()));

    session_instance = new OrgGnomeEvolutionDataserverMailSessionInterface (QString ("org.gnome.evolution.dataserver.Mail"),
                                                               QString ("/org/gnome/evolution/dataserver/Mail/Session"),
                                                               QDBusConnection::sessionBus(), parent);

    QObject::connect (session_instance, SIGNAL(sendReceiveComplete()), this, SIGNAL(sendReceiveCompleted()));

}

EmailMessageListModel::~EmailMessageListModel()
{
    delete session_instance;
}

int EmailMessageListModel::rowCount(const QModelIndex & parent) const {
    Q_UNUSED(parent);
    return shown_uids.length();
}

QVariant EmailMessageListModel::data(const QModelIndex & index, int role) const {
    int row = index.row ();
    const QHash<int, QByteArray> roles = roleNames();

    return  mydata(row, role);
}

QVariant EmailMessageListModel::mydata(int row, int role) const {
	
    QString iuid;
    CamelMessageInfoVariant minfo; 

    if (row  >= shown_uids.length())
        return QVariant();

    iuid = shown_uids[row];
    minfo = m_infos[iuid];

    if (role == MessageTimeStampTextRole)
    {
        QDateTime timeStamp = QDateTime::fromTime_t ((uint) minfo.date_sent);	    
        return (timeStamp.toString("hh:mm MM/dd/yyyy"));
    }
    else if (role == MessageSubjectTextRole) 
    {
	return QVariant(minfo.subject);
    }    
    else if (role == MessageHighPriorityRole)
    {
        // The camel API currently has a very limited concept of
        // message importance.
        return bool(minfo.flags & CAMEL_MESSAGE_FLAGGED);
    }
    else if (role == MessageAttachmentCountRole)
    {
	int numberOfAttachments = 0;
	
	if ((minfo.flags & CAMEL_MESSAGE_ATTACHMENTS) != 0)
		numberOfAttachments = 1; /* For now show just the presence of attachments */

        return numberOfAttachments;
    }
    else if (role == MessageAttachmentsRole)
    {
    	QDBusPendingReply<QString> reply;
	QString qmsg;
	CamelMimeMessage *message;
	CamelStream *stream;

	if ((minfo.flags & CAMEL_MESSAGE_ATTACHMENTS) == 0)
		return QStringList();

	qmsg = ((EmailMessageListModel *)this)->mimeMessage(iuid);

	message = camel_mime_message_new();
	stream = camel_stream_mem_new_with_buffer (qmsg.toLocal8Bit().data(), qmsg.length());
	
	camel_data_wrapper_construct_from_stream ((CamelDataWrapper *) message, stream, NULL);
	camel_stream_reset (stream, NULL);
	g_object_unref(stream);

        QStringList attachments;
	parseMultipartAttachmentName (attachments, (CamelMimePart *)message, (CamelMimePart *)message);
	g_object_unref (message);
        return attachments;
    }
    else if (role == MessageRecipientsRole)
    {
	QStringList recipients;
	QStringList mto;
	
	mto = minfo.to.split (">,");
	if (mto.length() == 1)
		mto = minfo.to.split (">");
	foreach (QString str, mto) {
		QStringList email = str.split ("<", QString::KeepEmptyParts);
		if (email.length() == 1)
			recipients << email[0];
		else 
			recipients << email[1];

	}
        return recipients;
    }
    else if (role == MessageRecipientsDisplayNameRole)
    {
        QStringList recipients;
        QStringList mto = minfo.to.split (">,");

        foreach (QString str, mto) {
                QStringList email = str.split ("<", QString::KeepEmptyParts);
		if (!email[0].isEmpty())
	                recipients << email[0];
		else
			recipients << email[1]; //Append email if name isn't present.
        }
        return recipients;
    }
    else if (role == MessageReadStatusRole)
    {
        if (minfo.flags & CAMEL_MESSAGE_SEEN)
		return 1;
	else
		return 0;
    }
    else if (role == MessageBodyTextRole)
    {
        QString uid = shown_uids[row];
        return bodyText (uid, TRUE);
    } 
    else if (role == MessageMimeId) 
    {
        QString uid = shown_uids[row];
	QString qmsg;
	CamelMimeMessage *message;
	CamelStream *stream;
	QString mid;

	qmsg = ((EmailMessageListModel *)this)->mimeMessage(uid);

	message = camel_mime_message_new();
	stream = camel_stream_mem_new_with_buffer (qmsg.toLocal8Bit().data(), qmsg.length());
	camel_data_wrapper_construct_from_stream ((CamelDataWrapper *) message, stream, NULL);
	camel_stream_reset (stream, NULL);
	g_object_unref(stream);

	mid = QString((char *)camel_medium_get_header (CAMEL_MEDIUM (message), "Message-ID"));
	g_object_unref (message);
	return QVariant(mid);

    }
    else if (role == MessageReferences)
    {
        QString uid = shown_uids[row];
	QString qmsg;
	CamelMimeMessage *message;
	CamelStream *stream;
	QString references;

	qmsg = ((EmailMessageListModel *)this)->mimeMessage(uid);

	message = camel_mime_message_new();
	stream = camel_stream_mem_new_with_buffer (qmsg.toLocal8Bit().data(), qmsg.length());
	camel_data_wrapper_construct_from_stream ((CamelDataWrapper *) message, stream, NULL);
	camel_stream_reset (stream, NULL);
	g_object_unref(stream);

	references = QString((char *)camel_medium_get_header (CAMEL_MEDIUM (message), "References"));
	g_object_unref (message);
	return QVariant(references);
    }
    else if (role == MessageHtmlBodyRole)
    {
        QString uid = shown_uids[row];
        return bodyText (uid, FALSE);
    }
    else if (role == MessageQuotedBodyRole)
    {	
	QString uid = shown_uids[row];
	QString body = bodyText(uid, TRUE);
	body.prepend('\n');
	body.replace('\n', "\n>");
	body.truncate(body.size() - 1);  // remove the extra ">" put there by QString.replace
	return body;
    }
    else if (role == MessageUuidRole || role == MessageIdRole)
    {
	QString uuid = m_current_hash +  minfo.uid;
        return uuid;
    }
    else if (role == MessageSenderDisplayNameRole)
    {
	QString str = minfo.from;
	QStringList email = str.split ("<", QString::KeepEmptyParts);

        if (email.size() >= 1)
            return email[0];
        else
            return ("");
    }
    else if (role == MessageSenderEmailAddressRole || role == MessageAddressTextRole)
    {
        QString str = minfo.from;
        QString address;
        QStringList email = str.split ("<", QString::KeepEmptyParts);

        if (email.size() > 1)
        {
	    address = email[1];
	    address.chop (1);
            return address;
        }
        else
            return str;
    }
    else if (role == MessageCcRole)
    {
        QStringList mcc = minfo.cc.split (">,");
        foreach (QString str, mcc) {
		str += ">";
        }

        return mcc;
    }
    else if (role == MessageBccRole)
    {
        return QStringList ();
    }
    else if (role == MessageTimeStampRole)
    {
        QDateTime timeStamp = QDateTime::fromTime_t ((uint) minfo.date_sent);
        return (timeStamp.toLocalTime());	    
    }
    else if (role == MessageSelectModeRole)
    {
       int selected = 0;
       QString uid = shown_uids[row];
       if (m_selectedMsgIds.contains(uid) == true)
           selected = 1;
        return (selected);
    } else {
	const QHash<int, QByteArray> roles = roleNames();
	g_print("Implement to fetch role: %s\n", (const char *)roles[role].constData());
    }

    return QVariant();
}

void EmailMessageListModel::updateSearch ()
{
    Q_ASSERT (m_folder_proxy);
    qDebug() << "Search for: " << search_str << "really began";
    SearchSortByExpression* op = new SearchSortByExpression(m_folder_proxy, this);
    op->setQuery(search_str); op->setSort(sortString(m_sortById));
    connect(op, SIGNAL(result(QStringList)), this, SLOT(onFolderUidsReset(QStringList)));
    connect(op, SIGNAL(finished()), op, SLOT(deleteLater()));
    op->start();

    timer->stop();
}

void EmailMessageListModel::setSearch(const QString& search)
{
    if (!m_folder_proxy)
	return;

    search_str = QString("(match-all (and "
                                 "(not (system-flag \"deleted\")) "
                                 "(not (system-flag \"junk\")) "
                                 "(or"
                                         "(header-contains \"From\" \"%1\")"
                                         "(header-contains \"To\" \"%1\")"
                                         "(header-contains \"Cc\" \"%1\")"
                                         "(header-contains \"Bcc\" \"%1\")"
                                         "(header-contains \"Subject\" \"%1\"))))").arg(search);
    qDebug() << "Search for: " << search_str;
    /* Start search when the user gives a keyb bread, in 1.5 secs*/
    timer->stop();
    timer->start(1500);
}

void EmailMessageListModel::sendReceive()
{
	const char *url;

	url = e_account_get_string (m_account, E_ACCOUNT_SOURCE_URL);
	if (m_current_folder.endsWith ("Outbox", Qt::CaseInsensitive)) {
		session_instance->sendReceive();
	} else if  (strncmp (url, "pop:", 4) == 0) {
		/* Fetch entire account for POP */
		session_instance->fetchAccount (m_account->uid);
	} else {
		/* For rest just do refreshInfo */
		m_folder_proxy->refreshInfo();
	}	
	
	emit sendReceiveBegin();
}

void EmailMessageListModel::cancelOperations()
{	
	emit sendReceiveCompleted();
	session_instance->cancelOperations();
}

void EmailMessageListModel::setFolderKey (QVariant id)
{
    const QString& newFolder = id.toString();
    if (m_current_folder == newFolder || newFolder.isEmpty()) {
        return;
    }
    cancelPendingFolderOperations();

    GetFolder* op = new GetFolder(m_store_proxy, m_lstore_proxy, this);
    op->setKnownFolders(m_folders); op->setFolderName(newFolder);
    connect (op, SIGNAL(result(QString,QString)), this, SLOT(setFolder(QString,QString)));
    connect (op, SIGNAL(finished()), op, SLOT(deleteLater()));
    op->start();
    addPendingFolderOp(op);
}

void EmailMessageListModel::onFolderUidsReset(const QStringList &uids)
{
        beginRemoveRows (QModelIndex(), 0, shown_uids.length()-1);
        shown_uids.clear ();
        folder_uids.clear();
        endRemoveRows ();
        folder_uids = uids;

        messages_present = (folder_uids.length() >= WINDOW_LIMIT);
        // now we got new uids and obtaining meta-data for them
        GetMessageInfo* op = new GetMessageInfo(m_folder_proxy, this);
        op->setMessageUids(folder_uids);
        connect(op, SIGNAL(finished()), op, SLOT(deleteLater()));
        connect(op, SIGNAL(result(CamelMessageInfoVariant)),this, SLOT(messageInfoAdded(CamelMessageInfoVariant)));
        op->start(WINDOW_LIMIT);
        addPendingFolderOp(op);
        emit folderReset();
}

void EmailMessageListModel::messageInfoAdded(const CamelMessageInfoVariant &info)
{
        const QString& uid = info.uid;
        if (!folder_uids.contains(uid)) {
            qWarning() << Q_FUNC_INFO << "obtained info does not fit to current folder";
            return;
        }

        if (shown_uids.contains(uid)) {
            qWarning() << Q_FUNC_INFO << uid << "already shown";
            return;
        }

        m_infos.insert (uid, info);

        LessThan lessThan(m_infos, m_sortById);
        QList<QString>::iterator iter = qLowerBound(shown_uids.begin(), shown_uids.end(), uid, lessThan);
        const int index = iter - shown_uids.begin();
        Q_ASSERT (index >= 0);
        beginInsertRows(QModelIndex(), index, index);
        qDebug() << "Adding UID: " << info.uid << "at:" << index;
        shown_uids.insert(iter, uid);
        endInsertRows();
}

void EmailMessageListModel::messageInfoUpdated(const CamelMessageInfoVariant &info)
{
    const QString& uid = info.uid;
    if (!shown_uids.contains(uid)) {
        /* This UID isn't displayed. Forsafe we'll remove any stale info if present.*/
        m_infos.remove (uid);
        return;
    }

    m_infos.insert (uid, info);
    if ((info.flags & CAMEL_MESSAGE_DELETED) != 0) {
            /* Message is deleted, it should be off the list now */
            int index;
            qDebug() << "Removing UID of msg with deleted flag set : " << uid;

            index = shown_uids.indexOf(uid);
            if (index != -1) {
                    beginRemoveRows (QModelIndex(), index, index);
                    shown_uids.removeAt(index);
                    endRemoveRows ();
            }
    } else {
            QModelIndex idx = createIndex (shown_uids.indexOf(uid), 0);
            emit dataChanged(idx, idx);
    }
}

void EmailMessageListModel::setFolder(const QString& newFolder, const QString& objectPath)
{
    if (m_current_folder == newFolder || newFolder.isEmpty()) {
        return;
    }

    m_current_folder = newFolder;
    createChecksum();

    messages_present = true;

    /* Clear message list before you load a folder. */
    beginRemoveRows (QModelIndex(), 0, shown_uids.length()-1);
    shown_uids.clear();
    endRemoveRows ();
    folder_uids.clear();
    m_infos.clear();
    m_messages->clear();

    if (m_folder_proxy)
        delete m_folder_proxy;

    m_folder_proxy = new OrgGnomeEvolutionDataserverMailFolderInterface (QString ("org.gnome.evolution.dataserver.Mail"),
                                                                        objectPath,
                                                                        QDBusConnection::sessionBus(), this);
    connect (m_folder_proxy, SIGNAL(FolderChanged(const QStringList &, const QStringList &, const QStringList &, const QStringList &)),
                                                this, SLOT(onFolderChanged(const QStringList &, const QStringList &, const QStringList &, const QStringList &)), Qt::UniqueConnection);
    SearchSortByExpression* op = new SearchSortByExpression(m_folder_proxy, this);
    op->setQuery(strNotDeleted); op->setSort(sortString(m_sortById));
    connect(op, SIGNAL(result(QStringList)), this, SLOT(onFolderUidsReset(QStringList)));
    connect(op, SIGNAL(finished()), op, SLOT(deleteLater()));
    connect(op, SIGNAL(finished()), this, SLOT(sendReceive()), Qt::QueuedConnection);
    op->start();
    addPendingFolderOp(op);
}

void EmailMessageListModel::setAccount(EAccount* account, const QString& objectPath)
{
    qDebug() << Q_FUNC_INFO << account->name;
    m_folders.clear();
    Q_ASSERT (account);
    m_account = account;
    m_store_proxy = new OrgGnomeEvolutionDataserverMailStoreInterface (QString ("org.gnome.evolution.dataserver.Mail"),
                                                                    objectPath,
                                                                    QDBusConnection::sessionBus(), this);

    InitFolders* op  = new InitFolders(account, m_store_proxy, this);
    Q_ASSERT (m_lstore_proxy);
    op->setLocalStore(m_lstore_proxy);
    connect(op, SIGNAL(finished()), op, SLOT(deleteLater()));
    connect(op, SIGNAL(result(CamelFolderInfoArrayVariant)), this, SLOT(onAccountFoldersFetched(CamelFolderInfoArrayVariant)));
    op->start();
    addPendingOpToList(m_pending_account_ops, op);
}

void EmailMessageListModel::onAccountFoldersFetched(const CamelFolderInfoArrayVariant& folders)
{
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT (m_folders.isEmpty());
    m_folders.append(folders);
    emit accountReset(); // account is finally set
}

bool EmailMessageListModel::stillMoreMessages ()
{
	return messages_present;
}

void EmailMessageListModel::getMoreMessages ()
{
        g_print(" SHOWN: %d, total : %d\n", shown_uids.length(), folder_uids.length());

        const int alreadyLoaded = folder_uids.length() - shown_uids.length();
        const int toLoad = WINDOW_LIMIT - alreadyLoaded;

        if (alreadyLoaded > 0 ) {
                qDebug() << Q_FUNC_INFO << "Showing from cache";
                GetMessageInfo* op = new GetMessageInfo(m_folder_proxy, this);
                QStringList toRetrieve((folder_uids.toSet() - shown_uids.toSet()).toList());
                op->setMessageUids(toRetrieve);
                connect(op, SIGNAL(finished()), op, SLOT(deleteLater()));
                connect(op, SIGNAL(result(CamelMessageInfoVariant)),this, SLOT(messageInfoAdded(CamelMessageInfoVariant)));

                if (toLoad <= 0) {
                    connect(op, SIGNAL(finished()), this, SIGNAL(messageRetrievalCompleted()));
                    op->start(WINDOW_LIMIT);
                } else {
                    op->start(alreadyLoaded);
                }
        }

        if (toLoad > 0){
                qDebug() <<  Q_FUNC_INFO << "Fetching more from server";
		/* See if we can fetch more mails & again search/sort and show */

                QDBusAbstractInterface* storageInterface = m_folder_proxy;
                const char *url = e_account_get_string (m_account, E_ACCOUNT_SOURCE_URL);
                if (strncmp (url, "pop:", 4) == 0) {
                   storageInterface = session_instance;
                }

                QDBusMessage msg = QDBusMessage::createMethodCall(storageInterface->service(), storageInterface->path(), storageInterface->interface(), QString("fetchOldMessages"));
                msg.setArguments(QVariantList() << QVariant(toLoad));

                /* Make this a async call with 10min timeout. At times it takes that time to download messages */
                QDBusPendingCall reply = m_folder_proxy->connection().asyncCall (msg, 10 * 60 * 1000);

                QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
                connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SIGNAL(sendReceiveCompleted()), Qt::QueuedConnection);
                connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), watcher, SLOT(deleteLater()));
	}
}

void EmailMessageListModel::onFolderChanged(const QStringList &added, const QStringList &removed, const QStringList &changed, const QStringList &recent)
{
	qDebug () << "Folder changed event: " << added.length() << " " << removed.length() << " " << changed.length() << " " << recent.length();

        if (shown_uids.isEmpty() && added.length() >=  WINDOW_LIMIT)
                messages_present = true;// we really need it?????

        if (!added.isEmpty()) {
            folder_uids << added;
            GetMessageInfo* op = new GetMessageInfo(m_folder_proxy, this);
            op->setMessageUids(added);
            connect(op, SIGNAL(finished()), this, SIGNAL(folderChanged()));
            connect(op, SIGNAL(finished()), op, SLOT(deleteLater()));
            connect(op, SIGNAL(result(CamelMessageInfoVariant)),this, SLOT(messageInfoAdded(CamelMessageInfoVariant)));
            op->start();
        }

        if (!changed.isEmpty()) {
            GetMessageInfo* op = new GetMessageInfo(m_folder_proxy, this);
            op->setMessageUids(changed);
            connect(op, SIGNAL(finished()), this, SIGNAL(folderChanged()));
            connect(op, SIGNAL(finished()), op, SLOT(deleteLater()));
            connect(op, SIGNAL(result(CamelMessageInfoVariant)),this, SLOT(messageInfoUpdated(CamelMessageInfoVariant)));
            op->start();
        }

        if (!removed.isEmpty()) {
            foreach (const QString& uid, removed) {
                    /* Removed uid */
                    int index;
                    qDebug() << "Removing UID: " << uid;

                    index = shown_uids.indexOf(uid);
                    if (index != -1) {
                            beginRemoveRows (QModelIndex(), index, index);
                            shown_uids.removeAt(index);
                            endRemoveRows ();
                    }

                    index = folder_uids.indexOf(uid);
                    m_infos.remove (uid);
                    folder_uids.removeAt(index);
            }
            emit folderChanged();
        }
}

QString EmailMessageListModel::accountKey() const
{
    if (m_account &&  m_account->uid) {
        return  m_account->uid;
    }
    return QString();
}

void EmailMessageListModel::setAccountKey (QVariant id)
{

    const QString& aid = id.toString ();

    if (aid == accountKey()) {
        qDebug() << Q_FUNC_INFO << "Setting same account";
        return;
    }
	 
    qDebug() << "Set Account: " << aid;
    if (aid.isEmpty())
	return;

    cancelPendingFolderOperations();
    cancelPendingOperations(m_pending_account_ops);

    if (!m_lstore_proxy) { // questionable ..??
	QDBusPendingReply<QDBusObjectPath> reply;
	reply = session_instance->getLocalStore();
        m_lstore_proxy_id = reply.value();

        m_lstore_proxy = new OrgGnomeEvolutionDataserverMailStoreInterface (QString ("org.gnome.evolution.dataserver.Mail"),
									m_lstore_proxy_id.path(),
									QDBusConnection::sessionBus(), this);
    }

    GetAccount* op = new GetAccount(session_instance, this);
    op->setAccountName(aid);
    connect(op, SIGNAL(finished()), op, SLOT(deleteLater()));
    connect(op, SIGNAL(result(EAccount*,QString)),this, SLOT(setAccount(EAccount*,QString)));
    op->start();
}

void EmailMessageListModel::sortBySender(int key)
{
	m_sortById = EmailMessageListModel::SortSender;
	m_sortKey = key;
	
        QList<CamelMessageInfoVariant> mlist;
        foreach (QString uid, shown_uids)
                mlist << m_infos[uid];

	qSort(mlist.begin(), mlist.end(), sortInfoSenderFunction);
	if (key)
		mlist = reverse(mlist);

	beginRemoveRows (QModelIndex(), 0, shown_uids.length()-1);
	shown_uids.clear();
	endRemoveRows ();
	beginInsertRows (QModelIndex(), 0, mlist.length()-1);
	foreach (CamelMessageInfoVariant info, mlist) {
		shown_uids << info.uid;
	}
	endInsertRows();
}

void EmailMessageListModel::sortBySubject(int key)
{
	m_sortById = EmailMessageListModel::SortSubject;
	m_sortKey = key;

	QList<CamelMessageInfoVariant> mlist;
	foreach (QString uid, shown_uids)
		mlist << m_infos[uid];

	qSort(mlist.begin(), mlist.end(), sortInfoSubFunction);
	if (key)
		mlist = reverse(mlist);

        beginRemoveRows (QModelIndex(), 0, shown_uids.length()-1);
        shown_uids.clear();
        endRemoveRows ();
        beginInsertRows (QModelIndex(), 0, mlist.length()-1);
        foreach (CamelMessageInfoVariant info, mlist) {
                shown_uids << info.uid;
        }
        endInsertRows();
}

void EmailMessageListModel::sortByDate(int key)
{
	m_sortById = EmailMessageListModel::SortDate;
	m_sortKey = key;

        QList<CamelMessageInfoVariant> mlist;
        foreach (QString uid, shown_uids)
                mlist << m_infos[uid];

	qSort(mlist.begin(), mlist.end(), sortInfoDateFunction);
	if (key)
		mlist = reverse(mlist);

        beginRemoveRows (QModelIndex(), 0, shown_uids.length()-1);
        shown_uids.clear();
        endRemoveRows ();
        beginInsertRows (QModelIndex(), 0, mlist.length()-1);
        foreach (CamelMessageInfoVariant info, mlist) {
                shown_uids << info.uid;
        }
        endInsertRows();

}

void EmailMessageListModel::sortByAttachment(int key)
{
    m_sortById = EmailMessageListModel::SortAttachment;
    m_sortKey = key;
    // TDB
}

void EmailMessageListModel::checkIfListPopulatedTillUuid()
{
    int ret_row = shown_uids.indexOf (m_UuidToShow);
    if (ret_row >= 0) {
        emit listPopulatedTillUuid(ret_row, m_UuidToShow);
        m_UuidToShow = QString();
    } else if (folder_uids.indexOf(m_UuidToShow) >= 0) {
        /* We have it, lets just load till that. */
        GetMessageInfo* op = new GetMessageInfo(m_folder_proxy, this);
        QStringList toRetrieve((folder_uids.toSet() - shown_uids.toSet()).toList());
        op->setMessageUids(toRetrieve);
        const int global_idx = toRetrieve.indexOf(m_UuidToShow);
        connect(op, SIGNAL(finished()), op, SLOT(deleteLater()));
        connect(op, SIGNAL(finished()), this, SLOT(checkIfListPopulatedTillUuid()));
        connect(op, SIGNAL(result(CamelMessageInfoVariant)),this, SLOT(messageInfoAdded(CamelMessageInfoVariant)));
        op->start(global_idx);
    } else {
        qWarning() << Q_FUNC_INFO << "FAIL to show message:" << m_UuidToShow;
        m_UuidToShow = QString();
    }
}

void EmailMessageListModel::cancelPendingFolderOperations()
{
   cancelPendingOperations(m_pending_folder_ops);
}

void EmailMessageListModel::cancelPendingOperations(const AsyncOperationList& list)
{
    const int count = list.count();
    if (count > 0) {
        qWarning() << Q_FUNC_INFO << "cancelling folder ops" << count;
    }

    foreach(QPointer<AsyncOperation> op, list) {
        if (!op.isNull()) {
            op->cancel();
        }
    }
}

int EmailMessageListModel::indexOf(const QString& uuid) const
{
    return shown_uids.indexOf (uuid);
}

void EmailMessageListModel::populateListTillUuid (const QString& account, const QString& folder, const QString& uuid)
{
    m_UuidToShow = uuid;
    if (m_current_folder == folder && account == m_account->uid) {
        checkIfListPopulatedTillUuid();
    } else {
        setAccountKey(account);
        setFolderKey(folder);
        connect (this, SIGNAL(folderReset()), SLOT(checkIfListPopulatedTillUuid()), Qt::UniqueConnection);
    }
}

QVariant EmailMessageListModel::messageId (int idx)
{
    QString uid = m_current_hash+shown_uids[idx];

    return uid;
}

QVariant EmailMessageListModel::subject (int idx)
{
    QString uid = shown_uids[idx];
    CamelMessageInfoVariant info = m_infos[uid];

    return info.subject;
}

QVariant EmailMessageListModel::mailSender (int idx)
{
    return mydata(idx, MessageAddressTextRole);
}

QVariant EmailMessageListModel::timeStamp (int idx)
{
    return mydata(idx, MessageTimeStampTextRole);
}

QVariant EmailMessageListModel::body (int idx)
{
    return mydata(idx, MessageBodyTextRole);
}

QVariant EmailMessageListModel::quotedBody (int idx)
{
    return mydata(idx, MessageQuotedBodyRole);
}

QVariant EmailMessageListModel::htmlBody (int idx)
{
    return mydata(idx, MessageHtmlBodyRole);
}

QVariant EmailMessageListModel::getMessageMimeId (int idx)
{
    return mydata(idx, MessageMimeId);
}

QVariant EmailMessageListModel::getReferences (int idx)
{
    return mydata(idx, MessageReferences);
}

QVariant EmailMessageListModel::attachments (int idx)
{
    return mydata(idx, MessageAttachmentsRole);
}

QVariant EmailMessageListModel::numberOfAttachments (int idx)
{
    return mydata(idx, MessageAttachmentCountRole);
}

QVariant EmailMessageListModel::toList (int idx)
{
    return mydata(idx, MessageRecipientsRole);
}

QVariant EmailMessageListModel::recipients (int idx)
{
        QStringList recipients;
	QString uid = shown_uids[idx];
	CamelMessageInfoVariant info = m_infos[uid];

        QStringList mto = info.to.split (">,");
        foreach (QString str, mto) {
                QStringList email = str.split ("<", QString::KeepEmptyParts);
                recipients << email[1];
        }
        if (info.cc.size() > 0)
        {
		mto = info.cc.split (">,");
        	foreach (QString str, mto) {
                	QStringList email = str.split ("<", QString::KeepEmptyParts);
                    	recipients << email[1];
		}
        }

    return recipients;
}

QVariant EmailMessageListModel::ccList (int idx)
{
    return mydata(idx, MessageCcRole);
}

QVariant EmailMessageListModel::bccList (int idx)
{
    return mydata(idx, MessageBccRole);
}

QVariant EmailMessageListModel::messageRead (int idx)
{
    return mydata(idx, MessageReadStatusRole);
}

int EmailMessageListModel::messagesCount ()
{
    return rowCount();
}

int EmailMessageListModel::totalCount ()
{
    return folder_uids.length();
}

void EmailMessageListModel::deSelectAllMessages()
{
    if (m_selectedMsgIds.size() == 0)
        return;
}

void EmailMessageListModel::selectMessage( int idx )
{
    QString uid = shown_uids[idx];

    if (!m_selectedMsgIds.contains (uid))
    {
        m_selectedMsgIds.append(uid);
    dataChanged(index(idx), index(idx));
    }
}

void EmailMessageListModel::deSelectMessage (int idx )
{
    QString uid = shown_uids[idx];

    m_selectedMsgIds.removeOne(uid);
    dataChanged(index(idx), index(idx));
}

void EmailMessageListModel::moveSelectedMessageIds(QVariant vFolderId)
{
   QDBusPendingReply<QDBusObjectPath> reply ;
   QDBusObjectPath dest_folder_path;

    reply = session_instance->getFolderFromUri (vFolderId.toString());
    reply.waitForFinished();

    dest_folder_path = reply.value();
    
    m_folder_proxy->transferMessagesTo (m_selectedMsgIds, dest_folder_path, true);

    foreach (QString uid, m_selectedMsgIds)
	setMessageFlag (uid, CAMEL_MESSAGE_DELETED | CAMEL_MESSAGE_SEEN, CAMEL_MESSAGE_DELETED | CAMEL_MESSAGE_SEEN);

    m_selectedMsgIds.clear();
}

void EmailMessageListModel::deleteSelectedMessageIds()
{
    if (m_selectedMsgIds.empty())
        return;
    deleteMessages(m_selectedMsgIds);
}

void EmailMessageListModel::setMessageFlag (QString uid, uint flag, uint set)
{
	QDBusPendingReply<bool> reply;

	if (uid.length() < 1) return;
	reply = m_folder_proxy->setMessageFlags (uid, flag, set);
        //reply.waitForFinished(); just do not wait for it
}

void EmailMessageListModel::deleteMessage(QVariant id)
{
	QString uuid = id.toString();
	QString uid = uuid.right(uuid.length()-8);
   	qDebug() << "Delete message " << id.toString(); 
   	setMessageFlag (uid, CAMEL_MESSAGE_DELETED | CAMEL_MESSAGE_SEEN, CAMEL_MESSAGE_DELETED | CAMEL_MESSAGE_SEEN);
}

void EmailMessageListModel::deleteMessages(QList<QString> list)
{
	//Not used, see if UID or UUID is used and implement accordingly.
	qDebug() << "Deleting multiple messages";
	foreach (QString uid, list)
		setMessageFlag (uid, CAMEL_MESSAGE_DELETED | CAMEL_MESSAGE_SEEN, CAMEL_MESSAGE_DELETED | CAMEL_MESSAGE_SEEN);
}

void EmailMessageListModel::markMessageAsRead (QVariant id)
{
   QString uuid = id.toString();
   QString uid = uuid.right(uuid.length()-8);

   qDebug() << "mark read message " << id.toString(); 
   setMessageFlag (uid, CAMEL_MESSAGE_SEEN, CAMEL_MESSAGE_SEEN);

}

void EmailMessageListModel::markMessageAsUnread (QVariant id)
{
   QString uuid = id.toString();
   QString uid = uuid.right(uuid.length()-8);

   qDebug() << "mark unread message " << id.toString(); 
   setMessageFlag (uid, CAMEL_MESSAGE_SEEN | CAMEL_MESSAGE_DELETED, 0);

}



/*! \reimp */
QModelIndex EmailMessageListModel::index(int row, int column, const QModelIndex &parent) const
{
    return hasIndex(row, column, parent) ? createIndex(row, column) : QModelIndex();
}

/*! \reimp */
QModelIndex EmailMessageListModel::parent(const QModelIndex &idx) const
{
    return QModelIndex();

    Q_UNUSED(idx)
}

/*! \reimp */
QModelIndex EmailMessageListModel::generateIndex(int row, int column, void *ptr)
{
    return index(row, column, QModelIndex());

    Q_UNUSED(ptr)
}

int EmailMessageListModel::columnCount(const QModelIndex &idx) const
{

    return 1;
    
    Q_UNUSED(idx)
}

void EmailMessageListModel::onMessageDownloadCompleted(QDBusPendingCallWatcher* watcher)
{
    Q_ASSERT (watcher);

    if (watcher->isError()) {
        qWarning() << Q_FUNC_INFO << "dbus error occured";
    } else {
        QDBusPendingReply <QString> reply = *watcher;
        const QString& qmsg = reply.value ();
        const QString& uuid = watcher->property(UUID_property).toString();
        m_messages->insert(uuid, qmsg);
        emit messageDownloadCompleted(uuid);
    }

}

void saveMimePart (QString uri, CamelMimePart *part, bool tmp)
{
	CamelStream *fstream;
	CamelDataWrapper *dw;
	QString downloadPath; 
	GError *error;

	if (!tmp) 
		downloadPath = QDir::homePath() + "/Downloads/" + uri;
	else {
		char *dpath = g_build_filename (e_get_user_cache_dir(), "tmp", (const char *)uri.toLocal8Bit().constData(), NULL);
		downloadPath = QString(dpath);
		g_free(dpath);
	}

	QFile f(downloadPath);

	dw = camel_medium_get_content ((CamelMedium *)part);

	if (f.exists())
		f.remove();
	
	fstream = camel_stream_fs_new_with_name (downloadPath.toLocal8Bit().constData(), O_WRONLY|O_CREAT, 0700, &error);
	if (!fstream) {
		g_print("ERROR: %s\n", error->message);
	}
	camel_data_wrapper_decode_to_stream (dw, fstream, NULL);
	camel_stream_flush (fstream, NULL);
	camel_stream_close (fstream, NULL);
	g_object_unref (fstream);
	qDebug() << "Successfully saved attachment: "+uri+" in: "+downloadPath;
}

bool saveMultipartAttachment (CamelMimePart *mpart, QString uri, bool tmp)
{
	int parts, i;
	CamelDataWrapper *containee;
	bool saved = false;
	
	/* Could be a problem if a mail hierarchy has same file name attachments */
	containee = camel_medium_get_content (CAMEL_MEDIUM(mpart));
	if (CAMEL_IS_MULTIPART(containee)) {
		parts = camel_multipart_get_number(CAMEL_MULTIPART(containee));
		for (i=0;i<parts && !saved;i++) {
			CamelMimePart *part = camel_multipart_get_part(CAMEL_MULTIPART(containee), i);

			saved = saveMultipartAttachment (part, uri, tmp);
		}
	} else if (camel_mime_part_get_filename(mpart) != NULL) {

		if (uri.isEmpty()) {	
			// We have to save all of the attachments in tmp dir of Evolution. 
			QString auri = QString(camel_mime_part_get_filename(mpart));
			saveMimePart (auri, mpart, tmp);
			return false;
		} else if (strcmp (camel_mime_part_get_filename(mpart), uri.toLocal8Bit().constData()) == 0) {
			saveMimePart (uri, mpart, tmp);
			return true;
		}
	} 

	return saved;
}

void EmailMessageListModel::saveAttachmentIn (int row, QString uri, bool tmp)
{
    	QDBusPendingReply<QString> reply;
	QString qmsg;
	CamelMimeMessage *message;
	CamelStream *stream;
    	QString iuid;

	iuid = shown_uids[row];

	qmsg = mimeMessage (iuid);

	message = camel_mime_message_new();
	stream = camel_stream_mem_new_with_buffer (qmsg.toLocal8Bit().constData(), qmsg.length());
	
	camel_data_wrapper_construct_from_stream ((CamelDataWrapper *) message, stream, NULL);
	camel_stream_reset (stream, NULL);
	g_object_unref(stream);

	saveMultipartAttachment ((CamelMimePart *)message, uri, tmp);
	g_object_unref (message);
}

void EmailMessageListModel::saveAttachmentsInTemp (int row)
{
	saveAttachmentIn (row, QString(""), true);
}

void EmailMessageListModel::saveAttachment (int row, QString uri)
{
	saveAttachmentIn (row, uri, false);
}

bool EmailMessageListModel::openAttachment (int row, QString uri)
{
    bool status = true;
    
    /* Lets save it first. */
    saveAttachment (row, uri);

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

