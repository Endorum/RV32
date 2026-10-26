#pragma once

#include <QMainWindow>
#include <QTimer>
#include <cstdint>

#include "CONFIG.hpp"
#include "MACHINE.hpp"
#include "ui_MainForm.h"

class MainForm : public QMainWindow, private Ui::MainForm
{
    Q_OBJECT
public:
    explicit MainForm(const Config& config, QWidget* parent = nullptr);

private:
    void runBatch();

    void updateState();

    Machine m_machine;
    QTimer m_clock;
    
    bool halted;
    CPU_STATE state;

    void halt();
    void cont();

    uint32_t bp;
    bool bp_set = false;

private slots:
    

    void btnStepClicked();
    void btnContClicked();
    void btnHaltClicked();
    void btnResetClicked();
    void btnSetClicked();
    void sendInput();
};
