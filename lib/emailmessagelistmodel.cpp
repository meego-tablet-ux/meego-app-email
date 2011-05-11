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
#include "emailmessagelistmodel.h"
#include <QMailMessage>
#include <QMailMessageKey>
#include <QMailStore>
#include <QMailServiceAction>
#include <QDateTime>
#include <QTimer>
#include <QProcess>
#include <qmailnamespace.h>
#include <glib.h>
#include <libedataserver/e-account-list.h>
#include <gconf/gconf-client.h>

#define WINDOW_LIMIT 20

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


#if 0
QString EmailMessageListModel::bodyHtmlText(QMailMessagePartContainer *container) const
{
    QMailMessageContentType contentType = container->contentType();

    if (container->multipartType() == QMailMessagePartContainerFwd::MultipartNone)
    {
        if (contentType.subType().toLower() == "html")
        {
            if (container->hasBody() && container->body().data().size() > 1)
                return container->body().data();
            else
            {
                connect (m_retrievalAction, SIGNAL(activityChanged(QMailServiceAction::Activity)),
                                            this, SLOT(downloadActivityChanged(QMailServiceAction::Activity)));
                QMailMessage *msg = (QMailMessage *)container;
                QMailMessageIdList ids;
                ids << msg->id();
                m_retrievalAction->retrieveMessages(ids, QMailRetrievalAction::Content);
                return " ";  // Put a space here as a place holder to notify UI that we do have html body.
                             // Should find a better way.
            }
        }
        return "";
    }

    if (!container->contentAvailable())
    {
        // if content is not available, attempts to downlaod from the server.
        connect (m_retrievalAction, SIGNAL(activityChanged(QMailServiceAction::Activity)),
                                            this, SLOT(downloadActivityChanged(QMailServiceAction::Activity)));
        QMailMessage *msg = (QMailMessage *)container;
        QMailMessageIdList ids;
        ids << msg->id();
        m_retrievalAction->retrieveMessages(ids, QMailRetrievalAction::Content);
        return " ";  // Put a space here as a place holder to notify UI that we do have html body.
    }

    QString text("");
    for ( uint i = 0; i < container->partCount(); i++ )
    {
        QMailMessagePart messagePart = container->partAt(i);
        contentType = messagePart.contentType();
        if (contentType.type().toLower() == "text" && contentType.subType().toLower() == "html")
        {
            if (messagePart.hasBody())
            {
                text += messagePart.body().data();
            }
            else
            {
                connect (m_retrievalAction, SIGNAL(activityChanged(QMailServiceAction::Activity)),
                                            this, SLOT(downloadActivityChanged(QMailServiceAction::Activity)));

                QMailMessagePart::Location location = messagePart.location();
                m_retrievalAction->retrieveMessagePart(location);
                text = " ";
                break;
            }
        }
        QMailMessagePart subPart;
        for (uint j = 0; j < messagePart.partCount(); j++)
        {
            subPart = messagePart.partAt(j);
            contentType = subPart.contentType();
            if (contentType.type().toLower() == "text" && contentType.subType().toLower() == "html")
            {
                if (subPart.hasBody())
                {
                    text += subPart.body().data();
                }
                else
                {
                    connect (m_retrievalAction, SIGNAL(activityChanged(QMailServiceAction::Activity)),
                                                this, SLOT(downloadActivityChanged(QMailServiceAction::Activity)));
                    QMailMessagePart::Location location = subPart.location();
                    m_retrievalAction->retrieveMessagePart(location);
                    text = " ";
                    break;
                }
            }
        }
    }
    return text;
}
#endif

