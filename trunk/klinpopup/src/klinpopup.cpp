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

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <asm/unistd.h>
#include "inotify.h"
#include "inotify-syscalls.h"

#include <QToolTip>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
//Added by qt3to4:
#include <QLabel>
#include <QCustomEvent>
#include <QPixmap>
#include <QFocusEvent>
#include <QEvent>

#include <kwindowsystem.h>
#include <kconfigdialog.h>
#include <kglobal.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kdeversion.h>
#include <kmenubar.h>
#include <kstatusbar.h>
#include <phonon/audioplayer.h>
#include <kmessagebox.h>
#include <kedittoolbar.h>
#include <kfileitem.h>
#include <kstdaccel.h>
#include <kstandardaction.h>
#include <kactioncollection.h>

//#include <kpixmap.h>
#include <kstandarddirs.h>
#include <ktoolinvocation.h>
#include <krandom.h>
#include <kdirlister.h>
#include <kguiitem.h>
#include <kstdguiitem.h>

#include "klinpopup.h"
#include "klinpopup.moc"
#include "settings.h"
#include "prefs.h"
#include "makepopup.h"

KLinPopup::KLinPopup()
	: KXmlGuiWindow(),
	  m_view(new KLinPopupView(this)), currentMessage(0), unreadMessages(0),
	  hasInotify(true), m_hostName(QString()), m_arLabel(new QLabel(this))
{
	setFocusPolicy(Qt::StrongFocus);

	setCentralWidget(m_view);
	setupActions();
	setAutoSaveSettings();

	cfg = new KConfig("klinpopuprc");
	readConfig();

	initSystemTray();

	// allow the view to change the statusbar and caption
// 	connect(m_view, SIGNAL(signalChangeStatusbar(const QString&)),
// 			this,   SLOT(changeStatusbar(const QString&)));
// 	connect(m_view, SIGNAL(signalChangeCaption(const QString&)),
// 			this,   SLOT(changeCaption(const QString&)));

	statusBar()->insertItem(QString(), ID_STATUS_TEXT, 1);
	m_arOffPic = KStandardDirs::locate("data", "klinpopup/ar_off.png");
	m_arOnPic = KStandardDirs::locate("data", "klinpopup/ar_on.png");
	m_arLabel->setPixmap(QPixmap(m_arOffPic));
	m_arLabel->setToolTip(i18n("Autoreply off"));
	statusBar()->addWidget(m_arLabel);

	updateStats();
	checkSmbclientBin();

	// use a timer to finish the constructor ASAP
	QTimer::singleShot(1, this, SLOT(startDirLister()));
}

/**
 * reimplemented, only hide when runDocked is checked
 */
bool KLinPopup::queryClose()
{
	if (optRunDocked && !kapp->sessionSaving() && m_systemTray) {
		hide();
		return false;
	} else {
		slotQuit();
		return false;
	}
}

/**
 * Reimplemented focusInEvent to check if we got the focus
 * and update the unreadMessages count.
 */
void KLinPopup::focusInEvent(QFocusEvent *e)
{
	if (e->gotFocus()) {
		if (!messageList.isEmpty() && !messageList.at(currentMessage)->isRead()) {
			--unreadMessages;
			messageList.at(currentMessage)->setRead();
		}
		updateStats();
		setTrayPixmap();
	}
}

// void KLinPopup::customEvent(QCustomEvent *e)
// {
// 	if (e->type() == QEvent::User+1) {
// 		popupFileTimerDone();
// 	} else if (e->type() == QEvent::User+2) {
// 		hasInotify = false;
// 		initTimer();
// 		popupFileTimer->start(1, true);
// 	}
// }

/**
 * setup menu, shortcuts, create GUI
 */
