
#include "dbustypes.h"


  // Marshall the CamelUserTagVariant data into a D-BUS argument
 QDBusArgument &operator<<(QDBusArgument &argument, const CamelUserTagVariant &d)
 {
     argument.beginStructure();
     argument << d.tag_name << d.tag_value;
     argument.endStructure();

     return argument;
    	 
 }

 // Retrieve the CamelUserTagVariant data from the D-BUS argument
 const QDBusArgument &operator>>(const QDBusArgument &argument, CamelUserTagVariant &d)
 {
     argument.beginStructure();
     argument >> d.tag_name >> d.tag_value;
     argument.endStructure();
     return argument;
 }

 // Marshall the CamelMessageInfoVariant data into a D-BUS argument
 QDBusArgument &operator<<(QDBusArgument &argument, const CamelMessageInfoVariant &d)
 {
     argument.beginStructure();
     argument << d.uid << d.subject << d.from << d.to << d.cc << d.mlist << d.preview << d.flags << d.size << d.date_sent << d.date_received << d.message_id << d.references_size << d.references << d.user_flags  << d.user_tags;
     argument.endStructure();
     return argument;	 
 }

 // Retrieve the CamelMessageInfoVariant data from the D-BUS argument
 const QDBusArgument &operator>>(const QDBusArgument &argument, CamelMessageInfoVariant &d)
 {

     argument.beginStructure();
     argument >> d.uid >> d.subject >> d.from >> d.to >> d.cc >> d.mlist >> d.preview >> d.flags >> d.size >> d.date_sent >> d.date_received >> d.message_id >> d.references_size >> d.references >> d.user_flags  >> d.user_tags;
     argument.endStructure();

     return argument;	 
 }

 // Marshall the CamelFolderInfoVariant data into a D-BUS argument
 QDBusArgument &operator<<(QDBusArgument &argument, const CamelFolderInfoVariant &d)
 {
     argument.beginStructure();
     argument << d.uri << d.folder_name << d.full_name << d.flags << d.unread_count << d.total_mail_count;
     argument.endStructure();
     return argument;	 
 }

 // Retrieve the CamelFolderInfoVariant  data from the D-BUS argument
 const QDBusArgument &operator>>(const QDBusArgument &argument, CamelFolderInfoVariant &d)
 {
     argument.beginStructure();
     argument >> d.uri >> d.folder_name >> d.full_name >> d.flags >> d.unread_count >> d.total_mail_count;     
     argument.endStructure();
     return argument;
 }

