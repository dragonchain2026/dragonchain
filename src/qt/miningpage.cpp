// Copyright (c) 2026 dragonchain
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/miningpage.h>
#include <qt/forms/ui_miningpage.h>

#include <qt/clientmodel.h>
#include <qt/rpcconsole.h>

#include <util.h>

#include <QDateTime>
#include <QInputDialog>
#include <univalue.h>
#include <QMessageBox>
#include <QThread>
#include <QTimer>


MiningPage::MiningPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MiningPage),
    clientModel(nullptr),
    miningThread(nullptr),
    networkStatsThread(nullptr),
    threadCount(1),
    statsTimer(nullptr)
{
    ui->setupUi(this);

    miningAddress = ui->miningAddressEdit->text();

    statsTimer = new QTimer(this);
    connect(statsTimer, SIGNAL(timeout()), this, SLOT(refreshStats()));
}

MiningPage::~MiningPage()
{
    miningActive = false;
    stopNetworkStatsThread();
    if (miningThread) {
        disconnect(miningThread, &QThread::finished, miningThread, &QObject::deleteLater);
        miningThread->wait(10000);
        delete miningThread;
        miningThread = nullptr;
    }
    delete ui;
}

void MiningPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
}

void MiningPage::updateButtonStates()
{
    bool active = miningActive.load();
    ui->startMiningButton->setEnabled(!active);
    ui->stopMiningButton->setEnabled(active);
    ui->configureButton->setEnabled(!active);
    ui->threadsSlider->setEnabled(!active);
    ui->miningAddressEdit->setReadOnly(active);

    if (active) {
        ui->miningStatusLabel->setText(tr("Mining Active"));
        ui->miningStatusLabel->setStyleSheet("font-weight: bold; color: green;");
    } else {
        ui->miningStatusLabel->setText(tr("Stopped"));
        ui->miningStatusLabel->setStyleSheet("font-weight: bold; color: #888;");
    }
}

void MiningPage::on_startMiningButton_clicked()
{
    if (!clientModel) return;

    miningAddress = ui->miningAddressEdit->text().trimmed();
    if (miningAddress.isEmpty()) {
        // Get a new address asynchronously — avoid blocking UI thread
        ui->startMiningButton->setEnabled(false);
        ui->miningStatusLabel->setText(tr("Generating address..."));
        QThread::create([this]() {
            std::string result;
            bool ok = RPCConsole::RPCExecuteCommandLine(result, "getnewaddress");
            QMetaObject::invokeMethod(this, [this, ok, result]() {
                if (ok) {
                    QString addr = QString::fromStdString(result).trimmed();
                    if (addr.startsWith('"') && addr.endsWith('"')) {
                        addr = addr.mid(1, addr.length() - 2);
                    }
                    if (!addr.isEmpty()) {
                        miningAddress = addr;
                        ui->miningAddressEdit->setText(miningAddress);
                    }
                }
                if (miningAddress.isEmpty()) {
                    QMessageBox::warning(this, tr("Mining"),
                        tr("No mining address configured. Please configure one first."));
                    updateButtonStates();
                    return;
                }
                doStartMining();
            }, Qt::QueuedConnection);
        })->start();
        return;
    }

    doStartMining();
}

void MiningPage::doStartMining()
{
    // generatetoaddress is gated behind -gen=1 (see rpc/mining.cpp); enable it
    // for the duration of GUI mining so the mining thread's RPC calls succeed.
    gArgs.ForceSetArg("-gen", "1");
    threadCount = ui->threadsSlider->value();
    startMiningThreads();
    statsTimer->start(2000);
    addLogLine(tr("Mining started (%1 threads)").arg(threadCount));
    updateButtonStates();
}

void MiningPage::on_stopMiningButton_clicked()
{
    stopMiningThreads();
    statsTimer->stop();
    gArgs.ForceSetArg("-gen", "0");
    addLogLine(tr("Mining stopped"));
    updateButtonStates();
    ui->localHashrateLabel->setText(tr("0 H/s"));
}

void MiningPage::on_configureButton_clicked()
{
    bool ok;
    QString address = QInputDialog::getText(this, tr("Configure Mining Address"),
        tr("Mining address:"), QLineEdit::Normal,
        miningAddress, &ok);
    if (ok && !address.isEmpty()) {
        miningAddress = address;
        ui->miningAddressEdit->setText(miningAddress);
        addLogLine(tr("Address set to: %1").arg(miningAddress));
    }
}

