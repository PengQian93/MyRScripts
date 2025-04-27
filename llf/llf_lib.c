
#include "setup.h"
#include "system_reg.h"
#include "serial.h"
#include "lib_debug.h"
#include "smp.h"
#include "fc_reg.h"
#include "codebank.h"
#include "spic.h"
#include "memory.h"
#include "scheduler0.h"
#include "fe_misc.h"
#include "timer.h"

#include "platform_pm.h"
#include "platform_global.h"

#include "ddr.h"
#include "lib_cpu.h"
#include "lib_fc_public.h"
#include "lib_retcode.h"
#include "lib_debug.h"
#include "lib_fio_public.h"
#include "lib_phy_public.h"
#include "lib_sblk_config.h"
#include "lib_hlc_com.h"
#include "llf_global.h"
#include "llf_public.h"
#include "llf_public_com.h"
#include "llf_lib_public.h"
#include "llfmp.h"
#include "stdarg.h"

#if (defined(RL6577_VA) && defined(FOR_WEIKE)) || defined(RL6643_VA)
void GPIO_init(U32 gpio_num)
{
    U32 temp;

    _REG32(0xff500c08) = 0;//software control
    temp = _REG32(0xff500c04);
    _REG32(0xff500c04) = temp | gpio_num; //OE
}

void GPIO_toggle(U32 gpio_num)
{
    _REG32(0xff500c00) ^= gpio_num;
}

void GPIO_SetHigh(U32 gpio_num)
{
    _REG32(0xff500c00) |= gpio_num;
}

void GPIO_SetLow(U32 gpio_num)
{

    _REG32(0xff500c00) &= ~gpio_num;
}
#endif

void spi_flash_wait_busy_llf()
{
    int test_reg = 0;
    int test_data = 0;
    U32 tmp_cnt;
    U32 delay_cnt = 0;

    // check SR until SR[0] is 1(SPIC is not busy)
    test_reg =  BA_SPI_FLASH_REG + SPI_SR_OFFSET;

    while(1)
    {
        test_data =  READ_REG_32(test_reg);
        if ((test_data & 0x0020) == 0x0020)
        {
            //    err = 1;
            llfDbgPrintk(SPIC_MSG, "SPI transition is error \r\n");
            break;
        }
        else
        {
            if (!(test_data & 0x01))
            {
                //llfprintk("SPI transition is ok \r\n");
                break;
            }
        }
        //llfprintk("SPIC is  busy!!! \r\n");

#if defined(RL6577_FPGA) || defined(RTS5771_FPGA) || defined(RL6643_FPGA)
        //delay about 1.23ms in cpu clk of 20MHz
        for( tmp_cnt = 0; tmp_cnt < 0x1fff; tmp_cnt++ )
        {
            __asm__ __volatile__ (
                "nop;"
                :::"ra"
            );

        }
#else
        //delay about 0.56ms in cpu clk of 700MHz
        for( tmp_cnt = 0; tmp_cnt < 0x1ffff; tmp_cnt++ )
        {
            __asm__ __volatile__ (
                "nop;"
                :::"ra"
            );

        }
#endif
        delay_cnt += 1;
        if ( delay_cnt > 18000 )
        {
            break;
        }
    }
}

void flash_rx_cmd_llf(int cmd)
{
    int test_reg = 0;
    int test_data = 0;

    // SPIC set SSIENR = 0
    test_reg =  BA_SPI_FLASH_REG + SPI_SSIENR_OFFSET;
    test_data =  0x00;
    WRITE_REG_32(test_reg, test_data);
    // SPIC set CTRLR0 (SIO,RX mode)
    test_reg =  BA_SPI_FLASH_REG + SPI_CTRLR0_OFFSET;
    test_data = (READ_REG_32(test_reg) & 0XFFF0FFFF) | 0x00000300;
    WRITE_REG_32(test_reg, test_data);
    // SPIC set DR0
    test_reg =  BA_SPI_FLASH_REG + SPI_DR0_OFFSET;
    test_data =  cmd;
    WRITE_REG_8(test_reg, test_data);
    // SPIC set SSIENR = 1(enable)
    test_reg =  BA_SPI_FLASH_REG + SPI_SSIENR_OFFSET;
    test_data =  0x01;
    WRITE_REG_32(test_reg, test_data);

    //llfprintk("now wait for spi_flash_busy..\r\n");
    spi_flash_wait_busy_llf();
}

void flash_wait_busy_llf()
{
    int test_reg = 0;
    int test_data = 0;
    U32 tmp_cnt;
    U32 delay_cnt = 0;

    while(1)
    {
        // SPIC set SSIENR = 0
        test_reg =  BA_SPI_FLASH_REG + SPI_SSIENR_OFFSET;
        test_data =  0x00;
        WRITE_REG_32(test_reg, test_data);
        // SPIC set CTRLR1 =1 (one data byte frame)
        test_reg =  BA_SPI_FLASH_REG + SPI_CTRLR1_OFFSET;
        test_data =  0x01;
        WRITE_REG_32(test_reg, test_data);
        // SPIC set AUTO_LENGTH(set dummy cycle)
        test_reg =  BA_SPI_FLASH_REG + SPI_AUTO_LENGTH_OFFSET;
#if defined(RL6577_FPGA) || defined(RTS5771_FPGA) || defined(RL6643_FPGA)
        test_data =  (READ_REG_32(test_reg) & 0xffff0000) | 0x00030000;
#else
#if defined(RL6577_VA)
        test_data =  (READ_REG_32(test_reg) & 0xffff0000) | 0xfc130004;
#elif defined(RTS5771_VA)
        test_data =  (READ_REG_32(test_reg) & 0xf0ff0000) | 0xf0034000;
#else
        test_data =  (READ_REG_32(test_reg) & 0xffff0000) | 0xfc030000;
#endif
#endif
        WRITE_REG_32(test_reg, test_data);

        flash_rx_cmd_llf(0x05); // 05H -> RDSR

        // SPIC read DR0: RDSR
        test_reg =  BA_SPI_FLASH_REG + SPI_DR0_OFFSET;
        test_data = READ_REG_8(test_reg);

        if(!(test_data & 0x1))
        {
            break;
        }
        //llfprintk("flash is busy!!! \r\n");

#if defined(RL6577_FPGA) || defined(RTS5771_FPGA) || defined(RL6643_FPGA)
        //delay about 1.23ms in cpu clk of 20MHz
        for( tmp_cnt = 0; tmp_cnt < 0x1fff; tmp_cnt++ )
        {
            __asm__ __volatile__ (
                "nop;"
                :::"ra"
            );

        }
#else
        //delay about 0.56ms in cpu clk of 700MHz
        for( tmp_cnt = 0; tmp_cnt < 0x1ffff; tmp_cnt++ )
        {
            __asm__ __volatile__ (
                "nop;"
                :::"ra"
            );

        }
#endif
        delay_cnt += 1;
        if ( delay_cnt > 18000 )
        {
            break;
        }
    }
}

void flash_tx_cmd_llf(int cmd)
{
    int test_reg = 0;
    int test_data = 0;

    // SPIC set SSIENR = 0
    test_reg =  BA_SPI_FLASH_REG + SPI_SSIENR_OFFSET;
    test_data =  0x00;
    WRITE_REG_32(test_reg, test_data);
    // SPIC set CTRLR0(TX mode)
    test_reg =  BA_SPI_FLASH_REG + SPI_CTRLR0_OFFSET;
    test_data = (READ_REG_32(test_reg) & 0xfff0fcff);
    WRITE_REG_32(test_reg, test_data);
    // SPIC set DR0(flash cmd)
    test_reg =  BA_SPI_FLASH_REG + SPI_DR0_OFFSET;
    test_data =  cmd;
    WRITE_REG_8(test_reg, test_data);
    // SPIC set SSIENR = 1(enable)
    test_reg =  BA_SPI_FLASH_REG + SPI_SSIENR_OFFSET;
    test_data =  0x01;
    WRITE_REG_32(test_reg, test_data);

    spi_flash_wait_busy_llf();

    flash_wait_busy_llf();
}

void flash_set_status_llf(int flsh_st)
{
    int info;
    int test_reg = 0;
    int test_data = 0;

    // flash_cmd: WREN (0x06)
    flash_tx_cmd_llf(0x06);
    // SPIC set SSIENR = 0
    test_reg =  BA_SPI_FLASH_REG + SPI_SSIENR_OFFSET;
    test_data =  0x00;
    WRITE_REG_32(test_reg, test_data);
    // SPIC set CTRLR0(TX mode)
    test_reg =  BA_SPI_FLASH_REG + SPI_CTRLR0_OFFSET;
    test_data = (READ_REG_32(test_reg) & 0xfff0fcff);
    WRITE_REG_32(test_reg, test_data);
    // SPIC set ADDR_LENGTH = 1(?)
    test_reg =  BA_SPI_FLASH_REG + SPI_ADDR_LENGTH_OFFSET;
    test_data = 1;
    info = READ_REG_32(test_reg);
    WRITE_REG_32(test_reg, test_data);
    // SPIC set DR0
    test_reg =  BA_SPI_FLASH_REG + SPI_DR0_OFFSET;
    test_data =  0x0001 | (flsh_st << 8); // 0x01 --> WRSR 0x40->data
    WRITE_REG_16(test_reg, test_data);
    // SPIC set SSIENR = 1
    test_reg =  BA_SPI_FLASH_REG + SPI_SSIENR_OFFSET;
    test_data =  0x01;
    WRITE_REG_32(test_reg, test_data);

    spi_flash_wait_busy_llf();

    // SPIC set SSIENR = 0
    test_reg =  BA_SPI_FLASH_REG + SPI_SSIENR_OFFSET;
    test_data =  0x00;
    WRITE_REG_32(test_reg, test_data);
    // SPIC set ADDR_LENGTH
    test_reg =  BA_SPI_FLASH_REG + SPI_ADDR_LENGTH_OFFSET;
    test_data =  info;
    WRITE_REG_32(test_reg, test_data);

    flash_wait_busy_llf();
}

void spi_init_llf ( void )
{
//	int error = 0;
    int test_reg = 0;
    int test_data = 0;

    // SPIC set SSIENR = 0
    test_reg =  BA_SPI_FLASH_REG + SPI_SSIENR_OFFSET;
    test_data =  0x00;
    WRITE_REG_32(test_reg, test_data);

    // SPIC set AUTO_LENGTH
    test_reg =  BA_SPI_FLASH_REG + SPI_AUTO_LENGTH_OFFSET;
#if defined(RL6577_FPGA) || defined(RTS5771_FPGA) || defined(RL6643_FPGA)
    test_data =  (READ_REG_32(test_reg) & 0xffff0000) | 0x00030000;
#else
#if defined(RL6577_VA)
    test_data =  (READ_REG_32(test_reg) & 0xffff0000) | 0xfc130004;
#elif defined(RTS5771_VA)
    test_data =  (READ_REG_32(test_reg) & 0xf0ff0000) | 0xf0034000;
#else
    test_data =  (READ_REG_32(test_reg) & 0xffff0000) | 0xfc030000;
#endif
#endif
    WRITE_REG_32(test_reg, test_data);

#if defined(RL6577_FPGA) || defined(RTS5771_FPGA) || defined(RL6643_FPGA)
    // SPIC set BAUDR div = 3
    test_reg =	BA_SPI_FLASH_REG + SPI_BAUDR_OFFSET;
#if defined(RL6577_FPGA) || defined(RTS5771_FPGA)
    test_data =  0x01;
#else
    test_data =  0x03;
#endif
    WRITE_REG_32(test_reg, test_data);
#else
    test_reg =	BA_SPI_FLASH_REG + SPI_BAUDR_OFFSET;
#if defined(RL6643_VA)
    test_data =  0x05;
#elif defined(RL6577_VA)
    if(IS_EA_MODE)
        test_data =  0x06;
    else
        test_data =  0x0c;
#elif defined(RTS5771_VA)
    if(IS_EA_MODE)
        test_data =  0x08;
    else
        test_data =  0x0d;
#else
    test_data =  0x0c;
#endif
    WRITE_REG_32(test_reg, test_data);
#endif
    // SPIC set SER   only have one SPI nor flash
    test_reg =  BA_SPI_FLASH_REG + SPI_SER_OFFSET;
    test_data =  0x01;
    // READ_REG_32(test_reg);
    WRITE_REG_32(test_reg, test_data);

    // SPIC set CTRLR1 = 3 (receive 3 data to trigger)
    test_reg =  BA_SPI_FLASH_REG + SPI_CTRLR1_OFFSET;
    test_data =  0x03;
    WRITE_REG_32(test_reg, test_data);
    // SPIC set VALID_CMD   singal channel W/R
    test_reg =  BA_SPI_FLASH_REG + SPI_VALID_CMD_OFFSET;
    test_data =  0x600;
    WRITE_REG_32(test_reg, test_data);
    //SPIC set CK_MTIMES to improve the check times
    test_reg =  BA_SPI_FLASH_REG + SPI_CTRLR0_OFFSET;
    test_data =  0x0F000000;
    WRITE_REG_32(test_reg, test_data);
}

void spi_flash_set_protection_llf(BOOL isProtect)
{
    U32 spi_burst_val;

    spi_burst_val = READ_REG_32(SPI_BURST_WRITE_EN);
    WRITE_REG_32(SPI_BURST_WRITE_EN, (spi_burst_val & 0xfffffffe));//disable burst write

    if(isProtect) //enable write protection
    {
#if defined(RL6577_VA)
        _REG32(0xA003ff40) = 0;
#endif
        flash_set_status_llf(0x9C);
    }
    else //disable write protection
    {
        flash_set_status_llf(0x00);
    }
}

#if defined(RL6577_VA)|| defined(RL6447_VA) ||defined(RTS5771_VA)

U32 coarse_rx_delay_calibration(U32 iface, U32 ulMode, U32 bank, U32 thin_rx_delay, U32 fifo_delay)
{
    U32 retry_count = 5;
    U32 coarse_range_max = 0x40;
    U32 coarse_rx_delay = 0;
    U32 ret_data_compare = ERR_OK;
    U32 cphase_min = 0xffffffff, cphase_max = 0;
    U32 be_no;
    U32 i;
    U32 ret;
    U8 ubRawDataK = 0;
    U8 tx_pd = 0;

#ifdef RTS5771_VA
    tx_pd = 1;
#endif
    ubRawDataK = (gubCalibrateConfig >> 5) & 0x01;

    llfDbgPrintk(ALWAYS_MSG, "[Coarse_Rx_Delay_Calibration] fifo_delay = %d, thin_delay = %d\r\n",
                 fifo_delay, thin_rx_delay);
    be_no = (_MEM08(BANK_IMAPPING_TABLE_ADDR + bank) & 0xFF) >> 4;
    ulMode = FR_CONFIG_CH(FR_FC_MODE, gubStartCH);

    /*****************************coarse calibration*************************/
    for(coarse_rx_delay = 0; coarse_rx_delay < coarse_range_max; coarse_rx_delay++)//coarse delay
    {
        //llfprintk("bank %d, delay %d\r\n", bank, coarse_rx_delay);
        //llfprintk("FR_REG32_DLL_X(ONFI_READ_CTRL_0)  %x\r\n",FR_REG32_DLL_X(ONFI_READ_CTRL_0));
        ret_data_compare = ERR_OK;

        FR_REG32_DLL_X( ONFI_DPI_CTRL_0) = FR_REG32_DLL_X( ONFI_DPI_CTRL_0) | 0xc;
        FR_REG32_DLL_X( ONFI_DPI_CTRL_0) = FR_REG32_DLL_X( ONFI_DPI_CTRL_0) & 0xfffffff3;

        onfi4_dqs_calibrate_ch(be_no, iface, ulMode, coarse_rx_delay, thin_rx_delay, fifo_delay);

        FR_REG32_DLL_X( ONFI_DPI_CTRL_0) = FR_REG32_DLL_X( ONFI_DPI_CTRL_0) | 0x30;

        FcBusyWait1ms(1);

        for(i = 0; i < retry_count; i++)
        {
            if(ubRawDataK)
                ret_data_compare |= llfReadCacheRawDataCompare(bank);
            else
                ret_data_compare |= llfReadCacheAndCompare(bank);
        }
        //llfprintk("ret_data_compare is %x\r\n",ret_data_compare);

        //llfprintk("coarse_rx_delay %d,cmp %x,ret_data_compare %d\r\n",coarse_rx_delay,cmp,ret_data_compare);
        if (ret_data_compare == ERR_OK)
        {
            if(coarse_rx_delay < cphase_min)
                cphase_min = coarse_rx_delay;
            if(coarse_rx_delay > cphase_max)
                cphase_max = coarse_rx_delay;
        }
    }

    llfDbgPrintk(ALWAYS_MSG,
                 "[Calibrate Coarse_RX_Delay] coarse_min = %d, coarse_max = %d, coarse_set = %d\r\n", cphase_min,
                 cphase_max, cphase_min);
    if((cphase_min == 0xffffffff) || (cphase_max - cphase_min) < 1)
    {
        ret = ERR_FLASH_CALIBRATE;
    }
    else
    {
        ret = ERR_OK;
#ifdef RTS5771_VA
        FR_REG32_DLL_X(ONFI_CE_SEL_DLY_1) = 0x00000000 | cphase_min << 16;
#else
        FR_REG32_DLL_X(ONFI_READ_CTRL_0) = 0x00000008 | cphase_min << 20;
#endif
        FR_REG32_DLL_X(ONFI_READ_CTRL_1) = 0x0000000 | fifo_delay;

        FR_REG32_DLL_X(ONFI_DQS_IN_DLY_0) = 0x0 | (thin_rx_delay << 0) | (thin_rx_delay << 8) |
                                            (thin_rx_delay << 16) | (thin_rx_delay << 24);
        FR_REG32_DLL_X(ONFI_DQS_IN_DLY_1) = 0x0 | (thin_rx_delay << 0) | (thin_rx_delay << 8) |
                                            (thin_rx_delay << 16) | (thin_rx_delay << 24);
        FR_REG32_DLL_X(ONFI_DQS_IN_DLY_2) = 0x0 | (thin_rx_delay << 0) | (thin_rx_delay << 8) |
                                            (thin_rx_delay << 16) | (thin_rx_delay << 24);
        FR_REG32_DLL_X(ONFI_DQS_IN_DLY_3) = 0x0 | (thin_rx_delay << 0) | (thin_rx_delay << 8) |
                                            (thin_rx_delay << 16) | (thin_rx_delay << 24);
        //update the value of DQS_IN_DLY_*
        FR_REG32_DLL_X( ONFI_DPI_CTRL_0) = 0x020f0032 | (tx_pd << 31);
        FR_REG32_DLL_X( ONFI_DPI_CTRL_1) = 0x0000000f;
        FR_REG32_DLL_X( ONFI_DPI_CTRL_0) = 0x020f0030 | (tx_pd << 31);
    }

    return ret;
}

U32 thin_rx_delay_calibration(U32 iface, U32 ulMode, U32 bank, U32 coarse_rx_delay_min,
                              U32 coarse_range_max,
                              U32 fifo_delay)
{
#if 0// defined(FTL_H3DTV5) || defined(FTL_H3DTV6)
    U32 pAddr;
    U32 cmp;
    U32 retry_count = 5;
    U32 thin_range_max = 0x20;
    U32 thin_rx_delay = 0;
    U32 ret_data_compare = ERR_OK;
    U32 thin_rx_delay_min = 0xffffffff;
    U32 thin_rx_delay_max = 0x0;
    U32 be_no;
    U32 i;
    U32 ret = ERR_OK;
    U8 lun_no;
    U32 s_data_addr;

    lun_no = bank / NandPara.ubBankNumPerLun;
    llfDbgPrintk(ALWAYS_MSG, "[Thin_Rx_Delay_Calibration] fifo_delay = %d, coarse_delay = %d\r\n",
                 fifo_delay, coarse_rx_delay_min);
    be_no = (_MEM08(BANK_IMAPPING_TABLE_ADDR + bank) & 0xFF) >> 4;
    ulMode = FR_CONFIG_CH(FR_FC_MODE, gubStartCH);
    pAddr = (TEMP_BUF_PHY_ADDR + NandPara.ubSectorNumPerPage * 512);
    s_data_addr = TEMP_HBUF_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun);
    _REG32(s_data_addr) = 0xca35ca35;
    _REG32(s_data_addr + 4) = 0x35ca35ca;
    _REG32(s_data_addr + 8) = 0x3535ca35;
    _REG32(s_data_addr + 12) = 0xca353535;
    /*****************************thin calibration*************************/
    for(thin_rx_delay = 0; thin_rx_delay < thin_range_max; thin_rx_delay++)
    {
        //llfprintk("bank %d, delay %d\r\n", bank, thin_rx_delay);
        ret_data_compare = ERR_OK;

        FR_REG32_DLL_X(ONFI_DPI_CTRL_0) = FR_G_PHY_REG32(ONFI_DPI_CTRL_0) | 0xc;
        FR_REG32_DLL_X(ONFI_DPI_CTRL_0) = FR_G_PHY_REG32(ONFI_DPI_CTRL_0) & 0xfffffff3;

        onfi4_dqs_calibrate_ch(be_no, iface, ulMode, coarse_rx_delay_min, thin_rx_delay, fifo_delay);

        FR_REG32_DLL_X( ONFI_DPI_CTRL_0) = FR_REG32_DLL_X( ONFI_DPI_CTRL_0) | 0x30;

        FcBusyWait1ms(1);
        for(i = 0; i < retry_count; i++)
        {
            //---------read----------
            gul_FW_TAG = llfBETagSetting(0xa0 + i, bank);
            FCReadTraining(ulMode, bank, lun_no, pAddr, 0x10);

            FcBusyWait10us(1);
            ret = FCCompletionPolling(&cmp, (gul_FW_TAG));
            if(ret != ERR_OK)
            {
                llfDbgPrintk(ALWAYS_MSG, "bank %d read %d fail cmp %x\r\n", bank, i, cmp);
                return ERR_FIO_TIMEOUT;
            }
            if((cmp & BE_COMPLETION_ERROR_MASK) != 0)
            {
                //llfDbgPrintk(LLF_MSG, "Bank %d read fail: 0x%x, thin_rx_delay %x\r\n", bank, cmp, thin_rx_delay);
                ret_data_compare = ERR_COMPARE_DATA;
                break;
            }
            else
            {
                // Clean read memory cache
                cache_area_dinval((TEMP_BUF_ADDR + NandPara.ubSectorNumPerPage * 512), 0x10);
                cache_dummy_update_read();
                ret_data_compare |= llfCompareData(s_data_addr,
                                                   (TEMP_BUF_ADDR + NandPara.ubSectorNumPerPage * 512),
                                                   0x10);
            }
        }

        //llfprintk("thin_rx_delay %d,cmp %x,ret_data_compare %d\r\n",thin_rx_delay,cmp,ret_data_compare);
        if (ret_data_compare == ERR_OK)
        {
            if(thin_rx_delay < thin_rx_delay_min)
                thin_rx_delay_min = thin_rx_delay;
            if(thin_rx_delay > thin_rx_delay_max)
                thin_rx_delay_max = thin_rx_delay;
        }
        else if((thin_rx_delay_min != 0xffffffff)
                && (thin_rx_delay_max - thin_rx_delay_min + 1) < AUTO_CALIBRATE_RX_GAP)
        {
            llfprintk("Calibrate thin_rx_delay temp: %d %d\r\n", thin_rx_delay_min, thin_rx_delay_max);
            thin_rx_delay_min = 0xffffffff;
            thin_rx_delay_max = 0x0;
        }
        else if ((thin_rx_delay_min != 0xffffffff) && (thin_rx_delay_max > 0x1d))
        {
            break;  // Stop calibrate
        }
    }
#else
    U32 retry_count = 5;
    U32 thin_range_max = 0x20;
    U32 thin_rx_delay = 0;
    U32 ret_data_compare = ERR_OK;
    U32 thin_rx_delay_min = 0xffffffff;
    U32 thin_rx_delay_max = 0x0;
    U32 be_no;
    U32 i;
    U32 ret = ERR_OK;
    U8 ubRawDataK = 0;
    U8 tx_pd = 0;

#ifdef RTS5771_VA
    tx_pd = 1;
#endif
    ubRawDataK = (gubCalibrateConfig >> 5) & 0x01;
    llfDbgPrintk(ALWAYS_MSG, "[Thin_Rx_Delay_Calibration] fifo_delay = %d, coarse_delay = %d\r\n",
                 fifo_delay, coarse_rx_delay_min);
    be_no = (_MEM08(BANK_IMAPPING_TABLE_ADDR + bank) & 0xFF) >> 4;
    iface = FR_CONFIG_CH(FR_FC_MODE, gubStartCH);

    /*****************************thin calibration*************************/
    for(thin_rx_delay = 0; thin_rx_delay < thin_range_max; thin_rx_delay++)
    {
        //llfprintk("bank %d, delay %d\r\n", bank, thin_rx_delay);
        ret_data_compare = ERR_OK;

        FR_REG32_DLL_X(ONFI_DPI_CTRL_0) = FR_G_PHY_REG32(ONFI_DPI_CTRL_0) | 0xc;
        FR_REG32_DLL_X(ONFI_DPI_CTRL_0) = FR_G_PHY_REG32(ONFI_DPI_CTRL_0) & 0xfffffff3;

        onfi4_dqs_calibrate_ch(be_no, iface, ulMode, coarse_rx_delay_min, thin_rx_delay, fifo_delay);

        FR_REG32_DLL_X( ONFI_DPI_CTRL_0) = FR_REG32_DLL_X( ONFI_DPI_CTRL_0) | 0x30;

        FcBusyWait1ms(1);
        for(i = 0; i < retry_count; i++)
        {
            if(ubRawDataK)
                ret_data_compare |= llfReadCacheRawDataCompare(bank);
            else
                ret_data_compare |= llfReadCacheAndCompare(bank);
        }

        //llfprintk("thin_rx_delay %d,cmp %x,ret_data_compare %d\r\n",thin_rx_delay,cmp,ret_data_compare);
        if (ret_data_compare == ERR_OK)
        {
            if(thin_rx_delay < thin_rx_delay_min)
                thin_rx_delay_min = thin_rx_delay;
            if(thin_rx_delay > thin_rx_delay_max)
                thin_rx_delay_max = thin_rx_delay;
        }
        else if((thin_rx_delay_min != 0xffffffff)
                && (thin_rx_delay_max - thin_rx_delay_min + 1) < gubNvmeThinGap)
        {
            llfprintk("Calibrate thin_rx_delay temp: %d %d\r\n", thin_rx_delay_min, thin_rx_delay_max);
            thin_rx_delay_min = 0xffffffff;
            thin_rx_delay_max = 0x0;
        }
        else if ((thin_rx_delay_min != 0xffffffff) && (thin_rx_delay_max > 0x1d))
        {
            break;  // Stop calibrate
        }
    }