void   append_part_to_string (QByteArray &str, CamelMimePart *part)
{
	CamelStream *stream;
	GByteArray *ba;

	stream = camel_stream_mem_new ();
	camel_data_wrapper_decode_to_stream ((CamelDataWrapper *)part, stream, NULL);
	camel_stream_close(stream, NULL);
	
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

QString EmailMessageListModel::bodyText(const QString &uid, bool plain) const
{
    	QDBusPendingReply<QString> reply;
	QString qmsg;
	QByteArray reparray;
	CamelMimeMessage *message;
	CamelStream *stream;

	qmsg = (*m_messages)[uid];
	if (qmsg.isEmpty()) {
		reply = m_folder_proxy->getMessage(uid);
		reply.waitForFinished();
		qmsg = reply.value ();
		m_messages->insert (uid, qmsg);
                qDebug() << "BT Fetching message " << uid;
        } else {
                qDebug() << "BT Got message from cache " << uid;
	}

	message = camel_mime_message_new();
	stream = camel_stream_mem_new_with_buffer (qmsg.toLocal8Bit().constData(), qmsg.length());
	camel_data_wrapper_construct_from_stream ((CamelDataWrapper *) message, stream, NULL);
	camel_stream_reset (stream, NULL);
	g_object_unref(stream);

	parseMultipartBody (reparray, (CamelMimePart *)message, plain);
 	
	return QString (reparray);
#if 0
    QMailMessagePartContainer *container = (QMailMessagePartContainer *)&mailMsg;
    QMailMessageContentType contentType = container->contentType();
    if (container->hasBody() && contentType.type().toLower() == "text" &&
               contentType.subType().toLower() == "plain")
    {
        return container->body().data();

    }

    QString text("");
    for ( uint i = 0; i < container->partCount(); i++ )
    {
        QMailMessagePart messagePart = container->partAt(i);

        contentType = messagePart.contentType();
        if (messagePart.hasBody() && contentType.type().toLower() == "text" &&
                                     contentType.subType().toLower() == "plain")
        {
            text += messagePart.body().data() + "\n";
        }
        QMailMessagePart subPart;
        for (uint j = 0; j < messagePart.partCount(); j++)
        {
            subPart = messagePart.partAt(j);
            contentType = subPart.contentType();
            if (subPart.hasBody() && contentType.type().toLower() == "text" &&
                                     contentType.subType().toLower() == "plain")
                text += subPart.body().data() + "\n";
        }
    }
    return text;
#endif
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
		attachments << QString (camel_mime_part_get_filename(mpart));
		qDebug() << "Attachment : " << QString (camel_mime_part_get_filename(mpart));
	} else if (mpart != top) {
		/* Check content type see if the attachment is a email */
	}
}

//![0]
EmailMessageListModel::EmailMessageListModel(QObject *parent)
    : QAbstractItemModel (parent)
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
    setRoleNames(roles);

    m_sortById = EmailMessageListModel::SortDate;
    m_sortKey = 1;
    m_messages = new QHash <QString, QString>;
    timer = new QTimer(this);
    search_str = QString();
    connect(timer, SIGNAL(timeout()), this, SLOT(updateSearch()));

    initMailServer();
/*
    QMailAccountIdList ids = QMailStore::instance()->queryAccounts(
            QMailAccountKey::status(QMailAccount::Enabled, QMailDataComparator::Includes),
            QMailAccountSortKey::name());

    QMailMessageKey accountKey = QMailMessageKey::parentAccountId(ids);
    QMailMessageListModel::setKey(accountKey);
    m_key = key();
    QMailMessageSortKey sortKey = QMailMessageSortKey::timeStamp(Qt::DescendingOrder);
    QMailMessageListModel::setSortKey(sortKey);
    m_selectedMsgIds.clear();
*/    
}

