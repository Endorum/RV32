#include "MainForm.hpp"
#include "CPU.hpp"
#include "DECODE.hpp"
#include "DEFS.hpp"
#include "ERROR.hpp"
#include "devices/SCREEN.hpp"
#include "memory_map.h"
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <radcpp/LabeledEdit.hpp>
#include <radcpp/SdlCanvas.hpp>
#include <radcpp/StatusLed.hpp>
#include <stdexcept>
#include <string>
#include <QLineEdit>
#include <system_error>
#include <vector>

#include <SDL3/SDL.h>

// Steps per timer tick: big enough to run fast, small enough to keep the window responsive
// static constexpr int stepsPerTick = 2000;


char sanitize_char(const char c){
    if ( c >= '!' && c <= '~') return c;
    else return '.';
}

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
    connect(comboMemoryView, &QComboBox::currentIndexChanged, this, &MainForm::comboMemoryViewCurrentIndexChanged);
    connect(btnGoto, &QPushButton::clicked, this, &MainForm::btnGotoClicked);
    // connect(sdlCanvas, &rad::SdlCanvas::render, this, &MainForm::updateScreen);
    
    m_clock.setTimerType(Qt::PreciseTimer);
    m_clock.start(16); // 16 ms = 60 fps
    
    elapsed.start();

    tblMemory->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    tblMemory->setColumnWidth(0, 80);
    
    tblMemory->horizontalHeader()->setSectionResizeMode(17, QHeaderView::Stretch);
    
    for(int i=1;i<17;i++){
        tblMemory->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Fixed);
        tblMemory->setColumnWidth(i, 30);        
    }
    
    mmOUT->installEventFilter(this);
    
    // halt the cpu and enable stepping
    halted = true;
    updateState();
}

// every tick?
void MainForm::runBatch() {

    uint64_t elTimeNs = elapsed.nsecsElapsed();
    double elSeconds = elTimeNs / 1e9;
    elapsed.restart();
    
    // continues updating
    // stepping ksteps per tick
    // udpating form every tick
    
    
    m_stepDebt += targetRate * elSeconds;
    uint32_t stepsPerTick = std::floor(m_stepDebt);
    m_stepDebt -= stepsPerTick;
    
    // double maxRate = (double)(targetRate) / 30.0f;
    int maxRate = targetRate / 30;
    if(maxRate == 0) maxRate = 1;
    
    
    if(stepsPerTick > maxRate){
        stepsPerTick = maxRate;
        m_stepDebt = 0.0;
    }
        
    if(targetRate >= UINT32_MAX){
        stepsPerTick = UINT32_MAX;
        m_stepDebt = 0.0;
    }
    
    
    if(!halted)
    for (uint32_t i = 0; i < stepsPerTick; ++i){
        
        if (m_machine.halted()) {
            m_clock.stop();
            return;
        }
        

        if(bp_set && m_machine.get_pc() == bp && !m_skipBp){
            halt();
            m_stepDebt = 0.0;
            break;
        }
        
        if(i % 1000 == 0){
            if((elapsed.nsecsElapsed() / 1e6) >= 10)  // if the tick is already longer than 10 ms, give the 6 ms left to gui
                break;
        }
        
        m_machine.step();
        m_skipBp = false;
    }
    
    updateState();
    

    

    
}

DisassRow MainForm::getDisassRow(uint32_t addr) {
    
    DisassRow row;
    
    uint32_t address = addr;

    row.address = std::format("{:08X}", address);    
    try {
        
        uint32_t word = m_machine.load(addr, WORD);    
        Instruction instr = decode(word, address);


        row.word    = std::format("{:08X}", word);

        row.instr = disassemble(instr, false);
        
    } catch (const TrapException&) {
        row.word    = "????????";
        row.instr   = "?";
    }

    
    return row;
}


void MainForm::updateDisass(){
    // center = pc
    // +- 10 rows
    
    
        
    int count = tblDisass->rowCount();
    int pcPos = 5;
    
    for(int i=0;i<count;i++){
        DisassRow row = getDisassRow( state.pc + (i - pcPos)*4 );
        
        auto* addrItem  = new QTableWidgetItem( row.address.c_str() );
        auto* wordItem  = new QTableWidgetItem( row.word.c_str() );        
        auto* instrItem = new QTableWidgetItem( row.instr.c_str());
        
        tblDisass->setItem(i, 0, addrItem);        
        tblDisass->setItem(i, 1, wordItem);
        tblDisass->setItem(i, 2, instrItem);
        
        if(i == pcPos){
            addrItem->setBackground(QColor("#2a3a6a"));
            wordItem->setBackground(QColor("#2a3a6a"));
            instrItem->setBackground(QColor("#2a3a6a"));
        }
    }
    
}