#endif
    thin_rx_delay = ((thin_rx_delay_min + thin_rx_delay_max) >> 1);
    llfDbgPrintk(ALWAYS_MSG,
                 "[Calibrate Thin_Rx_Delay] thin_min = %d, thin_max = %d, thin_set = %d\r\n",
                 thin_rx_delay_min, thin_rx_delay_max, thin_rx_delay);
    if((thin_rx_delay_min == 0xffffffff)
            || ((thin_rx_delay_max - thin_rx_delay_min + 1) < gubNvmeThinGap))
    {
        ret = ERR_FLASH_CALIBRATE;
    }
    else
    {
        ret = ERR_OK;
#ifdef RTS5771_VA
        FR_REG32_DLL_X(ONFI_CE_SEL_DLY_1) = 0x00000000 | coarse_rx_delay_min << 16;
#else
        FR_REG32_DLL_X(ONFI_READ_CTRL_0) = 0x00000008 | coarse_rx_delay_min << 20;
#endif
        FR_REG32_DLL_X(ONFI_READ_CTRL_1) = 0x0000000 | fifo_delay;

        FR_REG32_DLL_X(ONFI_DQS_IN_DLY_0) = 0x0 | (thin_rx_delay << 0) | (thin_rx_delay << 8) |
                                            (thin_rx_delay << 16) | (thin_rx_delay << 24);
        FR_REG32_DLL_X(ONFI_DQS_IN_DLY_1) = 0x0 | (thin_rx_delay << 0) | (thin_rx_delay << 8) |
                                            (thin_rx_delay << 16) | (thin_rx_delay << 24);
        FR_REG32_DLL_X(ONFI_DQS_IN_DLY_2) = 0x0 | (thin_rx_delay << 0) | (thin_rx_delay << 8) |
                                            (thin_rx_delay << 16) | (thin_rx_delay << 24);
        FR_REG32_DLL_X(ONFI_DQS_IN_DLY_3) = 0x0 | (thin_rx_delay << 0) | (thin_rx_delay << 8) |
                                            (thin_rx_delay << 16) | (thin_rx_delay << 24);
        //update the value of DQS_IN_DLY_*
        FR_REG32_DLL_X( ONFI_DPI_CTRL_0) = 0x020f0032 | (tx_pd << 31);
        FR_REG32_DLL_X( ONFI_DPI_CTRL_1) = 0x0000000f;
        FR_REG32_DLL_X( ONFI_DPI_CTRL_0) = 0x020f0030 | (tx_pd << 31);

        gubNvmeThinMax = thin_rx_delay_max;

        if(gubNvmeRxDelayMinTemp == 0xff)
        {
            gubNvmeRxDelayMinTemp = (thin_rx_delay_min + coarse_rx_delay_min * 2);
        }

        if(thin_rx_delay_max < 31)
        {
            gubNvmeRxDelayMaxTemp = (thin_rx_delay_max + coarse_rx_delay_min * 2);
        }
        else if(((coarse_rx_delay_min == (coarse_range_max - 1))) && (thin_rx_delay_max == 31))
        {
            gubNvmeThinMax = 29;
            gubNvmeRxDelayMaxTemp = (thin_rx_delay_max + coarse_rx_delay_min * 2);
        }
    }

    return ret;
}

U32 fifo_rx_calibration(U32 iface, U32 ulMode, U32 bank, U32 coarse_rx_delay_min, U32 thin_rx_delay)
{
#if 0//defined(FTL_H3DTV5) || defined(FTL_H3DTV6)
    U32 pAddr;
    U32 cmp;
    U32 retry_count = 5;
    U32 fifo_range_max = 25;
    U32 fifo_delay = 0;
    U32 ret = ERR_OK;
    U32 ret_data_compare = ERR_OK;
    U32 fifo_delay_min = 0xffffffff;
    U32 fifo_delay_max = 0x0;
    U32 be_no;
    U32 i;
    U8 lun_no;
    U32 s_data_addr;

    lun_no = bank / NandPara.ubBankNumPerLun;
    llfDbgPrintk(ALWAYS_MSG, "[fifo_Rx_calibration] coarse_delay = %d, thin_delay = %d\r\n",
                 coarse_rx_delay_min, thin_rx_delay);
    be_no = (_MEM08(BANK_IMAPPING_TABLE_ADDR + bank) & 0xFF) >> 4;
    iface = FR_CONFIG_CH(FR_FC_MODE, gubStartCH);
    pAddr = (TEMP_BUF_PHY_ADDR + NandPara.ubSectorNumPerPage * 512);
    s_data_addr = TEMP_HBUF_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun);
    _REG32(s_data_addr) = 0xca35ca35;
    _REG32(s_data_addr + 4) = 0x35ca35ca;
    _REG32(s_data_addr + 8) = 0x3535ca35;
    _REG32(s_data_addr + 12) = 0xca353535;
    /*****************************fifo calibration*************************/
    for(fifo_delay = 1; fifo_delay < fifo_range_max; fifo_delay++)
    {
        //llfprintk("bank %d, delay %d\r\n", bank, fifo_delay);
        ret_data_compare = ERR_OK;

        // reset fifo pointer
        FR_REG32_DLL_X(ONFI_DPI_CTRL_0) = FR_REG32_DLL_X(ONFI_DPI_CTRL_0) | 0xc;
        FR_REG32_DLL_X(ONFI_DPI_CTRL_0) = FR_REG32_DLL_X(ONFI_DPI_CTRL_0) & 0xfffffff3;

        onfi4_dqs_calibrate_ch(be_no, iface, ulMode, coarse_rx_delay_min, thin_rx_delay, fifo_delay);

        // disable 3 point calibration
        FR_REG32_DLL_X( ONFI_DPI_CTRL_0) = FR_REG32_DLL_X( ONFI_DPI_CTRL_0) | 0x30;

        FcBusyWait1ms(1);
        for(i = 0; i < retry_count; i++)
        {
            //---------read----------
            gul_FW_TAG = llfBETagSetting(TAG_READ_CACHE, bank);
            FCReadTraining(iface, bank, lun_no, pAddr, 0x10);

            FcBusyWait10us(1);
            ret = FCCompletionPolling(&cmp, (gul_FW_TAG));
            if(ret != ERR_OK)
            {
                U32 reg_v;

                llfDbgPrintk(ALWAYS_MSG, "bank %d read %d fail cmp %x fifo_delay %d\r\n", bank, i, cmp, fifo_delay);

                //reset onfi fifo pointer
                reg_v = FC_PHY_DLL_CH(ONFI_DPI_CTRL_0, 0) | 0xc;
                FR_G_PHY_REG32_W(ONFI_DPI_CTRL_0, reg_v );
                reg_v = FC_PHY_DLL_CH(ONFI_DPI_CTRL_0, 0) & 0xfffffff3;
                FR_G_PHY_REG32_W(ONFI_DPI_CTRL_0, reg_v );
                break;
            }
            if((cmp & BE_COMPLETION_ERROR_MASK) != 0)
            {
                //llfDbgPrintk(LLF_MSG, "Bank %d read fail: 0x%x, fifo_delay %x\r\n", bank, cmp, fifo_delay);
                ret_data_compare = ERR_COMPARE_DATA;
                break;
            }
            else
            {
                // Clean read memory cache
                cache_area_dinval((TEMP_BUF_ADDR + NandPara.ubSectorNumPerPage * 512), 0x10);
                cache_dummy_update_read();

                ret_data_compare = llfCompareData(s_data_addr,
                                                  (TEMP_BUF_ADDR + NandPara.ubSectorNumPerPage * 512),
                                                  0x10);
                if(ret_data_compare != ERR_OK)
                    break;
            }
        }
        //llfprintk("fifo_delay %d, cmp %x, ret_data_compare %d\r\n", fifo_delay, cmp, ret_data_compare);
        if ((ret_data_compare == ERR_OK) && (ret == ERR_OK))
        {
            if(fifo_delay < fifo_delay_min)
                fifo_delay_min = fifo_delay;
            if(fifo_delay > fifo_delay_max)
                fifo_delay_max = fifo_delay;
        }
        else if(fifo_delay_min != 0xffffffff)
        {
            break;
        }
    }
#else
    U32 retry_count = 5;
    U32 fifo_range_max = 0x20;
    U32 fifo_delay = 0;
    U32 ret = ERR_OK;
    U32 ret_data_compare = ERR_OK;
    U32 fifo_delay_min = 0xffffffff;
    U32 fifo_delay_max = 0x0;
    U32 be_no;
    U32 i;
    U8 ubRawDataK = 0;
    U8 tx_pd = 0;

#ifdef RTS5771_VA
    tx_pd = 1;
#endif

    ubRawDataK = (gubCalibrateConfig >> 5) & 0x01;

    llfDbgPrintk(ALWAYS_MSG, "[fifo_Rx_calibration] coarse_delay = %d, thin_delay = %d\r\n",
                 coarse_rx_delay_min, thin_rx_delay);
    be_no = (_MEM08(BANK_IMAPPING_TABLE_ADDR + bank) & 0xFF) >> 4;
    ulMode = FR_CONFIG_CH(FR_FC_MODE, gubStartCH);

    /*****************************fifo calibration*************************/
    for(fifo_delay = 1; fifo_delay < fifo_range_max; fifo_delay++)
    {
        //llfprintk("bank %d, delay %d\r\n", bank, fifo_delay);
        ret_data_compare = ERR_OK;

        // reset fifo pointer
        FR_REG32_DLL_X(ONFI_DPI_CTRL_0) = FR_REG32_DLL_X(ONFI_DPI_CTRL_0) | 0xc;
        FR_REG32_DLL_X(ONFI_DPI_CTRL_0) = FR_REG32_DLL_X(ONFI_DPI_CTRL_0) & 0xfffffff3;

        onfi4_dqs_calibrate_ch(be_no, iface, ulMode, coarse_rx_delay_min, thin_rx_delay, fifo_delay);

        // disable 3 point calibration
        FR_REG32_DLL_X( ONFI_DPI_CTRL_0) = FR_REG32_DLL_X( ONFI_DPI_CTRL_0) | 0x30;

        FcBusyWait1ms(1);
        for(i = 0; i < retry_count; i++)
        {
            if(ubRawDataK)
                ret_data_compare |= llfReadCacheRawDataCompare(bank);
            else
                ret_data_compare |= llfReadCacheAndCompare(bank);
        }
        //llfprintk("fifo_delay %d, cmp %x, ret_data_compare %d\r\n", fifo_delay, cmp, ret_data_compare);
        if (ret_data_compare == ERR_OK)
        {
            if(fifo_delay < fifo_delay_min)
                fifo_delay_min = fifo_delay;
            if(fifo_delay > fifo_delay_max)
                fifo_delay_max = fifo_delay;
        }
        else if(fifo_delay_min != 0xffffffff)
        {
            break;
        }
    }
#endif
    fifo_delay = fifo_delay_min + 3;
    llfDbgPrintk(ALWAYS_MSG, "[Calibrate fifo_delay] fifo_min = %d, fifo_max = %d, fifo_set = %d\r\n",
                 fifo_delay_min, fifo_delay_max,  fifo_delay);
    if((fifo_delay_min == 0xffffffff) || ((fifo_delay_max - fifo_delay_min) < 1))
    {
        ret = ERR_FLASH_CALIBRATE;
    }
    else
    {
        ret = ERR_OK;
#ifdef RTS5771_VA
        FR_REG32_DLL_X(ONFI_CE_SEL_DLY_1) = 0x00000000 | coarse_rx_delay_min << 16;
#else
        FR_REG32_DLL_X(ONFI_READ_CTRL_0) = 0x00000008 | coarse_rx_delay_min << 20;
#endif
        FR_REG32_DLL_X(ONFI_READ_CTRL_1) = 0x0000000 | fifo_delay;

        FR_REG32_DLL_X(ONFI_DQS_IN_DLY_0) = 0x0 | (thin_rx_delay << 0) | (thin_rx_delay << 8) |
                                            (thin_rx_delay << 16) | (thin_rx_delay << 24);
        FR_REG32_DLL_X(ONFI_DQS_IN_DLY_1) = 0x0 | (thin_rx_delay << 0) | (thin_rx_delay << 8) |
                                            (thin_rx_delay << 16) | (thin_rx_delay << 24);
        FR_REG32_DLL_X(ONFI_DQS_IN_DLY_2) = 0x0 | (thin_rx_delay << 0) | (thin_rx_delay << 8) |
                                            (thin_rx_delay << 16) | (thin_rx_delay << 24);
        FR_REG32_DLL_X(ONFI_DQS_IN_DLY_3) = 0x0 | (thin_rx_delay << 0) | (thin_rx_delay << 8) |
                                            (thin_rx_delay << 16) | (thin_rx_delay << 24);
        //update the value of DQS_IN_DLY_*
        FR_REG32_DLL_X( ONFI_DPI_CTRL_0) = 0x020f0032 | (tx_pd << 31);
        FR_REG32_DLL_X( ONFI_DPI_CTRL_1) = 0x0000000f;
        FR_REG32_DLL_X( ONFI_DPI_CTRL_0) = 0x020f0030 | (tx_pd << 31);
    }

    return ret;
}

void thin_rx_delay_setting()
{
    U8 i, thin_rx_delay, coarse_rx_delay;
    U8 tx_pd = 0;
#ifdef RTS5771_VA
    tx_pd = 1;
#endif

    for( i = 0; i < CH_NUM_MAX; i++  )
    {
        FR_REG_DLL_BASE_X = FR_DLL_REG_BASE + i * FR_ONFI_REG_SIZE;
        thin_rx_delay = ((gubNvmeThinDelayMax[i] + gubNvmeThinDelayMin[i]) >> 1);
        coarse_rx_delay = 0;
        while(thin_rx_delay > 31)
        {
            coarse_rx_delay++;
            thin_rx_delay -= 2;
        }
#ifdef RTS5771_VA
        FR_REG32_DLL_X(ONFI_CE_SEL_DLY_1) = 0x00000000 | coarse_rx_delay << 16;
#else
        FR_REG32_DLL_X(ONFI_READ_CTRL_0) = 0x00000008 | coarse_rx_delay << 20;
#endif

        FR_REG32_DLL_X(ONFI_DQS_IN_DLY_0) = 0x0 | (thin_rx_delay << 0) | (thin_rx_delay << 8) |
                                            (thin_rx_delay << 16) | (thin_rx_delay << 24);
        FR_REG32_DLL_X(ONFI_DQS_IN_DLY_1) = 0x0 | (thin_rx_delay << 0) | (thin_rx_delay << 8) |
                                            (thin_rx_delay << 16) | (thin_rx_delay << 24);
        FR_REG32_DLL_X(ONFI_DQS_IN_DLY_2) = 0x0 | (thin_rx_delay << 0) | (thin_rx_delay << 8) |
                                            (thin_rx_delay << 16) | (thin_rx_delay << 24);
        FR_REG32_DLL_X(ONFI_DQS_IN_DLY_3) = 0x0 | (thin_rx_delay << 0) | (thin_rx_delay << 8) |
                                            (thin_rx_delay << 16) | (thin_rx_delay << 24);
        //update the value of DQS_IN_DLY_*
        FR_REG32_DLL_X( ONFI_DPI_CTRL_0) = 0x020f0032 | (tx_pd << 31);
        FR_REG32_DLL_X( ONFI_DPI_CTRL_1) = 0x0000000f;
        FR_REG32_DLL_X( ONFI_DPI_CTRL_0) = 0x020f0030 | (tx_pd << 31);

        printk("Ch:%d,thindelaymin:%d,thindelaymax:%d,coarse_rx_delay:%d,thindelayset:%d\r\n", i,
               gubNvmeThinDelayMin[i], gubNvmeThinDelayMax[i], coarse_rx_delay, thin_rx_delay);
    }
}

#endif

void llfLoadDefaultTiming()
{
    U32 ulRegValue;
    if((FLASH_VENDOR(gulFlashVendorNum) == IS_MICRON) || (FLASH_VENDOR(gulFlashVendorNum) == IS_INTEL)
            || FLASH_VENDOR(gulFlashVendorNum) == IS_YMTC)
    {
        ulRegValue = (2 << 24) | (3 << 16) | (2 << 8) | (2 << 0);
        FR_G_CTRL_REG32_W(FR_CHN_DELAY_0, ulRegValue);

        ulRegValue = (10 << 24) | (6 << 16) | (11 << 8) | (2 << 0);
        FR_G_CTRL_REG32_W(FR_CHN_DELAY_1, ulRegValue);

        ulRegValue = (3 << 24) | (1 << 16) | (10 << 8) | (6 << 0);
        FR_G_CFG_REG32_W(FR_GLB_DELAY_0, ulRegValue);

        ulRegValue = 5;
        FR_G_CFG_REG32_W(FR_GLB_DELAY_1, ulRegValue);
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_TOSHIBA) ||
            (FLASH_VENDOR(gulFlashVendorNum) == IS_SANDISK)
            || (FLASH_VENDOR(gulFlashVendorNum) == IS_SAMSUNG)
            || (FLASH_VENDOR(gulFlashVendorNum) == IS_HYNIX))
    {
        ulRegValue = (3 << 24) | (3 << 16) | (3 << 8) | (1 << 0);
        FR_G_CTRL_REG32_W(FR_CHN_DELAY_0, ulRegValue);

        ulRegValue = (0x0a << 24) | (0x06 << 16) | (11 << 8) | (2 << 0);
        FR_G_CTRL_REG32_W(FR_CHN_DELAY_1, ulRegValue);
        FR_G_CFG_REG32_W(FR_GLB_DELAY_0, 0x03010a06);
        FR_G_CFG_REG32_W(FR_GLB_DELAY_1, 0x05);
    }

    //default timing seting 10/20 MHz
    if(FLASH_INTERFACE(gulFlashVendorNum) == (FLASH_IF_SUP_DDR2 | FLASH_IF_SUP_TOGGLE))
    {
        FR_G_CFG_REG32_W(FR_PHY_TIME0, 0x0);
        FR_G_CFG_REG32_W(FR_PHY_TIME1, 0x0);
        FR_G_CFG_REG32_W(FR_PHY_TIME2, 0x00020200);
        FR_G_CFG_REG32_W(FR_PHY_TIME3, 0x00000200);
        FR_G_CFG_REG32_W(FR_PHY_TIME4, 0x0);
    }
    else
    {
        FR_G_CFG_REG32_W(FR_PHY_TIME0, 0x0);
        FR_G_CFG_REG32_W(FR_PHY_TIME1, 0x0);
        FR_G_CFG_REG32_W(FR_PHY_TIME2, 0x0);
        FR_G_CFG_REG32_W(FR_PHY_TIME3, 0x0);
        FR_G_CFG_REG32_W(FR_PHY_TIME4, 0x0);
    }
}

void llfLoadTimingFromConfig()
{
    U32 ulRegValue = 0;

    //set fc parser timing from config
    ulRegValue = _MEM32(CONFIG_BASE_VA_ADDR + SBLK_OFFSET_FR_CHN_DELAY_0);
    FR_G_CTRL_REG32_W(FR_CHN_DELAY_0, ulRegValue);

    ulRegValue = _MEM32(CONFIG_BASE_VA_ADDR + SBLK_OFFSET_FR_CHN_DELAY_1);
    FR_G_CTRL_REG32_W(FR_CHN_DELAY_1, ulRegValue);

    ulRegValue = _MEM32(CONFIG_BASE_VA_ADDR + SBLK_OFFSET_FR_GLB_DELAY_0);
    FR_G_CFG_REG32_W(FR_GLB_DELAY_0, ulRegValue);

    ulRegValue = _MEM32(CONFIG_BASE_VA_ADDR + SBLK_OFFSET_FR_GLB_DELAY_1);
    FR_G_CFG_REG32_W(FR_GLB_DELAY_1, ulRegValue);

    ulRegValue = _MEM32(CONFIG_BASE_VA_ADDR + SBLK_OFFSET_FR_PAR_POLL_CFG);
    FR_G_CFG_REG32_W(FR_PAR_POLL_CFG, ulRegValue);

    //set fc phy timing from config
    ulRegValue = _MEM32(CONFIG_BASE_VA_ADDR + CONFIG_FR_PHY_TIME0);
    FR_G_CFG_REG32_W(FR_PHY_TIME0, ulRegValue);

    ulRegValue = _MEM32(CONFIG_BASE_VA_ADDR + CONFIG_FR_PHY_TIME1);
    FR_G_CFG_REG32_W(FR_PHY_TIME1, ulRegValue);

    ulRegValue = _MEM32(CONFIG_BASE_VA_ADDR + CONFIG_FR_PHY_TIME2);
    FR_G_CFG_REG32_W(FR_PHY_TIME2, ulRegValue);

    ulRegValue = _MEM32(CONFIG_BASE_VA_ADDR + CONFIG_FR_PHY_TIME3);
    FR_G_CFG_REG32_W(FR_PHY_TIME3, ulRegValue);

    ulRegValue = _MEM32(CONFIG_BASE_VA_ADDR + CONFIG_FR_PHY_TIME4);
    FR_G_CFG_REG32_W(FR_PHY_TIME4, ulRegValue);

    ulRegValue = _MEM32(CONFIG_BASE_VA_ADDR + CONFIG_FR_PHY_TIME5);
    FR_G_CFG_REG32_W(FR_PHY_TIME5, ulRegValue);

    ulRegValue = _MEM32(CONFIG_BASE_VA_ADDR + CONFIG_FR_PHY_TIME6);
    FR_G_CFG_REG32_W(FR_PHY_TIME6, ulRegValue);

    ulRegValue = _MEM32(CONFIG_BASE_VA_ADDR + CONFIG_FR_PHY_TIME7);
    FR_G_CFG_REG32_W(FR_PHY_TIME7, ulRegValue);
}

// Calibrate read (DQS input delay)
U32 llffioBECalibrate(U8 bank, U16 block_no, U8 ubIFType, U8 ubMode)
{
    U32 ret = ERR_OK;
    U8 be_no;
    be_no = (_MEM08(BANK_IMAPPING_TABLE_ADDR + bank) & 0xFF) >> 4;

#if defined(RL6447_VA)||defined(RL6577_VA) || defined(RTS5771_VA)
    U8 fifo_delay;
    U8 coarse_rx_delay;
    U8 thin_rx_delay;
    U32 tmp;
    U8 tx_pd = 0;
#ifdef RTS5771_VA
    tx_pd = 1;
#endif
    U32 coarse_range_max = 0x40;

    FR_REG_DLL_BASE_X = FR_DLL_REG_BASE + be_no * FR_ONFI_REG_SIZE;

    default_delay_chain_setting(FR_CONFIG_CH(FR_FC_MODE, gubStartCH), ubMode, &coarse_rx_delay,
                                &thin_rx_delay, &fifo_delay);
    if(ubMode < 9)
    {
        gubNvmeThinGap = 30;
    }
    else if (ubMode < 12)
    {
        gubNvmeThinGap = 30;
    }
    else if (ubMode < 13)
    {
        gubNvmeThinGap = 25;
    }
    else if (ubMode < 14)
    {
        gubNvmeThinGap = 20;
    }
    else if (ubMode < 15)
    {
        gubNvmeThinGap = 16;
    }
    else if (ubMode < 16)
    {
        gubNvmeThinGap = 14;
    }
    else if(ubMode < 17)
    {
        gubNvmeThinGap = 13;
    }
    else if (ubMode < 18)
    {
        gubNvmeThinGap = 12;
    }
    else if (ubMode < 19)
    {
        gubNvmeThinGap = 12;
    }
    else if (ubMode < 20)
    {
        gubNvmeThinGap = 12;
    }
    else
    {
        gubNvmeThinGap = 12;
    }
    coarse_rx_delay = 0;
    thin_rx_delay = 8;

    //llfprintk("fifo_delay %d, coarse_delay: %d, thin_delay: %d\r\n", fifo_delay, coarse_rx_delay, thin_rx_delay);

    if ((ubIFType == IF_ONFI_SDR) || (ubIFType == IF_TOGGLE_SDR))
    {
        // SDR mode need not to calibrate dqs input
        coarse_rx_delay = 0x0;
#ifdef RTS5771_VA
        FR_REG32_DLL_X(ONFI_CE_SEL_DLY_1) = 0x00000000 | coarse_rx_delay << 16;
#else
        FR_REG32_DLL_X(ONFI_READ_CTRL_0) = 0x00000008 | coarse_rx_delay << 20;
#endif
        FR_REG32_DLL_X(ONFI_READ_CTRL_1) = 0x0000000 | fifo_delay;
        ret = ERR_OK;
        return ret;
    }

    ubIFType = FR_CONFIG_CH(FR_FC_MODE, gubStartCH);
    ret = ERR_FLASH_CALIBRATE;
    gubNvmeRxDelayMinTemp = 0xff;
    gubNvmeRxDelayMaxTemp = 0;
    for (coarse_rx_delay = 0; coarse_rx_delay < coarse_range_max; coarse_rx_delay++)
    {
        gubThreePointKSetting = 3;
        gubNvmeThinMax = 0xff;
        tmp = thin_rx_delay_calibration(ubIFType, ubMode, bank, coarse_rx_delay, coarse_range_max,
                                        fifo_delay);
        if(tmp == ERR_FIO_TIMEOUT)
        {
            llfDbgPrintk(ALWAYS_MSG, "bank %d K timeout\r\n", bank);
            return ERR_FIO_TIMEOUT;
        }
        else if((ubMode == 0) && (tmp == ERR_OK))//10M
        {
            ret = ERR_OK;
            if(gubNvmeRxDelayMaxTemp == 0)
                gubNvmeRxDelayMaxTemp = 31;

            if((gubNvmeThinDelayMin[be_no] == 0xff)
                    || (gubNvmeThinDelayMin[be_no] < gubNvmeRxDelayMinTemp))
            {
                gubNvmeThinDelayMin[be_no] = gubNvmeRxDelayMinTemp;
            }

            if((gubNvmeThinDelayMax[be_no] == 0)
                    || (gubNvmeThinDelayMax[be_no] > gubNvmeRxDelayMaxTemp))
            {
                gubNvmeThinDelayMax[be_no] = gubNvmeRxDelayMaxTemp;
            }
            break;
        }
        else if(gubNvmeThinMax < 31)
        {
            ret = ERR_OK;
            thin_rx_delay = ((gubNvmeRxDelayMinTemp + gubNvmeRxDelayMaxTemp) >> 1);
            coarse_rx_delay = 0;
            while(thin_rx_delay >= 31)
            {
                coarse_rx_delay++;
                thin_rx_delay -= 2;
            }
#ifdef RTS5771_VA
            FR_REG32_DLL_X(ONFI_CE_SEL_DLY_1) = 0x00000000 | coarse_rx_delay << 16;
#else
            FR_REG32_DLL_X(ONFI_READ_CTRL_0) = 0x00000008 | coarse_rx_delay << 20;
#endif
            FR_REG32_DLL_X(ONFI_READ_CTRL_1) = 0x0000000 | fifo_delay;

            FR_REG32_DLL_X(ONFI_DQS_IN_DLY_0) = 0x0 | (thin_rx_delay << 0) | (thin_rx_delay << 8) |
                                                (thin_rx_delay << 16) | (thin_rx_delay << 24);
            FR_REG32_DLL_X(ONFI_DQS_IN_DLY_1) = 0x0 | (thin_rx_delay << 0) | (thin_rx_delay << 8) |
                                                (thin_rx_delay << 16) | (thin_rx_delay << 24);
            FR_REG32_DLL_X(ONFI_DQS_IN_DLY_2) = 0x0 | (thin_rx_delay << 0) | (thin_rx_delay << 8) |
                                                (thin_rx_delay << 16) | (thin_rx_delay << 24);
            FR_REG32_DLL_X(ONFI_DQS_IN_DLY_3) = 0x0 | (thin_rx_delay << 0) | (thin_rx_delay << 8) |
                                                (thin_rx_delay << 16) | (thin_rx_delay << 24);
            //update the value of DQS_IN_DLY_*
            FR_REG32_DLL_X( ONFI_DPI_CTRL_0) = 0x020f0032 | (tx_pd << 31);
            FR_REG32_DLL_X( ONFI_DPI_CTRL_1) = 0x0000000f;
            FR_REG32_DLL_X( ONFI_DPI_CTRL_0) = 0x020f0030 | (tx_pd << 31);

            if((gubNvmeThinDelayMin[be_no] == 0xff)
                    || (gubNvmeThinDelayMin[be_no] < gubNvmeRxDelayMinTemp))
            {
                gubNvmeThinDelayMin[be_no] = gubNvmeRxDelayMinTemp;
            }

            if((gubNvmeThinDelayMax[be_no] == 0)
                    || (gubNvmeThinDelayMax[be_no] > gubNvmeRxDelayMaxTemp))
            {
                gubNvmeThinDelayMax[be_no] = gubNvmeRxDelayMaxTemp;
            }

            break;
        }
    }

    if (ret != ERR_OK)
    {
        llfDbgPrintk(ALWAYS_MSG, "bank %d fail\r\n", bank);
        return ERR_FLASH_CALIBRATE;
    }

    thin_rx_delay = FR_REG32_DLL_X(ONFI_DQS_IN_DLY_1) & 0x1f;
#if defined(RL6577_VA)||defined(RTS5771_VA)
    gubThreePointKSetting = 3;
#else
    gubThreePointKSetting = 0;
#endif
    ret = fifo_rx_calibration(ubIFType, ubMode, bank, coarse_rx_delay, thin_rx_delay);
    fifo_delay = (FR_REG32_DLL_X(ONFI_READ_CTRL_1)) & 0x1f;
    llfprintk("bank %d coarse %d, thin %d, fifo_delay %d\r\n", bank,
              coarse_rx_delay, thin_rx_delay, fifo_delay);
#if defined(RL6577_VA)||defined(RTS5771_VA)
    FR_REG32_DLL_X(ONFI_DPI_CTRL_0) = FR_REG32_DLL_X(ONFI_DPI_CTRL_0) | 0x30;
#else
    FR_REG32_DLL_X(ONFI_DPI_CTRL_0) = FR_REG32_DLL_X(ONFI_DPI_CTRL_0) & 0xffffffcf;
#endif
    //printk("3 point k bit [4:5](3-disable 0-enable) %x\r\n", FR_REG32_DLL_X(ONFI_DPI_CTRL_0));
#else
    U32 ret_data_compare = ERR_OK;
    U32 rx_delay = 0;
    U32 rx_delay_min = 0xffffffff;
    U32 rx_delay_max = 0;
    U32 temp_rx_delay_min = 0xffffffff;
    U32 temp_rx_delay_max = 0;
    U32 k_min = 0;
    U32 k_max = 0x20;
    U32 phyCfgGap = 0;
    U32 head_pAddr;
    U32 pAddr;
    U32 cmp;
    U32 ulMode;
    U32 ce_no = 0;
    U8 lun_no;
    lun_no = bank / NandPara.ubBankNumPerLun;
    ce_no = (_MEM08(BANK_IMAPPING_TABLE_ADDR + bank) & 0xF);
    k_min = 0;
    k_max = 64;
    FR_REG_BASE_X = FR_REG_BASE + be_no * FR_REG_SIZE;
#ifndef RTS5771_FPGA
    FR_REG32_CH(FR_PHY_DELAY_CTRL, be_no) = 1;//calibrate by ce
    if(IS_6855_VERSION_TAG)
    {
        //before cfg delay chain,need to disable output
        FR_G_CTRL_REG32(FR_PHY_STATE) |= (0xf << 24);
    }
    FR_REG32_X(FR_PHY_DELAY_DQI0_CE0 + ce_no * 4) = 0;
    FR_REG32_X(FR_PHY_DELAY_DQI1_CE0 + ce_no * 4) = 0;
    if(IS_6855_VERSION_TAG)
        FR_G_CTRL_REG32(FR_PHY_STATE) &= (~(0xf << 24));//enable output
#endif
    if ((FLASH_VENDOR(gulFlashVendorNum) == IS_TOSHIBA)
            && (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_7TDK_SDR)) //Toshiba TLC flash
    {
        //kuo_20150707, for TLC need not to calibrate dqs input now
        // tlc flash can't do normal read in llffioBECalibrate()
        //kuo_20150420, for TLC test
        FR_REG32_X(FR_PHY_SDR_CFG) = 0; //kuo_20150420, for TLC test
    }
    else if ((ubIFType == IF_ONFI_SDR) || (ubIFType == IF_TOGGLE_SDR))
    {
        if(IS_6855_VERSION_TAG)
        {
            //before cfg delay chain,need to disable output
            FR_G_CTRL_REG32(FR_PHY_STATE) |= (0xf << 24);
        }
        // SDR mode need not to calibrate dqs input
        FR_REG32_X(FR_PHY_DELAY_CFG0 + ce_no * 4) = 0;
        if(IS_6855_VERSION_TAG)
            FR_G_CTRL_REG32(FR_PHY_STATE) &= (~(0xf << 24));//enable output
        ret = ERR_OK;
        return ret;
    }
    else
    {
        for(rx_delay = k_min; rx_delay < k_max; rx_delay++)
        {
            if((ubIFType == IF_ONFI_SDR) || (ubIFType == IF_TOGGLE_SDR)) // ONFI SDR or Toggle SDR
            {
                if(rx_delay > 7)
                {
                    break;
                }
                FR_REG32_X(FR_PHY_SDR_CFG) = (rx_delay & 0xF);
            }
            else// ONFI DDR, ONFI DDR 2, Toggle DDR
            {
                if(IS_6855_VERSION_TAG)
                {
                    //before cfg delay chain,need to disable output
                    FR_G_CTRL_REG32(FR_PHY_STATE) |= (0xf << 24);
                }
                FR_REG32_X(FR_PHY_DELAY_CFG0 + ce_no * 4) &= ~0x7F; // Clean first
                FR_REG32_X(FR_PHY_DELAY_CFG0 + ce_no * 4) |= (rx_delay & 0x7F);
                if(IS_6855_VERSION_TAG)
                    FR_G_CTRL_REG32(FR_PHY_STATE) &= (~(0xf << 24));//enable output
            }

            FcBusyWait1ms(1);

            //---------read----------
            _REG32(TEMP_HBUF_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun)) = 0xBadBad;
#ifdef IS_8K_PAGE
            _REG32(TEMP_HBUF_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun) + 16) = 0xBadBad;
#else
            _REG32(TEMP_HBUF_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun) + 32) = 0xBadBad;
