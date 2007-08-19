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

#include "klinpopup.h"
#include <kuniqueapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kconfigbase.h>
#include <kconfig.h>

static const char description[] =
	I18N_NOOP("WinPopup-Client for KDE");


int main(int argc, char *argv[])
{
	KAboutData about("klinpopup", 0, ki18n("KLinPopup"),
					 "0.3.90", ki18n(description),
					 KAboutData::License_GPL, ki18n("(C) 2004-2006 Gerd Fleischer"));
	about.addAuthor(ki18n("Gerd Fleischer"), ki18n("maintainer"), "gerdfleischer@web.de");

	KCmdLineArgs::init(argc, argv, &about);
	KUniqueApplication::addCmdLineOptions();

     if (!KUniqueApplication::start())
 		return 0;

	KUniqueApplication app;

	bool runDocked;
	KConfig *c = new KConfig("klinpopuprc");
	KConfigGroup cg(c, "Preferences");
	runDocked = cg.readEntry("RunDocked", false);

	KLinPopup *widget = new KLinPopup;
	if (!runDocked)
		widget->show();

	return app.exec();
}

// kate: tab-width 4; indent-width 4; replace-trailing-space-save on;
