#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

#pragma pack(push, 1)
struct RPKGHeader {
    std::uint32_t magic[4];
    std::uint32_t rom_version;
    std::uint32_t file_count;
    std::uint32_t header_size;
    std::uint32_t machine_uid;
};

struct RPKGEntry {
    std::uint64_t attrib;
    std::uint64_t last_modified;
    std::uint64_t full_path_length;
};
#pragma pack(pop)

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    isSaveCanceled(false)
{
    ui->setupUi(this);
    ui->packCancelButton->setDisabled(true);
    ui->packProgressWidget->setVisible(false);

    connect(ui->packZFolderBrowseBtn, &QPushButton::clicked, this, &MainWindow::onZFolderBrowseClicked);
    connect(ui->packSaveButton, &QPushButton::clicked, this, &MainWindow::onSaveClicked);
    connect(ui->packCancelButton, &QPushButton::clicked, this, &MainWindow::onCancelClicked);
    connect(this, &MainWindow::updateProgress, this, &MainWindow::onUpdateProgress);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onCancelClicked() {
    isSaveCanceled = true;
}

void MainWindow::onUpdateProgress(const int progressTotal) {
    ui->packProgressBar->setValue(progressTotal);
}

void MainWindow::onZFolderBrowseClicked() {
    QString path = QFileDialog::getExistingDirectory(this, tr("Choose the Z drive dump folder"));
    if (path.isEmpty()) {
        return;
    }

    ui->packZFolderBrowseLineEdit->setText(path);
}

void MainWindow::onSaveClicked() {
    QString text = ui->packZFolderBrowseLineEdit->text();
    if (text.isEmpty()) {
        QMessageBox::critical(this, tr("No Z drive dump folder specified"), tr("Please click <b>Browse</b> to select the path you want to pack."));
        return;
    }

    if (!fs::exists(text.toStdWString())) {
        QMessageBox::critical(this, tr("Z drive dump folder no longer exists!"), tr("You or other external factors have deleted the specified folder!"));
        return;
    }

    // Machine UID
    QString machineUidString = ui->packDeviceUIDLineEdit->text();
    if (machineUidString.isEmpty()) {
        QMessageBox::critical(this, tr("Machine UID is empty!"), tr("Please supply the machine UID from the phone that the Z drive is dumped from for this RPKG file!"));
        return;
    }

    bool isAllOk = false;
    std::uint32_t machineUid = machineUidString.toUInt(&isAllOk, 0);

    if (!isAllOk) {
        QMessageBox::critical(this, tr("Machine UID is invalid!"), tr("The entered machine UID string can not be converted to an integer!"));
        return;
    }

    QString savePath = QFileDialog::getSaveFileName(this, tr("Select the location to save the RPKG file"));
    if (savePath.isEmpty()) {
        return;
    }

    // All good now
    ui->packCancelButton->setEnabled(true);
    ui->packProgressBar->setValue(0);
    ui->packProgressWidget->setVisible(true);
    ui->packSaveButton->setEnabled(false);

    isSaveCanceled = false;

    QFuture<void> worker = QtConcurrent::run([this](QString savePath, QString sourceFolderPath, const std::uint32_t uid) {
        std::ofstream fout(fs::path(savePath.toStdWString()), std::ios_base::binary);
        RPKGHeader header;
        header.magic[0] = 'R';
        header.magic[1] = 'P';
        header.magic[2] = 'K';
        header.magic[3] = '2';
        header.rom_version = 0;
        header.file_count = 0;
        header.header_size = sizeof(RPKGHeader);
        header.machine_uid = uid;

        fs::path final_path = fs::absolute(sourceFolderPath.toStdWString());
        std::wstring final_path_str = final_path.wstring();

        std::size_t total_file = 0;

        fout.write(reinterpret_cast<const char*>(&header), header.header_size);
        for (auto &entry: fs::recursive_directory_iterator(final_path)) {
            if (isSaveCanceled) {
                return;
            }

            if (entry.is_regular_file()) {
                total_file++;
            }
        }

        for (auto &entry: fs::recursive_directory_iterator(final_path)) {
            if (isSaveCanceled) {
                return;
            }

            if (entry.is_regular_file()) {
                RPKGEntry rentry;

                // This includes read-only and archive attribute
                rentry.attrib = 1 | 0x20;
                rentry.last_modified = std::chrono::duration_cast<std::chrono::microseconds>(entry.last_write_time().
                    time_since_epoch()).count() + 62168256000 * 1000000;

                std::wstring entry_path_str = entry.path().wstring();
                entry_path_str.erase(entry_path_str.begin(), entry_path_str.begin() + final_path_str.size());

                for (auto i = 0; i < entry_path_str.size(); i++) {
                    if (entry_path_str[i] == '/') {
                        entry_path_str[i] = '\\';
                    }
                }

                if (entry_path_str.empty() || (entry_path_str[0] != '\\')) {
                    entry_path_str.insert(entry_path_str.begin(), '\\');
                }

                entry_path_str = L"Z:" + entry_path_str;
                rentry.full_path_length = entry_path_str.length();

                std::uint64_t rentry_fsize = entry.file_size();

                fout.write(reinterpret_cast<const char*>(&rentry), sizeof(RPKGEntry));
                for (std::size_t i = 0; i < entry_path_str.length(); i++) {
                    char16_t ccc = static_cast<char16_t>(entry_path_str[i]);
                    fout.write(reinterpret_cast<const char*>(&ccc), sizeof(char16_t));
                }
                fout.write(reinterpret_cast<const char*>(&rentry_fsize), sizeof(rentry_fsize));

                std::uint64_t left = rentry_fsize;
                static constexpr std::uint64_t CHUNK_SIZE = 0x10000;

                std::ifstream fin(entry.path(), std::ios_base::binary);
                std::vector<char> buffer;

                while (left > 0) {
                    if (isSaveCanceled) {
                        return;
                    }

                    const std::uint64_t take = std::min<std::uint64_t>(CHUNK_SIZE, left);
                    buffer.resize(take);

                    fin.read(buffer.data(), buffer.size());
                    fout.write(buffer.data(), buffer.size());

                    left -= take;
                }

                header.file_count++;

                if (total_file != 0) {
                    emit updateProgress(static_cast<int>(header.file_count * 100 / total_file));
                }
            }
        }

        fout.seekp(0, std::ios_base::beg);
        fout.write(reinterpret_cast<const char*>(&header), header.header_size);
    }, savePath, text, machineUid);

    while (!worker.isFinished()) {
        QCoreApplication::processEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    if (isSaveCanceled) {
        fs::remove(savePath.toStdWString());
    } else {
        QMessageBox::information(this, tr("Packing completed!"), tr("RPKG has successfully been packed"));
    }

    ui->packProgressWidget->setVisible(false);
    ui->packCancelButton->setEnabled(false);
    ui->packSaveButton->setEnabled(true);
}
