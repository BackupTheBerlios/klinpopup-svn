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

#ifndef KLINPOPUPVIEW_H
#define KLINPOPUPVIEW_H

#include <QWidget>
#include <QDateTime>

#include <klocale.h>
#include <kglobal.h>

#include "ui_klinpopupview_base.h"

/**
 * @short Main view
 * @author Gerd Fleischer <gerdfleischer@web.de>
 * @version 0.3.4
 */
class KLinPopupView : public Ui::KLinPopupview_base
{
	Q_OBJECT
public:
	KLinPopupView(QWidget *parent);
	virtual ~KLinPopupView();

	void displayNewMessage(const QString &, const QDateTime &, const QString &, int);

signals:
	void signalChangeStatusbar(const QString &text);
	void signalChangeCaption(const QString &text);
};

#endif // KLINPOPUPVIEW_H

// kate: tab-width 4; indent-width 4; replace-trailing-space-save on;
