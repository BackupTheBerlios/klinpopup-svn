/**************************************************************************
*   Copyright (C) 2004, 2005 by Gerd Fleischer                            *
*   gerdfleischer@web.de                                                  *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.             *
***************************************************************************/

//#define MY_EXTRA_DEBUG

#include <kdebug.h>

#include <QFile>
#include <QLabel>
#include <QStringList>
#include <QRegExp>
#include <QGroupBox>
#include <QToolTip>

#include <QSplitter>
#include <QTreeWidget>
#include <QKeyEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QEvent>
#include <QCloseEvent>

#include <kaction.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kuser.h>
#include <kiconloader.h>
#include <k3process.h>

#include "makepopup.h"
#include "makepopup.moc"

void readGroupsThread::run()
{
	threadOwner->readGroupList();
}

void readHostsThread::run()
{
	threadOwner->readHostList();
}

makePopup::makePopup(QWidget *parent, const QString &paramSender,
					 const QString &paramSmbclient, int paramEncoding, int paramView)
	: QWidget(parent, Qt::Window),
	  smbclientBin(paramSmbclient), messageReceiver(paramSender),
	  newMsgEncoding(paramEncoding), viewMode(paramView),
	  sendRefCount(0), sendError(0), justSending(false),
	  readHosts(this), readGroups(this)
{
//	initSmbCtx();
	setupLayout();

	connect(buttonSend, SIGNAL(clicked()), this, SLOT(slotButtonSend()));
	connect(buttonCancel, SIGNAL(clicked()), this, SLOT(finished()));

	if (viewMode == CLASSIC_VIEW) {
		connect(groupBox, SIGNAL(activated(const QString &)), this, SLOT(slotGroupboxChanged(const QString &)));
	 } else {
		connect(groupTreeView, SIGNAL(itemSelectionChanged()), this, SLOT(slotTreeViewSelectionChanged()));
		connect(groupTreeView, SIGNAL(expanded(QTreeWidgetItem *)), this, SLOT(slotTreeViewItemExpanded(Q3ListViewItem *)));
	}

	//initialize senderBox, groupBox and receiverBox
//	QString tmpHostName = smbCtx->netbios_name;
//	if (!tmpHostName.isEmpty()) senderBox->addItem(QString("test"));

	QString tmpLoginName = KUser().loginName();
	if (!tmpLoginName.isEmpty()) senderBox->addItem(tmpLoginName);

	QString tmpFullName = KUser().fullName();
	if (!tmpFullName.isEmpty()) senderBox->addItem(tmpFullName);

	if (messageReceiver.isEmpty()) {
		if (viewMode == CLASSIC_VIEW) groupBox->addItem(i18n("NO GROUP"), -1);
		readGroups.start();
	} else {
		groupBox->setEnabled(false);
		classicReceiverBox->addItem(messageReceiver);
		classicReceiverBox->setEnabled(false);
	}
}

makePopup::~makePopup()
{
}

void makePopup::closeEvent(QCloseEvent *e)
{
	e->ignore();
	finished();
}

/**
 * filter keypresses and restrict message size to 1600 bytes
 */
bool makePopup::eventFilter(QObject *, QEvent *e)
{
/*	if (e->type() == QEvent::KeyPress) {
		QKeyEvent *k = (QKeyEvent *)e;
		// do funny things to figure out the bytes of the message and react accordingly
		if ((k->key() >= Qt::Key_Space && k->key() <= Qt::Key_ydiaeresis) || k->key() == Qt::Key_Tab) {
			if ((messageText->length() + messageText->lines()) > 1600) return true;
		} else if (k->key() == Qt::Key_Return) {
			if (((messageText->length() + 1) + messageText->lines()) > 1600) return true;
		}
	}

	return false;*/
}