#endif
            cache_area_dwbinval(TEMP_BUF_ADDR, NandPara.ubSectorNumPerPage * 512); // 16k Data
            cache_area_dwbinval((TEMP_HBUF_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun)),
                                L2pPara.ubHdr4BLenPerPage * 4);
            cache_dummy_update_read();

            // set header DMA
            head_pAddr = TEMP_HBUF_PHY_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun);
            // set data DMA
            pAddr = (TEMP_BUF_PHY_ADDR + NandPara.ubSectorNumPerPage * 512);

            ulMode = FR_CONFIG_CH(FR_FC_MODE, gubStartCH);
            gul_FW_TAG = llfBETagSetting(TAG_READ_CACHE, bank);
            FCReadCacheDRAM(ulMode, bank, lun_no, 0, pAddr, NandPara.ubSectorNumPerPage * 512,
                            head_pAddr, DRAM_HEAD_SIZE); // read data only
            //llfprintk("bank	is %d, delay is %d\r\n",bank,i);

            FcBusyWait1ms(1);
            ret = FCCompletionPolling(&cmp, (gul_FW_TAG));
            if(ret != ERR_OK)
                break;
            if((cmp & BE_COMPLETION_ERROR_MASK) != 0)
            {
                llfDbgPrintk(LLF_MSG,	"Bank %d read fail: 0x%x, rx_delay %x\r\n", bank, cmp, rx_delay);
                ret_data_compare = ERR_COMPARE_DATA;
            }
            else
            {
                // Clean read memory cache
                cache_area_dinval((TEMP_BUF_ADDR + NandPara.ubSectorNumPerPage * 512),
                                  NandPara.ubSectorNumPerPage * 512); // 16k Data
                cache_area_dinval((TEMP_HBUF_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun)),
                                  L2pPara.ubHdr4BLenPerPage * 4);
                cache_dummy_update_read();

                // Compare header and data
                ret_data_compare = llfCompareHead(TEMP_HBUF_ADDR,
                                                  (TEMP_HBUF_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun)),
                                                  L2pPara.ubHdr4BLenPerPage * 4);
                ret_data_compare |= llfCompareData(TEMP_BUF_ADDR,
                                                   (TEMP_BUF_ADDR + NandPara.ubSectorNumPerPage * 512),
                                                   (NandPara.ubSectorNumPerPage * 512));
            }
            if (ret_data_compare == ERR_OK)
            {
                if(rx_delay < temp_rx_delay_min)
                    temp_rx_delay_min = rx_delay;
                if(rx_delay > temp_rx_delay_max)
                    temp_rx_delay_max = rx_delay;
            }
            else if (temp_rx_delay_min != 0xffffffff)
            {
                llfDbgPrintk(ALWAYS_MSG,  "RX Temp CH%d:CE%d DQS RX K %x-%x\r\n",
                             be_no, ce_no, temp_rx_delay_min, temp_rx_delay_max);
                if(temp_rx_delay_max - temp_rx_delay_min + 1 > phyCfgGap)
                {
                    rx_delay_min = temp_rx_delay_min;
                    rx_delay_max = temp_rx_delay_max;
                    phyCfgGap = temp_rx_delay_max - temp_rx_delay_min + 1;
                }
                temp_rx_delay_min = 0xffffffff;
                temp_rx_delay_max = 0;
            }
        }
    }
    if((temp_rx_delay_min != 0xffffffff) && ((temp_rx_delay_max - temp_rx_delay_min + 1) > phyCfgGap))
    {
        rx_delay_min = temp_rx_delay_min;
        rx_delay_max = temp_rx_delay_max;
        phyCfgGap = temp_rx_delay_max - temp_rx_delay_min + 1;
    }

    if((rx_delay_min == 0xffffffff) || ((rx_delay_max - rx_delay_min + 1) < 1))
    {
        ret = ERR_FLASH_CALIBRATE;
        if(IS_6855_VERSION_TAG)
        {
            //before cfg delay chain,need to disable output
            FR_G_CTRL_REG32(FR_PHY_STATE) |= (0xf << 24);
        }
        FR_REG32_X(FR_PHY_DELAY_CFG0 + ce_no * 4) = 0;
        if(IS_6855_VERSION_TAG)
            FR_G_CTRL_REG32(FR_PHY_STATE) &= (~(0xf << 24));//enable output

        llfDbgPrintk(ALWAYS_MSG,  "CH%d:CE%d DQS RX K %x %x-%x\r\n",
                     be_no, ce_no, rx_delay, rx_delay_min, rx_delay_max);
    }
    else
    {
        ret = ERR_OK;
        rx_delay = ((rx_delay_min + rx_delay_max) >> 1);
        llfDbgPrintk(ALWAYS_MSG, "CH%d:CE%d DQS RX K %d-%d=%d,set%d\r\n",
                     be_no, ce_no, rx_delay_min, rx_delay_max, rx_delay_max - rx_delay_min + 1, rx_delay);
        if(ubIFType == IF_ONFI_SDR)
        {
            FR_REG32_X(FR_PHY_SDR_CFG) = (rx_delay & 0xF);
        }
        else
        {
            if(IS_6855_VERSION_TAG)
            {
                //before cfg delay chain,need to disable output
                FR_G_CTRL_REG32(FR_PHY_STATE) |= (0xf << 24);
            }
            FR_REG32_X(FR_PHY_DELAY_CFG0 + ce_no * 4) &= ~(0x7F); // Clean first
            FR_REG32_X(FR_PHY_DELAY_CFG0 + ce_no * 4) |= (rx_delay & 0x7F);
            if(IS_6855_VERSION_TAG)
                FR_G_CTRL_REG32(FR_PHY_STATE) &= (~(0xf << 24));//enable output
        }
    }
#endif

    FcBusyWait1ms(1);

    return ret;
}

U32 WriteReadFlashCache(U8 bank, U8 ubClkMode)
{
    U32 pAddr;
    U32 head_pAddr;
    U32 ulMode, cmp;
    U32 i;
    U32 ulRetryCount = 10;
    U32 ret = ERR_OK;
    U8 lun_no;
    U8 ubRawDataK = 0;

    ubRawDataK = (gubCalibrateConfig >> 5) & 0x01;

    lun_no = bank / NandPara.ubBankNumPerLun;
    //set header data
    for (i = 0; i < (HEADER_DMA_MAX_LEN); i++)
    {
        _REG32(TEMP_HBUF_ADDR + (i * 4)) = ((i << 24) | (i << 16) | (i << 8) | i);
    }
    // DWB that cache line
    cache_area_dwbinval(TEMP_HBUF_ADDR, HEADER_MAX_LEN);
    cache_dummy_update_read();

    // Program DRAM for Data DMA
    if(ubRawDataK)
        llfProgramAllMixData(TEMP_BUF_ADDR, (NandPara.ubSectorNumPerPage * 512));
    else
        llfProgramSeqData(TEMP_BUF_ADDR, (NandPara.ubSectorNumPerPage * 512));
    // DWB that cache line
    cache_area_dwbinval(TEMP_BUF_ADDR, 0x4000);
    cache_dummy_update_read();

    ulMode = FR_CONFIG_CH(FR_FC_MODE, gubStartCH);

    //---------write----------
    head_pAddr = TEMP_HBUF_PHY_ADDR;
    pAddr = TEMP_BUF_PHY_ADDR;

    gul_FW_TAG = llfBETagSetting(TAG_WRITE_CACHE, bank);
    FCWriteCacheDRAM(ulMode, bank, lun_no, 0, pAddr, NandPara.ubSectorNumPerPage * 512,
                     head_pAddr, DRAM_HEAD_SIZE);
    FcBusyWait1ms(1);
    ret = FCCompletionPolling(&cmp, (gul_FW_TAG));
    if(ret == ERR_OK)
    {
        // llfprintk("w,b=%d,cmp=%x\r\n", bank, cmp);
        if((cmp & BE_COMPLETION_ERROR_MASK) != 0)
        {
            llfDbgPrintk(ALWAYS_MSG, "write cache error: %d\r\n", bank);
            return ERR_FLASH_CALIBRATE;
        }
    }
    else
    {
        llfprintk("write cache error %x\r\n", ret);
        return ERR_FLASH_CALIBRATE;
    }

    for(i = 0; i < ulRetryCount; i++)
    {
        //---------read----------
        _REG32(TEMP_HBUF_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun)) = 0xBadBad;
        _REG32(TEMP_HBUF_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun) + 32) = 0xBadBad;
        cache_area_dwbinval((TEMP_BUF_ADDR + NandPara.ubSectorNumPerPage * 512),
                            NandPara.ubSectorNumPerPage * 512);
        cache_area_dwbinval((TEMP_HBUF_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun)),
                            L2pPara.ubHdr4BLenPerPage * 4);
        cache_dummy_update_read();

        head_pAddr = TEMP_HBUF_PHY_ADDR + HEADER_MAX_LEN;
        pAddr = TEMP_BUF_PHY_ADDR + NandPara.ubSectorNumPerPage * 512;

        gul_FW_TAG = llfBETagSetting(TAG_READ_CACHE, bank);
        FCReadCacheDRAM(ulMode, bank, lun_no, 0, pAddr, NandPara.ubSectorNumPerPage * 512,
                        head_pAddr, DRAM_HEAD_SIZE );
        FcBusyWait1ms(1);
        ret = FCCompletionPolling(&cmp, (gul_FW_TAG));
        if(ret == ERR_OK)
        {
            // llfprintk("r b=%d,cmp=%x\r\n",bank, cmp);
            if((cmp & BE_COMPLETION_ERROR_MASK) != 0)
            {
                llfDbgPrintk(ALWAYS_MSG, "Read cache error %x\r\n", cmp);
                return ERR_FLASH_CALIBRATE;
            }
        }
        else
        {
            llfprintk("read cache error %x\r\n", ret);
            return ret;
        }
        // Clean read memory cache
        cache_area_dinval((TEMP_BUF_ADDR + NandPara.ubSectorNumPerPage * 512),
                          NandPara.ubSectorNumPerPage * 512);
        cache_area_dinval((TEMP_HBUF_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun)),
                          L2pPara.ubHdr4BLenPerPage * 4);
        cache_dummy_update_read();

        // Compare header and data
        ret |= llfCompareHead(TEMP_HBUF_ADDR,
                              (TEMP_HBUF_ADDR + (HEADER_MAX_LEN)),
                              L2pPara.ubHdr4BLenPerPage * 4);
        ret |= llfCompareData(TEMP_BUF_ADDR, (TEMP_BUF_ADDR + NandPara.ubSectorNumPerPage * 512),
                              (NandPara.ubSectorNumPerPage * 512));
        if(ret != ERR_OK)
        {
            llfDbgPrintk(ALWAYS_MSG, "Bank %d\r\n", bank);
            ret = ERR_COMPARE_DATA;
            break;
        }
    }

    FcBusyWait1ms(1);

    return ret;
}

U32 llfReadCacheAndCompare(U8 ubBankNo)
{
    U32 ulAddr, ulHeadAddr, cmp, ulMode;
    U32 ret = ERR_OK;
    U8 ubBankNum, lun_no = 0;
    U8 i = 0;
    U8 ubPipelineK = 0;

    cmp = 0;
    ubPipelineK = (gubCalibrateConfig >> 6) & 0x01;
    lun_no = ubBankNo / NandPara.ubBankNumPerLun;
    _REG32(TEMP_HBUF_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun)) = 0xBadBad;
    _REG32(TEMP_HBUF_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun) + 32) = 0xBadBad;
    cache_area_dwbinval(TEMP_BUF_ADDR, NandPara.ubSectorNumPerPage * 512);
    cache_area_dwbinval((TEMP_HBUF_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun)),
                        L2pPara.ubHdr4BLenPerPage * 4);
    cache_dummy_update_read();

    ulMode = FR_CONFIG_CH(FR_FC_MODE, gubStartCH);
    ulHeadAddr = TEMP_HBUF_PHY_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun);
    ulAddr = (TEMP_BUF_PHY_ADDR + NandPara.ubSectorNumPerPage * 512);

    if(ubPipelineK)
    {
        ubBankNum = UnbalancedGetBankNum();
        for(i = 0; i < ubBankNum; i++)
        {
            gul_FW_TAG = i;
            if(i == ubBankNo)
            {
                //llfDbgPrintk(ALWAYS_MSG, "llfReadCacheAndCompare readcache bank: %d\r\n", ubBankNo);
                FCReadCacheDRAM(ulMode, ubBankNo, lun_no, 0, ulAddr, NandPara.ubSectorNumPerPage * 512,
                                ulHeadAddr, DRAM_HEAD_SIZE);
            }
            else
            {
                //llfDbgPrintk(ALWAYS_MSG, "llfReadCacheAndCompare writecache bank: %d\r\n", i);
                FCWriteCacheDRAM(ulMode, i, lun_no, 0, TEMP_BUF_PHY_ADDR, NandPara.ubSectorNumPerPage * 512,
                                 TEMP_HBUF_PHY_ADDR, DRAM_HEAD_SIZE);
            }
        }
    }
    else
    {
        gul_FW_TAG = llfBETagSetting(TAG_READ_CACHE, ubBankNo);
        FCReadCacheDRAM(ulMode, ubBankNo, lun_no, 0, ulAddr, NandPara.ubSectorNumPerPage * 512,
                        ulHeadAddr, DRAM_HEAD_SIZE);
    }

    if(ubPipelineK)
    {
        ret = llfPollingAllBankCMP(&cmp, ubBankNo);
    }
    else
    {
        gul_FW_TAG = llfBETagSetting(TAG_READ_CACHE, ubBankNo);
        ret = FCCompletionPolling(&cmp, (gul_FW_TAG));
    }
    if(ret != ERR_OK)
    {
        llfDbgPrintk(ALWAYS_MSG, "[ReadCacheTimeout]bank %d\r\n", ubBankNo);
    }
    else
    {
        if((cmp & BE_COMPLETION_ERROR_MASK) != 0)
        {
            //llfDbgPrintk(ALWAYS_MSG, "[ReadCacheRF]bank %d, cmp %x\r\n", ubBankNo, cmp);
            ret = ERR_COMPARE_DATA;
        }
        else
        {
            cache_area_dinval((TEMP_BUF_ADDR + NandPara.ubSectorNumPerPage * 512),
                              NandPara.ubSectorNumPerPage * 512);
            cache_area_dinval((TEMP_HBUF_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun)),
                              L2pPara.ubHdr4BLenPerPage * 4);
            cache_dummy_update_read();

            // Compare header and data
            ret = llfCompareHead(TEMP_HBUF_ADDR,
                                 (TEMP_HBUF_ADDR + (HEADER_MAX_LEN * NandPara.ubPlaneNumPerLun)),
                                 L2pPara.ubHdr4BLenPerPage * 4);
#if 1
            ret |= llfCompareDataPer1KB(TEMP_BUF_ADDR,
                                        (TEMP_BUF_ADDR + NandPara.ubSectorNumPerPage * 512),
                                        (NandPara.ubSectorNumPerPage * 512));
#else
            ret |= llfCompareData(TEMP_DBUF_UNCACHE_ADDR,
                                  (TEMP_DBUF_UNCACHE_ADDR + NandPara.ubSectorNumPerPage * 512),
                                  (NandPara.ubSectorNumPerPage * 512));
#endif
        }
    }
    return ret;
}

U32 llfReadCacheRawDataCompare(U8 ubBankNo)
{
    U32 ulAddr, cmp, ulMode;
    U32 ret = ERR_OK;
    U16 uwTemp2kLength = 0;
    U8 i = 0;
    U8 ubBankNum, ubPipelineK = 0;

    cache_area_dwbinval(TEMP_BUF_ADDR, NandPara.ubSectorNumPerPage * 512);
    cache_dummy_update_read();

    cmp = 0;
    ubPipelineK = (gubCalibrateConfig >> 6) & 0x01;
    uwTemp2kLength = gub_Total_len_per_2K;
    ulMode = FR_CONFIG_CH(FR_FC_MODE, gubStartCH);
    ulAddr = (TEMP_BUF_PHY_ADDR + NandPara.ubSectorNumPerPage * 512);

    if(ubPipelineK)
    {
        ubBankNum = UnbalancedGetBankNum();
        for(i = 0; i < ubBankNum; i++)
        {
            gul_FW_TAG = i;
            if(i == ubBankNo)
            {
                //llfDbgPrintk(ALWAYS_MSG, "llfReadCacheRawDataCompare readcache bank: %d\r\n", ubBankNo);
                gub_Total_len_per_2K = uwTemp2kLength;
                FCReadCacheRedundant(ulMode, ubBankNo, ulAddr, NandPara.ubSectorNumPerPage * 512);
            }
            else
            {
                //llfDbgPrintk(ALWAYS_MSG, "llfReadCacheRawDataCompare writecache bank: %d\r\n", i);
                gub_Total_len_per_2K = 2048;
                FCWriteCacheRedundant(ulMode, i, 0, TEMP_BUF_PHY_ADDR, NandPara.ubSectorNumPerPage * 512,
                                      TEMP_HBUF_PHY_ADDR, DRAM_HEAD_SIZE);
            }
        }
    }
    else
    {
        gul_FW_TAG = llfBETagSetting(TAG_READ_CACHE, ubBankNo);
        FCReadCacheRedundant(ulMode, ubBankNo, ulAddr, NandPara.ubSectorNumPerPage * 512);
    }
    gub_Total_len_per_2K = uwTemp2kLength;

    if(ubPipelineK)
    {
        ret = llfPollingAllBankCMP(&cmp, ubBankNo);
    }
    else
    {
        gul_FW_TAG = llfBETagSetting(TAG_READ_CACHE, ubBankNo);
        ret = FCCompletionPolling(&cmp, (gul_FW_TAG));
    }
    if(ret != ERR_OK)
    {
        llfDbgPrintk(ALWAYS_MSG, "[ReadCacheTimeout]bank %d\r\n", ubBankNo);
    }
    else
    {
        if((cmp & BE_COMPLETION_ERROR_MASK) != 0)
        {
            //llfDbgPrintk(ALWAYS_MSG, "[ReadCacheRF]bank %d, cmp %x\r\n", ubBankNo, cmp);
            ret = ERR_COMPARE_DATA;
        }
        else
        {
            cache_area_dinval((TEMP_BUF_ADDR + NandPara.ubSectorNumPerPage * 512),
                              NandPara.ubSectorNumPerPage * 512);
            cache_dummy_update_read();

            // Compare data
#if 1
            ret = llfCompareDataPer1KB(TEMP_BUF_ADDR,
                                       (TEMP_BUF_ADDR + NandPara.ubSectorNumPerPage * 512),
                                       (NandPara.ubSectorNumPerPage * 512));
#else
            ret |= llfCompareData(TEMP_DBUF_UNCACHE_ADDR,
                                  (TEMP_DBUF_UNCACHE_ADDR + NandPara.ubSectorNumPerPage * 512),
                                  (NandPara.ubSectorNumPerPage * 512));
#endif
        }
    }
    return ret;
}

void llfSettingPerCh(U32 addr, U16 mode, U16 Speed)
{
    U8 i, j;
    U32 Offset = (SBLK_FC_HW_CONFIG2_PER_SETTING_LEN * mode);
    U32 ulDelayPhase = 0;

    for(i = 0; i < CH_NUM_MAX; i++)
    {
        FR_REG_BASE_X = FR_REG_BASE + i * FR_REG_SIZE;

        _REG32(addr + SBLK_OFFSET_NORMAL_FR_PHY_SDR_CFG_PER_CH
               + Offset + (i * WORD_BYTE_SIZE)) = FR_REG32_X(FR_PHY_SDR_CFG);
        for(j = 0; j < CE_NUM_MAX; j++)
        {
            if((FR_REG32_X(FR_PHY_DELAY_CFG0 + (j * WORD_BYTE_SIZE)) & 0x7f00) == 0)//DQS TX=0
            {
                ulDelayPhase = FR_REG32_X(FR_PHY_DELAY_CFG0 + (j * WORD_BYTE_SIZE)) & 0x7f;//save DQS RX
                ulDelayPhase += (((FR_REG32_X(FR_PHY_DELAY_DQO0_CE0 + (j * WORD_BYTE_SIZE)) & 0x7f) + 0x80) << 8);
                _REG32(addr + SBLK_OFFSET_NORMAL_FR_PHY_DELAY_CFG0_PER_CH
                       + Offset + (i * 0x20) + (j * WORD_BYTE_SIZE)) = ulDelayPhase;
                //printk("CH%d CE%d Delay Phase: 0x%x\r\n", i, j, ulDelayPhase);
            }
            else
            {
                _REG32(addr + SBLK_OFFSET_NORMAL_FR_PHY_DELAY_CFG0_PER_CH
                       + Offset + (i * 0x20) + (j * WORD_BYTE_SIZE)) = FR_REG32_X(FR_PHY_DELAY_CFG0 +
                               (j * WORD_BYTE_SIZE));
                //llfDbgPrintk(ALWAYS_MSG, "[DB] CH:%d,CE:%d: ulCEPhyCfg %x\r\n", i, j,
                //  FR_REG32_X(FR_PHY_DELAY_CFG0+ (j * WORD_BYTE_SIZE)));
            }
        }
    }

#if defined(RL6577_VA)||defined(RL6447_VA) || defined(RTS5771_VA)
    for(i = 0; i < CH_NUM_MAX; i++)
    {
        FR_REG_DLL_BASE_X = FR_DLL_REG_BASE + i * FR_ONFI_REG_SIZE;

        _REG32(addr + SBLK_OFFSET_NORMAL_FR_PHY_DELAY_CFG0_PER_CH
               + Offset + (i * 0x20) + 0) = FR_REG32_DLL_X(ONFI_DQS_IN_DLY_0);
        //llfprintk("onfi setting:0x%x\r\n", _REG32(addr + SBLK_OFFSET_NORMAL_FR_PHY_DELAY_CFG0_PER_CH));

        _REG32(addr + FR_PHY_FIFO_DELAY_PER_CH
               + Offset + (i * WORD_BYTE_SIZE)) = FR_REG32_DLL_X(ONFI_READ_CTRL_1);
#ifdef RTS5771_VA
        _REG32(addr + FR_PHY_RX_DELAY_PER_CH
               + Offset + (i * WORD_BYTE_SIZE)) = FR_REG32_DLL_X(ONFI_CE_SEL_DLY_1);
#else
        _REG32(addr + FR_PHY_RX_DELAY_PER_CH
               + Offset + (i * WORD_BYTE_SIZE)) = FR_REG32_DLL_X(ONFI_READ_CTRL_0);
#endif

        _REG32(addr + FR_PHY_OCDP_PER_CH
               + Offset + (i * WORD_BYTE_SIZE)) = FR_REG32_ZQ(ONFI_OCDP0_SET1);
        _REG32(addr + FR_PHY_OCDN_PER_CH
               + Offset + (i * WORD_BYTE_SIZE)) = FR_REG32_ZQ(ONFI_OCDN0_SET1);
        _REG32(addr + FR_PHY_ODTP_PER_CH
               + Offset + (i * WORD_BYTE_SIZE)) = FR_REG32_ZQ(ONFI_ODT_TTCP0_SET1);
        _REG32(addr + FR_PHY_ODTN_PER_CH
               + Offset + (i * WORD_BYTE_SIZE)) = FR_REG32_ZQ(ONFI_ODT_TTCN0_SET1);

        FR_REG_BASE_X = FR_REG_BASE + i * FR_REG_SIZE;
#if defined(RL6577_VA) || defined(RTS5771_VA)
        //use FR_PAD_PU_CTRL to replace FR_PAD_PULL in spec,need check later
        _REG32(addr + SBLK_OFFSET_NORMAL_FR_PAD_PULL_PER_CH
               + Offset + (i * WORD_BYTE_SIZE)) = FR_REG32_X(FR_PAD_PU_CTRL);
#ifdef FC_FULL_CALIBRATE
        _REG32(addr + SBLK_OFFSET_NORMAL_FR_PAD_CTRL_PER_CH
               + Offset + (i * WORD_BYTE_SIZE)) = gubOnfiTxPi[i];
#else
        _REG32(addr + SBLK_OFFSET_NORMAL_FR_PAD_CTRL_PER_CH
               + Offset + (i * WORD_BYTE_SIZE)) = 0xffffffff;
#endif
#endif
        _REG32(addr + SBLK_OFFSET_NORMAL_FR_PAD_CFG0_PER_CH
               + Offset + (i * WORD_BYTE_SIZE)) = FR_REG32_X(FR_PAD_CFG0);
        _REG32(addr + SBLK_OFFSET_NORMAL_FR_PAD_CFG1_PER_CH
               + Offset + (i * WORD_BYTE_SIZE)) = FR_REG32_X(FR_PAD_CFG1);
    }

#elif defined(RL6531_VB)
    for(i = 0; i < CH_NUM_MAX; i++)
    {
        FR_REG_BASE_X = FCPara.ulChRegBaseAddr[i];

        _REG32(addr + SBLK_OFFSET_NORMAL_FR_PAD_PULL_PER_CH
               + Offset + (i * WORD_BYTE_SIZE)) = FR_REG32_X(FR_PAD_PULL);
        _REG32(addr + SBLK_OFFSET_NORMAL_FR_PAD_DRV_CFG0_PER_CH
               + Offset + (i * WORD_BYTE_SIZE)) = FR_REG32_X(FR_PAD_DRV_CFG0);
        _REG32(addr + SBLK_OFFSET_NORMAL_FR_PAD_DRV_CFG1_PER_CH
               + Offset + (i * WORD_BYTE_SIZE)) = FR_REG32_X(FR_PAD_DRV_CFG1);
        _REG32(addr + SBLK_OFFSET_NORMAL_FR_PAD_DRV_CFG2_PER_CH
               + Offset + (i * WORD_BYTE_SIZE)) = FR_REG32_X(FR_PAD_DRV_CFG2);
        _REG32(addr + SBLK_OFFSET_NORMAL_FR_PAD_DRV_CFG3_PER_CH
               + Offset + (i * WORD_BYTE_SIZE)) = FR_REG32_X(FR_PAD_DRV_CFG3);
        _REG32(addr + SBLK_OFFSET_NORMAL_FR_PAD_ODT_CFG_PER_CH
               + Offset + (i * WORD_BYTE_SIZE)) = FR_REG32_X(FR_PAD_ODT_CFG);
        _REG32(addr + SBLK_OFFSET_NORMAL_FR_PAD_ODT_CTRL_PER_CH
               + Offset + (i * WORD_BYTE_SIZE)) = FR_REG32_X(FR_PAD_ODT_CTRL);
        _REG32(addr + SBLK_OFFSET_NORMAL_FR_PAD_CFG0_PER_CH
               + Offset + (i * WORD_BYTE_SIZE)) = FR_REG32_X(FR_PAD_CFG0);
        _REG32(addr + SBLK_OFFSET_NORMAL_FR_PAD_CFG1_PER_CH
               + Offset + (i * WORD_BYTE_SIZE)) = FR_REG32_X(FR_PAD_CFG1);
    }

#endif
}

