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

#include <QLabel>
#include <QDateTime>

#include <KLocale>
#include <KGlobal>
#include <KTextEdit>

#include "klinpopupview.h"
#include "klinpopupview.moc"
#include "settings.h"

KLinPopupView::KLinPopupView(QWidget *parent) : QWidget(parent)
{
	setupUi(this);
}

KLinPopupView::~KLinPopupView()
{
}

void KLinPopupView::displayNewMessage(const QString &sender,const QDateTime &time, const QString &message, int paramTimeFormat)
{
	QString tmpLocaleTime;

	if (time.isValid()) {
		switch (paramTimeFormat) {
			case 0:
				tmpLocaleTime = KGlobal::locale()->formatDateTime(time);
				break;
			case 1:
				tmpLocaleTime = KGlobal::locale()->formatTime(time.time());
				break;
			case 2:
				tmpLocaleTime = KGlobal::locale()->formatDate(time.date(), KLocale::FancyLongDate);
				break;
			default:
				break;
		}
	} else {
		tmpLocaleTime = QString();
	}

	lblFromValue->setText(sender);
	lblTimeValue->setText(tmpLocaleTime);
	message_field->setText(message);
}

// kate: tab-width 4; indent-width 4; replace-trailing-space-save on;

