#include "MainForm.hpp"
#include "CPU.hpp"
#include <cstddef>
#include <cstdlib>
#include <radcpp/LabeledEdit.hpp>
#include <string>
#include <QLineEdit>

// Steps per timer tick: big enough to run fast, small enough to keep the window responsive
static constexpr int kStepsPerTick = 20000;

MainForm::MainForm(const Config& config, QWidget* parent) : QMainWindow(parent), m_machine(config)
{
    setupUi(this);

    m_machine.start();
    std::cout << "\nSTARTING EXECUTION...\n" << std::endl;

    // Instead of the console's while(!halted()) loop: run in batches from the event loop
    connect(&m_clock, &QTimer::timeout, this, &MainForm::runBatch);
    connect(btnStep, &QPushButton::clicked, this, &MainForm::btnStepClicked);
    connect(btnCont, &QPushButton::clicked, this, &MainForm::btnContClicked);
    connect(btnHalt, &QPushButton::clicked, this, &MainForm::btnHaltClicked);
    connect(btnReset, &QPushButton::clicked, this, &MainForm::btnResetClicked);
    connect(btnSet, &QPushButton::clicked, this, &MainForm::btnSetClicked);
    connect(mmIN->lineEdit(), &QLineEdit::returnPressed, this, &MainForm::sendInput);

    m_clock.start(0);

    
    // halt the cpu and enable stepping
    halted = true;
    updateState();
}

void MainForm::runBatch()
{
    // continues updating
    // stepping ksteps per tick
    // udpating form every tick
    if(!halted)
    for (int i = 0; i < kStepsPerTick; ++i){
        
        if (m_machine.halted()) {
            m_clock.stop();
            return;
        }
        
        if(i % kStepsPerTick == 0){
            updateState();            
        }
        
        if(bp_set && state.pc >= bp){
            halt();
        }
        
        m_machine.step();
    }
}

void MainForm::updateState() {
    state = m_machine.get_cpu_state();
    edCycle->setText(std::to_string(state.cycle).c_str());
    
    for (int i=0; i < 32; i++) {
        tblRegisters->setItem(i, 1, new QTableWidgetItem(std::format("{:08X}", state.regfile[i]).c_str()));
    }
    
    edPc->setText(QString( std::format("{:08X}", state.pc).c_str() ));
    edCurrDiss->setText(QString( m_machine.curr_instr().c_str() ));
    
    const std::string out = m_machine.uart_output();
    if(!out.empty()){
        mmOUT->moveCursor(QTextCursor::End);
        mmOUT->insertPlainText(QString::fromStdString(out));
        mmOUT->ensureCursorVisible();
    }
}


void MainForm::btnStepClicked() {
    m_machine.step();
    updateState();
}


void MainForm::halt() {
    halted = true;
    
}

void MainForm::cont() {
    halted = false;    
}

void MainForm::btnContClicked()
{
    cont();
}

void MainForm::btnHaltClicked()
{
    halt();
}

void MainForm::btnResetClicked()
{
    m_machine.reset();
    mmIN->setText("");
    mmOUT->setPlainText("");
    updateState();
}

void MainForm::btnSetClicked()
{
    bp = std::strtod(edBreakpoint->text().toStdString().c_str(), nullptr);
    bp_set = true;
    lbBreakpoint->setText(edBreakpoint->text().toStdString().c_str());
    edBreakpoint->setText("");
}

void MainForm::sendInput() {
    m_machine.uart_input(mmIN->text().toStdString() + "\n");
    mmIN->setText("");
}