void makePopup::setupLayout()
{
	setFocusPolicy(Qt::WheelFocus);

	QGroupBox *messageTextBox;

	buttonSend = new KPushButton(this);
//	buttonSend->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)1, (QSizePolicy::SizeType)0, 0, 0, false));

	buttonCancel = new KPushButton(this);
//	buttonCancel->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)1, (QSizePolicy::SizeType)0, 0, 0, false));


	if (viewMode == CLASSIC_VIEW) {
		makePopupLayout = new QGridLayout(this);

		QLabel *senderLabel = new QLabel(i18n("From:"), this);
		senderBox = new KComboBox(this);
//		senderBox->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)7, (QSizePolicy::SizeType)0, 0, 0, false));
		senderBox->setEditable(true);
		QHBoxLayout *senderLayout = new QHBoxLayout();
		senderLayout->addWidget(senderLabel);
		senderLayout->addWidget(senderBox);

		QLabel *groupLabel = new QLabel(i18n("Group:"), this);
		groupBox = new KComboBox(this);
//		groupBox->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)7, (QSizePolicy::SizeType)0, 0, 0, false));
		QHBoxLayout *groupLayout = new QHBoxLayout();
		groupLayout->addWidget(groupLabel);
		groupLayout->addWidget(groupBox);

		QLabel *receiverLabel = new QLabel(i18n("To:"), this);
		classicReceiverBox = new KComboBox(this);
//		classicReceiverBox->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)7, (QSizePolicy::SizeType)0, 0, 0, false));
		classicReceiverBox->setEditable(true);
		QHBoxLayout *receiverLayout = new QHBoxLayout();
		receiverLayout->addWidget(receiverLabel);
		receiverLayout->addWidget(classicReceiverBox);

		int senderLabelWidth = senderLabel->sizeHint().width();
		int groupLabelWidth = groupLabel->sizeHint().width();
		int receiverLabelWidth = receiverLabel->sizeHint().width();

		if (senderLabelWidth > groupLabelWidth && senderLabelWidth > receiverLabelWidth) {
			groupLabel->setFixedWidth(senderLabelWidth);
			receiverLabel->setFixedWidth(senderLabelWidth);
		} else if (groupLabelWidth > senderLabelWidth && groupLabelWidth > receiverLabelWidth) {
			senderLabel->setFixedWidth(groupLabelWidth);
			receiverLabel->setFixedWidth(groupLabelWidth);
		} else {
			senderLabel->setFixedWidth(receiverLabelWidth);
			groupLabel->setFixedWidth(receiverLabelWidth);
		}

		messageTextBox = new QGroupBox(i18n("Message text"), this);
//		messageTextBox->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)3, (QSizePolicy::SizeType)3, 0, 0, false));
/*		messageTextBox->setLineWidth(1);
		messageTextBox->setColumnLayout(0, Qt::Vertical);
		messageTextBox->setInsideMargin(4);*/
		QGridLayout *messageTextBoxLayout = new QGridLayout(messageTextBox);

		messageText = new KTextEdit(messageTextBox);
//		messageText->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)3, (QSizePolicy::SizeType)3, 0, 0, false));
		messageTextBoxLayout->addWidget(messageText, 0, 0);

		makePopupLayout->addLayout(senderLayout, 0, 0, 0, 2);
		makePopupLayout->addLayout(groupLayout, 1, 1, 0, 2);
		makePopupLayout->addLayout(receiverLayout, 2, 2, 0, 2);
		makePopupLayout->addWidget(messageTextBox, 3, 3, 0, 2);
		makePopupLayout->addWidget( buttonSend, 4, 1);
		makePopupLayout->addWidget( buttonCancel, 4, 2);
		resize(QSize(375, 250).expandedTo(minimumSizeHint()));
	} else {
		makePopupLayout = new QGridLayout(this);
		QSplitter *sp = new QSplitter(this);
		sp->setOpaqueResize(true);

		QWidget *rightSplitLayout = new QWidget(sp);
//		rightSplitLayout->setMargin(3);
		groupTreeView = new QTreeWidget(rightSplitLayout);
		groupTreeView->setColumnCount(2);
//		groupTreeView->addColumn(i18n("Groups/Hosts"));
//		groupTreeView->addColumn(i18n("Comment"));
//		groupTreeView->setEnabled(true);
//		groupTreeView->setRootIsDecorated(true);
//		groupTreeView->setFrameStyle(Q3GroupBox::Box|Q3GroupBox::Plain);
//		groupTreeView->setSelectionMode(Q3ListView::Extended);
//		groupTreeView->setLineWidth(1);
//		groupTreeView->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)3, (QSizePolicy::SizeType)3, 0, 0, false));
//		sp->setResizeMode(rightSplitLayout, QSplitter::FollowSizeHint);

		QWidget *leftSplitLayout = new QWidget(sp);
// 		leftSplitLayout->setSpacing(6);
// 		leftSplitLayout->setMargin(3);
		QHBoxLayout *senderHBox = new QHBoxLayout(leftSplitLayout);
		QLabel *senderLabel = new QLabel(i18n("From:"));
		senderHBox->addWidget(senderLabel);
		senderBox = new KComboBox();
		senderHBox->addWidget(senderBox);
//		senderBox->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)7, (QSizePolicy::SizeType)0, 0, 0, false));
		senderBox->setEditable(true);

		QHBoxLayout *receiverHBox = new QHBoxLayout(leftSplitLayout);
		QLabel *receiverLabel = new QLabel(i18n("To:"));
		receiverHBox->addWidget(receiverLabel);
		treeViewReceiverBox = new KLineEdit();
		receiverHBox->addWidget(treeViewReceiverBox);
//		treeViewReceiverBox->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)7, (QSizePolicy::SizeType)0, 0, 0, false));

		messageTextBox = new QGroupBox(i18n("Message text"), leftSplitLayout);
//		messageTextBox->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)3, (QSizePolicy::SizeType)3, 0, 0, false));
//		messageTextBox->setLineWidth(1);
//		messageTextBox->setColumnLayout(0, Qt::Vertical);
//		messageTextBox->setInsideMargin(4);
		QGridLayout *messageTextBoxLayout = new QGridLayout(messageTextBox);

		messageText = new KTextEdit(messageTextBox);
//		messageText->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)7, (QSizePolicy::SizeType)7, 0, 0, false));
		messageTextBoxLayout->addWidget(messageText, 0, 0);

		int senderLabelWidth = senderLabel->sizeHint().width();
		int receiverLabelWidth = receiverLabel->sizeHint().width();

		if (senderLabelWidth < receiverLabelWidth)
			senderLabel->setFixedWidth(receiverLabelWidth);
		else
			receiverLabel->setFixedWidth(senderLabelWidth);

		makePopupLayout->addWidget(sp, 0, 3, 0, 4);
		makePopupLayout->addWidget( buttonSend, 4, 3);
		makePopupLayout->addWidget( buttonCancel, 4, 4);
		resize(QSize(575, 250).expandedTo(minimumSizeHint()));
	}

//	messageTextBox->setFrameStyle(Q3GroupBox::Box|Q3GroupBox::Plain);
//	messageTextBox->setAlignment(int(Q3GroupBox::Qt::AlignCenter));

//	messageText->setFrameStyle(KTextEdit::LineEditPanel|KTextEdit::Sunken);
//	messageText->setResizePolicy(KTextEdit::Manual);
//	messageText->setTextFormat(KTextEdit::PlainText);
//	messageText->setAutoFormatting(int(KTextEdit::AutoAll));

	languageChange();

	messageText->installEventFilter(this);
	messageText->setFocus();
}

