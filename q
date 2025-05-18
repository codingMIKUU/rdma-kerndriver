[    0.000000] microcode: microcode updated early to revision 0xd0003f5, date = 2024-08-02
[    0.000000] Linux version 5.4.0-86-generic (buildd@lgw01-amd64-041) (gcc version 9.3.0 (Ubuntu 9.3.0-17ubuntu1~20.04)) #97-Ubuntu SMP Fri Sep 17 19:19:40 UTC 2021 (Ubuntu 5.4.0-86.97-generic 5.4.133)
[    0.000000] Command line: BOOT_IMAGE=/boot/vmlinuz-5.4.0-86-generic root=UUID=7de8ba0b-9655-48cd-b568-ad06b26602a4 ro quiet splash default_hugepagesz=2MB hugepagesz=2M hugepages=8192 intel_iommu=on iommu=pt console=tty0 console=ttyS1,115200 quiet splash isolcpus=1,2 vt.handoff=7
[    0.000000] KERNEL supported cpus:
[    0.000000]   Intel GenuineIntel
[    0.000000]   AMD AuthenticAMD
[    0.000000]   Hygon HygonGenuine
[    0.000000]   Centaur CentaurHauls
[    0.000000]   zhaoxin   Shanghai  
[    0.000000] x86/fpu: Supporting XSAVE feature 0x001: 'x87 floating point registers'
[    0.000000] x86/fpu: Supporting XSAVE feature 0x002: 'SSE registers'
[    0.000000] x86/fpu: Supporting XSAVE feature 0x004: 'AVX registers'
[    0.000000] x86/fpu: Supporting XSAVE feature 0x020: 'AVX-512 opmask'
[    0.000000] x86/fpu: Supporting XSAVE feature 0x040: 'AVX-512 Hi256'
[    0.000000] x86/fpu: Supporting XSAVE feature 0x080: 'AVX-512 ZMM_Hi256'
[    0.000000] x86/fpu: Supporting XSAVE feature 0x200: 'Protection Keys User registers'
[    0.000000] x86/fpu: xstate_offset[2]:  576, xstate_sizes[2]:  256
[    0.000000] x86/fpu: xstate_offset[5]:  832, xstate_sizes[5]:   64
[    0.000000] x86/fpu: xstate_offset[6]:  896, xstate_sizes[6]:  512
[    0.000000] x86/fpu: xstate_offset[7]: 1408, xstate_sizes[7]: 1024
[    0.000000] x86/fpu: xstate_offset[9]: 2432, xstate_sizes[9]:    8
[    0.000000] x86/fpu: Enabled xstate features 0x2e7, context size is 2440 bytes, using 'compacted' format.
[    0.000000] BIOS-provided physical RAM map:
[    0.000000] BIOS-e820: [mem 0x0000000000000000-0x000000000008efff] usable
[    0.000000] BIOS-e820: [mem 0x000000000008f000-0x000000000008ffff] reserved
[    0.000000] BIOS-e820: [mem 0x0000000000090000-0x000000000009ffff] usable
[    0.000000] BIOS-e820: [mem 0x00000000000a0000-0x00000000000fffff] reserved
[    0.000000] BIOS-e820: [mem 0x0000000000100000-0x0000000042449fff] usable
[    0.000000] BIOS-e820: [mem 0x000000004244a000-0x000000004a452fff] reserved
[    0.000000] BIOS-e820: [mem 0x000000004a453000-0x000000004a6dbfff] usable
[    0.000000] BIOS-e820: [mem 0x000000004a6dc000-0x000000004b6dbfff] ACPI NVS
[    0.000000] BIOS-e820: [mem 0x000000004b6dc000-0x000000004c240fff] usable
[    0.000000] BIOS-e820: [mem 0x000000004c241000-0x000000004c346fff] reserved
[    0.000000] BIOS-e820: [mem 0x000000004c347000-0x000000004d801fff] usable
[    0.000000] BIOS-e820: [mem 0x000000004d802000-0x000000004d802fff] reserved
[    0.000000] BIOS-e820: [mem 0x000000004d803000-0x0000000051afffff] usable
[    0.000000] BIOS-e820: [mem 0x0000000051b00000-0x0000000051b00fff] reserved
[    0.000000] BIOS-e820: [mem 0x0000000051b01000-0x000000005eefdfff] usable
[    0.000000] BIOS-e820: [mem 0x000000005eefe000-0x000000006e3fefff] reserved
[    0.000000] BIOS-e820: [mem 0x000000006e3ff000-0x000000006f3fefff] ACPI NVS
[    0.000000] BIOS-e820: [mem 0x000000006f3ff000-0x000000006f7fefff] ACPI data
[    0.000000] BIOS-e820: [mem 0x000000006f7ff000-0x000000006f7fffff] usable
[    0.000000] BIOS-e820: [mem 0x000000006f800000-0x000000008fffffff] reserved
[    0.000000] BIOS-e820: [mem 0x00000000fe000000-0x00000000fe010fff] reserved
[    0.000000] BIOS-e820: [mem 0x0000000100000000-0x000000207fffffff] usable
[    0.000000] NX (Execute Disable) protection: active
[    0.000000] e820: update [mem 0x39259020-0x3926105f] usable ==> usable
[    0.000000] e820: update [mem 0x39259020-0x3926105f] usable ==> usable
[    0.000000] e820: update [mem 0x39226020-0x3925865f] usable ==> usable
[    0.000000] e820: update [mem 0x39226020-0x3925865f] usable ==> usable
[    0.000000] e820: update [mem 0x391f3020-0x3922565f] usable ==> usable
[    0.000000] e820: update [mem 0x391f3020-0x3922565f] usable ==> usable
[    0.000000] e820: update [mem 0x391c9020-0x391f265f] usable ==> usable
[    0.000000] e820: update [mem 0x391c9020-0x391f265f] usable ==> usable
[    0.000000] e820: update [mem 0x3919f020-0x391c865f] usable ==> usable
[    0.000000] e820: update [mem 0x3919f020-0x391c865f] usable ==> usable
[    0.000000] e820: update [mem 0x3916f020-0x3919e05f] usable ==> usable
[    0.000000] e820: update [mem 0x3916f020-0x3919e05f] usable ==> usable
[    0.000000] extended physical RAM map:
[    0.000000] reserve setup_data: [mem 0x0000000000000000-0x000000000008efff] usable
[    0.000000] reserve setup_data: [mem 0x000000000008f000-0x000000000008ffff] reserved
[    0.000000] reserve setup_data: [mem 0x0000000000090000-0x000000000009ffff] usable
[    0.000000] reserve setup_data: [mem 0x00000000000a0000-0x00000000000fffff] reserved
[    0.000000] reserve setup_data: [mem 0x0000000000100000-0x000000003916f01f] usable
[    0.000000] reserve setup_data: [mem 0x000000003916f020-0x000000003919e05f] usable
[    0.000000] reserve setup_data: [mem 0x000000003919e060-0x000000003919f01f] usable
[    0.000000] reserve setup_data: [mem 0x000000003919f020-0x00000000391c865f] usable
[    0.000000] reserve setup_data: [mem 0x00000000391c8660-0x00000000391c901f] usable
[    0.000000] reserve setup_data: [mem 0x00000000391c9020-0x00000000391f265f] usable
[    0.000000] reserve setup_data: [mem 0x00000000391f2660-0x00000000391f301f] usable
[    0.000000] reserve setup_data: [mem 0x00000000391f3020-0x000000003922565f] usable
[    0.000000] reserve setup_data: [mem 0x0000000039225660-0x000000003922601f] usable
[    0.000000] reserve setup_data: [mem 0x0000000039226020-0x000000003925865f] usable
[    0.000000] reserve setup_data: [mem 0x0000000039258660-0x000000003925901f] usable
[    0.000000] reserve setup_data: [mem 0x0000000039259020-0x000000003926105f] usable
[    0.000000] reserve setup_data: [mem 0x0000000039261060-0x0000000042449fff] usable
[    0.000000] reserve setup_data: [mem 0x000000004244a000-0x000000004a452fff] reserved
[    0.000000] reserve setup_data: [mem 0x000000004a453000-0x000000004a6dbfff] usable
[    0.000000] reserve setup_data: [mem 0x000000004a6dc000-0x000000004b6dbfff] ACPI NVS
[    0.000000] reserve setup_data: [mem 0x000000004b6dc000-0x000000004c240fff] usable
[    0.000000] reserve setup_data: [mem 0x000000004c241000-0x000000004c346fff] reserved
[    0.000000] reserve setup_data: [mem 0x000000004c347000-0x000000004d801fff] usable
[    0.000000] reserve setup_data: [mem 0x000000004d802000-0x000000004d802fff] reserved
[    0.000000] reserve setup_data: [mem 0x000000004d803000-0x0000000051afffff] usable
[    0.000000] reserve setup_data: [mem 0x0000000051b00000-0x0000000051b00fff] reserved
[    0.000000] reserve setup_data: [mem 0x0000000051b01000-0x000000005eefdfff] usable
[    0.000000] reserve setup_data: [mem 0x000000005eefe000-0x000000006e3fefff] reserved
[    0.000000] reserve setup_data: [mem 0x000000006e3ff000-0x000000006f3fefff] ACPI NVS
[    0.000000] reserve setup_data: [mem 0x000000006f3ff000-0x000000006f7fefff] ACPI data
[    0.000000] reserve setup_data: [mem 0x000000006f7ff000-0x000000006f7fffff] usable
[    0.000000] reserve setup_data: [mem 0x000000006f800000-0x000000008fffffff] reserved
[    0.000000] reserve setup_data: [mem 0x00000000fe000000-0x00000000fe010fff] reserved
[    0.000000] reserve setup_data: [mem 0x0000000100000000-0x000000207fffffff] usable
[    0.000000] efi: EFI v2.70 by Dell Inc.
[    0.000000] efi:  ACPI=0x6f7fe000  ACPI 2.0=0x6f7fe014  MEMATTR=0x5b3ec020  SMBIOS=0x69537000  SMBIOS 3.0=0x69535000 
[    0.000000] secureboot: Secure boot disabled
[    0.000000] SMBIOS 3.3.0 present.
[    0.000000] DMI: Dell Inc. PowerEdge R750xs/0PPTY2, BIOS 1.5.4 12/17/2021
[    0.000000] tsc: Detected 2400.000 MHz processor
[    0.000283] e820: update [mem 0x00000000-0x00000fff] usable ==> reserved
[    0.000329] e820: remove [mem 0x000a0000-0x000fffff] usable
[    0.000618] last_pfn = 0x2080000 max_arch_pfn = 0x400000000
[    0.000714] MTRR default type: uncachable
[    0.000759] MTRR fixed ranges enabled:
[    0.000806]   00000-9FFFF write-back
[    0.000808]   A0000-BFFFF uncachable
[    0.000854]   C0000-FFFFF write-protect
[    0.000856] MTRR variable ranges enabled:
[    0.000904]   0 base 000000000000 mask 3FC000000000 write-back
[    0.000951]   1 base 000080000000 mask 3FFF80000000 uncachable
[    0.000954]   2 base 00007F000000 mask 3FFFFF000000 uncachable
[    0.000999]   3 disabled
[    0.001001]   4 disabled
[    0.001002]   5 disabled
[    0.001047]   6 disabled
[    0.001049]   7 disabled
[    0.001050]   8 disabled
[    0.001095]   9 disabled
[    0.013500] x86/PAT: Configuration [0-7]: WB  WC  UC- UC  WB  WP  UC- WT  
[    0.020637] total RAM covered: 260080M
[    0.028324] Found optimal setting for mtrr clean up
[    0.028370]  gran_size: 64K 	chunk_size: 32M 	num_reg: 8  	lose cover RAM: 0G
[    0.038682] e820: update [mem 0x7f000000-0xffffffff] usable ==> reserved
[    0.038730] x2apic: enabled by BIOS, switching to x2apic ops
[    0.038777] last_pfn = 0x6f800 max_arch_pfn = 0x400000000
[    0.408706] check: Scanning 1 areas for low memory corruption
[    0.408896] Using GB pages for direct mapping
[    0.414955] secureboot: Secure boot disabled
[    0.414958] RAMDISK: [mem 0x39262000-0x41705fff]
[    0.414969] ACPI: Early table checksum verification disabled
[    0.414975] ACPI: RSDP 0x000000006F7FE014 000024 (v02 DELL  )
[    0.414983] ACPI: XSDT 0x000000006F40A188 0000E4 (v01 DELL   PE_SC3   00000000 DELL 01000013)
[    0.414995] ACPI: FACP 0x000000006F7F6000 000114 (v06 DELL   PE_SC3   00000000 DELL 00000001)
[    0.415008] ACPI: DSDT 0x000000006F770000 07FAD3 (v02 DELL   PE_SC3   00000003 DELL 00000001)
[    0.415015] ACPI: FACS 0x000000006F383000 000040
[    0.415021] ACPI: SSDT 0x000000006F7FB000 001571 (v02 INTEL  RAS_ACPI 00000001 INTL 20210331)
[    0.415028] ACPI: SSDT 0x000000006F7FA000 000745 (v02 INTEL  ADDRXLAT 00000001 INTL 20210331)
[    0.415035] ACPI: EINJ 0x000000006F7F9000 000150 (v01 DELL   PE_SC3   00000001 INTL 00000001)
[    0.415042] ACPI: BERT 0x000000006F7F8000 000030 (v01 DELL   PE_SC3   00000001 INTL 00000001)
[    0.415049] ACPI: ERST 0x000000006F7F7000 000230 (v01 DELL   PE_SC3   00000001 INTL 00000001)
[    0.415055] ACPI: HMAT 0x000000006F7F5000 000120 (v01 DELL   PE_SC3   00000001 DELL 00000001)
[    0.415061] ACPI: HPET 0x000000006F7F4000 000038 (v01 DELL   PE_SC3   00000001 DELL 00000001)
[    0.415066] ACPI: MCFG 0x000000006F7F3000 00003C (v01 DELL   PE_SC3   00000001 DELL 00000001)
[    0.415071] ACPI: MIGT 0x000000006F7F2000 000040 (v01 DELL   PE_SC3   00000000 DELL 00000001)
[    0.415076] ACPI: MSCT 0x000000006F7F1000 000090 (v01 DELL   PE_SC3   00000001 DELL 00000001)
[    0.415081] ACPI: WSMT 0x000000006F7F0000 000028 (v01 DELL   PE_SC3   00000000 DELL 00000001)
[    0.415086] ACPI: APIC 0x000000006F76F000 00025E (v04 DELL   PE_SC3   00000000 DELL 00000001)
[    0.415091] ACPI: SLIT 0x000000006F76E000 00002D (v01 DELL   PE_SC3   00000001 DELL 01000013)
[    0.415096] ACPI: SRAT 0x000000006F767000 006430 (v03 DELL   PE_SC3   00000002 DELL 01000013)
[    0.415101] ACPI: OEM4 0x000000006F5DF000 187A61 (v02 INTEL  CPU  CST 00003000 INTL 20210331)
[    0.415107] ACPI: OEM1 0x000000006F4CB000 113489 (v02 INTEL  CPU EIST 00003000 INTL 20210331)
[    0.415112] ACPI: OEM2 0x000000006F484000 046031 (v02 INTEL  CPU  HWP 00003000 INTL 20210331)
[    0.415117] ACPI: SSDT 0x000000006F40D000 0764A5 (v02 INTEL  SSDT  PM 00004000 INTL 20210331)
[    0.415122] ACPI: SSDT 0x000000006F40C000 000AA3 (v02 DELL   PE_SC3   00000000 DELL 00000001)
[    0.415128] ACPI: HEST 0x000000006F40B000 00017C (v01 DELL   PE_SC3   00000001 INTL 00000001)
[    0.415133] ACPI: SSDT 0x000000006F402000 007299 (v02 INTEL  SpsNm    00000002 INTL 20210331)
[    0.415138] ACPI: SSDT 0x000000006F7FD000 0003BD (v02 DELL   PE_SC3   00000002 DELL 00000001)
[    0.415143] ACPI: DMAR 0x000000006F401000 0000FA (v01 DELL   PE_SC3   00000001 DELL 00000001)
[    0.415148] ACPI: Reserving FACP table memory at [mem 0x6f7f6000-0x6f7f6113]
[    0.415149] ACPI: Reserving DSDT table memory at [mem 0x6f770000-0x6f7efad2]
[    0.415151] ACPI: Reserving FACS table memory at [mem 0x6f383000-0x6f38303f]
[    0.415152] ACPI: Reserving SSDT table memory at [mem 0x6f7fb000-0x6f7fc570]
[    0.415153] ACPI: Reserving SSDT table memory at [mem 0x6f7fa000-0x6f7fa744]
[    0.415154] ACPI: Reserving EINJ table memory at [mem 0x6f7f9000-0x6f7f914f]
[    0.415156] ACPI: Reserving BERT table memory at [mem 0x6f7f8000-0x6f7f802f]
[    0.415157] ACPI: Reserving ERST table memory at [mem 0x6f7f7000-0x6f7f722f]
[    0.415158] ACPI: Reserving HMAT table memory at [mem 0x6f7f5000-0x6f7f511f]
[    0.415159] ACPI: Reserving HPET table memory at [mem 0x6f7f4000-0x6f7f4037]
[    0.415161] ACPI: Reserving MCFG table memory at [mem 0x6f7f3000-0x6f7f303b]
[    0.415162] ACPI: Reserving MIGT table memory at [mem 0x6f7f2000-0x6f7f203f]
[    0.415163] ACPI: Reserving MSCT table memory at [mem 0x6f7f1000-0x6f7f108f]
[    0.415165] ACPI: Reserving WSMT table memory at [mem 0x6f7f0000-0x6f7f0027]
[    0.415166] ACPI: Reserving APIC table memory at [mem 0x6f76f000-0x6f76f25d]
[    0.415167] ACPI: Reserving SLIT table memory at [mem 0x6f76e000-0x6f76e02c]
[    0.415169] ACPI: Reserving SRAT table memory at [mem 0x6f767000-0x6f76d42f]
[    0.415170] ACPI: Reserving OEM4 table memory at [mem 0x6f5df000-0x6f766a60]
[    0.415171] ACPI: Reserving OEM1 table memory at [mem 0x6f4cb000-0x6f5de488]
[    0.415172] ACPI: Reserving OEM2 table memory at [mem 0x6f484000-0x6f4ca030]
[    0.415174] ACPI: Reserving SSDT table memory at [mem 0x6f40d000-0x6f4834a4]
[    0.415175] ACPI: Reserving SSDT table memory at [mem 0x6f40c000-0x6f40caa2]
[    0.415176] ACPI: Reserving HEST table memory at [mem 0x6f40b000-0x6f40b17b]
[    0.415178] ACPI: Reserving SSDT table memory at [mem 0x6f402000-0x6f409298]
[    0.415179] ACPI: Reserving SSDT table memory at [mem 0x6f7fd000-0x6f7fd3bc]
[    0.415181] ACPI: Reserving DMAR table memory at [mem 0x6f401000-0x6f4010f9]
[    0.415197] ACPI: Local APIC address 0xfee00000
[    0.415201] Setting APIC routing to cluster x2apic.
[    0.415296] SRAT: PXM 0 -> APIC 0x0000 -> Node 0
[    0.415298] SRAT: PXM 0 -> APIC 0x0001 -> Node 0
[    0.415298] SRAT: PXM 0 -> APIC 0x0002 -> Node 0
[    0.415299] SRAT: PXM 0 -> APIC 0x0003 -> Node 0
[    0.415300] SRAT: PXM 0 -> APIC 0x0004 -> Node 0
[    0.415301] SRAT: PXM 0 -> APIC 0x0005 -> Node 0
[    0.415302] SRAT: PXM 0 -> APIC 0x0006 -> Node 0
[    0.415303] SRAT: PXM 0 -> APIC 0x0007 -> Node 0
[    0.415303] SRAT: PXM 0 -> APIC 0x0008 -> Node 0
[    0.415304] SRAT: PXM 0 -> APIC 0x0009 -> Node 0
[    0.415305] SRAT: PXM 0 -> APIC 0x000a -> Node 0
[    0.415306] SRAT: PXM 0 -> APIC 0x000b -> Node 0
[    0.415307] SRAT: PXM 0 -> APIC 0x000c -> Node 0
[    0.415308] SRAT: PXM 0 -> APIC 0x000d -> Node 0
[    0.415308] SRAT: PXM 0 -> APIC 0x000e -> Node 0
[    0.415309] SRAT: PXM 0 -> APIC 0x000f -> Node 0
[    0.415310] SRAT: PXM 0 -> APIC 0x0010 -> Node 0
[    0.415311] SRAT: PXM 0 -> APIC 0x0011 -> Node 0
[    0.415312] SRAT: PXM 0 -> APIC 0x0012 -> Node 0
[    0.415313] SRAT: PXM 0 -> APIC 0x0013 -> Node 0
[    0.415314] SRAT: PXM 0 -> APIC 0x0014 -> Node 0
[    0.415315] SRAT: PXM 0 -> APIC 0x0015 -> Node 0
[    0.415315] SRAT: PXM 0 -> APIC 0x0016 -> Node 0
[    0.415316] SRAT: PXM 0 -> APIC 0x0017 -> Node 0
[    0.415317] SRAT: PXM 0 -> APIC 0x0018 -> Node 0
[    0.415318] SRAT: PXM 0 -> APIC 0x0019 -> Node 0
[    0.415319] SRAT: PXM 0 -> APIC 0x001a -> Node 0
[    0.415320] SRAT: PXM 0 -> APIC 0x001b -> Node 0
[    0.415320] SRAT: PXM 0 -> APIC 0x001c -> Node 0
[    0.415321] SRAT: PXM 0 -> APIC 0x001d -> Node 0
[    0.415322] SRAT: PXM 0 -> APIC 0x001e -> Node 0
[    0.415323] SRAT: PXM 0 -> APIC 0x001f -> Node 0
[    0.415351] ACPI: SRAT: Node 0 PXM 0 [mem 0x00000000-0x7fffffff]
[    0.415353] ACPI: SRAT: Node 0 PXM 0 [mem 0x100000000-0x207fffffff]
[    0.415370] NUMA: Initialized distance table, cnt=1
[    0.415374] NUMA: Node 0 [mem 0x00000000-0x7fffffff] + [mem 0x100000000-0x207fffffff] -> [mem 0x00000000-0x207fffffff]
[    0.415388] NODE_DATA(0) allocated [mem 0x207ffd2000-0x207fffcfff]
[    0.415822] Zone ranges:
[    0.415823]   DMA      [mem 0x0000000000001000-0x0000000000ffffff]
[    0.415825]   DMA32    [mem 0x0000000001000000-0x00000000ffffffff]
[    0.415827]   Normal   [mem 0x0000000100000000-0x000000207fffffff]
[    0.415828]   Device   empty
[    0.415830] Movable zone start for each node
[    0.415836] Early memory node ranges
[    0.415837]   node   0: [mem 0x0000000000001000-0x000000000008efff]
[    0.415839]   node   0: [mem 0x0000000000090000-0x000000000009ffff]
[    0.415840]   node   0: [mem 0x0000000000100000-0x0000000042449fff]
[    0.415841]   node   0: [mem 0x000000004a453000-0x000000004a6dbfff]
[    0.415842]   node   0: [mem 0x000000004b6dc000-0x000000004c240fff]
[    0.415843]   node   0: [mem 0x000000004c347000-0x000000004d801fff]
[    0.415844]   node   0: [mem 0x000000004d803000-0x0000000051afffff]
[    0.415845]   node   0: [mem 0x0000000051b01000-0x000000005eefdfff]
[    0.415846]   node   0: [mem 0x000000006f7ff000-0x000000006f7fffff]
[    0.415847]   node   0: [mem 0x0000000100000000-0x000000207fffffff]
[    0.416609] Zeroed struct page in unavailable ranges: 74356 pages
[    0.416611] Initmem setup node 0 [mem 0x0000000000001000-0x000000207fffffff]
[    0.416614] On node 0 totalpages: 33381772
[    0.416615]   DMA zone: 64 pages used for memmap
[    0.416616]   DMA zone: 1102 pages reserved
[    0.416617]   DMA zone: 3998 pages, LIFO batch:0
[    0.416678]   DMA32 zone: 5432 pages used for memmap
[    0.416679]   DMA32 zone: 347630 pages, LIFO batch:63
[    0.423558]   Normal zone: 516096 pages used for memmap
[    0.423558]   Normal zone: 33030144 pages, LIFO batch:63
[    0.679128] ACPI: PM-Timer IO Port: 0x508
[    0.679129] ACPI: Local APIC address 0xfee00000
[    0.679138] ACPI: X2APIC_NMI (uid[0xffffffff] high edge lint[0x1])
[    0.679140] ACPI: LAPIC_NMI (acpi_id[0xff] high edge lint[0x1])
[    0.679158] IOAPIC[0]: apic_id 8, version 32, address 0xfec00000, GSI 0-119
[    0.679160] ACPI: INT_SRC_OVR (bus 0 bus_irq 0 global_irq 2 dfl dfl)
[    0.679161] ACPI: INT_SRC_OVR (bus 0 bus_irq 9 global_irq 9 high level)
[    0.679162] ACPI: IRQ0 used by override.
[    0.679163] ACPI: IRQ9 used by override.
[    0.679165] Using ACPI (MADT) for SMP configuration information
[    0.679166] ACPI: HPET id: 0x8086a701 base: 0xfed00000
[    0.679170] TSC deadline timer available
[    0.679171] smpboot: Allowing 32 CPUs, 0 hotplug CPUs
[    0.679191] PM: Registered nosave memory: [mem 0x00000000-0x00000fff]
[    0.679192] PM: Registered nosave memory: [mem 0x0008f000-0x0008ffff]
[    0.679193] PM: Registered nosave memory: [mem 0x000a0000-0x000fffff]
[    0.679195] PM: Registered nosave memory: [mem 0x3916f000-0x3916ffff]
[    0.679196] PM: Registered nosave memory: [mem 0x3919e000-0x3919efff]
[    0.679196] PM: Registered nosave memory: [mem 0x3919f000-0x3919ffff]
[    0.679198] PM: Registered nosave memory: [mem 0x391c8000-0x391c8fff]
[    0.679198] PM: Registered nosave memory: [mem 0x391c9000-0x391c9fff]
[    0.679199] PM: Registered nosave memory: [mem 0x391f2000-0x391f2fff]
[    0.679199] PM: Registered nosave memory: [mem 0x391f3000-0x391f3fff]
[    0.679201] PM: Registered nosave memory: [mem 0x39225000-0x39225fff]
[    0.679201] PM: Registered nosave memory: [mem 0x39226000-0x39226fff]
[    0.679202] PM: Registered nosave memory: [mem 0x39258000-0x39258fff]
[    0.679203] PM: Registered nosave memory: [mem 0x39259000-0x39259fff]
[    0.679204] PM: Registered nosave memory: [mem 0x39261000-0x39261fff]
[    0.679205] PM: Registered nosave memory: [mem 0x4244a000-0x4a452fff]
[    0.679207] PM: Registered nosave memory: [mem 0x4a6dc000-0x4b6dbfff]
[    0.679208] PM: Registered nosave memory: [mem 0x4c241000-0x4c346fff]
[    0.679209] PM: Registered nosave memory: [mem 0x4d802000-0x4d802fff]
[    0.679210] PM: Registered nosave memory: [mem 0x51b00000-0x51b00fff]
[    0.679212] PM: Registered nosave memory: [mem 0x5eefe000-0x6e3fefff]
[    0.679212] PM: Registered nosave memory: [mem 0x6e3ff000-0x6f3fefff]
[    0.679212] PM: Registered nosave memory: [mem 0x6f3ff000-0x6f7fefff]
[    0.679214] PM: Registered nosave memory: [mem 0x6f800000-0x8fffffff]
[    0.679214] PM: Registered nosave memory: [mem 0x90000000-0xfdffffff]
[    0.679215] PM: Registered nosave memory: [mem 0xfe000000-0xfe010fff]
[    0.679215] PM: Registered nosave memory: [mem 0xfe011000-0xffffffff]
[    0.679217] [mem 0x90000000-0xfdffffff] available for PCI devices
[    0.679218] Booting paravirtualized kernel on bare hardware
[    0.679220] clocksource: refined-jiffies: mask: 0xffffffff max_cycles: 0xffffffff, max_idle_ns: 7645519600211568 ns
[    0.679226] setup_percpu: NR_CPUS:8192 nr_cpumask_bits:32 nr_cpu_ids:32 nr_node_ids:1
[    0.680388] percpu: Embedded 55 pages/cpu s188416 r8192 d28672 u262144
[    0.680395] pcpu-alloc: s188416 r8192 d28672 u262144 alloc=1*2097152
[    0.680396] pcpu-alloc: [0] 00 01 02 03 04 05 06 07 [0] 08 09 10 11 12 13 14 15 
[    0.680399] pcpu-alloc: [0] 16 17 18 19 20 21 22 23 [0] 24 25 26 27 28 29 30 31 
[    0.680428] Built 1 zonelists, mobility grouping on.  Total pages: 32859078
[    0.680428] Policy zone: Normal
[    0.680430] Kernel command line: BOOT_IMAGE=/boot/vmlinuz-5.4.0-86-generic root=UUID=7de8ba0b-9655-48cd-b568-ad06b26602a4 ro quiet splash default_hugepagesz=2MB hugepagesz=2M hugepages=8192 intel_iommu=on iommu=pt console=tty0 console=ttyS1,115200 quiet splash isolcpus=1,2 vt.handoff=7
[    0.680535] DMAR: IOMMU enabled
[    0.686691] Dentry cache hash table entries: 8388608 (order: 14, 67108864 bytes, linear)
[    0.689787] Inode-cache hash table entries: 4194304 (order: 13, 33554432 bytes, linear)
[    0.690096] mem auto-init: stack:off, heap alloc:on, heap free:off
[    0.696139] Calgary: detecting Calgary via BIOS EBDA area
[    0.696141] Calgary: Unable to locate Rio Grande table in EBDA - bailing!
[    0.960569] Memory: 130973656K/133527088K available (14339K kernel code, 2400K rwdata, 5016K rodata, 2736K init, 4964K bss, 2553432K reserved, 0K cma-reserved)
[    0.960575] random: get_random_u64 called from kmem_cache_open+0x2d/0x410 with crng_init=0
[    0.960717] SLUB: HWalign=64, Order=0-3, MinObjects=0, CPUs=32, Nodes=1
[    0.960728] ftrace: allocating 44609 entries in 175 pages
[    0.974542] rcu: Hierarchical RCU implementation.
[    0.974543] rcu: 	RCU restricting CPUs from NR_CPUS=8192 to nr_cpu_ids=32.
[    0.974543] 	Tasks RCU enabled.
[    0.974544] rcu: RCU calculated value of scheduler-enlistment delay is 25 jiffies.
[    0.974544] rcu: Adjusting geometry for rcu_fanout_leaf=16, nr_cpu_ids=32
[    0.976703] NR_IRQS: 524544, nr_irqs: 2312, preallocated irqs: 16
[    0.976953] random: crng done (trusting CPU's manufacturer)
[    0.976977] vt handoff: transparent VT on vt#7
[    0.976981] Console: colour dummy device 80x25
[    0.976987] printk: console [tty0] enabled
[    0.977008] printk: console [ttyS1] enabled
[    0.977024] ACPI: Core revision 20190816
[    0.978932] clocksource: hpet: mask: 0xffffffff max_cycles: 0xffffffff, max_idle_ns: 79635855245 ns
[    0.978973] APIC: Switch to symmetric I/O mode setup
[    0.978975] DMAR: Host address width 46
[    0.978976] DMAR: DRHD base: 0x000000bb7fc000 flags: 0x0
[    0.978980] DMAR: dmar0: reg_base_addr bb7fc000 ver 4:0 cap 8ed008c40780466 ecap 60000f050df
[    0.978981] DMAR: DRHD base: 0x000000d0ffc000 flags: 0x0
[    0.978984] DMAR: dmar1: reg_base_addr d0ffc000 ver 4:0 cap 8ed008c40780466 ecap 60000f050df
[    0.978984] DMAR: DRHD base: 0x000000e67fc000 flags: 0x0
[    0.978987] DMAR: dmar2: reg_base_addr e67fc000 ver 4:0 cap 8ed008c40780466 ecap 60000f050df
[    0.978987] DMAR: DRHD base: 0x000000fb7fc000 flags: 0x0
[    0.978990] DMAR: dmar3: reg_base_addr fb7fc000 ver 4:0 cap 8ed008c40780466 ecap 60000f050df
[    0.978991] DMAR: DRHD base: 0x000000a5ffc000 flags: 0x1
[    0.978993] DMAR: dmar4: reg_base_addr a5ffc000 ver 4:0 cap 8ed008c40780466 ecap 60000f050df
[    0.978994] DMAR: RMRR base: 0x0000004244a000 end: 0x0000004a451fff
[    0.978995] DMAR: RMRR base: 0x00000069465000 end: 0x00000069467fff
[    0.978995] DMAR: ATSR flags: 0x0
[    0.978997] DMAR-IR: IOAPIC id 8 under DRHD base  0xa5ffc000 IOMMU 4
[    0.978997] DMAR-IR: HPET id 0 under DRHD base 0xa5ffc000
[    0.978998] DMAR-IR: Queued invalidation will be enabled to support x2apic and Intr-remapping.
[    0.980522] DMAR-IR: Enabled IRQ remapping in x2apic mode
[    0.983043] ..TIMER: vector=0x30 apic1=0 pin1=2 apic2=-1 pin2=-1
[    1.002900] clocksource: tsc-early: mask: 0xffffffffffffffff max_cycles: 0x22983777dd9, max_idle_ns: 440795300422 ns
[    1.002903] Calibrating delay loop (skipped), value calculated using timer frequency.. 4800.00 BogoMIPS (lpj=9600000)
[    1.002905] pid_max: default: 32768 minimum: 301
[    1.006198] LSM: Security Framework initializing
[    1.006209] Yama: becoming mindful.
[    1.006301] AppArmor: AppArmor initialized
[    1.006430] Mount-cache hash table entries: 131072 (order: 8, 1048576 bytes, linear)
[    1.006531] Mountpoint-cache hash table entries: 131072 (order: 8, 1048576 bytes, linear)
[    1.006547] *** VALIDATE tmpfs ***
[    1.006679] *** VALIDATE proc ***
[    1.006723] *** VALIDATE cgroup1 ***
[    1.006724] *** VALIDATE cgroup2 ***
[    1.006801] x86/tme: not enabled by BIOS
[    1.006803] x86/cpu: User Mode Instruction Prevention (UMIP) activated
[    1.006829] mce: CPU0: Thermal monitoring enabled (TM1)
[    1.007033] process: using mwait in idle threads
[    1.007035] Last level iTLB entries: 4KB 0, 2MB 0, 4MB 0
[    1.007036] Last level dTLB entries: 4KB 0, 2MB 0, 4MB 0, 1GB 0
[    1.007040] Spectre V1 : Mitigation: usercopy/swapgs barriers and __user pointer sanitization
[    1.007042] Spectre V2 : Mitigation: Enhanced IBRS
[    1.007042] Spectre V2 : Spectre v2 / SpectreRSB mitigation: Filling RSB on context switch
[    1.007043] Spectre V2 : mitigation: Enabling conditional Indirect Branch Prediction Barrier
[    1.007045] Speculative Store Bypass: Mitigation: Speculative Store Bypass disabled via prctl and seccomp
[    1.007268] Freeing SMP alternatives memory: 40K
[    1.010984] smpboot: CPU0: Intel(R) Xeon(R) Silver 4314 CPU @ 2.40GHz (family: 0x6, model: 0x6a, stepping: 0x6)
[    1.011070] Performance Events: PEBS fmt4+-baseline, Icelake events, 32-deep LBR, full-width counters, Intel PMU driver.
[    1.011074] ... version:                5
[    1.011074] ... bit width:              48
[    1.011075] ... generic registers:      8
[    1.011075] ... value mask:             0000ffffffffffff
[    1.011076] ... max period:             00007fffffffffff
[    1.011076] ... fixed-purpose events:   4
[    1.011076] ... event mask:             0000000f000000ff
[    1.011112] rcu: Hierarchical SRCU implementation.
[    1.013079] NMI watchdog: Enabled. Permanently consumes one hw-PMU counter.
[    1.013289] smp: Bringing up secondary CPUs ...
[    1.013360] x86: Booting SMP configuration:
[    1.013361] .... node  #0, CPUs:        #1  #2  #3  #4  #5  #6  #7  #8  #9 #10 #11 #12 #13 #14 #15 #16 #17 #18 #19 #20 #21 #22 #23 #24 #25 #26 #27 #28 #29 #30 #31
[    1.193129] smp: Brought up 1 node, 32 CPUs
[    1.193129] smpboot: Max logical packages: 1
[    1.193129] smpboot: Total of 32 processors activated (153600.00 BogoMIPS)
[    1.197327] devtmpfs: initialized
[    1.197327] x86/mm: Memory block size: 2048MB
[    1.197327] PM: Registering ACPI NVS region [mem 0x4a6dc000-0x4b6dbfff] (16777216 bytes)
[    1.197327] PM: Registering ACPI NVS region [mem 0x6e3ff000-0x6f3fefff] (16777216 bytes)
[    1.197327] clocksource: jiffies: mask: 0xffffffff max_cycles: 0xffffffff, max_idle_ns: 7645041785100000 ns
[    1.197327] futex hash table entries: 8192 (order: 7, 524288 bytes, linear)
[    1.197327] pinctrl core: initialized pinctrl subsystem
[    1.197327] PM: RTC time: 07:55:51, date: 2025-05-17
[    1.197327] NET: Registered protocol family 16
[    1.197327] audit: initializing netlink subsys (disabled)
[    1.198917] audit: type=2000 audit(1747468551.220:1): state=initialized audit_enabled=0 res=1
[    1.198993] EISA bus registered
[    1.199002] cpuidle: using governor ladder
[    1.199002] cpuidle: using governor menu
[    1.199002] ACPI FADT declares the system doesn't support PCIe ASPM, so disable it
[    1.199002] ACPI: bus type PCI registered
[    1.199002] acpiphp: ACPI Hot Plug PCI Controller Driver version: 0.5
[    1.199002] PCI: MMCONFIG for domain 0000 [bus 00-ff] at [mem 0x80000000-0x8fffffff] (base 0x80000000)
[    1.199002] PCI: MMCONFIG at [mem 0x80000000-0x8fffffff] reserved in E820
[    1.199002] PCI: Using configuration type 1 for base access
[    1.199002] PCI: Dell System detected, enabling pci=bfsort.
[    2.932150] HugeTLB registered 2.00 MiB page size, pre-allocated 8192 pages
[    2.932152] HugeTLB registered 1.00 GiB page size, pre-allocated 0 pages
[    2.934985] ACPI: Added _OSI(Module Device)
[    2.934986] ACPI: Added _OSI(Processor Device)
[    2.934986] ACPI: Added _OSI(3.0 _SCP Extensions)
[    2.934987] ACPI: Added _OSI(Processor Aggregator Device)
[    2.934988] ACPI: Added _OSI(Linux-Dell-Video)
[    2.934988] ACPI: Added _OSI(Linux-Lenovo-NV-HDMI-Audio)
[    2.934989] ACPI: Added _OSI(Linux-HPI-Hybrid-Graphics)
[    3.055083] ACPI: 7 ACPI AML tables successfully acquired and loaded
[    3.060102] ACPI: [Firmware Bug]: BIOS _OSI(Linux) query ignored
[    3.085426] ACPI: Dynamic OEM Table Load:
[    3.279163] ACPI: Dynamic OEM Table Load:
[    3.309699] ACPI: Dynamic OEM Table Load:
[    3.607118] ACPI: Interpreter enabled
[    3.607138] ACPI: (supports S0 S5)
[    3.607139] ACPI: Using IOAPIC for interrupt routing
[    3.607223] HEST: Table parsing has been initialized.
[    3.607225] PCI: Using host bridge windows from ACPI; if necessary, use "pci=nocrs" and report a bug
[    3.626621] ACPI: Enabled 4 GPEs in block 00 to 7F
[    3.706135] ACPI: PCI Root Bridge [PC00] (domain 0000 [bus 00-16])
[    3.706139] acpi PNP0A08:00: _OSC: OS supports [ExtendedConfig ASPM ClockPM Segments MSI HPX-Type3]
[    3.706212] acpi PNP0A08:00: PCIe AER handled by firmware
[    3.706308] acpi PNP0A08:00: _OSC: platform does not support [SHPCHotplug LTR]
[    3.706485] acpi PNP0A08:00: _OSC: OS now controls [PCIeHotplug PME PCIeCapability]
[    3.706486] acpi PNP0A08:00: FADT indicates ASPM is unsupported, using BIOS configuration
[    3.707897] PCI host bridge to bus 0000:00
[    3.707899] pci_bus 0000:00: root bus resource [io  0x0000-0x0cf7 window]
[    3.707900] pci_bus 0000:00: root bus resource [io  0x1000-0x4fff window]
[    3.707901] pci_bus 0000:00: root bus resource [mem 0x000a0000-0x000bffff window]
[    3.707901] pci_bus 0000:00: root bus resource [mem 0x000c8000-0x000cffff window]
[    3.707902] pci_bus 0000:00: root bus resource [mem 0xfe010000-0xfe010fff window]
[    3.707903] pci_bus 0000:00: root bus resource [mem 0x90000000-0xa5ffffff window]
[    3.707903] pci_bus 0000:00: root bus resource [mem 0x380000000000-0x383fffffffff window]
[    3.707904] pci_bus 0000:00: root bus resource [bus 00-16]
[    3.707913] pci 0000:00:00.0: [8086:09a2] type 00 class 0x088000
[    3.708469] pci 0000:00:00.1: [8086:09a4] type 00 class 0x088000
[    3.709002] pci 0000:00:00.2: [8086:09a3] type 00 class 0x088000
[    3.709532] pci 0000:00:00.4: [8086:0998] type 00 class 0x060000
[    3.710063] pci 0000:00:02.0: [8086:09a6] type 00 class 0x088000
[    3.710072] pci 0000:00:02.0: reg 0x10: [mem 0x932fc000-0x932fdfff]
[    3.710603] pci 0000:00:02.1: [8086:09a7] type 00 class 0x088000
[    3.710611] pci 0000:00:02.1: reg 0x10: [mem 0x93200000-0x9327ffff]
[    3.710616] pci 0000:00:02.1: reg 0x14: [mem 0x93180000-0x931fffff]
[    3.711145] pci 0000:00:02.4: [8086:3456] type 00 class 0x130000
[    3.711156] pci 0000:00:02.4: reg 0x10: [mem 0x92f00000-0x92ffffff 64bit]
[    3.711161] pci 0000:00:02.4: reg 0x18: [mem 0x932f0000-0x932f3fff 64bit]
[    3.711167] pci 0000:00:02.4: reg 0x20: [mem 0x932c0000-0x932dffff 64bit]
[    3.711706] pci 0000:00:11.0: [8086:a1ec] type 00 class 0xff0000
[    3.711712] pci 0000:00:11.0: device has non-compliant BARs; disabling IO/MEM decoding
[    3.712255] pci 0000:00:11.5: [8086:a1d2] type 00 class 0x010601
[    3.712272] pci 0000:00:11.5: reg 0x10: [mem 0x932fa000-0x932fbfff]
[    3.712278] pci 0000:00:11.5: reg 0x14: [mem 0x93305000-0x933050ff]
[    3.712285] pci 0000:00:11.5: reg 0x18: [io  0x2068-0x206f]
[    3.712292] pci 0000:00:11.5: reg 0x1c: [io  0x2074-0x2077]
[    3.712298] pci 0000:00:11.5: reg 0x20: [io  0x2040-0x205f]
[    3.712305] pci 0000:00:11.5: reg 0x24: [mem 0x93080000-0x930fffff]
[    3.712341] pci 0000:00:11.5: PME# supported from D3hot
[    3.712884] pci 0000:00:14.0: [8086:a1af] type 00 class 0x0c0330
[    3.712905] pci 0000:00:14.0: reg 0x10: [mem 0x932e0000-0x932effff 64bit]
[    3.712967] pci 0000:00:14.0: PME# supported from D3hot D3cold
[    3.713512] pci 0000:00:14.2: [8086:a1b1] type 00 class 0x118000
[    3.713532] pci 0000:00:14.2: reg 0x10: [mem 0x93302000-0x93302fff 64bit]
[    3.714106] pci 0000:00:16.0: [8086:a1ba] type 00 class 0x078000
[    3.714133] pci 0000:00:16.0: reg 0x10: [mem 0x93301000-0x93301fff 64bit]
[    3.714215] pci 0000:00:16.0: PME# supported from D3hot
[    3.714729] pci 0000:00:16.1: [8086:a1bb] type 00 class 0x078000
[    3.714757] pci 0000:00:16.1: reg 0x10: [mem 0x93300000-0x93300fff 64bit]
[    3.714840] pci 0000:00:16.1: PME# supported from D3hot
[    3.715360] pci 0000:00:16.4: [8086:a1be] type 00 class 0x078000
[    3.715388] pci 0000:00:16.4: reg 0x10: [mem 0x932ff000-0x932fffff 64bit]
[    3.715471] pci 0000:00:16.4: PME# supported from D3hot
[    3.715989] pci 0000:00:17.0: [8086:a182] type 00 class 0x010601
[    3.716005] pci 0000:00:17.0: reg 0x10: [mem 0x932f8000-0x932f9fff]
[    3.716012] pci 0000:00:17.0: reg 0x14: [mem 0x93304000-0x933040ff]
[    3.716019] pci 0000:00:17.0: reg 0x18: [io  0x2060-0x2067]
[    3.716025] pci 0000:00:17.0: reg 0x1c: [io  0x2070-0x2073]
[    3.716032] pci 0000:00:17.0: reg 0x20: [io  0x2020-0x203f]
[    3.716038] pci 0000:00:17.0: reg 0x24: [mem 0x93000000-0x9307ffff]
[    3.716075] pci 0000:00:17.0: PME# supported from D3hot
[    3.716616] pci 0000:00:1c.0: [8086:a190] type 01 class 0x060400
[    3.717159] pci 0000:00:1c.0: PME# supported from D0 D3hot D3cold
[    3.717319] pci 0000:00:1c.4: [8086:a194] type 01 class 0x060400
[    3.717388] pci 0000:00:1c.4: PME# supported from D0 D3hot D3cold
[    3.717975] pci 0000:00:1c.5: [8086:a195] type 01 class 0x060400
[    3.718046] pci 0000:00:1c.5: PME# supported from D0 D3hot D3cold
[    3.718640] pci 0000:00:1d.0: [8086:a198] type 01 class 0x060400
[    3.718713] pci 0000:00:1d.0: PME# supported from D0 D3hot D3cold
[    3.719313] pci 0000:00:1f.0: [8086:a1cb] type 00 class 0x060100
[    3.719948] pci 0000:00:1f.2: [8086:a1a1] type 00 class 0x058000
[    3.719964] pci 0000:00:1f.2: reg 0x10: [mem 0x932f4000-0x932f7fff]
[    3.720542] pci 0000:00:1f.4: [8086:a1a3] type 00 class 0x0c0500
[    3.720562] pci 0000:00:1f.4: reg 0x10: [mem 0x932fe000-0x932fe0ff 64bit]
[    3.720583] pci 0000:00:1f.4: reg 0x20: [io  0x2000-0x201f]
[    3.721105] pci 0000:00:1f.5: [8086:a1a4] type 00 class 0x0c8000
[    3.721121] pci 0000:00:1f.5: reg 0x10: [mem 0xfe010000-0xfe010fff]
[    3.721709] pci 0000:00:1c.0: PCI bridge to [bus 01]
[    3.721713] pci 0000:00:1c.0:   bridge window [mem 0x92a00000-0x92dfffff]
[    3.721757] pci 0000:02:00.0: [1556:be00] type 01 class 0x060400
[    3.721921] pci 0000:00:1c.4: PCI bridge to [bus 02-03]
[    3.721925] pci 0000:00:1c.4:   bridge window [mem 0x92000000-0x928fffff]
[    3.721928] pci 0000:00:1c.4:   bridge window [mem 0x91000000-0x91ffffff 64bit pref]
[    3.721959] pci_bus 0000:03: extended config space not accessible
[    3.721974] pci 0000:03:00.0: [102b:0536] type 00 class 0x030000
[    3.721992] pci 0000:03:00.0: reg 0x10: [mem 0x91000000-0x91ffffff pref]
[    3.722003] pci 0000:03:00.0: reg 0x14: [mem 0x92808000-0x9280bfff]
[    3.722013] pci 0000:03:00.0: reg 0x18: [mem 0x92000000-0x927fffff]
[    3.722059] pci 0000:03:00.0: BAR 0: assigned to efifb
[    3.722152] pci 0000:02:00.0: PCI bridge to [bus 03]
[    3.722159] pci 0000:02:00.0:   bridge window [mem 0x92000000-0x928fffff]
[    3.722165] pci 0000:02:00.0:   bridge window [mem 0x91000000-0x91ffffff 64bit pref]
[    3.722226] pci 0000:04:00.0: [14e4:165f] type 00 class 0x020000
[    3.722257] pci 0000:04:00.0: reg 0x10: [mem 0x92e30000-0x92e3ffff 64bit pref]
[    3.722272] pci 0000:04:00.0: reg 0x18: [mem 0x92e40000-0x92e4ffff 64bit pref]
[    3.722287] pci 0000:04:00.0: reg 0x20: [mem 0x92e50000-0x92e5ffff 64bit pref]
[    3.722297] pci 0000:04:00.0: reg 0x30: [mem 0xfffc0000-0xffffffff pref]
[    3.722387] pci 0000:04:00.0: PME# supported from D0 D3hot D3cold
[    3.722425] pci 0000:04:00.0: 4.000 Gb/s available PCIe bandwidth, limited by 5 GT/s x1 link at 0000:00:1c.5 (capable of 8.000 Gb/s with 5 GT/s x2 link)
[    3.722546] pci 0000:04:00.1: [14e4:165f] type 00 class 0x020000
[    3.722577] pci 0000:04:00.1: reg 0x10: [mem 0x92e00000-0x92e0ffff 64bit pref]
[    3.722592] pci 0000:04:00.1: reg 0x18: [mem 0x92e10000-0x92e1ffff 64bit pref]
[    3.722607] pci 0000:04:00.1: reg 0x20: [mem 0x92e20000-0x92e2ffff 64bit pref]
[    3.722617] pci 0000:04:00.1: reg 0x30: [mem 0xfffc0000-0xffffffff pref]
[    3.722707] pci 0000:04:00.1: PME# supported from D0 D3hot D3cold
[    3.722852] pci 0000:00:1c.5: PCI bridge to [bus 04]
[    3.722858] pci 0000:00:1c.5:   bridge window [mem 0x92e00000-0x92efffff 64bit pref]
[    3.722893] pci 0000:00:1d.0: PCI bridge to [bus 05]
[    3.722916] pci_bus 0000:00: on NUMA node 0
[    3.723431] ACPI: PCI Root Bridge [PC01] (domain 0000 [bus 17-4f])
[    3.723433] acpi PNP0A08:01: _OSC: OS supports [ExtendedConfig ASPM ClockPM Segments MSI HPX-Type3]
[    3.723524] acpi PNP0A08:01: PCIe AER handled by firmware
[    3.723621] acpi PNP0A08:01: _OSC: platform does not support [SHPCHotplug]
[    3.723801] acpi PNP0A08:01: _OSC: OS now controls [PCIeHotplug PME PCIeCapability LTR]
[    3.723802] acpi PNP0A08:01: FADT indicates ASPM is unsupported, using BIOS configuration
[    3.723904] PCI host bridge to bus 0000:17
[    3.723905] pci_bus 0000:17: root bus resource [io  0x5000-0x7fff window]
[    3.723906] pci_bus 0000:17: root bus resource [mem 0xa6000000-0xbb7fffff window]
[    3.723907] pci_bus 0000:17: root bus resource [mem 0x384000000000-0x387fffffffff window]
[    3.723908] pci_bus 0000:17: root bus resource [bus 17-4f]
[    3.723914] pci 0000:17:00.0: [8086:09a2] type 00 class 0x088000
[    3.723980] pci 0000:17:00.1: [8086:09a4] type 00 class 0x088000
[    3.724039] pci 0000:17:00.2: [8086:09a3] type 00 class 0x088000
[    3.724099] pci 0000:17:00.4: [8086:0998] type 00 class 0x060000
[    3.724162] pci 0000:17:02.0: [8086:347a] type 01 class 0x060400
[    3.724175] pci 0000:17:02.0: reg 0x10: [mem 0x387ffff00000-0x387ffff1ffff 64bit]
[    3.724209] pci 0000:17:02.0: PME# supported from D0 D3hot D3cold
[    3.724554] pci 0000:18:00.0: [15b3:1017] type 00 class 0x020000
[    3.724804] pci 0000:18:00.0: reg 0x10: [mem 0xa8000000-0xa9ffffff 64bit pref]
[    3.725177] pci 0000:18:00.0: reg 0x30: [mem 0xfff00000-0xffffffff pref]
[    3.727199] pci 0000:18:00.0: PME# supported from D3cold
[    3.727608] pci 0000:18:00.1: [15b3:1017] type 00 class 0x020000
[    3.727842] pci 0000:18:00.1: reg 0x10: [mem 0xa6000000-0xa7ffffff 64bit pref]
[    3.728214] pci 0000:18:00.1: reg 0x30: [mem 0xfff00000-0xffffffff pref]
[    3.729885] pci 0000:18:00.1: PME# supported from D3cold
[    3.730212] pci 0000:17:02.0: PCI bridge to [bus 18]
[    3.730215] pci 0000:17:02.0:   bridge window [mem 0xa6000000-0xa9ffffff 64bit pref]
[    3.730220] pci_bus 0000:17: on NUMA node 0
[    3.730280] ACPI: PCI Root Bridge [PC02] (domain 0000 [bus 50-88])
[    3.730282] acpi PNP0A08:02: _OSC: OS supports [ExtendedConfig ASPM ClockPM Segments MSI HPX-Type3]
[    3.730371] acpi PNP0A08:02: PCIe AER handled by firmware
[    3.730466] acpi PNP0A08:02: _OSC: platform does not support [SHPCHotplug]
[    3.730647] acpi PNP0A08:02: _OSC: OS now controls [PCIeHotplug PME PCIeCapability LTR]
[    3.730648] acpi PNP0A08:02: FADT indicates ASPM is unsupported, using BIOS configuration
[    3.730749] PCI host bridge to bus 0000:50
[    3.730750] pci_bus 0000:50: root bus resource [io  0x8000-0xafff window]
[    3.730751] pci_bus 0000:50: root bus resource [mem 0xbb800000-0xd0ffffff window]
[    3.730752] pci_bus 0000:50: root bus resource [mem 0x388000000000-0x38bfffffffff window]
[    3.730753] pci_bus 0000:50: root bus resource [bus 50-88]
[    3.730759] pci 0000:50:00.0: [8086:09a2] type 00 class 0x088000
[    3.730821] pci 0000:50:00.1: [8086:09a4] type 00 class 0x088000
[    3.730881] pci 0000:50:00.2: [8086:09a3] type 00 class 0x088000
[    3.730941] pci 0000:50:00.4: [8086:0998] type 00 class 0x060000
[    3.731007] pci_bus 0000:50: on NUMA node 0
[    3.731084] ACPI: PCI Root Bridge [PC04] (domain 0000 [bus 89-c1])
[    3.731086] acpi PNP0A08:04: _OSC: OS supports [ExtendedConfig ASPM ClockPM Segments MSI HPX-Type3]
[    3.731172] acpi PNP0A08:04: PCIe AER handled by firmware
[    3.731266] acpi PNP0A08:04: _OSC: platform does not support [SHPCHotplug]
[    3.731449] acpi PNP0A08:04: _OSC: OS now controls [PCIeHotplug PME PCIeCapability LTR]
[    3.731449] acpi PNP0A08:04: FADT indicates ASPM is unsupported, using BIOS configuration
[    3.731547] PCI host bridge to bus 0000:89
[    3.731548] pci_bus 0000:89: root bus resource [io  0xb000-0xdfff window]
[    3.731549] pci_bus 0000:89: root bus resource [mem 0xd1000000-0xe67fffff window]
[    3.731549] pci_bus 0000:89: root bus resource [mem 0x38c000000000-0x38ffffffffff window]
[    3.731550] pci_bus 0000:89: root bus resource [bus 89-c1]
[    3.731556] pci 0000:89:00.0: [8086:09a2] type 00 class 0x088000
[    3.731619] pci 0000:89:00.1: [8086:09a4] type 00 class 0x088000
[    3.731677] pci 0000:89:00.2: [8086:09a3] type 00 class 0x088000
[    3.731735] pci 0000:89:00.4: [8086:0998] type 00 class 0x060000
[    3.731800] pci_bus 0000:89: on NUMA node 0
[    3.731855] ACPI: PCI Root Bridge [PC05] (domain 0000 [bus c2-fa])
[    3.731856] acpi PNP0A08:05: _OSC: OS supports [ExtendedConfig ASPM ClockPM Segments MSI HPX-Type3]
[    3.731940] acpi PNP0A08:05: PCIe AER handled by firmware
[    3.732034] acpi PNP0A08:05: _OSC: platform does not support [SHPCHotplug]
[    3.732213] acpi PNP0A08:05: _OSC: OS now controls [PCIeHotplug PME PCIeCapability LTR]
[    3.732214] acpi PNP0A08:05: FADT indicates ASPM is unsupported, using BIOS configuration
[    3.732309] PCI host bridge to bus 0000:c2
[    3.732310] pci_bus 0000:c2: root bus resource [io  0xe000-0xffff window]
[    3.732310] pci_bus 0000:c2: root bus resource [mem 0xe6800000-0xfb7fffff window]
[    3.732311] pci_bus 0000:c2: root bus resource [mem 0x390000000000-0x393fffffffff window]
[    3.732312] pci_bus 0000:c2: root bus resource [bus c2-fa]
[    3.732318] pci 0000:c2:00.0: [8086:09a2] type 00 class 0x088000
[    3.732377] pci 0000:c2:00.1: [8086:09a4] type 00 class 0x088000
[    3.732436] pci 0000:c2:00.2: [8086:09a3] type 00 class 0x088000
[    3.732494] pci 0000:c2:00.4: [8086:0998] type 00 class 0x060000
[    3.732553] pci 0000:c2:04.0: [8086:347c] type 01 class 0x060400
[    3.732565] pci 0000:c2:04.0: reg 0x10: [mem 0x393ffff00000-0x393ffff1ffff 64bit]
[    3.732598] pci 0000:c2:04.0: PME# supported from D0 D3hot D3cold
[    3.732835] pci 0000:c3:00.0: [1000:0015] type 00 class 0x010400
[    3.732854] pci 0000:c3:00.0: reg 0x10: [mem 0xe6800000-0xe68fffff 64bit pref]
[    3.732862] pci 0000:c3:00.0: reg 0x18: [mem 0xe6900000-0xe69fffff 64bit pref]
[    3.732867] pci 0000:c3:00.0: reg 0x20: [mem 0xe6a00000-0xe6afffff]
[    3.732872] pci 0000:c3:00.0: reg 0x24: [io  0xe000-0xe0ff]
[    3.732877] pci 0000:c3:00.0: reg 0x30: [mem 0xfff00000-0xffffffff pref]
[    3.733933] pci 0000:c3:00.0: supports D1 D2
[    3.733983] pci 0000:c2:04.0: PCI bridge to [bus c3]
[    3.733985] pci 0000:c2:04.0:   bridge window [io  0xe000-0xefff]
[    3.733986] pci 0000:c2:04.0:   bridge window [mem 0xe6a00000-0xe6afffff]
[    3.733988] pci 0000:c2:04.0:   bridge window [mem 0xe6800000-0xe69fffff 64bit pref]
[    3.733993] pci_bus 0000:c2: on NUMA node 0
[    3.734047] ACPI: PCI Root Bridge [UC06] (domain 0000 [bus fe])
[    3.734049] acpi PNP0A03:00: _OSC: OS supports [ExtendedConfig ASPM ClockPM Segments MSI HPX-Type3]
[    3.734118] acpi PNP0A03:00: PCIe AER handled by firmware
[    3.734197] acpi PNP0A03:00: _OSC: platform does not support [SHPCHotplug LTR]
[    3.734344] acpi PNP0A03:00: _OSC: OS now controls [PCIeHotplug PME PCIeCapability]
[    3.734345] acpi PNP0A03:00: FADT indicates ASPM is unsupported, using BIOS configuration
[    3.734461] PCI host bridge to bus 0000:fe
[    3.734461] pci_bus 0000:fe: root bus resource [bus fe]
[    3.734467] pci 0000:fe:00.0: [8086:3450] type 00 class 0x088000
[    3.734538] pci 0000:fe:00.1: [8086:3451] type 00 class 0x088000
[    3.734599] pci 0000:fe:00.2: [8086:3452] type 00 class 0x088000
[    3.734667] pci 0000:fe:00.3: [8086:0998] type 00 class 0x060000
[    3.734729] pci 0000:fe:00.5: [8086:3455] type 00 class 0x088000
[    3.734794] pci 0000:fe:02.0: [8086:3440] type 00 class 0x088000
[    3.734919] pci 0000:fe:02.1: [8086:3441] type 00 class 0x088000
[    3.735036] pci 0000:fe:02.2: [8086:3442] type 00 class 0x088000
[    3.735152] pci 0000:fe:04.0: [8086:3440] type 00 class 0x088000
[    3.735274] pci 0000:fe:04.1: [8086:3441] type 00 class 0x088000
[    3.735397] pci 0000:fe:04.2: [8086:3442] type 00 class 0x088000
[    3.735519] pci 0000:fe:04.3: [8086:3443] type 00 class 0x088000
[    3.735640] pci 0000:fe:05.0: [8086:3445] type 00 class 0x088000
[    3.735764] pci 0000:fe:05.1: [8086:3446] type 00 class 0x088000
[    3.735880] pci 0000:fe:05.2: [8086:3447] type 00 class 0x088000
[    3.735998] pci 0000:fe:06.0: [8086:3445] type 00 class 0x088000
[    3.736088] pci 0000:fe:06.1: [8086:3446] type 00 class 0x088000
[    3.736170] pci 0000:fe:06.2: [8086:3447] type 00 class 0x088000
[    3.736259] pci 0000:fe:07.0: [8086:3445] type 00 class 0x088000
[    3.736389] pci 0000:fe:07.1: [8086:3446] type 00 class 0x088000
[    3.736512] pci 0000:fe:07.2: [8086:3447] type 00 class 0x088000
[    3.736634] pci 0000:fe:0b.0: [8086:3448] type 00 class 0x088000
[    3.736711] pci 0000:fe:0b.1: [8086:3448] type 00 class 0x088000
[    3.736783] pci 0000:fe:0b.2: [8086:344b] type 00 class 0x088000
[    3.736857] pci 0000:fe:0c.0: [8086:344a] type 00 class 0x110100
[    3.736957] pci 0000:fe:0d.0: [8086:344a] type 00 class 0x110100
[    3.737050] pci 0000:fe:0e.0: [8086:344a] type 00 class 0x110100
[    3.737184] pci 0000:fe:0f.0: [8086:344a] type 00 class 0x110100
[    3.737333] pci 0000:fe:1a.0: [8086:2880] type 00 class 0x110100
[    3.737430] pci 0000:fe:1b.0: [8086:2880] type 00 class 0x110100
[    3.737523] pci 0000:fe:1c.0: [8086:2880] type 00 class 0x110100
[    3.737667] pci 0000:fe:1d.0: [8086:2880] type 00 class 0x110100
[    3.737813] pci_bus 0000:fe: on NUMA node 0
[    3.737879] ACPI: PCI Root Bridge [UC07] (domain 0000 [bus ff])
[    3.737881] acpi PNP0A03:01: _OSC: OS supports [ExtendedConfig ASPM ClockPM Segments MSI HPX-Type3]
[    3.737950] acpi PNP0A03:01: PCIe AER handled by firmware
[    3.738028] acpi PNP0A03:01: _OSC: platform does not support [SHPCHotplug LTR]
[    3.738175] acpi PNP0A03:01: _OSC: OS now controls [PCIeHotplug PME PCIeCapability]
[    3.738175] acpi PNP0A03:01: FADT indicates ASPM is unsupported, using BIOS configuration
[    3.738298] PCI host bridge to bus 0000:ff
[    3.738299] pci_bus 0000:ff: root bus resource [bus ff]
[    3.738308] pci 0000:ff:00.0: [8086:344c] type 00 class 0x088000
[    3.738420] pci 0000:ff:00.1: [8086:344c] type 00 class 0x088000
[    3.738532] pci 0000:ff:00.2: [8086:344c] type 00 class 0x088000
[    3.738629] pci 0000:ff:00.3: [8086:344c] type 00 class 0x088000
[    3.738722] pci 0000:ff:00.4: [8086:344c] type 00 class 0x088000
[    3.738849] pci 0000:ff:00.5: [8086:344c] type 00 class 0x088000
[    3.738975] pci 0000:ff:00.6: [8086:344c] type 00 class 0x088000
[    3.739061] pci 0000:ff:00.7: [8086:344c] type 00 class 0x088000
[    3.739195] pci 0000:ff:01.0: [8086:344c] type 00 class 0x088000
[    3.739324] pci 0000:ff:01.1: [8086:344c] type 00 class 0x088000
[    3.739446] pci 0000:ff:01.2: [8086:344c] type 00 class 0x088000
[    3.739572] pci 0000:ff:01.3: [8086:344c] type 00 class 0x088000
[    3.739699] pci 0000:ff:01.4: [8086:344c] type 00 class 0x088000
[    3.739838] pci 0000:ff:01.5: [8086:344c] type 00 class 0x088000
[    3.739978] pci 0000:ff:01.6: [8086:344c] type 00 class 0x088000
[    3.740127] pci 0000:ff:01.7: [8086:344c] type 00 class 0x088000
[    3.740276] pci 0000:ff:02.0: [8086:344c] type 00 class 0x088000
[    3.740385] pci 0000:ff:02.1: [8086:344c] type 00 class 0x088000
[    3.740483] pci 0000:ff:02.2: [8086:344c] type 00 class 0x088000
[    3.740601] pci 0000:ff:02.3: [8086:344c] type 00 class 0x088000
[    3.740685] pci 0000:ff:02.4: [8086:344c] type 00 class 0x088000
[    3.740770] pci 0000:ff:02.5: [8086:344c] type 00 class 0x088000
[    3.740886] pci 0000:ff:02.6: [8086:344c] type 00 class 0x088000
[    3.741011] pci 0000:ff:02.7: [8086:344c] type 00 class 0x088000
[    3.741136] pci 0000:ff:03.0: [8086:344c] type 00 class 0x088000
[    3.741275] pci 0000:ff:03.1: [8086:344c] type 00 class 0x088000
[    3.741407] pci 0000:ff:03.2: [8086:344c] type 00 class 0x088000
[    3.741543] pci 0000:ff:03.3: [8086:344c] type 00 class 0x088000
[    3.741687] pci 0000:ff:0a.0: [8086:344d] type 00 class 0x088000
[    3.741798] pci 0000:ff:0a.1: [8086:344d] type 00 class 0x088000
[    3.741911] pci 0000:ff:0a.2: [8086:344d] type 00 class 0x088000
[    3.742010] pci 0000:ff:0a.3: [8086:344d] type 00 class 0x088000
[    3.742105] pci 0000:ff:0a.4: [8086:344d] type 00 class 0x088000
[    3.742231] pci 0000:ff:0a.5: [8086:344d] type 00 class 0x088000
[    3.742357] pci 0000:ff:0a.6: [8086:344d] type 00 class 0x088000
[    3.742440] pci 0000:ff:0a.7: [8086:344d] type 00 class 0x088000
[    3.742574] pci 0000:ff:0b.0: [8086:344d] type 00 class 0x088000
[    3.742703] pci 0000:ff:0b.1: [8086:344d] type 00 class 0x088000
[    3.742824] pci 0000:ff:0b.2: [8086:344d] type 00 class 0x088000
[    3.742955] pci 0000:ff:0b.3: [8086:344d] type 00 class 0x088000
[    3.743081] pci 0000:ff:0b.4: [8086:344d] type 00 class 0x088000
[    3.743220] pci 0000:ff:0b.5: [8086:344d] type 00 class 0x088000
[    3.743361] pci 0000:ff:0b.6: [8086:344d] type 00 class 0x088000
[    3.743506] pci 0000:ff:0b.7: [8086:344d] type 00 class 0x088000
[    3.743655] pci 0000:ff:0c.0: [8086:344d] type 00 class 0x088000
[    3.743764] pci 0000:ff:0c.1: [8086:344d] type 00 class 0x088000
[    3.743862] pci 0000:ff:0c.2: [8086:344d] type 00 class 0x088000
[    3.743982] pci 0000:ff:0c.3: [8086:344d] type 00 class 0x088000
[    3.744067] pci 0000:ff:0c.4: [8086:344d] type 00 class 0x088000
[    3.744154] pci 0000:ff:0c.5: [8086:344d] type 00 class 0x088000
[    3.744269] pci 0000:ff:0c.6: [8086:344d] type 00 class 0x088000
[    3.744390] pci 0000:ff:0c.7: [8086:344d] type 00 class 0x088000
[    3.744516] pci 0000:ff:0d.0: [8086:344d] type 00 class 0x088000
[    3.744656] pci 0000:ff:0d.1: [8086:344d] type 00 class 0x088000
[    3.744787] pci 0000:ff:0d.2: [8086:344d] type 00 class 0x088000
[    3.744928] pci 0000:ff:0d.3: [8086:344d] type 00 class 0x088000
[    3.745081] pci 0000:ff:1d.0: [8086:344f] type 00 class 0x088000
[    3.745201] pci 0000:ff:1d.1: [8086:3457] type 00 class 0x088000
[    3.745312] pci 0000:ff:1e.0: [8086:3458] type 00 class 0x088000
[    3.745391] pci 0000:ff:1e.1: [8086:3459] type 00 class 0x088000
[    3.745463] pci 0000:ff:1e.2: [8086:345a] type 00 class 0x088000
[    3.745533] pci 0000:ff:1e.3: [8086:345b] type 00 class 0x088000
[    3.745603] pci 0000:ff:1e.4: [8086:345c] type 00 class 0x088000
[    3.745676] pci 0000:ff:1e.5: [8086:345d] type 00 class 0x088000
[    3.745746] pci 0000:ff:1e.6: [8086:345e] type 00 class 0x088000
[    3.745816] pci 0000:ff:1e.7: [8086:345f] type 00 class 0x088000
[    3.745886] pci_bus 0000:ff: on NUMA node 0
[    3.746592] HMAT: Memory (0x0 length 0x80000000) Flags:0003 Processor Domain:0 Memory Domain:0
[    3.746593] HMAT: Memory (0x100000000 length 0x1f80000000) Flags:0003 Processor Domain:0 Memory Domain:0
[    3.746595] HMAT: Locality: Flags:00 Type:Read Latency Initiator Domains:1 Target Domains:1 Base:100
[    3.746596]   Initiator-Target[0-0]:7600 nsec
[    3.746596] HMAT: Locality: Flags:00 Type:Write Latency Initiator Domains:1 Target Domains:1 Base:100
[    3.746597]   Initiator-Target[0-0]:7600 nsec
[    3.746598] HMAT: Locality: Flags:00 Type:Read Bandwidth Initiator Domains:1 Target Domains:1 Base:1
[    3.746598]   Initiator-Target[0-0]:1790 MB/s
[    3.746599] HMAT: Locality: Flags:00 Type:Write Bandwidth Initiator Domains:1 Target Domains:1 Base:1
[    3.746599]   Initiator-Target[0-0]:1910 MB/s
[    3.746915] iommu: Default domain type: Passthrough (set via kernel command line)
[    3.747038] SCSI subsystem initialized
[    3.747048] libata version 3.00 loaded.
[    3.747048] pci 0000:03:00.0: vgaarb: setting as boot VGA device
[    3.747048] pci 0000:03:00.0: vgaarb: VGA device added: decodes=io+mem,owns=io+mem,locks=none
[    3.747048] pci 0000:03:00.0: vgaarb: bridge control possible
[    3.747048] vgaarb: loaded
[    3.747048] ACPI: bus type USB registered
[    3.747048] usbcore: registered new interface driver usbfs
[    3.747048] usbcore: registered new interface driver hub
[    3.747048] usbcore: registered new device driver usb
[    3.747048] pps_core: LinuxPPS API ver. 1 registered
[    3.747048] pps_core: Software ver. 5.3.6 - Copyright 2005-2007 Rodolfo Giometti <giometti@linux.it>
[    3.747048] PTP clock support registered
[    3.747048] EDAC MC: Ver: 3.0.0
[    3.750922] Registered efivars operations
[    3.750951] PCI: Using ACPI for IRQ routing
[    3.753943] PCI: pci_cache_line_size set to 64 bytes
[    3.754249] e820: reserve RAM buffer [mem 0x0008f000-0x0008ffff]
[    3.754250] e820: reserve RAM buffer [mem 0x3916f020-0x3bffffff]
[    3.754251] e820: reserve RAM buffer [mem 0x3919f020-0x3bffffff]
[    3.754252] e820: reserve RAM buffer [mem 0x391c9020-0x3bffffff]
[    3.754253] e820: reserve RAM buffer [mem 0x391f3020-0x3bffffff]
[    3.754253] e820: reserve RAM buffer [mem 0x39226020-0x3bffffff]
[    3.754254] e820: reserve RAM buffer [mem 0x39259020-0x3bffffff]
[    3.754255] e820: reserve RAM buffer [mem 0x4244a000-0x43ffffff]
[    3.754255] e820: reserve RAM buffer [mem 0x4a6dc000-0x4bffffff]
[    3.754256] e820: reserve RAM buffer [mem 0x4c241000-0x4fffffff]
[    3.754256] e820: reserve RAM buffer [mem 0x4d802000-0x4fffffff]
[    3.754257] e820: reserve RAM buffer [mem 0x51b00000-0x53ffffff]
[    3.754258] e820: reserve RAM buffer [mem 0x5eefe000-0x5fffffff]
[    3.754258] e820: reserve RAM buffer [mem 0x6f800000-0x6fffffff]
[    3.754373] NetLabel: Initializing
[    3.754373] NetLabel:  domain hash size = 128
[    3.754373] NetLabel:  protocols = UNLABELED CIPSOv4 CALIPSO
[    3.754383] NetLabel:  unlabeled traffic allowed by default
[    3.754391] hpet0: at MMIO 0xfed00000, IRQs 2, 8, 0, 0, 0, 0, 0, 0
[    3.754391] hpet0: 8 comparators, 64-bit 24.000000 MHz counter
[    3.754907] clocksource: Switched to clocksource tsc-early
[    3.763377] *** VALIDATE bpf ***
[    3.763429] VFS: Disk quotas dquot_6.6.0
[    3.763444] VFS: Dquot-cache hash table entries: 512 (order 0, 4096 bytes)
[    3.763463] *** VALIDATE ramfs ***
[    3.763466] *** VALIDATE hugetlbfs ***
[    3.763534] AppArmor: AppArmor Filesystem Enabled
[    3.763548] pnp: PnP ACPI init
[    3.769585] pnp 00:00: Plug and Play ACPI device, IDs PNP0b00 (active)
[    3.769678] system 00:01: [io  0x0500-0x05fe] has been reserved
[    3.769679] system 00:01: [io  0x0400-0x041f] has been reserved
[    3.769679] system 00:01: [io  0x0600-0x061f] has been reserved
[    3.769680] system 00:01: [io  0x0ca0-0x0ca1] has been reserved
[    3.769681] system 00:01: [io  0x0ca4-0x0ca6] has been reserved
[    3.769683] system 00:01: [mem 0xff000000-0xffffffff] has been reserved
[    3.769685] system 00:01: Plug and Play ACPI device, IDs PNP0c02 (active)
[    3.769909] pnp 00:02: Plug and Play ACPI device, IDs PNP0501 (active)
[    3.770101] pnp 00:03: Plug and Play ACPI device, IDs PNP0501 (active)
[    3.770189] system 00:04: [mem 0xfd000000-0xfdabffff] has been reserved
[    3.770190] system 00:04: [mem 0xfdad0000-0xfdadffff] has been reserved
[    3.770191] system 00:04: [mem 0xfdb00000-0xfdffffff] has been reserved
[    3.770192] system 00:04: [mem 0xfe000000-0xfe00ffff] has been reserved
[    3.770193] system 00:04: [mem 0xfe011000-0xfe01ffff] has been reserved
[    3.770194] system 00:04: [mem 0xfe036000-0xfe03bfff] has been reserved
[    3.770194] system 00:04: [mem 0xfe03d000-0xfe3fffff] has been reserved
[    3.770195] system 00:04: [mem 0xfe410000-0xfe7fffff] has been reserved
[    3.770197] system 00:04: Plug and Play ACPI device, IDs PNP0c02 (active)
[    3.770408] system 00:05: [io  0x1000-0x10fe] has been reserved
[    3.770410] system 00:05: Plug and Play ACPI device, IDs PNP0c02 (active)
[    3.770949] pnp: PnP ACPI: found 6 devices
[    3.771882] thermal_sys: Registered thermal governor 'fair_share'
[    3.771883] thermal_sys: Registered thermal governor 'bang_bang'
[    3.771883] thermal_sys: Registered thermal governor 'step_wise'
[    3.771884] thermal_sys: Registered thermal governor 'user_space'
[    3.771884] thermal_sys: Registered thermal governor 'power_allocator'
[    3.776375] clocksource: acpi_pm: mask: 0xffffff max_cycles: 0xffffff, max_idle_ns: 2085701024 ns
[    3.776394] pci 0000:04:00.0: can't claim BAR 6 [mem 0xfffc0000-0xffffffff pref]: no compatible bridge window
[    3.776395] pci 0000:04:00.1: can't claim BAR 6 [mem 0xfffc0000-0xffffffff pref]: no compatible bridge window
[    3.776396] pci 0000:18:00.0: can't claim BAR 6 [mem 0xfff00000-0xffffffff pref]: no compatible bridge window
[    3.776397] pci 0000:18:00.1: can't claim BAR 6 [mem 0xfff00000-0xffffffff pref]: no compatible bridge window
[    3.776398] pci 0000:c3:00.0: can't claim BAR 6 [mem 0xfff00000-0xffffffff pref]: no compatible bridge window
[    3.776403] pci 0000:00:1c.0: bridge window [io  0x1000-0x0fff] to [bus 01] add_size 1000
[    3.776405] pci 0000:00:1c.0: bridge window [mem 0x00100000-0x000fffff 64bit pref] to [bus 01] add_size 200000 add_align 100000
[    3.776413] pci 0000:00:1c.0: BAR 15: assigned [mem 0x380000000000-0x3800001fffff 64bit pref]
[    3.776415] pci 0000:00:1c.5: BAR 14: assigned [mem 0x90000000-0x900fffff]
[    3.776416] pci 0000:00:1c.0: BAR 13: assigned [io  0x3000-0x3fff]
[    3.776418] pci 0000:00:1c.0: PCI bridge to [bus 01]
[    3.776420] pci 0000:00:1c.0:   bridge window [io  0x3000-0x3fff]
[    3.776423] pci 0000:00:1c.0:   bridge window [mem 0x92a00000-0x92dfffff]
[    3.776425] pci 0000:00:1c.0:   bridge window [mem 0x380000000000-0x3800001fffff 64bit pref]
[    3.776429] pci 0000:02:00.0: PCI bridge to [bus 03]
[    3.776434] pci 0000:02:00.0:   bridge window [mem 0x92000000-0x928fffff]
[    3.776437] pci 0000:02:00.0:   bridge window [mem 0x91000000-0x91ffffff 64bit pref]
[    3.776442] pci 0000:00:1c.4: PCI bridge to [bus 02-03]
[    3.776445] pci 0000:00:1c.4:   bridge window [mem 0x92000000-0x928fffff]
[    3.776447] pci 0000:00:1c.4:   bridge window [mem 0x91000000-0x91ffffff 64bit pref]
[    3.776451] pci 0000:04:00.0: BAR 6: assigned [mem 0x90000000-0x9003ffff pref]
[    3.776452] pci 0000:04:00.1: BAR 6: assigned [mem 0x90040000-0x9007ffff pref]
[    3.776453] pci 0000:00:1c.5: PCI bridge to [bus 04]
[    3.776456] pci 0000:00:1c.5:   bridge window [mem 0x90000000-0x900fffff]
[    3.776458] pci 0000:00:1c.5:   bridge window [mem 0x92e00000-0x92efffff 64bit pref]
[    3.776462] pci 0000:00:1d.0: PCI bridge to [bus 05]
[    3.776470] pci_bus 0000:00: resource 4 [io  0x0000-0x0cf7 window]
[    3.776471] pci_bus 0000:00: resource 5 [io  0x1000-0x4fff window]
[    3.776472] pci_bus 0000:00: resource 6 [mem 0x000a0000-0x000bffff window]
[    3.776472] pci_bus 0000:00: resource 7 [mem 0x000c8000-0x000cffff window]
[    3.776473] pci_bus 0000:00: resource 8 [mem 0xfe010000-0xfe010fff window]
[    3.776473] pci_bus 0000:00: resource 9 [mem 0x90000000-0xa5ffffff window]
[    3.776474] pci_bus 0000:00: resource 10 [mem 0x380000000000-0x383fffffffff window]
[    3.776475] pci_bus 0000:01: resource 0 [io  0x3000-0x3fff]
[    3.776476] pci_bus 0000:01: resource 1 [mem 0x92a00000-0x92dfffff]
[    3.776476] pci_bus 0000:01: resource 2 [mem 0x380000000000-0x3800001fffff 64bit pref]
[    3.776477] pci_bus 0000:02: resource 1 [mem 0x92000000-0x928fffff]
[    3.776478] pci_bus 0000:02: resource 2 [mem 0x91000000-0x91ffffff 64bit pref]
[    3.776478] pci_bus 0000:03: resource 1 [mem 0x92000000-0x928fffff]
[    3.776479] pci_bus 0000:03: resource 2 [mem 0x91000000-0x91ffffff 64bit pref]
[    3.776480] pci_bus 0000:04: resource 1 [mem 0x90000000-0x900fffff]
[    3.776480] pci_bus 0000:04: resource 2 [mem 0x92e00000-0x92efffff 64bit pref]
[    3.776551] pci 0000:17:02.0: BAR 14: assigned [mem 0xaa000000-0xaa1fffff]
[    3.776552] pci 0000:18:00.0: BAR 6: assigned [mem 0xaa000000-0xaa0fffff pref]
[    3.776553] pci 0000:18:00.1: BAR 6: assigned [mem 0xaa100000-0xaa1fffff pref]
[    3.776554] pci 0000:17:02.0: PCI bridge to [bus 18]
[    3.776556] pci 0000:17:02.0:   bridge window [mem 0xaa000000-0xaa1fffff]
[    3.776558] pci 0000:17:02.0:   bridge window [mem 0xa6000000-0xa9ffffff 64bit pref]
[    3.776561] pci_bus 0000:17: resource 4 [io  0x5000-0x7fff window]
[    3.776561] pci_bus 0000:17: resource 5 [mem 0xa6000000-0xbb7fffff window]
[    3.776562] pci_bus 0000:17: resource 6 [mem 0x384000000000-0x387fffffffff window]
[    3.776563] pci_bus 0000:18: resource 1 [mem 0xaa000000-0xaa1fffff]
[    3.776563] pci_bus 0000:18: resource 2 [mem 0xa6000000-0xa9ffffff 64bit pref]
[    3.776568] pci_bus 0000:50: resource 4 [io  0x8000-0xafff window]
[    3.776569] pci_bus 0000:50: resource 5 [mem 0xbb800000-0xd0ffffff window]
[    3.776570] pci_bus 0000:50: resource 6 [mem 0x388000000000-0x38bfffffffff window]
[    3.776574] pci_bus 0000:89: resource 4 [io  0xb000-0xdfff window]
[    3.776575] pci_bus 0000:89: resource 5 [mem 0xd1000000-0xe67fffff window]
[    3.776576] pci_bus 0000:89: resource 6 [mem 0x38c000000000-0x38ffffffffff window]
[    3.776582] pci 0000:c3:00.0: BAR 6: no space for [mem size 0x00100000 pref]
[    3.776583] pci 0000:c3:00.0: BAR 6: failed to assign [mem size 0x00100000 pref]
[    3.776583] pci 0000:c2:04.0: PCI bridge to [bus c3]
[    3.776585] pci 0000:c2:04.0:   bridge window [io  0xe000-0xefff]
[    3.776587] pci 0000:c2:04.0:   bridge window [mem 0xe6a00000-0xe6afffff]
[    3.776588] pci 0000:c2:04.0:   bridge window [mem 0xe6800000-0xe69fffff 64bit pref]
[    3.776591] pci_bus 0000:c2: resource 4 [io  0xe000-0xffff window]
[    3.776592] pci_bus 0000:c2: resource 5 [mem 0xe6800000-0xfb7fffff window]
[    3.776592] pci_bus 0000:c2: resource 6 [mem 0x390000000000-0x393fffffffff window]
[    3.776593] pci_bus 0000:c3: resource 0 [io  0xe000-0xefff]
[    3.776594] pci_bus 0000:c3: resource 1 [mem 0xe6a00000-0xe6afffff]
[    3.776594] pci_bus 0000:c3: resource 2 [mem 0xe6800000-0xe69fffff 64bit pref]
[    3.776651] NET: Registered protocol family 2
[    3.776869] IP idents hash table entries: 262144 (order: 9, 2097152 bytes, linear)
[    3.778763] tcp_listen_portaddr_hash hash table entries: 65536 (order: 8, 1048576 bytes, linear)
[    3.779257] TCP established hash table entries: 524288 (order: 10, 4194304 bytes, linear)
[    3.779743] TCP bind hash table entries: 65536 (order: 8, 1048576 bytes, linear)
[    3.779792] TCP: Hash tables configured (established 524288 bind 65536)
[    3.780055] UDP hash table entries: 65536 (order: 9, 2097152 bytes, linear)
[    3.780400] UDP-Lite hash table entries: 65536 (order: 9, 2097152 bytes, linear)
[    3.780581] NET: Registered protocol family 1
[    3.780585] NET: Registered protocol family 44
[    3.780816] pci 0000:03:00.0: Video device with shadowed ROM at [mem 0x000c0000-0x000dffff]
[    3.781003] PCI: CLS 0 bytes, default 64
[    3.781032] Trying to unpack rootfs image as initramfs...
[    4.005930] Freeing initrd memory: 135824K
[    4.006213] DMAR: dmar3: Using Queued invalidation
[    4.006217] DMAR: dmar0: Using Queued invalidation
[    4.006219] DMAR: dmar4: Using Queued invalidation
[    4.006467] pci 0000:00:00.0: Adding to iommu group 0
[    4.006487] pci 0000:00:00.1: Adding to iommu group 1
[    4.006508] pci 0000:00:00.2: Adding to iommu group 2
[    4.006527] pci 0000:00:00.4: Adding to iommu group 3
[    4.006589] pci 0000:00:02.0: Adding to iommu group 4
[    4.006608] pci 0000:00:02.1: Adding to iommu group 4
[    4.006626] pci 0000:00:02.4: Adding to iommu group 4
[    4.006674] pci 0000:00:11.0: Adding to iommu group 5
[    4.006693] pci 0000:00:11.5: Adding to iommu group 5
[    4.006739] pci 0000:00:14.0: Adding to iommu group 6
[    4.006758] pci 0000:00:14.2: Adding to iommu group 6
[    4.006820] pci 0000:00:16.0: Adding to iommu group 7
[    4.006840] pci 0000:00:16.1: Adding to iommu group 7
[    4.006858] pci 0000:00:16.4: Adding to iommu group 7
[    4.006877] pci 0000:00:17.0: Adding to iommu group 8
[    4.006925] pci 0000:00:1c.0: Adding to iommu group 9
[    4.006948] pci 0000:00:1c.4: Adding to iommu group 10
[    4.006972] pci 0000:00:1c.5: Adding to iommu group 11
[    4.006995] pci 0000:00:1d.0: Adding to iommu group 12
[    4.007071] pci 0000:00:1f.0: Adding to iommu group 13
[    4.007091] pci 0000:00:1f.2: Adding to iommu group 13
[    4.007112] pci 0000:00:1f.4: Adding to iommu group 13
[    4.007132] pci 0000:00:1f.5: Adding to iommu group 13
[    4.007155] pci 0000:02:00.0: Adding to iommu group 14
[    4.007361] pci 0000:02:00.0: Using iommu dma mapping
[    4.007367] pci 0000:03:00.0: Adding to iommu group 14
[    4.007450] pci 0000:04:00.0: Adding to iommu group 15
[    4.007486] pci 0000:04:00.1: Adding to iommu group 15
[    4.007508] pci 0000:17:00.0: Adding to iommu group 16
[    4.007527] pci 0000:17:00.1: Adding to iommu group 17
[    4.007546] pci 0000:17:00.2: Adding to iommu group 18
[    4.007566] pci 0000:17:00.4: Adding to iommu group 19
[    4.007588] pci 0000:17:02.0: Adding to iommu group 20
[    4.007709] pci 0000:18:00.0: Adding to iommu group 21
[    4.007806] pci 0000:18:00.1: Adding to iommu group 22
[    4.007827] pci 0000:50:00.0: Adding to iommu group 23
[    4.007846] pci 0000:50:00.1: Adding to iommu group 24
[    4.007865] pci 0000:50:00.2: Adding to iommu group 25
[    4.007886] pci 0000:50:00.4: Adding to iommu group 26
[    4.007906] pci 0000:89:00.0: Adding to iommu group 27
[    4.007925] pci 0000:89:00.1: Adding to iommu group 28
[    4.007946] pci 0000:89:00.2: Adding to iommu group 29
[    4.007965] pci 0000:89:00.4: Adding to iommu group 30
[    4.007985] pci 0000:c2:00.0: Adding to iommu group 31
[    4.008004] pci 0000:c2:00.1: Adding to iommu group 32
[    4.008023] pci 0000:c2:00.2: Adding to iommu group 33
[    4.008043] pci 0000:c2:00.4: Adding to iommu group 34
[    4.008064] pci 0000:c2:04.0: Adding to iommu group 35
[    4.008088] pci 0000:c3:00.0: Adding to iommu group 36
[    4.008108] pci 0000:fe:00.0: Adding to iommu group 37
[    4.008128] pci 0000:fe:00.1: Adding to iommu group 38
[    4.008147] pci 0000:fe:00.2: Adding to iommu group 39
[    4.008166] pci 0000:fe:00.3: Adding to iommu group 40
[    4.008185] pci 0000:fe:00.5: Adding to iommu group 41
[    4.008206] pci 0000:fe:02.0: Adding to iommu group 42
[    4.008226] pci 0000:fe:02.1: Adding to iommu group 43
[    4.008245] pci 0000:fe:02.2: Adding to iommu group 44
[    4.008265] pci 0000:fe:04.0: Adding to iommu group 45
[    4.008285] pci 0000:fe:04.1: Adding to iommu group 46
[    4.008304] pci 0000:fe:04.2: Adding to iommu group 47
[    4.008324] pci 0000:fe:04.3: Adding to iommu group 48
[    4.008344] pci 0000:fe:05.0: Adding to iommu group 49
[    4.008364] pci 0000:fe:05.1: Adding to iommu group 50
[    4.008384] pci 0000:fe:05.2: Adding to iommu group 51
[    4.008403] pci 0000:fe:06.0: Adding to iommu group 52
[    4.008425] pci 0000:fe:06.1: Adding to iommu group 53
[    4.008444] pci 0000:fe:06.2: Adding to iommu group 54
[    4.008464] pci 0000:fe:07.0: Adding to iommu group 55
[    4.008484] pci 0000:fe:07.1: Adding to iommu group 56
[    4.008505] pci 0000:fe:07.2: Adding to iommu group 57
[    4.008567] pci 0000:fe:0b.0: Adding to iommu group 58
[    4.008592] pci 0000:fe:0b.1: Adding to iommu group 58
[    4.008616] pci 0000:fe:0b.2: Adding to iommu group 58
[    4.008635] pci 0000:fe:0c.0: Adding to iommu group 59
[    4.008655] pci 0000:fe:0d.0: Adding to iommu group 60
[    4.008676] pci 0000:fe:0e.0: Adding to iommu group 61
[    4.008697] pci 0000:fe:0f.0: Adding to iommu group 62
[    4.008719] pci 0000:fe:1a.0: Adding to iommu group 63
[    4.008738] pci 0000:fe:1b.0: Adding to iommu group 64
[    4.008759] pci 0000:fe:1c.0: Adding to iommu group 65
[    4.008779] pci 0000:fe:1d.0: Adding to iommu group 66
[    4.008800] pci 0000:ff:00.0: Adding to iommu group 67
[    4.008820] pci 0000:ff:00.1: Adding to iommu group 68
[    4.008840] pci 0000:ff:00.2: Adding to iommu group 69
[    4.008859] pci 0000:ff:00.3: Adding to iommu group 70
[    4.008880] pci 0000:ff:00.4: Adding to iommu group 71
[    4.008902] pci 0000:ff:00.5: Adding to iommu group 72
[    4.008921] pci 0000:ff:00.6: Adding to iommu group 73
[    4.008941] pci 0000:ff:00.7: Adding to iommu group 74
[    4.008962] pci 0000:ff:01.0: Adding to iommu group 75
[    4.008981] pci 0000:ff:01.1: Adding to iommu group 76
[    4.009001] pci 0000:ff:01.2: Adding to iommu group 77
[    4.009022] pci 0000:ff:01.3: Adding to iommu group 78
[    4.009042] pci 0000:ff:01.4: Adding to iommu group 79
[    4.009062] pci 0000:ff:01.5: Adding to iommu group 80
[    4.009082] pci 0000:ff:01.6: Adding to iommu group 81
[    4.009104] pci 0000:ff:01.7: Adding to iommu group 82
[    4.009124] pci 0000:ff:02.0: Adding to iommu group 83
[    4.009144] pci 0000:ff:02.1: Adding to iommu group 84
[    4.009164] pci 0000:ff:02.2: Adding to iommu group 85
[    4.009185] pci 0000:ff:02.3: Adding to iommu group 86
[    4.009204] pci 0000:ff:02.4: Adding to iommu group 87
[    4.009224] pci 0000:ff:02.5: Adding to iommu group 88
[    4.009245] pci 0000:ff:02.6: Adding to iommu group 89
[    4.009265] pci 0000:ff:02.7: Adding to iommu group 90
[    4.009286] pci 0000:ff:03.0: Adding to iommu group 91
[    4.009306] pci 0000:ff:03.1: Adding to iommu group 92
[    4.009329] pci 0000:ff:03.2: Adding to iommu group 93
[    4.009349] pci 0000:ff:03.3: Adding to iommu group 94
[    4.009369] pci 0000:ff:0a.0: Adding to iommu group 95
[    4.009388] pci 0000:ff:0a.1: Adding to iommu group 96
[    4.009409] pci 0000:ff:0a.2: Adding to iommu group 97
[    4.009429] pci 0000:ff:0a.3: Adding to iommu group 98
[    4.009449] pci 0000:ff:0a.4: Adding to iommu group 99
[    4.009470] pci 0000:ff:0a.5: Adding to iommu group 100
[    4.009490] pci 0000:ff:0a.6: Adding to iommu group 101
[    4.009510] pci 0000:ff:0a.7: Adding to iommu group 102
[    4.009530] pci 0000:ff:0b.0: Adding to iommu group 103
[    4.009550] pci 0000:ff:0b.1: Adding to iommu group 104
[    4.009571] pci 0000:ff:0b.2: Adding to iommu group 105
[    4.009591] pci 0000:ff:0b.3: Adding to iommu group 106
[    4.009611] pci 0000:ff:0b.4: Adding to iommu group 107
[    4.009632] pci 0000:ff:0b.5: Adding to iommu group 108
[    4.009652] pci 0000:ff:0b.6: Adding to iommu group 109
[    4.009673] pci 0000:ff:0b.7: Adding to iommu group 110
[    4.009693] pci 0000:ff:0c.0: Adding to iommu group 111
[    4.009714] pci 0000:ff:0c.1: Adding to iommu group 112
[    4.009734] pci 0000:ff:0c.2: Adding to iommu group 113
[    4.009754] pci 0000:ff:0c.3: Adding to iommu group 114
[    4.009774] pci 0000:ff:0c.4: Adding to iommu group 115
[    4.009795] pci 0000:ff:0c.5: Adding to iommu group 116
[    4.009815] pci 0000:ff:0c.6: Adding to iommu group 117
[    4.009835] pci 0000:ff:0c.7: Adding to iommu group 118
[    4.009856] pci 0000:ff:0d.0: Adding to iommu group 119
[    4.009880] pci 0000:ff:0d.1: Adding to iommu group 120
[    4.009901] pci 0000:ff:0d.2: Adding to iommu group 121
[    4.009922] pci 0000:ff:0d.3: Adding to iommu group 122
[    4.009943] pci 0000:ff:1d.0: Adding to iommu group 123
[    4.009963] pci 0000:ff:1d.1: Adding to iommu group 124
[    4.010099] pci 0000:ff:1e.0: Adding to iommu group 125
[    4.010130] pci 0000:ff:1e.1: Adding to iommu group 125
[    4.010162] pci 0000:ff:1e.2: Adding to iommu group 125
[    4.010194] pci 0000:ff:1e.3: Adding to iommu group 125
[    4.010226] pci 0000:ff:1e.4: Adding to iommu group 125
[    4.010258] pci 0000:ff:1e.5: Adding to iommu group 125
[    4.010289] pci 0000:ff:1e.6: Adding to iommu group 125
[    4.010320] pci 0000:ff:1e.7: Adding to iommu group 125
[    4.010327] DMAR: Intel(R) Virtualization Technology for Directed I/O
[    4.011786] check: Scanning for low memory corruption every 60 seconds
[    4.012254] Initialise system trusted keyrings
[    4.012261] Key type blacklist registered
[    4.012284] workingset: timestamp_bits=36 max_order=25 bucket_order=0
[    4.013114] zbud: loaded
[    4.013366] squashfs: version 4.0 (2009/01/31) Phillip Lougher
[    4.013477] fuse: init (API version 7.31)
[    4.013487] *** VALIDATE fuse ***
[    4.013488] *** VALIDATE fuse ***
[    4.013560] Platform Keyring initialized
[    4.016947] Key type asymmetric registered
[    4.016948] Asymmetric key parser 'x509' registered
[    4.016952] Block layer SCSI generic (bsg) driver version 0.4 loaded (major 244)
[    4.016981] io scheduler mq-deadline registered
[    4.017443] pcieport 0000:00:1c.0: PME: Signaling with IRQ 125
[    4.017461] pcieport 0000:00:1c.0: pciehp: Slot #2 AttnBtn- PwrCtrl- MRL- AttnInd- PwrInd- HotPlug+ Surprise+ Interlock- NoCompl+ LLActRep+
[    4.017629] pcieport 0000:00:1c.4: PME: Signaling with IRQ 126
[    4.017775] pcieport 0000:00:1c.5: PME: Signaling with IRQ 127
[    4.017909] pcieport 0000:00:1d.0: PME: Signaling with IRQ 128
[    4.018040] pcieport 0000:17:02.0: PME: Signaling with IRQ 129
[    4.018142] pcieport 0000:c2:04.0: PME: Signaling with IRQ 130
[    4.018214] shpchp: Standard Hot Plug PCI Controller Driver version: 0.4
[    4.018265] efifb: probing for efifb
[    4.018284] efifb: No BGRT, not showing boot graphics
[    4.018285] efifb: framebuffer at 0x91000000, using 3072k, total 3072k
[    4.018286] efifb: mode is 1024x768x32, linelength=4096, pages=1
[    4.018286] efifb: scrolling: redraw
[    4.018287] efifb: Truecolor: size=8:8:8:8, shift=24:16:8:0
[    4.018312] fbcon: Deferring console take-over
[    4.018312] fb0: EFI VGA frame buffer device
[    4.018317] intel_idle: does not run on family 6 model 106
[    4.018694] input: Power Button as /devices/LNXSYSTM:00/LNXPWRBN:00/input/input0
[    4.018725] ACPI: Power Button [PWRF]
[    4.018926] Monitor-Mwait will be used to enter C-1 state
[    4.018935] Monitor-Mwait will be used to enter C-2 state
[    4.024204] ERST: Error Record Serialization Table (ERST) support is initialized.
[    4.024209] pstore: Registered erst as persistent store backend
[    4.024388] GHES: APEI firmware first mode is enabled by APEI bit and WHEA _OSC.
[    4.024469] Serial: 8250/16550 driver, 32 ports, IRQ sharing enabled
[    4.045207] 00:02: ttyS0 at I/O 0x3f8 (irq = 4, base_baud = 115200) is a 16550A
[    4.065965] 00:03: ttyS1 at I/O 0x2f8 (irq = 3, base_baud = 115200) is a 16550A
[    4.067011] Linux agpgart interface v0.103
[    4.210956] loop: module loaded
[    4.211301] libphy: Fixed MDIO Bus: probed
[    4.211302] tun: Universal TUN/TAP device driver, 1.6
[    4.211456] PPP generic driver version 2.4.2
[    4.211775] VFIO - User Level meta-driver version: 0.3
[    4.212029] ehci_hcd: USB 2.0 'Enhanced' Host Controller (EHCI) Driver
[    4.212033] ehci-pci: EHCI PCI platform driver
[    4.212052] ehci-platform: EHCI generic platform driver
[    4.212058] ohci_hcd: USB 1.1 'Open' Host Controller (OHCI) Driver
[    4.212059] ohci-pci: OHCI PCI platform driver
[    4.212079] ohci-platform: OHCI generic platform driver
[    4.212084] uhci_hcd: USB Universal Host Controller Interface driver
[    4.212279] xhci_hcd 0000:00:14.0: xHCI Host Controller
[    4.212284] xhci_hcd 0000:00:14.0: new USB bus registered, assigned bus number 1
[    4.213354] xhci_hcd 0000:00:14.0: hcc params 0x200077c1 hci version 0x100 quirks 0x0000000000009810
[    4.213358] xhci_hcd 0000:00:14.0: cache line size of 64 is not supported
[    4.213509] usb usb1: New USB device found, idVendor=1d6b, idProduct=0002, bcdDevice= 5.04
[    4.213510] usb usb1: New USB device strings: Mfr=3, Product=2, SerialNumber=1
[    4.213511] usb usb1: Product: xHCI Host Controller
[    4.213512] usb usb1: Manufacturer: Linux 5.4.0-86-generic xhci-hcd
[    4.213512] usb usb1: SerialNumber: 0000:00:14.0
[    4.213701] hub 1-0:1.0: USB hub found
[    4.213719] hub 1-0:1.0: 16 ports detected
[    4.215019] xhci_hcd 0000:00:14.0: xHCI Host Controller
[    4.215020] xhci_hcd 0000:00:14.0: new USB bus registered, assigned bus number 2
[    4.215022] xhci_hcd 0000:00:14.0: Host supports USB 3.0 SuperSpeed
[    4.215044] usb usb2: New USB device found, idVendor=1d6b, idProduct=0003, bcdDevice= 5.04
[    4.215045] usb usb2: New USB device strings: Mfr=3, Product=2, SerialNumber=1
[    4.215045] usb usb2: Product: xHCI Host Controller
[    4.215046] usb usb2: Manufacturer: Linux 5.4.0-86-generic xhci-hcd
[    4.215047] usb usb2: SerialNumber: 0000:00:14.0
[    4.215094] hub 2-0:1.0: USB hub found
[    4.215108] hub 2-0:1.0: 10 ports detected
[    4.215388] usb: port power management may be unreliable
[    4.215979] i8042: PNP: No PS/2 controller found.
[    4.216022] mousedev: PS/2 mouse device common for all mice
[    4.216086] rtc_cmos 00:00: RTC can wake from S4
[    4.216383] rtc_cmos 00:00: registered as rtc0
[    4.216393] rtc_cmos 00:00: alarms up to one month, y3k, 114 bytes nvram, hpet irqs
[    4.216397] i2c /dev entries driver
[    4.216435] device-mapper: uevent: version 1.0.3
[    4.216470] device-mapper: ioctl: 4.41.0-ioctl (2019-09-16) initialised: dm-devel@redhat.com
[    4.216483] platform eisa.0: Probing EISA bus 0
[    4.216484] platform eisa.0: EISA: Cannot allocate resource for mainboard
[    4.216485] platform eisa.0: Cannot allocate resource for EISA slot 1
[    4.216485] platform eisa.0: Cannot allocate resource for EISA slot 2
[    4.216486] platform eisa.0: Cannot allocate resource for EISA slot 3
[    4.216487] platform eisa.0: Cannot allocate resource for EISA slot 4
[    4.216488] platform eisa.0: Cannot allocate resource for EISA slot 5
[    4.216488] platform eisa.0: Cannot allocate resource for EISA slot 6
[    4.216489] platform eisa.0: Cannot allocate resource for EISA slot 7
[    4.216490] platform eisa.0: Cannot allocate resource for EISA slot 8
[    4.216491] platform eisa.0: EISA: Detected 0 cards
[    4.216493] intel_pstate: Intel P-state driver initializing
[    4.222788] intel_pstate: HWP enabled
[    4.223001] ledtrig-cpu: registered to indicate activity on CPUs
[    4.223005] EFI Variables Facility v0.08 2004-May-17
[    4.224320] drop_monitor: Initializing network drop monitor service
[    4.224437] NET: Registered protocol family 10
[    4.229162] Segment Routing with IPv6
[    4.229178] NET: Registered protocol family 17
[    4.229197] Key type dns_resolver registered
[    4.234135] RAS: Correctable Errors collector initialized.
[    4.234154] microcode: sig=0x606a6, pf=0x1, revision=0xd0003f5
[    4.234289] microcode: Microcode Update Driver: v2.2.
[    4.234569] *** VALIDATE rdt ***
[    4.234572] resctrl: L3 allocation detected
[    4.234573] resctrl: MB allocation detected
[    4.234573] resctrl: L3 monitoring detected
[    4.234575] IPI shorthand broadcast: enabled
[    4.234579] sched_clock: Marking stable (4232574389, 1988932)->(4265555046, -30991725)
[    4.234636] registered taskstats version 1
[    4.234643] Loading compiled-in X.509 certificates
[    4.235179] Loaded X.509 cert 'Build time autogenerated kernel key: d0809dce857ddaa6fe9812f63ec00856014edbcd'
[    4.235652] Loaded X.509 cert 'Canonical Ltd. Live Patch Signing: 14df34d1a87cf37625abec039ef2bf521249b969'
[    4.236113] Loaded X.509 cert 'Canonical Ltd. Kernel Module Signing: 88f752e560a1e0737e31163a466ad7b70a850c19'
[    4.236218] zswap: loaded using pool lzo/zbud
[    4.236270] Key type ._fscrypt registered
[    4.236271] Key type .fscrypt registered
[    4.236338] pstore: Using crash dump compression: deflate
[    4.240470] Key type big_key registered
[    4.242445] Key type encrypted registered
[    4.242447] AppArmor: AppArmor sha1 policy hashing enabled
[    4.242488] integrity: Loading X.509 certificate: UEFI:db
[    4.242506] integrity: Loaded X.509 cert 'Microsoft Corporation UEFI CA 2011: 13adbf4309bd82709c8cd54f316ed522988a1bd4'
[    4.242506] integrity: Loading X.509 certificate: UEFI:db
[    4.242518] integrity: Loaded X.509 cert 'Microsoft Windows Production PCA 2011: a92902398e16c49778cd90f99e4f9ae17c55af53'
[    4.242518] integrity: Loading X.509 certificate: UEFI:db
[    4.242590] integrity: Loaded X.509 cert 'VMware, Inc.: 4ad8ba0472073d28127706ddc6ccb9050441bbc7'
[    4.242590] integrity: Loading X.509 certificate: UEFI:db
[    4.242736] integrity: Loaded X.509 cert 'VMware, Inc.: VMware Secure Boot Signing: 04597f3e1ffb240bba0ff0f05d5eb05f3e15f6d7'
[    4.242751] integrity: Loading X.509 certificate: UEFI:MokListRT
[    4.242898] integrity: Loaded X.509 cert 'Canonical Ltd. Master Certificate Authority: ad91990bc22ab1f517048c23b6655a268e345a63'
[    4.243061] ima: No TPM chip found, activating TPM-bypass!
[    4.243064] ima: Allocated hash algorithm: sha1
[    4.243068] ima: No architecture policies found
[    4.243078] evm: Initialising EVM extended attributes:
[    4.243078] evm: security.selinux
[    4.243078] evm: security.SMACK64
[    4.243079] evm: security.SMACK64EXEC
[    4.243079] evm: security.SMACK64TRANSMUTE
[    4.243079] evm: security.SMACK64MMAP
[    4.243080] evm: security.apparmor
[    4.243080] evm: security.ima
[    4.243080] evm: security.capability
[    4.243081] evm: HMAC attrs: 0x1
[    4.246292] PM:   Magic number: 9:901:925
[    4.246312] clockevents clockevent18: hash matches
[    4.246326] pci 0000:ff:0c.0: hash matches
[    4.246561] rtc_cmos 00:00: setting system clock to 2025-05-17T07:55:54 UTC (1747468554)
[    4.250683] Freeing unused decrypted memory: 2040K
[    4.251244] Freeing unused kernel image memory: 2736K
[    4.278957] Write protecting the kernel read-only data: 22528k
[    4.280200] Freeing unused kernel image memory: 2008K
[    4.280548] Freeing unused kernel image memory: 1128K
[    4.285725] x86/mm: Checked W+X mappings: passed, no W+X pages found.
[    4.285727] Run /init as init process
[    4.381106] wmi_bus wmi_bus-PNP0C14:00: WQBC data block query control method not found
[    4.382786] memtrack: loading out-of-tree module taints kernel.
[    4.382805] memtrack: module verification failed: signature and/or required key missing - tainting kernel
[    4.383018] i801_smbus 0000:00:1f.4: SPD Write Disable is set
[    4.383046] i801_smbus 0000:00:1f.4: SMBus using PCI interrupt
[    4.383125] ahci 0000:00:11.5: version 3.0
[    4.383272] ahci 0000:00:11.5: AHCI 0001.0301 32 slots 2 ports 6 Gbps 0x3 impl SATA mode
[    4.383273] ahci 0000:00:11.5: flags: 64bit ncq sntf pm led clo only pio slum part ems deso sadm sds apst 
[    4.383314] memtrack::init_module done.
[    4.384547] megasas: 07.713.01.00-rc1
[    4.385379] Compat-mlnx-ofed backport release: 77239cf
[    4.385380] Backport based on https://:@git-nbu.nvidia.com/r/a/mlnx_ofed/mlnx-ofa_kernel-4.0.git 77239cf
[    4.385381] compat.git: https://:@git-nbu.nvidia.com/r/a/mlnx_ofed/mlnx-ofa_kernel-4.0.git
[    4.398959] scsi host0: ahci
[    4.399187] scsi host1: ahci
[    4.399229] ata1: SATA max UDMA/133 abar m524288@0x93080000 port 0x93080100 irq 132
[    4.399231] ata2: SATA max UDMA/133 abar m524288@0x93080000 port 0x93080180 irq 132
[    4.412034] tg3.c:v3.137 (May 11, 2014)
[    4.412618] ahci 0000:00:17.0: AHCI 0001.0301 32 slots 8 ports 6 Gbps 0xff impl SATA mode
[    4.412620] ahci 0000:00:17.0: flags: 64bit ncq sntf pm led clo only pio slum part ems deso sadm sds apst 
[    4.412760] megaraid_sas 0000:c3:00.0: BAR:0x0  BAR's base_addr(phys):0x00000000e6800000  mapped virt_addr:0x000000006221ebcb
[    4.412762] megaraid_sas 0000:c3:00.0: FW now in Ready state
[    4.412764] megaraid_sas 0000:c3:00.0: 63 bit DMA mask and 63 bit consistent mask
[    4.413207] megaraid_sas 0000:c3:00.0: firmware supports msix	: (128)
[    4.413528] megaraid_sas 0000:c3:00.0: requested/available msix 33/33
[    4.413530] megaraid_sas 0000:c3:00.0: current msix/online cpus	: (33/32)
[    4.413531] megaraid_sas 0000:c3:00.0: RDPQ mode	: (enabled)
[    4.413533] megaraid_sas 0000:c3:00.0: Current firmware supports maximum commands: 1517	 LDIO threshold: 0
[    4.414338] megaraid_sas 0000:c3:00.0: Configured max firmware commands: 1516
[    4.422650] megaraid_sas 0000:c3:00.0: Performance mode :Latency
[    4.422651] megaraid_sas 0000:c3:00.0: FW supports sync cache	: Yes
[    4.422655] megaraid_sas 0000:c3:00.0: megasas_disable_intr_fusion is called outbound_intr_mask:0x40000009
[    4.430537] tg3 0000:04:00.0 eth0: Tigon3 [partno(BCM95720) rev 5720000] (PCI Express) MAC address 08:92:04:a5:a4:7a
[    4.430539] tg3 0000:04:00.0 eth0: attached PHY is 5720C (10/100/1000Base-T Ethernet) (WireSpeed[1], EEE[1])
[    4.430540] tg3 0000:04:00.0 eth0: RXcsums[1] LinkChgREG[0] MIirq[0] ASF[1] TSOcap[1]
[    4.430541] tg3 0000:04:00.0 eth0: dma_rwctrl[00000001] dma_mask[64-bit]
[    4.450388] tg3 0000:04:00.1 eth1: Tigon3 [partno(BCM95720) rev 5720000] (PCI Express) MAC address 08:92:04:a5:a4:7b
[    4.450390] tg3 0000:04:00.1 eth1: attached PHY is 5720C (10/100/1000Base-T Ethernet) (WireSpeed[1], EEE[1])
[    4.450391] tg3 0000:04:00.1 eth1: RXcsums[1] LinkChgREG[0] MIirq[0] ASF[1] TSOcap[1]
[    4.450391] tg3 0000:04:00.1 eth1: dma_rwctrl[00000001] dma_mask[64-bit]
[    4.451637] tg3 0000:04:00.1 eno8403: renamed from eth1
[    4.456435] mlx5_core: unknown parameter 'log_num_mgm_entry_size' ignored
[    4.456436] mlx5_core: unknown parameter '=' ignored
[    4.456436] mlx5_core: unknown parameter '-1' ignored
[    4.460225] mlx5_core 0000:18:00.0: firmware version: 16.35.4506
[    4.460259] mlx5_core 0000:18:00.0: 126.016 Gb/s available PCIe bandwidth (8 GT/s x16 link)
[    4.503405] scsi host3: ahci
[    4.503472] scsi host4: ahci
[    4.503532] scsi host5: ahci
[    4.503648] scsi host6: ahci
[    4.503707] scsi host7: ahci
[    4.503764] scsi host8: ahci
[    4.503828] scsi host9: ahci
[    4.503887] scsi host10: ahci
[    4.503914] ata3: SATA max UDMA/133 abar m524288@0x93000000 port 0x93000100 irq 133
[    4.503916] ata4: SATA max UDMA/133 abar m524288@0x93000000 port 0x93000180 irq 133
[    4.503917] ata5: SATA max UDMA/133 abar m524288@0x93000000 port 0x93000200 irq 133
[    4.503918] ata6: SATA max UDMA/133 abar m524288@0x93000000 port 0x93000280 irq 133
[    4.503919] ata7: SATA max UDMA/133 abar m524288@0x93000000 port 0x93000300 irq 133
[    4.503920] ata8: SATA max UDMA/133 abar m524288@0x93000000 port 0x93000380 irq 133
[    4.503922] ata9: SATA max UDMA/133 abar m524288@0x93000000 port 0x93000400 irq 133
[    4.503923] ata10: SATA max UDMA/133 abar m524288@0x93000000 port 0x93000480 irq 133
[    4.554909] usb 1-11: new high-speed USB device number 2 using xhci_hcd
[    4.603006] tg3 0000:04:00.0 eno8303: renamed from eth0
[    4.704445] usb 1-11: New USB device found, idVendor=7392, idProduct=7822, bcdDevice= 2.00
[    4.704446] usb 1-11: New USB device strings: Mfr=1, Product=2, SerialNumber=3
[    4.704446] usb 1-11: Product: 802.11n WLAN Adapter
[    4.704447] usb 1-11: Manufacturer: Realtek
[    4.704448] usb 1-11: SerialNumber: 00e04c000001
[    4.713046] ata2: SATA link down (SStatus 4 SControl 300)
[    4.713063] ata1: SATA link down (SStatus 0 SControl 300)
[    4.817021] ata10: SATA link down (SStatus 0 SControl 300)
[    4.817038] ata4: SATA link down (SStatus 0 SControl 300)
[    4.817054] ata6: SATA link down (SStatus 0 SControl 300)
[    4.817070] ata3: SATA link down (SStatus 0 SControl 300)
[    4.817087] ata9: SATA link down (SStatus 0 SControl 300)
[    4.817103] ata7: SATA link down (SStatus 0 SControl 300)
[    4.817118] ata5: SATA link down (SStatus 0 SControl 300)
[    4.817134] ata8: SATA link down (SStatus 0 SControl 300)
[    4.830909] usb 1-12: new high-speed USB device number 3 using xhci_hcd
[    4.878517] mlx5_core 0000:18:00.0: Rate limit: 127 rates are supported, range: 0Mbps to 97656Mbps
[    4.878681] mlx5_core 0000:18:00.0: E-Switch: Total vports 2, per vport: max uc(128) max mc(2048)
[    4.883065] mlx5_core 0000:18:00.0: Port module event: module 0, Cable plugged
[    4.883324] mlx5_core 0000:18:00.0: mlx5_pcie_event:304:(pid 448): PCIe slot advertised sufficient power (75W).
[    4.893193] mlx5_core 0000:18:00.0: MLX5E: StrdRq(1) RqSz(8) StrdSz(2048) RxCqeCmprss(0 basic)
[    4.980214] usb 1-12: New USB device found, idVendor=05c6, idProduct=90b3, bcdDevice=ff.ff
[    4.980215] usb 1-12: New USB device strings: Mfr=1, Product=2, SerialNumber=3
[    4.980216] usb 1-12: Product: Android
[    4.980216] usb 1-12: Manufacturer: Android
[    4.980217] usb 1-12: SerialNumber: 5902a096
[    4.990916] megaraid_sas 0000:c3:00.0: FW provided supportMaxExtLDs: 0	max_lds: 32
[    4.990919] megaraid_sas 0000:c3:00.0: controller type	: iMR(0MB)
[    4.990921] megaraid_sas 0000:c3:00.0: Online Controller Reset(OCR)	: Enabled
[    4.990923] megaraid_sas 0000:c3:00.0: Secure JBOD support	: No
[    4.990925] megaraid_sas 0000:c3:00.0: NVMe passthru support	: Yes
[    4.990928] megaraid_sas 0000:c3:00.0: FW provided TM TaskAbort/Reset timeout	: 6 secs/60 secs
[    4.990930] megaraid_sas 0000:c3:00.0: JBOD sequence map support	: Yes
[    4.990931] megaraid_sas 0000:c3:00.0: PCI Lane Margining support	: No
[    5.023011] megaraid_sas 0000:c3:00.0: NVME page size	: (4096)
[    5.023193] megaraid_sas 0000:c3:00.0: megasas_enable_intr_fusion is called outbound_intr_mask:0x40000000
[    5.023193] megaraid_sas 0000:c3:00.0: INIT adapter done
[    5.038954] tsc: Refined TSC clocksource calibration: 2394.382 MHz
[    5.038974] clocksource: tsc: mask: 0xffffffffffffffff max_cycles: 0x22837c6a932, max_idle_ns: 440795328211 ns
[    5.039056] clocksource: Switched to clocksource tsc
[    5.085268] megaraid_sas 0000:c3:00.0: Snap dump wait time	: 25
[    5.085270] megaraid_sas 0000:c3:00.0: pci id		: (0x1000)/(0x0015)/(0x1028)/(0x1f3d)
[    5.085270] megaraid_sas 0000:c3:00.0: unevenspan support	: yes
[    5.085271] megaraid_sas 0000:c3:00.0: firmware crash dump	: no
[    5.085271] megaraid_sas 0000:c3:00.0: JBOD sequence map	: enabled
[    5.085406] scsi host2: Avago SAS based MegaRAID driver
[    5.089816] mlx5_core 0000:18:00.1: firmware version: 16.35.4506
[    5.089848] mlx5_core 0000:18:00.1: 126.016 Gb/s available PCIe bandwidth (8 GT/s x16 link)
[    5.090279] scsi 2:0:8:0: Direct-Access     SEAGATE  DL2400MM0159     ST5A PQ: 0 ANSI: 6
[    5.106909] usb 1-14: new high-speed USB device number 4 using xhci_hcd
[    5.201203] scsi 2:0:11:0: Direct-Access     ATA      MZ7LH480HBHQ0D3  HG58 PQ: 0 ANSI: 6
[    5.244313] scsi 2:2:0:0: Direct-Access     DELL     PERC H345 Front  5.16 PQ: 0 ANSI: 5
[    5.254319] sd 2:0:8:0: Attached scsi generic sg0 type 0
[    5.254387] scsi 2:0:11:0: Attached scsi generic sg1 type 0
[    5.254451] scsi 2:2:0:0: Attached scsi generic sg2 type 0
[    5.255002] usb 1-14: New USB device found, idVendor=1604, idProduct=10c0, bcdDevice= 0.00
[    5.255003] usb 1-14: New USB device strings: Mfr=0, Product=0, SerialNumber=0
[    5.255547] hub 1-14:1.0: USB hub found
[    5.255566] hub 1-14:1.0: 4 ports detected
[    5.255647] sd 2:0:8:0: [sda] Disabling DIF Type 2 protection
[    5.296762] sd 2:0:8:0: [sda] 4688430768 512-byte logical blocks: (2.40 TB/2.18 TiB)
[    5.296764] sd 2:0:8:0: [sda] 4096-byte physical blocks
[    5.297696] sd 2:0:8:0: [sda] Write Protect is off
[    5.297697] sd 2:0:8:0: [sda] Mode Sense: df 00 10 08
[    5.299479] sd 2:0:8:0: [sda] Write cache: disabled, read cache: enabled, supports DPO and FUA
[    5.373078] sd 2:2:0:0: [sdc] 4687134720 512-byte logical blocks: (2.40 TB/2.18 TiB)
[    5.410530] sd 2:2:0:0: [sdc] Write Protect is off
[    5.410530] sd 2:2:0:0: [sdc] Mode Sense: 1f 00 10 08
[    5.449804] sd 2:2:0:0: [sdc] Write cache: disabled, read cache: disabled, supports DPO and FUA
[    5.489545] sd 2:2:0:0: [sdc] Optimal transfer size 65536 bytes
[    5.527071] mlx5_core 0000:18:00.1: Rate limit: 127 rates are supported, range: 0Mbps to 97656Mbps
[    5.527235] mlx5_core 0000:18:00.1: E-Switch: Total vports 2, per vport: max uc(128) max mc(2048)
[    5.531859] mlx5_core 0000:18:00.1: Port module event: module 1, Cable plugged
[    5.532139] mlx5_core 0000:18:00.1: mlx5_pcie_event:304:(pid 447): PCIe slot advertised sufficient power (75W).
[    5.541917] mlx5_core 0000:18:00.1: MLX5E: StrdRq(1) RqSz(8) StrdSz(2048) RxCqeCmprss(0 basic)
[    5.724346] sd 2:0:11:0: [sdb] 937703088 512-byte logical blocks: (480 GB/447 GiB)
[    5.724347] sd 2:0:11:0: [sdb] 4096-byte physical blocks
[    5.737066]  sda: sda1 sda2 sda3
[    5.738946] mlx5_core 0000:18:00.0 ens1f0np0: renamed from eth0
[    5.767530] mlx5_core 0000:18:00.1 ens1f1np1: renamed from eth1
[    5.802935]  sdc: sdc1 sdc2
[    5.840210] sd 2:0:11:0: [sdb] Write Protect is off
[    5.840215] sd 2:0:11:0: [sdb] Mode Sense: 6b 00 10 08
[    5.962939] usb 1-14.1: new high-speed USB device number 5 using xhci_hcd
[    5.996096] sd 2:0:11:0: [sdb] Write cache: enabled, read cache: enabled, supports DPO and FUA
[    6.035384] sd 2:0:8:0: [sda] Attached SCSI disk
[    6.063075] usb 1-14.1: New USB device found, idVendor=1604, idProduct=10c0, bcdDevice= 0.00
[    6.063077] usb 1-14.1: New USB device strings: Mfr=0, Product=0, SerialNumber=0
[    6.063764] hub 1-14.1:1.0: USB hub found
[    6.063788] hub 1-14.1:1.0: 4 ports detected
[    6.142939] usb 1-14.2: new high-speed USB device number 6 using xhci_hcd
[    6.148711] sd 2:2:0:0: [sdc] Attached SCSI disk
[    6.233816]  sdb: sdb1
[    6.255895] usb 1-14.2: New USB device found, idVendor=413c, idProduct=0006, bcdDevice= 0.00
[    6.255899] usb 1-14.2: New USB device strings: Mfr=1, Product=2, SerialNumber=3
[    6.255902] usb 1-14.2: Product: DRAC 5 Virtual Keyboard and Mouse
[    6.255905] usb 1-14.2: Manufacturer: DELLEMC
[    6.255906] usb 1-14.2: SerialNumber: DELL413C-1
[    6.266190] hidraw: raw HID events driver (C) Jiri Kosina
[    6.276589] usbcore: registered new interface driver usbhid
[    6.276591] usbhid: USB HID core driver
[    6.278592] input: DELLEMC DRAC 5 Virtual Keyboard and Mouse as /devices/pci0000:00/0000:00:14.0/usb1/1-14/1-14.2/1-14.2:1.0/0003:413C:0006.0001/input/input1
[    6.278653] hid-generic 0003:413C:0006.0001: input,hidraw0: USB HID v1.01 Mouse [DELLEMC DRAC 5 Virtual Keyboard and Mouse] on usb-0000:00:14.0-14.2/input0
[    6.278724] input: DELLEMC DRAC 5 Virtual Keyboard and Mouse as /devices/pci0000:00/0000:00:14.0/usb1/1-14/1-14.2/1-14.2:1.1/0003:413C:0006.0002/input/input2
[    6.335163] hid-generic 0003:413C:0006.0002: input,hidraw1: USB HID v1.01 Keyboard [DELLEMC DRAC 5 Virtual Keyboard and Mouse] on usb-0000:00:14.0-14.2/input1
[    6.342939] usb 1-14.3: new high-speed USB device number 7 using xhci_hcd
[    6.459918] usb 1-14.3: New USB device found, idVendor=413c, idProduct=a102, bcdDevice= 3.16
[    6.459923] usb 1-14.3: New USB device strings: Mfr=1, Product=2, SerialNumber=3
[    6.459925] usb 1-14.3: Product: iDRAC Virtual NIC USB Device
[    6.459928] usb 1-14.3: Manufacturer: Dell(TM)
[    6.459930] usb 1-14.3: SerialNumber: 5678
[    6.542950] usb 1-14.4: new high-speed USB device number 8 using xhci_hcd
[    6.545661] sd 2:0:11:0: [sdb] Attached SCSI disk
[    6.643113] usb 1-14.4: New USB device found, idVendor=1604, idProduct=10c0, bcdDevice= 0.00
[    6.643117] usb 1-14.4: New USB device strings: Mfr=0, Product=0, SerialNumber=0
[    6.644071] hub 1-14.4:1.0: USB hub found
[    6.644099] hub 1-14.4:1.0: 4 ports detected
[   17.249454] EXT4-fs (sdc2): mounted filesystem with ordered data mode. Opts: (null)
[   18.349646] systemd[1]: Inserted module 'autofs4'
[   18.464594] systemd[1]: systemd 245.4-4ubuntu3.24 running in system mode. (+PAM +AUDIT +SELINUX +IMA +APPARMOR +SMACK +SYSVINIT +UTMP +LIBCRYPTSETUP +GCRYPT +GNUTLS +ACL +XZ +LZ4 +SECCOMP +BLKID +ELFUTILS +KMOD +IDN2 -IDN +PCRE2 default-hierarchy=hybrid)
[   18.483009] systemd[1]: Detected architecture x86-64.
[   18.514411] systemd[1]: Set hostname to <dcn3-PowerEdge-R750xs>.
[   19.153821] systemd[1]: Configuration file /run/systemd/system/netplan-ovs-cleanup.service is marked world-inaccessible. This has no effect as configuration data is accessible via APIs without restrictions. Proceeding anyway.
[   19.262838] systemd[1]: /lib/systemd/system/snapd.service:23: Unknown key name 'RestartMode' in section 'Service', ignoring.
[   19.805425] systemd[1]: Created slice system-modprobe.slice.
[   19.805661] systemd[1]: Created slice system-serial\x2dgetty.slice.
[   19.805857] systemd[1]: Created slice system-systemd\x2dfsck.slice.
[   19.806083] systemd[1]: Created slice User and Session Slice.
[   19.806118] systemd[1]: Started Forward Password Requests to Wall Directory Watch.
[   19.806286] systemd[1]: Set up automount Arbitrary Executable File Formats File System Automount Point.
[   19.806314] systemd[1]: Reached target User and Group Name Lookups.
[   19.806323] systemd[1]: Reached target Slices.
[   19.806331] systemd[1]: Reached target Mounting snaps.
[   19.808628] systemd[1]: Listening on RPCbind Server Activation Socket.
[   19.808703] systemd[1]: Listening on Syslog Socket.
[   19.808749] systemd[1]: Listening on fsck to fsckd communication Socket.
[   19.808773] systemd[1]: Listening on initctl Compatibility Named Pipe.
[   19.808856] systemd[1]: Listening on Journal Audit Socket.
[   19.808891] systemd[1]: Listening on Journal Socket (/dev/log).
[   19.808940] systemd[1]: Listening on Journal Socket.
[   19.808984] systemd[1]: Listening on udev Control Socket.
[   19.809010] systemd[1]: Listening on udev Kernel Socket.
[   19.809580] systemd[1]: Mounting Huge Pages File System...
[   19.810193] systemd[1]: Mounting POSIX Message Queue File System...
[   19.810751] systemd[1]: Mounting NFSD configuration filesystem...
[   19.811326] systemd[1]: Mounting RPC Pipe File System...
[   19.812124] systemd[1]: Mounting Kernel Debug File System...
[   19.812731] systemd[1]: Mounting Kernel Trace File System...
[   19.813665] systemd[1]: Starting Journal Service...
[   19.813728] systemd[1]: Condition check resulted in Kernel Module supporting RPCSEC_GSS being skipped.
[   19.814402] systemd[1]: Starting Set the console keyboard layout...
[   19.815099] systemd[1]: Starting Create list of static device nodes for the current kernel...
[   19.815692] systemd[1]: Starting Load Kernel Module chromeos_pstore...
[   19.816223] systemd[1]: Starting Load Kernel Module drm...
[   19.816749] systemd[1]: Starting Load Kernel Module efi_pstore...
[   19.817301] systemd[1]: Starting Load Kernel Module pstore_blk...
[   19.817831] systemd[1]: Starting Load Kernel Module pstore_zone...
[   19.818400] systemd[1]: Starting Load Kernel Module ramoops...
[   19.827858] systemd[1]: Condition check resulted in Set Up Additional Binary Formats being skipped.
[   19.827883] systemd[1]: Condition check resulted in File System Check on Root Device being skipped.
[   19.834247] systemd[1]: Starting Load Kernel Modules...
[   19.835018] systemd[1]: Starting Remount Root and Kernel File Systems...
[   19.835630] systemd[1]: Starting udev Coldplug all Devices...
[   19.836234] systemd[1]: Starting Uncomplicated firewall...
[   19.836808] systemd[1]: Started Read required files in advance.
[   19.837837] systemd[1]: Mounted Huge Pages File System.
[   19.837905] systemd[1]: Mounted POSIX Message Queue File System.
[   19.837969] systemd[1]: Mounted Kernel Debug File System.
[   19.838019] systemd[1]: Mounted Kernel Trace File System.
[   19.866638] systemd[1]: Finished Uncomplicated firewall.
[   19.873660] systemd[1]: Finished Create list of static device nodes for the current kernel.
[   19.878005] EXT4-fs (sdc2): re-mounted. Opts: errors=remount-ro
[   19.878965] systemd[1]: Finished Remount Root and Kernel File Systems.
[   19.879576] systemd[1]: Activating swap /swapfile...
[   19.879935] systemd[1]: Condition check resulted in Rebuild Hardware Database being skipped.
[   19.880593] systemd[1]: Starting Load/Save Random Seed...
[   19.881243] systemd[1]: Starting Create System Users...
[   19.895402] systemd[1]: modprobe@pstore_zone.service: Succeeded.
[   19.895592] systemd[1]: Finished Load Kernel Module pstore_zone.
[   19.895756] systemd[1]: modprobe@pstore_blk.service: Succeeded.
[   19.895933] systemd[1]: Finished Load Kernel Module pstore_blk.
[   19.909957] systemd[1]: Finished udev Coldplug all Devices.
[   19.933105] systemd[1]: Starting Helper to synchronize boot up for ifupdown...
[   19.940238] systemd[1]: Started Journal Service.
[   19.965282] pstore: ignoring unexpected backend 'efi'
[   20.030933] Adding 2097148k swap on /swapfile.  Priority:-2 extents:6 across:2260988k FS
[   20.046613] systemd-journald[559]: Received client request to flush runtime journal.
[   20.128165] RPC: Registered named UNIX socket transport module.
[   20.128166] RPC: Registered udp transport module.
[   20.128166] RPC: Registered tcp transport module.
[   20.128167] RPC: Registered tcp NFSv4.1 backchannel transport module.
[   20.225149] lp: driver loaded but no devices found
[   20.228815] ppdev: user-space parallel port driver
[   20.320463] Installing knfsd (copyright (C) 1996 okir@monad.swb.de).
[   20.362818] systemd-journald[559]: File /var/log/journal/525c332dae86497483246fcc16cb0ddd/system.journal corrupted or uncleanly shut down, renaming and replacing.
[   22.857542] ACPI Error: No handler for Region [SYSI] (00000000ba08a7a2) [IPMI] (20190816/evregion-129)
[   22.866891] ACPI Error: Region IPMI (ID=7) has no handler (20190816/exfldio-261)
[   22.874302] No Local Variables are initialized for Method [_GHL]
[   22.874303] No Arguments are initialized for method [_GHL]
[   22.874306] ACPI Error: Aborting method \_SB.PMI0._GHL due to previous error (AE_NOT_EXIST) (20190816/psparse-529)
[   22.884714] ACPI Error: Aborting method \_SB.PMI0._PMC due to previous error (AE_NOT_EXIST) (20190816/psparse-529)
[   22.895068] ACPI Error: AE_NOT_EXIST, Evaluating _PMC (20190816/power_meter-743)
[   22.902532] IPMI message handler: version 39.2
[   22.913931] fbcon: Taking over console
[   22.913979] Console: switching to colour frame buffer device 128x48
[   22.996477] ipmi device interface
[   23.028988] ipmi_si: IPMI System Interface driver
[   23.029012] ipmi_si dmi-ipmi-si.0: ipmi_platform: probing via SMBIOS
[   23.029014] ipmi_platform: ipmi_si: SMBIOS: io 0xca8 regsize 1 spacing 4 irq 10
[   23.029015] ipmi_si: Adding SMBIOS-specified kcs state machine
[   23.029101] ipmi_si IPI0001:00: ipmi_platform: probing via ACPI
[   23.029123] ipmi_si IPI0001:00: ipmi_platform: [io  0x0ca8] regsize 1 spacing 4 irq 10
[   23.029124] ipmi_si dmi-ipmi-si.0: Removing SMBIOS-specified kcs state machine in favor of ACPI
[   23.029125] ipmi_si: Adding ACPI-specified kcs state machine
[   23.029208] ipmi_si: Trying ACPI-specified kcs state machine at i/o address 0xca8, slave address 0x20, irq 10
[   23.091746] cryptd: max_cpu_qlen set to 1000
[   23.091789] dcdbas dcdbas: Dell Systems Management Base Driver (version 5.6.0-3.3)
[   23.126592] AVX2 version of gcm_enc/dec engaged.
[   23.126593] AES CTR mode by8 optimization enabled
[   23.422931] ipmi_si IPI0001:00: The BMC does not support setting the recv irq bit, compensating, but the BMC needs to be fixed.
[   23.491010] ipmi_si IPI0001:00: Using irq 10
[   23.527843] ipmi_si IPI0001:00: IPMI message handler: Found new BMC (man_id: 0x0002a2, prod_id: 0x0100, dev_id: 0x20)
[   23.605080] ipmi_si IPI0001:00: IPMI kcs interface initialized
[   23.718504] ipmi_ssif: IPMI SSIF Interface driver
[   24.407489] intel_rapl_common: Found RAPL domain package
[   24.407498] intel_rapl_common: Found RAPL domain dram
[   24.407501] intel_rapl_common: DRAM domain energy unit 15300pj
[   24.486278] cdc_ether 1-14.3:1.0 eth0: register 'cdc_ether' at usb-0000:00:14.0-14.3, CDC Ethernet Device, 08:92:04:a5:a4:77
[   24.486296] usbcore: registered new interface driver cdc_ether
[   24.487657] cdc_ether 1-14.3:1.0 idrac: renamed from eth0
[   24.586095] rndis_host 1-12:1.0 usb0: register 'rndis_host' at usb-0000:00:14.0-12, RNDIS device, 12:44:06:1b:e0:29
[   24.586155] usbcore: registered new interface driver rndis_host
[   24.603411] cfg80211: Loading compiled-in X.509 certificates for regulatory database
[   24.603572] cfg80211: Loaded X.509 cert 'sforshee: 00b28ddf47aef9cea7'
[   25.141123] mgag200 0000:03:00.0: remove_conflicting_pci_framebuffers: bar 0: 0x91000000 -> 0x91ffffff
[   25.141125] mgag200 0000:03:00.0: remove_conflicting_pci_framebuffers: bar 1: 0x92808000 -> 0x9280bfff
[   25.141125] mgag200 0000:03:00.0: remove_conflicting_pci_framebuffers: bar 2: 0x92000000 -> 0x927fffff
[   25.141127] checking generic (91000000 300000) vs hw (91000000 1000000)
[   25.141127] fb0: switching to mgag200drmfb from EFI VGA
[   25.141233] Console: switching to colour dummy device 80x25
[   25.141244] mgag200 0000:03:00.0: vgaarb: deactivate vga console
[   25.145910] [TTM] Zone  kernel: Available graphics memory: 65652244 KiB
[   25.145911] [TTM] Zone   dma32: Available graphics memory: 2097152 KiB
[   25.145911] [TTM] Initializing pool allocator
[   25.145914] [TTM] Initializing DMA pool allocator
[   25.161117] rtl8192cu: Chip version 0x11
[   25.172475] fbcon: mgag200drmfb (fb0) is primary device
[   25.192730] rtl8192cu: Board Type 0
[   25.192808] rtl_usb: rx_max_size 15360, rx_urb_num 8, in_ep 1
[   25.192822] rtl8192cu: Loading firmware rtlwifi/rtl8192cufw_TMSC.bin
[   25.192865] ieee80211 phy0: Selected rate control algorithm 'rtl_rc'
[   25.193140] usbcore: registered new interface driver rtl8192cu
[   25.341900] cfg80211: loaded regulatory.db is malformed or signature is missing/invalid
[   25.538936] Console: switching to colour frame buffer device 128x48
[   25.539771] mgag200 0000:03:00.0: fb0: mgag200drmfb frame buffer device
[   25.559437] usbcore: registered new interface driver rtl8xxxu
[   25.561008] rtl8192cu 1-11:1.0 wlx08beac3d3a3f: renamed from wlan0
[   25.578942] [drm] Initialized mgag200 1.0.0 20110418 for 0000:03:00.0 on minor 0
[   25.652125] ib_uverbs init
[   29.564181] audit: type=1400 audit(1747468579.812:2): apparmor="STATUS" operation="profile_load" profile="unconfined" name="/usr/bin/lxc-start" pid=1188 comm="apparmor_parser"
[   29.564202] audit: type=1400 audit(1747468579.812:3): apparmor="STATUS" operation="profile_load" profile="unconfined" name="libreoffice-senddoc" pid=1166 comm="apparmor_parser"
[   29.564402] audit: type=1400 audit(1747468579.812:4): apparmor="STATUS" operation="profile_load" profile="unconfined" name="libreoffice-xpdfimport" pid=1171 comm="apparmor_parser"
[   29.564974] audit: type=1400 audit(1747468579.812:5): apparmor="STATUS" operation="profile_load" profile="unconfined" name="/usr/sbin/tcpdump" pid=1165 comm="apparmor_parser"
[   29.566232] audit: type=1400 audit(1747468579.812:6): apparmor="STATUS" operation="profile_load" profile="unconfined" name="lxc-container-default" pid=1169 comm="apparmor_parser"
[   29.566234] audit: type=1400 audit(1747468579.812:7): apparmor="STATUS" operation="profile_load" profile="unconfined" name="lxc-container-default-cgns" pid=1169 comm="apparmor_parser"
[   29.566235] audit: type=1400 audit(1747468579.812:8): apparmor="STATUS" operation="profile_load" profile="unconfined" name="lxc-container-default-with-mounting" pid=1169 comm="apparmor_parser"
[   29.566237] audit: type=1400 audit(1747468579.812:9): apparmor="STATUS" operation="profile_load" profile="unconfined" name="lxc-container-default-with-nesting" pid=1169 comm="apparmor_parser"
[   29.597353] audit: type=1400 audit(1747468579.844:10): apparmor="STATUS" operation="profile_load" profile="unconfined" name="/usr/lib/cups/backend/cups-pdf" pid=1175 comm="apparmor_parser"
[   29.597355] audit: type=1400 audit(1747468579.844:11): apparmor="STATUS" operation="profile_load" profile="unconfined" name="/usr/sbin/cupsd" pid=1175 comm="apparmor_parser"
[   30.955127] new mount options do not match the existing superblock, will be ignored
[   33.061374] NFSD: Using UMH upcall client tracking operations.
[   33.061376] NFSD: starting 90-second grace period (net f00000c0)
[   33.192990] mlx5_core 0000:18:00.0 ens1f0np0: Link up
[   33.196212] IPv6: ADDRCONF(NETDEV_CHANGE): ens1f0np0: link becomes ready
[   33.516047] aufs 5.4.3-20200302
[   33.533925] mlx5_core 0000:18:00.1 ens1f1np1: Link up
[   33.551407] rtl8192cu: MAC auto ON okay!
[   33.561929] rtl8192cu: Tx queue select: 0x05
[   34.198286] rtl8192c_common: Polling FW ready fail! REG_MCUFWDL:0x00030006.
[   34.205246] rtl8192c_common: Firmware is not ready to run!
[   34.554160] IPv6: ADDRCONF(NETDEV_CHANGE): ens1f1np1: link becomes ready
[   35.648058] mgag200 0000:03:00.0: Video card doesn't support cursors with partial transparency.
[   35.648060] mgag200 0000:03:00.0: Not enabling hardware cursor.
[   35.672350] wlx08beac3d3a3f: authenticate with 5c:a0:00:2e:2a:01
[   35.683387] wlx08beac3d3a3f: send auth to 5c:a0:00:2e:2a:01 (try 1/3)
[   35.727560] wlx08beac3d3a3f: authenticated
[   35.730942] wlx08beac3d3a3f: associate with 5c:a0:00:2e:2a:01 (try 1/3)
[   35.788631] wlx08beac3d3a3f: RX AssocResp from 5c:a0:00:2e:2a:01 (capab=0x8431 status=0 aid=9)
[   35.789344] wlx08beac3d3a3f: associated
[   35.841972] IPv6: ADDRCONF(NETDEV_CHANGE): wlx08beac3d3a3f: link becomes ready
[   36.346432] tg3 0000:04:00.0 eno8303: Link is up at 1000 Mbps, full duplex
[   36.346452] tg3 0000:04:00.0 eno8303: Flow control is off for TX and off for RX
[   36.346457] tg3 0000:04:00.0 eno8303: EEE is enabled
[   36.346484] IPv6: ADDRCONF(NETDEV_CHANGE): eno8303: link becomes ready
[   36.400202] tg3 0000:04:00.1 eno8403: Link is up at 1000 Mbps, full duplex
[   36.400220] tg3 0000:04:00.1 eno8403: Flow control is off for TX and off for RX
[   36.400232] tg3 0000:04:00.1 eno8403: EEE is enabled
[   36.400254] IPv6: ADDRCONF(NETDEV_CHANGE): eno8403: link becomes ready
[   37.833500] rfkill: input handler disabled
[   62.134242] bpfilter: Loaded bpfilter_umh pid 2393
[   62.134802] Started bpfilter
[   62.375036] bridge: filtering via arp/ip/ip6tables is no longer available by default. Update your scripts to load br_netfilter if you need this.
[   62.618427] kauditd_printk_skb: 68 callbacks suppressed
[   62.618430] audit: type=1400 audit(1747468613.356:80): apparmor="STATUS" operation="profile_replace" info="same as current profile, skipping" profile="unconfined" name="/usr/bin/lxc-start" pid=2443 comm="apparmor_parser"
[   62.641317] audit: type=1400 audit(1747468613.380:81): apparmor="STATUS" operation="profile_replace" info="same as current profile, skipping" profile="unconfined" name="lxc-container-default" pid=2447 comm="apparmor_parser"
[   62.641325] audit: type=1400 audit(1747468613.380:82): apparmor="STATUS" operation="profile_replace" info="same as current profile, skipping" profile="unconfined" name="lxc-container-default-cgns" pid=2447 comm="apparmor_parser"
[   62.641330] audit: type=1400 audit(1747468613.380:83): apparmor="STATUS" operation="profile_replace" info="same as current profile, skipping" profile="unconfined" name="lxc-container-default-with-mounting" pid=2447 comm="apparmor_parser"
[   62.641335] audit: type=1400 audit(1747468613.380:84): apparmor="STATUS" operation="profile_replace" info="same as current profile, skipping" profile="unconfined" name="lxc-container-default-with-nesting" pid=2447 comm="apparmor_parser"
[   64.967569] audit: type=1400 audit(1747468615.708:85): apparmor="STATUS" operation="profile_load" profile="unconfined" name="docker-default" pid=2524 comm="apparmor_parser"
[   70.566006] Bridge firewalling registered
[   70.762114] Initializing XFRM netlink socket
