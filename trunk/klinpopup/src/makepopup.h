/***************************************************************************
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

#ifndef MAKEPOPUP_H
#define MAKEPOPUP_H

#define CLASSIC_VIEW 0
#define TREE_VIEW 1

#define ENC_LOCAL8BIT 0
#define ENC_UTF8 1
#define ENC_LATIN1 2
#define ENC_ASCII 3

#include <libsmbclient.h>

#include <q3hbox.h>
#include <q3vbox.h>
#include <QLayout>
#include <QThread>
#include <qmap.h>
#include <QStringList>
#include <q3listview.h>
//Added by qt3to4:
#include <Q3GridLayout>
#include <QEvent>
#include <QCloseEvent>

#include <kpushbutton.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <ktextedit.h>

class makePopup;

class readGroupsThread : public QThread
{
	public:
		readGroupsThread(makePopup *owner)
			: threadOwner(owner) {}

		// default destructor

	protected:
		void run();

	private:
		makePopup *threadOwner;
};

class readHostsThread : public QThread
{
	public:
		readHostsThread(makePopup *owner)
			: threadOwner(owner) {}

		// default destructor

	protected:
		void run();

	private:
		makePopup *threadOwner;
};

class makePopup : public QWidget
{
	Q_OBJECT

public:
	makePopup(QWidget *, const char *, const QString &, const QString &, int, int);
	~makePopup();

	void readGroupList();
	void readHostList();
	static void auth_smbc_get_data(const char *server,const char *share,
								   char *workgroup, int wgmaxlen,
								   char *username, int unmaxlen,
								   char *password, int pwmaxlen);

protected:
	void closeEvent(QCloseEvent *);
	bool eventFilter(QObject *, QEvent *);

private slots:
	void slotButtonSend();
	void finished();
	void slotGroupboxChanged(const QString &);
	void slotSendCmdExit(K3Process *);
	void slotTreeViewItemExpanded(Q3ListViewItem *);
	void slotTreeViewSelectionChanged();

private:
	void setupLayout();
	void initSmbCtx();
	void sendPopup();
	void queryFinished();

	Q3GridLayout* makePopupLayout;
	Q3ListView *groupTreeView;
	KComboBox *senderBox, *groupBox, *classicReceiverBox;
	KLineEdit *treeViewReceiverBox;
	KTextEdit *messageText;
	KPushButton *buttonSend, *buttonCancel;
	QString smbclientBin, messageReceiver, currentGroup;
	int newMsgEncoding, viewMode, sendRefCount, sendError;
	SMBCCTX *smbCtx;
	bool allProcessesStarted, justSending;
	QMap<QString, QString> errorHosts;
	QStringList allGroupHosts;
	readHostsThread readHosts;
	readGroupsThread readGroups;

protected slots:
    virtual void languageChange();
};

#endif // MAKE_POPUP_H

// kate: tab-width 4; indent-width 4; replace-trailing-space-save on;
