#pragma once

#include <QMainWindow>
#include <QTimer>
#include <cstdint>
#include <string>
#include <vector>
#include <QEvent>
#include <QElapsedTimer>

#include "CONFIG.hpp"
#include "MACHINE.hpp"
#include "ui_MainForm.h"


struct DisassRow{
    std::string address;
    std::string word;
    std::string instr;
};

class MainForm : public QMainWindow, private Ui::MainForm
{
    Q_OBJECT
public:
    explicit MainForm(const Config& config, QWidget* parent = nullptr);

private:
    void runBatch();

    void updateState();
    void updateScreen();

    Machine m_machine;
    QTimer m_clock;
    QElapsedTimer elapsed;
    double m_stepDebt = 0.0;
    
    bool halted;
    CPU_STATE state;
 
    void halt();
    void cont();

    uint32_t bp;
    bool bp_set = false;

    bool m_skipBp = false;


    bool machine_on = false;

    std::vector<DisassRow> disassBuffer;

    DisassRow getDisassRow(uint32_t addr);
    void updateDisass();
    
    uint32_t targetRate = UINT32_MAX;
    
    uint32_t showAddress;
    uint32_t getAddressCobox();
    void updateMemoryView(bool goBtn);
    
    void updateCSRs();
    void updateFloppyDisks();
    void updateTickRate();

private slots:
    

    void btnStepClicked();
    void btnContClicked();
    void btnHaltClicked();
    void btnResetClicked();
    void btnSetClicked();
    void comboMemoryViewCurrentIndexChanged(int index);
    void btnGotoClicked();




protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
};


