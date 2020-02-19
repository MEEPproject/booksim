# BST: BookSim

BookSim is a cycle-accurate interconnection network simulator originally
developed and introduced with the Principles and Practices of Interconnection
Networks book.
This BookSim version extends the funcionallity of BookSim 2.0 availiable
[here](https://github.com/booksim/booksim2) and is part of the BST Tools
published in ISPASS-2020 (bibtex reference below).
The main contributions of this version are:
1. Low latency single- and multi-hop bypass routers. The later are more commonly know as SMART.
2. An API to easily interconnect BookSim with other simulation environments such as gem5.
3. A set of scripts to launch large sets of simulations, generate plots, debug, etc.

```bibtex
@inproceedings{perez2020bst,
  title={BST: A BookSim-based toolset to simulate NoCs with single- and multi-hop bypass},
  author={P{\'e}rez, Iv{\'a}n and Vallejo, Enrique and Moret{\'o}, Miquel and Beivide, Ram{\'o}n},
  booktitle={2020 IEEE International Symposium on Performance Analysis of Systems and Software (ISPASS)},
  year={2020}
}
```

---
## Table of contents
1. [Quick start](#quick-start)
2. [Usage instructions](#usage-instructions)
3. [Directory structure](#directory-structure)
4. [BookSim API: Integration in other systems](#api)
5. [Notes](#notes)


---

### Quick start <a name='quick-start'></a>

In order to compile BookSim you need bison and flex. 
You can use the following commands to install them in a Debian Linux distro:
```bash
apt install bison
apt install flex
```

Compile it using make:
```bash
cd booksim2
make
```

And execute it with the following command.
```bash
./booksim ../examples/8x8_mesh_bypass_baseline.cfg
```

The final lines of the output should look similar to these:
```bash
class,load,traffic,min_plat,avg_plat,max_plat,his_plat,min_nlat,avg_nlat,max_nlat,his_nlat,min_flat,avg_flat,max_flat,his_flat,min_frag,avg_frag,max_frag,min_sent_packets,avg_sent_packets,max_sent_packets,min_accepted_packets,avg_accepted_packets,max_accepted_packets,min_sent_flits,avg_sent_flits,max_sent_flits,min_accepted_flits,avg_accepted_flits,max_accepted_flits,avg_sent_packet_size,avg_accepted_packet_size,hops,his_hops,smart_hpc,his_smart_hpc,smart_hops,his_smart_hops,bypassed_flits,sal_alloc_per_flit,sag_alloc_per_flit,switch_arbiter_input_conflict,buffer_busy,buffer_conflict,buffer_full,buffer_reserved,crossbar_conflict,output_blocked,la_buffer_busy,la_buffer_conflict,la_buffer_full,la_buffer_reserved,la_crossbar_conflict,la_sa_winners_killed,la_output_blocked
0,0.9,uniform,2455,4803.5,32866,"[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,150,806,2231,3986,5389,7036,8846,11711,13555,15139,17274,17305,17442,16811,13904,13746,13509,12221,10940,9961,9345,8131,6561,5849,5267,5337,5481,5278,5254,5124,4964,5084,5113,5348,5232,3999,3649,3293,3233,3326,3238,3108,2649,2528,2218,2064,1884,1718,1779,1488,1364,1135,953,867,777,782,737,732,688,522,504,442,481,438,467,400,428,358,331,453,414,426,365,494,633,768,849,992,1019,1025,1167,1234,1180,1067,1042,982,793,821,770,809,909,958,988,954,1099,1111,1062,1042,1218,1037,1093,1009,995,963,993,1042,1050,978,896,780,789,814,890,835,761,761,846,750,722,606,612,563,581,502,497,438,447,398,321,286,322,242,215,176,165,176,189,145,138,118,99,72,70,63,31,44,59,67,38,21,23,25,28,15,13,17,22,48,27,84,28,34,45,16,48,40,32,20,32,30,13,31,30,49,34,81,38,36,32,41,49,25,27,42,38,35,51,48,32,53,39,34,49,52,39,25,20,26,36,57,19524]",7,88.3212,3281,"[148713,181778,50764,17894,8753,4945,3138,2117,1519,1018,728,681,524,394,342,320,250,247,194,180,163,118,113,87,88,79,58,56,40,30,38,37,24,19,12,19,16,4,3,12,6,6,6,1,4,7,9,1,4,3,1,2,1,2,1,1,0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]",7,97.3664,3281,"[159461,195944,56162,20423,10477,6118,4074,2867,2189,1512,1166,1089,851,727,638,600,482,434,345,357,278,266,206,196,181,157,146,102,91,80,97,75,47,56,39,41,34,19,16,26,26,19,17,7,13,19,12,4,7,6,5,3,4,2,1,3,1,0,0,1,0,0,2,3,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]",0,0,0,0.02032,0.132916,0.23946,0.1297,0.132915,0.13598,0.02032,0.132916,0.23946,0.1297,0.132915,0.13598,1,1,6.01204,"[0,6565,24405,43106,55957,61916,61570,54367,44770,31834,20056,11614,5928,2566,778,140,0,0,0,0,0,0,0,0,0]",-nan,"[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]",0,"[425572,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]",0.686594,0,0,9.00084,0,0,0,0,4.95605,0,0,0,0,0,3.03721,0.579721,0
1,0.9,uniform,2443,4719.79,31912,"[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,51,190,360,530,701,904,1148,1432,1597,1867,1831,1918,1788,1531,1420,1404,1334,1206,1058,960,879,746,612,588,556,555,571,539,594,540,593,577,519,560,386,367,392,319,345,380,326,270,268,215,221,176,176,170,152,155,124,93,75,76,77,81,76,73,56,47,36,47,40,45,46,40,44,35,47,52,48,54,83,82,81,119,109,113,121,129,117,114,116,101,94,88,97,94,92,112,101,123,121,127,108,122,120,126,104,107,98,97,112,110,129,105,83,81,81,101,98,89,86,74,66,90,66,54,50,57,57,74,40,54,27,35,33,19,20,30,18,17,14,19,16,11,15,15,11,5,6,6,6,5,2,4,3,4,2,0,1,1,3,1,0,5,2,5,6,6,3,6,1,3,2,4,2,0,1,4,6,6,6,5,6,6,3,7,7,4,2,0,4,3,7,6,3,10,7,3,2,4,7,5,1,1,5,3,10,4,1993]",9,21.3618,70,"[44702,99,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]",5,16.944,66,"[246118,179,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]",0,0.8744,30,0.0023,0.0140172,0.02554,0.01284,0.0140138,0.0156,0.0115,0.0700822,0.1277,0.0642,0.0700694,0.078,4.99973,5.00004,5.99411,"[0,705,2585,4603,5849,6612,6496,5768,4534,3352,2118,1213,584,280,91,11,0,0,0,0,0,0,0,0,0]",-nan,"[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]",0,"[44801,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]",0.900617,0,0,0.239091,0,0,0,0,0.0439456,0,0,0,0,0,0.0249541,0.399133,0
Total run time 19.7239
```
This CSV output is required by the launch script hosted in `scripts/booksim_launcher/launcher.py`

---
### Directory structure <a name='directory-structure'></a>

The project is organized as follows:

- **booksim2** contains the source code of BookSim.
- **examples** has multiple files with the most relevant configurations.
- **scripts** contains scripts to ease the launching of experiments,
the generation of plots, etc
```bash
booksim-unican/
├── booksim2    # source code
├── examples    # simulation configuration files
└── scripts     # scripts to launch simulations, generate plots, debug, etc
    ├── booksim_launcher    # Simulation launcher
    ├── pipeline            # SMART pipeline profiling (debug)
    ├── plotter             # Result representation using Matplotlib
    ├── synfull             # Synfull execution tests
    └── util                # Miscelaneous (fix config files, execute sims...)
```

---
### BookSim API: Integration in other systems <a name='api'></a>

The API to use BookSim as a library in other simulations is composed of the
following structure and functions (see [booksim2/booksim_wrapper.hpp](booksim2/booksim_wrapper.hpp)):

1) BookSim construction. It requires a BookSim configuration file
(see examples).

```C++
BooksimWrapper(std::string const & config_file);
```

2) To inject packets in the network use ```GeneratePacket```. It requires the
source and destination nodes, the packet size, the packet class, and the
queueing latency of the packet in the internal simulator. If the function
returns -1 means that it wasn't possible to generate the packet, e.g. the
injection queue was full.

```C++
int GeneratePacket(int source, int dest, int size, int cl, long time);
```

3) To simulate the BookSim network use ```RunCycles```. It requires the number
of cycles to simulate, however I've only tested it with 1 cycle.

```C++
void RunCycles(unsigned int cycles);
```

4) To retire packets from the BookSim network use ```RetirePacket```. It
returns a ```RetiredPacket``` data type. If the pid is -1, there wasn't any
packet ready to be retired.  You have to iterate trough this function until it
returns -1 to dequeue the packets from all the ejection queues.

```C++
struct RetiredPacket {
    int pid; // packet ID
    int c; // packet class
    int ps; // packet size
    int plat; // packet latency
    int nlat; // network latency
    int hops; // number of hops done
    int shops; // number of SMART hops done
    int br; // bypassed routers
};

RetiredPacket RetirePacket();
```

5) To check if there are packets in the network you can use
```CheckInFlightPackets``` this is usefull to save some execution time not
calling BookSim when the network is empty.

```C++
bool CheckInFlightPackets();
```
5) ``UpdateSimTime`` updates the internal cycle counter of BookSim by adding
the number of cycles passed as argument. This is usefull to synchronize the
timestamps between the host simulator and BookSim, when the host is event
driven like in the case of gem5.

```C++
void UpdateSimTime(int cycles);
```

---
### Notes <a name='notes'></a>

> Parts of code to improve in the future:
>
>   - BufferState needs a refactorization:
>       - Very long files:
>           - split in files based on buffering policies?
>       - Simplify VCT and FBFCL: child class of Private Buf.
>           - Can they be used in bypass and SMART routers?
>   - Single-cycle routers refactorization:
>       - A lot of code replication among the different classes of bypass
>         routers
>   - Router rename to match the terminology used in the papers.
>   - Detect incompatible parameters in bypass routers.

> Documentation improvements:
>   - Scripts usage
