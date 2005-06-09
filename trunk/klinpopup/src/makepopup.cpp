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
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

//#define MY_EXTRA_DEBUG

#include "config.h"

#include <kdebug.h>

#include <qfile.h>
#include <qlabel.h>
#include <qstringlist.h>
#include <qregexp.h>
#include <qgroupbox.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qsplitter.h>

#include <kaction.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kuser.h>
#include <kiconloader.h>
#include <kprocess.h>

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

makePopup::makePopup(QWidget *parent, const char *name, const QString &paramSender,
					 const QString &paramSmbclient, int paramEncoding, int paramView)
	: QWidget(parent, name, WType_TopLevel),
	  smbclientBin(paramSmbclient), messageReceiver(paramSender),
	  newMsgEncoding(paramEncoding), viewMode(paramView),
	  sendRefCount(0), sendError(0), justSending(false),
	  readHosts(this), readGroups(this)
{
	initSmbCtx();
	kdDebug() << "initialized smbCtx" << endl;
	setupLayout();
	kdDebug() << "setup layout" << endl;

	connect(buttonSend, SIGNAL(clicked()), this, SLOT(slotButtonSend()));
	connect(buttonCancel, SIGNAL(clicked()), this, SLOT(finished()));

	if (viewMode == CLASSIC_VIEW) {
		connect(groupBox, SIGNAL(activated(const QString &)), this, SLOT(slotGroupboxChanged(const QString &)));
	 } else {
		connect(groupTreeView, SIGNAL(selectionChanged()), this, SLOT(slotTreeViewSelectionChanged()));
		connect(groupTreeView, SIGNAL(expanded(QListViewItem *)), this, SLOT(slotTreeViewItemExpanded(QListViewItem *)));
	}

	//initialize senderBox, groupBox and receiverBox
	QString tmpHostName = smbCtx->netbios_name;
	if (!tmpHostName.isEmpty()) senderBox->insertItem(tmpHostName);

	QString tmpLoginName = KUser().loginName();
	if (!tmpLoginName.isEmpty()) senderBox->insertItem(tmpLoginName);

	QString tmpFullName = KUser().fullName();
	if (!tmpFullName.isEmpty()) senderBox->insertItem(tmpFullName);

	if (messageReceiver.isEmpty()) {
		if (viewMode == CLASSIC_VIEW) groupBox->insertItem(i18n("NO GROUP"), -1);
		readGroups.start();
	} else {
		groupBox->setEnabled(false);
		classicReceiverBox->insertItem(messageReceiver);
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
	if (e->type() == QEvent::KeyPress) {
		QKeyEvent *k = (QKeyEvent *)e;
		// do funny things to figure out the bytes of the message and react accordingly
		if ((k->key() >= Qt::Key_Space && k->key() <= Qt::Key_ydiaeresis) || k->key() == Qt::Key_Tab) {
			if ((messageText->length() + messageText->lines()) > 1600) return true;
		} else if (k->key() == Qt::Key_Return) {
			if (((messageText->length() + 1) + messageText->lines()) > 1600) return true;
		}
	}

	return false;
}

void makePopup::setupLayout()
{
	setFocusPolicy(QWidget::WheelFocus);

	QGroupBox *messageTextBox;

	buttonSend = new KPushButton(this, "buttonSend");
	buttonSend->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)1, (QSizePolicy::SizeType)0, 0, 0, false));

	buttonCancel = new KPushButton(this, "buttonCancel");
	buttonCancel->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)1, (QSizePolicy::SizeType)0, 0, 0, false));


	if (viewMode == CLASSIC_VIEW) {
		makePopupLayout = new QGridLayout(this, 5, 3, 11, 6, "makePopupLayout");

		QLabel *senderLabel = new QLabel(i18n("From:"), this);
		senderBox = new KComboBox(this);
		senderBox->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)7, (QSizePolicy::SizeType)0, 0, 0, false));
		senderBox->setEditable(true);
		QBoxLayout *senderLayout = new QHBoxLayout(QBoxLayout::LeftToRight);
		senderLayout->add(senderLabel);
		senderLayout->add(senderBox);

		QLabel *groupLabel = new QLabel(i18n("Group:"), this);
		groupBox = new KComboBox(this);
		groupBox->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)7, (QSizePolicy::SizeType)0, 0, 0, false));
		QBoxLayout *groupLayout = new QHBoxLayout(QBoxLayout::LeftToRight);
		groupLayout->add(groupLabel);
		groupLayout->add(groupBox);

		QLabel *receiverLabel = new QLabel(i18n("To:"), this);
		classicReceiverBox = new KComboBox(this);
		classicReceiverBox->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)7, (QSizePolicy::SizeType)0, 0, 0, false));
		classicReceiverBox->setEditable(true);
		QBoxLayout *receiverLayout = new QHBoxLayout(QBoxLayout::LeftToRight);
		receiverLayout->add(receiverLabel);
		receiverLayout->add(classicReceiverBox);

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
		messageTextBox->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)3, (QSizePolicy::SizeType)3, 0, 0, false));
		messageTextBox->setLineWidth(1);
		messageTextBox->setColumnLayout(0, Qt::Vertical);
		messageTextBox->setInsideMargin(4);
		QGridLayout *messageTextBoxLayout = new QGridLayout(messageTextBox->layout());

		messageText = new KTextEdit(messageTextBox);
		messageText->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)3, (QSizePolicy::SizeType)3, 0, 0, false));
		messageTextBoxLayout->addWidget(messageText, 0, 0);

		makePopupLayout->addMultiCellLayout(senderLayout, 0, 0, 0, 2);
		makePopupLayout->addMultiCellLayout(groupLayout, 1, 1, 0, 2);
		makePopupLayout->addMultiCellLayout(receiverLayout, 2, 2, 0, 2);
		makePopupLayout->addMultiCellWidget(messageTextBox, 3, 3, 0, 2);
		makePopupLayout->addWidget( buttonSend, 4, 1);
		makePopupLayout->addWidget( buttonCancel, 4, 2);
		resize(QSize(375, 250).expandedTo(minimumSizeHint()));
	} else {
		makePopupLayout = new QGridLayout( this, 5, 5, 11, 6, "makePopupLayout");
		QSplitter *sp = new QSplitter(this);
		sp->setOpaqueResize(true);

		QVBox *rightSplitLayout = new QVBox(sp);
		rightSplitLayout->setMargin(3);
		groupTreeView = new QListView(rightSplitLayout, "groupTreeView");
		groupTreeView->addColumn(i18n("Groups/Hosts"));
		groupTreeView->addColumn(i18n("Comment"));
		groupTreeView->setEnabled(true);
		groupTreeView->setRootIsDecorated(true);
		groupTreeView->setFrameStyle(QGroupBox::Box|QGroupBox::Plain);
		groupTreeView->setSelectionMode(QListView::Extended);
		groupTreeView->setLineWidth(1);
		groupTreeView->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)3, (QSizePolicy::SizeType)3, 0, 0, false));
		sp->setResizeMode(rightSplitLayout, QSplitter::FollowSizeHint);

		QVBox *leftSplitLayout = new QVBox(sp);
		leftSplitLayout->setSpacing(6);
		leftSplitLayout->setMargin(3);
		QHBox *senderHBox = new QHBox(leftSplitLayout);
		QLabel *senderLabel = new QLabel(i18n("From:"), senderHBox);
		senderBox = new KComboBox(senderHBox);
		senderBox->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)7, (QSizePolicy::SizeType)0, 0, 0, false));
		senderBox->setEditable(true);

		QHBox *receiverHBox = new QHBox(leftSplitLayout);
		QLabel *receiverLabel = new QLabel(i18n("To:"), receiverHBox);
		treeViewReceiverBox = new KLineEdit(receiverHBox);
		treeViewReceiverBox->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)7, (QSizePolicy::SizeType)0, 0, 0, false));

		messageTextBox = new QGroupBox(i18n("Message text"), leftSplitLayout);
		messageTextBox->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)3, (QSizePolicy::SizeType)3, 0, 0, false));
		messageTextBox->setLineWidth(1);
		messageTextBox->setColumnLayout(0, Qt::Vertical);
		messageTextBox->setInsideMargin(4);
		QGridLayout *messageTextBoxLayout = new QGridLayout(messageTextBox->layout());

		messageText = new KTextEdit(messageTextBox);
		messageText->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)7, (QSizePolicy::SizeType)7, 0, 0, false));
		messageTextBoxLayout->addWidget(messageText, 0, 0);

		int senderLabelWidth = senderLabel->sizeHint().width();
		int receiverLabelWidth = receiverLabel->sizeHint().width();

		if (senderLabelWidth < receiverLabelWidth)
			senderLabel->setFixedWidth(receiverLabelWidth);
		else
			receiverLabel->setFixedWidth(senderLabelWidth);

		makePopupLayout->addMultiCellWidget(sp, 0, 3, 0, 4);
		makePopupLayout->addWidget( buttonSend, 4, 3);
		makePopupLayout->addWidget( buttonCancel, 4, 4);
		resize(QSize(575, 250).expandedTo(minimumSizeHint()));
	}

	messageTextBox->setFrameStyle(QGroupBox::Box|QGroupBox::Plain);
	messageTextBox->setAlignment(int(QGroupBox::AlignCenter));

	messageText->setFrameStyle(KTextEdit::LineEditPanel|KTextEdit::Sunken);
	messageText->setResizePolicy(KTextEdit::Manual);
	messageText->setTextFormat(KTextEdit::PlainText);
	messageText->setAutoFormatting(int(KTextEdit::AutoAll));

	languageChange();

	messageText->installEventFilter(this);
	messageText->setFocus();
	kdDebug() << "really setup layout" << endl;
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
			errorHostsString += it.data().upper();
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
	if (readGroups.running()) readGroups.terminate();
	if (readHosts.running()) readHosts.terminate();

	readGroups.wait();
	readHosts.wait();

	smbc_free_context(smbCtx, 1);

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
 * sends the messages with smbclient, opens a KProcess for each
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

	if (tmpReceiverString.find(hostComment) > -1) {
		tmpReceiverList += tmpReceiverString.left(tmpReceiverString.find(" ("));
	} else if (tmpReceiverString == i18n("Whole workgroup")) {
		tmpReceiverList = allGroupHosts;
	} else {
		tmpReceiverList = QStringList::split(whiteSpaces, tmpReceiverString, false);
	}

	allProcessesStarted = false;
	justSending = true;
	QStringList::ConstIterator end = tmpReceiverList.end();
	for (QStringList::ConstIterator it = tmpReceiverList.begin(); it != end; ++it) {
		sendRefCount++;

		KProcess *p = new KProcess(this);
		*p << smbclientBin << "-M" << *it;
		*p << "-N" << "-U" << senderBox->currentText() << "-";

		errorHosts.insert(QString::number((unsigned long)(p), 10), *it);

		connect(p, SIGNAL(processExited(KProcess *)), this, SLOT(slotSendCmdExit(KProcess *)));

		if (p->start(KProcess::NotifyOnExit, KProcess::Stdin)) {
			QString tmpText = messageText->text();

///@TODO: does local8Bit work for all environments?
			switch (newMsgEncoding)
			{
				case ENC_LOCAL8BIT:
					p->writeStdin(tmpText.local8Bit(), tmpText.local8Bit().length());
					break;
				case ENC_UTF8:
					p->writeStdin(tmpText.utf8(), tmpText.utf8().length());
					break;
				case ENC_LATIN1:
					p->writeStdin(tmpText.latin1(), tmpText.length());
					break;
				case ENC_ASCII:
					p->writeStdin(tmpText.ascii(), tmpText.length());
					break;
				default:
					p->writeStdin(tmpText.local8Bit(), tmpText.local8Bit().length());
					break;
			}
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

void makePopup::slotSendCmdExit(KProcess *_p)
{
	if (_p && _p->normalExit() && _p->exitStatus() == 0) {
		errorHosts.erase(QString::number((unsigned long)(_p), 10));
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
	smbCtx = smbc_new_context();
	if (smbCtx) {
		smbCtx->callbacks.auth_fn = makePopup::auth_smbc_get_data;
		smbCtx->timeout = 2000; // any effect?
#ifdef HAVE_SMBCCTX_OPTIONS
		smbCtx->options.urlencode_readdir_entries = 1;
#endif
		smbCtx = smbc_init_context(smbCtx);
		smbc_set_context(smbCtx);
	} // else what?
	kdDebug() << "really initialized smbCtx" << endl;
}

/**
 * read available groups with libsmbclient
 */
void makePopup::readGroupList()
{
	QString ownGroup = QString::null;

	if (smbCtx != 0) {
		// get own workgoup first
		ownGroup = QString::fromUtf8(smbCtx->workgroup, -1);

		kdDebug() << "own group: " << ownGroup << " If this is WORKGROUP and you use 3.0.15preX we may have a problem." << endl;
		if (ownGroup == "WORKGROUP") ownGroup = QString::null; // workaround for SAMBA 3.0.15pre2+

		SMBCFILE *dirfd;
		struct smbc_dirent *dirp = 0;

		// first level should be the workgroups
		dirfd = smbCtx->opendir(smbCtx, "smb://");

		if (dirfd >= 0) {
			do {
				dirp = smbCtx->readdir(smbCtx, dirfd);
				if (dirp == 0) break;
				QString tmpGroup = QString::fromUtf8( dirp->name );
				if (dirp->smbc_type == SMBC_WORKGROUP) {
					if (tmpGroup == ownGroup) {
						if (viewMode == CLASSIC_VIEW) {
							groupBox->insertItem(tmpGroup + " (" + i18n("own") + ")", -1);
						} else {
							QListViewItem *tmpItem = new QListViewItem(groupTreeView, tmpGroup, i18n("own"));
							tmpItem->setPixmap(0, SmallIcon("network_local"));
							tmpItem->setExpandable(true);
							tmpItem->setSelectable(false);
						}
					} else {
						if (viewMode == CLASSIC_VIEW) {
							groupBox->insertItem(tmpGroup, -1);
						} else {
							QListViewItem *tmpItem = new QListViewItem(groupTreeView, tmpGroup);
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

	// initialize with own group if possible
	if (!ownGroup.isEmpty()) {
		if (viewMode == CLASSIC_VIEW) groupBox->setCurrentText(ownGroup + " (" + i18n("own") + ")");
		currentGroup = ownGroup;
		readHosts.start();
	}
}

/**
 * read available hosts from group with libsmbclient
 */
void makePopup::readHostList()
{
	if (smbCtx != 0) {


		SMBCFILE *dirfd;
		struct smbc_dirent *dirp = 0;

		// next level should be the hosts
		QString tmpGroup = "smb://";
		tmpGroup.append(currentGroup);
		kdDebug() << tmpGroup << endl;

		dirfd = smbCtx->opendir(smbCtx, tmpGroup);

		if (dirfd >= 0) {
			do {
				dirp = smbCtx->readdir(smbCtx, dirfd);
				if (dirp == 0) break;

				QString tmpHost = QString::fromUtf8(dirp->name);

				if (dirp->smbc_type == SMBC_SERVER) {
					allGroupHosts += tmpHost;
					QString tmpComment = QString::fromUtf8(dirp->comment);

					if (viewMode == TREE_VIEW) {
						QListViewItem *tmpGroupItem = groupTreeView->findItem(currentGroup, 0);
						if (tmpGroupItem != 0) {
							QListViewItem *tmpHostItem = new QListViewItem(tmpGroupItem, tmpHost, tmpComment);
							tmpHostItem->setPixmap(0, SmallIcon("server"));
							tmpHostItem->setExpandable(false);
						}
						tmpGroupItem->setOpen(true);
					} else {
						if (!tmpComment.isEmpty()) tmpHost.append(" (" + tmpComment + ")");
						classicReceiverBox->insertItem(tmpHost, -1);
					}
				}
			} while (dirp);
		}
		smbCtx->closedir(smbCtx, dirfd);
		if (viewMode == CLASSIC_VIEW && allGroupHosts.count() > 1) classicReceiverBox->insertItem(i18n("Whole workgroup"), -1);
	}
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
	if (currentGroup.find(" (") > 1) currentGroup = currentGroup.left(currentGroup.find(" ("));

	if (readHosts.running()) readHosts.exit();
	readHosts.start();
}

void makePopup::slotTreeViewItemExpanded(QListViewItem *clickedItem)
{
	if (clickedItem == 0 || clickedItem->isSelectable() || clickedItem->childCount() != 0) return;

	currentGroup = clickedItem->text(0);

	if (readHosts.running()) readHosts.exit();
	readHosts.start();
}

void makePopup::slotTreeViewSelectionChanged()
{
	QString selectedHosts;
	QListViewItemIterator it(groupTreeView, QListViewItemIterator::Selected|QListViewItemIterator::NotExpandable);
	while (it.current()) {
		selectedHosts += it.current()->text(0);
		selectedHosts += " ";
		++it;
	}
	treeViewReceiverBox->setText(selectedHosts.stripWhiteSpace());
}

void makePopup::languageChange()
{
	QToolTip::add(senderBox, i18n("Choose sender"));
	QWhatsThis::add(senderBox, i18n("This box lets you customize the sender value."));
	if (viewMode == CLASSIC_VIEW) {
		QToolTip::add(groupBox, i18n("Available workgroups"));
		QWhatsThis::add(groupBox, i18n("Which workgroups are available in the network."));
		QToolTip::add(classicReceiverBox, i18n("Addressee(s) of this message"));
		QWhatsThis::add(classicReceiverBox, i18n("Which computer shall receive this message?"));
	} else {
		QToolTip::add(treeViewReceiverBox, i18n("Addressee(s) of this message"));
		QWhatsThis::add(treeViewReceiverBox, i18n("Which computer shall receive this message?"));
	}
	buttonSend->setText( i18n("&Send"));
	buttonSend->setAccel( QKeySequence( i18n("Alt+S")));
	QToolTip::add( buttonSend, i18n("Send the message" ) );
	QWhatsThis::add( buttonSend, i18n("Send the message and close the dialog."));
	buttonCancel->setText( i18n("&Cancel"));
	buttonCancel->setAccel( QKeySequence(i18n("Alt+C")));
	QToolTip::add( buttonCancel, i18n("Cancel the message"));
	QWhatsThis::add( buttonCancel, i18n("Cancel the dialog without sending the message."));
}

/**
 * method to get authentication data for smbc_init
 */
void makePopup::auth_smbc_get_data(const char */*server*/,const char */*share*/,
								char */*workgroup*/, int /*wgmaxlen*/,
								char */*username*/, int /*unmaxlen*/,
								char */*password*/, int /*pwmaxlen*/)
{
	// no need to really authenticate for the stuff we want
}

// kate: tab-width 4; indent-width 4; replace-trailing-space-save on;
