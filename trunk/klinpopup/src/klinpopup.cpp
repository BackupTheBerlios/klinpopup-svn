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

#include <qtooltip.h>
#include <qdir.h>
#include <qfile.h>
#include <qstringlist.h>

#include <kwin.h>
#include <kconfigdialog.h>
#include <kglobal.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kdeversion.h>
#include <kmenubar.h>
#include <kstatusbar.h>
#include <kaudioplayer.h>
#include <kmessagebox.h>
#include <kedittoolbar.h>
#include <kfileitem.h>
#include <kstdaccel.h>
#include <kstdaction.h>

#include "klinpopup.h"
#include "klinpopup.moc"
#include "settings.h"
#include "prefs.h"
#include "makepopup.h"

bool selectThread::openInotify()
{
	fd = inotify_init();

	if (fd < 0) {
		switch (errno) {
			case ENOSYS:
				inotifyErrorEvent *ie = new inotifyErrorEvent();
				kapp->postEvent(owner, ie);
				return false;
				break;
		}
	} else {
		wd = inotify_add_watch (fd, POPUP_DIR, IN_MOVED_TO);
		if (wd < 0) {
			inotifyErrorEvent *ie = new inotifyErrorEvent();
			kapp->postEvent(owner, ie);
			closeInotify();
			return false;
		}
	}

	kdDebug() << "Using inotify" << endl;
	return true;
}

void selectThread::closeInotify()
{
	if (fd >=0) {
		int c = ::close(fd);
		if (c == 0) fd = -1;
	}
}

void selectThread::watch()
{
	int pending;
	int select_retval = 1;
	fd_set read_fds;
	struct timeval timeout;

	while (select_retval >= 0 && restart) {
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;
		FD_ZERO (&read_fds);
		FD_SET (fd, &read_fds);
		select_retval = select(fd + 1, &read_fds, 0, 0, &timeout);
		if (select_retval > 0) {
			ioctl(fd, FIONREAD, &pending);
			char *buffer = new char[pending];
			read(fd, buffer, pending);
			delete buffer;
			newMessagesEvent *nm = new newMessagesEvent();
			kapp->postEvent(owner, nm);
		}
	}
}

void selectThread::run()
{
	if (openInotify()) {
		watch();
		closeInotify();
	}
}

KLinPopup::KLinPopup()
	: KMainWindow( 0, "KLinPopup" ),
	  m_view(new KLinPopupView(this)), watcher(0),
	  unreadMessages(0), hasInotify(true)
{
	setFocusPolicy(QWidget::StrongFocus);

	// tell the KMainWindow that this is indeed the main widget
	setCentralWidget(m_view);

	// then, setup our actions
	setupActions();

	// apply the saved mainwindow settings, if any, and ask the mainwindow
	// to automatically save settings if changed: window size, toolbar
	// position, icon size, etc.
	setAutoSaveSettings();

	cfg = KGlobal::config();
	readConfig();

	initSystemTray();

	// allow the view to change the statusbar and caption
	connect(m_view, SIGNAL(signalChangeStatusbar(const QString&)),
			this,   SLOT(changeStatusbar(const QString&)));
	connect(m_view, SIGNAL(signalChangeCaption(const QString&)),
			this,   SLOT(changeCaption(const QString&)));


	updateStats();
	checkSmbclientBin();

	messageList.setAutoDelete(true);

	// use a timer to finish the constructor ASAP
	QTimer *watchTimer = new QTimer(this);
	connect(watchTimer, SIGNAL(timeout()), this, SLOT(startWatch()));
	watchTimer->start(1, true);
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
		if (!messageList.isEmpty() && !messageList.current()->isRead()) {
			unreadMessages--;
			messageList.current()->setRead();
		}
		updateStats();
		if (unreadMessages == 0) m_systemTray->changeTrayPixmap(NORMAL_ICON);
	}
}

void KLinPopup::customEvent(QCustomEvent *e)
{
	if (e->type() == QEvent::User+1) {
		popupFileTimerDone();
	} else if (e->type() == QEvent::User+2) {
		hasInotify = false;
		initTimer();
		popupFileTimer->start(1, true);
	}
}

/**
 * setup menu, shortcuts, create GUI
 */
