#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onZFolderBrowseClicked();
    void onSaveClicked();
    void onUpdateProgress(const int progressTotal);
    void onCancelClicked();

signals:
    void updateProgress(const int progressTotal);

private:
    Ui::MainWindow *ui;
    bool isSaveCanceled;
};

#endif // MAINWINDOW_H
