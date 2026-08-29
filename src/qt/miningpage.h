// Copyright (c) 2026 dragonchain
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_MININGPAGE_H
#define BITCOIN_QT_MININGPAGE_H

#include <QWidget>
#include <QList>
#include <QElapsedTimer>
#include <atomic>

class ClientModel;
class QThread;
class QTimer;

namespace Ui {
    class MiningPage;
}

class MiningPage : public QWidget
{
    Q_OBJECT

public:
    explicit MiningPage(QWidget *parent = nullptr);
    ~MiningPage();

    void setClientModel(ClientModel *model);

private Q_SLOTS:
    void on_startMiningButton_clicked();
    void on_stopMiningButton_clicked();
    void on_configureButton_clicked();
    void on_threadsSlider_valueChanged(int value);
    void refreshStats();

private:
    Ui::MiningPage *ui;
    ClientModel *clientModel;

    // Mining state
    QThread *miningThread;
    QThread *networkStatsThread;
    QString miningAddress;
    std::atomic<bool> miningActive{false};
    std::atomic<uint64_t> totalHashes{0};
    std::atomic<int> blocksMined{0};
    std::atomic<double> lastDifficulty{0.0};
    int threadCount;
    QElapsedTimer sessionTimer;
    QTimer *statsTimer;

    // Internal methods
    void startMiningThreads();
    void stopMiningThreads();
    void doStartMining();
    void startNetworkStatsThread();
    void stopNetworkStatsThread();
    void addLogLine(const QString &line);
    double calculateLocalHashrate() const;
    void updateButtonStates();
};

#endif // BITCOIN_QT_MININGPAGE_H
