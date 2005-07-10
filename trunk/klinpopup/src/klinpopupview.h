
#ifndef KLINPOPUPVIEW_H
#define KLINPOPUPVIEW_H

#include <qwidget.h>
#include <qdatetime.h>

#include <klocale.h>
#include <kglobal.h>

#include "klinpopupview_base.h"

/**
 * @short Main view
 * @author Gerd Fleischer <gerdfleischer@web.de>
 * @version 0.3.1
 */
class KLinPopupView : public KLinPopupview_base
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
