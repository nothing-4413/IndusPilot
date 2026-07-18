#pragma once

#include <QMainWindow>
#include <QString>
#include <QStringList>

class QListWidget;
class QStackedWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    QWidget* buildLoginPage();
    QWidget* buildDashboardPage();
    QWidget* buildTablePage(const QString& title, const QStringList& headers);
    QWidget* buildAiPage();

    QListWidget* navigation_{nullptr};
    QStackedWidget* pages_{nullptr};
};