void KLinPopup::setupActions()
{
	KStdAction::quit(this, SLOT(slotQuit()), actionCollection());

	setStandardToolBarMenuEnabled(true);
	createStandardStatusBarAction();

	m_menubarAction = KStdAction::showMenubar(this, SLOT(optionsShowMenubar()), actionCollection());

	KStdAction::keyBindings(this, SLOT(optionsConfigureKeys()), actionCollection());
	KStdAction::configureToolbars(this, SLOT(optionsConfigureToolbars()), actionCollection());
	KStdAction::preferences(this, SLOT(optionsPreferences()), actionCollection());

	newPopupAction = new KAction(i18n("&New"),
								 "mail_new", CTRL+Key_N,
								 this, SLOT(newPopup()),
								 actionCollection(), "new_popup");
	replyPopupAction = new KAction(i18n("&Reply"),
								   "mail_reply", CTRL+Key_R,
								   this, SLOT(replyPopup()),
								   actionCollection(), "reply_popup");
	firstPopupAction = new KAction(i18n("&First"),
								   "start", CTRL+Key_B,
								   this, SLOT(firstPopup()),
								   actionCollection(), "first_popup");
	prevPopupAction = new KAction(i18n("&Previous"),
								  "back", CTRL+Key_P,
								  this, SLOT(prevPopup()),
								  actionCollection(), "previous_popup");
	nextPopupAction = new KAction(i18n("&Next"),
								  "forward", CTRL+Key_F,
								  this, SLOT(nextPopup()),
								  actionCollection(), "next_popup");
	lastPopupAction = new KAction(i18n("&Last"),
								  "finish", CTRL+Key_L,
								  this, SLOT(lastPopup()),
								  actionCollection(), "last_popup");
	unreadPopupAction = new KAction(i18n("&Unread"),
									"new_popup", CTRL+Key_U,
									this, SLOT(unreadPopup()),
									actionCollection(), "unread_popup");
	deletePopupAction = new KAction(i18n("&Delete"),
									"mail_delete", CTRL+Key_D,
									this, SLOT(deletePopup()),
									actionCollection(), "delete_popup");
	createGUI();
}

/**
 * init the systemtray
 */
void KLinPopup::initSystemTray()
{
	m_systemTray = new SystemTray(this, "systemTray");
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
	if (watcher) {
		watcher->stop();
		watcher->wait();
	}
	kapp->quit();
}

/**
 * init the popupFileTimer
 */
void KLinPopup::initTimer()
{
	popupFileTimer = new QTimer(this);
	connect(popupFileTimer, SIGNAL(timeout()), this, SLOT(popupFileTimerDone()));
}

void KLinPopup::startWatch()
{
	if (checkPopupFileDirectory()) {
		initWatch();
		popupFileTimerDone();
	}
}

void KLinPopup::initWatch()
{
	watcher = new selectThread();
	watcher->setData(this);
	watcher->start();
}

/**
 * Check if the popupFileDirectory exists and if the permissions are ok.
 * Should return true in almost every situation.
 */