void MiningPage::on_threadsSlider_valueChanged(int value)
{
    ui->threadsValueLabel->setText(QString::number(value));
    threadCount = value;
}

void MiningPage::startMiningThreads()
{
    miningActive = true;
    totalHashes = 0;
    blocksMined = 0;
    sessionTimer.start();

    startNetworkStatsThread();

    // Single mining thread — avoids orphan blocks from parallel generatetoaddress calls.
    // genproclimit controls internal parallelism of the yespower hasher.
    // max_tries=100000 reduces cs_main hold time to ~0.5s (was 5s with 1M),
    // dramatically cutting UI thread lock contention.
    const int MAX_TRIES = 100000;
    QString addr = miningAddress;
    miningThread = QThread::create([this, addr]() {
        while (miningActive.load()) {
            std::string cmd = "generatetoaddress 1 " + addr.toStdString() + " " + std::to_string(MAX_TRIES);
            std::string result;
            try {
                RPCConsole::RPCExecuteCommandLine(result, cmd);
            } catch (const UniValue& e) {
                QMetaObject::invokeMethod(this, [this, e]() {
                    std::string msg = "error";
                    try { msg = find_value(e, "message").get_str(); } catch (...) {}
                    addLogLine(tr("Mining error: %1").arg(QString::fromStdString(msg)));
                }, Qt::QueuedConnection);
                continue;
            } catch (const std::exception& e) {
                QMetaObject::invokeMethod(this, [this, e]() {
                    addLogLine(tr("Mining error: %1").arg(QString::fromStdString(e.what())));
                }, Qt::QueuedConnection);
                continue;
            }
            QString qresult = QString::fromStdString(result).trimmed();

            // An RPC error is returned as a JSON object {"code":...,"message":...}
            // (e.g. generatetoaddress blocked when -gen=1 is not set). No hashes were
            // performed and no block was found, so don't count either.
            if (qresult.contains("\"code\"") || qresult.contains("\"error\"")) {
                QMetaObject::invokeMethod(this, [this, qresult]() {
                    addLogLine(tr("Mining error: %1").arg(qresult.left(120)));
                }, Qt::QueuedConnection);
                continue;
            }

            // Success is a JSON array of block hashes (e.g. ["<hash>"]). An empty
            // array "[]" means the nonce search exhausted MAX_TRIES without a block.
            if (qresult.startsWith('[') && qresult != "[]") {
                // Block found: count difficulty * 2^32 effective hashes, matching
                // the unit used by getnetworkhashps. At low difficulty the block is
                // found long before MAX_TRIES, so counting MAX_TRIES here would
                // grossly overstate the local hashrate (the "local > network" bug).
                double diff = lastDifficulty.load();
                if (diff > 0.0) {
                    totalHashes += (uint64_t)(diff * 4294967296.0);
                } else {
                    totalHashes += MAX_TRIES; // fallback before first difficulty refresh
                }
                blocksMined++;
                QString blockHash;
                int start = qresult.indexOf('"');
                if (start >= 0) {
                    int end = qresult.indexOf('"', start + 1);
                    if (end > start) {
                        blockHash = qresult.mid(start + 1, end - start - 1);
                    }
                }
                if (blockHash.length() >= 16) {
                    QMetaObject::invokeMethod(this, [this, blockHash]() {
                        addLogLine(tr("Block found: %1...").arg(blockHash.left(16)));
                    }, Qt::QueuedConnection);
                } else {
                    QMetaObject::invokeMethod(this, [this]() {
                        addLogLine(tr("Block found!"));
                    }, Qt::QueuedConnection);
                }
            } else {
                // No block found — MAX_TRIES hashes were actually done.
                totalHashes += MAX_TRIES;
            }
        }
    });
    connect(miningThread, &QThread::finished, miningThread, &QObject::deleteLater);
    miningThread->start();
}

void MiningPage::stopMiningThreads()
{
    miningActive = false;
    stopNetworkStatsThread();
    if (miningThread) {
        // Disconnect deleteLater to prevent double-delete:
        //   the thread's finished signal triggers deleteLater,
        //   but we manually delete here. Without disconnect,
        //   both paths destroy the same QThread → crash.
        disconnect(miningThread, &QThread::finished, miningThread, &QObject::deleteLater);
        miningThread->wait(10000);
        delete miningThread;
        miningThread = nullptr;
    }
}