EmailMessageListModel::~EmailMessageListModel()
{
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

    if (row  > shown_uids.length())
        return QVariant();

    iuid = shown_uids[row];
    minfo = m_infos[iuid];

    if (role == MessageTimeStampTextRole)
    {
        QDateTime timeStamp = QDateTime::fromTime_t ((uint) minfo.date_received);	    
        return (timeStamp.toString("hh:mm MM/dd/yyyy"));
    }
    else if (role == MessageSubjectTextRole) 
    {
	return QVariant(minfo.subject);
    }    
    else if (role == MessageAttachmentCountRole)
    {
	int numberOfAttachments = 0;
	
	if ((minfo.flags & CAMEL_MESSAGE_ATTACHMENTS) != 0)
		numberOfAttachments = 1; /* For now show just the presence of attachments */
#if 0	    
        // return number of attachments
        QMailMessage message(idFromIndex(index));
        if (!message.status() & QMailMessageMetaData::HasAttachments)
            return 0;

        int numberOfAttachments = 0;
        for (uint i = 1; i < message.partCount(); i++)
        {
            QMailMessagePart sourcePart = message.partAt(i);
            if (!(sourcePart.multipartType() == QMailMessagePartContainer::MultipartNone))
                continue;

            QMailMessageContentType contentType = sourcePart.contentType();
            if (sourcePart.hasBody() && contentType.type().toLower() == "text" &&
                                     contentType.subType().toLower() == "plain")
                continue;

            if (i == 1 && contentType.type().toLower() == "text" && contentType.subType().toLower() == "html")
                continue;

            numberOfAttachments += 1;
        }
#endif
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

	qmsg =(* m_messages)[iuid];
	if (qmsg.isEmpty()) {
		reply = m_folder_proxy->getMessage(iuid);
		reply.waitForFinished();
		qmsg = reply.value ();
		m_messages->insert(QString(iuid), qmsg);
		qDebug() << "AR Fetching message " << iuid << ": " << (*m_messages)[iuid].isEmpty();
	} else {
		qDebug() << "AR Got message from cache " << iuid;
	}

	message = camel_mime_message_new();
	stream = camel_stream_mem_new_with_buffer (qmsg.toLocal8Bit().data(), qmsg.length());
	
	camel_data_wrapper_construct_from_stream ((CamelDataWrapper *) message, stream, NULL);
	camel_stream_reset (stream, NULL);
	g_object_unref(stream);

        QStringList attachments;
	parseMultipartAttachmentName (attachments, (CamelMimePart *)message, (CamelMimePart *)message);
	g_object_unref (message);
#if 0
        // return a stringlist of attachments
        QMailMessage message(idFromIndex(index));
        if (!message.status() & QMailMessageMetaData::HasAttachments)
            return QStringList();

        QStringList attachments;
        for (uint i = 1; i < message.partCount(); i++)
        {
            QMailMessagePart sourcePart = message.partAt(i);
            if (!(sourcePart.multipartType() == QMailMessagePartContainer::MultipartNone))
                continue;

            QMailMessageContentType contentType = sourcePart.contentType();
            if (sourcePart.hasBody() && contentType.type().toLower() == "text" &&
                                     contentType.subType().toLower() == "plain")
                continue;

            if (i == 1 && contentType.type().toLower() == "text" && contentType.subType().toLower() == "html")
                continue;

            attachments << sourcePart.displayName();
        }
#endif
        return attachments;
    }
    else if (role == MessageRecipientsRole)
    {
	QStringList recipients;
	QStringList mto = minfo.to.split (">,");
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
    else if (role == MessageHtmlBodyRole)
    {
        QString uid = shown_uids[row];
        //QMailMessage message (idFromIndex(index));
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
	QString name;
	QStringList email = str.split ("<", QString::KeepEmptyParts);

	if (email[0].isEmpty())
		name = email[1];
	else
		name = email[0];
        return name;
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
        QDateTime timeStamp = QDateTime::fromTime_t ((uint) minfo.date_received);
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
	QString sort;
	if (m_sortById == EmailMessageListModel::SortDate)
		sort = QString ("date");
	else if (m_sortById == EmailMessageListModel::SortSubject)
		sort = QString ("subject");
	else if (m_sortById == EmailMessageListModel::SortSender)
		sort = QString ("sender");
	else
		sort = QString ("date");

    QDBusPendingReply<QStringList> reply = m_folder_proxy->searchSortByExpression (search_str, sort, false);
    reply.waitForFinished();
    
    beginRemoveRows (QModelIndex(), 0, shown_uids.length()-1);
    shown_uids.clear ();
    endRemoveRows ();
    beginInsertRows (QModelIndex(), 0, reply.value().length()-1);
    shown_uids = reply.value ();
    endInsertRows();

    qDebug() << "Search count: "<< shown_uids.length();

    timer->stop();
}

void EmailMessageListModel::setSearch(const QString search)
{
    char *query;
    const char *str = search.toLocal8Bit().constData();
    query = g_strdup_printf("(match-all (and " 
				"(not (system-flag \"deleted\")) "
			        "(not (system-flag \"junk\")) "
				"(or" 
					"(header-contains \"From\" \"%s\")"
                      			"(header-contains \"To\" \"%s\")"
                      			"(header-contains \"Cc\" \"%s\")"
                      			"(header-contains \"Bcc\" \"%s\")"
		      			"(header-contains \"Subject\" \"%s\"))))", str, str, str, str, str);

    qDebug() << "Search for: " << search;
    
    search_str = QString(query);
    /* Start search when the user gives a keyb bread, in 1.5 secs*/
    timer->stop();
    timer->start(1500);

#if 0
    if(search.isEmpty())
    {
        setKey(QMailMessageKey::nonMatchingKey());
    }else
    {
        if(m_search == search)
            return;
        QMailMessageKey subjectKey = QMailMessageKey::subject(search, QMailDataComparator::Includes);
        QMailMessageKey toKey = QMailMessageKey::recipients(search, QMailDataComparator::Includes);
        QMailMessageKey fromKey = QMailMessageKey::sender(search, QMailDataComparator::Includes);
        setKey(m_key & (subjectKey | toKey | fromKey));
    }
    m_search = search;
#endif
}

void EmailMessageListModel::setFolderKey (QVariant id)
{
    int count=0;
    if (m_current_folder == id.toString()) {
	return;
    }
    m_current_folder = id.toString ();
    CamelFolderInfoVariant c_info;
    const char *fstr = m_current_folder.toLocal8Bit().constData();

    if (!fstr || !*fstr) {
	return;
    }
    
    qDebug() << "Set folder key: " << m_current_folder;
    foreach (CamelFolderInfoVariant info, m_folders) {
	if (m_current_folder == info.uri)
		c_info = info;	
    }

    if (m_store_proxy && m_store_proxy->isValid()) {
	QDBusPendingReply<QDBusObjectPath> reply = m_store_proxy->getFolder (QString(c_info.full_name));
	reply.waitForFinished();
	m_folder_proxy_id = reply.value ();
    }
    
    /* Clear message list before you load a folder. */
    beginRemoveRows (QModelIndex(), 0, shown_uids.length()-1);
    shown_uids.clear();
    endRemoveRows ();
    folder_uids.clear();
    m_infos.clear();
    m_messages->clear();

    m_folder_proxy = new OrgGnomeEvolutionDataserverMailFolderInterface (QString ("org.gnome.evolution.dataserver.Mail"),
                                                                        m_folder_proxy_id.path(),
                                                                        QDBusConnection::sessionBus(), this);
	{
		QString sort;
		search_str = QString("(match-all (and " 
						      "(not (system-flag \"deleted\")) "
			        		      "(not (system-flag \"junk\")))) ");
		if (m_sortById == EmailMessageListModel::SortDate)
			sort = QString ("date");
		else if (m_sortById == EmailMessageListModel::SortSubject)
			sort = QString ("subject");

		else if (m_sortById == EmailMessageListModel::SortSender)
			sort = QString ("sender");
		else
			sort = QString ("date");

		/* Don't blindly load all UIDs. Just load non-deleted and non junk only */
		QDBusPendingReply<QStringList> reply = m_folder_proxy->searchSortByExpression(search_str, sort, false);
	        reply.waitForFinished();
		folder_uids = reply.value ();
	}
	{
		QDBusPendingReply<> reply = m_folder_proxy->prepareSummary();
		reply.waitForFinished();
	}
	beginInsertRows(QModelIndex(), 0, WINDOW_LIMIT-1);
	foreach (QString uid, folder_uids) {
		QDBusError error;
		CamelMessageInfoVariant info;
		//qDebug() << "Fetching uid " << uid;
		QDBusPendingReply <CamelMessageInfoVariant> reply = m_folder_proxy->getMessageInfo (uid);
                reply.waitForFinished();
		//qDebug() << "Decoing..." << reply.isFinished() << "or error ? " << reply.isError() << " valid ? "<< reply.isValid();
		if (reply.isError()) {
			error = reply.error();	
			qDebug() << "Error: " << error.name () << " " << error.message();
			continue;
		}	
		info = reply.value ();
		m_infos.insert (uid, info);
		shown_uids << uid;
		count++;
		if (count >= WINDOW_LIMIT)
			break;
	}
	endInsertRows();

        connect (m_folder_proxy, SIGNAL(FolderChanged(const QStringList &, const QStringList &, const QStringList &, const QStringList &)),
                                            this, SLOT(myFolderChanged(const QStringList &, const QStringList &, const QStringList &, const QStringList &)));



#if 0
    m_currentFolderId = id.value<QMailFolderId>();
    if (!m_currentFolderId.isValid())
        return;
    QMailMessageKey folderKey = QMailMessageKey::parentFolderId(m_currentFolderId);
    QMailMessageListModel::setKey(folderKey);
    m_key=key();
    QMailMessageSortKey sortKey = QMailMessageSortKey::timeStamp(Qt::DescendingOrder);
    QMailMessageListModel::setSortKey(sortKey);
#endif
}

void EmailMessageListModel::getMoreMessages ()
{
	int i, count;

	count = shown_uids.length();
	beginInsertRows(QModelIndex(), count-1, count+WINDOW_LIMIT-1);
	for (i=count; i < count+WINDOW_LIMIT; i++) {
		QString uid = folder_uids[i];
		QDBusError error;
		CamelMessageInfoVariant info;
		//qDebug() << "Fetching uid " << uid;
		QDBusPendingReply <CamelMessageInfoVariant> reply = m_folder_proxy->getMessageInfo (uid);
                reply.waitForFinished();
		//qDebug() << "Decoing..." << reply.isFinished() << "or error ? " << reply.isError() << " valid ? "<< reply.isValid();
		if (reply.isError()) {
			error = reply.error();	
			qDebug() << "Error: " << error.name () << " " << error.message();
			continue;
		}	
		info = reply.value ();
		m_infos.insert (uid, info);
		shown_uids << uid;
	}
	endInsertRows();

	sortMails ();
	emit messageRetrievalCompleted();
}

void EmailMessageListModel::myFolderChanged(const QStringList &added, const QStringList &removed, const QStringList &changed, const QStringList &recent)
{
	qDebug () << "Folder changed event: " << added.length() << " " << removed.length() << " " << changed.length() << " " << recent.length();
        beginInsertRows(QModelIndex(), shown_uids.length(), shown_uids.length()+added.length()-1);
	foreach (QString uid, added) {
		/* Add uid */
		qDebug() << "Adding UID: " << uid;
		shown_uids << uid;
		folder_uids << uid;

		/* Add message info */
                QDBusError error;
                CamelMessageInfoVariant info;
                qDebug() << "Fetching uid " << uid;
                QDBusPendingReply <CamelMessageInfoVariant> reply = m_folder_proxy->getMessageInfo (uid);
                reply.waitForFinished();
                qDebug() << "Decoing..." << reply.isFinished() << "or error ? " << reply.isError() << " valid ? "<< reply.isValid();
                if (reply.isError()) {
                        error = reply.error();
                        qDebug() << "Error: " << error.name () << " " << error.message();
                        continue;
                }
                info = reply.value ();
                m_infos.insert (uid, info);

	}
        endInsertRows();

	foreach (QString uid, removed) {
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

	foreach (QString uid, changed) {
		/* Add uid */
		qDebug() << "Changed UID: " << uid;

		/* Add message info */
                QDBusError error;
                CamelMessageInfoVariant info;
                qDebug() << "Fetching uid " << uid;
                QDBusPendingReply <CamelMessageInfoVariant> reply = m_folder_proxy->getMessageInfo (uid);
                reply.waitForFinished();
                qDebug() << "Decoing..." << reply.isFinished() << "or error ? " << reply.isError() << " valid ? "<< reply.isValid();
                if (reply.isError()) {
                        error = reply.error();
                        qDebug() << "Error: " << error.name () << " " << error.message();
                        continue;
                }
                info = reply.value ();
                m_infos.insert (uid, info);
		
		if (shown_uids.indexOf(uid) != -1) {
			QModelIndex idx = createIndex (shown_uids.indexOf(uid), 0);
			emit dataChanged(idx, idx);
		}
	}

	sortMails ();
}

void EmailMessageListModel::setAccountKey (QVariant id)
{
    GConfClient *client;
    EIterator *iter;
    EAccountList *account_list;
    EAccount *acc;
    QString aid = id.toString ();
    int index=-1;

    if (m_account && m_account->uid && strcmp (m_account->uid, (const char *)id.toString().toLocal8Bit().constData()) == 0) {
	g_debug("Setting same account");
	return;
    }
	 
    qDebug() << "Set Account: " << id.toString();
    client = gconf_client_get_default ();
    account_list = e_account_list_new (client);
    g_object_unref (client);

    iter = e_list_get_iterator (E_LIST (account_list));
    while (e_iterator_is_valid (iter)) {
        index++;
        acc = (EAccount *) e_iterator_get (iter);
        if (strcmp ((const char *)aid.toLocal8Bit().constData(), acc->uid) == 0)
             m_account = acc;
        e_iterator_next (iter);
    }

    g_object_ref (m_account);
    g_object_unref (iter);
    g_object_unref (account_list);

    const char *url;

    url = e_account_get_string (m_account, E_ACCOUNT_SOURCE_URL);
OrgGnomeEvolutionDataserverMailSessionInterface *instance = OrgGnomeEvolutionDataserverMailSessionInterface::instance(this);
    if (instance && instance->isValid()) {
        QDBusPendingReply<QDBusObjectPath> reply = instance->getStore (QString(url));
        reply.waitForFinished();
        m_store_proxy_id = reply.value();
        m_store_proxy = new OrgGnomeEvolutionDataserverMailStoreInterface (QString ("org.gnome.evolution.dataserver.Mail"),
                                                                        m_store_proxy_id.path(),
                                                                        QDBusConnection::sessionBus(), this);

        if (m_store_proxy && m_store_proxy->isValid()) {
                QDBusPendingReply<CamelFolderInfoArrayVariant> reply = m_store_proxy->getFolderInfo (QString(""),
                                                                        CAMEL_STORE_FOLDER_INFO_RECURSIVE|CAMEL_STORE_FOLDER_INFO_FAST | CAMEL_STORE_FOLDER_INFO_SUBSCRIBED);
                reply.waitForFinished();
                m_folders = reply.value ();

        }
    }

    

#if 0
    QMailAccountId accountId = id.value<QMailAccountId>();
    QMailAccountIdList ids;
    if (!accountId.isValid() || id == -1)
    {
        ids = QMailStore::instance()->queryAccounts(
                QMailAccountKey::status(QMailAccount::Enabled, QMailDataComparator::Includes),
                QMailAccountSortKey::name());
    }
    else
        ids << accountId;

    QMailFolderIdList folderIdList;

    for (int i = 0; i < ids.size(); i++)
    {
        QMailFolderKey key = QMailFolderKey::parentAccountId(accountId);
        QMailFolderIdList  mailFolderIds = QMailStore::instance()->queryFolders(key);
        foreach (QMailFolderId folderId, mailFolderIds)
        {
            QMailFolder folder(folderId);
            if (QString::compare(folder.displayName(), "INBOX", Qt::CaseInsensitive) == 0)
            {
                folderIdList << folderId;
                break;
            }
        }
    }

    QMailMessageKey accountKey = QMailMessageKey::parentAccountId(ids);
    QMailMessageListModel::setKey(accountKey);

    // default to INBOX for now
    QMailMessageKey folderKey = QMailMessageKey::parentFolderId(folderIdList);
    QMailMessageListModel::setKey(folderKey);//!FIXME: should this be folderKey&accountKey?

    QMailMessageSortKey sortKey = QMailMessageSortKey::timeStamp(Qt::DescendingOrder);
    QMailMessageListModel::setSortKey(sortKey);

    m_key= key();
#endif 
}

bool sortInfoFunction (const CamelMessageInfoVariant &info1, const CamelMessageInfoVariant &info2, int id, int asc)
{
  	bool ret;

	if (id == EmailMessageListModel::SortSender) {
		ret = info1.from < info2.from;
	} else if (id == EmailMessageListModel::SortSubject) {
		ret = info1.subject < info2.subject;
        } else if (id == EmailMessageListModel::SortDate) {
		ret = info1.date_received < info2.date_received;
        } else {
		return FALSE;
	}
	if (asc)
		return ret;
	else
		return !ret;

}

bool sortInfoSenderFunction (const CamelMessageInfoVariant &info1, const CamelMessageInfoVariant &info2)
{
	return sortInfoFunction (info1, info2, EmailMessageListModel::SortSender, 1);
}
bool sortInfoSubFunction (const CamelMessageInfoVariant &info1, const CamelMessageInfoVariant &info2)
{
        return sortInfoFunction (info1, info2, EmailMessageListModel::SortSubject, 1);
}
bool sortInfoDateFunction (const CamelMessageInfoVariant &info1, const CamelMessageInfoVariant &info2)
{
        return sortInfoFunction (info1, info2, EmailMessageListModel::SortDate, 1);
}

void EmailMessageListModel::sortMails ()
{
    if (m_sortById == EmailMessageListModel::SortSender)
        sortBySender (m_sortKey);
    else if (m_sortById == EmailMessageListModel::SortSubject)
        sortBySender (m_sortKey);
    else
        sortByDate (m_sortKey);
}

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
        return sortInfoFunction (info1, info2, EmailMessageListModel::SortSender, 0);
}
bool sortInfoSubFunctionDes (const CamelMessageInfoVariant &info1, const CamelMessageInfoVariant &info2)
{
        return sortInfoFunction (info1, info2, EmailMessageListModel::SortSubject, 0);
}
bool sortInfoDateFunctionDes (const CamelMessageInfoVariant &info1, const CamelMessageInfoVariant &info2)
{
        return sortInfoFunction (info1, info2, EmailMessageListModel::SortDate, 0);
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

void EmailMessageListModel::initMailServer()
{
    return;
}

QVariant EmailMessageListModel::indexFromMessageId (QString uuid)
{
    QString uid = uuid.right(uuid.length()-8);
    CamelMessageInfoVariant info = m_infos[uid];

    for (int row = 0; row < rowCount(); row++)
    {
	if (info.uid == uid)
            return row;
    }
    return -1;
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
	mto = info.cc.split (">,");
        foreach (QString str, mto) {
                QStringList email = str.split ("<", QString::KeepEmptyParts);
                recipients << email[1];
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

void EmailMessageListModel::deSelectAllMessages()
{
    if (m_selectedMsgIds.size() == 0)
        return;
#if 0
    QMailMessageIdList msgIds = m_selectedMsgIds;
    m_selectedMsgIds.clear();
    foreach (QMailMessageId msgId,  msgIds)
    {
        for (int row = 0; row < rowCount(); row++)
        {
            QVariant vMsgId = data(index(row), QMailMessageModelBase::MessageIdRole);
    
            if (msgId == vMsgId.value<QMailMessageId>())
                dataChanged (index(row), index(row));
        }
    }
#endif    
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

void EmailMessageListModel::deleteSelectedMessageIds()
{
    if (m_selectedMsgIds.size() == 0)
        return;
    deleteMessages(m_selectedMsgIds);
}

void EmailMessageListModel::setMessageFlag (QString uid, uint flag, uint set)
{
	QDBusPendingReply<bool> reply;

	reply = m_folder_proxy->setMessageFlags (uid, flag, set);
	reply.waitForFinished();
}

void EmailMessageListModel::deleteMessage(QVariant id)
{
   	qDebug() << "Delete message " << id.toString(); 
   	setMessageFlag (id.toString(), CAMEL_MESSAGE_DELETED | CAMEL_MESSAGE_SEEN, CAMEL_MESSAGE_DELETED | CAMEL_MESSAGE_SEEN);
}

void EmailMessageListModel::deleteMessages(QList<QString> list)
{
	qDebug() << "Deleting multiple messages";
	foreach (QString uid, list)
		setMessageFlag (uid, CAMEL_MESSAGE_DELETED | CAMEL_MESSAGE_SEEN, CAMEL_MESSAGE_DELETED | CAMEL_MESSAGE_SEEN);
}

void EmailMessageListModel::markMessageAsRead (QVariant id)
{
   qDebug() << "mark read message " << id.toString(); 
   setMessageFlag (id.toString(), CAMEL_MESSAGE_SEEN, CAMEL_MESSAGE_SEEN);

}

void EmailMessageListModel::markMessageAsUnread (QVariant id)
{
   qDebug() << "mark unread message " << id.toString(); 
   setMessageFlag (id.toString(), CAMEL_MESSAGE_SEEN | CAMEL_MESSAGE_DELETED, 0);

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


void EmailMessageListModel::downloadActivityChanged(QMailServiceAction::Activity activity)
{
	Q_UNUSED (activity)
#if 0
    if (QMailServiceAction *action = static_cast<QMailServiceAction*>(sender()))
    {
        if (activity == QMailServiceAction::Successful)
        {
            if (action == m_retrievalAction)
            {
                emit messageDownloadCompleted();
            }
        }
        else if (activity == QMailServiceAction::Failed)
        {
            //  Todo:  hmm.. may be I should emit an error here.
            emit messageDownloadCompleted();
        }
    }
#endif
}

void saveMimePart (QString uri, CamelMimePart *part)
{
	QString downloadPath = QDir::homePath() + "/Downloads/" + uri;
	QFile f(downloadPath);
	CamelStream *fstream;
	CamelDataWrapper *dw;

	dw = camel_medium_get_content ((CamelMedium *)part);

	if (f.exists())
		f.remove();
	
	fstream = camel_stream_fs_new_with_name (downloadPath.toLocal8Bit().constData(), O_WRONLY|O_CREAT, 0600, NULL);
				
	camel_data_wrapper_decode_to_stream (dw, fstream, NULL);
	camel_stream_flush (fstream, NULL);
	camel_stream_close (fstream, NULL);
	g_object_unref (fstream);
	qDebug() << "Successfully saved attachment: "+uri;
}

bool saveMultipartAttachment (CamelMimePart *mpart, QString uri)
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

			saved = saveMultipartAttachment (part, uri);
		}
	} else if (camel_mime_part_get_filename(mpart) != NULL && 
			strcmp (camel_mime_part_get_filename(mpart), uri.toLocal8Bit().constData()) == 0) {
		saveMimePart (uri, mpart);
		return true;
	} 

	return saved;
}

void EmailMessageListModel::saveAttachment (int row, QString uri)
{
    	QDBusPendingReply<QString> reply;
	QString qmsg;
	CamelMimeMessage *message;
	CamelStream *stream;
    	QString iuid;

	iuid = shown_uids[row];

	qmsg =(* m_messages)[iuid];
	if (qmsg.isEmpty()) {
		reply = m_folder_proxy->getMessage(iuid);
		reply.waitForFinished();
		qmsg = reply.value ();
		m_messages->insert(QString(iuid), qmsg);
		qDebug() << "SaveAttach: Fetching message " << iuid << ": " << (*m_messages)[iuid].isEmpty();
	} else {
		qDebug() << "SaveAttach: Got message from cache " << iuid;
	}
	

	message = camel_mime_message_new();
	stream = camel_stream_mem_new_with_buffer (qmsg.toLocal8Bit().constData(), qmsg.length());
	
	camel_data_wrapper_construct_from_stream ((CamelDataWrapper *) message, stream, NULL);
	camel_stream_reset (stream, NULL);
	g_object_unref(stream);

	saveMultipartAttachment ((CamelMimePart *)message, uri);
	g_object_unref (message);
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

