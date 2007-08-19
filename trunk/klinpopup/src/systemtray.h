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
 *   51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.             *
 ***************************************************************************/

#ifndef SYSTEMTRAY_H
#define SYSTEMTRAY_H

#include <QPixmap>
#include <QIcon>
#include <ksystemtrayicon.h>

class QTimer;

class SystemTray : public KSystemTrayIcon
{
	Q_OBJECT

public:
	SystemTray(QWidget *parent = 0);
	virtual ~SystemTray();

	void changeTrayPixmap(int iconSwitch);

private:
	QWidget *parentWidget;
	QIcon m_trayPix, newTrayPixmap;
};

#endif // SYSTEMTRAY_H

// kate: tab-width 4; indent-width 4; replace-trailing-space-save on;
