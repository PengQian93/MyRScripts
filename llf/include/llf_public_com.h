#ifndef  _LLF_PUBLIC_COM_H_
#define  _LLF_PUBLIC_COM_H_


#define TAG_RESET			0xf0
#define TAG_READ_ID			0xf1
#define TAG_READ            0xf2
#define TAG_MULTIREAD       0xf3
#define TAG_READ_CACHE      0xf4
#define TAG_WRITE           0xfa
#define TAG_WRITE_CACHE     0xf5
#define TAG_ERASE           0xf6
#define TAG_POLLING_STATUS	0xf7
#define TAG_SETFEATURE		0xf8
#define TAG_GETFEATURE      0xf9
#define TAG_DEFECT_READ		0xfa
#define TAG_READ_PARA		0xfb


#define OP_TAG_MASK                 0xff00
#define BANK_NO_MASK                0x000f
#define GROUP_NO_MASK               0xfff0
#define BE_CH_ERASE_STATUS          ((1<<NandPara.ubChNum) - 1)
#define BE_COMPLETION_ERROR_MASK    0xffb
#define HEADER_DMA_MAX_LEN          16  // 16*4=64
#define HEADER_CRC_OFFSET           (L2pPara.ubHdr4BLenPerPage*4 - 4)
#define HEADER_MAX_LEN              (HEADER_DMA_MAX_LEN * 4)

#define LLF_HEAD_SIZE               (64)
#define LLF_DATA_SIZE               (16 * 1024)


//**********for LLF flow************/
// LLF mode
#define LLF_INHERIT_OR_FORMAT        0
#define LLF_FORCE_INHERIT            1
#define LLF_FORCE_FORMAT             2
#define LLF_FORMAT_ERASE_NONDEFECT   3  // read defect mark then do erase
#define LLF_DEFECT_BAD_BLOCK         4
#define LLF_ONLY_UPDATE_FW           9  // backdoor for debug
#define LLF_CHECK_BS                 10 // check block for RMA

//LLF step
#define STEP_CALIBRATE       	0
#define STEP_LOAD_RDT_START  	1
#define STEP_INHERIT_DONE       2
#define STEP_FORMAT_INIT     	3
#define STEP_FORMAT_START     	4
#define STEP_FORMAT_END     	5

// LLF inherit
#define STATIC_SBLK_INFO      1
#define DYNAMIC_SBLK_INFO     2

//LLF calibrate
#define STEP_CALIBRATE_OVER    0
#define STEP_CALIBRATE_INIT    1
#define STEP_START_CALIBRTE    2

//LLF all
#define STEP_LLF_INHERIT       0
#define STEP_LLF_ERASE         1
#define STEP_LLF_EXECUTE       2

//Write Config
#define STEP_WRITE_CONFIG_OVER    0
#define STEP_WRITE_CONFIG_INIT    1
#define STEP_WRITE_CONFIG_ERASE   2

//MST_MERGE
#define STEP_WRITE_CONFIG_MST     1


#if defined(RTS5771_VA) || defined(RTS5771_FPGA)
#define IMEM_REGION_BASE			(0x80100000)
#define FAST_A1_CODE_SIZE			(0xC000)//48KB
//#define IMEM_CODE_PART_BASE         (0x80100000 + 160*1024 - FAST_A1_CODE_SIZE)
//#define IMEM_CODE_SIZE              (0x28000)//160KB
#define DMEM_CODE_PART_BASE         (0x80200000) //A1_part0 code-> 48KB
#define DMEM_CODE_SIZE				(128*1024)
#define IMEM_PART_RAM_BASE          (IMEM_CODE_PART_BASE + 4)
#define A1_CODE_SIZE				(FAST_A1_CODE_SIZE + DMEM_CODE_SIZE)

