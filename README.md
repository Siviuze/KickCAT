# A simple C++ EtherCAT master stack.

Kick-start your slaves!

### Current state:
 - Can go to OP state
 - Can read and write PI
 - CoE: read and write SDO - blocking and async call
 - CoE: Emergency message
 - Bus diagnostic: can reset and get errors counters
 - hook to configure non compliant slaves
 - consecutives writes to reduce latency - up to 255 datagrams in flight
 - build for Linux and PikeOS

### TODO:
 - CoE: segmented transfer - partial implementation
 - CoE: diagnosis message - 0x10F3
 - Bus diagnostic: auto discover broken wire (on top of error counters)
 - Link: handle interface redundancy
 - More profiles: FoE, EoE, AoE, SoE
 - Distributed clock
 - AF_XDP Linux socket to improve performance
 - Addressing groups

## Operatings systems
## Linux
To improve latency on Linux, you have to
 - use Linux RT (PREMPT_RT patches),
 - set a real time scheduler for the program (i.e. with chrt)
 - disable NIC IRQ coalescing (with ethtool)
 - disable RT throttling
 - isolate ethercat task and network IRQ on a dedicated core
 - change network IRQ priority

## PikeOS
 - Tested on PikeOS 5.1 for native personnality (p4ext)
 - You have to provide the CMake cross-toolchain file which shall define PIKEOS variable (or adapt the main CMakelists.txt to your needs)
 - examples are provided with a working but non-optimal process/thread configuration (examples/PikeOS/p4ext_config.c): feel free to adapt it to your needs


## EtherCAT doc
https://infosys.beckhoff.com/english.php?content=../content/1033/tc3_io_intro/1257993099.html&id=3196541253205318339
https://www.ethercat.org/download/documents/EtherCAT_Device_Protocol_Poster.pdf

registers:
https://download.beckhoff.com/download/Document/io/ethercat-development-products/ethercat_esc_datasheet_sec2_registers_3i0.pdf

eeprom :
https://infosys.beckhoff.com/english.php?content=../content/1033/tc3_io_intro/1358008331.html&id=5054579963582410224

various:
https://sir.upc.edu/wikis/roblab/index.php/Development/Ethercat

diag:
https://www.automation.com/en-us/articles/2014-2/diagnostics-with-ethercat-part-4
https://infosys.beckhoff.com/english.php?content=../content/1033/ethercatsystem/1072509067.html&id=
https://knowledge.ni.com/KnowledgeArticleDetails?id=kA00Z000000kHwESAU