void makePopup::queryFinished()
{
	if (!allProcessesStarted || sendRefCount != 0) return;

	if (errorHosts.isEmpty()) {
		KMessageBox::information(this, i18n("Message sent!"), i18n("Success"), "ShowMessageSentSuccess");
	} else {
		QString errorHostsString;
		QMap<QString, QString>::ConstIterator end = errorHosts.end();
		for (QMap<QString, QString>::ConstIterator it = errorHosts.begin(); it != end; ++it) {
			errorHostsString += it.value().toUpper();
			errorHostsString += ", ";
		}
		errorHostsString.truncate(errorHostsString.length() - 2);
		int tmpYesNo = KMessageBox::warningYesNo(this, i18n("Message could not be sent to %1!\n"
															"Edit/Try again?").arg(errorHostsString));
		if (tmpYesNo == KMessageBox::Yes) {
			errorHosts.clear();
			return;
		}
	}

	justSending = false;
	finished();
}

/**
 * hide and destroy when message has been sent or canceled
 */
void makePopup::finished()
{
	if (justSending) return;

	hide();

	// This should be safe here
	if (readGroups.isRunning()) readGroups.terminate();
	if (readHosts.isRunning()) readHosts.terminate();

	readGroups.wait();
	readHosts.wait();

	deleteLater();
}

