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

#include <kdebug.h>

#include <unistd.h>

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
#include <QProcess>
#include <QTimer>

#include <KStatusBar>
#include <KLocale>
#include <KMessageBox>
#include <KUser>
#include <KPushButton>
#include <KComboBox>
#include <KLineEdit>
#include <KTextEdit>

#include "makepopup.h"
#include "makepopup.moc"

makePopup::makePopup(QWidget *parent, const QString &paramSender,
					 const QString &paramSmbclient, int paramEncoding, int paramView)
	: QWidget(parent, Qt::Window),
	  smbclientBin(paramSmbclient), messageReceiver(paramSender),
	  newMsgEncoding(paramEncoding), viewMode(paramView),
	  sendRefCount(0), sendError(0), justSending(false),
	  passedInitialHost(false)
{
	setupLayout();

	connect(buttonSend, SIGNAL(clicked()), this, SLOT(slotButtonSend()));
	connect(buttonCancel, SIGNAL(clicked()), this, SLOT(finished()));

	if (viewMode == CLASSIC_VIEW) {
		connect(groupBox, SIGNAL(activated(const QString &)), this, SLOT(slotGroupboxChanged()));
	 } else {
		connect(groupTreeView, SIGNAL(itemSelectionChanged()), this, SLOT(slotTreeViewSelectionChanged()));
	}

	//initialize senderBox, groupBox and receiverBox
	senderBox->addItem(getHostname());

	ownGroup = QString();
	currentHost = QString::fromLatin1("LOCALHOST");

	QString tmpLoginName = KUser().loginName();
	if (!tmpLoginName.isEmpty()) senderBox->addItem(tmpLoginName);

	QString tmpFullName = KUser().property(KUser::FullName).toString();
	if (!tmpFullName.isEmpty()) senderBox->addItem(tmpFullName);

	if (messageReceiver.isEmpty()) {
		if (viewMode == CLASSIC_VIEW) groupBox->addItem(i18n("NO GROUP"), -1);
		QTimer::singleShot(1, this, SLOT(startScan()));
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

void makePopup::setupLayout()
{
	setFocusPolicy(Qt::WheelFocus);

	QGroupBox *messageTextBox;

	QHBoxLayout *buttonLayout = new QHBoxLayout();
	buttonSend = new KPushButton(this);
	buttonCancel = new KPushButton(this);
	buttonLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum));
	buttonLayout->addWidget(buttonSend);
	buttonLayout->addWidget(buttonCancel);

	statusBar = new KStatusBar();
	statusBar->setSizeGripEnabled(false);
	statusBar->insertItem(i18n("0/1600 Bytes"), ID_BYTES, 1);


	if (viewMode == CLASSIC_VIEW) {
		makePopupLayout = new QGridLayout(this);

		QLabel *senderLabel = new QLabel(i18n("From:"), this);
		senderBox = new KComboBox(this);
		senderBox->setEditable(true);

		QLabel *groupLabel = new QLabel(i18n("Group:"), this);
		groupBox = new KComboBox(this);

		QLabel *receiverLabel = new QLabel(i18n("To:"), this);
		classicReceiverBox = new KComboBox(this);
		classicReceiverBox->setEditable(true);

		messageTextBox = new QGroupBox(i18n("Message text"), this);
		QGridLayout *messageTextBoxLayout = new QGridLayout(messageTextBox);

		messageText = new KTextEdit(messageTextBox);
		messageTextBoxLayout->addWidget(messageText, 0, 0);

		makePopupLayout->addWidget(senderLabel, 0, 0);
		makePopupLayout->addWidget(senderBox, 0, 1);
		makePopupLayout->addWidget(groupLabel, 1 , 0);
		makePopupLayout->addWidget(groupBox, 1, 1);
		makePopupLayout->addWidget(receiverLabel, 2, 0);
		makePopupLayout->addWidget(classicReceiverBox, 2, 1);
		makePopupLayout->addWidget(messageTextBox, 3, 0, 1, 2);
		makePopupLayout->addLayout(buttonLayout, 4, 0, 1, 2);
		makePopupLayout->addWidget(statusBar, 5, 0, 1, 2);
		makePopupLayout->setColumnStretch(1, 1);
		resize(QSize(375, 250).expandedTo(minimumSizeHint()));
	} else {
		makePopupLayout = new QGridLayout(this);
		QSplitter *sp = new QSplitter(this);
		sp->setOpaqueResize(true);

		QWidget *rightSplitWidget = new QWidget(sp);
		QGridLayout *rightSplitLayout = new QGridLayout(rightSplitWidget);
		groupTreeView = new QTreeWidget(rightSplitWidget);
		groupTreeView->setColumnCount(2);
		QStringList headers;
		headers << i18n("Groups/Hosts") << i18n("Comment");
		groupTreeView->setHeaderLabels(headers);
		rightSplitLayout->addWidget(groupTreeView);

		QWidget *leftSplitWidget = new QWidget(sp);
		QGridLayout *leftSplitLayout = new QGridLayout(leftSplitWidget);
		QLabel *senderLabel = new QLabel(i18n("From:"));
		senderBox = new KComboBox();
		senderBox->setEditable(true);

		QLabel *receiverLabel = new QLabel(i18n("To:"));
		treeViewReceiverBox = new KLineEdit();

		messageTextBox = new QGroupBox(i18n("Message text"), leftSplitWidget);
		QGridLayout *messageTextBoxLayout = new QGridLayout(messageTextBox);

		messageText = new KTextEdit(messageTextBox);
		messageTextBoxLayout->addWidget(messageText, 0, 0);

		leftSplitLayout->addWidget(senderLabel, 0, 0);
		leftSplitLayout->addWidget(senderBox, 0, 1);
		leftSplitLayout->addWidget(receiverLabel, 1, 0);
		leftSplitLayout->addWidget(treeViewReceiverBox, 1, 1);
		leftSplitLayout->addWidget(messageTextBox, 2, 0, 1, 2);

		makePopupLayout->addWidget(sp, 0, 0);
		makePopupLayout->addLayout(buttonLayout, 1, 0);
		makePopupLayout->addWidget(statusBar, 2, 0);
		makePopupLayout->setRowStretch(0, 1);
		resize(QSize(575, 250).expandedTo(minimumSizeHint()));
	}

	languageChange();

	connect(messageText, SIGNAL(textChanged()), this, SLOT(changeStatusBar()));
	messageText->setFocus();
}