void KLinPopup::setupActions()
{
	KStandardAction::quit(this, SLOT(slotQuit()), actionCollection());

	setStandardToolBarMenuEnabled(true);
	createStandardStatusBarAction();

	m_menubarAction = KStandardAction::showMenubar(this, SLOT(optionsShowMenubar()), actionCollection());

	KStandardAction::keyBindings(this, SLOT(optionsConfigureKeys()), actionCollection());
	KStandardAction::configureToolbars(this, SLOT(optionsConfigureToolbars()), actionCollection());
	KStandardAction::preferences(this, SLOT(optionsPreferences()), actionCollection());

	autoReplyAction = new KToggleAction(KIcon("mail_replyall"), i18n("&Autoreply"), this);
	autoReplyAction->setShortcut(Qt::CTRL+Qt::Key_A);
	actionCollection()->addAction("auto_reply", autoReplyAction);
	connect(autoReplyAction, SIGNAL(triggered(bool)), this, SLOT(statusAutoReply()));

	newPopupAction = new KAction(KIcon("mail_new"), i18n("&New"), this);
	newPopupAction->setShortcut(Qt::CTRL+Qt::Key_N);
	actionCollection()->addAction("new_popup", newPopupAction);
	connect(newPopupAction, SIGNAL(triggered(bool)), this, SLOT(newPopup()));

	replyPopupAction = new KAction(KIcon("mail_reply"), i18n("&Reply"), this);
	replyPopupAction->setShortcut(Qt::CTRL+Qt::Key_R);
	actionCollection()->addAction("reply_popup", replyPopupAction);
	connect(replyPopupAction, SIGNAL(triggered(bool)), this, SLOT(replyPopup()));

	firstPopupAction = new KAction(KIcon("start"), i18n("&First"), this);
	firstPopupAction->setShortcut(Qt::CTRL+Qt::Key_B);
	actionCollection()->addAction("first_popup", firstPopupAction);
	connect(firstPopupAction, SIGNAL(triggered(bool)), this, SLOT(firstPopup()));

	prevPopupAction = new KAction(KIcon("back"), i18n("&Previous"), this);
	prevPopupAction->setShortcut(Qt::CTRL+Qt::Key_P);
	actionCollection()->addAction("previous_popup", prevPopupAction);
	connect(prevPopupAction, SIGNAL(triggered(bool)), this, SLOT(prevPopup()));

	nextPopupAction = new KAction(KIcon("forward"), i18n("&Next"), this);
	nextPopupAction->setShortcut(Qt::CTRL+Qt::Key_F);
	actionCollection()->addAction("next_popup", nextPopupAction);
	connect(nextPopupAction, SIGNAL(triggered(bool)), this, SLOT(nextPopup()));

	lastPopupAction = new KAction(KIcon("finish"), i18n("&Last"), this);
	lastPopupAction->setShortcut(Qt::CTRL+Qt::Key_L);
	actionCollection()->addAction("last_popup", lastPopupAction);
	connect(lastPopupAction, SIGNAL(triggered(bool)), this, SLOT(lastPopup()));

	unreadPopupAction = new KAction(KIcon("new_popup"), i18n("&Unread"), this);
	unreadPopupAction->setShortcut(Qt::CTRL+Qt::Key_U);
	actionCollection()->addAction("unread_popup", unreadPopupAction);
	connect(unreadPopupAction, SIGNAL(triggered(bool)), this, SLOT(unreadPopup()));

	deletePopupAction = new KAction(KIcon("mail_delete"), i18n("&Delete"), this);
	deletePopupAction->setShortcut(Qt::CTRL+Qt::Key_D);
	actionCollection()->addAction("delete_popup", deletePopupAction);
	connect(deletePopupAction, SIGNAL(triggered(bool)), this, SLOT(deletePopup()));

	createGUI();
}

/**
 * init the systemtray
 */
void KLinPopup::initSystemTray()
{
	m_systemTray = new SystemTray(this);
	connect(m_systemTray, SIGNAL(quitSelected()), this, SLOT(slotQuit()));
	m_systemTray->show();
}

/**
 * save window settings etc. and exit
 */
void KLinPopup::slotQuit()
{
	hide();
	saveAutoSaveSettings();
/*	if (watcher) {
		watcher->stop();
		watcher->wait();
	}*/
	kapp->quit();
}

/**
 * init the popupFileTimer
 */
void KLinPopup::startDirLister()
{
	if (checkPopupFileDirectory()) {
		dirLister = new KDirLister();
		dirLister->setAutoUpdate(true);
		connect(dirLister, SIGNAL(newItems(const KFileItemList &)), this, SLOT(newMessages(const KFileItemList &)));
		connect(dirLister, SIGNAL(completed()), this, SLOT(slotListCompleted()));
		dirLister->openUrl(KUrl(POPUP_DIR));
	}
}