bool KLinPopup::checkPopupFileDirectory()
{
	QDir dir(POPUP_DIR);
	if (! dir.exists()) {
		int tmpYesNo =  KMessageBox::warningYesNo(this, i18n("Working directory /var/lib/klinpopup/ does not exist!\n"
															 "Shall I create it? (May need root password)"));
		if (tmpYesNo == KMessageBox::Yes) {
			QStringList kdesuArgs;
			kdesuArgs += "-c";
			kdesuArgs += "mkdir -p -m 0777 /var/lib/klinpopup";
			if (KApplication::kdeinitExecWait("kdesu", kdesuArgs) == 0) return true;
		}
	} else {
		KFileItem tmpFileItem = KFileItem(KFileItem::Unknown, KFileItem::Unknown, "/var/lib/klinpopup");
		mode_t tmpPerms = tmpFileItem.permissions();

		#ifdef MY_EXTRA_DEBUG
		kdDebug() << tmpPerms << endl;
		#endif

		if (tmpPerms != 0777) {

			kdDebug() << "Perms not ok!" << endl;

			int tmpYesNo =  KMessageBox::warningYesNo(this, i18n("Permissions of the working directory /var/lib/klinpopup/ are wrong!\n"
																 "Fix? (May need root password)"));
			if (tmpYesNo == KMessageBox::Yes) {
				QStringList kdesuArgs;
				kdesuArgs += "-c";
				kdesuArgs += "chmod 0777 /var/lib/klinpopup";
				if (KApplication::kdeinitExecWait("kdesu", kdesuArgs) == 0)
					return true;
			}
		} else {
			return true;
		}
	}

	int tmpContinueQuit = KMessageBox::warningYesNo(this,
													i18n("There is a serious problem with the working directory!\n"
														 "Only sending messages will work, "
														 "else you can manually fix and restart KLinPopup."),
													i18n("Warning"),
													i18n("&Continue"),
													i18n("&Quit"),
													"ShowWarningContinueQuit");
	if (tmpContinueQuit != KMessageBox::Yes) slotQuit();

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
	kdDebug() << tmpSmbclientBin.exists() << endl;
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
void KLinPopup::popupFileTimerDone()
{
// maybe we can use inotify here some day
	if (checkPopupFileDirectory()) {
		QDir dir(POPUP_DIR);
		const QFileInfoList *popupFiles = dir.entryInfoList(QDir::Files, QDir::Name);
		if (popupFiles) {
			QFileInfoListIterator it(*popupFiles);
			QFileInfo *popupFileInfo;
			while((popupFileInfo = it.current()) != 0) {
				++it;
				if (popupFileInfo -> isFile()) {
					QString popupFileName(popupFileInfo -> fileName());
					QString popupFilePath(POPUP_DIR);
					popupFilePath.append("/");
					popupFilePath.append(popupFileName);

					QFile popupFile(popupFilePath);

					kdDebug() << "popupFile " << popupFileName << endl;

					if (popupFile.open(IO_ReadOnly)) {
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
							kdDebug() << "Message file not removed - how that?" << endl;

						signalNewMessage(sender, machine, ip, time, text);
					}
				}
			}
		}
		if (!hasInotify) popupFileTimer->start(optTimerInterval * 1000, true);
	}
}

/**
 * append and signal a new message
 */
void KLinPopup::signalNewMessage(const QString &popupSender, const QString &popupMachine, const QString &popupIp,
								 const QString &popupTime, const QString &messageText)
{
	kdDebug() << "Popup received" << endl;

	int currentMessage = messageList.at();
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
					messageList.current()->setRead();
				}
			}
			updateStats();
			break;
		case MS_SOUND_TRAY:    //play sound
			showPopup();
			KAudioPlayer::play(optNewPopupSound);
			if (isActiveWindow()) {
				unreadMessages--;
				messageList.current()->setRead();
				if (unreadMessages == 0)
					m_systemTray->changeTrayPixmap(NORMAL_ICON);
			} else {
				m_systemTray->changeTrayPixmap(NEW_ICON);
			}
			updateStats();
			break;
		case MS_ACTIVATE:    //activate Window
			showPopup();
			show();
			KWin::forceActiveWindow(winId());
			unreadMessages--;
			messageList.current()->setRead();
			if (unreadMessages == 0)
				m_systemTray->changeTrayPixmap(NORMAL_ICON);
			updateStats();
			break;
		case MS_ALL:    //all
			showPopup();
			KAudioPlayer::play(optNewPopupSound);
			show();
			KWin::forceActiveWindow(winId());
			unreadMessages--;
			messageList.current()->setRead();
			if (unreadMessages == 0)
				m_systemTray->changeTrayPixmap(NORMAL_ICON);
			updateStats();
			break;
	}

	if (optExternalCommand) runExternalCommand();
}

/**
 * show the updated stats
 */
void KLinPopup::updateStats()
{
	checkMessageMap();

	changeCaption(i18n("%1/%2")
				  .arg(messageList.at() + 1)
				  .arg(messageList.count()));
	QString statText = (i18n("Message %1/%2 - %3 Unread")
						.arg(messageList.at() + 1)
						.arg(messageList.count())
						.arg(unreadMessages));
	changeStatusbar(statText);
	QToolTip::remove(m_systemTray);
	QToolTip::add(m_systemTray, statText);
}

/**
 * create the sender text
 */