U32 llfSetSBlockHWConfig(U32 sblk_addr, U8 ubType, U8 ubMode)
{
    U32 i;
    U32 table_version;
    U32 version = 0;
    U32 sub_version = 0;
    _REG32(sblk_addr + SBLK_OFFSET_FR_DATA_ENTRY_SIZE) = FR_CONFIG_REG32(FR_CACHE_DATA_ENTRY_SIZE0);
    _REG32(sblk_addr + SBLK_OFFSET_FR_HEAD_ENTRY_SIZE) = FR_CONFIG_REG32(FR_CACHE_HEAD_ENTRY_SIZE0);
    _REG32(sblk_addr + SBLK_OFFSET_FR_LIST_ENTRY_SIZE0) = FR_CONFIG_REG32(FR_CACHE_LIST_ENTRY_SIZE0);
    _REG32(sblk_addr + SBLK_OFFSET_FR_LIST_ENTRY_SIZE1) = FR_CONFIG_REG32(FR_CACHE_LIST_ENTRY_SIZE1);

    _REG32(sblk_addr + SBLK_OFFSET_FR_LBN_SIZE) = 0;
#if !(defined(RTS5771_FPGA)||defined(RTS5771_VA))
    _REG32(sblk_addr + SBLK_OFFSET_FR_CRC_EN) = 1;
#endif
    //Rona:fix later,xor related function need to be verified later.
    //_REG32(sblk_addr + SBLK_OFFSET_FC_XOR_CFGR) = FC_TOP_REG(FC_XOR_CFGR);
    //_REG32(sblk_addr + SBLK_OFFSET_FC_XOR_CFG_LOOP_LENR0) = FC_TOP_REG(FC_XOR_CFG_LOOP_LENR0);
    //_REG32(sblk_addr + SBLK_OFFSET_FC_XOR_CFG_LOOP_LENR1) = FC_TOP_REG(FC_XOR_CFG_LOOP_LENR1);

    _REG32(sblk_addr + SBLK_OFFSET_FR_PAR_POLL_CFG) = FR_CONFIG_CH(FR_PAR_POLL_CFG, gubStartCH);
    _REG32(sblk_addr + SBLK_OFFSET_FR_PAR_CFG) = FR_CONFIG_REG32(FR_PAR_CFG);
    _REG32(sblk_addr + SBLK_OFFSET_FR_CHN_DELAY_0) = FR_REG32(FR_CHN_DELAY_0);
    _REG32(sblk_addr + SBLK_OFFSET_FR_CHN_DELAY_1) = FR_REG32(FR_CHN_DELAY_1);
    _REG32(sblk_addr + SBLK_OFFSET_FR_GLB_DELAY_0) = FR_CONFIG_REG32(FR_GLB_DELAY_0);
    _REG32(sblk_addr + SBLK_OFFSET_FR_GLB_DELAY_1) = FR_CONFIG_REG32(FR_GLB_DELAY_1);
    _REG32(sblk_addr + SBLK_OFFSET_FR_PHY_TIME0) = FR_CONFIG_REG32(FR_PHY_TIME0);
    _REG32(sblk_addr + SBLK_OFFSET_FR_PHY_TIME1) = FR_CONFIG_REG32(FR_PHY_TIME1);
    _REG32(sblk_addr + SBLK_OFFSET_FR_PHY_TIME2) = FR_CONFIG_REG32(FR_PHY_TIME2);
    _REG32(sblk_addr + SBLK_OFFSET_FR_PHY_TIME3) = FR_CONFIG_REG32(FR_PHY_TIME3);
    _REG32(sblk_addr + SBLK_OFFSET_FR_PHY_TIME4) = FR_CONFIG_REG32(FR_PHY_TIME4);
    _REG32(sblk_addr + SBLK_OFFSET_FR_PHY_TIME5) = FR_CONFIG_REG32(FR_PHY_TIME5);
    _REG32(sblk_addr + SBLK_OFFSET_FR_PHY_TIME6) = FR_CONFIG_REG32(FR_PHY_TIME6);
    _REG32(sblk_addr + SBLK_OFFSET_FR_PHY_TIME7) = FR_CONFIG_REG32(FR_PHY_TIME7);
    _REG32(sblk_addr + SBLK_OFFSET_FR_MAC_CFG) = FC_TOP_REG(FR_MAC_CFG);

    // _REG32(sblk_addr + SBLK_OFFSET_FR_ECC_CTRL) = FC_TOP_REG(FR_ECC_CTRL);
    // _REG32(sblk_addr + SBLK_OFFSET_FR_ECC_CFG0) = FC_TOP_REG(FR_ECC_CFG0);
    _REG32(sblk_addr + SBLK_OFFSET_FR_ECC_CFG1) = FC_TOP_REG(FR_ECC_CFG1);

    _REG32(sblk_addr + SBLK_OFFSET_FR_IF_TYPE)  = (U32)ubType;
    _REG32(sblk_addr + SBLK_OFFSET_FR_CLK_MODE) = (U32)ubMode;
#ifdef RL6643_VA
    _REG08(sblk_addr + SBLK_OFFSET_FR_CLK_OFFSET) = gubFcClkOffset;
#endif

#if (!defined(RL6643_FPGA) && !defined(RL6643_VA))
    _REG32(sblk_addr + SBLK_OFFSET_AES_KEY0) = FC_TOP_REG(AES_KEY0);
    _REG32(sblk_addr + SBLK_OFFSET_AES_KEY1) = FC_TOP_REG(AES_KEY1);
    _REG32(sblk_addr + SBLK_OFFSET_AES_KEY2) = FC_TOP_REG(AES_KEY2);
    _REG32(sblk_addr + SBLK_OFFSET_AES_KEY3) = FC_TOP_REG(AES_KEY3);
    _REG32(sblk_addr + SBLK_OFFSET_AES_KEY4) = FC_TOP_REG(AES_KEY4);
    _REG32(sblk_addr + SBLK_OFFSET_AES_KEY5) = FC_TOP_REG(AES_KEY5);
    _REG32(sblk_addr + SBLK_OFFSET_AES_KEY6) = FC_TOP_REG(AES_KEY6);
    _REG32(sblk_addr + SBLK_OFFSET_AES_KEY7) = FC_TOP_REG(AES_KEY7);
    _REG32(sblk_addr + SBLK_OFFSET_AES_MODE_SEL) = FC_TOP_REG(AES_MODE_SEL);
#endif

    table_version = PARSER_INDEX_ADDR + PARSER_INDEX_SIZE + SEQUENCER_INDEX_SIZE +
                    AUTO_INSERT_INDEX_SIZE;
    for(i = 0; i < FC_TABLE_INFO_SIZE; i++)
    {
        if((_MEM08(table_version + i) == '_') &&
                ((_MEM08(table_version + i + 1) == 'v') ||
                 (_MEM08(table_version + i + 1) == 'V')))// Find _Vx.xx
        {
            for(i += 2; i < FC_TABLE_INFO_SIZE; i++)
            {
                if(_MEM08(table_version + i) == '.')
                    break;
                version = version * 10 +  _MEM08(table_version + i) - '0';
            }
            if(version >= 100)
            {
                llfprintk("Warning Table version invalid: %d\r\n", version);
                version = 100;
            }
            _REG08(sblk_addr + SBLK_OFFSET_FC_TABLEVERSION) = version;

            for(i += 1; i < FC_TABLE_INFO_SIZE; i++)
            {
                if(_MEM08(table_version + i) == '.')
                    break;
                sub_version = sub_version * 10 +  _MEM08(table_version + i) - '0';
            }
            if(sub_version >= 100)
            {
                llfprintk("Warning Table sub version invalid: %d\r\n", sub_version);
                sub_version = 100;
            }
            _REG08(sblk_addr + SBLK_OFFSET_FC_TABLESUBVERSION) = sub_version;
            break;
        }
    }
    if(((FLASH_VENDOR(gulFlashVendorNum) == IS_MICRON) ||
            (FLASH_VENDOR(gulFlashVendorNum) == IS_INTEL)) &&
            (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_B0K))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'B';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = '0';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = 'K';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = 'B';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if(((FLASH_VENDOR(gulFlashVendorNum) == IS_MICRON) ||
             (FLASH_VENDOR(gulFlashVendorNum) == IS_INTEL)) &&
            (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_B16))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'B';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = '1';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = '6';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = 'A';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if(((FLASH_VENDOR(gulFlashVendorNum) == IS_MICRON) ||
             (FLASH_VENDOR(gulFlashVendorNum) == IS_INTEL)) &&
            (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_B17))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'B';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = '1';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = '7';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = 'A';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if(((FLASH_VENDOR(gulFlashVendorNum) == IS_MICRON) ||
             (FLASH_VENDOR(gulFlashVendorNum) == IS_INTEL)) &&
            (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_B27A ||
             FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_B27B))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'B';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = '2';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = '7';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = 'A';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if(((FLASH_VENDOR(gulFlashVendorNum) == IS_MICRON) ||
             (FLASH_VENDOR(gulFlashVendorNum) == IS_INTEL)) &&
            (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_B37R))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'B';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = '3';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = '7';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = 'R';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if(((FLASH_VENDOR(gulFlashVendorNum) == IS_MICRON) ||
             (FLASH_VENDOR(gulFlashVendorNum) == IS_INTEL)) &&
            (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_B36R))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'B';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = '3';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = '6';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = 'R';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if(((FLASH_VENDOR(gulFlashVendorNum) == IS_MICRON) ||
             (FLASH_VENDOR(gulFlashVendorNum) == IS_INTEL)) &&
            (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_B47R))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'B';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = '4';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = '7';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = 'R';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if(((FLASH_VENDOR(gulFlashVendorNum) == IS_MICRON) ||
             (FLASH_VENDOR(gulFlashVendorNum) == IS_INTEL)) &&
            (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_B58R))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'B';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = '5';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = '8';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = 'R';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if(((FLASH_VENDOR(gulFlashVendorNum) == IS_MICRON) ||
             (FLASH_VENDOR(gulFlashVendorNum) == IS_INTEL)) &&
            (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_N18A ||
             FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_N28))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'N';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = '1';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = '8';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = 'A';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_INTEL ||
             FLASH_VENDOR(gulFlashVendorNum) == IS_MICRON)
            && (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_N38A))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'N';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = '3';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = '8';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = 'A';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_INTEL ||
             FLASH_VENDOR(gulFlashVendorNum) == IS_MICRON)
            && (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_N38B))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'N';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = '3';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = '8';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = 'B';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_INTEL ||
             FLASH_VENDOR(gulFlashVendorNum) == IS_MICRON)
            && (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_N48R))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'N';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = '4';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = '8';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = 'R';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_INTEL ||
             FLASH_VENDOR(gulFlashVendorNum) == IS_MICRON)
            && (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_Q5171A))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'Q';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = '5';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = '7';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = 'A';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if(((FLASH_VENDOR(gulFlashVendorNum) == IS_TOSHIBA) ||
             (FLASH_VENDOR(gulFlashVendorNum) == IS_SANDISK)) &&
            ((FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_8T22_SDR) ||
             (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_9T23_SDR_TOGGLE) ||
             (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_XT23_SDR_TOGGLE_64GB)))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'B';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = 'I';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = 'C';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = 'S';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if(((FLASH_VENDOR(gulFlashVendorNum) == IS_TOSHIBA) ||
             (FLASH_VENDOR(gulFlashVendorNum) == IS_SANDISK)) &&
            ((FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_XT24) ||
             (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_XT24_64G) ||
             (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_BICS4P5_256Gb) ||
             (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_BICS4P5_512Gb)))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'B';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = 'I';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = 'C';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = '4';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_SANDISK) &&
            (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_BICS5_512Gb
             || FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_BICS5_1024Gb
             || FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_BICS5_1024Gb_ODT))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'S';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = 'B';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = 'I';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = '5';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_SANDISK) &&
            (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_BICS4_QLC))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'S';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = 'B';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = 'Q';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = '4';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_SANDISK) &&
            (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_BICS5_QLC))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'S';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = 'B';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = 'Q';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = '5';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_TOSHIBA) &&
            ((FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_XT25_TOGGLE_64GB)
             || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_XT25_TOGGLE_128GB)))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'B';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = 'I';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = 'C';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = '5';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_SAMSUNG) &&
            (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_SSV2_128G))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'S';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = 'S';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = 'V';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = '2';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_SAMSUNG) &&
            (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_SSV4
             || FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_SSV4_64G))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'S';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = 'S';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = 'V';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = '4';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_SAMSUNG) &&
            ((FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_SSV5)
             || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_SSV5_64G)))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'S';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = 'S';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = 'V';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = '5';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_SAMSUNG) &&
            ((FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_SSV6)
             || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_SSV6_1v8)
             || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_SSV6_512Gb)
             || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_SSV6_512Gb_1v8)))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'S';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = 'S';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = 'V';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = '6';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if(IS_HYNIX == FLASH_VENDOR(gulFlashVendorNum) &&
            IS_H3DTV3 == FLASH_SERIAL_NUM(gulFlashVendorNum))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'H';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = '3';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = 'V';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = '3';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_SAMSUNG) &&
            ((FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_SSV7_512Gb)))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'S';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = 'S';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = 'V';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = '7';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_HYNIX) &&
            ((FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_H3DTV4)
             || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_H3DTV4_512Gb)))

    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'H';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = '3';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = 'V';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = '4';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_HYNIX) &&
            ((FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_H3DTV5)
             || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_H3DQV5)))

    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'H';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = '3';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = 'V';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = '5';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_HYNIX) &&
            ((FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_H3DTV6)
             || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_H3DTV6_1Tb)))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'H';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = '3';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = 'V';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = '6';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_HYNIX) &&
            ((FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_H3DTV7_512Gb)
             || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_H3DTV7_1Tb)))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'H';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = '3';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = 'V';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = '7';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_YMTC) &&
            ((FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YG2T)
             || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YG2T_CS2)))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'Y';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = 'G';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = '2';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = 'T';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_YMTC) &&
            (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX2T))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'Y';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = 'X';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = '2';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = 'T';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_YMTC) &&
            (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX3T_WYS
             || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX3T_WDS)))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'Y';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = 'X';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = '3';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = 'T';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_YMTC) &&
            (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX2Q))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'Y';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = 'X';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = '2';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = 'Q';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }
    else if((FLASH_VENDOR(gulFlashVendorNum) == IS_YMTC) &&
            (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX3T_WDS))
    {
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII0) = 'Y';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII1) = 'X';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII2) = '3';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII3) = 'D';
        _REG08(sblk_addr + SBLK_OFFSET_FLASH_ASCII4) = '\0';
    }

    llfDbgPrintk(ALWAYS_MSG, "Table Version %sV%d.%d\r\n",
                 (char *)(sblk_addr + SBLK_OFFSET_FLASH_ASCII0), version, sub_version);

#ifdef RL6531_VB
    DBGPRINTK(ALWAYS_MSG, "Recorde LDPC Matrix Addr.\r\n");
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_24_EN_A_BEGIN) = (U32)ldpc_24_encoder_a_begin;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_24_EN_A_END) = (U32)ldpc_24_encoder_a_end;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_24_EN_B_BEGIN) = (U32)ldpc_24_encoder_b_begin;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_24_EN_B_END) = (U32)ldpc_24_encoder_b_end;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_24_DE_A_BEGIN) = (U32)ldpc_24_decoder_a_begin;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_24_DE_A_END) = (U32)ldpc_24_decoder_a_end;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_24_DE_B_BEGIN) = (U32)ldpc_24_decoder_b_begin;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_24_DE_B_END) = (U32)ldpc_24_decoder_b_end;

    _REG32(sblk_addr + SBLK_OFFSET_LDPC_45_EN_A_BEGIN) = (U32)ldpc_45_encoder_a_begin;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_45_EN_A_END) = (U32)ldpc_45_encoder_a_end;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_45_EN_B_BEGIN) = (U32)ldpc_45_encoder_b_begin;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_45_EN_B_END) = (U32)ldpc_45_encoder_b_end;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_45_DE_A_BEGIN) = (U32)ldpc_45_decoder_a_begin;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_45_DE_A_END) = (U32)ldpc_45_decoder_a_end;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_45_DE_B_BEGIN) = (U32)ldpc_45_decoder_b_begin;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_45_DE_B_END) = (U32)ldpc_45_decoder_b_end;

    _REG32(sblk_addr + SBLK_OFFSET_LDPC_66_EN_A_BEGIN) = (U32)ldpc_66_encoder_a_begin;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_66_EN_A_END) = (U32)ldpc_66_encoder_a_end;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_66_EN_B_BEGIN) = (U32)ldpc_66_encoder_b_begin;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_66_EN_B_END) = (U32)ldpc_66_encoder_b_end;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_66_DE_A_BEGIN) = (U32)ldpc_66_decoder_a_begin;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_66_DE_A_END) = (U32)ldpc_66_decoder_a_end;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_66_DE_B_BEGIN) = (U32)ldpc_66_decoder_b_begin;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_66_DE_B_END) = (U32)ldpc_66_decoder_b_end;

    _REG32(sblk_addr + SBLK_OFFSET_LDPC_87_EN_A_BEGIN) = (U32)ldpc_87_encoder_a_begin;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_87_EN_A_END) = (U32)ldpc_87_encoder_a_end;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_87_EN_B_BEGIN) = (U32)ldpc_87_encoder_b_begin;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_87_EN_B_END) = (U32)ldpc_87_encoder_b_end;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_87_DE_A_BEGIN) = (U32)ldpc_87_decoder_a_begin;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_87_DE_A_END) = (U32)ldpc_87_decoder_a_end;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_87_DE_B_BEGIN) = (U32)ldpc_87_decoder_b_begin;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_87_DE_B_END) = (U32)ldpc_87_decoder_b_end;

    _REG32(sblk_addr + SBLK_OFFSET_LDPC_A8_EN_A_BEGIN) = (U32)ldpc_a8_encoder_a_begin;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_A8_EN_A_END) = (U32)ldpc_a8_encoder_a_end;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_A8_EN_B_BEGIN) = (U32)ldpc_a8_encoder_b_begin;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_A8_EN_B_END) = (U32)ldpc_a8_encoder_b_end;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_A8_DE_A_BEGIN) = (U32)ldpc_a8_decoder_a_begin;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_A8_DE_A_END) = (U32)ldpc_a8_decoder_a_end;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_A8_DE_B_BEGIN) = (U32)ldpc_a8_decoder_b_begin;
    _REG32(sblk_addr + SBLK_OFFSET_LDPC_A8_DE_B_END) = (U32)ldpc_a8_decoder_b_end;
#endif

#if defined(RL6447_VA)
    _REG08(sblk_addr + SBLK_OFFSET_LDPC_CODE_RATE) = gubLdpcCodeRate;
    _REG16(sblk_addr + SBLK_OFFSET_LDPC_PARITY_LEN) = guwLdpcParityLen;
    DbgPrintk(ALWAYS_MSG, "LDPC: %d %d\r\n", gubLdpcCodeRate, guwLdpcParityLen);
#elif defined(RL6531_VB)
    _REG16(sblk_addr + SBLK_OFFSET_FR_ECC_CFG) = guwLdpcParityLen;
    DbgPrintk(ALWAYS_MSG, "LDPC: %d %d\r\n", gubLdpcCodeRate, guwLdpcParityLen);
#else
    _REG08(sblk_addr + SBLK_OFFSET_LDPC_CODE_RATE) = gubECC_CFG;
    _REG16(sblk_addr + SBLK_OFFSET_LDPC_PARITY_LEN) = guwLdpcParityLen;
    DbgPrintk(ALWAYS_MSG, "LDPC: %d %d\r\n", gubECC_CFG, guwLdpcParityLen);
#endif

    return ERR_OK;
}

U32 llfSetSBlockFWConfig(U32 sblk_addr)
{
    int i, j;
    struct _TSB_RETRY  *pTsb_Retry_Setting;
    U8 ubDDRselect = 0;//DDR800;
#if 0
    U16 SnapshotGroupNum;
    U16 DynamicSBlockBegin;
    U16 DynamicSBlockEnd;
    U16 SnapshotGroupBegin;
    U16 SnapshotGroupEnd;
    U16 L2PGroupNum;
    U16 L2PGroupEnd;
#endif
    U16 SLCCacheGroupEnd;
    U16 SLCCacheGroupNum;
    U16 SLCBuffGroupEnd;
    U16 SLCBuffGroupNum;

    SLCCacheGroupNum = _MEM16(CONFIG_BASE_VA_ADDR + CONFIG_LLF_SLC_CACHE_NUM);
    SLCBuffGroupNum = _MEM16(CONFIG_BASE_VA_ADDR + CONFIG_LLF_SLC_BUFF_NUM);

#if 0//calcularte in calculatesnapshotarea()
    L2PGroupNum = L2P_GROUP_NUM;
    DynamicSBlockBegin = 0;
    // The Number of Block for Dynamic SBlock is depends on the number of Bank is enough to preserved for
    // current, current-backup, pre-current and pre-current-backup, so it must bigger than 4.
    if (NandPara.ubBankNum <= 4)
    {
        DynamicSBlockEnd = NandPara.ubPlaneNumPerLun * 2; // 2 * [ number of block per one BlockStripes ]
    }
    else
    {
        DynamicSBlockEnd = NandPara.ubPlaneNumPerLun;
    }

    SnapshotGroupNum = 3;

    SnapshotGroupBegin = (DynamicSBlockEnd) / NandPara.ubPlaneNumPerLun;
    SnapshotGroupEnd = SnapshotGroupBegin + SnapshotGroupNum;
    L2PGroupEnd = SnapshotGroupEnd + L2PGroupNum;
#endif

#ifdef RL6531_VB
    _MEM32(CONFIG_BASE_VA_ADDR + CONFIG_BANK_A1_ENTRY_POINT_OFFSET) = FDMINIT_RAM_BASE;
    _MEM32(CONFIG_BASE_VA_ADDR + CONFIG_BANK_A2_ENTRY_POINT_OFFSET) = NEW_BANKA2INIT_RAM_BASE;
#endif

    SLCCacheGroupEnd = guwCacll2PGroupEnd + SLCCacheGroupNum;
    SLCBuffGroupEnd = SLCCacheGroupEnd + SLCBuffGroupNum;

    ASSERT(LLF_MSG, (NandPara.ulLastMpPageByteNum & 0x000001ff) == 0);
    ASSERT(LLF_MSG, NandPara.ubBankNum <= (1 << NandPara.ubBankNumShift));

    for (i = 0; i < (SBLK_OFFSET_EUI64L - SBLK_OFFSET_FW_VERSION); i++)
    {
        _REG08(sblk_addr + SBLK_OFFSET_FW_VERSION + i) = _MEM08(CONFIG_BASE_VA_ADDR +
                CONFIG_FWVER_OFFSET + i);
    }

    _REG08(sblk_addr + SBLK_OFFSET_LLF_MODE) = gubLLFMode;
    printk("Write gubLLFMode = %x to Sblock \r\n", _REG08(sblk_addr + SBLK_OFFSET_LLF_MODE));
    _REG08(sblk_addr + SBLK_OFFSET_RDT_IMG_TAG) = gubRdtImg;
    _REG16(sblk_addr + SBLK_OFFSET_SHARED_PAGE_NUM) = _REG16(CONFIG_BASE_VA_ADDR +
            CONFIG_LLF_SHARED_PAGE_NUM);

    printk("Pure-SLC Mode: %d\r\n", gfPureSLC);
    _REG08(sblk_addr + SBLK_OFFSET_PESUDO_SLC_MODE) = gfPureSLC;

    _REG08(sblk_addr + SBLK_OFFSET_CH_NUM_SHIFT) = NandPara.ubChNumShift;
    _REG08(sblk_addr + SBLK_OFFSET_CE_NUM_SHIFT)    = llfCalLog2(NandPara.ubCENumPerCh);
    _REG08(sblk_addr + SBLK_OFFSET_LUN_NUM_SHIFT_PER_CE) = NandPara.ubLunNumPerCEShift;
    _REG08(sblk_addr + SBLK_OFFSET_PLANE_NUM_SHIFT_PER_LUN) = NandPara.ubPlaneNumPerLunShift;

    _REG08(sblk_addr + SBLK_OFFSET_BLOCK_NUM_SHIFT_PER_LUN) = NandPara.ubBlockNumPerLunShift;
    _REG08(sblk_addr + SBLK_OFFSET_PAGE_NUM_SHIFT_PER_BLOCK) = NandPara.ubPageNumPerBlockShift;
    _REG08(sblk_addr + SBLK_OFFSET_SLC_PAGE_NUM_SHIFT_PER_BLOCK) = NandPara.ubSLCPageNumPerBlockShift;
    _REG08(sblk_addr + SBLK_OFFSET_BANK_NUM_SHIFT)  = NandPara.ubBankNumShift;
    _REG08(sblk_addr + SBLK_OFFSET_MP_BLOCK_NUM_SHIFT) = NandPara.ubMpBlockNumPerLunShift;

    _REG08(sblk_addr + SBLK_OFFSET_LBN_NUM_SHIFT_PER_MP_PAGE) = NandPara.ubLbnNumPerMpPageShift;
    _REG08(sblk_addr + SBLK_OFFSET_MP_PAGE_BYTE_NUM_SHIFT) = NandPara.ubMpPageByteNumShift;
    _REG08(sblk_addr + SBLK_OFFSET_CACHE_SECTOR_NUM_SHIFT_PER_LBN) = gubCacheSectorNumPerLbnShift;

    _REG08(sblk_addr + SBLK_OFFSET_CH_NUM)    = NandPara.ubChNum;
    _REG08(sblk_addr + SBLK_OFFSET_CE_NUM)    = NandPara.ubCENumPerCh;
    _REG08(sblk_addr + SBLK_OFFSET_LUN_NUM_PER_CE)   = NandPara.ubLunNumPerCE;
    _REG08(sblk_addr + SBLK_OFFSET_PLANE_NUM_PER_LUN) = NandPara.ubPlaneNumPerLun;

    _REG16(sblk_addr + SBLK_OFFSET_BLOCK_NUM_PER_LUN) = NandPara.uwBlockNumPerLun;
    _REG16(sblk_addr + SBLK_OFFSET_PAGE_NUM_PER_BLOCK) = NandPara.uwPageNumPerBlock;
    _REG16(sblk_addr + SBLK_OFFSET_SLC_PAGE_NUM_PER_BLOCK) = NandPara.uwSLCPageNumPerBlock;

    _REG16(sblk_addr + SBLK_OFFSET_BANK_NUM)  = NandPara.ubBankNum;
    _REG16(sblk_addr + SBLK_OFFSET_MP_BLOCK_NUM) = NandPara.uwMpBlockNumPerLun;

    _REG16(sblk_addr + SBLK_OFFSET_LBN_NUM_PER_MP_PAGE) = NandPara.uwLbnNumPerMpPage;
    _REG32(sblk_addr + SBLK_OFFSET_MP_PAGE_BYTE_NUM_NEW)  = NandPara.ulLastMpPageByteNum;

    _REG16(sblk_addr + SBLK_OFFSET_CACHE_SECTOR_NUM_PER_LBN) = guwCacheSectorNumPerLbn;
    _REG16(sblk_addr + SBLK_OFFSET_BLOCK_NUM_PER_CE) = NandPara.uwBlockNumPerCE;
    _REG08(sblk_addr + SBLK_OFFSET_FC_DIFF_EN) = _MEM08(CONFIG_BASE_VA_ADDR + CONFIG_FC_DIFF_EN);
    _REG08(sblk_addr + SBLK_OFFSET_NAND_ODT_EN) = _MEM08(CONFIG_BASE_VA_ADDR + CONFIG_NAND_ODT_EN);

    _REG08(sblk_addr + SBLK_OFFSET_SECTOR_NUM_PER_PAGE) = NandPara.ubSectorNumPerPage;
#if defined(RTS5771_FPGA) || defined(RTS5771_VA)
    _REG08(sblk_addr + SBLK_OFFSET_PLANE_GROUP_NUM_PER_LUN) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_PLANE_GROUP_NUM_PER_LUN);