/**
 * Check if the popupFileDirectory exists and if the permissions are ok.
 * Should return true in almost every situation.
 */
bool KLinPopup::checkPopupFileDirectory()
{
	QDir dir(POPUP_DIR);
	if (! dir.exists()) {
		int tmpYesNo =  KMessageBox::warningYesNo(this, i18n("Working directory %1 does not exist!\n"
															 "Shall I create it? (May need root password)").arg(POPUP_DIR));
		if (tmpYesNo == KMessageBox::Yes) {
			QStringList kdesuArgs = QStringList(QString("-c mkdir -p -m 0777 " + POPUP_DIR));
			if (KToolInvocation::kdeinitExecWait("kdesu", kdesuArgs) == 0) return true;
		}
	} else {
		KFileItem tmpFileItem = KFileItem(KFileItem::Unknown, KFileItem::Unknown, POPUP_DIR);
		mode_t tmpPerms = tmpFileItem.permissions();

		#ifdef MY_EXTRA_DEBUG
		kDebug() << tmpPerms << endl;
		#endif

		if (tmpPerms != 0777) {

			kDebug() << "Perms not ok!" << endl;

			int tmpYesNo =  KMessageBox::warningYesNo(this, i18n("Permissions of the working directory %1 are wrong!\n"
																 "Fix? (May need root password)").arg(POPUP_DIR));
			if (tmpYesNo == KMessageBox::Yes) {
				QStringList kdesuArgs = QStringList(QString("-c chmod 0777 " + POPUP_DIR));
				if (KToolInvocation::kdeinitExecWait("kdesu", kdesuArgs) == 0)
					return true;
			}
		} else {
			return true;
		}
	}

// 	int tmpContinueQuit = KMessageBox::warningYesNo(this,
// 													i18n("There is a serious problem with the working directory!\n"
// 														 "Only sending messages will work, "
// 														 "else you can manually fix and restart KLinPopup."),
// 													i18n("Warning"),
// 													KStdGuiItem::cont(),
// 													KGuiItem::quit(),
// 													"ShowWarningContinueQuit");
// 	if (tmpContinueQuit != KMessageBox::Yes) slotQuit();

	return false;
}

/**
 * check the smbclient binary
 */
void KLinPopup::checkSmbclientBin()
{
///@TODO: check execution permission?
	QFile tmpSmbclientBin(optSmbclientBin);

	#ifdef MY_EXTRA_DEBUG
	kDebug() << tmpSmbclientBin.exists() << endl;
	#endif

	if (tmpSmbclientBin.exists()) {
		newPopupAction->setEnabled(true);
		replyPopupAction->setEnabled(true);
	} else {
		newPopupAction->setEnabled(false);
		replyPopupAction->setEnabled(false);
		KMessageBox::information(this, i18n("Can't find smbclient executable!\n"
											"Sending messages will not work.\n"
											"Please set the right path in Settings->System."),
								 i18n("Information"), "ShowSmbclientMissing");
	}
}

/**
 * called when popupFileTimer is done,
 * looks for new messages and parses them
 */
void KLinPopup::newMessages(const KFileItemList &items)
{
	KFileItem *tmpItem;
	foreach (tmpItem, items) {
		if (tmpItem->isFile()) {
			QFile popupFile(tmpItem->url().path());

			if (popupFile.open(QIODevice::ReadOnly)) {
				QTextStream stream(&popupFile);
				QString line;
				QString sender;
				QString machine;
				QString ip;
				QString time;
				QString text;
				int i = 0;

				while (! stream.atEnd()) {
					i++;
					line = stream.readLine();

					if (i == 1)  {
						sender.append(line);
						continue;
					}
					if (i == 2) {
						machine.append(line);
						continue;
					}
					if (i == 3) {
						ip.append(line);
						continue;
					}
					if (i == 4) {
						time.append(line);
						continue;
					}
					if (i > 5) {
						text.append('\n');
					}
					text.append(line);
				}

				popupFile.close();

				// delete file
				if (! popupFile.remove())
					kDebug() << "Message file not removed - how that?" << endl;
					signalNewMessage(sender, machine, ip, time, text);
			}
		}
	}
}