#define BE_VERSION_TAG        (FW_CODE_ADDR + A1_CODE_SIZE - 8)
#else
#define BE_VERSION_TAG        (FW_CODE_ADDR + MAX_IMEM_CODE_SIZE - 8)
#endif
#define FW_RDT_TAG            (FW_CODE_ADDR + RDT_CODE_SIZE - 8)
#if defined(RL6577_VA) || defined(RTS5771_VA) || defined(RTS5771_FPGA)
#define RDT_CODE_SIZE         (MAX_IMEM_CODE_SIZE)
#elif defined(RL6447_VA)
#define RDT_CODE_SIZE         (MAX_IMEM_CODE_SIZE)
#else
#define RDT_CODE_SIZE         (0x32000) //200KB
#endif

//LLF DBT type
#define LLF_DBT_NONE      0
#define LLF_DBT_INIT      1
#define LLF_DBT_RDT       2
#define LLF_DBT_SYSTEM    3
#define LLF_DBT_FACTORY   4

// llf function name
#define NONE_FUNC             0
#define INIT_FUNC             1
#define GET_DEV_INFO_FUNC     2
#define ERASE_ALL_FUNC        3
#define CALIBRATE_FUNC        4
#define BUILD_ODBT_FUNC       5
#define WRITE_CONFIG_FUNC     6
#define BURNING_ALL_FUNC      7

// LLF Erase mode
#define ERASE_ALL             0
#define ERASE_FW_BLOCK        1
#define ERASE_ALL_WITH_DBT    2

//Check Threshold
#define DEFAULT_DEFECT_RATIO  80   // 8%
#define DEFECT_RATIO_SHIFT    4    //   1/16

#if (defined(RL6577_VA) || defined(RL6643_VA)) && (defined(FTL_B27A) || defined(FTL_B27B) || defined(FTL_N28A) || defined(FTL_SSV6) || defined(FTL_H3DTV7) || defined(FTL_SSV7) || defined(FTL_YX3T_WDS))
#define SNAPSHOT_PAGE_SHIFT_LLF 15
#define SNAPSHOT_BANK_SHIFT 26
#define SNAPSHOT_PAGE_MASK_LLF  0x7FF	// 11 bit
#define SNAPSHOT_BANK_MASK_LLF  0x3F	// 6 bit
#define SNAPSHOT_BLOCK_MASK_LLF 0x7FFF	// 15 bit
#elif (defined(RL6577_VA) || defined(RL6643_VA)) && (defined(FTL_N38A) || defined(FTL_N38B) || defined(FTL_Q5171A))
#define SNAPSHOT_PAGE_SHIFT_LLF 15
#define SNAPSHOT_BANK_SHIFT 27
#define SNAPSHOT_PAGE_MASK_LLF  0xFFF   // 12 bit
#define SNAPSHOT_BANK_MASK_LLF  0x1F    // 5 bit
#define SNAPSHOT_BLOCK_MASK_LLF 0x7FFF  // 15 bit
#else
//for inherit from whole system
#define SNAPSHOT_BLOCK_MASK_LLF		0xFFFF
#define SNAPSHOT_BANK_MASK_LLF	 	0x3F	// 6 bit
#define SNAPSHOT_BANK_SHIFT 		26
#define SNAPSHOT_PAGE_SHIFT_LLF     16
#define SNAPSHOT_PAGE_MASK_LLF      0x3FF	// 10 bit
#endif

#ifndef RL6531_VB
// TODO:: Must same to block_info define
#define BS_INFO_SIZE                0x10    //3DW
#define BAD_INFO_OFFSET             0x4    //3DW
#define PF_INFO_OFFSET             	0x8    //3DW
#define PF_INFO_BIT_SHIFT			30
#define NEW_BAD_INFO_OFFSET			0x8
#define NEW_BAD_INFO_MASK			0x7ffff

#elif defined(RTS5771_FPGA)||defined(RTS5771_VA)
#define BS_INFO_SIZE                (0x20)  //8DW
#define BAD_INFO_OFFSET             (0x8)   //2DW
#define NEW_BAD_INFO_OFFSET         (0x10)  //4DW

#else
#define BS_INFO_SIZE                0xC    //3DW
#define BAD_INFO_OFFSET             0x4    //3DW
#define PF_INFO_OFFSET              0x8    //3DW
#define PF_INFO_BIT_SHIFT           29
#define NEW_BAD_INFO_OFFSET         0x8
#define NEW_BAD_INFO_MASK           0x7ffff
#endif