void makePopup::changeStatusBar()
{
	QString tmpText = messageText->document()->toPlainText();
	int bytes = tmpText.toUtf8().size();
	if (bytes < 1601)
		statusBar->changeItem(i18n("%1/1600 Bytes", bytes), ID_BYTES);
    else
		statusBar->changeItem(i18n("%1/1600 Bytes - message will be truncated", bytes), ID_BYTES);
}

/**
 * read the hostname, no Qt/KDE function for this?
 */
QString makePopup::getHostname()
{
    char *tmp = new char[255];
    gethostname(tmp, 255);
    QString hostname = tmp;
    if (hostname.contains('.')) {
        hostname.remove(hostname.indexOf('.'), hostname.length());
    }
    hostname = hostname.toUpper();
    return hostname;
}

void makePopup::queryFinished()
{
	if (!allProcessesStarted || sendRefCount > 0) return;

	if (sendFailedHosts.isEmpty()) {
		KMessageBox::information(this, i18n("Message sent!"), i18n("Success"), "ShowMessageSentSuccess");
	} else {
		QString errorHostsString;
		foreach (QString host, sendFailedHosts) {
			errorHostsString += host.toUpper();
			errorHostsString += ", ";
		}
		errorHostsString.truncate(errorHostsString.length() - 2);
		int tmpYesNo = KMessageBox::warningYesNo(this, i18n("Message could not be sent to %1!\n"
															"Edit/Try again?", errorHostsString));
		if (tmpYesNo == KMessageBox::Yes) {
			sendFailedHosts.clear();
			justSending = false;
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
 * sends the messages with smbclient, opens a NamedProcess for each
 */
void makePopup::sendPopup()
{
	QString tmpText = messageText->document()->toPlainText();
	if (tmpText.toUtf8().size() > 1600) {
		int tmpYesNo = KMessageBox::warningYesNo(this, i18n("The message is too long, it will be truncated by samba!\n"
															"Send anyway?"),
												 i18n("Warning"),
												 KStandardGuiItem::yes(),
												 KStandardGuiItem::no(),
												 "ShowEditAgain");
		if (tmpYesNo == KMessageBox::No) {
			messageText->setFocus();
			return;
		}
	}

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
	sendRefCount = 0;
	foreach (QString receiver, tmpReceiverList) {
		sendRefCount++;

		NamedProcess *p = new NamedProcess(receiver, this);
		QStringList args;
		args << "-M" << receiver << "-N" << "-U" << senderBox->currentText() << "-";

		connect(p, SIGNAL(namedFinished(int, QProcess::ExitStatus, QString)),
				this, SLOT(slotSendCmdExit(int, QProcess::ExitStatus, QString)));

		p->start(smbclientBin, args);
		p->write(tmpText.toUtf8());
		p->closeWriteChannel();
	}
	allProcessesStarted = true;
	queryFinished();
}

void makePopup::slotSendCmdExit(int exitCode, QProcess::ExitStatus status, QString name)
{
	if (exitCode > 0 || status != QProcess::NormalExit) {
		sendFailedHosts.append(name);
	}
	sendRefCount--;
	queryFinished();
}

void makePopup::startScan()
{
	currentHosts.clear();
	currentGroups.clear();
	currentGroup = QString();

	scanProcess = new QProcess();
	QStringList args;
	args << "-N" << "-g" << "-L" << currentHost << "-";

	connect(scanProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(scanNetwork(int, QProcess::ExitStatus)));

	scanProcess->setProcessChannelMode(QProcess::MergedChannels);
	scanProcess->start(smbclientBin, args);
}

/**
 * read available groups/hosts
 */
void makePopup::scanNetwork(int i, QProcess::ExitStatus status)
{
	if (i > 0 || status == QProcess::CrashExit) {
		todo.removeAll(currentHost);
		done += currentHost;
		if (currentHost == QString::fromLatin1("LOCALHOST"))
			KMessageBox::error(this, i18n("Connection to localhost failed. Is your samba server running?"),
							   QString::fromLatin1("KLinPopup"));
	} else {
		QByteArray outputData = scanProcess->readAll();
		if (!outputData.isEmpty()) {
			QString outputString = QString::fromUtf8(outputData.data());
			QStringList outputList = outputString.split('\n');
			QRegExp group("Workgroup\\|(.[^\\|]+)\\|(.+)"), host("Server\\|(.[^\\|]+)\\|(.+)"),
					info("Domain=\\[([^\\]]+)\\] OS=\\[([^\\]]+)\\] Server=\\[([^\\]]+)\\]"),
					error("Connection.*failed");
			foreach (QString line, outputList) {
				if (info.indexIn(line) != -1) currentGroup = info.cap(1);
				if (host.indexIn(line) != -1) currentHosts[host.cap(1)] = host.cap(2);
				if (group.indexIn(line) != -1) currentGroups[group.cap(1)] = group.cap(2);
				if (error.indexIn(line) != -1) currentGroup = QString::fromLatin1("failed");
			}
		}

		delete scanProcess;
		scanProcess = 0;

		// Drop the first cycle - it's only the initial search host,
		// the next round are the real masters. GF

		if (passedInitialHost) {

			// move currentHost from todo to done
			todo.removeAll(currentHost);
			done += currentHost;

			if (viewMode == CLASSIC_VIEW) {
				if (!currentGroup.isEmpty()) {
					QStringList tmpHostList;
					QMap<QString, QString>::const_iterator i = currentHosts.constBegin();
					while (i != currentHosts.constEnd()) {
						tmpHostList << i.key();
						++i;
					}
					allGroups[currentGroup] = tmpHostList;
					if (currentGroup == ownGroup) {
						currentGroup += QString(" (");
						currentGroup += i18n("own");
						currentGroup += QString(")");
						groupBox->insertItem(0, currentGroup);
					} else {
						groupBox->addItem(currentGroup);
					}
				}
			} else {
				QTreeWidgetItem *tmpGroupItem = 0;
				if (!currentGroup.isEmpty()) {
					QStringList groupInfo;
					groupInfo << currentGroup;
					tmpGroupItem = new QTreeWidgetItem(groupInfo);
					tmpGroupItem->setFlags(Qt::ItemIsEnabled);
					groupTreeView->addTopLevelItem(tmpGroupItem);
					if (currentGroup == ownGroup) {
						tmpGroupItem->setText(1, i18n("own"));
						tmpGroupItem->setExpanded(true);
					}
				}

				if (tmpGroupItem && !currentHosts.isEmpty()) {
					QMap<QString, QString>::const_iterator i = currentHosts.constBegin();
					while (i != currentHosts.constEnd()) {
						QTreeWidgetItem *tmpHostItem = new QTreeWidgetItem(tmpGroupItem);
						tmpHostItem->setText(0, i.key());
						tmpHostItem->setText(1, i.value());
						++i;
					}
				}
			}

			if (!currentGroups.isEmpty()) {
				//loop through the read groups and check for new ones
				foreach (QString groupMaster, currentGroups) {
					if (!done.contains(groupMaster)) todo += groupMaster;
				}
			}

		} else {
			kDebug() << currentGroup << endl;
			passedInitialHost = true;
			ownGroup = currentGroup;
			if (!currentGroups.isEmpty()) {
				if (viewMode == CLASSIC_VIEW) groupBox->clear();
				foreach (QString groupMaster, currentGroups) {
					todo += groupMaster;
				}
			}
		}
	}

	// maybe restart cycle
	if (todo.count()) {
		currentHost = todo.at(0);
		startScan();
	} else {
		// initialize with own group if possible or if it's only one
		if (viewMode == CLASSIC_VIEW) {
			if (!ownGroup.isEmpty()) groupBox->setCurrentIndex(0);
			slotGroupboxChanged();
		} else if (viewMode == TREE_VIEW && groupTreeView->topLevelItemCount() == 1) {
			groupTreeView->topLevelItem(0)->setExpanded(true);
		}
	}
}

/**
 * will be called if groupbox is changed
 */
void makePopup::slotGroupboxChanged()
{
	classicReceiverBox->clear();
	QString selectedGroup = groupBox->currentText();
	if (selectedGroup == i18n("NO GROUP")) return;
	if (selectedGroup.indexOf(" (") > 1) selectedGroup = selectedGroup.left(currentGroup.indexOf(" ("));

	QStringList::const_iterator i = allGroups[selectedGroup].constBegin();
	while (i != allGroups[selectedGroup].constEnd()) {
		classicReceiverBox->addItem(*i);
		++i;
	}

}

void makePopup::slotTreeViewSelectionChanged()
{
	QString selectedHosts;
	QTreeWidgetItemIterator it(groupTreeView, QTreeWidgetItemIterator::Selected);
	while (*it) {
		selectedHosts += (*it)->text(0);
		selectedHosts += " ";
		++it;
	}
	treeViewReceiverBox->setText(selectedHosts.trimmed());
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
	buttonSend->setText(i18n("&Send"));
	buttonSend->setShortcut( QKeySequence(i18n("Alt+S")));
	buttonSend->setToolTip(i18n("Send the message" ) );
	buttonSend->setWhatsThis( i18n("Send the message and close the dialog."));
	buttonCancel->setText( i18n("&Cancel"));
	buttonCancel->setShortcut( QKeySequence(i18n("Alt+C")));
	buttonCancel->setToolTip(i18n("Cancel the message"));
	buttonCancel->setWhatsThis( i18n("Cancel the dialog without sending the message."));
}

// kate: tab-width 4; indent-width 4; replace-trailing-space-save on;