/**
 * called when sendButton emits clicked()
 * checks for empty hostbox and sends message
 */
void makePopup::slotButtonSend()
{
	QString tmpHostBox;
	if (viewMode == CLASSIC_VIEW)
		tmpHostBox = classicReceiverBox->currentText();
	else
		tmpHostBox = treeViewReceiverBox->text();

	if (tmpHostBox.isEmpty()) {
		KMessageBox::information(this, i18n("Please state an addressee!"));
		if (viewMode == CLASSIC_VIEW)
			classicReceiverBox->setFocus();
		else
			treeViewReceiverBox->setFocus();
	} else {
		sendPopup();
	}
}

/**
 * sends the messages with smbclient, opens a K3Process for each
 */
void makePopup::sendPopup()
{
	QString tmpReceiverString;
	if (viewMode == CLASSIC_VIEW)
		tmpReceiverString = classicReceiverBox->currentText();
	else
		tmpReceiverString = treeViewReceiverBox->text();

	QStringList tmpReceiverList;
	QRegExp hostComment("\\s\\(.*\\)"), whiteSpaces("\\s+");

	if (tmpReceiverString.indexOf(hostComment) > -1) {
		tmpReceiverList += tmpReceiverString.left(tmpReceiverString.indexOf(" ("));
	} else if (tmpReceiverString == i18n("Whole workgroup")) {
		tmpReceiverList = allGroupHosts;
	} else {
		tmpReceiverList = tmpReceiverString.split(whiteSpaces, QString::SkipEmptyParts);
	}

	allProcessesStarted = false;
	justSending = true;
	QStringList::ConstIterator end = tmpReceiverList.end();
	for (QStringList::ConstIterator it = tmpReceiverList.begin(); it != end; ++it) {
		sendRefCount++;

		K3Process *p = new K3Process(this);
		*p << smbclientBin << "-M" << *it;
		*p << "-N" << "-U" << senderBox->currentText() << "-";

		errorHosts.insert(QString::number((unsigned long)(p), 10), *it);

		connect(p, SIGNAL(processExited(K3Process *)), this, SLOT(slotSendCmdExit(K3Process *)));

		if (p->start(K3Process::NotifyOnExit, K3Process::Stdin)) {
			QString tmpText = messageText->document()->toPlainText();
			p->writeStdin(tmpText.toUtf8(), tmpText.toUtf8().length());
			if (!p->closeStdin()) {
///@TODO: does this work, how to test this?
				delete p;
			}
		} else {
			slotSendCmdExit(0);
		}
	}
	allProcessesStarted = true;
	queryFinished();
}

void makePopup::slotSendCmdExit(K3Process *_p)
{
	if (_p && _p->normalExit() && _p->exitStatus() == 0) {
		errorHosts.remove(QString::number((unsigned long)(_p), 10));
	}
	delete _p;
	sendRefCount--;
	queryFinished();
}

/**
 * init the libsmbclient context
 */
