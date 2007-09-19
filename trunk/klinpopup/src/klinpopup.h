/***************************************************************************
*   Copyright (C) 2004, 2005 by Gerd Fleischer                                  *
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

#ifndef KLINPOPUP_H
#define KLINPOPUP_H

#define ID_STATUS_TEXT 10
#define ID_AUTO_REPLY 11
#define NEW_ICON 0
#define NORMAL_ICON 1
#define NEW_ICON_AR 2
#define NORMAL_ICON_AR 3
#define MS_NOTHING 0
#define MS_SOUND_TRAY 1
#define MS_ACTIVATE 2
#define MS_ALL 3

#include <QTimer>
#include <QString>
#include <QList>
#include <QThread>
#include <QEvent>
#include <QProcess>
#include <QFocusEvent>
#include <QLabel>
#include <QHideEvent>
#include <QFile>

#include <kuniqueapplication.h>
#include <kaction.h>
#include <ktoggleaction.h>
#include <kconfig.h>
//#include <kkeydialog.h>
//#include <KShortcutsDialog>
#include <kfileitem.h>
#include <kxmlguiwindow.h>

#include "popupmessage.h"
#include "klinpopupview.h"
#include "systemtray.h"

const QString POPUP_DIR = "/var/lib/klinpopup";

class KDirLister;

/**
 * @short Main window class
 * @author Gerd Fleischer <gerdfleischer@web.de>
 * @version 0.3.4
 */
class KLinPopup : public KXmlGuiWindow
{
	Q_OBJECT
public:
	KLinPopup();

public slots:
	void newPopup();

protected:
	void hideEvent(QHideEvent *) { hide(); }
	void changeEvent(QEvent *);

private slots:
	void startDirLister();
	void newMessages(const KFileItemList &);
	void listCompleted();
	void signalNewMessage(const QString &, const QString &, const QString &, const QString &, const QString &);
	void exit();
	void replyPopup();
	void firstPopup() { currentMessage = 1; showPopup(); popupHelper(); }
	void prevPopup() { --currentMessage; showPopup(); popupHelper(); }
	void nextPopup() { ++currentMessage; showPopup(); popupHelper(); }
	void lastPopup() { currentMessage = messageList.count(); showPopup(); popupHelper(); }
	void unreadPopup();
	void deletePopup();
	void optionsShowMenubar();
//	void optionsConfigureKeys() { KShortcutsDialog::configure(actionCollection()); }
	void optionsConfigureToolbars();
	void optionsPreferences();
	void newToolbarConfig();
	void changeStatusbar(const QString &);
	void settingsChanged();
	void updateStats();
	void statusAutoReply();

private:
	virtual bool queryClose();
	void setupAccel();
	void setupActions();
	void initSystemTray();
	void saveMessages();
	void readSavedMessages();
	bool checkPopupFileDirectory();
	void checkSmbclientBin();
	void checkMessageMap();
	void showPopup();
	QString createSenderText();
	void readConfig();
	void popupHelper();
	void runExternalCommand();
	void autoReply(const QString &);
	void setTrayPixmap();

	KLinPopupView *m_view;

	//config and actions
	KConfig *cfg;
	SystemTray *m_systemTray;
	KToggleAction *m_menubarAction;
	KToggleAction *autoReplyAction;
	KAction *newPopupAction;
	KAction *replyPopupAction;
	KAction *firstPopupAction;
	KAction *prevPopupAction;
	KAction *nextPopupAction;
	KAction *lastPopupAction;
	KAction *unreadPopupAction;
	KAction *deletePopupAction;

	int currentMessage, unreadMessages;
	QFile messagesFile;
	QString messageText, m_hostName, m_arOffPic, m_arOnPic;
	QString popupFileDirectory;
	QList<popupMessage *> messageList;
	QLabel *m_arLabel;
	KDirLister *dirLister;

	//option variables
	bool optRunDocked;
	bool optDisplaySender;
	bool optDisplayMachine;
	bool optDisplayIP;
	int optTimeFormat;
	int optNewMessageSignaling;
	QString optNewPopupSound;
	bool optExternalCommand;
	QString optExternalCommandURL;
	QString optArMsg;
	int optMakePopupView;
	QString optSmbclientBin;
	int optEncoding;
};

#endif // KLINPOPUP_H

// kate: tab-width 4; indent-width 4; replace-trailing-space-save on;