#define FW_IN_IMEM1        0
#define FW_IN_DRAM         1
#ifdef MST_MERGE
#define FW_FOR_MST         2
#endif

#define MAX_DCACHE_SIZE    0x8000  // 4381: 0x8000 ; 4281: 0x4000
#define MAX_DB_NUM_SIZE    1024

#define PLANE_NUM_MAX		8
#define LUN_NUM_MAX         4

//#define NEW_MUL_WR_CACHE
#define MUL_WR_CACHE_COUNT 200

#define REPLACE_FW
//#define REPLACE_FW_WITH_SBLK_TABLE

//Auto calibrate
#define AUTO_K_ENABLE
//#define AUTO_K_DQS_DQ_FIX90
#define AUTO_CALIBRATE_TX_START  0
#define AUTO_CALIBRATE_RX_START  1
#define AUTO_CALIBRATE_VREF_START  2
#define AUTO_ENHANCE_DRIVING     1
#if defined(RL6577_VA) || defined(RL6447_VA) || defined(RTS5771_VA)
#define AUTO_CALIBRATE_TX_GAP    5
#define AUTO_CALIBRATE_RX_GAP    10
#elif defined(RL6643_VA) || defined(RL6531_VB) || defined(RL6643_FPGA)
#define AUTO_CALIBRATE_TX_GAP    20
#define AUTO_CALIBRATE_RX_GAP    20
#endif

//Save original RDT result and DBT
//#define KEEP_ORIG_DBT
//#define AVG_ERASE_COUNT_TEST
#define LLF_CHECK_SYSTEMBLK_ERASE_FAIL
//#if defined(RL6577_VA) || defined(RL6643_VA) || defined(RL6531_VB)
#if defined(RL6643_VA) || defined(RL6531_VB)
#define FC_FULL_CALIBRATE
#endif
#if defined(RL6643_VA)
#define HANDLE_BLK_INFO_BEYOND_64K
#endif

#ifdef LLF_AUTO_RMA
enum TxMemBlockStatus
{
    TxMemNormal = 0,
    TxMemECC,
    TxMemEmpty
};
enum TxMemPart
{
    TxMemPart1 = 1,
    TxMemPart2
};
enum AutoRMACheckRemap
{
    Remap_Null = 0,
    Remap_Pro,
    Remap_New
};

#define STATIC_SBLK_BLK_UPPER_BOUND 4
#define STATIC_SBLK_BANK_UPPER_BOUND 8
#define NAND_LOG_REC_MAX 256
#define NAND_LOG_LEN 0x1000 //4K
#define SMART_INFO_MAX 19
#define AUTORMA_UART_MEM_MAX_LEN 180
#define SPLBN_UART_BYTES_OFFSET_OLD 0x00900000
#define SPLBN_UART_MAX_LBN_INDEX_OLD 256 //0x00100000
#define SPLBN_UART_BYTES_OFFSET_NEW 0x00100000
#define SPLBN_UART_MAX_LBN_INDEX_NEW 1536 //0x00600000

//print type(SRAM)
#define PRINT_MASK              0xFF
#define PRINT_TLC_ALLPAGE       0x10
#define PRINT_SLC_ALLPAGE       0x11
#define PRINT_TLC_PAGE0         0x20
#define PRINT_SLC_PAGE0         0x21
#define PRINT_TLC_BLOCK         0x30
#define PRINT_SLC_BLOCK         0x31
#define PRINT_REMAP             0x40
#define PRINT_SNAPSHOT          0x41
#define PRINT_NANDLOG           0x42
#define PRINT_UR_FILE           0x50
#define PRINT_IDLE              0x00
#define PRINT_END               0xFF

//print type(level)
#define PRINT_UART              0x01
#define PRINT_MEM               0x02
#define PRINT_BOTH              0x03
#define PRINT_MEM_FLUSH         0x04
#endif

#endif