#else
    _REG08(sblk_addr + SBLK_OFFSET_LAST_PAGE_NUM_PER_BLOCK) = NandPara.uwLastPageNumPerBlock;
#endif
    _REG08(sblk_addr + SBLK_OFFSET_HDR_4B_LEN_FOR_PAGE) = L2pPara.ubHdr4BLenPerPage;
    ASSERT(LLF_MSG, _MEM08(CONFIG_BASE_VA_ADDR + CONFIG_LLF_GC_LIMIT) > 5);
    _REG08(sblk_addr + SBLK_OFFSET_GC_FREE_LIMIT) = _MEM08(CONFIG_BASE_VA_ADDR + CONFIG_LLF_GC_LIMIT);
    _REG08(sblk_addr + SBLK_OFFSET_SLC_CACHE_GC_FREE_LIMIT) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_LLF_SLC_CACHE_GC_LIMIT);
    _REG32(sblk_addr + SBLK_OFFSET_NA_TEST_ITEM) = _MEM32(CONFIG_BASE_VA_ADDR +
            CONFIG_NAND_TEST_ITEM);
    _REG08(sblk_addr + SBLK_OFFSET_L2P_GC_FREE_LIMIT) = 3;
    _REG08(sblk_addr + SBLK_OFFSET_IS_INTEL_B0KB) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_LLF_IS_INTEL_B0KB);

    _MEM64(sblk_addr + SBLK_OFFSET_MAX_LBA) = gulMaxLBAAddr;
    _REG32(sblk_addr + SBLK_OFFSET_FLASH_VENDOR_NUM) = gulFlashVendorNum;
    _REG16(sblk_addr + SBLK_OFFSET_MAX_PE_CYCLE) = NandPara.uwMaxPECycle;
#ifdef MST_MERGE
    _REG08(sblk_addr + SBLK_OFFSET_MST_MERGE) = gubLlfMSTMergeEnable;
#endif
    _REG08(sblk_addr + SBLK_OFFSET_MARK_TOTAL_BAD) = _REG08(CONFIG_BASE_VA_ADDR +
            CONFIG_MARK_TOTAL_BAD);
    _REG16(sblk_addr + SBLK_OFFSET_LATER_BAD_THRESHOLD) = _REG16(CONFIG_BASE_VA_ADDR +
            CONFIG_LATER_BAD_THRESHOLD);
    _REG16(sblk_addr + SBLK_OFFSET_SYSTEM_BLOCK_BEGIN_INDEX)	  = guwCaclDynSBlockBegin;
    _REG16(sblk_addr + SBLK_OFFSET_SYSTEM_BLOCK_END_INDEX)		  = guwCaclDynSBlockEnd;
    _REG16(sblk_addr + SBLK_OFFSET_SNAP_MP_BLOCK_BEGIN_INDEX)     = guwCaclSSGroupBegin;
    _REG16(sblk_addr + SBLK_OFFSET_SNAP_MP_BLOCK_END_INDEX)       = guwCaclSSGroupEnd;

    _REG16(sblk_addr + SBLK_OFFSET_L2P_MP_BLOCK_END_INDEX)		  = guwCacll2PGroupEnd;

#if defined LLF_CHECK_SYSTEMBLK_ERASE_FAIL
#ifndef SBLK_EXPAND
    gulSysblk &= ~gulEraseFailSysblk;//for system block that erase fail but not factory bad
#else
    U8 bank_no, block_no, ch_no, ce_no;
    U8 ubBankNum = (NandPara.ubBankNum > SYS_BANK_NUM) ? SYS_BANK_NUM : NandPara.ubBankNum;
    for(bank_no = 0; bank_no < ubBankNum; bank_no++)
    {
        for(block_no = 0; block_no < SYS_BLK; block_no++)
        {
            if(gulEraseFailSysblk & (1 << (bank_no * SYS_BLK + block_no)))
            {
                ch_no = (_MEM08(BANK_IMAPPING_TABLE_ADDR + bank_no + gubSblkBankStart) & 0xFF) >> 4;
                ce_no = (_MEM08(BANK_IMAPPING_TABLE_ADDR + bank_no + gubSblkBankStart) & 0xF);
                gulSblkCHCEMap[ch_no] &= ~(1 << ((ce_no << SYS_BLK_SHIFT) + block_no));
                guwSBlkNo[bank_no][block_no] = 0xff;
            }
        }
    }
#endif
#endif
#if defined(RL6531_VB)
    _REG32(sblk_addr + SBLK_OFFSET_SYS_BLK_CFG_L)  = (gulSysblk & 0xffff);
    _REG32(sblk_addr + SBLK_OFFSET_SYS_BLK_CFG_H)  = (gulSysblk >> 16);
#else
    _REG32(sblk_addr + SBLK_OFFSET_SYS_BLK_CFG2)  = gulSysblk;
#endif

    _REG16(sblk_addr + SBLK_OFFSET_SLC_CACHE_MP_BLOCK_END_INDEX)        = SLCCacheGroupEnd;
    _REG16(sblk_addr + SBLK_OFFSET_SLC_BUFF_MP_BLOCK_END_INDEX)        = SLCBuffGroupEnd;

    _REG08(sblk_addr + SBLK_OFFSET_MP_MODE_SELECT)   = gubMpModeSelect;
    _REG32(sblk_addr + SBLK_OFFSET_DEBUG_LEVEL_SELECT) = DebugPara.ulDebugLevel;
    _REG08(sblk_addr + SBLK_OFFSET_END_OF_LIFE)              = 0;
    // TODO::: The real driving is be created by calibrating.
    _REG16(sblk_addr + SBLK_OFFSET_DEFAULT_DRIVING_SETTING)     = NandPara.uwDrivingSetting;
    _REG16(sblk_addr + SBLK_OFFSET_REAL_DRIVING_SETTING)        = NandPara.uwDrivingSetting;
    //_REG08(sblk_addr + SBLK_OFFSET_LDPC_TABLE_EXIST)	= _REG08(CONFIG_BASE_VA_ADDR +
    //        SBLK_OFFSET_LDPC_TABLE_EXIST);

    _REG16(sblk_addr + SBLK_OFFSET_FULL_PAGE_PER_BLOCK)  = _REG16(CONFIG_BASE_VA_ADDR +
            CONFIG_FULL_PAGE_PER_BLOCK);

    _REG16(sblk_addr + SBLK_OFFSET_CODE_SIEZ_PAGE_NUM) = (FW_CODE_SIZE +
            (NandPara.ubSectorNumPerPage * 512) - 1) / (NandPara.ubSectorNumPerPage * 512);
    _REG16(sblk_addr + SBLK_OFFSET_CODE_SIEZ_PAGE_NUM)  = TOTAL_IMEM_CODE_SIZE /
            (NandPara.ulLastMpPageByteNum / NandPara.ubPlaneNumPerLun);
    _REG08(sblk_addr + SBLK_OFFSET_REREAD_TIMES )  = _REG08(CONFIG_BASE_VA_ADDR +
            SBLK_OFFSET_REREAD_TIMES);
    _REG08(sblk_addr + SBLK_OFFSET_RC_RETRY_TIMES )  = _REG08(CONFIG_BASE_VA_ADDR +
            SBLK_OFFSET_RC_RETRY_TIMES);
    // Toshiba read retry setting
    pTsb_Retry_Setting = (struct _TSB_RETRY *)TSB_RETRY_SETTING_ADDR;

    for (i = 0; i < 4; i++)
    {
        _REG08(sblk_addr + SBLK_OFFSET_TSB_RERTY_TABLE_BASE + 0x000 + i) =
            pTsb_Retry_Setting->ubTsbRetryPreCmd[i];
    }

    for (i = 0; i < 4; i++)
    {
        _REG08(sblk_addr + SBLK_OFFSET_TSB_RERTY_TABLE_BASE + 0x004 + i) =
            pTsb_Retry_Setting->ubTsbTriggerRetryCmd[i];
    }

    for (i = 0; i < 20; i++)
    {
        _REG08(sblk_addr + SBLK_OFFSET_TSB_RERTY_TABLE_BASE + 0x008 + i) =
            pTsb_Retry_Setting->ubTsbRetryAddr[i];
    }

    for (i = 0; i < 4; i++)
    {
        _REG08(sblk_addr + SBLK_OFFSET_TSB_RERTY_TABLE_BASE + 0x01C + i) =
            pTsb_Retry_Setting->ubTsbRetryEndCmd[i];
    }

    for (i = 0; i < 20; i++)
    {
        for (j = 0; j < 8; j++)
            _REG08(sblk_addr + SBLK_OFFSET_TSB_RERTY_TABLE_BASE + 0x020 + i * 8 + j) =
                pTsb_Retry_Setting->ubData[i][j];
    }

    _REG08(sblk_addr + SBLK_OFFSET_TSB_RERTY_TABLE_BASE + 0x0C0) = pTsb_Retry_Setting->ubMaxRetry;
    _REG08(sblk_addr + SBLK_OFFSET_TSB_RERTY_TABLE_BASE + 0x0C1) = pTsb_Retry_Setting->ubCmdSetNum;

    for(i = 0; i < 16; i++) // ce mapping from configs
    {
        _REG32(sblk_addr + SBLK_OFFSET_FC_DIE_MAPPING_CE + i * 4) = _MEM32(CONFIG_BASE_VA_ADDR +
                CONFIG_FC_DIE_MAPPING_CE + i * 4);;
    }

    _REG16(sblk_addr + SBLK_OFFSET_FC_TABLE_VERSION) = (FC_TABLE_VALID | FC_TABLE_VERSION); //V1
    _REG16(sblk_addr + SBLK_OFFSET_FC_PARS_TABLE_SIZE) = FC_PARSER_TABLE_SIZE;
    _REG16(sblk_addr + SBLK_OFFSET_FC_SEQS_TABLE_SIZE) = FC_SENQUENCER_TABLE_SIZE;
    _REG08(sblk_addr + SBLK_OFFSET_FW_UPDATE_TAG) = 0;

    _REG16(sblk_addr + SBLK_OFFSET_LDPC_G_MATRIX_SIZE) = FC_LDPC_G_MATRIX_SIZE;
    _REG16(sblk_addr + SBLK_OFFSET_LDPC_ET_MATRIX_SIZE) = FC_LDPC_ET_MATRIX_SIZE;
    _REG16(sblk_addr + SBLK_OFFSET_LLR_MATRIX_SIZE) = FC_LDPC_LLR_MATRIX_SIZE;

    _REG32(sblk_addr + SBLK_OFFSET_AES_KEY0)  = _MEM32(CONFIG_BASE_VA_ADDR + CONFIG_AES_KEY0);
    _REG32(sblk_addr + SBLK_OFFSET_AES_KEY1)  = _MEM32(CONFIG_BASE_VA_ADDR + CONFIG_AES_KEY1);
    _REG32(sblk_addr + SBLK_OFFSET_AES_KEY2)  = _MEM32(CONFIG_BASE_VA_ADDR + CONFIG_AES_KEY2);
    _REG32(sblk_addr + SBLK_OFFSET_AES_KEY3)  = _MEM32(CONFIG_BASE_VA_ADDR + CONFIG_AES_KEY3);
    _REG32(sblk_addr + SBLK_OFFSET_AES_KEY4)  = _MEM32(CONFIG_BASE_VA_ADDR + CONFIG_AES_KEY4);
    _REG32(sblk_addr + SBLK_OFFSET_AES_KEY5)  = _MEM32(CONFIG_BASE_VA_ADDR + CONFIG_AES_KEY5);
    _REG32(sblk_addr + SBLK_OFFSET_AES_KEY6)  = _MEM32(CONFIG_BASE_VA_ADDR + CONFIG_AES_KEY6);
    _REG32(sblk_addr + SBLK_OFFSET_AES_KEY7)  = _MEM32(CONFIG_BASE_VA_ADDR + CONFIG_AES_KEY7);
    // _REG32(sblk_addr + SBLK_OFFSET_AES_MODE_SEL)  = _MEM32(CONFIG_BASE_VA_ADDR + CONFIG_AES_MODE_SEL);
    _REG08(sblk_addr + SBLK_OFFSET_SDRC_PAGE_NUM) = _MEM08(CONFIG_BASE_VA_ADDR + CONFIG_SDRC_PAGE_NUM);
    _REG08(sblk_addr + SBLK_OFFSET_NORMAL_CAPACITY_OP) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_NORMAL_CAPACITY_OP);

    _REG08(sblk_addr + SBLK_OFFSET_SEARIAL_MULTILUN) = gubIsSerialMultiLUN;
#ifdef AUTO_DETECT_DIE
    _REG08(sblk_addr + SBLK_OFFSET_REAL_LUN_NUM) = gubRealLunNum;
    _REG32(sblk_addr + SBLK_OFFSET_BANK_TRUE_LUN) = gulBankMapPro[0];
    _REG32(sblk_addr + SBLK_OFFSET_BANK_TRUE_LUN + 4) = gulBankMapPro[1];

    //printk("[ADD] RealLunNum %d BankMap %x %x\r\n", gubRealLunNum, gulBankMapPro[1], gulBankMapPro[0]);
    for (i = 0; i < CH_NUM_MAX; i++)
    {
        _REG32(sblk_addr + SBLK_OFFSET_LUN_MAP + (8 * i)) = gulCHDieMap[i][0];
        _REG32(sblk_addr + SBLK_OFFSET_LUN_MAP + (8 * i) + 4) = gulCHDieMap[i][1];
        //printk("[ADD] CH %d DieMap %x %x\r\n", i, gulCHDieMap[i][1], gulCHDieMap[i][0]);
    }
#else
    _REG08(sblk_addr + SBLK_OFFSET_REAL_LUN_NUM) = _MEM08(CONFIG_BASE_VA_ADDR + CONFIG_LUN_NUM_OFFSET);
#endif
    _REG08(sblk_addr + SBLK_OFFSET_ROWADDR_LUN_SHIFT) = gubRowAddrLunShift;

#ifndef RL6531_VB
    _REG16(sblk_addr + SBLK_OFFSET_WQOS_LIMIT) = _MEM16(CONFIG_BASE_VA_ADDR + SBLK_OFFSET_WQOS_LIMIT);
    _REG16(sblk_addr + SBLK_OFFSET_RQOS_LIMIT) = _MEM16(CONFIG_BASE_VA_ADDR + SBLK_OFFSET_RQOS_LIMIT);
#endif
#if defined(RL6643_VA) || defined(RL6855_VA)
    _REG08(sblk_addr + SBLK_OFFSET_VIRTUAL_BANK_NUM) = _MEM08(CONFIG_BASE_VA_ADDR +
            SBLK_OFFSET_VIRTUAL_BANK_NUM);
    _REG16(sblk_addr + SBLK_OFFSET_VIRTUAL_BS_START) = _MEM08(CONFIG_BASE_VA_ADDR +
            SBLK_OFFSET_VIRTUAL_BS_START);
#endif

//=================================Store RDT setting START====================================//
    _REG08(sblk_addr + SBLK_OFFSET_RDT_TEST_OPTION1)  = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_TEST_OPTION1);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_TEST_OPTION2)  = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_TEST_OPTION2);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_TEST_OPTION3)  = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_TEST_OPTION3);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_MAX_ROUND)  = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_MAX_ROUND);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_MIXTEST_MAX_ROUND)  = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_MIXTEST_MAX_ROUND);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_IS_NONSLCMODE_TAG)  = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_IS_NONSLCMODE_TAG);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_SLC_ROUND_MODE)  = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG__SLC_ROUND_MODE);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_READ_WRITE_FLOW_MODE)  = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_READ_WRITE_FLOW_MODE);
    _REG16(sblk_addr + SBLK_OFFSET_RDT_MP_TEST_TIME)  = _MEM16(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_MP_TEST_TIME);
//  _REG16(sblk_addr + SBLK_OFFSET_RDT_TIME_LIMIT)  = _MEM16(CONFIG_BASE_VA_ADDR +  //unrealized
//          CONFIG_RDT_TIME_LIMIT);
    _REG16(sblk_addr + SBLK_OFFSET_RDT_RFAIL_THRESHOLD)  = _MEM16(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_RFAIL_THRESHOLD);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_SLCRFAIL_THRESHOLD)  = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_SLCRFAIL_THRESHOLD);
    _REG16(sblk_addr + SBLK_OFFSET_RDT_DEFECT_RATIO_PER_DIE)  = _MEM16(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_DEFECT_RATIO_PER_DIE);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_TLCECCSAFE_THRESHOLD)  = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_TLCECCSAFE_THRESHOLD);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_OPENREADECC_THRESHOLD)  = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_OPENREADECC_THRESHOLD);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_MAX_ERASE_TIME)  = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_MAX_ERASE_TIME);
    _REG16(sblk_addr + SBLK_OFFSET_RDT_WRITE_REQUEST)  = _MEM16(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_WRITE_REQUEST);
    _REG16(sblk_addr + SBLK_OFFSET_RDT_CACHE_WRITE_REQUEST)  = _MEM16(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_CACHE_WRITE_REQUEST);
    _REG16(sblk_addr + SBLK_OFFSET_RDT_TLC_WRITE_REQUEST)  = _MEM16(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_TLC_WRITE_REQUEST);
    _REG16(sblk_addr + SBLK_OFFSET_RDT_DIFFECC_NUMINBANK)  = _MEM16(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_DIFFECC_NUMINBANK);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_ECCFAILLIMIT_InDIFFROUND)  = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_ECCFAILLIMIT_InDIFFROUND);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_PATTERN_MODE)  = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_PATTERN_MODE);
    _REG32(sblk_addr + SBLK_OFFSET_RDT_PATTERN_0)  = _MEM32(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_PATTERN_0);
    _REG32(sblk_addr + SBLK_OFFSET_RDT_PATTERN_1)  = _MEM32(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_PATTERN_1);
    _REG32(sblk_addr + SBLK_OFFSET_RDT_PATTERN_2)  = _MEM32(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_PATTERN_2);
    _REG32(sblk_addr + SBLK_OFFSET_RDT_PATTERN_3)  = _MEM32(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_PATTERN_3);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_LLF_NORMAL_EN)  = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_LLF_NORMAL_EN);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_LLF_PER_DIE_EN)  = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_LLF_PER_DIE_EN);
//	_REG08(sblk_addr + SBLK_OFFSET_RDT_REREAD_TIMES)	= _MEM08(CONFIG_BASE_VA_ADDR +  //unrealized
//			CONFIG_RDT_REREAD_TIMES);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_TLC_RETRY_CNT)	= _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_TLC_RETRY_CNT);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_STRESS_CYCLE)	= _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_STRESS_CYCLE);
//  _REG08(sblk_addr + SBLK_OFFSET_RDT_WRITE_SEQUENCE_CYCLE)	= _MEM08(CONFIG_BASE_VA_ADDR +  //unrealized
//		    CONFIG_RDT__WRITE_SEQUENCE_CYCLE);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_RETRYPAGENUMLIMT)	= _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_RETRYPAGENUMLIMT);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_TLCSTRESSTESTCYCLE)	= _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_TLCSTRESSTESTCYCLE);
    _REG08(sblk_addr + SBLK_OFFSET_RDTDEFECTBSLIMIT)	= _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_RDTDEFECTBSLIMIT);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_CPU_CLK_DIV)	= _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_CPU_CLK_DIV);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_CUSTOMER) = _MEM08(CONFIG_BASE_VA_ADDR + CONFIG_RDT_CUSTOMER);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_SPEEDTEST_GROUP_NUM) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_SPEEDTEST_GROUP_NUM);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_SPEEDTEST_MB_GAP) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_SPEEDTEST_MB_GAP);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_CUTOFF_LOW_TEMPERATURE)	= _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_CUTOFF_LOW_TEMPERATURE);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_CUTOFF_TEMPERATURE)	= _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_CUTOFF_TEMPERATURE);
#ifndef RL6531_VB
    _REG16(sblk_addr + SBLK_OFFSET_RDT_LATER_BAD_THRESHOLD)	= _MEM16(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_LATER_BAD_THRESHOLD);
#endif
    _REG32(sblk_addr + SBLK_OFFSET_RDT_STRESSTEST_SLC_PAGE)	= _MEM32(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_STRESSTEST_SLC_PAGE);
    _REG32(sblk_addr + SBLK_OFFSET_RDT_STRESSTEST_TLC_PAGE0)	= _MEM32(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_STRESSTEST_TLC_PAGE0);
    _REG32(sblk_addr + SBLK_OFFSET_RDT_STRESSTEST_TLC_PAGE1)	= _MEM32(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_STRESSTEST_TLC_PAGE1);
    _REG32(sblk_addr + SBLK_OFFSET_RDT_STRESSTEST_TLC_PAGE2)	= _MEM32(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_STRESSTEST_TLC_PAGE2);
    _REG08(sblk_addr + SBLK_OFFSET_WSRDT_TEST_OPTION)	= _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_WSRDT_TEST_OPTION);
    _REG16(sblk_addr + SBLK_OFFSET_RDT_LED_CUSTOM_MADE) = _REG16(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_LED_CUSTOM_MADE);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_OPENREAD_MARKBAD)	= _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_OPENREAD_MARKBAD);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_RRT_CTR)	= _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_RRT_CTR);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_SINGLE_TEST_CTR)	= _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_SINGLE_TEST_CTR);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_MARK_PF_CTR)	= _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_MARK_PF_CTR);
#if defined(RL6577_VA) || defined(RTS5771_VA)
    if(gubRdtImg == 1)
    {
        if(gubLlfMSTMergeEnable == 1)
        {
            _REG08(sblk_addr + SBLK_OFFSET_RDT_EXTEND_RESULT_BLK) = gubLlfMSTMergeEnable;
        }
        else
        {
            _REG08(sblk_addr + SBLK_OFFSET_RDT_EXTEND_RESULT_BLK) = _MEM08(CONFIG_BASE_VA_ADDR +
                    SBLK_OFFSET_RDT_EXTEND_RESULT_BLK);
        }
    }
    else
    {
        _REG08(sblk_addr + SBLK_OFFSET_RDT_EXTEND_RESULT_BLK) = gubExtendRdtResultBlk;
    }
#endif
    _REG08(sblk_addr + SBLK_OFFSET_RDT_MULTI_CORE_OPTION)	= _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_MULTI_CORE_OPTION);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_DISABLE_LINK_RAMDISK)	= _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_DISABLE_LINK_RAMDISK);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_AFTERFIRSTROUND_MARKBANK)	= _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_AFTERFIRSTROUND_MARKBANK);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_WriteSequence_CNT) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_WriteSequenceCount);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_SnapReadTest) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_SnapReadTest);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_WriteReadMixTest) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_WriteReadMixTest);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_SLCSpecialTestRound) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_SLCSpecialTestRound);
    _REG32(sblk_addr + SBLK_OFFSET_RDT_RETRY_INDEX_SHIFT) = _MEM32(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_RETRY_INDEX_SHIFT);
    if((gubRdtImg == 1) || (((gubFWFeatureSetting & 0x40) != 0) && (gubLLFMode != LLF_FORCE_FORMAT)))
    {
        _REG16(sblk_addr + SBLK_OFFSET_RDT_DEFECT_THRESHOLD)  = _MEM16(CONFIG_BASE_VA_ADDR +
                CONFIG_RDT_DEFECT_THRESHOLD);
        _REG16(sblk_addr + SBLK_OFFSET_RDT_ECC_THRESHOLD)  = _MEM16(CONFIG_BASE_VA_ADDR +
                CONFIG_RDT_ECC_THRESHOLD);
        _REG08(sblk_addr + SBLK_OFFSET_RDT_SLCECC_THRESHOLD)  = _REG08(CONFIG_BASE_VA_ADDR +
                CONFIG_RDT_SLCECC_THRESHOLD);
        _REG16(sblk_addr + SBLK_OFFSET_RDT_PFAIL_THRESHOLD)  = _MEM16(CONFIG_BASE_VA_ADDR +
                CONFIG_RDT_PFAIL_THRESHOLD);
        _REG16(sblk_addr + SBLK_OFFSET_RDT_EFAIL_THRESHOLD)  = _MEM16(CONFIG_BASE_VA_ADDR +
                CONFIG_RDT_EFAIL_THRESHOLD);
    }
    _REG08(sblk_addr + SBLK_OFFSET_RDT_BLK_RETRY_RATIO)  = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_BLK_RETRY_RATIO);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_BANK_RETRY_RATIO)  = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_BANK_RETRY_RATIO);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_PF2RF_CNT)  = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_PF2RFCNT);
    _REG16(sblk_addr + SBLK_OFFSET_RDT_MODE4_DELAY)  = _MEM16(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_MODE4_DELAY);
    _REG16(sblk_addr + SBLK_OFFSET_RDT_MODE4_BATCHSIZE)  = _MEM16(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_MODE4_BATCHSIZE);
    _REG32(sblk_addr + SBLK_OFFSET_UNICORE_VERSION) = _MEM32(CONFIG_BASE_VA_ADDR +
            SBLK_OFFSET_UNICORE_VERSION);
    _REG32(sblk_addr + SBLK_OFFSET_UNICORE_VERSION + 4) = _MEM32(CONFIG_BASE_VA_ADDR +
            SBLK_OFFSET_UNICORE_VERSION + 4);
    _REG08(sblk_addr + SBLK_OFFSET_RDT_WRMIX_MARK_BANK) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_WRMIX_MARK_BANK);
    _REG32(sblk_addr + SBLK_OFFSET_RDT_ROUND_MODE) = _MEM32(CONFIG_BASE_VA_ADDR +
            CONFIG_RDT_ROUND_MODE);

