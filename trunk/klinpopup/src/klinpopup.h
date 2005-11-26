/***************************************************************************
*   Copyright (C) 2004 by Gerd Fleischer                                  *
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

#ifndef KLINPOPUP_H
#define KLINPOPUP_H

#define POPUP_DIR "/var/lib/klinpopup"
#define ID_STATUS_TEXT 10
#define NEW_ICON 0
#define NORMAL_ICON 1
#define MS_NOTHING 0
#define MS_SOUND_TRAY 1
#define MS_ACTIVATE 2
#define MS_ALL 3

#include <qtimer.h>
#include <qstring.h>
#include <qptrlist.h>
#include <qthread.h>
#include <qevent.h>

#include <kuniqueapplication.h>
#include <kmainwindow.h>
#include <kaction.h>
#include <kconfig.h>
#include <kkeydialog.h>

#include "popupmessage.h"
#include "klinpopupview.h"
#include "systemtray.h"

class KLinPopup;

class newMessagesEvent : public QCustomEvent
{
	public:
		newMessagesEvent() : QCustomEvent(QEvent::User+1) {}
};

class inotifyErrorEvent : public QCustomEvent
{
	public:
		inotifyErrorEvent() : QCustomEvent(QEvent::User+2) {}
};

class selectThread : public QThread
{
	public:
		void setData( KLinPopup* parent) { owner = parent; fd = -1, restart = 1; }
		void stop() { restart = 0; }

	protected:
		virtual void run();

	private:
		KLinPopup *owner;
		int fd, wd, restart;

		bool openInotify();
		void closeInotify();
		void watch();
};

/**
 * @short Main window class
 * @author Gerd Fleischer <gerdfleischer@web.de>
 * @version 0.3.2
 */
class KLinPopup : public KMainWindow
{
	Q_OBJECT
public:
	KLinPopup();

public slots:
	void newPopup();

protected:
	void hideEvent(QHideEvent *) { hide(); }
	void focusInEvent(QFocusEvent *);
	void customEvent(QCustomEvent *);

private slots:
	void slotQuit();
	void startWatch();
	void popupFileTimerDone();
	void signalNewMessage(const QString &, const QString &, const QString &, const QString &, const QString &);
	void replyPopup();
	void firstPopup() {  messageList.first(); showPopup(); popupHelper(); }
	void prevPopup() { messageList.prev(); showPopup(); popupHelper(); }
	void nextPopup() { messageList.next(); showPopup(); popupHelper(); }
	void lastPopup() { messageList.last(); showPopup(); popupHelper(); }
	void unreadPopup();
	void deletePopup() { messageList.remove(); showPopup(); popupHelper(); }
	void optionsShowMenubar();
	void optionsConfigureKeys() { KKeyDialog::configure(actionCollection()); }
	void optionsConfigureToolbars();
	void optionsPreferences();
	void newToolbarConfig();
	void changeStatusbar(const QString &);
	void changeCaption(const QString &text) { setCaption(text); }
	void settingsChanged();

private:
	virtual bool queryClose();
	void setupAccel();
	void setupActions();
	void initSystemTray();
	void initWatch();
	void initTimer();
	bool checkPopupFileDirectory();
	void checkSmbclientBin();
	void checkMessageMap();
	void showPopup();
	void updateStats();
	QString createSenderText();
	void readConfig();
	void popupHelper();

	KLinPopupView *m_view;
	selectThread *watcher;

	//config and actions
	KConfig *cfg;
	SystemTray *m_systemTray;
	KToggleAction *m_menubarAction;
	KAction *newPopupAction;
	KAction *replyPopupAction;
	KAction *firstPopupAction;
	KAction *prevPopupAction;
	KAction *nextPopupAction;
	KAction *lastPopupAction;
	KAction *unreadPopupAction;
	KAction *deletePopupAction;

	int unreadMessages;
	bool hasInotify;
	QTimer *popupFileTimer;
	QString messageText;
	QString popupFileDirectory;
	QPtrList<popupMessage> messageList;

	//option variables
	bool optRunDocked;
	int optTimerInterval;
	bool optDisplaySender;
	bool optDisplayMachine;
	bool optDisplayIP;
	int optTimeFormat;
	int optNewMessageSignaling;
	QString optNewPopupSound;
	int optMakePopupView;
	QString optSmbclientBin;
	int optEncoding;
};

#endif // KLINPOPUP_H

// kate: tab-width 4; indent-width 4; replace-trailing-space-save on;