QString KLinPopup::createSenderText()
{
	QString tmpSenderText = "";
	if (optDisplaySender == true) {
		tmpSenderText.append(messageList.current()->sender());
	}
	if (optDisplayMachine == true) {
		if (!tmpSenderText.isEmpty()) {
			tmpSenderText.append("/");
		}
		tmpSenderText.append(messageList.current()->machine());
	}
	if (optDisplayIP == true) {
		if (!tmpSenderText.isEmpty()) {
			tmpSenderText.append("/");
		}
		tmpSenderText.append(messageList.current()->ip());
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
	int actualPopup = messageList.at();

	if (popupCounter != 0) {
		replyPopupAction->setEnabled(true);
		deletePopupAction->setEnabled(true);
	} else {
		replyPopupAction->setEnabled(false);
		deletePopupAction->setEnabled(false);
	}

	if (popupCounter > 1 && actualPopup > 0) {
		prevPopupAction->setEnabled(true);
		firstPopupAction->setEnabled(true);
	} else {
		prevPopupAction->setEnabled(false);
		firstPopupAction->setEnabled(false);
	}

	if (popupCounter > 1 && actualPopup < (popupCounter - 1)) {
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
								messageList.current()->time(),
								messageList.current()->text(),
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
	makePopup *newPopupView = new makePopup(this, "New message", "", optSmbclientBin, optEncoding, optMakePopupView);
	newPopupView->setCaption(i18n("New message"));

	//some unnecessary fooling around, random window position
	//but with a reasonable offset from parent
	int tmpRandX = 70 + (KApplication::random() % 50);
	int tmpRandY = 70 + (KApplication::random() % 70);
	newPopupView->move(x() + tmpRandX, y() + tmpRandY);

	newPopupView->show();
}

/**
 * reply message
 */
void KLinPopup::replyPopup()
{
	QString sender = messageList.current()->machine();

	makePopup *replyPopupView = new makePopup(this, "Reply message", sender, optSmbclientBin, optEncoding, 0);
	replyPopupView->setCaption(i18n("Reply to %1").arg(sender.upper()));

	int tmpRandX = 70 + (KApplication::random() % 50);
	int tmpRandY = 70 + (KApplication::random() % 70);
	replyPopupView->move(x() + tmpRandX, y() + tmpRandY);

	replyPopupView->show();
}

/**
 * search next unread message
 */
void KLinPopup::unreadPopup()
{
	for (uint i = 0; i < messageList.count(); i++) {
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
	if (!messageList.isEmpty() && !messageList.current()->isRead()) {
		unreadMessages--;
		messageList.current()->setRead();
	}
	if (unreadMessages == 0)
		m_systemTray->changeTrayPixmap(NORMAL_ICON);

	updateStats();
}

/**
 * run an external command
 */
void KLinPopup::runExternalCommand()
{
	QString program = QString::null;
	QString args = QString::null;
	int pos = optExternalCommandURL.find(" ");
	if (pos > 0) {
		program = optExternalCommandURL.left(pos);
		args = optExternalCommandURL.right(optExternalCommandURL.length() - pos - 1);
		KApplication::kdeinitExec(program, QStringList::split(" ", args)); // don't care about the result
	} else if (!optExternalCommandURL.isEmpty()) {
		KApplication::kdeinitExec(program); // don't care about the result
	}
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
	saveMainWindowSettings(KGlobal::config(), autoSaveGroup());
	KEditToolbar dlg(actionCollection());
	connect(&dlg, SIGNAL(newToolbarConfig()), this, SLOT(newToolbarConfig()));
	dlg.exec();
}

/**
 * rebuild GUI after toolbar change
 */
void KLinPopup::newToolbarConfig()
{
	// this slot is called when user clicks "Ok" or "Apply" in the toolbar editor.
	// recreate our GUI, and re-apply the settings (e.g. "text under icons", etc.)
	createGUI();

	applyMainWindowSettings(KGlobal::config(), autoSaveGroup());
}

/**
 * show config dialog
 */
void KLinPopup::optionsPreferences()
{
		// The preference dialog is derived from prefs-base.ui which is subclassed into Prefs
		//
		// compare the names of the widgets in the .ui file
		// to the names of the variables in the .kcfg file
		KConfigDialog *dialog = new KConfigDialog(this, "settings", Settings::self(), KDialogBase::Swallow);
		dialog->addPage(new Prefs(), i18n("Settings"), "settings");
		connect(dialog, SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));
		dialog->show();
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
	optNewPopupSound = Settings::soundURL().stripWhiteSpace();
	optExternalCommand = Settings::externalCommand();
	optExternalCommandURL = Settings::externalCommandURL().simplifyWhiteSpace();
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
	Settings::writeConfig();
	readConfig();

	// Avoid empty receiverBox
	if (!optDisplaySender && !optDisplayMachine && !optDisplayIP) {
		Settings::setDisplaySender(true);
		Settings::writeConfig();
		optDisplaySender = Settings::displaySender();
	}

	if (optNewMessageSignaling == MS_SOUND_TRAY && unreadMessages != 0) {
		m_systemTray->changeTrayPixmap(NEW_ICON);
	}

	if (!hasInotify) popupFileTimer->changeInterval(optTimerInterval * 1000);
	showPopup();
	checkSmbclientBin();
}

/**
 * change the Statusbar text
 */
void KLinPopup::changeStatusbar(const QString &text)
{
	if (!statusBar()->hasItem(ID_STATUS_TEXT)) {
		statusBar()->insertItem(text, ID_STATUS_TEXT, 1, true);
	} else {
		statusBar()->changeItem(text, ID_STATUS_TEXT);
	}
}

// kate: tab-width 4; indent-width 4; replace-trailing-space-save on;