/**
 * append and signal a new message
 */
void KLinPopup::signalNewMessage(const QString &popupSender, const QString &popupMachine, const QString &popupIp,
								 const QString &popupTime, const QString &messageText)
{
	kDebug() << "Popup received" << endl;

//	int currentMessage = messageList.at();
	QDateTime tmpDateTime = QDateTime::fromString(popupTime, Qt::ISODate);
	if (!tmpDateTime.isValid()) tmpDateTime = QDateTime::currentDateTime();
	messageList.append(new popupMessage(popupSender, popupMachine, popupIp, tmpDateTime, messageText));
	unreadMessages++;

	switch (optNewMessageSignaling) {
		case MS_NOTHING:    //nothing
			if (currentMessage > -1) messageList.at(currentMessage);
			//show it if it's the only one
			if (messageList.count() == 1) {
				showPopup();
				if (isActiveWindow()) {
					unreadMessages--;
					messageList.at(currentMessage)->setRead();
				}
			}
			updateStats();
			break;
		case MS_SOUND_TRAY:    //play sound
			currentMessage = messageList.count();
			showPopup();
//			KAudioPlayer::play(optNewPopupSound);
			if (isActiveWindow()) {
				--unreadMessages;
				messageList.at(currentMessage)->setRead();
			}
			setTrayPixmap();
			updateStats();
			break;
		case MS_ACTIVATE:    //activate Window
			currentMessage = messageList.count();
			showPopup();
			show();
			KWindowSystem::forceActiveWindow(winId());
			--unreadMessages;
			messageList.at(currentMessage)->setRead();
			setTrayPixmap();
			updateStats();
			break;
		case MS_ALL:    //all
			currentMessage = messageList.count();
			showPopup();
//			KAudioPlayer::play(optNewPopupSound);
			show();
			KWindowSystem::forceActiveWindow(winId());
			--unreadMessages;
			messageList.at(currentMessage)->setRead();
			setTrayPixmap();
			updateStats();
			break;
	}

	if (optExternalCommand) runExternalCommand();
	if (autoReplyAction->isChecked()) autoReply(popupMachine);
}

/**
 * show the updated stats
 */
void KLinPopup::updateStats()
{
	checkMessageMap();

	setCaption(i18n("%1/%2", currentMessage, messageList.count()));
	QString statText = i18n("Message %1/%2 - %3 Unread",
							currentMessage,
							messageList.count(),
							unreadMessages);
	changeStatusbar(statText);
	m_systemTray->setToolTip(statText);
}

/**
 * create the sender text
 */
QString KLinPopup::createSenderText()
{
	QString tmpSenderText = "";
	if (optDisplaySender == true) {
		tmpSenderText.append(messageList.at(currentMessage)->sender());
	}
	if (optDisplayMachine == true) {
		if (!tmpSenderText.isEmpty()) {
			tmpSenderText.append("/");
		}
		tmpSenderText.append(messageList.at(currentMessage)->machine());
	}
	if (optDisplayIP == true) {
		if (!tmpSenderText.isEmpty()) {
			tmpSenderText.append("/");
		}
		tmpSenderText.append(messageList.at(currentMessage)->ip());
	}
	return tmpSenderText;
}

/**
 * check message status
 * enable/disable buttons accordingly
 */
void KLinPopup::checkMessageMap()
{
	int popupCounter = messageList.count();

	if (popupCounter != 0) {
		replyPopupAction->setEnabled(true);
		deletePopupAction->setEnabled(true);
	} else {
		replyPopupAction->setEnabled(false);
		deletePopupAction->setEnabled(false);
	}

	if (popupCounter > 1 && currentMessage > 0) {
		prevPopupAction->setEnabled(true);
		firstPopupAction->setEnabled(true);
	} else {
		prevPopupAction->setEnabled(false);
		firstPopupAction->setEnabled(false);
	}

	if (popupCounter > 1 && currentMessage < (popupCounter - 1)) {
		nextPopupAction->setEnabled(true);
		lastPopupAction->setEnabled(true);
	} else {
		nextPopupAction->setEnabled(false);
		lastPopupAction->setEnabled(false);
	}

	if (unreadMessages > 0)
		unreadPopupAction->setEnabled(true);
	else
		unreadPopupAction->setEnabled(false);
}

