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

#include <qstring.h>
#include <qdatetime.h>

/**
 * a class to hold a message
 */
class popupMessage
{
	public:
		popupMessage(const QString &paramSender,
					const QString &paramMachine,
					const QString &paramIP,
					const QDateTime &paramTime,
					const QString &paramText)
			: messageSender(paramSender),
			senderMachine(paramMachine),
			senderIP(paramIP),
			messageText(paramText),
			messageTime(paramTime),
			readStatus(false) {}

		//default destructor

		const QString &sender() { return messageSender; }
		const QString &machine() { return senderMachine; }
		const QString &ip() { return senderIP; }
		const QDateTime &time() { return messageTime; }
		const QString &text() { return messageText; }
		bool isRead() { return readStatus; }
		void setRead() {  readStatus = true; }

	private:
		QString messageSender, senderMachine, senderIP, messageText;
		QDateTime messageTime;
		bool readStatus;
};

// kate: tab-width 4; indent-width 4; replace-trailing-space-save on;