void MiningPage::refreshStats()
{
    // Local stats only — all fast, non-blocking calculations
    double localHash = calculateLocalHashrate();
    if (localHash >= 1000.0) {
        ui->localHashrateLabel->setText(tr("%1 KH/s").arg(localHash / 1000.0, 0, 'f', 1));
    } else {
        ui->localHashrateLabel->setText(tr("%1 H/s").arg(localHash, 0, 'f', 0));
    }

    ui->blocksMinedLabel->setText(QString::number(blocksMined.load()));

    qint64 elapsed = sessionTimer.elapsed() / 1000;
    int hours = elapsed / 3600;
    int mins = (elapsed % 3600) / 60;
    int secs = elapsed % 60;
    ui->uptimeLabel->setText(QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(mins, 2, 10, QChar('0'))
        .arg(secs, 2, 10, QChar('0')));

    // Network hashrate and difficulty are fetched asynchronously
    // by networkStatsThread — see startNetworkStatsThread().
}

void MiningPage::startNetworkStatsThread()
{
    // Background thread for RPC calls to avoid blocking the UI event loop.
    // RPCExecuteCommandLine is synchronous and can stall if the daemon is
    // busy (mining, syncing). Running it in a separate thread keeps the
    // GUI responsive; results are posted back via QueuedConnection.
    networkStatsThread = QThread::create([this]() {
        while (miningActive.load()) {
            // Network hashrate — 20-block window for responsiveness
            std::string nethashResult;
            if (RPCConsole::RPCExecuteCommandLine(nethashResult, "getnetworkhashps 20")) {
                double netHash = QString::fromStdString(nethashResult).trimmed().toDouble();
                if (netHash > 0) {
                    QMetaObject::invokeMethod(this, [this, netHash]() {
                        if (netHash >= 1e12) {
                            ui->networkHashrateLabel->setText(tr("%1 TH/s").arg(netHash / 1e12, 0, 'f', 2));
                        } else if (netHash >= 1e9) {
                            ui->networkHashrateLabel->setText(tr("%1 GH/s").arg(netHash / 1e9, 0, 'f', 2));
                        } else if (netHash >= 1e6) {
                            ui->networkHashrateLabel->setText(tr("%1 MH/s").arg(netHash / 1e6, 0, 'f', 2));
                        } else if (netHash >= 1e3) {
                            ui->networkHashrateLabel->setText(tr("%1 KH/s").arg(netHash / 1e3, 0, 'f', 2));
                        } else {
                            ui->networkHashrateLabel->setText(tr("%1 H/s").arg(netHash, 0, 'f', 0));
                        }
                    }, Qt::QueuedConnection);
                }
            }

            // Difficulty — also cache for local hashrate calculation
            std::string diffResult;
            if (RPCConsole::RPCExecuteCommandLine(diffResult, "getdifficulty")) {
                double diff = QString::fromStdString(diffResult).trimmed().toDouble();
                if (diff > 0) {
                    QMetaObject::invokeMethod(this, [this, diff]() {
                        lastDifficulty.store(diff);
                        ui->difficultyLabel->setText(QString::number(diff, 'f', 10));
                    }, Qt::QueuedConnection);
                }
            }

            QThread::sleep(2);
        }
    });
    networkStatsThread->start();
}

void MiningPage::stopNetworkStatsThread()
{
    if (networkStatsThread) {
        networkStatsThread->wait(5000);
        delete networkStatsThread;
        networkStatsThread = nullptr;
    }
}

double MiningPage::calculateLocalHashrate() const
{
    qint64 elapsed = sessionTimer.elapsed();
    if (elapsed <= 0) return 0.0;

    // Primary: difficulty-based estimate from actual blocks found.
    // Matches the unit used by getnetworkhashps (effective hashes/sec).
    int blocks = blocksMined.load();
    double diff = lastDifficulty.load();
    if (blocks > 0 && diff > 0.0) {
        return (double)blocks * diff * 4294967296.0 / ((double)elapsed / 1000.0);
    }

    // Fallback: counter-based estimate (less accurate, but works before first block)
    uint64_t hashes = totalHashes.load();
    return (double)hashes / ((double)elapsed / 1000.0);
}

void MiningPage::addLogLine(const QString &line)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    ui->miningLogWidget->appendPlainText(QString("[%1] %2").arg(timestamp, line));
}