/**
 * show popup
 */
void KLinPopup::showPopup()
{
	if (!messageList.isEmpty()) {
		m_view->displayNewMessage(createSenderText(),
								messageList.at(currentMessage-1)->time(),
								messageList.at(currentMessage-1)->text(),
								optTimeFormat);
	} else {
		m_view->displayNewMessage("", QDateTime::QDateTime(), "", optTimeFormat);
	}
}

/**
 * new message
 */
void KLinPopup::newPopup()
{
	makePopup *newPopupView = new makePopup(this, "", optSmbclientBin, optEncoding, optMakePopupView);
	newPopupView->setWindowTitle(i18n("New message"));

	//some unnecessary fooling around, random window position
	//but with a reasonable offset from parent
	int tmpRandX = 70 + (KRandom::random() % 50);
	int tmpRandY = 70 + (KRandom::random() % 70);
	newPopupView->move(x() + tmpRandX, y() + tmpRandY);

	newPopupView->show();
}

void KLinPopup::deletePopup()
{
	delete messageList.at(currentMessage);
	messageList.removeAt(currentMessage);
	--currentMessage;
	showPopup();
	popupHelper();
}

/**
 * reply message
 */
void KLinPopup::replyPopup()
{
	QString sender = messageList.at(currentMessage)->machine();

	makePopup *replyPopupView = new makePopup(this, sender, optSmbclientBin, optEncoding, 0);
	replyPopupView->setWindowTitle(i18n("Reply to %1").arg(sender.toUpper()));

	int tmpRandX = 70 + (KRandom::random() % 50);
	int tmpRandY = 70 + (KRandom::random() % 70);
	replyPopupView->move(x() + tmpRandX, y() + tmpRandY);

	replyPopupView->show();
}

/**
 * search next unread message
 */
void KLinPopup::unreadPopup()
{
	for (int i = 0; i < messageList.count(); i++) {
		if (messageList.at(i)->isRead() == false) {
			break;
		}
	}

	showPopup();
	popupHelper();
}

/**
 * common code to check readStatus and tray icon
 */
void KLinPopup::popupHelper()
{
	if (!messageList.isEmpty() && !messageList.at(currentMessage)->isRead()) {
		unreadMessages--;
		messageList.at(currentMessage)->setRead();
	}

	setTrayPixmap();
	updateStats();
}

/**
 * run an external command
 */
void KLinPopup::runExternalCommand()
{
	QString program = QString();
	QString args = QString();
	int pos = optExternalCommandURL.indexOf(" ");
	if (pos > 0) {
		program = optExternalCommandURL.left(pos);
		args = optExternalCommandURL.right(optExternalCommandURL.length() - pos - 1);
		KToolInvocation::kdeinitExec(program, args.split(" ")); // don't care about the result
	} else if (!optExternalCommandURL.isEmpty()) {
		KToolInvocation::kdeinitExec(program); // don't care about the result
	}
}

/**
 * automatically send an away message
 */
void KLinPopup::statusAutoReply()
{
	if (autoReplyAction->isChecked()) {
		m_arLabel->setPixmap(QPixmap(m_arOnPic));
		m_arLabel->setToolTip(i18n("Autoreply on"));
	} else {
		m_arLabel->setPixmap(QPixmap(m_arOffPic));
		m_arLabel->setToolTip(i18n("Autoreply off"));
	}

	setTrayPixmap();
}