//=================================Store RDT setting END====================================//

//=============store the ddr setting===================
#if defined(RL6447_VA)
    for(i = (DDRCLK_NUM - 1); i >= 0; i--)
    {
        if(DramPara_default[i].frequency & DDR_FREQ_VALID)
        {
            // 0:ddr800  1:ddr1066   2:ddr1333
            ubDDRselect = i;
            break;
        }
    }
#endif

    _REG08(sblk_addr + SBLK_OFFSET_DDR_MODE) = ubDDRselect;

    _REG08(sblk_addr + SBLK_OFFSET_DDR_ROW_BIT) = gubdram_row_bit;
    _REG08(sblk_addr + SBLK_OFFSET_DDR_COL_BIT) = gubdram_colum_bit;
    _REG08(sblk_addr + SBLK_OFFSET_DDR_DQ_WIDTH) = gubdram_dq_width;

    _REG08(sblk_addr + SBLK_OFFSET_FC_DIFF_EN) = gubFCDiffEnable;
    _REG08(sblk_addr + SBLK_OFFSET_NAND_ODT_EN) = gubNANDODTEnable;
    _REG08(sblk_addr + SBLK_OFFSET_NAND_ODT_CFG) = gubNANDODTCfg;
    _REG08(sblk_addr + SBLK_OFFSET_NAND_VREF_EN) = gubNANDVrefEn;
    _REG08(sblk_addr + SBLK_OFFSET_NAND_DRISTR) = gubNandDriv;
#ifdef RL6531_VB
    _REG08(sblk_addr + SBLK_OFFSET_NAND_TSB_DRISTR) = gubNandDriv;
#else
    _REG32(sblk_addr + SBLK_OFFSET_FC_DRIVESTRENGTH) = (gulFc_ocd & 0xf) | 0xc0;
    _REG32(sblk_addr + SBLK_OFFSET_FR_PAD_DQS_ODT_CFG) = (gulFc_dqs_odt & 0xf) | 0xc0;
    _REG32(sblk_addr + SBLK_OFFSET_FR_PAD_DQ_RE_ODT_CFG) = (gulFc_dq_re_odt & 0xf) | 0xc0;
    _REG32(sblk_addr + SBLK_OFFSET_FR_PAD_DQS_ODT_CTRL) = (gulFc_dqs_odt_en & 0xff) | 0xc00;
    _REG32(sblk_addr + SBLK_OFFSET_FR_PAD_DQ_RE_ODT_CTRL) = (gulFc_dq_re_odt_en & 0xff) | 0xc00;
#endif
#if defined(RL6577_VA)
    _REG08(sblk_addr + SBLK_OFFSET_ZQ_K_EN) = gubZQKEn;
#endif
    _REG08(sblk_addr + SBLK_OFFSET_STABLE_ENTRYSIZE_SHIFT) =
        _MEM08(CONFIG_BASE_VA_ADDR + CONFIG_STABLE_ENTRYSIZE_SHIFT);

    _REG32(sblk_addr + SBLK_OFFSET_SYS_BANKMAPPING) = _MEM32(CONFIG_BASE_VA_ADDR +
            CONFIG_SYS_BANKMAPPING);
    _REG32(sblk_addr + SBLK_OFFSET_FLASH_TYPE) = _MEM32(CONFIG_BASE_VA_ADDR + CONFIG_FLASH_TYPE);

#ifndef RL6531_VB
    for(i = 0; i < 8; i++)
    {
        _REG32(sblk_addr + SBLK_OFFSET_TCG_OPAL_PSID + i * 4) = _MEM32(CONFIG_BASE_VA_ADDR +
                SBLK_OFFSET_TCG_OPAL_PSID + i * 4);
        llfprintk("[OPAL] PSID: 0x%x\r\n", _MEM32(sblk_addr + SBLK_OFFSET_TCG_OPAL_PSID + i * 4));
    }
    _REG08(sblk_addr + SBLK_OFFSET_TCG_SSC_SUPPORT) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_TCG_SSC_SUPPORT);
    llfprintk("[OPAL] SUPPORT TCG: 0x%x\r\n", _REG08(sblk_addr + SBLK_OFFSET_TCG_SSC_SUPPORT));
#endif
#if defined(RL6577_VA)
    _REG08(sblk_addr + SBLK_OFFSET_REDUCE_DATA_BUFF_CNT) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_REDUCE_DATA_BUFF_CNT);
#endif

    // temperature control
    _REG08(sblk_addr + SBLK_OFFSET_TEMPERATURE_OFFSET) = gubTemperatureOffset;
    _REG08(sblk_addr + SBLK_OFFSET_TEMPERATURE_MAX) = gubTemperatureMax;
    _REG08(sblk_addr + SBLK_OFFSET_TEMPERATURE_MIN) = gubTemperatureMin;
#if defined(RL6577_VA) || defined(RTS5771_VA) || defined(RTS5771_FPGA)
    _REG08(sblk_addr + SBLK_OFFSET_EXT_TT1) = _MEM08(CONFIG_BASE_VA_ADDR + CONFIG_EXT_TT1);
    _REG08(sblk_addr + SBLK_OFFSET_TEMPERATURE_LIMIT) = gubTemperatureLimit;
    _REG08(sblk_addr + SBLK_OFFSET_EXT_TT2) = _MEM08(CONFIG_BASE_VA_ADDR + CONFIG_EXT_TT2);
#else
    _REG16(sblk_addr + SBLK_OFFSET_TEMPERATURE_LIMIT) = guwTemperatureLimit;
#endif
    _REG08(sblk_addr + SBLK_OFFSET_FIX_TEMPERATURE) = gubFixTemperature;
    _REG08(sblk_addr + SBLK_OFFSET_SMART_INFO) = gubSmartInfo;
    _REG08(sblk_addr + SBLK_OFFSET_SMART_INFO1) = _MEM08(CONFIG_BASE_VA_ADDR + CONFIG_SMART_INFO1);
    _REG16(sblk_addr + SBLK_OFFSET_TM1) = _MEM16(CONFIG_BASE_VA_ADDR + CONFIG_TM1);
    _REG16(sblk_addr + SBLK_OFFSET_TM1_LOWER) = _MEM16(CONFIG_BASE_VA_ADDR + CONFIG_TM1_LOWER);
    _REG16(sblk_addr + SBLK_OFFSET_TM2) = _MEM16(CONFIG_BASE_VA_ADDR + CONFIG_TM2);
    _REG16(sblk_addr + SBLK_OFFSET_TM2_LOWER) = _MEM16(CONFIG_BASE_VA_ADDR + CONFIG_TM2_LOWER);

#ifdef RL6643_VA
    _REG08(sblk_addr + SBLK_OFFSET_FE_0x212_LOW_BYTE) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_FE_0x212_LOW_BYTE);
    _REG08(sblk_addr + SBLK_OFFSET_FE_0x21E_HIGH_BYTE) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_FE_0x21E_HIGH_BYTE);
    _MEM16(sblk_addr + SBLK_OFFSET_PERFORMANCE_CTRL) = _MEM16(CONFIG_BASE_VA_ADDR +
            CONFIG_PERFORMANCE_CTRL);
#endif


#ifdef BLK_REMAP_PRO
#ifdef RDT_REMAP
    if(_MEM32(FW_RDT_TAG) == SIGNATURE_RDT)
    {
        _MEM08(sblk_addr + SBLK_OFFSET_REMAP_TAG) = _MEM08(CONFIG_BASE_VA_ADDR + SBLK_OFFSET_REMAP_TAG);
        _MEM16(sblk_addr + SBLK_OFFSET_REMAP_TABLE_PER_BANK) = _MEM16(CONFIG_BASE_VA_ADDR +
                CONFIG_REMAP_TABLE_PER_BANK);
    }
    else
#endif
    {
        _MEM32(sblk_addr + SBLK_OFFSET_REMAP_START) 	=  NandPara.uwMpBlockNumPerLun;
        _MEM32(sblk_addr + SBLK_OFFSET_REAL_BLOCK_NUM)	=  guwRealMPBlkNum;
        _MEM16(sblk_addr + SBLK_OFFSET_REMAP_TABLE_PER_BANK)  =  guwReMapTableSizePerBank;
        llfprintk("Remap start %d, guwRealMPBlkNum %d, Perbank lenth is %d bytes\r\n",
                  _MEM32(sblk_addr + SBLK_OFFSET_REMAP_START)
                  , guwRealMPBlkNum, guwReMapTableSizePerBank);
        //_REG32(sblk_addr + SBLK_OFFSET_REMAP_CHECK)	  = guwRemapTableCheck;
    }
#endif

#ifdef IS_NEXTGEN
    _MEM16(sblk_addr + SBLK_OFFSET_DIRECT_TLC_AMOUNT) = _MEM16(CONFIG_BASE_VA_ADDR +
            CONFIG_DIRECT_TLC_AMOUNT);
    llfprintk("[DT]Driect TLC amount:%d(GB)\r\n",
              _MEM16(CONFIG_BASE_VA_ADDR + SBLK_OFFSET_DIRECT_TLC_AMOUNT));
#endif

#ifdef REPLACE_IMAGE
#ifdef REPLACE_IMAGE_DEBUG
    llfprintk("[RI][DEBUG] write RINum(%x) to SBLK\r\n", gulReplaceImageNum);
#endif
    _REG32(sblk_addr + SBLK_OFFSET_REPLACE_IMAGE) = gulReplaceImageNum;
#endif

#ifdef COM_BANKING
    _REG32(sblk_addr + SBLK_OFFSET_FAIL_IMAGE_BITMAP) = gulFailedImageBitMap;
    llfDbgPrintk(ALWAYS_MSG, "ulFailedImageBitMap:0x%x\r\n", gulFailedImageBitMap);
#endif

#if defined(RL6643_VA) || defined(RL6531_VB)
    _MEM08(sblk_addr + SBLK_OFFSET_FCCLKTHROT_EN) = _MEM08(CONFIG_BASE_VA_ADDR + CONFIG_FCCLKTHROT_EN);
    _MEM08(sblk_addr + SBLK_OFFSET_TEMPTHROT_UPPER) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_TEMPTHROT_UPPER);
    _MEM08(sblk_addr + SBLK_OFFSET_TEMPTHROT_LOWER) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_TEMPTHROT_LOWER);
    _MEM08(sblk_addr + SBLK_OFFSET_HIGHTEMPTHROT_UPPER) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_HIGHTEMPTHROT_UPPER);
    _MEM08(sblk_addr + SBLK_OFFSET_HIGHTEMPTHROT_LOWER) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_HIGHTEMPTHROT_LOWER);
    _MEM08(sblk_addr + SBLK_OFFSET_LOWTEMPTHROT_UPPER) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_LOWTEMPTHROT_UPPER);
    _MEM08(sblk_addr + SBLK_OFFSET_LOWTEMPTHROT_LOWER) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_LOWTEMPTHROT_LOWER);
    _MEM08(sblk_addr + SBLK_OFFSET_TEMPTHROT_FC_CLOCK) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_TEMPTHROT_FC_CLOCK);
    _MEM16(sblk_addr + SBLK_OFFSET_LOWTEMPTHROT_POWERON_LIMIT) = _MEM16(CONFIG_BASE_VA_ADDR +
            CONFIG_LOWTEMPTHROT_POWERON_LIMIT);

    if(_MEM08(CONFIG_BASE_VA_ADDR + CONFIG_FCCLKTHROT_EN) != 0) //enable temperature throtting
    {
        llfprintk("Enable FC throtting, Super High: _%d_%d, High _%d_%d\r\n",
                  _MEM08(sblk_addr + SBLK_OFFSET_TEMPTHROT_UPPER),
                  _MEM08(sblk_addr + SBLK_OFFSET_TEMPTHROT_LOWER),
                  _MEM08(sblk_addr + SBLK_OFFSET_HIGHTEMPTHROT_UPPER),
                  _MEM08(sblk_addr + SBLK_OFFSET_HIGHTEMPTHROT_LOWER));
        llfprintk("Enable FC throtting,LOW _%d_%d, FC CLK %d ,limit %d\r\n",
                  _MEM08(sblk_addr + SBLK_OFFSET_LOWTEMPTHROT_UPPER),
                  _MEM08(sblk_addr + SBLK_OFFSET_LOWTEMPTHROT_LOWER),
                  _MEM08(sblk_addr + SBLK_OFFSET_TEMPTHROT_FC_CLOCK),
                  _MEM16(sblk_addr + SBLK_OFFSET_LOWTEMPTHROT_POWERON_LIMIT));
    }
#endif

#ifdef AVG_ERASE_COUNT_TEST
    CheckInheritEC();
    _REG32(sblk_addr + SBLK_OFFSET_DATA_BS_AVG_EC) = gulDataBsAvgEraseCnt;
    _REG32(sblk_addr + SBLK_OFFSET_L2P_BS_AVG_EC) = gulL2PBsAvgEraseCnt;
    llfprintk("Write DataBSAvgEC = %x, L2PBSAvgEC = %x to Sblock(in [LLF] mode)\r\n",
              gulDataBsAvgEraseCnt, gulL2PBsAvgEraseCnt);
#endif
#if defined(RL6643_VA)
    _MEM08(sblk_addr + SBLK_OFFSET_PAGE_NUM_PER_RAID) = gubRaidPagePerRaid;
    _MEM08(sblk_addr + SBLK_OFFSET_PAGE_NUM_PER_RAID_TLC) = gubTLCRaidPagePerRaid;
#endif
    _REG08(sblk_addr + SBLK_OFFSET_IDLE_THRESHOLD) = _MEM08(CONFIG_BASE_VA_ADDR +
            SBLK_OFFSET_IDLE_THRESHOLD);
    _REG08(sblk_addr + SBLK_OFFSET_NOTR_GAP_THRESHOLD) = _MEM08(CONFIG_BASE_VA_ADDR +
            SBLK_OFFSET_NOTR_GAP_THRESHOLD);
#if (!(defined(RTS5771_VA) || defined(RTS5771_FPGA)))
    _REG08(sblk_addr + SBLK_OFFSET_SMART_MAX_TBW) = _MEM08(CONFIG_BASE_VA_ADDR +
            SBLK_OFFSET_SMART_MAX_TBW);
#endif
    for(i = 0; i < 8; i++)
    {
        _MEM16(sblk_addr + SBLK_OFFSET_AUTO_CAPACITY_0 + i * 2) = gulAutoCapacity[i];
    }
#if (!(defined(RTS5771_VA) || defined(RTS5771_FPGA)))
    _MEM08(sblk_addr + SBLK_OFFSET_FW_FEATURE_SETTING) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_FW_FEATURE_SETTING);
#endif

#if defined(RL6643_VA) || defined(RL6855_VA)
    _MEM08(sblk_addr + SBLK_OFFSET_SATA_SSC_ENABLE) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_SATA_SSC_ENABLE);
#endif

#if defined(RL6447_VA)
    memcpy((void*)(sblk_addr + SBLK_OFFSET_DDR_800_SETTING),  &DramPara_default[0],
           SBLK_SIZE_OF_DDR_SETTING);
    memcpy((void*)(sblk_addr + SBLK_OFFSET_DDR_1066_SETTING),  &DramPara_default[1],
           SBLK_SIZE_OF_DDR_SETTING);
    memcpy((void*)(sblk_addr + SBLK_OFFSET_DDR_1333_SETTING),  &DramPara_default[2],
           SBLK_SIZE_OF_DDR_SETTING);

    cache_area_dwbinval(ALIGN_32(sblk_addr + SBLK_OFFSET_DDR_800_SETTING),
                        SBLK_SIZE_OF_DDR_SETTING * 3);
    cache_dummy_update_read(sblk_addr + SBLK_OFFSET_DDR_800_SETTING + (SBLK_SIZE_OF_DDR_SETTING *
                            3) - WORD_BYTE_SIZE);
#endif

#if defined(RL6577_VA)
    for (i = 0; i < (SBLK_OFFSET_DSP_LENGTH - 0x20); i++)
    {
        _REG08(sblk_addr + SBLK_OFFSET_DSP_BASE + i) = _MEM08(CONFIG_BASE_VA_ADDR +
                SBLK_OFFSET_DSP_BASE + i);
    }

    _REG08(sblk_addr + SBLK_OFFSET_DSP_RD_DIVISOR_T) = _MEM08(CONFIG_BASE_VA_ADDR +
            SBLK_OFFSET_DSP_RD_DIVISOR_T);
    _REG08(sblk_addr + SBLK_OFFSET_DSP_RD_DIVISOR_M) = _MEM08(CONFIG_BASE_VA_ADDR +
            SBLK_OFFSET_DSP_RD_DIVISOR_M);
    _REG08(sblk_addr + SBLK_OFFSET_DSP_RISK_BOT_SLOPE) = _MEM08(CONFIG_BASE_VA_ADDR +
            SBLK_OFFSET_DSP_RISK_BOT_SLOPE);
    _REG08(sblk_addr + SBLK_OFFSET_DSP_RISK_BOT_SLOPE_2) = _MEM08(CONFIG_BASE_VA_ADDR +
            SBLK_OFFSET_DSP_RISK_BOT_SLOPE_2);
    _REG16(sblk_addr + SBLK_OFFSET_DSP_RISK_BOT_CELL) = _MEM16(CONFIG_BASE_VA_ADDR +
            SBLK_OFFSET_DSP_RISK_BOT_CELL);
    _REG16(sblk_addr + SBLK_OFFSET_DSP_RISK_BOT_CELL_2) = _MEM16(CONFIG_BASE_VA_ADDR +
            SBLK_OFFSET_DSP_RISK_BOT_CELL_2);
    _REG08(sblk_addr + SBLK_OFFSET_DSP_PARA_CTRL) = _MEM08(CONFIG_BASE_VA_ADDR +
            SBLK_OFFSET_DSP_PARA_CTRL);

    if(_MEM08(CONFIG_BASE_VA_ADDR + SBLK_OFFSET_DSP_PARA_CTRL))
    {
        _REG16(sblk_addr + SBLK_OFFSET_DSP_RRT_ECC_TH) = _MEM16(CONFIG_BASE_VA_ADDR +
                SBLK_OFFSET_DSP_RRT_ECC_TH);
        _REG16(sblk_addr + SBLK_OFFSET_DSP_RISK_ECC_TH) = _MEM16(CONFIG_BASE_VA_ADDR +
                SBLK_OFFSET_DSP_RISK_ECC_TH);
        _REG16(sblk_addr + SBLK_OFFSET_DSP_RISK_RD_ECC_TH) = _MEM16(CONFIG_BASE_VA_ADDR +
                SBLK_OFFSET_DSP_RISK_RD_ECC_TH);
        _REG16(sblk_addr + SBLK_OFFSET_DSP_RISK_RD_VT_TH) = _MEM16(CONFIG_BASE_VA_ADDR +
                SBLK_OFFSET_DSP_RISK_RD_VT_TH);
        _REG08(sblk_addr + SBLK_OFFSET_DSP_RD_SLOW_VT_TH) = _MEM08(CONFIG_BASE_VA_ADDR +
                SBLK_OFFSET_DSP_RD_SLOW_VT_TH);
    }
    else
    {
        _REG16(sblk_addr + SBLK_OFFSET_DSP_RRT_ECC_TH) = 70;
        _REG16(sblk_addr + SBLK_OFFSET_DSP_RISK_ECC_TH) = 180;
        _REG16(sblk_addr + SBLK_OFFSET_DSP_RISK_RD_ECC_TH) = 150;
        _REG16(sblk_addr + SBLK_OFFSET_DSP_RISK_RD_VT_TH) = 15;
        _REG08(sblk_addr + SBLK_OFFSET_DSP_RD_SLOW_VT_TH) = 12;
    }
#endif

    _MEM16(sblk_addr + SBLK_OFFSET_SECURITY_FUNC) = _MEM16(CONFIG_BASE_VA_ADDR +
            SBLK_OFFSET_SECURITY_FUNC);
    _MEM08(sblk_addr + SBLK_OFFSET_EXTEND_L2PAREA) = _MEM08(CONFIG_BASE_VA_ADDR +
            SBLK_OFFSET_EXTEND_L2PAREA);
    _MEM16(sblk_addr + SBLK_OFFSET_LED_LIGHT) = _MEM16(CONFIG_BASE_VA_ADDR + SBLK_OFFSET_LED_LIGHT);

    _MEM08(sblk_addr + SBLK_OFFSET_SNAP_READ_EN) = _MEM08(CONFIG_BASE_VA_ADDR +
            CONFIG_SNAP_READ_EN);

#if defined(RL6643_VA) || defined(RL6643_FPGA)
    _MEM16(sblk_addr + SBLK_OFFSET_LOW_POWER_CONTROL) = _MEM16(CONFIG_BASE_VA_ADDR +
            SBLK_OFFSET_LOW_POWER_CONTROL);
    _REG32(sblk_addr + SBLK_OFFSET_SET_FW_OPERATION) = _MEM32(CONFIG_BASE_VA_ADDR +
            SBLK_OFFSET_SET_FW_OPERATION);
