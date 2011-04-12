
#ifndef DBUSTYPES_H
#define DBUSTYPES_H


#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QMetaType>
#include <QtDBus/QtDBus>

#define CAMEL_STORE_FOLDER_INFO_FAST       (1 << 0)
#define CAMEL_STORE_FOLDER_INFO_RECURSIVE  (1 << 1)
#define CAMEL_STORE_FOLDER_INFO_SUBSCRIBED (1 << 2)

struct CamelUserTagVariant 
{
   QString tag_name;
   QString tag_value;
};
Q_DECLARE_METATYPE ( CamelUserTagVariant )

  // Marshall the CamelUserTagVariant data into a D-BUS argument
 QDBusArgument &operator<<(QDBusArgument &argument, const CamelUserTagVariant &mystruct);
 // Retrieve the CamelUserTagVariant data from the D-BUS argument
 const QDBusArgument &operator>>(const QDBusArgument &argument, CamelUserTagVariant &mystruct);

/*
      <!-- Structure of CamelMessageInfoBase
         sssssss - uid, sub, from, to, cc, mlist, preview
	 uu - flags, size
	 tt - date_sent, date_received
	 t  - message_id
	 iat - references
	 as - userflags
	 a(ss) - usertags
      -->
      <arg name="info" type="(sssssssuutttiatasa(ss))" direction="in"/>	
*/

struct CamelMessageInfoVariant
{
    QString uid;
    QString subject;
    QString from;
    QString to;
    QString cc;
    QString mlist;
    QString preview;
    unsigned int flags;
    unsigned int size;
    qulonglong date_sent;
    qulonglong date_received;
    qulonglong message_id;
    int references_size;
    QList <qulonglong> references;
    QList <QString> user_flags;
    QList <CamelUserTagVariant> user_tags;
};
Q_DECLARE_METATYPE ( CamelMessageInfoVariant )

 // Marshall the CamelMessageInfoVariant data into a D-BUS argument
 QDBusArgument &operator<<(QDBusArgument &argument, const CamelMessageInfoVariant &mystruct);
 // Retrieve the CamelMessageInfoVariant data from the D-BUS argument
 const QDBusArgument &operator>>(const QDBusArgument &argument, CamelMessageInfoVariant &mystruct);

 /* CamelFolderInfo struct content 
      <arg name="infos" type="a(sssuii)" direction="out"/>
		   string:uri,
		   string:folder_name,
		   string:full_name, (ex: parent/child_name)
		   uint32:flags,
		   int:unread_count,
		   int:total_mail_count

 */

struct CamelFolderInfoVariant
{
    QString uri;
    QString folder_name;
    QString full_name;
    unsigned int flags;
    int unread_count;
    int total_mail_count;
};
Q_DECLARE_METATYPE ( CamelFolderInfoVariant )

 // Marshall the CamelFolderInfoVariant data into a D-BUS argument
 QDBusArgument &operator<<(QDBusArgument &argument, const CamelFolderInfoVariant &mystruct);
 // Retrieve the CamelFolderInfoVariant  data from the D-BUS argument
 const QDBusArgument &operator>>(const QDBusArgument &argument, CamelFolderInfoVariant &mystruct);

typedef QList < CamelFolderInfoVariant > CamelFolderInfoArrayVariant;
Q_DECLARE_METATYPE ( CamelFolderInfoArrayVariant )
	
inline void registerMyDataTypes() {
    qDBusRegisterMetaType< CamelUserTagVariant >();
    qDBusRegisterMetaType< CamelMessageInfoVariant >();
    qDBusRegisterMetaType< CamelFolderInfoVariant >();
    qDBusRegisterMetaType< CamelFolderInfoArrayVariant >();
}

#endif   //MYTYPES_H
