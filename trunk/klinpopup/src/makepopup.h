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

#include <QProcess>
#include <QMap>
#include <QStringList>
#include <QEvent>
#include <QCloseEvent>

#include <kpushbutton.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <ktextedit.h>

class QGridLayout;
class QTreeWidget;
class QTreeWidgetItem;
class makePopup;

class NamedProcess : public QProcess
{
	Q_OBJECT

	public:
		NamedProcess(const QString &name, QObject *parent = 0)
			: QProcess(parent), processName(name)
		{
			connect(this, SIGNAL(finished(int, QProcess::ExitStatus)),
					this, SLOT(processFinished(int, QProcess::ExitStatus)));
		}

	private:
		QString processName;

	private slots:
		void processFinished(int exitCode, QProcess::ExitStatus status)
		{
			emit namedFinished(exitCode, status, processName);
		}

	signals:
		void namedFinished(int, QProcess::ExitStatus, QString);
};

class makePopup : public QWidget
{
	Q_OBJECT

public:
	makePopup(QWidget *, const QString &, const QString &, int, int);
	~makePopup();

protected:
	void closeEvent(QCloseEvent *);
	bool eventFilter(QObject *, QEvent *);

private slots:
	void slotButtonSend();
	void finished();
	void slotGroupboxChanged();
	void slotSendCmdExit(int, QProcess::ExitStatus, QString);
	void startScan();
	void scanNetwork(int, QProcess::ExitStatus);
	void slotTreeViewSelectionChanged();

private:
	void setupLayout();
	QString getHostname();
	void sendPopup();
	void queryFinished();

	QGridLayout* makePopupLayout;
	QTreeWidget *groupTreeView;
	KComboBox *senderBox, *groupBox, *classicReceiverBox;
	KLineEdit *treeViewReceiverBox;
	KTextEdit *messageText;
	KPushButton *buttonSend, *buttonCancel;
	QString smbclientBin, messageReceiver, currentGroup, currentHost, ownGroup;
	int newMsgEncoding, viewMode, sendRefCount, sendError;
	bool allProcessesStarted, justSending, passedInitialHost;
	QStringList allGroupHosts, sendFailedHosts, todo, done;
	QMap<QString, QString> currentHosts, currentGroups;
	QMap<QString, QStringList> allGroups;
	QProcess *scanProcess;

protected slots:
    virtual void languageChange();
};

#endif // MAKE_POPUP_H

// kate: tab-width 4; indent-width 4; replace-trailing-space-save on;
