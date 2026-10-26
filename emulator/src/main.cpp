#include <QApplication>
#include <iostream>
#include <string>

#include "CONFIG.hpp"
#include "MACHINE.hpp"
#include "MainForm.hpp"
#include "radcpp_skin.hpp"

static void showUsage(const std::string& argv0)
{
    std::cout
        << "Usage: " << argv0 << " [options]\n"
        << "\n"
        << "Flags:\n"
        << "  -d                    enable debug output\n"
        << "  -l                    enable logging\n"
        << "  -s                    single-step execution\n"
        << "  -headless             no window, run in the console (UART on stdout/stdin)\n"
        << "\n"
        << "Files:\n"
        << "  -firmware=<path>      firmware image loaded into ROM\n"
        << "  -harddisk=<path>      harddisk image\n"
        << "  -removeable=<path>    removable media image\n"
        << "\n"
        << "Addresses / sizes (hex, e.g. 0x80000000):\n"
        << "  -b=<addr>             add breakpoint (repeatable)\n"
        << "  -reset_vector=<addr>  initial PC\n"
        << "  -tohost=<addr>        tohost address (test ROM exit)\n"
        << "  -ram_start=<addr>     -ram_size=<size>\n"
        << "  -rom_start=<addr>     -rom_size=<size>\n"
        << "  -hd_start=<addr>      -hd_size=<size>\n"
        << "\n"
        << "Example:\n"
        << "  " << argv0 << " -firmware=fw.bin -reset_vector=0x80000000 -b=0x80000010 -d\n";
}

// the console emulator from before the GUI: no window, UART output goes to stdout
static int runHeadless(int argc, char* argv[])
{
    Config config = parse_arguments(argc, argv);
    std::cout << "Configuration: \n" << config.str() << std::endl;

    Machine machine(config);
    machine.start();
    while (!machine.halted()) {
        machine.step();
        std::cout << machine.uart_output() << std::flush;   // the UART buffers since the GUI
    }
    return 0;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        showUsage(argv[0]);
        return 1;
    }
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-headless") return runHeadless(argc, argv);
    }

    QApplication app(argc, argv);   // takes Qt's own options out of argc/argv, the emulator's stay
    if (argc < 2) {
        showUsage(argv[0]);
        return 1;
    }

    // same arguments as the console emulator
    Config config = parse_arguments(argc, argv);
    std::cout << "Configuration: \n" << config.str() << std::endl;

    radcpp_skin::apply(app);   // Fusion, font and the skin from radcpp.radcpp
    MainForm form(config);
    form.show();
    return app.exec();
}
