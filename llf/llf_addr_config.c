#include "lib_basetype.h"
#include "setup.h"
#include "llf_public.h"
#include "memory.h"



const unsigned int ulLlfAddrSigHead __attribute__((section(".llf_addr_sig_head"))) = SIGNATURE_END;
const unsigned int ulLlfConfigAddr __attribute__((section(".llf_config_address"))) =
    CONFIG_BASE_VA_ADDR;
const unsigned int ulLlfRamAddr __attribute__((section(".llf_ram_address"))) = IMEM1_RAM_BASE;
const unsigned int ulLlfFcTableAddr __attribute__((section(".llf_fctable_address"))) =
    PARSER_TABLE_VA_ADDR;
const unsigned int ulLlfWsFwAddr __attribute__((section(".llf_wsfw_address"))) = FW_CODE_ADDR;
const unsigned int ulLlfAddrSigTail __attribute__((section(".llf_addr_sig_tail"))) = SIGNATURE_END;