#endif
    _REG32(sblk_addr + SBLK_OFFSET_SNAP_HOST_CLAIM_CAPACITY) = (U32)((0xAA55 << 16) |
            (guwHostClaimCapacity & 0xFFFF)); // 0xAA55 is just a tag for tool to recognize

    // Copy defect table of System block
    memcpy((void*)(sblk_addr + SBLK_OFFSET_SYSTEM_BLOCK_DBT), (const void*)(SYS_BLK_DBT_ADDR),
           SYSTEM_BLOCK_MAX_NUM * SYS_BLK_DBT_BYTE_SIZE_PER_BLOCK);
    cache_area_dwbinval(ALIGN_32(sblk_addr + SBLK_OFFSET_SYSTEM_BLOCK_DBT),
                        SYSTEM_BLOCK_MAX_NUM * SYS_BLK_DBT_BYTE_SIZE_PER_BLOCK);
    cache_dummy_update_read();
    cache_area_dwbinval(ALIGN_32(sblk_addr), SBLK_SIZE);
    cache_dummy_update_read(); // dummy read back

    // Record DBT Type
    _MEM32(sblk_addr + SBLK_OFFSET_LLF_TYPE_RECORD) = ((gulLLFTypeRecord << 4) | (gfDBTInitDone & 0xf));

    // Record host write sector for K3
    if (gfDBTInitDone == LLF_DBT_SYSTEM)
    {
        _MEM32(sblk_addr + SBLK_OFFSET_WRITE_MB_RECORD_0) = gulWriteMBRecord[0];
        _MEM32(sblk_addr + SBLK_OFFSET_WRITE_MB_RECORD_1) = gulWriteMBRecord[1];
    }
    else
    {
        _MEM32(sblk_addr + SBLK_OFFSET_WRITE_MB_RECORD_0) = 0;
        _MEM32(sblk_addr + SBLK_OFFSET_WRITE_MB_RECORD_1) = 0;
    }
    printk("Record LLF history %x HostWrite %x_%x\r\n",
           _MEM32(sblk_addr + SBLK_OFFSET_LLF_TYPE_RECORD),
           _MEM32(sblk_addr + SBLK_OFFSET_WRITE_MB_RECORD_1),
           _MEM32(sblk_addr + SBLK_OFFSET_WRITE_MB_RECORD_0));

    return ERR_OK;
}
#if defined(RTS5771_FPGA) || defined(RTS5771_VA)
void llfFCCmdSlcWrite_Nondata(U32 ulMode, U8 ubBankNo, U8 ubLunNo, U16 uwBlockNo, U16 uwPageNo)
{
    U32 row_addr;
    U8 hlc_size;
    U8 dcnt = 0;

#if defined (BLK_REMAP_PRO)
    uwBlockNo = BlkRemapProCalc(uwBlockNo, ubBankNo);
#endif

    MultiLunSetRowAddr(uwBlockNo, uwPageNo, &row_addr, ubBankNo);

    gsHlcslcWriteNondata.control_bit.AsU32 = 0;
    gsHlcslcWriteNondata.control_bit.bits.cmd_valid = 1;
    gsHlcslcWriteNondata.control_bit.bits.hlc_compress = 0;
    gsHlcslcWriteNondata.control_bit.bits.cmd_index =
        g_parser.index.SlcWrite_Nondata;//need to change table
    gsHlcslcWriteNondata.control_bit.bits.with_auto_bm = 0;
    gsHlcslcWriteNondata.control_bit.bits.with_dram_address = 0;
    gsHlcslcWriteNondata.control_bit.bits.with_cache_list_index = 0;
    gsHlcslcWriteNondata.control_bit.bits.data_list_mask_en = 0;
    gsHlcslcWriteNondata.control_bit.bits.data_en = 0;
    gsHlcslcWriteNondata.control_bit.bits.head_en = 0;
    gsHlcslcWriteNondata.control_bit.bits.head_chk_unc_en = 0;
    gsHlcslcWriteNondata.control_bit.bits.scramble_bypass = 1;
    gsHlcslcWriteNondata.control_bit.bits.ecc_bypass = 1;
    gsHlcslcWriteNondata.control_bit.bits.abort = 0;
    gsHlcslcWriteNondata.control_bit.bits.drop = 0;


    gsHlcslcWriteNondata.head2.bytes.FW_tag = gul_FW_TAG;
    gsHlcslcWriteNondata.head2.bytes.IF_sel = ulMode;
    gsHlcslcWriteNondata.head2.bytes.agitation_idx = 0;
    gsHlcslcWriteNondata.head2.bytes.xor_data_dir = 0;
    gsHlcslcWriteNondata.head2.bytes.xor_buff_idx = 0;

    gsHlcslcWriteNondata.head3.AsU32 = 0;
    gsHlcslcWriteNondata.head3.bits.cmp_sel = 0;
    gubCmpSel = gsHlcslcWriteNondata.head3.bits.cmp_sel;

    if((FLASH_VENDOR(gulFlashVendorNum) == IS_YMTC)
            && ((FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YG2T)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YG2T_CS2)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX2T)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX2Q)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX3T_WYS)))
    {
        gsHlcslcWriteNondata.parameter[dcnt++] = (U8)row_addr;
        gsHlcslcWriteNondata.parameter[dcnt++] = (U8)(row_addr >> 8);
        gsHlcslcWriteNondata.parameter[dcnt++] = (U8)(row_addr >> 16);
        gsHlcslcWriteNondata.parameter[dcnt++] = (U8)(row_addr >> 24);
    }
    else // IS_SANDISK
    {
#ifdef MULTI_LUN
        gsHlcslcWriteNondata.parameter[dcnt++] = (U8)row_addr;
        gsHlcslcWriteNondata.parameter[dcnt++] = (U8)(row_addr >> 8);
        gsHlcslcWriteNondata.parameter[dcnt++] = (U8)(row_addr >> 16);
#endif
    }
    gsHlcslcWriteNondata.parameter[dcnt++] = 0;
    gsHlcslcWriteNondata.parameter[dcnt++] = 0;
    gsHlcslcWriteNondata.parameter[dcnt++] = (U8)row_addr;
    gsHlcslcWriteNondata.parameter[dcnt++] = (U8)(row_addr >> 8);
    gsHlcslcWriteNondata.parameter[dcnt++] = (U8)(row_addr >> 16);
    if((FLASH_VENDOR(gulFlashVendorNum) == IS_YMTC)
            && ((FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YG2T)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YG2T_CS2)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX2T)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX2Q)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX3T_WYS)))
    {
        gsHlcslcWriteNondata.parameter[dcnt++] = (U8)(row_addr >> 24);
    }

    gsHlcslcWriteNondata.parameter[dcnt++] = (U8)row_addr;
    gsHlcslcWriteNondata.parameter[dcnt++] = (U8)(row_addr >> 8);
    gsHlcslcWriteNondata.parameter[dcnt++] = (U8)(row_addr >> 16);
    if((FLASH_VENDOR(gulFlashVendorNum) == IS_YMTC)
            && ((FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YG2T)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YG2T_CS2)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX2T)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX2Q)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX3T_WYS)))
    {
        gsHlcslcWriteNondata.parameter[dcnt++] = (U8)(row_addr >> 24);
    }

    hlc_size = ((dcnt & 0x3) == 0) ? (dcnt / 4) : (dcnt / 4 + 1);
    hlc_size += 3;
    gsHlcslcWriteNondata.control_bit.bits.hlcmd_size = hlc_size - 1;

    PushFcCmdFifo(ubBankNo, &gsHlcslcWriteNondata.control_bit.AsU32, hlc_size);
}
#else
void llfFCCmdSlcWrite_Nondata(U32 ulMode, U8 ubBankNo, U8 ubLunNo, U16 uwBlockNo, U16 uwPageNo)
{
    U32 row_addr;
    U8 hlc_size;
    U8 dcnt = 0;

#if defined (BLK_REMAP_PRO)
    uwBlockNo = BlkRemapProCalc(uwBlockNo, ubBankNo);
#endif
    MultiLunSetRowAddr(uwBlockNo, uwPageNo, &row_addr, ubBankNo);

    gsHlcslcWriteNondata.control_bit.bits.cmd_valid = 1;
    gsHlcslcWriteNondata.control_bit.bits.hlcmd_size = 3;

    gsHlcslcWriteNondata.control_bit.bits.cmd_index =
        g_parser.index.SlcWrite_Nondata;//need to change table
    gsHlcslcWriteNondata.control_bit.bits.with_dram_address = 0;
    gsHlcslcWriteNondata.control_bit.bits.with_cache_list_index = 0;
    gsHlcslcWriteNondata.control_bit.bits.data_list_mask_en = 0;
    gsHlcslcWriteNondata.control_bit.bits.data_en = 0;
    gsHlcslcWriteNondata.control_bit.bits.head_en = 0;
    gsHlcslcWriteNondata.control_bit.bits.head_chk_unc_en = 0;
    gsHlcslcWriteNondata.control_bit.bits.head_chk_lbn_en = 0;
    gsHlcslcWriteNondata.control_bit.bits.cmd_stop = 0;

    gsHlcslcWriteNondata.control_bit.bits.scramble_bypass = 1;
    gsHlcslcWriteNondata.control_bit.bits.ecc_bypass = 1;
    gsHlcslcWriteNondata.control_bit.bits.aes_bypass = 1;
    gsHlcslcWriteNondata.control_bit.bits.abort = 0;
    gsHlcslcWriteNondata.control_bit.bits.drop = 0;


    gsHlcslcWriteNondata.head2.bytes.FW_tag = gul_FW_TAG;
    gsHlcslcWriteNondata.head2.bytes.IF_sel = ulMode;
    gsHlcslcWriteNondata.head2.bytes.xor_data_dir = 0;
    gsHlcslcWriteNondata.head2.bytes.xor_buff_idx = 0;

    gsHlcslcWriteNondata.head3.AsU32 = 0;
#if defined(RL6577_FPGA) || defined(RL6577_VA) || defined(RL6643_FPGA) || defined(RL6643_VA)
    gsHlcslcWriteNondata.head3.bits.cmp_sel = 0;
    gubCmpSel = gsHlcslcWriteNondata.head3.bits.cmp_sel;
#endif
    if((FLASH_VENDOR(gulFlashVendorNum) == IS_YMTC)
            && ((FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YG2T)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YG2T_CS2)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX2T)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX2Q)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX3T_WYS)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX3T_WDS)))
    {
        gsHlcslcWriteNondata.parameter[dcnt++] = (U8)row_addr;
        gsHlcslcWriteNondata.parameter[dcnt++] = (U8)(row_addr >> 8);
        gsHlcslcWriteNondata.parameter[dcnt++] = (U8)(row_addr >> 16);
        gsHlcslcWriteNondata.parameter[dcnt++] = (U8)(row_addr >> 24);
    }
    else // IS_SANDISK
    {
#ifdef MULTI_LUN
        gsHlcslcWriteNondata.parameter[dcnt++] = (U8)row_addr;
        gsHlcslcWriteNondata.parameter[dcnt++] = (U8)(row_addr >> 8);
        gsHlcslcWriteNondata.parameter[dcnt++] = (U8)(row_addr >> 16);
#endif
    }
    gsHlcslcWriteNondata.parameter[dcnt++] = 0;
    gsHlcslcWriteNondata.parameter[dcnt++] = 0;
    gsHlcslcWriteNondata.parameter[dcnt++] = (U8)row_addr;
    gsHlcslcWriteNondata.parameter[dcnt++] = (U8)(row_addr >> 8);
    gsHlcslcWriteNondata.parameter[dcnt++] = (U8)(row_addr >> 16);
    if((FLASH_VENDOR(gulFlashVendorNum) == IS_YMTC)
            && ((FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YG2T)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YG2T_CS2)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX2T)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX2Q)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX3T_WYS)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX3T_WDS)))
    {
        gsHlcslcWriteNondata.parameter[dcnt++] = (U8)(row_addr >> 24);
    }

    gsHlcslcWriteNondata.parameter[dcnt++] = (U8)row_addr;
    gsHlcslcWriteNondata.parameter[dcnt++] = (U8)(row_addr >> 8);
    gsHlcslcWriteNondata.parameter[dcnt++] = (U8)(row_addr >> 16);
    if((FLASH_VENDOR(gulFlashVendorNum) == IS_YMTC)
            && ((FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YG2T)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YG2T_CS2)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX2T)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX2Q)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX3T_WYS)
                || (FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_YX3T_WDS)))
    {
        gsHlcslcWriteNondata.parameter[dcnt++] = (U8)(row_addr >> 24);
    }

    hlc_size = ((dcnt & 0x3) == 0) ? (dcnt / 4) : (dcnt / 4 + 1);
    hlc_size += 3;

    PushFcCmdFifo(ubBankNo, &gsHlcslcWriteNondata.control_bit.AsU32, hlc_size);
}
#endif
/*
void SetLDPCmatrix(U32 matrix_addr)
{
    U32  i;
    U32 addr;
    U32 ecc_cfg, oob_2k;

    //LP mode
    FC_LDEC_REG(FR_LDEC_HD_STRATEGY) |= 1;
    FC_LDEC_REG(FR_LDEC_HD_STRATEGY) &= 0xfffffefd;//set bit1&bit8 to 0
    //matrix writable
    FC_LENC_REG(FR_LENC_CLR) = 1;//reset LENC
    while( (FC_LENC_REG(FR_LENC_CLR) & 0x1) == 1 ); //wait for reset done
    FC_DSP_REG(FR_DSP_LDPC_CLR) = 1;//reset LDEC & DSP
    while( (FC_DSP_REG(FR_DSP_LDPC_CLR) & 0x1) == 1 );//wait for reset done
    FC_LENC_REG(FR_LENC_G_MATRIX_CFG_EN) = 1;
    FC_LDEC_REG(FR_LDEC_PCM_CFG_MODE) = 1;

    addr = matrix_addr;
    for(i = 0; i < LDPC_G_MATRIX_SIZE; i += 4)
    {
        FC_LENC_REG(FR_G_MATRIX_OFFSET + i) = _REG32(addr + i);
    }
    addr += LDPC_G_MATRIX_SIZE;
    for(i = 0; i < LDPC_ET_MATRIX_SIZE; i += 4)
    {
        FC_LDEC_REG(FR_ET_MATRIX_OFFSET + i) = _REG32(addr + i);
    }
    addr += LDPC_ET_MATRIX_SIZE;
    for(i = 0; i < LDPC_LLR_MATRIX_SIZE; i += 4)
    {
        FC_LDEC_REG(FR_LLR_MATRIX_OFFSET + i) = _REG32(addr + i);
    }

    //wait for load matrix done and disable writable
    FC_LENC_REG(FR_LENC_G_MATRIX_CFG_EN) = 0;
    FC_LENC_REG(FR_LENC_PARA) &= 0xfffffff8;//clear bit0~2
    FC_LENC_REG(FR_LENC_PARA) |= (gubLdpcCodeRate & 0x7);//only update bit0~2

    FC_LENC_REG(FR_LENC_INIT) = 1;
    while( FC_LENC_REG(FR_LENC_INIT) ==  1 );//wait for load matrix done
    FC_LDEC_REG(FR_LDEC_PCM_CFG_MODE) = 0;

    FC_TOP_REG(FR_LDEC_CODERATE) = gubLdpcCodeRate;
    ecc_cfg = guwLdpcParityLen;
    FR_G_CFG_REG32_W(FR_ECC_CFG, (ecc_cfg | (ecc_cfg << 16)) );

    oob_2k = ecc_cfg + (1 << 11) + (gubHeaderlen >> 1) + 4;
    for( i = 0; i < 4; i++ )
    {
        FC_TOP_REG(FR_FC_CR_LEN0 + i * 4) = (oob_2k << 16) | (oob_2k);
        FR_G_CFG_REG32_W(FR_FC_CR_LEN0 + i * 4, (oob_2k << 16) | (oob_2k) );
    }
    llfDbgPrintk(INIT_MSG, "[INIT] ECC length: %d, code length: %d\r\n", ecc_cfg, oob_2k);
}

void Init_ECC_cfg()
{
    U32 ecc_cfg, oob_2k;
    U8 i;

    FC_TOP_REG(FR_LDEC_CODERATE) = gubLdpcCodeRate;
    ecc_cfg = guwLdpcParityLen;
    FR_G_CFG_REG32_W(FR_ECC_CFG, (ecc_cfg | (ecc_cfg << 16)) );

    oob_2k = ecc_cfg + (1 << 11) + (gubHeaderlen >> 1) + 4;
    for( i = 0; i < 4; i++ )
    {
        FC_TOP_REG(FR_FC_CR_LEN0 + i * 4) = (oob_2k << 16) | (oob_2k);
        FR_G_CFG_REG32_W(FR_FC_CR_LEN0 + i * 4, (oob_2k << 16) | (oob_2k) );
    }
    llfDbgPrintk(INIT_MSG, "[INIT] ECC length: %d, code length: %d\r\n", ecc_cfg, oob_2k);
}
*/
void SetParserSequencerTable(U32 parser_addr, U32 sequencer_addr)
{
    U32  j;

    for(j = 0; j < PARSER_TABLE_SIZE; j += 4)
    {
        FR_G_CTRL_REG32_W(FR_PARSER_OFFSET + j, _REG32(parser_addr + j) );
    }
    for(j = 0; j < SEQUENCER_TABLE_SIZE; j += 4)
    {
        FR_G_CTRL_REG32_W(FR_SEQUENCER_OFFSET + j, _REG32(sequencer_addr + j) );
    }
}

void SetParserSequencerIndex(U32 parser_index_addr, U32 sequencer_index_addr,
                             U32 auto_insert_index_addr)
{
    U32 i, j, tmp;
    for(i = 0; i < PARSER_MAX_LEN; i++)
    {
        INDEX_ParserTable1[i] = _REG16(parser_index_addr + i * 2);
    }
    for(i = 0; i < 3; i++)
    {
        for(j = 0; j < MAX_SEQUENCER_PHASE_NUM; j++)
        {
            INDEX_SequencerTable1[i][j] = _REG08(sequencer_index_addr + i * 0x10  + j);
        }
    }

    for (i = 0; i < 4; i++)
    {
        tmp = (U32)(&INDEX_SequencerTable1[0][i * 4]);
        FR_G_CFG_REG32_W(FR_SEQ_INDEX_0_0 + (i * 4), *(U32 *)(tmp) );

        tmp = (U32)(&INDEX_SequencerTable1[1][i * 4]);
        FR_G_CFG_REG32_W(FR_SEQ_INDEX_1_0 + (i * 4), *(U32 *)(tmp) );


        tmp = (U32)(&INDEX_SequencerTable1[2][i * 4]);
        FR_G_CFG_REG32_W(FR_SEQ_INDEX_2_0 + (i * 4), *(U32 *)(tmp) );
    }

    //assign SDR start end phase DW offset, HW auto insert
    FR_G_CFG_REG32_W(FR_SDR_PHASE_INDEX_0, (*(U32 *)(auto_insert_index_addr)));//0x100;

    //assign DDR start(etc.) phase DW offset, HW auto insert
    FR_G_CFG_REG32_W(FR_DDR_PHASE_INDEX_0, (*(U32 *)(auto_insert_index_addr + 4)));
    FR_G_CFG_REG32_W(FR_DDR_PHASE_INDEX_1, (*(U32 *)(auto_insert_index_addr + 8)));

    //assign DDR2 start(etc.) phase DW offset, HW auto insert
    FR_G_CFG_REG32_W(FR_DDR2_PHASE_INDEX_0, (*(U32 *)(auto_insert_index_addr + 0xC)));//0x312F2D2C;
    FR_G_CFG_REG32_W(FR_DDR2_PHASE_INDEX_1, (*(U32 *)(auto_insert_index_addr + 0x10)));//0x37353033;
    FR_G_CFG_REG32_W(FR_DDR2_PHASE_INDEX_2, (*(U32 *)(auto_insert_index_addr + 0x14)));//0x0000393A;
}

void SetSeedTable(U32 seed_table_addr)
{
    U32 i;
    FC_TOP_REG(FR_SCR_CTRL) = 251;

    for(i = 0; i < SEED_SIZE; i += 4)
    {
        FC_TOP_REG(FR_SEED_OFFSET + i) = _REG32(seed_table_addr + i);
    }
}

#if defined(RL6577_FPGA) || defined(RL6577_VA) || defined(RL6643_FPGA)|| defined(RL6643_VA) || defined(RTS5771_FPGA) || defined(RTS5771_VA)

void llfBECheckStatus()
{
    U32 be_tag, ret;
    U16 cmp_status;
    U8 bank_no;
    U32 block_no;
    U32 CMP_MASK;
    U32 cmp, err_info0, err_info1;
    U32 ulTimeoutTick = 0;

    PLLF_UNI_INFO pLLF_uni_info;
    pLLF_uni_info = (PLLF_UNI_INFO)LLF_UNI_INFO_ADDR;

#ifdef BLK_REMAP_PRO
    U8 i;
    U16 MpNum;
#endif

#ifdef ERASEALL_TWICE
#if !defined(FTL_N38A) && !defined(FTL_N38B) && !defined(FTL_Q5171A)
    U8 ubLunNo = 0, ubEraseTwiceCnt = 0;
    U32 ulMode = FR_CONFIG_CH(FR_FC_MODE, gubStartCH);
#endif
#endif

    if(gubCmpSel)
        CMP_MASK = 0x3ff800;//bank cmp queue:bit11~21
    else
        CMP_MASK = 0x7ff;//normal cmp queue:bit0~10

    while(gufLastEraseFlag)
    {
        if((FC_TOP_REG(FR_CMPQ_BC) & CMP_MASK) == 0)
        {
            FcBusyWait1ms(1);
            if(++ulTimeoutTick > 10000)
            {
                gubErasetimeout = 1;
                return;
            }
            continue;
        }
        cmp = FcPollCompletion(&err_info0, &err_info1);
        be_tag = cmp >> 16;
        cmp_status = (U16)(cmp & CMP_ERR);
        bank_no = (U8)(be_tag & BANK_NO_MASK);
        block_no = (U16)((be_tag & GROUP_NO_MASK) >> 4);
        //llfprintk("bank:%d,blk:%d\r\n", bank_no, block_no);
        if(gubEraseNum == 1)
        {
        }
        else if(gubEraseNum == 2)
        {
            block_no += 4096;
        }
        else if(gubEraseNum == 3)
        {
            block_no += 8192;
        }
        else if(gubEraseNum == 4)
        {
            block_no += 12288;
        }
        else if(gubEraseNum == 5)
        {
            block_no += 16384;
        }
        else if(gubEraseNum == 6)
        {
            block_no += 20480;
        }
        else
        {
            printk("[ERR] gubEraseNum %d \r\n", gubEraseNum);
        }

#ifdef ERASEALL_TWICE
        if(cmp_status != 0)
        {
            if((gubFWFeatureSetting & 0x20) != 0)
            {
#if defined(FTL_N38A) || defined(FTL_N38B) || defined(FTL_Q5171A)
                llfprintk("N38A not support ERASEALL_TWICE\r\n");
#else
                // llfprintk("gubEraseNum = %d, block_no = %d\r\n", gubEraseNum, block_no);
                ASSERT_LLF(gufLastEraseFlag == 1);
                ubLunNo = bank_no / NandPara.ubBankNumPerLun;
                ubEraseTwiceCnt = 0;
                gul_FW_TAG = cmp >> 16;
                FCSingleErase(ulMode, bank_no, ubLunNo, block_no, 0, 0);
                ubEraseTwiceCnt++;
                while(ubEraseTwiceCnt)
                {
                    if((FC_TOP_REG(FR_CMPQ_BC) & CMP_MASK) == 0)
                    {
                        FcBusyWait1ms(1);
                        if(++ulTimeoutTick > 10000)
                        {
                            gubErasetimeout = 1;
                            return;
                        }
                        continue;
                    }
                    ubEraseTwiceCnt--;
                    cmp = FcPollCompletion(&err_info0, &err_info1);
                    cmp_status = (U16)(cmp & CMP_ERR);
                }
#endif
            }
        }
#endif

        if(cmp_status != 0)
        {
            // llfprintk("gubEraseNum = %d, block_no = %d\r\n", gubEraseNum, block_no);
            if(block_no < SYSTEM_BLOCK_MAX_NUM)
            {
                llfMarkSystemBlkDefect(SYS_BLK_DBT_ADDR, bank_no, block_no);
                //llfprintk("mark bad bank%d block%d\r\n", bank_no, block_no);
            }
#if defined(LLF_CHECK_SYSTEMBLK_ERASE_FAIL)
#ifndef SBLK_EXPAND
            if(block_no < SYS_BLK && bank_no < 8)
            {
                gulEraseFailSysblk |= 1 << (block_no + 4 * bank_no);
            }
#else
            U8 ubBankNum = (NandPara.ubBankNum > SYS_BANK_NUM) ? SYS_BANK_NUM : NandPara.ubBankNum;
            if(block_no >= gubSblkStart && block_no < (gubSblkStart + SYS_BLK)
                    && bank_no >= gubSblkBankStart && bank_no < (gubSblkBankStart + ubBankNum))
            {
                gulEraseFailSysblk |= (1 << ((block_no - gubSblkStart) + 4 * (bank_no - gubSblkBankStart)));
            }
#endif
#endif
#ifdef BLK_REMAP_PRO
            if (gubLLFMode == LLF_INHERIT_OR_FORMAT || gubLLFMode == LLF_FORCE_INHERIT
                    || gubLLFMode == LLF_DEFECT_BAD_BLOCK)
            {
                ;
            }
            else
            {
                if (block_no >= SYSTEM_BLOCK_MAX_NUM)
                {
                    MpNum = block_no / NandPara.ubPlaneNumPerLun;
                    //llfprintk("Try to push some good single block into Remap table! block %d\r\n", block_no);
                    //llfprintk("guwMpDefectBlock[%d] %d\r\n",bank_no, guwMpDefectBlock[bank_no]);
                    if(guwMpDefectBlock[bank_no] == MpNum)
                    {
                        //second
                        for (i = 0;  i < NandPara.ubPlaneNumPerLun ; i ++)
                        {
                            if ((MpNum * NandPara.ubPlaneNumPerLun) + i >= block_no)
                            {
                                if((MpNum * NandPara.ubPlaneNumPerLun) + i != block_no)
                                    guwRemapTempBlock[bank_no][i] = (MpNum * NandPara.ubPlaneNumPerLun) + i;
                                else
                                    guwRemapTempBlock[bank_no][i] = 0xFFFF;
                            }
                        }
                    }
                    else
                    {
                        //push tempblock into table
                        if (guwMpDefectBlock[bank_no] != 0xFFFF)
                            LlfCreatReMappingTable(bank_no);

                        guwMpDefectBlock[bank_no] = MpNum;

                        //First
                        for (i = 0;  i < NandPara.ubPlaneNumPerLun ; i ++)
                        {
                            if((MpNum * NandPara.ubPlaneNumPerLun) + i != block_no)
                                guwRemapTempBlock[bank_no][i] = (MpNum * NandPara.ubPlaneNumPerLun) + i;
                            else
                                guwRemapTempBlock[bank_no][i] = 0xFFFF;
                        }

                    }

                    if (block_no == NandPara.uwBlockNumPerLun - 1)
                    {
                        LlfCreatReMappingTable(bank_no);
                    }

                }
            }
#endif

#ifdef SPECIAL_SYSTEM_BLOCK
            if ((!gubRdtImg) && (LLF_FORCE_FORMAT == gubLLFMode)
                    && (gfDBTInitDone <= LLF_DBT_INIT))
            {
                llfSetSpDBT(bank_no, block_no, SP_DBT_ADDR);
            }
#endif

            ret = llfSetDBT(bank_no, block_no, DBT_ADDR);

            if(ret != ERR_OK)
            {
                return;
            }
            pLLF_uni_info->uwData4++;  // add erase bad counts
        }
#ifdef BLK_REMAP_PRO
        if (block_no == NandPara.uwBlockNumPerLun - 1)
        {
            LlfCreatReMappingTable(bank_no);
        }
#endif
        gubEraseCmpNum[bank_no]--;
        gufLastEraseFlag--;
    }
}
#elif defined(RL6447_VA)||defined(RL6531_VB)
void llfBECheckStatus()
{
    U32 be_tag, ret;
    U32 cmp, err_info0, err_info1;
    U16 cmp_status;
    U8 bank_no;
    U32 block_no;
    U32 ulTimeoutTick = 0;
    //U8 plane;
    //U8 numBadInSuperBlock;
    PLLF_UNI_INFO pLLF_uni_info;
    pLLF_uni_info = (PLLF_UNI_INFO)LLF_UNI_INFO_ADDR;

    while(gufLastEraseFlag)
    {
        if(FC_TOP_REG(FR_CMPQ_BC) == 0)
        {
            FcBusyWait1ms(1);
            if(++ulTimeoutTick > 10000)
            {
                gubErasetimeout = 1;
                return;
            }
            continue;
        }
        cmp = FcPollCompletion(&err_info0, &err_info1);
        be_tag = cmp >> 16;
        cmp_status = (U16)(cmp & 0x7fff);
        bank_no = (U8)(be_tag & BANK_NO_MASK);
        block_no = gubEraseNum;
        //plane = block_no & (NandPara.ubPlaneNumPerLun - 1);

        if(cmp_status != 0)
        {
            // llfprintk("gubEraseNum = %d, block_no = %d\r\n", gubEraseNum, block_no);
            if(block_no < SYSTEM_BLOCK_MAX_NUM)
            {

                llfMarkSystemBlkDefect(SYS_BLK_DBT_ADDR, bank_no, block_no);
                //llfprintk("mark bad bank%d block%d\r\n", bank_no, block_no);
            }
#if defined(LLF_CHECK_SYSTEMBLK_ERASE_FAIL)
#ifndef SBLK_EXPAND
            if(block_no < SYS_BLK && bank_no < 8)
            {
                gulEraseFailSysblk |= 1 << (block_no + 4 * bank_no);
            }
#else
            U8 ubBankNum = (NandPara.ubBankNum > SYS_BANK_NUM) ? SYS_BANK_NUM : NandPara.ubBankNum;
            if(block_no >= gubSblkStart && block_no < gubSblkStart + SYS_BLK
                    && bank_no >= gubSblkBankStart && bank_no < (gubSblkBankStart + ubBankNum))
            {
                gulEraseFailSysblk |= (1 << ((block_no - gubSblkStart) + 4 * (bank_no - gubSblkBankStart)));
            }
#endif
#endif
            ret = llfSetDBT(bank_no, block_no, DBT_ADDR);
            if(ret != ERR_OK)
            {
                return;
            }
            pLLF_uni_info->uwData4++;  // add erase bad counts
        }
        gufLastEraseFlag--;
        gubEraseCmpNum[bank_no]--;
    }
    /*
        if((FLASH_VENDOR(gulFlashVendorNum) == IS_TOSHIBA)
                && ((FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_8T22_SDR)
                    || ((FLASH_SERIAL_NUM(gulFlashVendorNum) == IS_9T23_SDR_TOGGLE))))
        {
            gsHlcWriteCacheDRAM.parameter[dcnt++] = (U8)(row_addr);
            gsHlcWriteCacheDRAM.parameter[dcnt++] = (U8)((row_addr) >> 8);
            gsHlcWriteCacheDRAM.parameter[dcnt++] = (U8)((row_addr) >> 16); // Column address
        }

        hlc_size = ((dcnt & 0x3) == 0) ? (dcnt / 4) : (dcnt / 4 + 1);
#if !SCRAMBLE_BYPASS
        hlc_size += 10;
#else
        hlc_size += 7;
#endif

        gsHlcWriteCacheDRAM.control_bit.bits.hlcmd_size = hlc_size - 1;

        llfPushFcCmdFifo(ubBankNo, &gsHlcWriteCacheDRAM.control_bit.AsU32, hlc_size);
    */
}

#endif

#if defined(RL6643_VA) || defined(RL6577_VA) || defined(RL6447_VA) || defined(RTS5771_FPGA) || defined(RTS5771_VA)
/*
 * Send a char to terminal.
 */
void llf_serial_put_char( char c)
{
#ifndef UART_HWFIFO_MODE
    while ((READ_REG_32(UART1_BASE + REG_lsr) & LSR_THE) == 0) ;
    WRITE_REG_32(UART1_BASE + REG_thr, (U32) c);
#else
    while(!(PushElement((CircleQueue *)UART_SWFIFO, c)))
    {
        Core0_manual_popelement();
    }
#endif
}

int llfSprintF(char *buf, const char *fmt, ...);

int llfvsprintf(char *buf, const char *fmt, const U32 *dp)
{
    char *p, *s;
    U8 str[16];
    int ticklen = 0;

    s = buf;

    str[0] = '0';
    str[1] = '1';
    str[2] = '2';
    str[3] = '3';
    str[4] = '4';
    str[5] = '5';
    str[6] = '6';
    str[7] = '7';
    str[8] = '8';
    str[9] = '9';
    str[10] = 'A';
    str[11] = 'B';
    str[12] = 'C';
    str[13] = 'D';
    str[14] = 'E';
    str[15] = 'F';

    for (; *fmt != '\0'; ++fmt)
    {
        if (*fmt != '%')
        {
            if(buf)
            {
                if((s - buf) < LLF_UART_MEM_MAX_LEN)
                    *s++ = *fmt;
            }
            else
            {
                llf_serial_put_char(*fmt);
            }
            continue;
        }

        if (*++fmt == 's')
        {
            if(buf)
            {
                for (p = (char *)*dp++; *p != '\0'; p++)
                {
                    if((s - buf) < LLF_UART_MEM_MAX_LEN)
                    {
                        *s++ = *p;
                    }
                }
            }
            else
            {
                for (p = (char *)*dp++; *p != '\0'; p++)
                {
                    llf_serial_put_char(*p);
                }
            }
        }
        else    /* Length of item is bounded */
        {
            char tmp[20], *q = tmp;
            int alt = 0;
            int shift = 28;

            if (*fmt == '#')
            {
                alt = 1;
                fmt++;
            }

            if (*fmt == 'h')
            {
                shift = 12;
                fmt++;
            }

            if (*fmt == 'h')
            {
                shift = 4;
                fmt++;
            }

            /*
             * Before each format q points to tmp buffer
             * After each format q points past end of item
             */

            if ((*fmt | 0x20) == 'x')
            {
                /* With x86 gcc, sizeof(long) == sizeof(int) */
                const long *lp = (const long *)dp;
                long h = *lp++;
                int ncase = (*fmt & 0x20);
                dp = (const U32 *)lp;

                if (alt)
                {
                    *q++ = '0';
                    *q++ = 'X' | ncase;
                }

                for (; shift >= 0; shift -= 4)
                    * q++ = str[(h >> shift) & 0xF] | ncase;
            }
            else if (*fmt == 'd')
            {
                int i = *dp++;
                char *r;
                if (i < 0)
                {
                    *q++ = '-';
                    i = -i;
                }

                p = q;  /* save beginning of digits */

                do
                {
                    *q++ = '0' + (i % 10);
                    i /= 10;
                }
                while (i);

                /* reverse digits, stop in middle */
                r = q;  /* don't alter q */

                while (--r > p)
                {
                    i = *r;
                    *r = *p;
                    *p++ = i;
                }
            }
            else if (*fmt == '@')
            {
                unsigned char *r;

                union
                {
                    long l;
                    unsigned char c[4];
                } u;

                const long *lp = (const long *)dp;
                u.l = *lp++;
                dp = (const U32 *)lp;

                for (r = &u.c[0]; r < &u.c[4]; ++r)
                    q += llfSprintF(q, "%d.", *r);
                --q;

            }
            else if (*fmt == '!')
            {
                char *r;
                p = (char *)*dp++;

                for (r = p + LLF_ETH_ALEN; p < r; ++p)
                    q += llfSprintF(q, "%hhX:", *p);
                --q;
            }
            else if (*fmt == 'c')
            {
                *q++ = *dp++;
            }
            else
                *q++ = *fmt;

            /* now output the saved string */
            for (p = tmp; p < q; ++p)
            {
                if(buf)
                {
                    if((s - buf) < LLF_UART_MEM_MAX_LEN)
                        *s++ = *p;
                }
                else
                {
                    llf_serial_put_char(*p);
                }
                ticklen++;
            }
        }
    }

    if (buf)
    {
        if((s - buf) == LLF_UART_MEM_MAX_LEN)
        {
            *(s - 3) = '\0';
            *(s - 2) = '\r';
            *(s - 1) = '\n';
        }
    }
    else
    {
        if (*--fmt != '\n')
        {
            //gulTicks length > 7
            if(ticklen < 7)
            {
                llf_serial_put_char('\t');
            }
            llf_serial_put_char('\t');
        }
    }

    return (s - buf);
}

