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
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

//#include <kdebug.h>

#include <qcheckbox.h>

#include <kurlrequester.h>

#include "prefs.h"
#include "prefs.moc"

Prefs::Prefs()
	: Prefs_base()
{
	if (!kcfg_ExternalCommand->isChecked())
		kcfg_ExternalCommandURL->setDisabled(true);

	connect(kcfg_ExternalCommand, SIGNAL(toggled(bool)), this, SLOT(disableExternalCommandURL(bool)));
}

void Prefs::disableExternalCommandURL(bool toggle)
{
	kcfg_ExternalCommandURL->setDisabled(!toggle);
}
// kate: tab-width 4; indent-width 4; replace-trailing-space-save on;

