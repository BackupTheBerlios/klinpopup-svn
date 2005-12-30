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

#include <kdebug.h>

#include <qcheckbox.h>
#include <qcombobox.h>

#include <kurlrequester.h>

#include "prefs.h"
#include "prefs.moc"

Prefs::Prefs()
	: Prefs_base()
{
	connect(kcfg_ToggleSignaling, SIGNAL(activated(int)), this, SLOT(toggleSoundURL(int)));
	connect(kcfg_ExternalCommand, SIGNAL(toggled(bool)), this, SLOT(toggleExternalCommandURL(bool)));
}

void Prefs::toggleURLs()
{
	if (kcfg_ToggleSignaling->currentItem() == 0 || kcfg_ToggleSignaling->currentItem() == 2)
		kcfg_SoundURL->setDisabled(true);
	else
		kcfg_SoundURL->setDisabled(false);

	if (!kcfg_ExternalCommand->isChecked())
		kcfg_ExternalCommandURL->setDisabled(true);
}

void Prefs::toggleSoundURL(int index)
{
	if (index == 0 || index == 2)
		kcfg_SoundURL->setDisabled(true);
	else
		kcfg_SoundURL->setDisabled(false);
}

void Prefs::toggleExternalCommandURL(bool toggle)
{
	kcfg_ExternalCommandURL->setDisabled(!toggle);
}

// kate: tab-width 4; indent-width 4; replace-trailing-space-save on;

