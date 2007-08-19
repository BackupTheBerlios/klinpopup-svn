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

#include <kapplication.h>
#include <kmenu.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kstandardaction.h>
#include <klocale.h>

#include "klinpopup.h"
#include "systemtray.h"
#include "systemtray.moc"

SystemTray::SystemTray(QWidget *parent) : KSystemTrayIcon(parent)
{
	m_trayPix = loadIcon("klinpopup");
	setIcon(m_trayPix);
	QMenu *popupMenu = contextMenu();
	popupMenu->addAction(loadIcon("mail_new"), i18n("&New Message"), parent, SLOT(newPopup()));
	popupMenu->addSeparator();
	popupMenu->addAction(loadIcon("configure"), i18n("&Configure"), parent, SLOT(optionsPreferences()));
}

SystemTray::~SystemTray()
{
}

void SystemTray::changeTrayPixmap(int iconSwitch)
{
	switch (iconSwitch) {
		case NEW_ICON:
			newTrayPixmap = loadIcon("new_popup");
			break;
		case NORMAL_ICON:
			newTrayPixmap = loadIcon("klinpopup");
			break;
		case NEW_ICON_AR:
			newTrayPixmap = loadIcon("new_popup_ar");
			break;
		case NORMAL_ICON_AR:
			newTrayPixmap = loadIcon("klinpopup_ar");
			break;
		default:
			newTrayPixmap = loadIcon("klinpopup");
			break;
	}

	setIcon(newTrayPixmap);
}

// kate: tab-width 4; indent-width 4; replace-trailing-space-save on;
