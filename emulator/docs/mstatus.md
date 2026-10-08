| Bit / Bits | Name  |         Maske RV64 | Bedeutung                                          |
| ---------: | ----- | -----------------: | -------------------------------------------------- |
|          1 | SIE   | 0x0000000000000002 | Globaler Interrupt Enable für S-Mode               |
|          3 | MIE   | 0x0000000000000008 | Globaler Interrupt Enable für M-Mode               |
|          5 | SPIE  | 0x0000000000000020 | Vorheriger Wert von SIE vor einem Trap             |
|          6 | UBE   | 0x0000000000000040 | Endianness von U-Mode Datenzugriffen               |
|          7 | MPIE  | 0x0000000000000080 | Vorheriger Wert von MIE vor einem Trap             |
|          8 | SPP   | 0x0000000000000100 | Privilege Mode vor einem S-Mode-Trap               |
|       10:9 | VS    | 0x0000000000000600 | Status des Vector-Kontexts                         |
|      12:11 | MPP   | 0x0000000000001800 | Privilege Mode vor einem M-Mode-Trap               |
|      14:13 | FS    | 0x0000000000006000 | Floating-Point-Kontextstatus                       |
|      16:15 | XS    | 0x0000000000018000 | Status weiterer Extensions                         |
|         17 | MPRV  | 0x0000000000020000 | Loads/Stores mit Privilege aus MPP ausführen       |
|         18 | SUM   | 0x0000000000040000 | S-Mode darf auf U-Pages zugreifen                  |
|         19 | MXR   | 0x0000000000080000 | Executable Pages dürfen auch gelesen werden        |
|         20 | TVM   | 0x0000000000100000 | S-Mode VM-Verwaltung nach M-Mode trapen            |
|         21 | TW    | 0x0000000000200000 | WFI in niedrigeren Modi einschränken               |
|         22 | TSR   | 0x0000000000400000 | SRET aus S-Mode nach M-Mode trapen                 |
|         23 | SPELP | 0x0000000000800000 | Previous Expected Landing Pad für S-Mode (Zicfilp) |
|         24 | SDT   | 0x0000000001000000 | S-Mode Double-Trap-Erkennung (Ssdbltrp)            |
|      33:32 | UXL   | 0x0000000300000000 | XLEN des U-Modes                                   |
|      35:34 | SXL   | 0x0000000C00000000 | XLEN des S-Modes                                   |
|         36 | SBE   | 0x0000001000000000 | Endianness von S-Mode Datenzugriffen               |
|         37 | MBE   | 0x0000002000000000 | Endianness von M-Mode Datenzugriffen               |
|         38 | GVA   | 0x0000004000000000 | mtval enthält Guest Virtual Address                |
|         39 | MPV   | 0x0000008000000000 | Vorheriger Virtualization Mode                     |
|         41 | MPELP | 0x0000020000000000 | Previous Expected Landing Pad für M-Mode (Zicfilp) |
|         42 | MDT   | 0x0000040000000000 | M-Mode Double-Trap-Erkennung (Smdbltrp)            |
|         63 | SD    | 0x8000000000000000 | Mindestens einer von FS, VS, XS ist Dirty          |

|  Bit | Name | 0                       | 1                      |
| ---: | ---- | ----------------------- | ---------------------- |
|    1 | SIE  | S-Interrupts global aus | S-Interrupts global an |
|    3 | MIE  | M-Interrupts global aus | M-Interrupts global an |
|    5 | SPIE | vorheriges SIE=0        | vorheriges SIE=1       |
|    7 | MPIE | vorheriges MIE=0        | vorheriges MIE=1       |