void makePopup::initSmbCtx()
{
/*	smbCtx = smbc_new_context();
	if (smbCtx) {
		smbCtx->callbacks.auth_fn = makePopup::auth_smbc_get_data;
		smbCtx->timeout = 2000; // any effect?
#ifdef HAVE_SMBCCTX_OPTIONS
		smbCtx->options.urlencode_readdir_entries = 1;
#endif
		smbCtx = smbc_init_context(smbCtx);
//		smbc_set_context(smbCtx);
	} else {
		kDebug() << "Error getting new smbCtx!" << endl;
	}*/
}

/**
 * read available groups with libsmbclient
 */
void makePopup::readGroupList()
{
/*	QString ownGroup = QString::null;

	if (smbCtx != 0) {
		// get own workgoup first
		ownGroup = QString::fromUtf8(smbCtx->workgroup, -1);

		kDebug() << "own group: " << ownGroup << endl;

		if (ownGroup == "WORKGROUP") {
			ownGroup = QString::null; // workaround for SAMBA 3.0.15pre2+
			kDebug() << "If this is WORKGROUP and you use Samba >= 3.0.20 we may have a problem." << endl;
			kDebug() << "Autodetection of own workgroup disabled." << endl;
		}

		SMBCFILE *dirfd;
		struct smbc_dirent *dirp = 0;

		// first level should be the workgroups
		dirfd = smbCtx->opendir(smbCtx, "smb://");

		if (dirfd) {
			do {
				dirp = smbCtx->readdir(smbCtx, dirfd);
				if (dirp == 0) break;
				QString tmpGroup = QString::fromUtf8( dirp->name );
				if (dirp->smbc_type == SMBC_WORKGROUP) {
					if (tmpGroup == ownGroup) {
						if (viewMode == CLASSIC_VIEW) {
							groupBox->insertItem(tmpGroup + " (" + i18n("own") + ")", -1);
						} else {
							Q3ListViewItem *tmpItem = new Q3ListViewItem(groupTreeView, tmpGroup, i18n("own"));
							tmpItem->setPixmap(0, SmallIcon("network_local"));
							tmpItem->setExpandable(true);
							tmpItem->setSelectable(false);
						}
					} else {
						if (viewMode == CLASSIC_VIEW) {
							groupBox->insertItem(tmpGroup, -1);
						} else {
							Q3ListViewItem *tmpItem = new Q3ListViewItem(groupTreeView, tmpGroup);
							tmpItem->setPixmap(0, SmallIcon("network_local"));
							tmpItem->setExpandable(true);
							tmpItem->setSelectable(false);
						}
					}
				}
			} while (dirp);
		}
		smbCtx->closedir(smbCtx, dirfd);
	}

	// initialize with own group if possible or if it's only one
	if (!ownGroup.isEmpty()) {
		if (viewMode == CLASSIC_VIEW) groupBox->setCurrentText(ownGroup + " (" + i18n("own") + ")");
		currentGroup = ownGroup;
		readHosts.start();
	} else if (viewMode == CLASSIC_VIEW && groupBox->count() == 2) {
		groupBox->setCurrentItem(1);
		currentGroup = groupBox->currentText();
		readHosts.start();
	} else if (viewMode == TREE_VIEW && groupTreeView->childCount() == 1) {
		currentGroup = groupTreeView->firstChild()->text(0);
		readHosts.start();
	}*/
}

/**
 * read available hosts from group with libsmbclient
 */