uint32_t MainForm::getAddressCobox(){
    std::string label = comboMemoryView->currentText().toStdString();
    
    if(label == "ROM")  return ROM_START;
    if(label == "RAM")  return RAM_START;
    if(label == "HD")   return HD_START;
        
    if(label == "SP")   return state.regfile[sp];
    if(label == "PC")   return state.pc;

    
    return 0;
}

void MainForm::updateMemoryView(bool goBtn){
    
    uint32_t addr;
    
    if(!goBtn)
        addr = getAddressCobox();
    else
        addr = showAddress;
    
    
    int rowCount = tblMemory->rowCount();
    
    
    for(int i=0;i<rowCount;i++){
        uint32_t rowAddr = addr + i * 16;
        auto* addrItem = new QTableWidgetItem( QString( std::format("{:08X}", rowAddr).c_str() ) ); // starts at address
        
        tblMemory->setItem(i, 0, addrItem);
        
        std::string asciiRow = "";
            
        for(int j=0;j<16;j++){
            
            try {
                uint8_t byte = m_machine.load(rowAddr + j, BYTE);
                

                asciiRow += sanitize_char( (char)(byte) );

                auto* byteItem = new QTableWidgetItem( QString( std::format("{:02X}", byte).c_str() ) );
                tblMemory->setItem(i, j + 1, byteItem);
            } catch (const TrapException&) {
                tblMemory->setItem(i, j + 1, new QTableWidgetItem(QString("??")));
            }
            
        }
        
        auto* asciiItem = new QTableWidgetItem( QString( asciiRow.c_str() ) );
        tblMemory->setItem(i, 17, asciiItem);
        
        
        
    }
}

void MainForm::updateFloppyDisks() {
    
}


void MainForm::updateCSRs() {
    tblCSRs->setItem(0, 1, new QTableWidgetItem( QString( std::format("{:08X}", state.csr[(int)CSR_ADDR::mstatus]).c_str() ) ) );
    tblCSRs->setItem(1, 1, new QTableWidgetItem( QString( std::format("{:08X}", state.csr[(int)CSR_ADDR::mcause]).c_str() ) ) );
    tblCSRs->setItem(2, 1, new QTableWidgetItem( QString( std::format("{:08X}", state.csr[(int)CSR_ADDR::mepc]).c_str() ) ) );
    tblCSRs->setItem(3, 1, new QTableWidgetItem( QString( std::format("{:08X}", state.csr[(int)CSR_ADDR::mtval]).c_str() ) ) );
    tblCSRs->setItem(4, 1, new QTableWidgetItem( QString( std::format("{:08X}", state.csr[(int)CSR_ADDR::mtvec]).c_str() ) ) );
    tblCSRs->setItem(5, 1, new QTableWidgetItem( QString( std::format("{:08X}", state.csr[(int)CSR_ADDR::scause]).c_str() ) ) );
    tblCSRs->setItem(6, 1, new QTableWidgetItem( QString( std::format("{:08X}", state.csr[(int)CSR_ADDR::sepc]).c_str() ) ) );
    tblCSRs->setItem(7, 1, new QTableWidgetItem( QString( std::format("{:08X}", state.csr[(int)CSR_ADDR::stval]).c_str() ) ) );
    tblCSRs->setItem(8, 1, new QTableWidgetItem( QString( std::format("{:08X}", state.csr[(int)CSR_ADDR::stvec]).c_str() ) ) );
    tblCSRs->setItem(9, 1, new QTableWidgetItem( QString( std::format("{:08X}", state.csr[(int)CSR_ADDR::satp]).c_str() ) ) );
}