void KLinPopup::autoReply(const QString &host)
{
	if (m_hostName.isEmpty()) {
		QString theHostName = QString::null;
		char *tmp = new char[255];

		if (tmp != 0) {
			gethostname(tmp, 255);
			m_hostName = tmp;
			if (m_hostName.contains('.') != 0) m_hostName.remove(m_hostName.indexOf('.'), m_hostName.length());
			m_hostName = m_hostName.toUpper();
		}
	}

	if (host.toUpper() != "LOCALHOST" && host.toUpper() != m_hostName) { /// prevent endless loop
		K3Process *p = new K3Process(this);
		*p << optSmbclientBin << "-M" << host;
		*p << "-N" << "-";

		connect(p, SIGNAL(processExited(K3Process *)), this, SLOT(slotSendCmdExit(K3Process *)));

		if (p->start(K3Process::NotifyOnExit, K3Process::Stdin)) {
			p->writeStdin(optArMsg.toUtf8(), optArMsg.toUtf8().length());
			if (!p->closeStdin()) {
				///@TODO: does this work, how to test this?
				delete p;
			}
		} else {
			slotSendCmdExit(0);
		}
	}
}

void KLinPopup::slotSendCmdExit(K3Process *_p)
{
	delete _p;
}

/**
 * toggle Menubar
 */
void KLinPopup::optionsShowMenubar()
{
	if (menuBar()->isHidden())
		menuBar()->show();
	else
		menuBar()->hide();
}

/**
 * show toolbar config dialog
 */
void KLinPopup::optionsConfigureToolbars()
{
	// use the standard toolbar editor
	saveMainWindowSettings(KConfigGroup(KGlobal::config(), autoSaveGroup()));
	KEditToolBar dlg(actionCollection());
	connect(&dlg, SIGNAL(newToolbarConfig()), this, SLOT(newToolbarConfig()));
	dlg.exec();
}

/**
 * rebuild GUI after toolbar change
 */
void KLinPopup::newToolbarConfig()
{
	setupGUI();
	applyMainWindowSettings(KConfigGroup(KGlobal::config(), autoSaveGroup()));
}

/**
 * show config dialog
 */
void KLinPopup::optionsPreferences()
{
		KConfigDialog *dialog = new KConfigDialog(this, "settings", Settings::self());
		Prefs *prefs = new Prefs();
		dialog->addPage(prefs, i18n("Settings"));
		connect(dialog, SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));
		dialog->show();
		prefs->toggleURLs();
}

/**
 * read configuration
 */
void KLinPopup::readConfig()
{
	optRunDocked = Settings::runDocked();
	optDisplaySender = Settings::displaySender();
	optDisplayMachine = Settings::displayMachine();
	optDisplayIP = Settings::displayIP();
	optTimeFormat = Settings::timeFormat();
	optNewMessageSignaling = Settings::toggleSignaling();
	optNewPopupSound = Settings::soundURL().trimmed();
	optExternalCommand = Settings::externalCommand();
	optExternalCommandURL = Settings::externalCommandURL().simplified();
	optArMsg = Settings::arMsg();
	optTimerInterval= Settings::timerInterval();
	optMakePopupView = Settings::makePopupView();
	optSmbclientBin=Settings::smbclientBin();
	optEncoding=Settings::encoding();
}

/**
 * called when settings have changed
 * adapt options to changes
 */
void KLinPopup::settingsChanged()
{
	Settings::self()->writeConfig();
	readConfig();

	// Avoid empty receiverBox
	if (!optDisplaySender && !optDisplayMachine && !optDisplayIP) {
		Settings::setDisplaySender(true);
		Settings::self()->writeConfig();
		optDisplaySender = Settings::displaySender();
	}

	if (optNewMessageSignaling == MS_SOUND_TRAY) {
		setTrayPixmap();
	}

	if (!hasInotify) popupFileTimer->setInterval(optTimerInterval * 1000);
	showPopup();
	checkSmbclientBin();
}

/**
 * change the Statusbar text
 */
void KLinPopup::changeStatusbar(const QString &text)
{
	statusBar()->changeItem(text, ID_STATUS_TEXT);
}

void KLinPopup::setTrayPixmap()
{
	if (unreadMessages == 0) {
		if (autoReplyAction->isChecked())
			m_systemTray->changeTrayPixmap(NORMAL_ICON_AR);
		else
			m_systemTray->changeTrayPixmap(NORMAL_ICON);
	} else {
		if (autoReplyAction->isChecked())
			m_systemTray->changeTrayPixmap(NEW_ICON_AR);
		else
			m_systemTray->changeTrayPixmap(NEW_ICON);
	}
}

// kate: tab-width 4; indent-width 4; replace-trailing-space-save on;
