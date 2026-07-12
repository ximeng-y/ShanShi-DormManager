#include "windowtheme.h"

#include <QApplication>
#include <QEvent>
#include <QLibrary>
#include <QTimer>
#include <QWidget>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

windowtheme::windowtheme(QObject* parent)
	: QObject(parent)
{
}

void windowtheme::install(QApplication& application)
{
	application.installEventFilter(new windowtheme(&application));
}

bool windowtheme::eventFilter(QObject* watched, QEvent* event)
{
	auto* widget = qobject_cast<QWidget*>(watched);
	if (widget != nullptr && widget->isWindow() && event->type() == QEvent::Show) {
		apply_light_title_bar(widget);
		QTimer::singleShot(0, widget, [widget]() {
			apply_light_title_bar(widget);
		});
	}
	return QObject::eventFilter(watched, event);
}

void windowtheme::apply_light_title_bar(QWidget* widget)
{
#ifdef Q_OS_WIN
	if (widget == nullptr) {
		return;
	}
	using dwm_set_window_attribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
	QLibrary dwm_library(QStringLiteral("dwmapi"));
	const auto set_attribute = reinterpret_cast<dwm_set_window_attribute>(
		dwm_library.resolve("DwmSetWindowAttribute"));
	if (set_attribute == nullptr) {
		return;
	}
	const WId window_id = widget->effectiveWinId();
	if (window_id == 0) {
		return;
	}
	const HWND handle = reinterpret_cast<HWND>(window_id);
	const BOOL use_dark_mode = FALSE;
	constexpr DWORD immersive_dark_mode = 20;
	constexpr DWORD immersive_dark_mode_legacy = 19;
	if (FAILED(set_attribute(handle, immersive_dark_mode, &use_dark_mode, sizeof(use_dark_mode)))) {
		set_attribute(handle, immersive_dark_mode_legacy, &use_dark_mode, sizeof(use_dark_mode));
	}
	constexpr DWORD border_color_attribute = 34;
	constexpr DWORD caption_color_attribute = 35;
	constexpr DWORD text_color_attribute = 36;
	const COLORREF border_color = RGB(220, 227, 234);
	const COLORREF caption_color = RGB(255, 255, 255);
	const COLORREF text_color = RGB(36, 52, 71);
	set_attribute(handle, border_color_attribute, &border_color, sizeof(border_color));
	set_attribute(handle, caption_color_attribute, &caption_color, sizeof(caption_color));
	set_attribute(handle, text_color_attribute, &text_color, sizeof(text_color));
#else
	Q_UNUSED(widget);
#endif
}
