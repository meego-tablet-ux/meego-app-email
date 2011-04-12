/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import Qt 4.7
import MeeGo.Labs.Components 0.1
import MeeGo.App.Email 0.1

Window {
    id: scene
    property string topicSender: qsTr("Sender")
    property string topicSubject: qsTr("Subject")
    property string topicDate: qsTr("Date Sent")
    property int senderSortKey: 0
    property int subjectSortKey: 0
    property int dateSortKey: 0

    property string folderListViewTitle: ""
    property int animationDuration: 250

    property variant currentMailAccountId: 0;   // holds the actual QMailAccountId object
    property int currentMailAccountIndex: 0;    // holds the current account index to account list model
    property variant currentFolderId: 0;
    property string currentAccountDisplayName;

    property string subjectLabel: qsTr("Subject:")
    property string sortLabel: qsTr("Sort messages by:")
    property string goToFolderLabel: qsTr("Go to folder:")

    property int currentMessageIndex;
    property string mailSender: "";
    property string mailSubject: "";
    property variant mailRecipients: []
    property variant mailCc: []
    property variant mailBcc: []
    property variant mailId;
    property bool mailReadFlag;
    property string mailTimeStamp: "";
    property string mailBody: "";
    property string mailHtmlBody: "";
    property string mailQuotedBody: "";
    property variant mailAttachments: []
    property int numberOfMailAttachments: 0
    property int accountPageClickCount: 0
    property int folderListViewClickCount: 0
    property bool refreshInProgress: false
    property bool callFromRemote: false
    property bool composerIsCurrentPage: false
    property string errMsg: "";

    title: qsTr("Email")
    showsearch: true
    filterModel: []

    EmailAgent {
        id: emailAgent;

        onSyncBegin: {
            scene.refreshInProgress = true;
        }

        onSyncCompleted: {
            scene.refreshInProgress = false;
        }

        onError: {
            scene.refreshInProgress = false;
            if (code != 1040)
            {
                errMsg = msg;
                showErrorMessage();
            }
        }
    }

    EmailMessageListModel {
        id: messageListModel
    }

    FolderListModel {
        id: mailFolderListModel
    }

    EmailAccountListModel {
        id: mailAccountListModel

        onAccountAdded: {
            var accountList = new Array();
            accountList = mailAccountListModel.getAccountList();
            accountList.push(qsTr("Account switcher"));
            scene.filterModel = accountList;
        }

        onAccountRemoved: {
            var accountList = new Array();
            accountList = mailAccountListModel.getAccountList();
            accountList.push(qsTr("Account switcher"));
            scene.filterModel = accountList;
        }
    }

    function showErrorMessage()
    {
        messageBoxLoader.sourceComponent = messageBoxcomponent;
        messageBoxLoader.item.parent = scene.container;
    }

    ListModel {
        id: toModel
    }

    ListModel {
        id: ccModel
    }

    ListModel {
        id: attachmentsModel
    }

    function setMessageDetails (composer, messageID, replyToAll) {
        var dateline = qsTr ("On %1 %2 wrote:\n").arg(messageListModel.timeStamp (messageID)).arg(messageListModel.mailSender (messageID));

        composer.quotedBody = dateline + messageListModel.quotedBody (messageID);
        attachmentsModel.clear();
        composer.attachmentsModel = attachmentsModel;
        toModel.clear();
        toModel.append({"name": "", "email": messageListModel.mailSender(messageID)});
        composer.toModel = toModel;


        if (replyToAll == true)
        {
            ccModel.clear();
            var recipients = new Array();
            recipients = messageListModel.recipients(messageID);
            var idx;
            for (idx = 0; idx < recipients.length; idx++)
                ccModel.append({"name": "", "email": recipients[idx]});
            composer.ccModel = ccModel;
        }
        // "Re:" is not supposed to be translated as per RFC 2822 section 3.6.5
        // Internet Message Format - http://www.faqs.org/rfcs/rfc2822.html
        //
        // "If this is done, only one instance of the literal string
        // "Re: " ought to be used since use of other strings or more
        // than one instance can lead to undesirable consequences."
        // Also see: http://www.chemie.fu-berlin.de/outerspace/netnews/son-of-1036.html#5.4
        // FIXME: Also need to only add Re: if it isn't already in the subject
        // to prevent "Re: Re: Re: Re: " subjects.
        composer.subject = "Re: " + messageListModel.subject (messageID);  //i18n ok
    }

    Loader {
        id:messageBoxLoader
    }

   Component {
        id:messageBoxcomponent
        ModalDialog {
            id:confirmDialog
            z:500
            rightButtonText: qsTr("OK")
            dialogTitle: qsTr("Error")
            Text {
                id:confirmMsg
                text: scene.errMsg;
                anchors.centerIn:parent
                color:theme_fontColorNormal
                font.pixelSize: theme_fontPixelSizeLarge
                elide: Text.ElideRight
            }
            onDialogClicked: {
                messageBoxLoader.sourceComponent = undefined
            }

        }
    }

    FuzzyDateTime {
        id: fuzzy
    }

    Loader {
        anchors.fill: parent
        id: dialogLoader
    }

    ListModel {
        id: mailAttachmentModel
        property int idx: 0
        function init() {
            clear();
             for (idx = 0; idx < scene.mailAttachments.length; idx ++)
             {
                 append({"uri": scene.mailAttachments[idx]});
             }
        }
    }


    ListModel {
        id: toListModel
        property int idx: 0
        function init() {
            clear();
            for (idx = 0; idx < scene.mailRecipients.length; idx ++)
            {
                append({"email": scene.mailRecipients[idx]});
            }
        }
    }

    ListModel {
        id: ccListModel
        property int idx: 0
        function init() {
            clear();
            for (idx = 0; idx < scene.mailCc.length; idx ++)
            {
                append({"email": scene.mailCc[idx]});
            }
        }
    }

    ListModel {
        id: bccListModel
        property int idx: 0
        function init() {
            clear();
            for (idx = 0; idx < scene.mailBcc.length; idx ++)
            {
                append({"email": scene.mailBcc[idx]});
            }
        }
    }

    applicationPage: mailAccount

    Connections {
        target: mainWindow
        onCall: {
            var cmd = parameters[0];
            var cdata = parameters[1]; // cdata contains "attachmentt1,attachment2,..."

            callFromRemote = true;
            if (cmd == "openComposer") {
                var datalist = cdata.split(',');
                scene.mailAttachments = datalist;
                mailAttachmentModel.init();
                if (scene.composerIsCurrentPage)
                    scene.previousApplicationPage();
                scene.addApplicationPage(composer);
            }
            else
            {
                var msgUuid = parameters[1];
                var msgIdx = messageListModel.indexFromMessageId(msgUuid);
                scene.currentMessageIndex = msgIdx;
                if (cmd == "reply")
                {   
                    if (scene.composerIsCurrentPage)
                        scene.previousApplicationPage();
                    var newPage;
                    scene.addApplicationPage(composer);
                    newPage = scene.currentApplication;
                    setMessageDetails (newPage.composer, scene.currentMessageIndex, false);
                }
                else if (cmd == "replyAll")
                {
                    if (scene.composerIsCurrentPage)
                        scene.previousApplicationPage();
                    var newPage;
                    scene.addApplicationPage(composer);
                    newPage = scene.currentApplication;
                    setMessageDetails (newPage.composer, scene.currentMessageIndex, 2);
                }
                else if (cmd == "forward")
                {
                    if (scene.composerIsCurrentPage)
                        scene.previousApplicationPage();
                    var newPage;
                    scene.addApplicationPage(composer);
                    newPage = scene.currentApplication;

                    newPage.composer.quotedBody = qsTr("-------- Forwarded Message --------") + messageListModel.quotedBody (scene.currentMessageIndex);
                    newPage.composer.subject = qsTr("[Fwd: %1]").arg(messageListModel.subject (scene.currentMessageIndex));
                    scene.mailAttachments = messageListModel.attachments(scene.currentMessageIndex);
                    mailAttachmentModel.init();
                    newPage.composer.attachmentsModel = mailAttachmentModel;

                }
                else if (cmd == "openReader") {
                    updateReadingView(msgIdx);
                }
            }
        }
    }

    ///When a selection is made in the filter menu, you will get a signal here:
    onFilterTriggered: {
        if (index == (scene.filterModel.length - 1)) {
            scene.applicationPage = mailAccount;
        }
        else
        {
            scene.currentMailAccountId = mailAccountListModel.getAccountIdByIndex(index);
            scene.currentMailAccountIndex = index;
            scene.currentAccountDisplayName = mailAccountListModel.getDisplayNameByIndex(index);
            scene.folderListViewTitle = qsTr("%1 %2").arg(currentAccountDisplayName).arg(mailFolderListModel.inboxFolderName());
            scene.applicationPage = null;
            scene.applicationPage = folderList;
            messageListModel.setAccountKey (scene.currentMailAccountId);
            mailFolderListModel.setAccountKey (scene.currentMailAccountId);
            scene.folderListViewClickCount = 0;
            scene.currentFolderId = mailFolderListModel.inboxFolderId();
        }
    }

    ///Subscribe to window search events:
    onSearch: {
        console.log("search query: " + needle)
    }

    Component {
        id: folderList
        ApplicationPage {
            id: folderListView
            anchors.fill: parent
            title: scene.folderListViewTitle

            Component.onCompleted: {
                mailFolderListModel.setAccountKey (currentMailAccountId);
                var idx = mailFolderListModel.indexFromFolderId(0);
		//mailFolderListModel.setFolderKey (mailFolderListModel.inboxFolderId());
		scene.currentFolderId = mailFolderListModel.inboxFolderId();
                scene.folderListViewTitle = qsTr("%1 %2").arg(currentAccountDisplayName).arg(inboxLabel);
                scene.folderListViewClickCount = 0;
            }

            Component.onDestruction: {
                scene.folderListViewClickCount = 0;
                scene.accountPageClickCount= 0;
            }

            onSearch: {
                console.log("Application search query" + needle);
		messageListModel.setSearch (needle);
             }

             property int dateSortKey: 1
             property int senderSortKey: 1
             property int subjectSortKey: 1

             menuContent: Item {
                 height: folderListMenu.height
                 width: folderListMenu.width
                 FolderListMenu {
                     id: folderListMenu
                 }
             }

             FolderListView {}
        }
    }

    Component {
        id: mailAccount
        ApplicationPage {
            id: accountListView
            anchors.fill: parent
            title: qsTr("Account list")
            property int idx: 0
            Component.onCompleted: {
                var accountList = new Array();
                accountList = mailAccountListModel.getAccountList();
                accountList.push(qsTr("Account switcher"));
                scene.filterModel = accountList;
            }

            AccountPage {}
        }
    }

    Component {
        id: composer
        ApplicationPage {
            id: composerPage

            Component.onCompleted: {
                scene.composerIsCurrentPage = true;
            }

            Component.onDestruction: {
                scene.composerIsCurrentPage = false;
            }

            property alias composer: composerView.composer
            anchors.fill: parent
            title: qsTr("Composer")
            ComposerView {
                id: composerView
            }
        }
    }

    function updateReadingView (msgid)
    {
        // This function will be used to update the reading view wit speicfied msgid.
        scene.previousApplicationPage();
        scene.mailId = messageListModel.messageId(msgid);
        scene.mailSubject = messageListModel.subject(msgid);
        scene.mailSender = messageListModel.mailSender(msgid);
        scene.mailTimeStamp = messageListModel.timeStamp(msgid);
        scene.mailBody = messageListModel.body(msgid);
        scene.mailHtmlBody = messageListModel.htmlBody(msgid);
        scene.mailQuotedBody = messageListModel.quotedBody(msgid);
        scene.mailAttachments = messageListModel.attachments(msgid);
        scene.numberOfMailAttachments = messageListModel.numberOfAttachments(msgid);
        scene.mailRecipients = messageListModel.toList(msgid);
        toListModel.init();
        scene.mailCc = messageListModel.ccList(msgid);
        ccListModel.init();
        scene.mailBcc = messageListModel.ccList(msgid);
        bccListModel.init();
        mailAttachmentModel.init();
        scene.currentMessageIndex = msgid;
        emailAgent.markMessageAsRead (scene.mailId);
        scene.addApplicationPage(reader);
    }

    Component {
        id: reader
        ApplicationPage {
            id: readingView
            anchors.fill: parent
            title: scene.mailSubject

            Component.onDestruction: {
                scene.accountPageClickCount = 0;
                scene.folderListViewClickCount = 0;
            }

            menuContent: Item {
                anchors.fill:parent
                height: 50
                id: markAsReadUnread
                width: menuLabel.width + 20
                Text {
                    id: menuLabel
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 14
                    text: scene.mailReadFlag ? qsTr("Mark as unread") : qsTr("Mark as read") 
                    color:theme_fontColorNormal
                    font.pixelSize: theme_fontPixelSizeLarge
                }
                MouseArea{
                anchors.fill: parent
                onClicked: {
                    if (scene.mailReadFlag)
                    {
                        emailAgent.markMessageAsUnread (scene.mailId);
                        scene.mailReadFlag = 0;
                    }
                    else
                    {
                        emailAgent.markMessageAsRead (scene.mailId);
                        scene.mailReadFlag = 1;
                    }
                        readingView.closeMenu();
                    }
                }
            }

            ReadingView {
            }
        }
    }
}