void MainForm::updateState() {
    state = m_machine.get_cpu_state();
    edCycle->setText(std::to_string(state.cycle).c_str());
    
    for (int i=0; i < 32; i++) {
        uint32_t value = state.regfile[i];
        tblRegisters->setItem(i, 1, new QTableWidgetItem(std::format("{:08X}",  value).c_str()));
        if(value == 0x00000000){
            tblRegisters->item(i, 1)->setForeground(QColor("#505050"));
        }else {
            tblRegisters->item(i, 1)->setData(Qt::ForegroundRole, QVariant());
        }
    }
    
    edPc->setText(QString( std::format("{:08X}", state.pc).c_str() ));
    
    switch(state.priv){
        default: break;
        case PRIV_LEVEL::USER:          ; break;
        case PRIV_LEVEL::SUPERVISOR:    ; break;
        case PRIV_LEVEL::RESERVED:      ; break; // error?
        case PRIV_LEVEL::MACHINE:       ; break;
    
    }
    
    ledUserMode->setOn(state.priv == PRIV_LEVEL::USER);
    ledSuperMode->setOn(state.priv == PRIV_LEVEL::SUPERVISOR);
    ledMachineMode->setOn(state.priv == PRIV_LEVEL::MACHINE);
    
    if(state.priv == PRIV_LEVEL::RESERVED){
        ledUserMode->setState(rad::StatusLed::State::Off);
        ledSuperMode->setState(rad::StatusLed::State::Off);
        ledMachineMode->setState(rad::StatusLed::State::Off);
    }
    
    const std::string out = m_machine.uart_output();
    if(!out.empty()){
        
        mmOUT->moveCursor(QTextCursor::End);
        QTextCursor cursor = mmOUT->textCursor();
        
        for(int i=0;i<out.size();i++){
            const char c = out[i];
            if(c == '\b'){
                cursor.deletePreviousChar();
                continue;
            }
            if(c == '\r'){
                continue;
            }
            
            cursor.insertText(QString::fromStdString(std::string(1, c)));
        }
        
        mmOUT->setTextCursor(cursor);
        // mmOUT->insertPlainText(QString::fromStdString(out));
        mmOUT->ensureCursorVisible();
    }
    
    if(halted){
        
    }
    
    updateDisass();
    updateMemoryView(false);
    
    updateFloppyDisks();
    
    updateCSRs();
    updateTickRate();
    
}

void MainForm::updateScreen() {
    
    // SDL_Renderer* renderer = sdlCanvas->renderer();
    // if(!renderer){
    //     // TODO: Error msg in errorText()
    //     std::cout << "Could not create renderer: " << sdlCanvas->errorText().toStdString() << std::endl;
    //     return;
    // }
    
    // if(SCREEN_HEIGHT != sdlCanvas->height() || SCREEN_WIDTH != sdlCanvas->width()){
    //     // something sometijhng error
    //     return;
    // }
    
    // const auto& screen = m_machine.screen_output();
    
    // for (int idx=0; idx<SCREEN_HEIGHT*SCREEN_WIDTH; idx++) {
    //     u8 px = screen[idx];

    //     u8 r = ((px & 0xC0) >> 6) * 0x55;
    //     u8 g = ((px & 0x30) >> 4) * 0x55;
    //     u8 b = ((px & 0x0C) >> 2) * 0x55;
    //     SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    
    //     float x = (idx / SCREEN_WIDTH);
    //     float y = (idx % SCREEN_WIDTH);
        
    //     SDL_RenderPoint(renderer, x, y);
    // }
}



void MainForm::btnStepClicked() {
    m_machine.step();
    updateState();
}


void MainForm::halt() {
    halted = true;
    updateDisass();
    
}

void MainForm::cont() {
    halted = false;    
}

void MainForm::btnContClicked()
{
    m_stepDebt = 0.0;
    m_skipBp = true;
    cont();
}

void MainForm::btnHaltClicked()
{
    halt();
}

void MainForm::btnResetClicked()
{
    if(machine_on == false){
        halted = false;
    }
    
    ledRun->setOn(true);
    
    m_machine.reset();
    mmOUT->setPlainText("");
    updateState();
}

void MainForm::btnSetClicked()
{
    bp = std::strtoul(edBreakpoint->text().toStdString().c_str(), nullptr, 16);
    bp_set = true;
    edBreakpoint->setText("");
    std::cout << "Set Breakpoint at " << bp << std::endl;
}




void MainForm::comboMemoryViewCurrentIndexChanged(int index) {
    updateMemoryView(false);
    edMemoryAddress->clear();
}

void MainForm::btnGotoClicked() {
    try {
        showAddress = std::stoi((edMemoryAddress->text()).toStdString(), nullptr, 16);
    } catch (const std::invalid_argument&) {
        showAddress = 0;
    }
    
    updateMemoryView(true);
}

bool MainForm::eventFilter(QObject* watched, QEvent* event) {

    if(watched == mmOUT && event->type() == QEvent::KeyPress){
        QKeyEvent* key = static_cast<QKeyEvent*>(event);

        
        if(key->key() == Qt::Key_Backspace){
            //
            m_machine.uart_input(std::string(1, (char)(0x7F)));
        }
        
        else{
            // "normal" keys
            m_machine.uart_input(key->text().toStdString());
        }
        
        return true;
    }
    
    return QMainWindow::eventFilter(watched, event);
}

void MainForm::updateTickRate() {
    if(rad1ps->isChecked())     {targetRate = 1; return;}
    if(rad10ps->isChecked())    {targetRate = 10; return;}
    if(rad100ps->isChecked())   {targetRate = 100; return;}
    if(rad1kps->isChecked())    {targetRate = 1000; return;}
    if(rad1mps->isChecked())    {targetRate = 1000000; return;}
    if(radmax->isChecked())     {targetRate = UINT32_MAX; return;}
    targetRate = UINT_MAX;
}