void makePopup::readHostList()
{
// 	if (smbCtx != 0) {
//
// 		SMBCFILE *dirfd;
// 		struct smbc_dirent *dirp = 0;
//
// 		// next level should be the hosts
// 		QString tmpGroup = "smb://";
// 		tmpGroup.append(currentGroup);
// //		kDebug() << tmpGroup << endl;
//
// 		dirfd = smbCtx->opendir(smbCtx, tmpGroup);
// //		kDebug() << dirfd << endl;
//
// 		if (dirfd) {
// 			do {
// 				dirp = smbCtx->readdir(smbCtx, dirfd);
// 				if (dirp == 0) break;
//
// 				QString tmpHost = QString::fromUtf8(dirp->name);
//
// 				if (dirp->smbc_type == SMBC_SERVER) {
// 					allGroupHosts += tmpHost;
// 					QString tmpComment = QString::fromUtf8(dirp->comment);
//
// 					if (viewMode == TREE_VIEW) {
// 						Q3ListViewItem *tmpGroupItem = groupTreeView->findItem(currentGroup, 0);
// 						if (tmpGroupItem != 0) {
// 							Q3ListViewItem *tmpHostItem = new Q3ListViewItem(tmpGroupItem, tmpHost, tmpComment);
// 							tmpHostItem->setPixmap(0, SmallIcon("server"));
// 							tmpHostItem->setExpandable(false);
// 						}
// 						tmpGroupItem->setOpen(true);
// 					} else {
// 						if (!tmpComment.isEmpty()) tmpHost.append(" (" + tmpComment + ")");
// 						classicReceiverBox->insertItem(tmpHost, -1);
// 					}
// 				}
// 			} while (dirp);
// 		}
// 		smbCtx->closedir(smbCtx, dirfd);
// 		if (viewMode == CLASSIC_VIEW && allGroupHosts.count() > 1) classicReceiverBox->insertItem(i18n("Whole workgroup"), -1);
// 	}
}

/**
 * will be called if groupbox is changed
 */
void makePopup::slotGroupboxChanged(const QString &)
{
	classicReceiverBox->clear();
	allGroupHosts.clear();
	currentGroup = groupBox->currentText();
	if (currentGroup == i18n("NO GROUP")) return;
	if (currentGroup.indexOf(" (") > 1) currentGroup = currentGroup.left(currentGroup.indexOf(" ("));

	if (readHosts.isRunning()) readHosts.exit();
	readHosts.start();
}

void makePopup::slotTreeViewItemExpanded(QTreeWidgetItem *clickedItem)
{
	if (clickedItem == 0 || clickedItem->childCount() != 0) return;

	currentGroup = clickedItem->text(0);

	if (readHosts.isRunning()) readHosts.exit();
	readHosts.start();
}

void makePopup::slotTreeViewSelectionChanged()
{
// 	QString selectedHosts;
// 	Q3ListViewItemIterator it(groupTreeView, Q3ListViewItemIterator::Selected|Q3ListViewItemIterator::NotExpandable);
// 	while (it.current()) {
// 		selectedHosts += it.current()->text(0);
// 		selectedHosts += " ";
// 		++it;
// 	}
// 	treeViewReceiverBox->setText(selectedHosts.trimmed());
}

void makePopup::languageChange()
{
	senderBox->setToolTip(i18n("Choose sender"));
	senderBox->setWhatsThis( i18n("This box lets you customize the sender value."));
	if (viewMode == CLASSIC_VIEW) {
		groupBox->setToolTip(i18n("Available workgroups"));
		groupBox->setWhatsThis( i18n("Which workgroups are available in the network."));
		classicReceiverBox->setToolTip(i18n("Addressee(s) of this message"));
		classicReceiverBox->setWhatsThis( i18n("Which computer shall receive this message?"));
	} else {
		treeViewReceiverBox->setToolTip(i18n("Addressee(s) of this message"));
		treeViewReceiverBox->setWhatsThis( i18n("Which computer shall receive this message?"));
	}
	buttonSend->setText( i18n("&Send"));
//	buttonSend->setAccel( QKeySequence( i18n("Alt+S")));
	buttonSend->setToolTip(i18n("Send the message" ) );
	buttonSend->setWhatsThis( i18n("Send the message and close the dialog."));
	buttonCancel->setText( i18n("&Cancel"));
//	buttonCancel->setAccel( QKeySequence(i18n("Alt+C")));
	buttonCancel->setToolTip(i18n("Cancel the message"));
	buttonCancel->setWhatsThis( i18n("Cancel the dialog without sending the message."));
}

// kate: tab-width 4; indent-width 4; replace-trailing-space-save on;