int llfSprintF(char *buf, const char *fmt, ...)
{
    return llfvsprintf(buf, fmt, ((const U32 *)&fmt) + 1);
}

void llfprintk(const char *fmt, ...)
{
    U32 flag;
    va_list ap;
    va_start(ap, fmt);
#ifdef UART_HWFIFO_MODE
    spin_trylock_irqsave(&g_print_lock, &flag);
#else
    spin_lock_irqsave(&g_print_lock, &flag);
#endif
    llfprint_to_ram(fmt, ap);
    va_end(ap);
    spin_unlock_irqrestore(&g_print_lock, &flag);
}

void llfDbgPrintk(unsigned int level, const char *fmt, ...)
{
    U32 flag;
    va_list ap;
    va_start(ap, fmt);
#ifdef UART_HWFIFO_MODE
    spin_trylock_irqsave(&g_print_lock, &flag);
#else
    spin_lock_irqsave(&g_print_lock, &flag);
#endif

    if (level & DebugPara.ulDebugLevel)
    {
        llfprint_to_ram(fmt, ap);
    }
    va_end(ap);
    spin_unlock_irqrestore(&g_print_lock, &flag);
}

void llfprint_to_ram(const char *fmt, const U32 *dp)
{
#if defined(RL6643_VA)
    gulTicks = (READ_REG_32(FAST_REG_CNT0) ? READ_REG_32(FAST_REG_CNT0) : READ_REG_32(FAST_REG_CNT0));
#endif

#ifdef BACKEND_FTL
    U32 ticks = gulTicks + gulUartTimeStamp;
#else
#if defined(RL6577_FPGA) || defined(RL6577_VA) || defined(RTS5771_FPGA) || defined(RTS5771_VA)
    U32 ticks = GET_TICK;
#else
    U32 ticks = gulTicks;
#endif
#endif
    const char *typed = "%x";
    const U32 *tickp = &ticks;

    SET_REG_32(SYS_SCRATCH_REG1, PAD_UART_TXD_OE);//enable uart txd pad oe
#ifdef UART_HWFIFO_MODE
    CLR_REG_32(0xff50131c, 0x1); //disable uart mask
#endif

    (void)llfvsprintf(0, typed, tickp);
    (void)llfvsprintf(0, fmt, dp);

#ifndef UART_HWFIFO_MODE
    while ((READ_REG_32(UART1_BASE + REG_lsr) & LSR_TEMT) == 0) ;//wait uart tx done
    CLR_REG_32(SYS_SCRATCH_REG1, PAD_UART_TXD_OE);//disable uart txd pad oe
#endif

    char *buf;
    int len;
    int ticklen = 9;
    len = LLF_UART_MEM_MAX_LEN + ticklen;
    if((gubLLFMemMsgFlag == 0)
#if defined(RL6577_VA) || defined(RL6447_VA) || defined(RTS5771_FPGA) || defined(RTS5771_VA)
            && ((gubCalibrateConfig & 0x80) == 0x80)
#endif
      )
    {
        if(len > gulLLFMemMsgFreeRoom)
        {
            buf = (char*)(LLFMEMMSGBUF_BASE_ADDR + LLFMEMMSGBUF_TOTALSIZE - gulLLFMemMsgFreeRoom);
            if(gulLLFMemMsgFreeRoom > 2)
            {
                memset(buf, '\0', gulLLFMemMsgFreeRoom - 2);
            }
            _MEM08(LLFMEMMSGBUF_BASE_ADDR + LLFMEMMSGBUF_TOTALSIZE - 2) = '\r';
            _MEM08(LLFMEMMSGBUF_BASE_ADDR + LLFMEMMSGBUF_TOTALSIZE - 1) = '\n';
            gubLLFMemMsgFlag = 1;
            return;
        }
        else
        {
            buf = (char*)(LLFMEMMSGBUF_BASE_ADDR + LLFMEMMSGBUF_TOTALSIZE - gulLLFMemMsgFreeRoom);
            memset(buf, '\0', len);
            llfvsprintf(buf, typed, tickp);
            _MEM08(buf + ticklen - 1) = ' ';
            gulLLFMemMsgFreeRoom -= ticklen;
            len = llfvsprintf((char *)((U32)buf + ticklen), fmt, dp) - 1;
            len = (len > LLF_UART_MEM_MAX_LEN) ? (LLF_UART_MEM_MAX_LEN) : (len);
            gulLLFMemMsgFreeRoom -= len;
        }
    }
}
#endif

void llfResetAndRecoverFC10M(U8 ubFcMode, U8 ubClockMode)
{
    U32 ulRegValue;
    PVENDOR_CMD_RESPONSE pLLFResponseInfo;
#if defined(RL6577_VA) || defined(RL6447_VA) || defined(RTS5771_VA)
    U8 ubTempFix90;

    ubTempFix90 = gubDqsFix90;
#endif
    pLLFResponseInfo = (PVENDOR_CMD_RESPONSE)LLF_RES_BUF_VA_ADDR;
    pLLFResponseInfo->err_msg_num = 0;

    llfDbgPrintk(ALWAYS_MSG, "FCSoftReset %d\r\n", ubFcMode, ubClockMode);
    FC_TOP_REG(FC_BIU_OCP_STOP) = 1; //stop ocp bus
    while(1)
    {
        if(FC_TOP_REG(FC_BIU_OCP_DONE) == 1)
            break;
    }
    ulRegValue = _REG32(U_SOFT_RST_CTRL);
    _REG32(U_SOFT_RST_CTRL) = ulRegValue & (~U_SOFT_RST_FC_BIT);
    _REG32(U_SOFT_RST_CTRL) = ulRegValue | (U_SOFT_RST_FC_BIT);

    llfDbgPrintk(ALWAYS_MSG, "init onfi4 start, default mode: %d\r\n", gubNandDefaultMode);
#if defined(RL6643_VA) || defined(RL6531_VB) || defined(RL6643_FPGA)
    if(gubNandDefaultMode == 0)

    {
        ChangeFCClk(ONFI_SDR, FC_PLL_CLK_10M);
        FR_G_CFG_REG32_W(FR_FC_MODE, ONFI_SDR);
    }
    else
    {
        ChangeFCClk(ONFI_DDR2_TOGGLE, FC_PLL_CLK_10M);
        if((FLASH_VENDOR(gulFlashVendorNum) == IS_SANDISK) || (FLASH_VENDOR(gulFlashVendorNum) == IS_HYNIX))
        {
            fc_diff_setting(ONFI_DDR2_TOGGLE, 0, gubFCDiffEnable);
        }
        else
        {
            fc_diff_setting(ONFI_DDR2_TOGGLE, 0, 0);
        }
        FR_G_CFG_REG32_W(FR_FC_MODE, ONFI_DDR2_TOGGLE);
    }
#if defined(RL6643_VA) || defined(RL6643_FPGA)
    onfi4_ocd_odt_setting(FC_OCD_DRIVE, FC_OCD_DRIVE, FC_ODT_CFG, FC_ODT_CFG);//10M,ocd:50ohm,odt:150ohm
#else
    Fc_ocd_odt_setting(FC_OCD_DRIVE, FC_ODT_CFG);
#endif
#elif defined(RL6577_VA) || defined(RL6447_VA) || defined(RTS5771_VA)
    gubDqsFix90 = 1;
    if(gubNandDefaultMode == 0)
    {
        ChangeFCClk(ONFI_SDR, FC_PLL_CLK_10M);
        onfi4_change_setting(ONFI_SDR, FC_PLL_CLK_10M, FC_OCD_DRIVE, FC_ODT_CFG, FC_ODT_CFG, 0);
        FR_G_CFG_REG32_W(FR_FC_MODE, ONFI_SDR);
    }
    else
    {
        ChangeFCClk(ONFI_DDR2_TOGGLE, FC_PLL_CLK_10M);
        onfi4_change_setting(ONFI_DDR2_TOGGLE, FC_PLL_CLK_10M, FC_OCD_DRIVE, FC_ODT_CFG, FC_ODT_CFG, 0);
        if((FLASH_VENDOR(gulFlashVendorNum) == IS_SANDISK) || (FLASH_VENDOR(gulFlashVendorNum) == IS_HYNIX))
        {
            fc_diff_setting(ONFI_DDR2_TOGGLE, 0, 1);
        }
        else
        {
            fc_diff_setting(ONFI_DDR2_TOGGLE, 0, 0);
        }
        FR_G_CFG_REG32_W(FR_FC_MODE, ONFI_DDR2_TOGGLE);
    }
#endif
    SetParserSequencerTable(PARSER_TABLE_VA_ADDR, SEQUENCER_TABLE_VA_ADDR);
    SetParserSequencerIndex(PARSER_INDEX_ADDR, SEQUENCER_INDEX_ADDR, AUTO_INSERT_INDEX_ADDR);
    FcCopySeedFromROM();
    llfFcInitReg();
    HlcParserIndexInit();
    Change_ldpc(gubECC_CFG);

#if defined(RL6643_VA) || defined(RTS5771_VA) || defined(RTS5771_FPGA) || defined(RL6643_FPGA)
#else
    if(!AES_BYPASS)
    {
        InitAESEngine();
    }
#endif

#if defined(RL6577_VA) || defined(RL6447_VA) || defined(RTS5771_VA)
    gubDqsFix90 = ubTempFix90;
#endif

}

#if defined(RL6643_FPGA)|| defined(RL6643_VA)
void llfUpdateVrefToEFUSE(U32 VrefCfg)
{
    U32 value, ret = ERR_OK;
    U8 i, j, count, updateTag = 0, currentTag = 0, hasUpdate = 0;
    PVENDOR_CMD_RESPONSE pResponseInfo;
    U32 VrefDefaultSet = 0x124;

    if(IS_6855_VERSION_TAG)
        VrefDefaultSet = 0x11e;

    pResponseInfo = (PVENDOR_CMD_RESPONSE)LLF_RES_BUF_VA_ADDR;

    llfInitErrorMessage();

    for(i = 0; i < 4; i++)
    {
        count = ((~i) & 0x3);
        value = efuse_read(EFUSE_VREF_ROMODT + count * 2); //DW12~15
        if((value & 0xF) == TAG_FOR_VREF_ODT_CFG)
        {
            currentTag = 1;//already exsit Vref tag
            value >>= 8;
            if((value & 0x1ff) != VrefCfg)
            {
                updateTag = 1;
            }
            break;
        }
    }


    if(currentTag == 0)//no vref info
    {
        if(VrefCfg != VrefDefaultSet)//need to write efuse
        {
            for(i = 0; i < 4; i++)
            {
                value = efuse_read(EFUSE_VREF_ROMODT + i * 2);
                if(value == 0)
                {
                    if(IS_6855_VERSION_TAG)
                        value = ((0x52 << 20) | (VrefCfg << 8) | 0 << 4 | TAG_FOR_VREF_ODT_CFG);
                    else
                        value = ((0x44 << 20) | (VrefCfg << 8) | 3 << 4 | TAG_FOR_VREF_ODT_CFG);
                    efuse_write(EFUSE_VREF_ROMODT + i * 2, value);
                    hasUpdate = 1;
                    break;
                }
            }
            if(hasUpdate != 1)
            {
                ret = ERR_UPDATE_VREF;
                llfDbgPrintk(ALWAYS_MSG, "vref update fail\r\n");
                AddErrorMessage(0, 0, ret);
            }
        }
    }
    else if(updateTag == 1)//already exsit vref tag,but diff with calibrated value,need update
    {
        if(count >= 3)
        {
            ret = ERR_UPDATE_VREF;
            llfDbgPrintk(ALWAYS_MSG, "Efuse no memory\r\n");
            AddErrorMessage(0, 0, ret);
        }
        else
        {
            for(j = count + 1; j < 4; j++)
            {
                value = efuse_read(EFUSE_VREF_ROMODT + j * 2);
                if(value == 0)
                {
                    if(IS_6855_VERSION_TAG)
                        value = ((0x52 << 20) | (gulVrefCfg << 8) | 0 << 4 | TAG_FOR_VREF_ODT_CFG);
                    else
                        value = ((0x44 << 20) | (gulVrefCfg << 8) | 3 << 4 | TAG_FOR_VREF_ODT_CFG);
                    efuse_write(EFUSE_VREF_ROMODT + j * 2, value);
                    hasUpdate = 1;
                    break;
                }
            }
            if(hasUpdate != 1)
            {
                ret = ERR_UPDATE_VREF;
                llfDbgPrintk(ALWAYS_MSG, "vref update fail\r\n");
                AddErrorMessage(0, 0, ret);
            }
        }
    }

    if (pResponseInfo->err_msg_num != 0)
    {
        llfDbgPrintk(ALWAYS_MSG, "Failed to update vref\r\n");
        pResponseInfo->res_state = VENDOR_CMD_IDLE;
        pResponseInfo->res_progress = 100;
        pResponseInfo->res_err_code = ERR_UPDATE_VREF;
        return;
    }
}
#endif

#ifdef AUTO_DETECT_DIE
// sync SSDMP auto gen method
void ChangeDieMappingCEforMTLandAIPR()
{
    U8 ubCh, ubLoopCnt, ubSkipCHShift = 0;
    U32 ulShiftAddr, ulShiftValue, ulDieMapAddr;
    U32 i, ulMapSample[2][2];

    // special case
    if ((NandPara.ubChNum == 1) || ((NandPara.ubChNum == 4) && (NandPara.ubCENumPerCh == 4)))
    {
        printk("[ADD][MtLun] No change CE map with %d x %d\r\n", NandPara.ubChNum, NandPara.ubCENumPerCh);
        return;
    }

    ulDieMapAddr = CONFIG_BASE_VA_ADDR + SBLK_OFFSET_FC_DIE_MAPPING_CE;
    ulMapSample[0][0] = _MEM32(ulDieMapAddr);
    ulMapSample[0][1] = _MEM32(ulDieMapAddr + 4);
    ulMapSample[1][0] = _MEM32(ulDieMapAddr + CMD_FIFO_NUM_PER_CH);
    ulMapSample[1][1] = _MEM32(ulDieMapAddr + CMD_FIFO_NUM_PER_CH + 4);

    /* CH0: x x C 4 x x 8 0
       CH1: D 5 x x 9 1 x x
       CH2: x x E 6 x x A 2
       CH3: F 7 x x B 3 x x */
    if ((NandPara.ubChNum == 4) && (NandPara.ubCENumPerCh == 1) &&
            (_MEM08(ulDieMapAddr) == 0) &&
            (_MEM08(ulDieMapAddr + CMD_FIFO_NUM_PER_CH + 2) == 1))
    {
        ubLoopCnt = 2;
        ulMapSample[0][0] = 0x18100800;
        ulMapSample[0][1] = 0x1C140C04;
        ulMapSample[1][0] = 0x09011911;
        ulMapSample[1][1] = 0x0D051D15;
    }
    /* 2-CE ex:
       CH0: x x 6 4 x x 2 0
       CH1: x x x x x x x x
       CH2: x x 7 5 x x 3 1
       CH3: x x x x x x x x */
    else if ((NandPara.ubChNum == 2) &&
             (_MEM08(ulDieMapAddr) == 0) &&
             (_MEM08(ulDieMapAddr + (CMD_FIFO_NUM_PER_CH * 2)) == 1))
    {
        ubLoopCnt = 2;
        ubSkipCHShift = 1;
        if (NandPara.ubCENumPerCh == 1)
        {
            ulMapSample[0][0] = 0x0C080400;
            ulMapSample[0][1] = 0x0E0A0602;
            ulMapSample[1][0] = 0x1C181410;
            ulMapSample[1][1] = 0x1E1A1612;
        }
        else if (NandPara.ubCENumPerCh == 2)
        {
            ulMapSample[0][0] = 0x0A080200;
            ulMapSample[0][1] = 0x0E0C0604;
            ulMapSample[1][0] = 0x1A181210;
            ulMapSample[1][1] = 0x1E1C1614;
        }
        else
        {
            ulMapSample[0][0] = 0x06040200;
            ulMapSample[0][1] = 0x0E0C0A08;
            ulMapSample[1][0] = 0x16141210;
            ulMapSample[1][1] = 0x1E1C1A18;
        }
    }
    else
    {
        ubLoopCnt = 1;
        if ((NandPara.ubCENumPerCh > 1) &&
                (_MEM08(ulDieMapAddr) == 0) &&
                (_MEM08(ulDieMapAddr + 2) == NandPara.ubChNum))
        {
            if (NandPara.ubChNum == 4)
            {
                ulMapSample[0][0] = 0x18041000;
                ulMapSample[0][1] = 0x1C0C1408;
            }
            else // ???
            {
                ulMapSample[0][0] = 0x0C020800;
                ulMapSample[0][1] = 0x0E060A04;
            }
        }
        else // normal
        {
            if (NandPara.ubChNum == 4)
            {
                if (NandPara.ubCENumPerCh == 1)
                {
                    ulMapSample[0][0] = 0x18100800;
                    ulMapSample[0][1] = 0x1C140C04;
                }
                else if (NandPara.ubCENumPerCh == 2)
                {
                    ulMapSample[0][0] = 0x14100400;
                    ulMapSample[0][1] = 0x1C180C08;
                }
            }
            else if (NandPara.ubChNum == 2)
            {
                if (NandPara.ubCENumPerCh == 1)
                {
                    ulMapSample[0][0] = 0x0C080400;
                    ulMapSample[0][1] = 0x0E0A0602;
                }
                else if (NandPara.ubCENumPerCh == 2)
                {
                    ulMapSample[0][0] = 0x0A080200;
                    ulMapSample[0][1] = 0x0E0C0604;
                }
                else //if (NandPara.ubCENumPerCh == 4)
                {
                    ulMapSample[0][0] = 0x06040200;
                    ulMapSample[0][1] = 0x0E0C0A08;
                }
            }
        }
    }

    for (ubCh = 0; ubCh < (NandPara.ubChNum << ubSkipCHShift); ubCh += ubLoopCnt)
    {
        for (i = 0; i < ubLoopCnt; i++)
        {
            ulShiftAddr = ((ubCh + i) * CMD_FIFO_NUM_PER_CH);
            ulShiftValue = ((ubCh >> ubSkipCHShift) * 0x01010101);
            _MEM32(ulDieMapAddr + ulShiftAddr) = ulMapSample[i][0] + ulShiftValue;
            _MEM32(ulDieMapAddr + ulShiftAddr + 4) = ulMapSample[i][1] + ulShiftValue;
        }
    }
    for (i = ((NandPara.ubChNum << ubSkipCHShift) * CMD_FIFO_NUM_PER_CH); i < 64; i++)
    {
        _MEM08(ulDieMapAddr + i) = i;
    }

    printk("[ADD][MtLun] DieMappingCE:\r\n");
    for (ubCh = 0; ubCh < 8; ubCh++)
    {
        printk("  %x_%x\r\n",
               _MEM32(ulDieMapAddr + (ubCh * CMD_FIFO_NUM_PER_CH) + 4),
               _MEM32(ulDieMapAddr + (ubCh * CMD_FIFO_NUM_PER_CH)));
    }
}
#endif

void CreateBankConfigByDieMap(void)
{
    U8 ubCh, ubCe, ubLun, ubMaxBankNum, ubBank;
    U8 ubDieNumPerCE, ubDieNumPerCEMax, ubCeNum, ubTotalDieNum, ubChNum;
    U8 ubChDie[CH_NUM_MAX], ubRealCh[CH_NUM_MAX];
    U8 ubLunInterleave;
    U32 i, ulDiemap;

    gulBankMapPro[0] = 0;
    gulBankMapPro[1] = 0;
    gubTrueBankNum = 0;
    ubMaxBankNum = 0xF;
    ubDieNumPerCE = 0;
    ubDieNumPerCEMax = 0;
    ubCeNum = 0;
    ubTotalDieNum = 0;
    ubChNum = 0;
    ubBank = 0;

    for(ubCh = 0; ubCh < CH_NUM_MAX; ubCh++)
    {
        ubChDie[ubCh] = 0;
        ubRealCh[ubCh] = 0xFF;
        if(gulCHDieMap[ubCh][0] != 0 || gulCHDieMap[ubCh][1] != 0)
        {
            ubRealCh[ubCh] = ubChNum;
            ubChNum++;
        }
    }

    for(ubCh = 0; ubCh < CH_NUM_MAX; ubCh++)
    {
        for(ubCe = 0; ubCe < CE_NUM_MAX; ubCe++)
        {
            if(ubCe < 4)
            {
                ulDiemap = gulCHDieMap[ubCh][0] >> (ubCe * 8) & 0xFF;
            }
            else
            {
                ulDiemap = gulCHDieMap[ubCh][1] >> ((ubCe - 4) * 8) & 0xFF;
            }
            if(ulDiemap != 0)
            {
                ubLun = 0xFF;
                ubDieNumPerCE = 0;
                ubCeNum++;
                for(i = 0; i < 8; i ++)
                {
                    if(((ulDiemap >> i) & 0x1) == 1)
                    {
                        if (ubLun == 0xFF)
                        {
                            ubLun = i; // record first Lun index
                        }
                        ubDieNumPerCE++;
                        ubTotalDieNum++;
                    }
                }
                if (ubLun == 0xFF)
                {
                    printk("[ERR][ADD] find no Lun %d_%d\r\n", ubCh, ubCe);
                    ubLun = 0;
                }
                if(ubDieNumPerCE > ubDieNumPerCEMax)
                {
                    ubDieNumPerCEMax = ubDieNumPerCE;
                }

                ubBank = (ubChDie[ubCh]++) * ubChNum + ubRealCh[ubCh];
                _MEM08(CONFIG_BASE_VA_ADDR + SBLK_OFFSET_FC_DIE_MAPPING_CE + (ubCh * 8) + ubCe) = ubBank;
                printk("[ADD] dieMap Ch %d Ce %d bank %d\r\n", ubCh, ubCe, ubBank);

                if (ubBank < 8)
                {
                    gulBankMapPro[0] = gulBankMapPro[0] | (ubLun << (ubBank * 4));
                }
                else if (ubBank >= 8)
                {
                    gulBankMapPro[1] = gulBankMapPro[1] | (ubLun << ((ubBank - 8) * 4));
                }
                gubTrueBankNum++;
            }
            else
            {
                _MEM08(CONFIG_BASE_VA_ADDR + SBLK_OFFSET_FC_DIE_MAPPING_CE + (ubCh * 8) + ubCe) = ubMaxBankNum;
                ubMaxBankNum--;
            }
        }
    }
    NandPara.ubBankNum = gubTrueBankNum;
    NandPara.ubChNum = ubChNum;
    NandPara.ubCENumPerCh = ubCeNum / ubChNum;

    // gubIsSerialMultiLUN
    // 0 for normal, 1 for serial lun,
    // 2 for 1-Lun but NOT Lun0 in CE
    // 3 for Multi-Lun need Auto-detect
    if(ubDieNumPerCEMax == 1)
    {
        gubIsSerialMultiLUN = 2;
        gubRealLunNum = 1;
        guwRealBlockNum = NandPara.uwBlockNumPerLun;
        NandPara.ubLunNumPerCE = 1;
    }
    else if(ubDieNumPerCEMax > 1)
    {
        gubIsSerialMultiLUN = 3;
        gubRealLunNum = ubDieNumPerCEMax;
        guwRealBlockNum = NandPara.uwBlockNumPerLun;

        ubLunInterleave = _MEM08(CONFIG_BASE_VA_ADDR + SBLK_OFFSET_LUN_INTERLEAVE);

        if (ubLunInterleave)
        {
#ifdef MTL_SUPPORT_NAND
            // allow to do MTL in 6577
            NandPara.ubBankNum = (NandPara.ubChNum * NandPara.ubCENumPerCh * gubRealLunNum);
            NandPara.ubLunNumPerCE = ubDieNumPerCEMax;
#ifdef RL6577_VA
            // 6577 can't support larger than 16 banks, not use BANK_NUM_MAX here
            if (NandPara.ubBankNum > 16)
            {
                printk("[WARN][ADD] 6577 can't support banknum %d do MTL, change to extend\r\n",
                       NandPara.ubBankNum);
                NandPara.ubBankNum = (NandPara.ubChNum * NandPara.ubCENumPerCh);
                ubLunInterleave = 0;
            }
#elif defined(RTS5771_FPGA) || defined(RTS5771_VA)
            if (NandPara.ubBankNum > BANK_NUM_MAX)
            {
                printk("[WARN][ADD] 6817 can't support banknum %d do MTL, change to extend\r\n",
                       NandPara.ubBankNum);
                NandPara.ubBankNum = (NandPara.ubChNum * NandPara.ubCENumPerCh);
                ubLunInterleave = 0;
            }
#endif
#else       // current nand not support interleave
            printk("[WARN][ADD] Current nand not support MTL, change to extend\r\n");
            ubLunInterleave = 0;
#endif
        }
        if (!ubLunInterleave)
        {
            NandPara.uwBlockNumPerLun = NandPara.uwBlockNumPerLun * ubDieNumPerCEMax;
            NandPara.ubBlockNumPerLunShift = (U16)cal_shift((U32)NandPara.uwBlockNumPerLun);
            if(NandPara.uwBlockNumPerLun % (1 << NandPara.ubBlockNumPerLunShift))
            {
                NandPara.ubBlockNumPerLunShift++;
            }
            NandPara.ubLunNumPerCE = 1;
        }
    }
    else
    {
        printk("[ERR][ADD] Wrong gulCHDieMap, no die is detected\r\n");
    }

    printk("[ADD] IsSerialMultiLUN %d\r\n", gubIsSerialMultiLUN);

    NandPara.ubBankNumShift = (U8)cal_shift((U32)NandPara.ubBankNum);
    if(NandPara.ubBankNum % (1 << NandPara.ubBankNumShift))
    {
        NandPara.ubBankNumShift++;
    }
    NandPara.ubLunNumPerCEShift = (U8)cal_shift((U32)NandPara.ubLunNumPerCE);
    NandPara.uwMpBlockNumPerLun = (NandPara.uwBlockNumPerLun / NandPara.ubPlaneNumPerLun);
    NandPara.ubMpBlockNumPerLunShift = (NandPara.ubBlockNumPerLunShift -
                                        NandPara.ubPlaneNumPerLunShift);

    printk("----> BankNum = %d, TotalDie = %d <----\r\n", NandPara.ubBankNum, ubTotalDieNum);
    printk("----> ChNum = %d, CENumPerCh = %d <----\r\n", NandPara.ubChNum, NandPara.ubCENumPerCh);
    printk("----> RealLunNum = %d, LunNumPerCE = %d_%d <----\r\n", gubRealLunNum,
           NandPara.ubLunNumPerCE, NandPara.ubLunNumPerCEShift);

#ifdef AUTO_DETECT_DIE
    // sync SSDMP auto gen method
    ChangeDieMappingCEforMTLandAIPR();
#endif
}

