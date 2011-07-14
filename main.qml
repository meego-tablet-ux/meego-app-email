/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 	
 * http://www.apache.org/licenses/LICENSE-2.0
 */

import QtQuick 1.0
import MeeGo.Components 0.1
import MeeGo.App.Email 0.1

Window {
    id: window
    property string topicSender: qsTr("Sender")
    property string topicSubject: qsTr("Subject")
    property string topicDate: qsTr("Date Sent")
    property int senderSortKey: 0
    property int subjectSortKey: 0
    property int dateSortKey: 0

    property string folderListViewTitle: ""
    property int animationDuration: 250

    property variant currentMailAccountId: 0;   // holds the actual QMailAccountId object
    property int currentMailAccountIndex: -1;    // holds the current account index to account list model
    property variant currentFolderId: 0;
    property string currentFolderName
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
    property bool editableDraft:false;
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
    property bool composeInTextMode: true
    property string errMsg: "";
    property variant argv: [] 
    property variant accountFilterModel: []

    Theme {
        id: theme
    }

    toolBarTitle: qsTr("Email")
    fullScreen: true
    automaticBookSwitching: false
    anchors.fill: parent

    showToolBarSearch: false

    bookMenuModel: accountFilterModel
   
    overlayItem:  Item {
        id: globalSpaceItems
        anchors.fill: parent

        ModalDialog {
            id:confirmDialog
            showCancelButton: false
            showAcceptButton: true
            acceptButtonText: qsTr("OK")
            title: qsTr("Error")

            content: Item {
                id:confirmMsg
                anchors.fill: parent
                anchors.margins: 10
                Text {
                    text: window.errMsg;
                    anchors.fill: parent
                    color:theme.fontColorNormal
                    font.pixelSize: theme.fontPixelSizeLarge
                    wrapMode: Text.Wrap
                }
            }
            onAccepted: {}
        }
       ModalDialog {
            id:passwordDialog
            property alias dlgText: prompt.text
            property alias dlgKey: key.text
            showCancelButton: true
            showAcceptButton: true
            cancelButtonText: qsTr("Cancel")
            acceptButtonText: qsTr("OK")
            title: qsTr("Error")
            height: 400

            content: Item {
                id:passwordContent
                anchors.fill: parent
                anchors.margins: 10
                Column {
                    anchors.fill: parent
                    spacing: 3
                    Text {
                        id:prompt
                        anchors.topMargin: 10
                        width: parent.width
                        text: window.errMsg;
                        clip: false
                        color:theme_fontColorNormal
                        font.pixelSize: theme_fontPixelSizeLarge
                        wrapMode: Text.Wrap
                    }

                    TextEntry {
                        id: entry
                        width: parent.width
                        anchors.topMargin: 10
                        anchors.bottomMargin: 5
                        defaultText: qsTr("password")
                        textInput.echoMode: TextInput.Password
                    }
                    Text {
                        id:key
                        text: qsTr("key")
                        visible: false
                    }
                }
            }

            onAccepted: {
                mailAccountListModel.addPassword (dlgKey, entry.text);
            }
        }
    }


    EmailAgent {
        id: emailAgent;
    }

    SaveRestoreState {
        id: saveRestore

        onSaveRequired: internal.save()

        Component.onCompleted: {
            if (!restoreRequired)
                return;
            internal.restore();
        }
    }

    QtObject {
        id: internal

        property string currentPage: "currentPage"

        function save()
        {
            saveRestore.setValue(currentPage, window.pageStack.currentPage.objectName);

            saveRestore.setValue("window.currentMailAccountId", window.currentMailAccountId);
            saveRestore.setValue("window.currentMailAccountIndex", window.currentMailAccountIndex);
            saveRestore.setValue("window.currentFolderId", window.currentFolderId);
            saveRestore.setValue("window.currentFolderName", window.currentFolderName);
            saveRestore.setValue("window.currentAccountDisplayName", window.currentAccountDisplayName);
            saveRestore.setValue("window.currentMessageIndex", window.currentMessageIndex);

            window.pageStack.currentPage.save(saveRestore);
            saveRestore.sync();
        }

        function restore()
        {
            window.currentMailAccountId = saveRestore.value("window.currentMailAccountId");
            window.currentMailAccountIndex = saveRestore.value("window.currentMailAccountIndex");
            window.currentFolderId = saveRestore.value("window.currentFolderId");
            window.currentFolderName = saveRestore.value("window.currentFolderName");
            window.currentAccountDisplayName = saveRestore.value("window.currentAccountDisplayName");
            window.currentMessageIndex = saveRestore.value("window.currentMessageIndex");

            messageListModel.setAccountKey (window.currentMailAccountId);
            mailFolderListModel.setAccountKey (window.currentMailAccountId);
            window.folderListViewTitle = window.currentAccountDisplayName + " " + mailFolderListModel.inboxFolderName();
            window.folderListViewClickCount = 0;

//            NOTE TO KIRILL: IT CRASHES HERE
//            var msgid = window.currentMessageIndex;
//            window.mailId = messageListModel.messageId(msgid);
//            window.mailSubject = messageListModel.subject(msgid);
//            window.mailSender = messageListModel.mailSender(msgid);
//            window.mailTimeStamp = messageListModel.timeStamp(msgid);
//            window.mailBody = messageListModel.body(msgid);
//            window.mailHtmlBody = messageListModel.htmlBody(msgid);
//            window.mailQuotedBody = messageListModel.quotedBody(msgid);
//            window.mailAttachments = messageListModel.attachments(msgid);
//            window.numberOfMailAttachments = messageListModel.numberOfAttachments(msgid);
//            window.mailRecipients = messageListModel.toList(msgid);
//            toListModel.init();
//            window.mailCc = messageListModel.ccList(msgid);
//            ccListModel.init();
//            window.mailBcc = messageListModel.ccList(msgid);
//            bccListModel.init();
//            mailAttachmentModel.init();
//            window.currentMessageIndex = msgid;
//            messageListModel.markMessageAsRead (window.mailId);

            restoreCurrentPage();
        }

        function restoreCurrentPage()
        {
            var cp = saveRestore.value(internal.currentPage);
            if (cp == "folderListView") {             
                window.addPage(folderList);
            } else if (cp == "accountListView") {
                window.addPage(mailAccount);
            } else if (cp == "composerPage") {              
                window.addPage(composer);
            } else if (cp == "readingView") {
                window.addPage(reader);
            }
            window.pageStack.currentPage.restore(saveRestore);
        }
    }


    EmailMessageListModel {
        id: messageListModel
        onFolderChanged: {
            var count;
            count = mailFolderListModel.getFolderMailCount();
            mailAccountListModel.setUnreadCount (window.currentMailAccountId, count);
        }
    }

    FolderListModel {
        id: mailFolderListModel
    }

    EmailAccountListModel {
        id: mailAccountListModel

        function updateAccountList() {
            var accountList = new Array();
            accountList = mailAccountListModel.getAllDisplayNames();
            accountList.push(qsTr("Account switcher"));
            window.accountFilterModel = accountList;
        }

        onAccountAdded: { updateAccountList(); }

        onAccountRemoved: { updateAccountList(); }

        onDataChanged: { updateAccountList(); }
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

    Component.onCompleted: {
        switchBook (mailAccount);
        window.fastPageSwitch = true;
    }

    function setMessageDetails (composer, messageID, replyToAll) {
        var dateline = qsTr ("On %1 %2 wrote:").arg(messageListModel.timeStamp (messageID)).arg(messageListModel.mailSender (messageID));

        composer.quotedBody = "\n" + dateline + "\n" + messageListModel.quotedBody (messageID); //i18n ok
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
	composer.messageMimeID = messageListModel.getMessageMimeId (messageID);
	composer.messageReferences = messageListModel.getReferences (messageID);
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
             for (idx = 0; idx < window.mailAttachments.length; idx ++)
             {
                 append({"uri": window.mailAttachments[idx]});
             }
        }
    }


    ListModel {
        id: toListModel
        property int idx: 0
        function init() {
            clear();
            for (idx = 0; idx < window.mailRecipients.length; idx ++)
            {
                append({"email": window.mailRecipients[idx]});
            }
        }
    }

    ListModel {
        id: ccListModel
        property int idx: 0
        function init() {
            clear();
            for (idx = 0; idx < window.mailCc.length; idx ++)
            {
                append({"email": window.mailCc[idx]});
            }
        }
    }

    ListModel {
        id: bccListModel
        property int idx: 0
        function init() {
            clear();
            for (idx = 0; idx < window.mailBcc.length; idx ++)
            {
                append({"email": window.mailBcc[idx]});
            }
        }
    }

    Connections {
        target: mailAccountListModel
        onAskPassword: {
                console.log ("ASKING PASSWORD: " + title + " " + prompt + " " + key);
                passwordDialog.title = title;
                passwordDialog.dlgText = prompt;
                passwordDialog.dlgKey = key;
                passwordDialog.show ();
        }
        onSendReceiveBegin : {
            window.refreshInProgress = true;
        }

        onSendReceiveCompleted: {
            console.log ("Send receive completed");
            window.refreshInProgress = false;
        }


    }

    Connections {
        target: messageListModel

        onSendReceiveBegin : {
            window.refreshInProgress = true;
        }

        onSendReceiveCompleted: {
            console.log ("Send receive completed");
            window.refreshInProgress = false;
        }


    }

    Connections {
        target: mainWindow
        onCall: {
            var cmd = parameters[0];
            var cdata = parameters[1];

            callFromRemote = true;
            if (cmd == "openComposer") {
                // This is the command for opening up the composer window with attachments.
                // cdata only contains a list of attachment files names 
                var datalist = cdata.split(',');
                window.mailAttachments = datalist;
                mailAttachmentModel.init();
                if (window.composerIsCurrentPage)
                    window.popPage();
                window.addPage(composer);
            }
            else if (cmd == "compose")
            {
                // This is the new command to open the composer with to, subject and message body.
                // the cdata contains: recipients;;subject_text;;bodyFilePath
                argv = cdata.split(";;");
                var to = "";
                var subject = "";
                var bodyPath = "";
                
                if (argv.length > 0)
                    to = argv[0];

                if (argv.length > 1)
                    subject = argv[1];

                if (argv.length > 2)
                    bodyPath = argv[2];

                if (window.composerIsCurrentPage)
                    window.popPage();
                var newPage;
                window.addPage(composer);
                newPage = window.pageStack.currentPage;
                if (to != "")
                {
                    toModel.clear();
                    toModel.append({"name": "", "email": to});
                    newPage.composer.toModel = toModel;
                }

                if (subject != "")
                {
                    newPage.composer.subject = subject;
                }

                if (bodyPath != "")
                {
                    newPage.composer.quotedBody = emailAgent.getMessageBodyFromFile(bodyPath);
                }
            }
            else
            {
                if (cmd == "reply" || cmd == "replyAll" || cmd == "forward" || cmd == "openReader")
                {
                    var msgUuid = parameters[1];
                    var msgIdx;
                    var currentAccount = mailAccountListModel.getAccountByUuid (msgUuid);
                    var currentFolder = mailAccountListModel.getFolderByUuid (msgUuid);

                    mailFolderListModel.setAccountKey (currentAccount);
                    messageListModel.setAccountKey (currentAccount);
                    messageListModel.setFolderKey (currentFolder);

                    msgIdx = messageListModel.indexFromMessageId(msgUuid);
                    window.currentMessageIndex = msgIdx;
                }

                if (cmd == "reply")
                {   
                    if (window.composerIsCurrentPage)
                        window.popPage();
                    var newPage;
                    window.addPage(composer);
                    newPage = window.pageStack.currentPage;
                    setMessageDetails (newPage.composer, window.currentMessageIndex, false);
                }
                else if (cmd == "replyAll")
                {
                    if (window.composerIsCurrentPage)
                        window.popPage();
                    var newPage;
                    window.addPage(composer);
                    newPage = window.pageStack.currentPage;
                    setMessageDetails (newPage.composer, window.currentMessageIndex, 2);
                }
                else if (cmd == "forward")
                {
                    if (window.composerIsCurrentPage)
                        window.popPage();
                    var newPage;
                    window.addPage(composer);
                    newPage = window.pageStack.currentPage;

                    newPage.composer.quotedBody = "\n" + qsTr("-------- Forwarded Message --------") + messageListModel.quotedBody (window.currentMessageIndex);
                    newPage.composer.subject = qsTr("[Fwd: %1]").arg(messageListModel.subject (window.currentMessageIndex));
                    window.mailAttachments = messageListModel.attachments(window.currentMessageIndex);
                    messageListModel.saveAttachmentsInTemp (window.currentMessageIndex);
                    mailAttachmentModel.init();
                    newPage.composer.attachmentsModel = mailAttachmentModel;

                }
                else if (cmd == "openReader") {
                    updateReadingView(msgIdx);
                }
            }
        }
    }

    ///When a selection is made in the account filter menu, you will get a signal here:
    onBookMenuTriggered: {
        if (index == (window.accountFilterModel.length - 1)) {
            window.switchBook(mailAccount);
        }
        else
        {
            window.currentMailAccountId = mailAccountListModel.getAccountIdByIndex(index);
            window.currentMailAccountIndex = index;
            window.currentAccountDisplayName = mailAccountListModel.getDisplayNameByIndex(index);
            messageListModel.setAccountKey (window.currentMailAccountId);
            mailFolderListModel.setAccountKey (window.currentMailAccountId);
            window.folderListViewTitle = currentAccountDisplayName + " " + mailFolderListModel.inboxFolderName();
            window.folderListViewClickCount = 0;
            window.currentFolderId = mailFolderListModel.inboxFolderId();
            window.currentFolderName = mailFolderListModel.inboxFolderName();
            window.switchBook(folderList);
        }
    }

    ///Subscribe to window search events:
    onSearch: {
        console.log("search query: " + needle)
    }

    Component {
        id: folderList
        AppPage {
            id: folderListView
            anchors.fill: parent
            pageTitle: window.folderListViewTitle
            enableCustomActionMenu: true
            objectName: "folderListView"

            property bool folderListPageHasFocus: true

            TopItem { id: folderListTopItem }

            function closeMenu()
            {
                contextActionMenu.hide();
            }

            function save(saveRestore)
            {
                //TODO: implement me
                folderListContainer.save(saveRestore);
            }

            function restore(saveRestore)
            {
                //TODO: implement me
                window.folderListViewClickCount = 0;
                folderListContainer.restore(saveRestore);
            }

            Component.onCompleted: {
                mailFolderListModel.setAccountKey (currentMailAccountId);
                window.currentFolderId = mailFolderListModel.inboxFolderId();
                //window.folderListViewTitle = currentAccountDisplayName + " " + mailFolderListModel.inboxFolderName();
                window.folderListViewClickCount = 0;
            }

            Component.onDestruction: {
                window.folderListViewClickCount = 0;
                window.accountPageClickCount= 0;
            }

            onSearch: {
                console.log("Application search query" + needle);
                messageListModel.setSearch (needle);
            }

            onActivated: { folderListPageHasFocus = true }

            onDeactivated: { folderListPageHasFocus = false }

            onActionMenuIconClicked: {
                if (folderListPageHasFocus) {
                    contextActionMenu.setPosition(mouseX, mouseY)
                    contextActionMenu.show()
                }
            }

            property int dateSortKey: 1
            property int senderSortKey: 1
            property int subjectSortKey: 1

            ContextMenu {
                id: contextActionMenu

                content: Item {
                    height: folderListMenu.height
                    width: folderListMenu.width
                    FolderListMenu {
                        id: folderListMenu
                    }
                }
            }
            PageBackground {
                contents: FolderListView {
                    id: folderListContainer
                }
                toolbar: FolderListViewToolbar {
                    id: folderListViewToolbar
                    folderListContainer: folderListContainer

                    onEditModeBegin: {
                        messageListModel.deSelectAllMessages();
                        folderListContainer.inSelectMode = true;
                        folderListContainer.numOfSelectedMessages = 0;
                    }

                    onEditModeEnd: {
                        messageListModel.deSelectAllMessages();
                        folderListContainer.inSelectMode = false;
                    }
                }
            }
        }
    }

    Component {
        id: mailAccount
        AppPage {
            id: accountListView
            anchors.fill: parent
            pageTitle: qsTr("Account list")
            objectName: "accountListView"

            property int idx: 0
            Component.onCompleted: {
                var accountList = new Array();
                accountList = mailAccountListModel.getAllDisplayNames();
                accountList.push(qsTr("Account switcher"));
                window.accountFilterModel = accountList;
            }
            PageBackground {
                contents: AccountPage {}
                toolbar: AccountViewToolbar {}
            }

            function save(saveRestore)
            {
                //TODO: implement me
            }

            function restore(saveRestore)
            {
                //TODO: implement me
            }
        }
    }

    Component {
        id: composer
        AppPage {
            id: composerPage
            objectName: "composerPage"

            TopItem { id: composerTopItem }

            function save(saveRestore)
            {
                //TODO: implement me
                composerView.save(saveRestore);
            }

            function restore(saveRestore)
            {
                //TODO: implement me
                composerView.restore(saveRestore);
            }

            Component.onCompleted: {

                if (window.editableDraft)
                {                    
                    composerView.composer.textBody= window.mailBody
                    composerView.composer.subject= window.mailSubject

                    var idx;
                    composerView.composer.toModel.clear();
                    for (idx = 0; idx < window.mailRecipients.length; idx++)
                        composerView.composer.toModel.append({"name": "", "email": window.mailRecipients[idx]});

                    composerView.composer.bccModel.clear();
                    for (idx = 0; idx < window.mailCc.length; idx ++)
                        composerView.composer.bccModel.append({"email": window.mailCc[idx]});

                    composerView.composer.bccModel.clear();
                    for (idx = 0; idx < window.mailBcc.length; idx ++)
                        composerView.composer.bccModel.append({"email": window.mailBcc[idx]});

                    composerView.composer.attachmentsModel.clear();
                    for (idx = 0; idx < window.mailAttachments.length; idx ++)
                        composerView.composer.attachmentsModel.append({"uri": window.mailAttachments[idx]});
                }

                window.editableDraft= false
                window.composerIsCurrentPage = true;
            }

            Component.onDestruction: {
                window.composerIsCurrentPage = false;
            }

            property alias composer: composerView.composer
            anchors.fill: parent
            pageTitle: qsTr("Composer")
            PageBackground {
                contents: ComposerView {
                    id: composerView
                }
                toolbar: ComposerViewToolbar {
                }
            }
        }
    }

    function updateReadingView (msgid)
    {
        // This function will be used to update the reading view wit speicfied msgid.
        window.popPage();
        window.mailId = messageListModel.messageId(msgid);
        window.mailSubject = messageListModel.subject(msgid);
        window.mailSender = messageListModel.mailSender(msgid);
        window.mailTimeStamp = messageListModel.timeStamp(msgid);
        window.mailBody = messageListModel.body(msgid);
        window.mailHtmlBody = messageListModel.htmlBody(msgid);
        window.mailQuotedBody = messageListModel.quotedBody(msgid);
        window.mailAttachments = messageListModel.attachments(msgid);
        window.numberOfMailAttachments = messageListModel.numberOfAttachments(msgid);
        window.mailRecipients = messageListModel.toList(msgid);
        toListModel.init();
        window.mailCc = messageListModel.ccList(msgid);
        ccListModel.init();
        window.mailBcc = messageListModel.bccList(msgid);
        bccListModel.init();
        mailAttachmentModel.init();
        window.currentMessageIndex = msgid;
        messageListModel.markMessageAsRead (window.mailId);
        window.addPage(reader);
    }

    Component {
        id: reader
        AppPage {
            id: readingView
            anchors.fill: parent
            pageTitle: window.mailSubject
            objectName: "readingView"

            TopItem { id: readingViewTopItem }

            Component.onDestruction: {
                window.accountPageClickCount = 0;
                window.folderListViewClickCount = 0;
            }

            function save(saveRestore)
            {
                //TODO: implement me
                reading.save(saveRestore)
            }

            function restore(saveRestore)
            {
                //TODO: implement me
                reading.restore(saveRestore)
            }

            actionMenuModel : window.mailReadFlag ? [qsTr("Mark as unread")] : [qsTr("Mark as read")]
            actionMenuPayload: [0]

            onActionMenuTriggered: {
                if (selectedItem == 0) {
                    if (window.mailReadFlag)
                    {
                        messageListModel.markMessageAsUnread (window.mailId);
                        window.mailReadFlag = 0;
                    }
                    else
                    {
                        messageListModel.markMessageAsRead (window.mailId);
                        window.mailReadFlag = 1;
                    }
                }
            }
            PageBackground {
                contents: ReadingView {
                    id: reading
                }
                toolbar: ReadingViewBottomBar {
                    progressBarText: reading.progressBarText
                    progressBarVisible: reading.progressBarVisible
                    progressBarPercentage: reading.progressBarPercentage
                }
            }
        }
    }
}
