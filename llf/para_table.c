//====master====
//#ifdef BL_ROM
//#include "bl_basetype.h"
//#include "bl_serial.h"
//===unicore2===
#include "lib_basetype.h"
#include "serial.h"
//==============

#include "lib_retcode.h"
#include "nand_test.h"



#if defined(FTL_B16A)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para;
    U32 ulU32Para;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;
    U16 uwBlkEndurance = 1;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_ONFI) //ONFI
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] ONFI\r\n", ubStr);
        }

        if ((_REG16(ulParaTableAddr + 4) != 0x03FE) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("004 [ERR] Revision number: 0x%x [Spec] 0x03FE\r\n", _REG16(ulParaTableAddr + 4));
        }

        if (_REG32(ulParaTableAddr + 4) != 0)
        {
            printk("006 MTL support:%d\r\n", _REG16(ulParaTableAddr + 6) & 0x0002);
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0008) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Prog/Erase:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0040) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Read:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x2000) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] NV-DDR3:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }

        if (((_REG16(ulParaTableAddr + 8) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 COPYBACK:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x0800) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] ODT:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x1000) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] GET/SET Feature by LUN:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }

        if ((_REG08(ulParaTableAddr + 10) != 0x0F) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("010 [ERR] ONFI-JEDEC JTG primary advanced command: 0x%x [Spec] 0x0F\r\n",
                   _REG08(ulParaTableAddr + 10));
        }

        for(i = 0; i < 12; i++) //Micron / Intel
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        if ((_REG32(ulParaTableAddr + 80) != 0x00004000) && (_REG32(ulParaTableAddr + 80) != 0))
        {
            printk("080 [ERR] Data Bytes pre Page: 0x%x [Spec] 0x00004000\r\n", _REG32(ulParaTableAddr + 80));
        }
        if ((_REG16(ulParaTableAddr + 84) != 0x08A0) && (_REG32(ulParaTableAddr + 84) != 0))
        {
            printk("084 [ERR] Spare Bytes pre Page: 0x%x [Spec] 0x08A0\r\n", _REG16(ulParaTableAddr + 84));
        }
        if ((_REG32(ulParaTableAddr + 92) != 0x00000900) && (_REG32(ulParaTableAddr + 92) != 0))
        {
            printk("092 [ERR] Pages pre Blk: 0x%x [Spec] 0x00000900\r\n", _REG32(ulParaTableAddr + 92));
        }
        if ((_REG32(ulParaTableAddr + 96) != 0x000003F0) && (_REG32(ulParaTableAddr + 96) != 0))
        {
            printk("096 [ERR] Blks pre LUN: 0x%x [Spec] 0x000003F0\r\n", _REG32(ulParaTableAddr + 96));
        }
        printk("100 LUNs pre CE: 0x%x\r\n", _REG08(ulParaTableAddr + 100));

        uwU16Para = _REG08(ulParaTableAddr + 104);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 103);
        if (uwU16Para == 0x0034)
        {
            printk("103 [Fortis] MaxBadBlkPreLun: 0x0034\r\n");
        }
        else
        {
            printk("103 [ERR] MaxBadBlkPreLun: 0x%x\r\n", uwU16Para);
        }

        uwU16Para = _REG08(ulParaTableAddr + 106);
        for (i = 0; i < uwU16Para; i++)
        {
            uwBlkEndurance *= 10;
        }
        uwBlkEndurance *= _REG08(ulParaTableAddr + 105);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 105);
        if (uwU16Para == 0x020F)
        {
            printk("105 [Fortis] Blk endurance: 0x020F ->%d\r\n", uwBlkEndurance);
        }
        else
        {
            printk("105 [ERR] Blk endurance: 0x%x ->%d\r\n", uwU16Para, uwBlkEndurance);
        }

        if (_REG08(ulParaTableAddr + 158) != 0x1B)
        {
            printk("158 [ERR] NV_DDR2/3 features: 0x%x [Spec] 0x1B\r\n", _REG08(ulParaTableAddr + 158));
        }

        if (_REG16(ulParaTableAddr + 160) == 0x007F)
        {
            printk("160 [Fortis] NV_DDR3 timing mode support: 0x007F\r\n");
        }
        else if (_REG16(ulParaTableAddr + 160) != 0)
        {
            printk("160 [ERR] NV_DDR3 timing mode support: 0x%x\r\n", _REG16(ulParaTableAddr + 160));
        }

        if (_REG08(ulParaTableAddr + 180) != 0x10)
        {
            printk("180 [ERR] Read Retry Options: 0x%x [Spec] 0x10\r\n", _REG08(ulParaTableAddr + 180));
        }

        ulU32Para = _REG08(ulParaTableAddr + 184);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 183);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 182);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 181);
        if (ulU32Para != 0x0000FFFF)
        {
            printk("181 [ERR] Read Retry Options available: 0x%x [Spec] 0x0000FFFF\r\n", ulU32Para);
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_B17A)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para;
    U32 ulU32Para;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;
    U16 uwBlkEndurance = 1;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_ONFI) //ONFI
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] ONFI\r\n", ubStr);
        }

        if ((_REG16(ulParaTableAddr + 4) != 0x03FE) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("004 [ERR] Revision number: 0x%x [Spec] 0x03FE\r\n", _REG16(ulParaTableAddr + 4));
        }

        if (_REG32(ulParaTableAddr + 4) != 0)
        {
            printk("006 MTL support:%d\r\n", _REG16(ulParaTableAddr + 6) & 0x0002);
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0008) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Prog/Erase:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0040) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Read:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x2000) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] NV-DDR3:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }

        if (((_REG16(ulParaTableAddr + 8) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 COPYBACK:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x0800) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] ODT:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x1000) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] GET/SET Feature by LUN:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }

        if ((_REG08(ulParaTableAddr + 10) != 0x0F) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("010 [ERR] ONFI-JEDEC JTG primary advanced command: 0x%x [Spec] 0x0F\r\n",
                   _REG08(ulParaTableAddr + 10));
        }

        for(i = 0; i < 12; i++) //Micron / Intel
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        if ((_REG32(ulParaTableAddr + 80) != 0x00004000) && (_REG32(ulParaTableAddr + 80) != 0))
        {
            printk("080 [ERR] Data Bytes pre Page: 0x%x [Spec] 0x00004000\r\n", _REG32(ulParaTableAddr + 80));
        }
        if ((_REG16(ulParaTableAddr + 84) != 0x08A0) && (_REG32(ulParaTableAddr + 84) != 0))
        {
            printk("084 [ERR] Spare Bytes pre Page: 0x%x [Spec] 0x08A0\r\n", _REG16(ulParaTableAddr + 84));
        }
        if ((_REG32(ulParaTableAddr + 92) != 0x00000900) && (_REG32(ulParaTableAddr + 92) != 0))
        {
            printk("092 [ERR] Pages pre Blk: 0x%x [Spec] 0x00000900\r\n", _REG32(ulParaTableAddr + 92));
        }
        if ((_REG32(ulParaTableAddr + 96) != 0x000007E0) && (_REG32(ulParaTableAddr + 96) != 0))
        {
            printk("096 [ERR] Blks pre LUN: 0x%x [Spec] 0x000007E0\r\n", _REG32(ulParaTableAddr + 96));
        }
        printk("100 LUNs pre CE: 0x%x\r\n", _REG08(ulParaTableAddr + 100));

        uwU16Para = _REG08(ulParaTableAddr + 104);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 103);
        if (uwU16Para == 0x0068)
        {
            printk("103 [Fortis] MaxBadBlkPreLun: 0x0068\r\n");
        }
        else
        {
            printk("103 [ERR] MaxBadBlkPreLun: 0x%x\r\n", uwU16Para);
        }

        uwU16Para = _REG08(ulParaTableAddr + 106);
        for (i = 0; i < uwU16Para; i++)
        {
            uwBlkEndurance *= 10;
        }
        uwBlkEndurance *= _REG08(ulParaTableAddr + 105);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 105);
        if (uwU16Para == 0x020F)
        {
            printk("105 [Fortis] Blk endurance: 0x020F ->%d\r\n", uwBlkEndurance);
        }
        else if (uwU16Para == 0x030A)
        {
            printk("105 [FortisMax] Blk endurance: 0x030A ->%d\r\n", uwBlkEndurance);
        }
        else
        {
            printk("105 [ERR] Blk endurance: 0x%x ->%d\r\n", uwU16Para, uwBlkEndurance);
        }

        if (_REG08(ulParaTableAddr + 158) != 0x1B)
        {
            printk("158 [ERR] NV_DDR2/3 features: 0x%x [Spec] 0x1B\r\n", _REG08(ulParaTableAddr + 158));
        }

        if (_REG16(ulParaTableAddr + 160) == 0x007F)
        {
            printk("160 [Fortis] NV_DDR3 timing mode support: 0x007F\r\n");
        }
        else
        {
            printk("160 [ERR] NV_DDR3 timing mode support: 0x%x\r\n", _REG16(ulParaTableAddr + 160));
        }

        if (_REG08(ulParaTableAddr + 180) != 0x10)
        {
            printk("180 [ERR] Read Retry Options: 0x%x [Spec] 0x10\r\n", _REG08(ulParaTableAddr + 180));
        }

        ulU32Para = _REG08(ulParaTableAddr + 184);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 183);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 182);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 181);
        if (ulU32Para != 0x0000FFFF)
        {
            printk("181 [ERR] Read Retry Options available: 0x%x [Spec] 0x0000FFFF\r\n", ulU32Para);
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_B27A)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para;
    U32 ulU32Para;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;
    U16 uwBlkEndurance = 1;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_ONFI) //ONFI
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] ONFI\r\n", ubStr);
        }

        if ((_REG16(ulParaTableAddr + 4) != 0x03FE) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("004 [ERR] Revision number: 0x%x [Spec] 0x03FE\r\n", _REG16(ulParaTableAddr + 4));
        }

        if (_REG32(ulParaTableAddr + 4) != 0)
        {
            printk("006 MTL support:%d\r\n", _REG16(ulParaTableAddr + 6) & 0x0002);
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0008) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Prog/Erase:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0040) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Read:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x2000) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] NV-DDR3:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }

        if (((_REG16(ulParaTableAddr + 8) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 COPYBACK:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x0800) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] ODT:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x1000) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] GET/SET Feature by LUN:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }

        if ((_REG08(ulParaTableAddr + 10) != 0x0F) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("010 [ERR] ONFI-JEDEC JTG primary advanced command: 0x%x [Spec] 0x0F\r\n",
                   _REG08(ulParaTableAddr + 10));
        }

        for(i = 0; i < 12; i++) //Micron / Intel
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        if ((_REG32(ulParaTableAddr + 80) != 0x00004000) && (_REG32(ulParaTableAddr + 80) != 0))
        {
            printk("080 [ERR] Data Bytes pre Page: 0x%x [Spec] 0x00004000\r\n", _REG32(ulParaTableAddr + 80));
        }
        if ((_REG16(ulParaTableAddr + 84) != 0x08A0) && (_REG32(ulParaTableAddr + 84) != 0))
        {
            printk("084 [ERR] Spare Bytes pre Page: 0x%x [Spec] 0x08A0\r\n", _REG16(ulParaTableAddr + 84));
        }
        if ((_REG32(ulParaTableAddr + 92) != 0x00001440) && (_REG32(ulParaTableAddr + 92) != 0))
        {
            printk("092 [ERR] Pages pre Blk: 0x%x [Spec] 0x00001440\r\n", _REG32(ulParaTableAddr + 92));
        }
        if ((_REG32(ulParaTableAddr + 96) != 0x000003B0) && (_REG32(ulParaTableAddr + 96) != 0))
        {
            printk("096 [ERR] Blks pre LUN: 0x%x [Spec] 0x000003B0\r\n", _REG32(ulParaTableAddr + 96));
        }
        printk("100 LUNs pre CE: 0x%x\r\n", _REG08(ulParaTableAddr + 100));

        uwU16Para = _REG08(ulParaTableAddr + 104);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 103);
        if (uwU16Para == 0x0038)
        {
            printk("103 [Fortis] MaxBadBlkPreLun: 0x0038\r\n");
        }
        else
        {
            printk("103 [ERR] MaxBadBlkPreLun: 0x%x\r\n", uwU16Para);
        }

        uwU16Para = _REG08(ulParaTableAddr + 106);
        for (i = 0; i < uwU16Para; i++)
        {
            uwBlkEndurance *= 10;
        }
        uwBlkEndurance *= _REG08(ulParaTableAddr + 105);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 105);
        if (uwU16Para == 0x0302)
        {
            printk("105 [Fortis] Blk endurance: 0x0302 ->%d\r\n", uwBlkEndurance);
        }
        else if (uwU16Para == 0x030A)
        {
            printk("105 [FortisMax] Blk endurance: 0x030A ->%d\r\n", uwBlkEndurance);
        }
        else
        {
            printk("105 [ERR] Blk endurance: 0x%x ->%d\r\n", uwU16Para, uwBlkEndurance);
        }

        if (_REG08(ulParaTableAddr + 158) != 0x1B)
        {
            printk("158 [ERR] NV_DDR2/3 features: 0x%x [Spec] 0x1B\r\n", _REG08(ulParaTableAddr + 158));
        }

        if (_REG16(ulParaTableAddr + 160) == 0x00FF)
        {
            printk("160 [Fortis] NV_DDR3 timing mode support: 0x00FF\r\n");
        }
        else
        {
            printk("160 [ERR] NV_DDR3 timing mode support: 0x%x\r\n", _REG16(ulParaTableAddr + 160));
        }

        if (_REG08(ulParaTableAddr + 180) != 0x14)
        {
            printk("180 [ERR] Read Retry Options: 0x%x [Spec] 0x14\r\n", _REG08(ulParaTableAddr + 180));
        }

        ulU32Para = _REG08(ulParaTableAddr + 184);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 183);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 182);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 181);
        if (ulU32Para != 0x000FFFFF)
        {
            printk("181 [ERR] Read Retry Options available: 0x%x [Spec] 0x000FFFFF\r\n", ulU32Para);
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_B27B)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para;
    U32 ulU32Para;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;
    U16 uwBlkEndurance = 1;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_ONFI) //ONFI
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] ONFI\r\n", ubStr);
        }

        if ((_REG16(ulParaTableAddr + 4) != 0x0400) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("004 [ERR] Revision number: 0x%x [Spec] 0x0400\r\n", _REG16(ulParaTableAddr + 4));
        }

        if (_REG32(ulParaTableAddr + 4) != 0)
        {
            printk("006 MTL support:%d\r\n", _REG16(ulParaTableAddr + 6) & 0x0002);
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0008) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Prog/Erase:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0040) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Read:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x2000) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] NV-DDR3:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }

        if (((_REG16(ulParaTableAddr + 8) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 COPYBACK:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x0800) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] ODT:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x1000) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] GET/SET Feature by LUN:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }

        if ((_REG08(ulParaTableAddr + 10) != 0x0F) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("010 [ERR] ONFI-JEDEC JTG primary advanced command: 0x%x [Spec] 0x0F\r\n",
                   _REG08(ulParaTableAddr + 10));
        }

        for(i = 0; i < 12; i++) //Micron / Intel
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        if ((_REG32(ulParaTableAddr + 80) != 0x00004000) && (_REG32(ulParaTableAddr + 80) != 0))
        {
            printk("080 [ERR] Data Bytes pre Page: 0x%x [Spec] 0x00004000\r\n", _REG32(ulParaTableAddr + 80));
        }
        if ((_REG16(ulParaTableAddr + 84) != 0x08A0) && (_REG32(ulParaTableAddr + 84) != 0))
        {
            printk("084 [ERR] Spare Bytes pre Page: 0x%x [Spec] 0x08A0\r\n", _REG16(ulParaTableAddr + 84));
        }
        if ((_REG32(ulParaTableAddr + 92) != 0x00000D80) && (_REG32(ulParaTableAddr + 92) != 0))
        {
            printk("092 [ERR] Pages pre Blk: 0x%x [Spec] 0x00000D80\r\n", _REG32(ulParaTableAddr + 92));
        }
        if ((_REG32(ulParaTableAddr + 96) != 0x00000548) && (_REG32(ulParaTableAddr + 96) != 0))
        {
            printk("096 [ERR] Blks pre LUN: 0x%x [Spec] 0x00000548\r\n", _REG32(ulParaTableAddr + 96));
        }
        printk("100 LUNs pre CE: 0x%x\r\n", _REG08(ulParaTableAddr + 100));

        uwU16Para = _REG08(ulParaTableAddr + 104);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 103);
        if (uwU16Para == 0x0050)
        {
            printk("103 [Fortis] MaxBadBlkPreLun: 0x0050\r\n");
        }
        else if (uwU16Para == 0x0048)
        {
            printk("103 [FortisMax] MaxBadBlkPreLun: 0x0048\r\n");
        }
        else
        {
            printk("103 [ERR] MaxBadBlkPreLun: 0x%x\r\n", uwU16Para);
        }

        uwU16Para = _REG08(ulParaTableAddr + 106);
        for (i = 0; i < uwU16Para; i++)
        {
            uwBlkEndurance *= 10;
        }
        uwBlkEndurance *= _REG08(ulParaTableAddr + 105);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 105);
        if (uwU16Para == 0x0303)
        {
            printk("105 [Fortis] Blk endurance: 0x0303 ->%d\r\n", uwBlkEndurance);
        }
        else if (uwU16Para == 0x024B)
        {
            printk("105 [FortisMax] Blk endurance: 0x024B ->%d\r\n", uwBlkEndurance);
        }
        else
        {
            printk("105 [ERR] Blk endurance: 0x%x ->%d\r\n", uwU16Para, uwBlkEndurance);
        }

        if (_REG08(ulParaTableAddr + 158) != 0x1B)
        {
            printk("158 [ERR] NV_DDR2/3 features: 0x%x [Spec] 0x1B\r\n", _REG08(ulParaTableAddr + 158));
        }

        if (_REG16(ulParaTableAddr + 160) == 0x03FF)
        {
            printk("160 [Fortis] NV_DDR3 timing mode support: 0x03FF\r\n");
        }
        else
        {
            printk("160 [ERR] NV_DDR3 timing mode support: 0x%x\r\n", _REG16(ulParaTableAddr + 160));
        }

        if (_REG08(ulParaTableAddr + 180) != 0x14)
        {
            printk("180 [ERR] Read Retry Options: 0x%x [Spec] 0x14\r\n", _REG08(ulParaTableAddr + 180));
        }

        ulU32Para = _REG08(ulParaTableAddr + 184);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 183);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 182);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 181);
        if (ulU32Para != 0x000FFFFF)
        {
            printk("181 [ERR] Read Retry Options available: 0x%x [Spec] 0x000FFFFF\r\n", ulU32Para);
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_B37R)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para;
    U32 ulU32Para;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;
    U16 uwBlkEndurance = 1;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_ONFI) //ONFI
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] ONFI\r\n", ubStr);
        }

        if ((_REG16(ulParaTableAddr + 4) != 0x0600) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("004 [ERR] Revision number: 0x%x [Spec] 0x0600\r\n", _REG16(ulParaTableAddr + 4));
        }

        if (_REG32(ulParaTableAddr + 4) != 0)
        {
            printk("006 MTL support:%d\r\n", _REG16(ulParaTableAddr + 6) & 0x0002);
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0008) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Prog/Erase:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0040) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Read:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x2000) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] NV-DDR3:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }

        if (((_REG16(ulParaTableAddr + 8) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 COPYBACK:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x0800) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] ODT:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x1000) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] GET/SET Feature by LUN:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }

        if ((_REG08(ulParaTableAddr + 10) != 0x0F) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("010 [ERR] ONFI-JEDEC JTG primary advanced command: 0x%x [Spec] 0x0F\r\n",
                   _REG08(ulParaTableAddr + 10));
        }

        for(i = 0; i < 12; i++) //Micron / Intel
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        if ((_REG32(ulParaTableAddr + 80) != 0x00004000) && (_REG32(ulParaTableAddr + 80) != 0))
        {
            printk("080 [ERR] Data Bytes pre Page: 0x%x [Spec] 0x00004000\r\n", _REG32(ulParaTableAddr + 80));
        }
        if ((_REG16(ulParaTableAddr + 84) != 0x07A8) && (_REG32(ulParaTableAddr + 84) != 0))
        {
            printk("084 [ERR] Spare Bytes pre Page: 0x%x [Spec] 0x07A8\r\n", _REG16(ulParaTableAddr + 84));
        }
        if ((_REG32(ulParaTableAddr + 92) != 0x00000600) && (_REG32(ulParaTableAddr + 92) != 0))
        {
            printk("092 [ERR] Pages pre Blk: 0x%x [Spec] 0x00000600\r\n", _REG32(ulParaTableAddr + 92));
        }
        if ((_REG32(ulParaTableAddr + 96) != 0x00000B90) && (_REG32(ulParaTableAddr + 96) != 0))
        {
            printk("096 [ERR] Blks pre LUN: 0x%x [Spec] 0x00000B90\r\n", _REG32(ulParaTableAddr + 96));
        }
        printk("100 LUNs pre CE: 0x%x\r\n", _REG08(ulParaTableAddr + 100));

        uwU16Para = _REG08(ulParaTableAddr + 104);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 103);
        if (uwU16Para == 0x0090)
        {
            printk("103 [Media] MaxBadBlkPreLun: 0x0090\r\n");
        }
        else
        {
            printk("103 [ERR] MaxBadBlkPreLun: 0x%x\r\n", uwU16Para);
        }

        uwU16Para = _REG08(ulParaTableAddr + 106);
        for (i = 0; i < uwU16Para; i++)
        {
            uwBlkEndurance *= 10;
        }
        uwBlkEndurance *= _REG08(ulParaTableAddr + 105);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 105);
        if (uwU16Para == 0x0305)
        {
            printk("105 [Media] Blk endurance: 0x0305 ->%d\r\n", uwBlkEndurance);
        }
        else
        {
            printk("105 [ERR] Blk endurance: 0x%x ->%d\r\n", uwU16Para, uwBlkEndurance);
        }

        if (_REG08(ulParaTableAddr + 158) != 0x1B)
        {
            printk("158 [ERR] NV_DDR2/3 features: 0x%x [Spec] 0x1B\r\n", _REG08(ulParaTableAddr + 158));
        }

        if (_REG16(ulParaTableAddr + 160) == 0x03FF)
        {
            printk("160 [Media] NV_DDR3 timing mode support: 0x03FF\r\n");
        }
        else
        {
            printk("160 [ERR] NV_DDR3 timing mode support: 0x%x\r\n", _REG16(ulParaTableAddr + 160));
        }

        if (_REG08(ulParaTableAddr + 180) != 0x14)
        {
            printk("180 [ERR] Read Retry Options: 0x%x [Spec] 0x14\r\n", _REG08(ulParaTableAddr + 180));
        }

        ulU32Para = _REG08(ulParaTableAddr + 184);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 183);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 182);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 181);
        if (ulU32Para != 0x000FFFFF)
        {
            printk("181 [ERR] Read Retry Options available: 0x%x [Spec] 0x000FFFFF\r\n", ulU32Para);
        }

        if ((_REG16(ulParaTableAddr + 186) != 0) || (_REG32(ulParaTableAddr + 188) != 0)
                || (_REG32(ulParaTableAddr + 192) != 0))
        {
            if (_REG08(ulParaTableAddr + 186) != 0xDD)
            {
                printk("186 [ERR] AddressCycleReadRetry enablement Feature Address: 0x%x [Spec] 0xDD\r\n",
                       _REG08(ulParaTableAddr + 186));
            }
            uwU16Para = _REG08(ulParaTableAddr + 188);
            uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 187);
            if (uwU16Para != 0x0101)
            {
                printk("187 [ERR] AddressCycleReadRetry subfeature Position/BitPosition: 0x%x [Spec] 0x0101\r\n",
                       uwU16Para);
            }
            ulU32Para = _REG08(ulParaTableAddr + 192);
            ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 191);
            ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 190);
            ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 189);
            if (ulU32Para != 0x000003FF)
            {
                printk("189 [ERR] AddressCycleReadRetry Options available: 0x%x [Spec] 0x000003FF)\r\n", ulU32Para);
            }
            uwU16Para = _REG08(ulParaTableAddr + 194);
            uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 193);
            if ((uwU16Para & 0x3) == 0x0)
            {
                printk("193 SLC mode Op-Code: 0x0 [DAh/DFh](Feature Address 91h)\r\n");
            }
            else if ((uwU16Para & 0x3) == 0x1)
            {
                printk("193 SLC mode Op-Code: 0x1 [3Bh/3Ch](Feature Address 91h)\r\n");
            }
            else if ((uwU16Para & 0x3) == 0x2)
            {
                printk("193 SLC mode Op-Code: 0x2 [3Bh/3Ch]\r\n");
            }
            else //(uwU16Para & 0x3) == 0x3
            {
                printk("193 SLC mode Op-Code: 0x3 [3Bh]\r\n");
            }
            printk("193 SLC mode support (Factory Reserved Setting): %d\r\n", uwU16Para & 0x4);
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_B47R)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para;
    U32 ulU32Para;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;
    U16 uwBlkEndurance = 1;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_ONFI) //ONFI
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] ONFI\r\n", ubStr);
        }

        if ((_REG16(ulParaTableAddr + 4) != 0x0800) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("004 [ERR] Revision number: 0x%x [Spec] 0x0800\r\n", _REG16(ulParaTableAddr + 4));
        }

        if (_REG32(ulParaTableAddr + 4) != 0)
        {
            printk("006 MTL support:%d\r\n", _REG16(ulParaTableAddr + 6) & 0x0002);
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0008) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Prog/Erase:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0040) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Read:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x2000) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] NV-DDR3:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }

        if (((_REG16(ulParaTableAddr + 8) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 COPYBACK:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x0800) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] ODT:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x1000) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] GET/SET Feature by LUN:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }

        if ((_REG08(ulParaTableAddr + 10) != 0x0F) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("010 [ERR] ONFI-JEDEC JTG primary advanced command: 0x%x [Spec] 0x0F\r\n",
                   _REG08(ulParaTableAddr + 10));
        }

        for(i = 0; i < 12; i++) //Micron / Intel
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        if ((_REG32(ulParaTableAddr + 80) != 0x00004000) && (_REG32(ulParaTableAddr + 80) != 0))
        {
            printk("080 [ERR] Data Bytes pre Page: 0x%x [Spec] 0x00004000\r\n", _REG32(ulParaTableAddr + 80));
        }
        if ((_REG16(ulParaTableAddr + 84) != 0x07B0) && (_REG32(ulParaTableAddr + 84) != 0))
        {
            printk("084 [ERR] Spare Bytes pre Page: 0x%x [Spec] 0x07B0\r\n", _REG16(ulParaTableAddr + 84));
        }
        if ((_REG32(ulParaTableAddr + 92) != 0x00000840) && (_REG32(ulParaTableAddr + 92) != 0))
        {
            printk("092 [ERR] Pages pre Blk: 0x%x [Spec] 0x00000840\r\n", _REG32(ulParaTableAddr + 92));
        }
        if ((_REG32(ulParaTableAddr + 96) != 0x000008B0) && (_REG32(ulParaTableAddr + 96) != 0))
        {
            printk("096 [ERR] Blks pre LUN: 0x%x [Spec] 0x000008B0\r\n", _REG32(ulParaTableAddr + 96));
        }
        printk("100 LUNs pre CE: 0x%x\r\n", _REG08(ulParaTableAddr + 100));

        uwU16Para = _REG08(ulParaTableAddr + 104);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 103);
        if (uwU16Para == 0x0078)
        {
            printk("103 [Fortis] MaxBadBlkPreLun: 0x0078\r\n");
        }
        else if (uwU16Para == 0x00D8)
        {
            printk("103 [Media] MaxBadBlkPreLun: 0x00D8\r\n");
        }
        else
        {
            printk("103 [ERR] MaxBadBlkPreLun: 0x%x\r\n", uwU16Para);
        }

        uwU16Para = _REG08(ulParaTableAddr + 106);
        for (i = 0; i < uwU16Para; i++)
        {
            uwBlkEndurance *= 10;
        }
        uwBlkEndurance *= _REG08(ulParaTableAddr + 105);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 105);
        if (uwU16Para == 0x0303)
        {
            printk("105 [Fortis] Blk endurance: 0x0303 ->%d\r\n", uwBlkEndurance);
        }
        else if (uwU16Para == 0x0207)
        {
            printk("105 [Media] Blk endurance: 0x0207 ->%d\r\n", uwBlkEndurance);
        }
        else
        {
            printk("105 [ERR] Blk endurance: 0x%x ->%d\r\n", uwU16Para, uwBlkEndurance);
        }

        if (_REG08(ulParaTableAddr + 158) != 0x1B)
        {
            printk("158 [ERR] NV_DDR2/3 features: 0x%x [Spec] 0x1B\r\n", _REG08(ulParaTableAddr + 158));
        }

        if (_REG16(ulParaTableAddr + 160) == 0x1FFF)
        {
            printk("160 [Fortis] NV_DDR3 timing mode support: 0x1FFF\r\n");
        }
        else if (_REG16(ulParaTableAddr + 160) == 0x03FF)
        {
            printk("160 [Media] NV_DDR3 timing mode support: 0x03FF\r\n");
        }
        else
        {
            printk("160 [ERR] NV_DDR3 timing mode support: 0x%x\r\n", _REG16(ulParaTableAddr + 160));
        }

        if (_REG08(ulParaTableAddr + 186) != 0x96)
        {
            printk("186 [ERR] AddressCycleReadRetry enablement Feature Address: 0x%x [Spec] 0x96\r\n",
                   _REG08(ulParaTableAddr + 186));
        }
        uwU16Para = _REG08(ulParaTableAddr + 188);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 187);
        if (uwU16Para != 0x0401)
        {
            printk("187 [ERR] AddressCycleReadRetry subfeature Position/BitPosition: 0x%x [Spec] 0x0401\r\n",
                   uwU16Para);
        }
        ulU32Para = _REG08(ulParaTableAddr + 192);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 191);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 190);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 189);
        if (ulU32Para != 0x000000FF)
        {
            printk("189 [ERR] AddressCycleReadRetry Options available: 0x%x [Spec] 0x000000FF)\r\n", ulU32Para);
        }
        uwU16Para = _REG08(ulParaTableAddr + 194);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 193);
        if ((uwU16Para & 0x3) == 0x0)
        {
            printk("193 SLC mode Op-Code: 0x0 [DAh/DFh](Feature Address 91h)\r\n");
        }
        else if ((uwU16Para & 0x3) == 0x1)
        {
            printk("193 SLC mode Op-Code: 0x1 [3Bh/3Ch](Feature Address 91h)\r\n");
        }
        else if ((uwU16Para & 0x3) == 0x2)
        {
            printk("193 SLC mode Op-Code: 0x2 [3Bh/3Ch]\r\n");
        }
        else //(uwU16Para & 0x3) == 0x3
        {
            printk("193 SLC mode Op-Code: 0x3 [3Bh]\r\n");
        }
        printk("193 SLC mode support (Factory Reserved Setting): %d\r\n", uwU16Para & 0x4);
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_B58R)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para;
    U32 ulU32Para;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;
    U16 uwBlkEndurance = 1;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_ONFI) //ONFI
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] ONFI\r\n", ubStr);
        }

        if ((_REG16(ulParaTableAddr + 4) != 0x1000) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("004 [ERR] Revision number: 0x%x [Spec] 0x0800\r\n", _REG16(ulParaTableAddr + 4));
        }

        if (_REG32(ulParaTableAddr + 4) != 0)
        {
            printk("006 MTL support:%d\r\n", _REG16(ulParaTableAddr + 6) & 0x0002);
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0008) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Prog/Erase:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0040) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Read:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x2000) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] NV-DDR3:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }

        if (((_REG16(ulParaTableAddr + 8) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 COPYBACK:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x0800) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] ODT:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x1000) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] GET/SET Feature by LUN:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }

        if ((_REG08(ulParaTableAddr + 10) != 0x0F) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("010 [ERR] ONFI-JEDEC JTG primary advanced command: 0x%x [Spec] 0x0F\r\n",
                   _REG08(ulParaTableAddr + 10));
        }

        for(i = 0; i < 12; i++) //Micron / Intel
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        if ((_REG32(ulParaTableAddr + 80) != 0x00004000) && (_REG32(ulParaTableAddr + 80) != 0))
        {
            printk("080 [ERR] Data Bytes pre Page: 0x%x [Spec] 0x00004000\r\n", _REG32(ulParaTableAddr + 80));
        }
        if ((_REG16(ulParaTableAddr + 84) != 0x07B0) && (_REG32(ulParaTableAddr + 84) != 0))
        {
            printk("084 [ERR] Spare Bytes pre Page: 0x%x [Spec] 0x07B0\r\n", _REG16(ulParaTableAddr + 84));
        }
        if ((_REG32(ulParaTableAddr + 92) != 0x00000AE0) && (_REG32(ulParaTableAddr + 92) != 0))
        {
            printk("092 [ERR] Pages pre Blk: 0x%x [Spec] 0x00000840\r\n", _REG32(ulParaTableAddr + 92));
        }
        if ((_REG32(ulParaTableAddr + 96) != 0x00000D4A) && (_REG32(ulParaTableAddr + 96) != 0))
        {
            printk("096 [ERR] Blks pre LUN: 0x%x [Spec] 0x000008B0\r\n", _REG32(ulParaTableAddr + 96));
        }
        printk("100 LUNs pre CE: 0x%x\r\n", _REG08(ulParaTableAddr + 100));

        uwU16Para = _REG08(ulParaTableAddr + 104);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 103);
        if (uwU16Para == 0x00D2)
        {
            printk("103 [Fortis] MaxBadBlkPreLun: 0x00D2\r\n");
        }
        else if (uwU16Para == 0x012C)
        {
            printk("103 [Media] MaxBadBlkPreLun: 0x012C\r\n");
        }
        else
        {
            printk("103 [ERR] MaxBadBlkPreLun: 0x%x\r\n", uwU16Para);
        }

        uwU16Para = _REG08(ulParaTableAddr + 106);
        for (i = 0; i < uwU16Para; i++)
        {
            uwBlkEndurance *= 10;
        }
        uwBlkEndurance *= _REG08(ulParaTableAddr + 105);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 105);
        if (uwU16Para == 0x0303)
        {
            printk("105 [Fortis] Blk endurance: 0x0303 ->%d\r\n", uwBlkEndurance);
        }
        else if (uwU16Para == 0x0301)
        {
            printk("105 [Media] Blk endurance: 0x0301 ->%d\r\n", uwBlkEndurance);
        }
        else
        {
            printk("105 [ERR] Blk endurance: 0x%x ->%d\r\n", uwU16Para, uwBlkEndurance);
        }

        if (_REG08(ulParaTableAddr + 158) != 0x1B)
        {
            printk("158 [ERR] NV_DDR2/3 features: 0x%x [Spec] 0x1B\r\n", _REG08(ulParaTableAddr + 158));
        }

        if (_REG16(ulParaTableAddr + 160) == 0xFFFF)
        {
            printk("160 [Fortis] NV_DDR3 timing mode support: 0xFFFF\r\n");
        }
        else if (_REG16(ulParaTableAddr + 160) == 0x1FFF)
        {
            printk("160 [Media] NV_DDR3 timing mode support: 0x1FFF\r\n");
        }
        else
        {
            printk("160 [ERR] NV_DDR3 timing mode support: 0x%x\r\n", _REG16(ulParaTableAddr + 160));
        }

        if (_REG08(ulParaTableAddr + 186) != 0x96)
        {
            printk("186 [ERR] AddressCycleReadRetry enablement Feature Address: 0x%x [Spec] 0x96\r\n",
                   _REG08(ulParaTableAddr + 186));
        }
        uwU16Para = _REG08(ulParaTableAddr + 188);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 187);
        if (uwU16Para != 0x0401)
        {
            printk("187 [ERR] AddressCycleReadRetry subfeature Position/BitPosition: 0x%x [Spec] 0x0401\r\n",
                   uwU16Para);
        }
        ulU32Para = _REG08(ulParaTableAddr + 192);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 191);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 190);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 189);
        if (ulU32Para != 0x000000FF)
        {
            printk("189 [ERR] AddressCycleReadRetry Options available: 0x%x [Spec] 0x000000FF)\r\n", ulU32Para);
        }
        uwU16Para = _REG08(ulParaTableAddr + 194);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 193);
        if ((uwU16Para & 0x3) == 0x0)
        {
            printk("193 SLC mode Op-Code: 0x0 [DAh/DFh](Feature Address 91h)\r\n");
        }
        else if ((uwU16Para & 0x3) == 0x1)
        {
            printk("193 SLC mode Op-Code: 0x1 [3Bh/3Ch](Feature Address 91h)\r\n");
        }
        else if ((uwU16Para & 0x3) == 0x2)
        {
            printk("193 SLC mode Op-Code: 0x2 [3Bh/3Ch]\r\n");
        }
        else //(uwU16Para & 0x3) == 0x3
        {
            printk("193 SLC mode Op-Code: 0x3 [3Bh]\r\n");
        }
        printk("193 SLC mode support (Factory Reserved Setting): %d\r\n", uwU16Para & 0x4);
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_N28A)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para;
    U32 ulU32Para;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;
    U16 uwBlkEndurance = 1;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_ONFI) //ONFI
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] ONFI\r\n", ubStr);
        }

        if ((_REG16(ulParaTableAddr + 4) != 0x0200) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("004 [ERR] Revision number: 0x%x [Spec] 0x0200\r\n", _REG16(ulParaTableAddr + 4));
        }

        if (_REG32(ulParaTableAddr + 4) != 0)
        {
            printk("006 MTL support:%d\r\n", _REG16(ulParaTableAddr + 6) & 0x0002);
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0008) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Prog/Erase:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0040) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Read:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x2000) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] NV-DDR3:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }

        if (((_REG16(ulParaTableAddr + 8) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 COPYBACK:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x0800) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] ODT:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x1000) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] GET/SET Feature by LUN:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }

        if ((_REG08(ulParaTableAddr + 10) != 0x0F) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("010 [ERR] ONFI-JEDEC JTG primary advanced command: 0x%x [Spec] 0x0F\r\n",
                   _REG08(ulParaTableAddr + 10));
        }

        for(i = 0; i < 12; i++) //Micron / Intel
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        if ((_REG32(ulParaTableAddr + 80) != 0x00004000) && (_REG32(ulParaTableAddr + 80) != 0))
        {
            printk("080 [ERR] Data Bytes pre Page: 0x%x [Spec] 0x00004000\r\n", _REG32(ulParaTableAddr + 80));
        }
        if ((_REG16(ulParaTableAddr + 84) != 0x08A0) && (_REG32(ulParaTableAddr + 84) != 0))
        {
            printk("084 [ERR] Spare Bytes pre Page: 0x%x [Spec] 0x08A0\r\n", _REG16(ulParaTableAddr + 84));
        }
        if ((_REG32(ulParaTableAddr + 92) != 0x00001200) && (_REG32(ulParaTableAddr + 92) != 0))
        {
            printk("092 [ERR] Pages pre Blk: 0x%x [Spec] 0x00001200\r\n", _REG32(ulParaTableAddr + 92));
        }
        if ((_REG32(ulParaTableAddr + 96) != 0x000007B0) && (_REG32(ulParaTableAddr + 96) != 0))
        {
            printk("096 [ERR] Blks pre LUN: 0x%x [Spec] 0x000007B0\r\n", _REG32(ulParaTableAddr + 96));
        }
        printk("100 LUNs pre CE: 0x%x\r\n", _REG08(ulParaTableAddr + 100));

        uwU16Para = _REG08(ulParaTableAddr + 104);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 103);
        if (uwU16Para == 0x0050)
        {
            //0x006C in old Spec(02/28/19), 0x0050 in new Spec(07/11/19)
            printk("103 [Media] MaxBadBlkPreLun: 0x0050\r\n");
        }
        else
        {
            printk("103 [ERR] MaxBadBlkPreLun: 0x%x\r\n", uwU16Para);
        }

        uwU16Para = _REG08(ulParaTableAddr + 106);
        for (i = 0; i < uwU16Para; i++)
        {
            uwBlkEndurance *= 10;
        }
        uwBlkEndurance *= _REG08(ulParaTableAddr + 105);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 105);
        if (uwU16Para == 0x020F)
        {
            printk("105 [Media] Blk endurance: 0x020F ->%d\r\n", uwBlkEndurance);
        }
        else
        {
            printk("105 [ERR] Blk endurance: 0x%x ->%d\r\n", uwU16Para, uwBlkEndurance);
        }

        if (_REG08(ulParaTableAddr + 158) != 0x1B)
        {
            printk("158 [ERR] NV_DDR2/3 features: 0x%x [Spec] 0x1B\r\n", _REG08(ulParaTableAddr + 158));
        }

        if (_REG16(ulParaTableAddr + 160) == 0x00FF)
        {
            printk("160 [Media] NV_DDR3 timing mode support: 0x00FF\r\n");
        }
        else
        {
            printk("160 [ERR] NV_DDR3 timing mode support: 0x%x\r\n", _REG16(ulParaTableAddr + 160));
        }

        if (_REG08(ulParaTableAddr + 180) != 0x14)
        {
            printk("180 [ERR] Read Retry Options: 0x%x [Spec] 0x14\r\n", _REG08(ulParaTableAddr + 180));
        }

        ulU32Para = _REG08(ulParaTableAddr + 184);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 183);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 182);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 181);
        if (ulU32Para != 0x000FFFFF)
        {
            printk("181 [ERR] Read Retry Options available: 0x%x [Spec] 0x000FFFFF\r\n", ulU32Para);
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_N38A)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para;
    //U32 ulU32Para;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;
    U16 uwBlkEndurance = 1;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing (without table in Spec)
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_ONFI) //ONFI
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] ONFI\r\n", ubStr);
        }

        if (_REG32(ulParaTableAddr + 4) != 0)
        {
            printk("006 MTL support:%d\r\n", _REG16(ulParaTableAddr + 6) & 0x0002);
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0008) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Prog/Erase:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0040) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Read:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x2000) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] NV-DDR3:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }

        if (((_REG16(ulParaTableAddr + 8) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 COPYBACK:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x0800) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] ODT:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x1000) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] GET/SET Feature by LUN:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }

        for(i = 0; i < 12; i++) //Micron / Intel
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        printk("100 LUNs pre CE: 0x%x\r\n", _REG08(ulParaTableAddr + 100));

        uwU16Para = _REG08(ulParaTableAddr + 104);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 103);
        printk("103 MaxBadBlkPreLun: 0x%x\r\n", uwU16Para);

        uwU16Para = _REG08(ulParaTableAddr + 106);
        for (i = 0; i < uwU16Para; i++)
        {
            uwBlkEndurance *= 10;
        }
        uwBlkEndurance *= _REG08(ulParaTableAddr + 105);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 105);
        printk("105 Blk endurance: 0x%x ->%d\r\n", uwU16Para, uwBlkEndurance);

        printk("160 NV_DDR3 timing mode support: 0x%x\r\n", _REG16(ulParaTableAddr + 160));

        if (_REG32(ulParaTableAddr + 192) != 0)
        {
            uwU16Para = _REG08(ulParaTableAddr + 194);
            uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 193);
            if ((uwU16Para & 0x3) == 0x0)
            {
                printk("193 SLC mode Op-Code: 0x0 [DAh/DFh](Feature Address 91h)\r\n");
            }
            else if ((uwU16Para & 0x3) == 0x1)
            {
                printk("193 SLC mode Op-Code: 0x1 [3Bh/3Ch](Feature Address 91h)\r\n");
            }
            else if ((uwU16Para & 0x3) == 0x2)
            {
                printk("193 SLC mode Op-Code: 0x2 [3Bh/3Ch]\r\n");
            }
            else //(uwU16Para & 0x3) == 0x3
            {
                printk("193 SLC mode Op-Code: 0x3 [3Bh]\r\n");
            }
            printk("193 SLC mode support (Factory Reserved Setting): %d\r\n", uwU16Para & 0x4);
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_N38B)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para;
    //U32 ulU32Para;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;
    U16 uwBlkEndurance = 1;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing (without table in Spec)
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_ONFI) //ONFI
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] ONFI\r\n", ubStr);
        }

        if (_REG32(ulParaTableAddr + 4) != 0)
        {
            printk("006 MTL support:%d\r\n", _REG16(ulParaTableAddr + 6) & 0x0002);
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0008) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Prog/Erase:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0040) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Read:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x2000) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] NV-DDR3:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }

        if (((_REG16(ulParaTableAddr + 8) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 COPYBACK:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x0800) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] ODT:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x1000) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] GET/SET Feature by LUN:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }

        for(i = 0; i < 12; i++) //Micron / Intel
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        printk("100 LUNs pre CE: 0x%x\r\n", _REG08(ulParaTableAddr + 100));

        uwU16Para = _REG08(ulParaTableAddr + 104);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 103);
        printk("103 MaxBadBlkPreLun: 0x%x\r\n", uwU16Para);

        uwU16Para = _REG08(ulParaTableAddr + 106);
        for (i = 0; i < uwU16Para; i++)
        {
            uwBlkEndurance *= 10;
        }
        uwBlkEndurance *= _REG08(ulParaTableAddr + 105);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 105);
        printk("105 Blk endurance: 0x%x ->%d\r\n", uwU16Para, uwBlkEndurance);

        printk("160 NV_DDR3 timing mode support: 0x%x\r\n", _REG16(ulParaTableAddr + 160));

        if (_REG32(ulParaTableAddr + 192) != 0)
        {
            uwU16Para = _REG08(ulParaTableAddr + 194);
            uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 193);
            if ((uwU16Para & 0x3) == 0x0)
            {
                printk("193 SLC mode Op-Code: 0x0 [DAh/DFh](Feature Address 91h)\r\n");
            }
            else if ((uwU16Para & 0x3) == 0x1)
            {
                printk("193 SLC mode Op-Code: 0x1 [3Bh/3Ch](Feature Address 91h)\r\n");
            }
            else if ((uwU16Para & 0x3) == 0x2)
            {
                printk("193 SLC mode Op-Code: 0x2 [3Bh/3Ch]\r\n");
            }
            else //(uwU16Para & 0x3) == 0x3
            {
                printk("193 SLC mode Op-Code: 0x3 [3Bh]\r\n");
            }
            printk("193 SLC mode support (Factory Reserved Setting): %d\r\n", uwU16Para & 0x4);
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}

#elif defined(FTL_Q5171A)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para;
    //U32 ulU32Para;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;
    U16 uwBlkEndurance = 1;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing (without table in Spec)
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_ONFI) //ONFI
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] ONFI\r\n", ubStr);
        }

        if (_REG32(ulParaTableAddr + 4) != 0)
        {
            printk("006 MTL support:%d\r\n", _REG16(ulParaTableAddr + 6) & 0x0002);
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0008) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Prog/Erase:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0040) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Read:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x2000) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] NV-DDR3:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }

        if (((_REG16(ulParaTableAddr + 8) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 COPYBACK:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x0800) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] ODT:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x1000) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] GET/SET Feature by LUN:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }

        for(i = 0; i < 12; i++) //Micron / Intel
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        printk("100 LUNs pre CE: 0x%x\r\n", _REG08(ulParaTableAddr + 100));

        uwU16Para = _REG08(ulParaTableAddr + 104);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 103);
        printk("103 MaxBadBlkPreLun: 0x%x\r\n", uwU16Para);

        uwU16Para = _REG08(ulParaTableAddr + 106);
        for (i = 0; i < uwU16Para; i++)
        {
            uwBlkEndurance *= 10;
        }
        uwBlkEndurance *= _REG08(ulParaTableAddr + 105);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 105);
        printk("105 Blk endurance: 0x%x ->%d\r\n", uwU16Para, uwBlkEndurance);

        printk("160 NV_DDR3 timing mode support: 0x%x\r\n", _REG16(ulParaTableAddr + 160));

        if (_REG32(ulParaTableAddr + 192) != 0)
        {
            uwU16Para = _REG08(ulParaTableAddr + 194);
            uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 193);
            if ((uwU16Para & 0x3) == 0x0)
            {
                printk("193 SLC mode Op-Code: 0x0 [DAh/DFh](Feature Address 91h)\r\n");
            }
            else if ((uwU16Para & 0x3) == 0x1)
            {
                printk("193 SLC mode Op-Code: 0x1 [3Bh/3Ch](Feature Address 91h)\r\n");
            }
            else if ((uwU16Para & 0x3) == 0x2)
            {
                printk("193 SLC mode Op-Code: 0x2 [3Bh/3Ch]\r\n");
            }
            else //(uwU16Para & 0x3) == 0x3
            {
                printk("193 SLC mode Op-Code: 0x3 [3Bh]\r\n");
            }
            printk("193 SLC mode support (Factory Reserved Setting): %d\r\n", uwU16Para & 0x4);
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}

#elif defined(FTL_N48R)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para;
    U32 ulU32Para;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;
    U16 uwBlkEndurance = 1;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_ONFI) //ONFI
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] ONFI\r\n", ubStr);
        }

        if ((_REG16(ulParaTableAddr + 4) != 0x0800) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("004 [ERR] Revision number: 0x%x [Spec] 0x0800\r\n", _REG16(ulParaTableAddr + 4));
        }

        if (_REG32(ulParaTableAddr + 4) != 0)
        {
            printk("006 MTL support:%d\r\n", _REG16(ulParaTableAddr + 6) & 0x0002);
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0008) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Prog/Erase:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0040) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Read:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x2000) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] NV-DDR3:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }

        if (((_REG16(ulParaTableAddr + 8) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 COPYBACK:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x0800) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] ODT:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x1000) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] GET/SET Feature by LUN:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }

        if ((_REG08(ulParaTableAddr + 10) != 0x0F) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("010 [ERR] ONFI-JEDEC JTG primary advanced command: 0x%x [Spec] 0x0F\r\n",
                   _REG08(ulParaTableAddr + 10));
        }

        for(i = 0; i < 12; i++) //Micron / Intel
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        if ((_REG32(ulParaTableAddr + 80) != 0x00004000) && (_REG32(ulParaTableAddr + 80) != 0))
        {
            printk("080 [ERR] Data Bytes pre Page: 0x%x [Spec] 0x00004000\r\n", _REG32(ulParaTableAddr + 80));
        }
        if ((_REG16(ulParaTableAddr + 84) != 0x07B0) && (_REG32(ulParaTableAddr + 84) != 0))
        {
            printk("084 [ERR] Spare Bytes pre Page: 0x%x [Spec] 0x07B0\r\n", _REG16(ulParaTableAddr + 84));
        }
        if ((_REG32(ulParaTableAddr + 92) != 0x00000B00) && (_REG32(ulParaTableAddr + 92) != 0))
        {
            printk("092 [ERR] Pages pre Blk: 0x%x [Spec] 0x00000B00\r\n", _REG32(ulParaTableAddr + 92));
        }
        if ((_REG32(ulParaTableAddr + 96) != 0x00000CC4) && (_REG32(ulParaTableAddr + 96) != 0))
        {
            printk("096 [ERR] Blks pre LUN: 0x%x [Spec] 0x00000CC4\r\n", _REG32(ulParaTableAddr + 96));
        }
        printk("100 LUNs pre CE: 0x%x\r\n", _REG08(ulParaTableAddr + 100));

        uwU16Para = _REG08(ulParaTableAddr + 104);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 103);
        if (uwU16Para == 0x0098)
        {
            printk("103 [Fortis] MaxBadBlkPreLun: 0x0098\r\n");
        }
        else if (uwU16Para == 0x00F8)
        {
            printk("103 [Media] MaxBadBlkPreLun: 0x00F8\r\n");
        }
        else
        {
            printk("103 [ERR] MaxBadBlkPreLun: 0x%x\r\n", uwU16Para);
        }

        uwU16Para = _REG08(ulParaTableAddr + 106);
        for (i = 0; i < uwU16Para; i++)
        {
            uwBlkEndurance *= 10;
        }
        uwBlkEndurance *= _REG08(ulParaTableAddr + 105);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 105);
        if (uwU16Para == 0x020F)
        {
            printk("105 [Fortis] Blk endurance: 0x020F ->%d\r\n", uwBlkEndurance);
        }
        else if (uwU16Para == 0x0209)
        {
            printk("105 [Media] Blk endurance: 0x0209 ->%d\r\n", uwBlkEndurance);
        }
        else
        {
            printk("105 [ERR] Blk endurance: 0x%x ->%d\r\n", uwU16Para, uwBlkEndurance);
        }

        if (_REG08(ulParaTableAddr + 158) != 0x1B)
        {
            printk("158 [ERR] NV_DDR2/3 features: 0x%x [Spec] 0x1B\r\n", _REG08(ulParaTableAddr + 158));
        }

        if (_REG16(ulParaTableAddr + 160) == 0x1FFF)
        {
            printk("160 [Fortis] NV_DDR3 timing mode support: 0x1FFF\r\n");
        }
        else if (_REG16(ulParaTableAddr + 160) == 0x03FF)
        {
            printk("160 [Media] NV_DDR3 timing mode support: 0x03FF\r\n");
        }
        else
        {
            printk("160 [ERR] NV_DDR3 timing mode support: 0x%x\r\n", _REG16(ulParaTableAddr + 160));
        }

        if (_REG08(ulParaTableAddr + 186) != 0x96)
        {
            printk("186 [ERR] AddressCycleReadRetry enablement Feature Address: 0x%x [Spec] 0x96\r\n",
                   _REG08(ulParaTableAddr + 186));
        }
        uwU16Para = _REG08(ulParaTableAddr + 188);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 187);
        if (uwU16Para != 0x0401)
        {
            printk("187 [ERR] AddressCycleReadRetry subfeature Position/BitPosition: 0x%x [Spec] 0x0401\r\n",
                   uwU16Para);
        }
        ulU32Para = _REG08(ulParaTableAddr + 192);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 191);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 190);
        ulU32Para = (ulU32Para << 8) | _REG08(ulParaTableAddr + 189);
        if (ulU32Para != 0x000000FF)
        {
            printk("189 [ERR] AddressCycleReadRetry Options available: 0x%x [Spec] 0x000000FF)\r\n", ulU32Para);
        }
        uwU16Para = _REG08(ulParaTableAddr + 194);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 193);
        if ((uwU16Para & 0x3) == 0x0)
        {
            printk("193 SLC mode Op-Code: 0x0 [DAh/DFh](Feature Address 91h)\r\n");
        }
        else if ((uwU16Para & 0x3) == 0x1)
        {
            printk("193 SLC mode Op-Code: 0x1 [3Bh/3Ch](Feature Address 91h)\r\n");
        }
        else if ((uwU16Para & 0x3) == 0x2)
        {
            printk("193 SLC mode Op-Code: 0x2 [3Bh/3Ch]\r\n");
        }
        else //(uwU16Para & 0x3) == 0x3
        {
            printk("193 SLC mode Op-Code: 0x3 [3Bh]\r\n");
        }
        printk("193 SLC mode support (Factory Reserved Setting): %d\r\n", uwU16Para & 0x4);
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_SSV2)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para, uwU16Para_2;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_JESD) //JESD
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] JESD\r\n", ubStr);
        }

        for(i = 0; i < 12; i++)
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        printk("92 PageNum: 0x%x BlkNum: 0x%x LunNum: 0x%x\r\n", _REG32(ulParaTableAddr + 92),
               _REG32(ulParaTableAddr + 96), _REG08(ulParaTableAddr + 100));

        if ((_REG08(ulParaTableAddr + 105) & 0x1) == 0)
        {
            printk("105 [ERR] program cache supported: 0\r\n");
        }
        if ((_REG08(ulParaTableAddr + 105) & 0x2) == 0)
        {
            printk("105 [ERR] read cache supported: 0\r\n");
        }

        if ((_REG16(ulParaTableAddr + 146) & 0x0100) != 0x0100) //266M
        {
            printk("146 [ERR] DDR speed grade: 0x%x\r\n", _REG16(ulParaTableAddr + 146));

        }

        printk("169 Driver strength support: 0x%x\r\n", _REG08(ulParaTableAddr + 196));
        printk("208 Guaranteed valid blocks at beginning of target: 0x%x\r\n",
               _REG08(ulParaTableAddr + 208));

        uwU16Para = _REG08(ulParaTableAddr + 210);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 209);
        printk("209 Block endurance for guaranteed valid blockst: 0x%x\r\n", uwU16Para);

        for (i = 0; i < 4; i++) //byte 211~242
        {
            uwU16Para = _REG08(ulParaTableAddr + 214 + (i << 3));
            uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 213 + (i << 3));
            uwU16Para_2 = _REG08(ulParaTableAddr + 216 + (i << 3));
            uwU16Para_2 = (uwU16Para << 8) | _REG08(ulParaTableAddr + 215 + (i << 3));
            printk("%d Blk%d ECC correctability bits:0x%x Codeword size:0x%x Max BadBlk:0x%x Blk endurance:0x%x\r\n",
                   211 + (i << 3), i, _REG08(ulParaTableAddr + 211 + (i << 3)),
                   _REG08(ulParaTableAddr + 212 + (i << 3)), uwU16Para, uwU16Para_2);
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_SSV4)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para, uwU16Para_2;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_JESD) //JESD
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] JESD\r\n", ubStr);
        }

        for(i = 0; i < 12; i++)
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        printk("92 PageNum: 0x%x BlkNum: 0x%x LunNum: 0x%x\r\n", _REG32(ulParaTableAddr + 92),
               _REG32(ulParaTableAddr + 96), _REG08(ulParaTableAddr + 100));

        if ((_REG08(ulParaTableAddr + 105) & 0x1) == 0)
        {
            printk("105 [ERR] program cache supported: 0\r\n");
        }
        if ((_REG08(ulParaTableAddr + 105) & 0x2) == 0)
        {
            printk("105 [ERR] read cache supported: 0\r\n");
        }

        if ((_REG16(ulParaTableAddr + 146) & 0x0100) != 0x0100) //266M
        {
            printk("146 [ERR] DDR speed grade: 0x%x\r\n", _REG16(ulParaTableAddr + 146));

        }

        printk("169 Driver strength support: 0x%x\r\n", _REG08(ulParaTableAddr + 196));
        printk("208 Guaranteed valid blocks at beginning of target: 0x%x\r\n",
               _REG08(ulParaTableAddr + 208));

        uwU16Para = _REG08(ulParaTableAddr + 210);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 209);
        printk("209 Block endurance for guaranteed valid blockst: 0x%x\r\n", uwU16Para);

        for (i = 0; i < 4; i++) //byte 211~242
        {
            uwU16Para = _REG08(ulParaTableAddr + 214 + (i << 3));
            uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 213 + (i << 3));
            uwU16Para_2 = _REG08(ulParaTableAddr + 216 + (i << 3));
            uwU16Para_2 = (uwU16Para << 8) | _REG08(ulParaTableAddr + 215 + (i << 3));
            printk("%d Blk%d ECC correctability bits:0x%x Codeword size:0x%x Max BadBlk:0x%x Blk endurance:0x%x\r\n",
                   211 + (i << 3), i, _REG08(ulParaTableAddr + 211 + (i << 3)),
                   _REG08(ulParaTableAddr + 212 + (i << 3)), uwU16Para, uwU16Para_2);
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_SSV5)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para, uwU16Para_2;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_JESD) //JESD
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] JESD\r\n", ubStr);
        }

        for(i = 0; i < 12; i++)
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        printk("92 PageNum: 0x%x BlkNum: 0x%x LunNum: 0x%x\r\n", _REG32(ulParaTableAddr + 92),
               _REG32(ulParaTableAddr + 96), _REG08(ulParaTableAddr + 100));

        if ((_REG08(ulParaTableAddr + 105) & 0x1) == 0)
        {
            printk("105 [ERR] program cache supported: 0\r\n");
        }
        if ((_REG08(ulParaTableAddr + 105) & 0x2) == 0)
        {
            printk("105 [ERR] read cache supported: 0\r\n");
        }

        if ((_REG16(ulParaTableAddr + 146) & 0x0400) != 0x0400) //400M
        {
            printk("146 [ERR] DDR speed grade: 0x%x\r\n", _REG16(ulParaTableAddr + 146));

        }

        printk("169 Driver strength support: 0x%x\r\n", _REG08(ulParaTableAddr + 196));
        printk("208 Guaranteed valid blocks at beginning of target: 0x%x\r\n",
               _REG08(ulParaTableAddr + 208));

        uwU16Para = _REG08(ulParaTableAddr + 210);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 209);
        printk("209 Block endurance for guaranteed valid blockst: 0x%x\r\n", uwU16Para);

        for (i = 0; i < 4; i++) //byte 211~242
        {
            uwU16Para = _REG08(ulParaTableAddr + 214 + (i << 3));
            uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 213 + (i << 3));
            uwU16Para_2 = _REG08(ulParaTableAddr + 216 + (i << 3));
            uwU16Para_2 = (uwU16Para << 8) | _REG08(ulParaTableAddr + 215 + (i << 3));
            printk("%d Blk%d ECC correctability bits:0x%x Codeword size:0x%x Max BadBlk:0x%x Blk endurance:0x%x\r\n",
                   211 + (i << 3), i, _REG08(ulParaTableAddr + 211 + (i << 3)),
                   _REG08(ulParaTableAddr + 212 + (i << 3)), uwU16Para, uwU16Para_2);
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_SSV6)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para, uwU16Para_2;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_JESD) //JESD
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] JESD\r\n", ubStr);
        }

        for(i = 0; i < 12; i++)
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        printk("92 PageNum: 0x%x BlkNum: 0x%x LunNum: 0x%x\r\n", _REG32(ulParaTableAddr + 92),
               _REG32(ulParaTableAddr + 96), _REG08(ulParaTableAddr + 100));

        if ((_REG08(ulParaTableAddr + 105) & 0x1) == 0)
        {
            printk("105 [ERR] program cache supported: 0\r\n");
        }
        if ((_REG08(ulParaTableAddr + 105) & 0x2) == 0)
        {
            printk("105 [ERR] read cache supported: 0\r\n");
        }

        if ((_REG16(ulParaTableAddr + 146) & 0x0800) != 0x0800) //600M in Spec
        {
            printk("146 [ERR] DDR speed grade: 0x%x\r\n", _REG16(ulParaTableAddr + 146));

        }

        printk("169 Driver strength support: 0x%x\r\n", _REG08(ulParaTableAddr + 196));
        printk("208 Guaranteed valid blocks at beginning of target: 0x%x\r\n",
               _REG08(ulParaTableAddr + 208));

        uwU16Para = _REG08(ulParaTableAddr + 210);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 209);
        printk("209 Block endurance for guaranteed valid blockst: 0x%x\r\n", uwU16Para);

        for (i = 0; i < 4; i++) //byte 211~242
        {
            uwU16Para = _REG08(ulParaTableAddr + 214 + (i << 3));
            uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 213 + (i << 3));
            uwU16Para_2 = _REG08(ulParaTableAddr + 216 + (i << 3));
            uwU16Para_2 = (uwU16Para << 8) | _REG08(ulParaTableAddr + 215 + (i << 3));
            printk("%d Blk%d ECC correctability bits:0x%x Codeword size:0x%x Max BadBlk:0x%x Blk endurance:0x%x\r\n",
                   211 + (i << 3), i, _REG08(ulParaTableAddr + 211 + (i << 3)),
                   _REG08(ulParaTableAddr + 212 + (i << 3)), uwU16Para, uwU16Para_2);
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}

#elif defined(FTL_SSV7)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para, uwU16Para_2;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_JESD) //JESD
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] JESD\r\n", ubStr);
        }

        for(i = 0; i < 12; i++)
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        printk("92 PageNum: 0x%x BlkNum: 0x%x LunNum: 0x%x\r\n", _REG32(ulParaTableAddr + 92),
               _REG32(ulParaTableAddr + 96), _REG08(ulParaTableAddr + 100));

        if ((_REG08(ulParaTableAddr + 105) & 0x1) == 0)
        {
            printk("105 [ERR] program cache supported: 0\r\n");
        }
        if ((_REG08(ulParaTableAddr + 105) & 0x2) == 0)
        {
            printk("105 [ERR] read cache supported: 0\r\n");
        }

        if ((_REG16(ulParaTableAddr + 146) & 0x1000) != 0x1000) //800M in Spec
        {
            printk("146 [ERR] DDR speed grade: 0x%x\r\n", _REG16(ulParaTableAddr + 146));

        }

        printk("169 Driver strength support: 0x%x\r\n", _REG08(ulParaTableAddr + 169));
        printk("208 Guaranteed valid blocks at beginning of target: 0x%x\r\n",
               _REG08(ulParaTableAddr + 208));

        uwU16Para = _REG08(ulParaTableAddr + 210);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 209);
        printk("209 Block endurance for guaranteed valid blockst: 0x%x\r\n", uwU16Para);

        for (i = 0; i < 4; i++) //byte 211~242
        {
            uwU16Para = _REG08(ulParaTableAddr + 214 + (i << 3));
            uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 213 + (i << 3));
            uwU16Para_2 = _REG08(ulParaTableAddr + 216 + (i << 3));
            uwU16Para_2 = (uwU16Para << 8) | _REG08(ulParaTableAddr + 215 + (i << 3));
            printk("%d Blk%d ECC correctability bits:0x%x Codeword size:0x%x Max BadBlk:0x%x Blk endurance:0x%x\r\n",
                   211 + (i << 3), i, _REG08(ulParaTableAddr + 211 + (i << 3)),
                   _REG08(ulParaTableAddr + 212 + (i << 3)), uwU16Para, uwU16Para_2);
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}

#elif defined(FTL_H3DTV5) || defined(FTL_H3DTV6) || defined(FTL_H3DTV7)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_JESD) //JESD
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] JESD\r\n", ubStr);
        }

        for(i = 0; i < 12; i++)
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        printk("92 PageNum: 0x%x BlkNum: 0x%x LunNum: 0x%x\r\n", _REG32(ulParaTableAddr + 92),
               _REG32(ulParaTableAddr + 96), _REG08(ulParaTableAddr + 100));

        if ((_REG08(ulParaTableAddr + 105) & 0x1) == 0)
        {
            printk("105 [ERR] program cache supported: 0\r\n");
        }
        if ((_REG08(ulParaTableAddr + 105) & 0x2) == 0)
        {
            printk("105 [ERR] read cache supported: 0\r\n");
        }

        if ((_REG16(ulParaTableAddr + 146) & 0x0FFF) != 0x0FFF) //533M
        {
            printk("146 [ERR] DDR speed grade: 0x%x\r\n", _REG16(ulParaTableAddr + 146));

        }

        printk("169 Driver strength support: 0x%x\r\n", _REG08(ulParaTableAddr + 196));
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_H3DQV5)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_JESD) //JESD
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] JESD\r\n", ubStr);
        }

        for(i = 0; i < 12; i++)
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        printk("92 PageNum: 0x%x BlkNum: 0x%x LunNum: 0x%x\r\n", _REG32(ulParaTableAddr + 92),
               _REG32(ulParaTableAddr + 96), _REG08(ulParaTableAddr + 100));

        if ((_REG08(ulParaTableAddr + 105) & 0x1) == 0)
        {
            printk("105 [ERR] program cache supported: 0\r\n");
        }
        if ((_REG08(ulParaTableAddr + 105) & 0x2) == 0)
        {
            printk("105 [ERR] read cache supported: 0\r\n");
        }

        if ((_REG16(ulParaTableAddr + 146) & 0x07FF) != 0x07FF) //400M
        {
            printk("146 [ERR] DDR speed grade: 0x%x\r\n", _REG16(ulParaTableAddr + 146));

        }

        printk("169 Driver strength support: 0x%x\r\n", _REG08(ulParaTableAddr + 196));
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_YG2T)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;
    U16 uwBlkEndurance = 1;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_ONFI) //ONFI
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] ONFI\r\n", ubStr);
        }

        if ((_REG16(ulParaTableAddr + 4) != 0x03FE) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("004 [ERR] Revision number: 0x%x [Spec] 0x03FE\r\n", _REG16(ulParaTableAddr + 4));
        }

        if (_REG32(ulParaTableAddr + 4) != 0)
        {
            printk("006 MTL support:%d\r\n", _REG16(ulParaTableAddr + 6) & 0x0002);
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0008) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Prog/Erase:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0040) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Read:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x2000) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] NV-DDR3:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }

        if (((_REG16(ulParaTableAddr + 8) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 COPYBACK:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x0800) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] ODT:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x1000) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] GET/SET Feature by LUN:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }

        if ((_REG08(ulParaTableAddr + 10) != 0x0F) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("010 [ERR] ONFI-JEDEC JTG primary advanced command: 0x%x [Spec] 0x0F\r\n",
                   _REG08(ulParaTableAddr + 10));
        }

        for(i = 0; i < 12; i++) //YMTC
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        if ((_REG32(ulParaTableAddr + 80) != 0x00004000) && (_REG32(ulParaTableAddr + 80) != 0))
        {
            printk("080 [ERR] Data Bytes pre Page: 0x%x [Spec] 0x00004000\r\n", _REG32(ulParaTableAddr + 80));
        }
        if ((_REG16(ulParaTableAddr + 84) != 0x0800) && (_REG32(ulParaTableAddr + 84) != 0))
        {
            printk("084 [ERR] Spare Bytes pre Page: 0x%x [Spec] 0x0800\r\n", _REG16(ulParaTableAddr + 84));
        }
        if ((_REG32(ulParaTableAddr + 92) != 0x00000480) && (_REG32(ulParaTableAddr + 92) != 0))
        {
            printk("092 [ERR] Pages pre Blk: 0x%x [Spec] 0x00000480\r\n", _REG32(ulParaTableAddr + 92));
        }
        if ((_REG32(ulParaTableAddr + 96) != 0x000007DC) && (_REG32(ulParaTableAddr + 96) != 0))
        {
            printk("096 [ERR] Blks pre LUN: 0x%x [Spec] 0x000007DC\r\n", _REG32(ulParaTableAddr + 96));
        }
        printk("100 LUNs pre CE: 0x%x\r\n", _REG08(ulParaTableAddr + 100));

        uwU16Para = _REG08(ulParaTableAddr + 104);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 103);
        if (uwU16Para != 0)
        {
            if (uwU16Para == 0x0080)
            {
                printk("103 [CS0][CS2] MaxBadBlkPreLun: 0x0080\r\n");
            }
            else if (uwU16Para == 0x0050)
            {
                printk("103 [E-lite] MaxBadBlkPreLun: 0x0050\r\n");
            }
            else
            {
                printk("103 [ERR] MaxBadBlkPreLun: 0x%x\r\n", uwU16Para);
            }
        }

        uwU16Para = _REG08(ulParaTableAddr + 106);
        for (i = 0; i < uwU16Para; i++)
        {
            uwBlkEndurance *= 10;
        }
        uwBlkEndurance *= _REG08(ulParaTableAddr + 105);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 105);
        if (uwU16Para != 0)
        {
            if (uwU16Para == 0x0303)
            {
                printk("105 [CS2] Blk endurance: 0x0303 ->%d\r\n", uwBlkEndurance);
            }
            else if (uwU16Para == 0x020F)
            {
                printk("105 [CS0] Blk endurance: 0x020F ->%d\r\n", uwBlkEndurance);
            }
            else if (uwU16Para == 0x0503)
            {
                printk("105 [E-lite] Blk endurance: 0x0503 ->%d\r\n", uwBlkEndurance);
            }
            else
            {
                printk("105 [ERR] Blk endurance: 0x%x ->%d\r\n", uwU16Para, uwBlkEndurance);
            }
        }

        if (_REG08(ulParaTableAddr + 158) == 0x3B)
        {
            printk("158 [CS2][E-lite] NV_DDR3 features: 0x3B\r\n");
        }
        else if (_REG08(ulParaTableAddr + 158) == 0x39)
        {
            printk("158 [CS0] NV_DDR3 features: 0x39\r\n");
        }
        else
        {
            printk("158 [ERR] NV_DDR3 features: 0x%x\r\n", _REG08(ulParaTableAddr + 158));
        }

        if (_REG16(ulParaTableAddr + 160) == 0x00FF)
        {
            printk("160 [CS2][E-lite] NV_DDR3 timing mode support: 0x00FF\r\n");
        }
        else if (_REG16(ulParaTableAddr + 160) == 0x0000)
        {
            printk("160 [CS0] NV_DDR3 timing mode support: 0x0000\r\n");
        }
        else
        {
            printk("160 [ERR] NV_DDR3 timing mode support: 0x%x [Spec] 0x00FF\r\n",
                   _REG16(ulParaTableAddr + 160));
        }

        if (_REG16(ulParaTableAddr + 166) == 0x06FF)
        {
            printk("166 [CS2][E-lite] Vendor specific command support: 0x06FF\r\n");
        }
        else if (_REG16(ulParaTableAddr + 166) == 0x069F)
        {
            printk("166 [CS2] Vendor specific command support: 0x069F\r\n");
        }
        else
        {
            printk("166 [ERR] Vendor specific command support: 0x%x\r\n", _REG16(ulParaTableAddr + 166));
        }

        if (_REG16(ulParaTableAddr + 168) == 0xDECF)
        {
            printk("168 [CS2] Vendor specific feature support: 0xDECF\r\n");
        }
        else if (_REG16(ulParaTableAddr + 168) == 0x5ECF)
        {
            printk("168 [CS0] Vendor specific feature support: 0x5ECF\r\n");
        }
        else if (_REG16(ulParaTableAddr + 168) == 0xFECF)
        {
            printk("168 [E-lite] Vendor specific feature support: 0xFECF\r\n");
        }
        else
        {
            printk("168 [ERR] Vendor specific feature support: 0x%x\r\n", _REG16(ulParaTableAddr + 168));
        }

        if (_REG08(ulParaTableAddr + 170) != 0x07)
        {
            printk("170 [ERR] Additional driver strength: 0x%x [Spec] 0x07\r\n", _REG08(ulParaTableAddr + 170));
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_YX2T)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;
    U16 uwBlkEndurance = 1;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_ONFI) //ONFI
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] ONFI\r\n", ubStr);
        }

        if ((_REG16(ulParaTableAddr + 4) != 0x07FE) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("004 [ERR] Revision number: 0x%x [Spec] 0x07FE\r\n", _REG16(ulParaTableAddr + 4));
        }

        if (_REG32(ulParaTableAddr + 4) != 0)
        {
            printk("006 MTL support:%d\r\n", _REG16(ulParaTableAddr + 6) & 0x0002);
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0008) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Prog/Erase:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0040) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Read:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x2000) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] NV-DDR3:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }

        if (((_REG16(ulParaTableAddr + 8) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 COPYBACK:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x0800) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] ODT:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x1000) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] GET/SET Feature by LUN:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }

        if ((_REG08(ulParaTableAddr + 10) != 0x0F) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("010 [ERR] ONFI-JEDEC JTG primary advanced command: 0x%x [Spec] 0x0F\r\n",
                   _REG08(ulParaTableAddr + 10));
        }

        for(i = 0; i < 12; i++) //YMTC
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        if ((_REG32(ulParaTableAddr + 80) != 0x00004000) && (_REG32(ulParaTableAddr + 80) != 0))
        {
            printk("080 [ERR] Data Bytes pre Page: 0x%x [Spec] 0x00004000\r\n", _REG32(ulParaTableAddr + 80));
        }
        if ((_REG16(ulParaTableAddr + 84) != 0x0800) && (_REG32(ulParaTableAddr + 84) != 0))
        {
            printk("084 [ERR] Spare Bytes pre Page: 0x%x [Spec] 0x0800\r\n", _REG16(ulParaTableAddr + 84));
        }
        if ((_REG32(ulParaTableAddr + 92) != 0x00000900) && (_REG32(ulParaTableAddr + 92) != 0))
        {
            printk("092 [ERR] Pages pre Blk: 0x%x [Spec] 0x00000900\r\n", _REG32(ulParaTableAddr + 92));
        }
        if ((_REG32(ulParaTableAddr + 96) != 0x000007BC) && (_REG32(ulParaTableAddr + 96) != 0))
        {
            printk("096 [ERR] Blks pre LUN: 0x%x [Spec] 0x000007BC\r\n", _REG32(ulParaTableAddr + 96));
        }
        printk("100 LUNs pre CE: 0x%x\r\n", _REG08(ulParaTableAddr + 100));

        uwU16Para = _REG08(ulParaTableAddr + 104);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 103);
        if (uwU16Para != 0)
        {
            if (uwU16Para == 0x0068)
            {
                printk("103 MaxBadBlkPreLun: 0x0068\r\n");
            }
            else
            {
                printk("103 [ERR] MaxBadBlkPreLun: 0x%x [Spec] 0x0068\r\n", uwU16Para);
            }
        }

        uwU16Para = _REG08(ulParaTableAddr + 106);
        for (i = 0; i < uwU16Para; i++)
        {
            uwBlkEndurance *= 10;
        }
        uwBlkEndurance *= _REG08(ulParaTableAddr + 105);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 105);
        if (uwU16Para != 0)
        {
            if (uwU16Para == 0x0303)
            {
                printk("105 Blk endurance: 0x0303 ->%d\r\n", uwBlkEndurance);
            }
            else
            {
                printk("105 [ERR] Blk endurance: 0x%x ->%d [Spec] 0x0303 ->3000\r\n", uwU16Para, uwBlkEndurance);
            }
        }

        if (_REG08(ulParaTableAddr + 158) != 0x3B)
        {
            printk("158 [ERR] NV_DDR3 features: 0x%x [Spec] 0x3B\r\n", _REG08(ulParaTableAddr + 158));
        }

        if (_REG16(ulParaTableAddr + 160) == 0x1FFF)
        {
            printk("160 NV_DDR3 timing mode support: 0x1FFF\r\n");
        }
        else
        {
            printk("160 [ERR] NV_DDR3 timing mode support: 0x%x [Spec] 0x1FFF\r\n",
                   _REG16(ulParaTableAddr + 160));
        }

        if (_REG16(ulParaTableAddr + 166) != 0x37FF)
        {
            printk("166 [ERR] Vendor specific command support: 0x%x [Spec] 0x37FF\r\n",
                   _REG16(ulParaTableAddr + 166));
        }

        if (_REG16(ulParaTableAddr + 168) != 0x7ACD)
        {
            printk("168 [ERR] Vendor specific feature support: 0x%x [Spec] 0x7ACD\r\n",
                   _REG16(ulParaTableAddr + 168));
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_YX3T) || defined(FTL_YX3T_WDS)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;
    U16 uwBlkEndurance = 1;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_ONFI) //ONFI
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] ONFI\r\n", ubStr);
        }

        if ((_REG16(ulParaTableAddr + 4) != 0x1FFE) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("004 [ERR] Revision number: 0x%x [Spec] 0x1FFE\r\n", _REG16(ulParaTableAddr + 4));
        }

        if (_REG32(ulParaTableAddr + 4) != 0)
        {
            printk("006 MTL support:%d\r\n", _REG16(ulParaTableAddr + 6) & 0x0002);
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0008) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Prog/Erase:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0040) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Read:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x2000) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] NV-DDR3:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }

        if (((_REG16(ulParaTableAddr + 8) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 COPYBACK:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x0800) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] ODT:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x1000) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] GET/SET Feature by LUN:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }

        if ((_REG08(ulParaTableAddr + 10) != 0x0F) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("010 [ERR] ONFI-JEDEC JTG primary advanced command: 0x%x [Spec] 0x0F\r\n",
                   _REG08(ulParaTableAddr + 10));
        }

        for(i = 0; i < 12; i++) //YMTC
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        if ((_REG32(ulParaTableAddr + 80) != 0x00004000) && (_REG32(ulParaTableAddr + 80) != 0))
        {
            printk("080 [ERR] Data Bytes pre Page: 0x%x [Spec] 0x00004000\r\n", _REG32(ulParaTableAddr + 80));
        }
        if ((_REG16(ulParaTableAddr + 84) != 0x0800) && (_REG32(ulParaTableAddr + 84) != 0))
        {
            printk("084 [ERR] Spare Bytes pre Page: 0x%x [Spec] 0x0800\r\n", _REG16(ulParaTableAddr + 84));
        }
        if ((_REG32(ulParaTableAddr + 92) != 0x00000900) && (_REG32(ulParaTableAddr + 92) != 0))
        {
            printk("092 [ERR] Pages pre Blk: 0x%x [Spec] 0x00000900\r\n", _REG32(ulParaTableAddr + 92));
        }
        if ((_REG32(ulParaTableAddr + 96) != 0x000007CC) && (_REG32(ulParaTableAddr + 96) != 0))
        {
            printk("096 [ERR] Blks pre LUN: 0x%x [Spec] 0x000007BC\r\n", _REG32(ulParaTableAddr + 96));
        }
        printk("100 LUNs pre CE: 0x%x\r\n", _REG08(ulParaTableAddr + 100));

        uwU16Para = _REG08(ulParaTableAddr + 104);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 103);
        if (uwU16Para != 0)
        {
            if (uwU16Para == 0x0078)
            {
                printk("103 MaxBadBlkPreLun: 0x0078\r\n");
            }
            else
            {
                printk("103 [ERR] MaxBadBlkPreLun: 0x%x [Spec] 0x0078\r\n", uwU16Para);
            }
        }

        uwU16Para = _REG08(ulParaTableAddr + 106);
        for (i = 0; i < uwU16Para; i++)
        {
            uwBlkEndurance *= 10;
        }
        uwBlkEndurance *= _REG08(ulParaTableAddr + 105);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 105);
        if (uwU16Para != 0)
        {
            if (uwU16Para == 0x0303)
            {
                printk("105 Blk endurance: 0x0303 ->%d\r\n", uwBlkEndurance);
            }
            else
            {
                printk("105 [ERR] Blk endurance: 0x%x ->%d [Spec] 0x0303 ->3000\r\n", uwU16Para, uwBlkEndurance);
            }
        }

        if (_REG08(ulParaTableAddr + 158) != 0x3B)
        {
            printk("158 [ERR] NV_DDR3 features: 0x%x [Spec] 0x3B\r\n", _REG08(ulParaTableAddr + 158));
        }

        if (_REG16(ulParaTableAddr + 160) == 0x7FFF)
        {
            printk("160 NV_DDR3 timing mode support: 0x7FFF\r\n");
        }
        else
        {
            printk("160 [ERR] NV_DDR3 timing mode support: 0x%x [Spec] 0x7FFF\r\n",
                   _REG16(ulParaTableAddr + 160));
        }

        if (_REG16(ulParaTableAddr + 166) != 0x7FFF)
        {
            printk("166 [ERR] Vendor specific command support: 0x%x [Spec] 0x7FFF\r\n",
                   _REG16(ulParaTableAddr + 166));
        }

        if (_REG16(ulParaTableAddr + 168) != 0x7ECF)
        {
            printk("168 [ERR] Vendor specific feature support: 0x%x [Spec] 0x7ECF\r\n",
                   _REG16(ulParaTableAddr + 168));
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_YX3T_WDS)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;
    U16 uwBlkEndurance = 1;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_ONFI) //ONFI
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] ONFI\r\n", ubStr);
        }

        if ((_REG16(ulParaTableAddr + 4) != 0x1FFE) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("004 [ERR] Revision number: 0x%x [Spec] 0x1FFE\r\n", _REG16(ulParaTableAddr + 4));
        }

        if (_REG32(ulParaTableAddr + 4) != 0)
        {
            printk("006 MTL support:%d\r\n", _REG16(ulParaTableAddr + 6) & 0x0002);
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0008) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Prog/Erase:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0040) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Read:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x2000) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] NV-DDR3:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }

        if (((_REG16(ulParaTableAddr + 8) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 COPYBACK:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x0800) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] ODT:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x1000) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] GET/SET Feature by LUN:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }

        if ((_REG08(ulParaTableAddr + 10) != 0x0F) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("010 [ERR] ONFI-JEDEC JTG primary advanced command: 0x%x [Spec] 0x0F\r\n",
                   _REG08(ulParaTableAddr + 10));
        }

        for(i = 0; i < 12; i++) //YMTC
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        if ((_REG32(ulParaTableAddr + 80) != 0x00004000) && (_REG32(ulParaTableAddr + 80) != 0))
        {
            printk("080 [ERR] Data Bytes pre Page: 0x%x [Spec] 0x00004000\r\n", _REG32(ulParaTableAddr + 80));
        }
        if ((_REG16(ulParaTableAddr + 84) != 0x0800) && (_REG32(ulParaTableAddr + 84) != 0))
        {
            printk("084 [ERR] Spare Bytes pre Page: 0x%x [Spec] 0x0800\r\n", _REG16(ulParaTableAddr + 84));
        }
        if ((_REG32(ulParaTableAddr + 92) != 0x00001050) && (_REG32(ulParaTableAddr + 92) != 0))
        {
            printk("092 [ERR] Pages pre Blk: 0x%x [Spec] 0x00001050\r\n", _REG32(ulParaTableAddr + 92));
        }
        if ((_REG32(ulParaTableAddr + 96) != 0x00000882) && (_REG32(ulParaTableAddr + 96) != 0))
        {
            printk("096 [ERR] Blks pre LUN: 0x%x [Spec] 0x00000882\r\n", _REG32(ulParaTableAddr + 96));
        }
        printk("100 LUNs pre CE: 0x%x\r\n", _REG08(ulParaTableAddr + 100));

        uwU16Para = _REG08(ulParaTableAddr + 104);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 103);
        if (uwU16Para != 0)
        {
            if (uwU16Para == 0x0078)
            {
                printk("103 MaxBadBlkPreLun: 0x0078\r\n");
            }
            else
            {
                printk("103 [ERR] MaxBadBlkPreLun: 0x%x [Spec] 0x0078\r\n", uwU16Para);
            }
        }

        uwU16Para = _REG08(ulParaTableAddr + 106);
        for (i = 0; i < uwU16Para; i++)
        {
            uwBlkEndurance *= 10;
        }
        uwBlkEndurance *= _REG08(ulParaTableAddr + 105);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 105);
        if (uwU16Para != 0)
        {
            if (uwU16Para == 0x0305)
            {
                printk("105 Blk endurance: 0x0305 ->%d\r\n", uwBlkEndurance);
            }
            else
            {
                printk("105 [ERR] Blk endurance: 0x%x ->%d [Spec] 0x0305 ->5000\r\n", uwU16Para, uwBlkEndurance);
            }
        }

        if (_REG08(ulParaTableAddr + 158) != 0x3B)
        {
            printk("158 [ERR] NV_DDR3 features: 0x%x [Spec] 0x3B\r\n", _REG08(ulParaTableAddr + 158));
        }

        if (_REG16(ulParaTableAddr + 160) == 0xFFFF)
        {
            printk("160 NV_DDR3 timing mode support: 0x7FFF\r\n");
        }
        else
        {
            printk("160 [ERR] NV_DDR3 timing mode support: 0x%x [Spec] 0xFFFF\r\n",
                   _REG16(ulParaTableAddr + 160));
        }

        if (_REG16(ulParaTableAddr + 166) != 0x7FFF)
        {
            printk("166 [ERR] Vendor specific command support: 0x%x [Spec] 0x7FFF\r\n",
                   _REG16(ulParaTableAddr + 166));
        }

        if (_REG16(ulParaTableAddr + 168) != 0x7ECF)
        {
            printk("168 [ERR] Vendor specific feature support: 0x%x [Spec] 0x7ECF\r\n",
                   _REG16(ulParaTableAddr + 168));
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_YX2Q)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;
    U16 uwBlkEndurance = 1;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_ONFI) //ONFI
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] ONFI\r\n", ubStr);
        }

        if ((_REG16(ulParaTableAddr + 4) != 0x07FE) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("004 [ERR] Revision number: 0x%x [Spec] 0x07FE\r\n", _REG16(ulParaTableAddr + 4));
        }

        if (_REG32(ulParaTableAddr + 4) != 0)
        {
            printk("006 MTL support:%d\r\n", _REG16(ulParaTableAddr + 6) & 0x0002);
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0008) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Prog/Erase:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0040) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Read:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x2000) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] NV-DDR3:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }

        if (((_REG16(ulParaTableAddr + 8) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 COPYBACK:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x0800) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] ODT:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x1000) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] GET/SET Feature by LUN:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }

        if ((_REG08(ulParaTableAddr + 10) != 0x0F) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("010 [ERR] ONFI-JEDEC JTG primary advanced command: 0x%x [Spec] 0x0F\r\n",
                   _REG08(ulParaTableAddr + 10));
        }

        for(i = 0; i < 12; i++) //YMTC
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        if ((_REG32(ulParaTableAddr + 80) != 0x00004000) && (_REG32(ulParaTableAddr + 80) != 0))
        {
            printk("080 [ERR] Data Bytes pre Page: 0x%x [Spec] 0x00004000\r\n", _REG32(ulParaTableAddr + 80));
        }
        if ((_REG16(ulParaTableAddr + 84) != 0x0800) && (_REG32(ulParaTableAddr + 84) != 0))
        {
            printk("084 [ERR] Spare Bytes pre Page: 0x%x [Spec] 0x0800\r\n", _REG16(ulParaTableAddr + 84));
        }
        if ((_REG32(ulParaTableAddr + 92) != 0x00000BE8) && (_REG32(ulParaTableAddr + 92) != 0))
        {
            printk("092 [ERR] Pages pre Blk: 0x%x [Spec] 0x00000BE8\r\n", _REG32(ulParaTableAddr + 92));
        }
        if ((_REG32(ulParaTableAddr + 96) != 0x00000F72) && (_REG32(ulParaTableAddr + 96) != 0))
        {
            printk("096 [ERR] Blks pre LUN: 0x%x [Spec] 0x00000F72\r\n", _REG32(ulParaTableAddr + 96));
        }
        printk("100 LUNs pre CE: 0x%x\r\n", _REG08(ulParaTableAddr + 100));

        uwU16Para = _REG08(ulParaTableAddr + 104);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 103);
        if (uwU16Para != 0)
        {
            printk("103 MaxBadBlkPreLun: 0x%x\r\n", uwU16Para);
        }

        uwU16Para = _REG08(ulParaTableAddr + 106);
        for (i = 0; i < uwU16Para; i++)
        {
            uwBlkEndurance *= 10;
        }
        uwBlkEndurance *= _REG08(ulParaTableAddr + 105);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 105);
        if (uwU16Para != 0)
        {
            if (uwU16Para == 0x0301)
            {
                printk("105 Blk endurance: 0x0301 ->%d\r\n", uwBlkEndurance);
            }
            else
            {
                printk("105 [ERR] Blk endurance: 0x%x ->%d [Spec] 0x0301 ->1000\r\n", uwU16Para, uwBlkEndurance);
            }
        }

        if (_REG08(ulParaTableAddr + 158) != 0x3B)
        {
            printk("158 [ERR] NV_DDR3 features: 0x%x [Spec] 0x3B\r\n", _REG08(ulParaTableAddr + 158));
        }

        if (_REG16(ulParaTableAddr + 160) == 0x1FFF)
        {
            printk("160 NV_DDR3 timing mode support: 0x1FFF\r\n");
        }
        else
        {
            printk("160 [ERR] NV_DDR3 timing mode support: 0x%x [Spec] 0x1FFF\r\n",
                   _REG16(ulParaTableAddr + 160));
        }

        if (_REG16(ulParaTableAddr + 166) != 0x3DFF)
        {
            printk("166 [ERR] Vendor specific command support: 0x%x [Spec] 0x3DFF\r\n",
                   _REG16(ulParaTableAddr + 166));
        }

        if (_REG16(ulParaTableAddr + 168) != 0x7ACD)
        {
            printk("168 [ERR] Vendor specific feature support: 0x%x [Spec] 0x7ACD\r\n",
                   _REG16(ulParaTableAddr + 168));
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_TSB_BICS3)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para, uwU16Para_2;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_JESD) //JESD
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] JESD\r\n", ubStr);
        }

        if ((_REG16(ulParaTableAddr + 4) != 0x0004) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("004 [ERR] Revision number: 0x%x [Spec] 0x0004\r\n", _REG16(ulParaTableAddr + 4));
        }

        if (_REG32(ulParaTableAddr + 4) != 0)
        {
            printk("006 MTL support:%d\r\n", _REG16(ulParaTableAddr + 6) & 0x0002);
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0008) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Prog/Erase:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Read:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0040) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] Toggle Mode DDR3:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }

        if (((_REG16(ulParaTableAddr + 8) & 0x0004) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] GET/SET Feature:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 COPYBACK:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }

        uwU16Para = _REG08(ulParaTableAddr + 12);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 11);
        if ((uwU16Para != 0x0081) && ((_REG32(ulParaTableAddr + 8) != 0)
                                      || (_REG32(ulParaTableAddr + 12) != 0)))
        {
            printk("011 [ERR] Secondary commands supported: 0x%x [Spec] 0x0081\r\n", uwU16Para);
        }

        for(i = 0; i < 12; i++) //KIOXIA
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        if ((_REG32(ulParaTableAddr + 80) != 0x00004000) && (_REG32(ulParaTableAddr + 80) != 0))
        {
            printk("080 [ERR] Data Bytes pre Page: 0x%x [Spec] 0x00004000\r\n", _REG32(ulParaTableAddr + 80));
        }
        if ((_REG16(ulParaTableAddr + 84) != 0x07A0) && (_REG32(ulParaTableAddr + 84) != 0))
        {
            printk("084 [ERR] Spare Bytes pre Page: 0x%x [Spec] 0x07A0\r\n", _REG16(ulParaTableAddr + 84));
        }
        if ((_REG32(ulParaTableAddr + 92) != 0x00000300) && (_REG32(ulParaTableAddr + 92) != 0))
        {
            printk("092 [ERR] Pages pre Blk: 0x%x [Spec] 0x00000300\r\n", _REG32(ulParaTableAddr + 92));
        }

        if (_REG32(ulParaTableAddr + 96) == 0x00000B8C)
        {
            printk("096 [32G_Die] Blks pre LUN: 0x00000B8C\r\n");
        }
        else if (_REG32(ulParaTableAddr + 96) == 0x0000171C)
        {
            printk("096 [64G_Die] Blks pre LUN: 0x0000171C\r\n");
        }
        else
        {
            printk("096 [ERR] Blks pre LUN: 0x%x\r\n", _REG32(ulParaTableAddr + 96));
        }
        printk("100 LUNs pre CE: 0x%x\r\n", _REG08(ulParaTableAddr + 100));

        if (_REG08(ulParaTableAddr + 105) != 0x07)
        {
            printk("105 [ERR] Multi-plane operation attributes: 0x%x [Spec] 0x07\r\n",
                   _REG08(ulParaTableAddr + 105));
        }

        if (_REG08(ulParaTableAddr + 169) != 0x03)
        {
            printk("169 [ERR] Driver strength support: 0x%x [Spec] 0x03\r\n", _REG08(ulParaTableAddr + 169));
        }

        printk("208 Guaranteed valid blocks of target: 0x%x\r\n", _REG08(ulParaTableAddr + 208));

        uwU16Para = _REG08(ulParaTableAddr + 210);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 209);
        printk("209 Block endurance for guaranteed valid blocks: 0x%x\r\n", uwU16Para);

        for (i = 0; i < 4; i++) //byte 211~242
        {
            uwU16Para = _REG08(ulParaTableAddr + 214 + (i << 3));
            uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 213 + (i << 3));
            uwU16Para_2 = _REG08(ulParaTableAddr + 216 + (i << 3));
            uwU16Para_2 = (uwU16Para << 8) | _REG08(ulParaTableAddr + 215 + (i << 3));
            printk("%d Blk%d ECC correctability bits:0x%x Codeword size:0x%x Max BadBlk:0x%x Blk endurance:0x%x\r\n",
                   211 + (i << 3), i, _REG08(ulParaTableAddr + 211 + (i << 3)),
                   _REG08(ulParaTableAddr + 212 + (i << 3)), uwU16Para, uwU16Para_2);
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_TSB_BICS4)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para, uwU16Para_2;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_JESD) //JESD
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] JESD\r\n", ubStr);
        }

        if ((_REG16(ulParaTableAddr + 4) != 0x0004) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("004 [ERR] Revision number: 0x%x [Spec] 0x0004\r\n", _REG16(ulParaTableAddr + 4));
        }

        if (_REG32(ulParaTableAddr + 4) != 0)
        {
            printk("006 MTL support:%d\r\n", _REG16(ulParaTableAddr + 6) & 0x0002);
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0008) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Prog/Erase:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Read:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0040) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] Toggle Mode DDR3:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }

        if (((_REG16(ulParaTableAddr + 8) & 0x0004) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] GET/SET Feature:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 COPYBACK:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }

        uwU16Para = _REG08(ulParaTableAddr + 12);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 11);
        if ((uwU16Para != 0x0080) && ((_REG32(ulParaTableAddr + 8) != 0)
                                      || (_REG32(ulParaTableAddr + 12) != 0)))
        {
            printk("011 [ERR] Secondary commands supported: 0x%x [Spec] 0x0080\r\n", uwU16Para);
        }

        for(i = 0; i < 12; i++) //KIOXIA
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        if ((_REG32(ulParaTableAddr + 80) != 0x00004000) && (_REG32(ulParaTableAddr + 80) != 0))
        {
            printk("080 [ERR] Data Bytes pre Page: 0x%x [Spec] 0x00004000\r\n", _REG32(ulParaTableAddr + 80));
        }
        if ((_REG16(ulParaTableAddr + 84) != 0x07A0) && (_REG32(ulParaTableAddr + 84) != 0))
        {
            printk("084 [ERR] Spare Bytes pre Page: 0x%x [Spec] 0x07A0\r\n", _REG16(ulParaTableAddr + 84));
        }
        if ((_REG32(ulParaTableAddr + 92) != 0x00000480) && (_REG32(ulParaTableAddr + 92) != 0))
        {
            printk("092 [ERR] Pages pre Blk: 0x%x [Spec] 0x00000480\r\n", _REG32(ulParaTableAddr + 92));
        }

        if (_REG32(ulParaTableAddr + 96) == 0x00000F4C)
        {
            printk("096 [64G_Die] Blks pre LUN: 0x00000F4C\r\n");
        }
        else if (_REG32(ulParaTableAddr + 96) == 0x0000079C)
        {
            printk("096 [32G_Die] Blks pre LUN: 0x0000079C\r\n");
        }
        else
        {
            printk("096 [ERR] Blks pre LUN: 0x%x\r\n", _REG32(ulParaTableAddr + 96));
        }
        printk("100 LUNs pre CE: 0x%x\r\n", _REG08(ulParaTableAddr + 100));

        if (_REG08(ulParaTableAddr + 105) != 0x07)
        {
            printk("105 [ERR] Multi-plane operation attributes: 0x%x [Spec] 0x07\r\n",
                   _REG08(ulParaTableAddr + 105));
        }

        if (_REG08(ulParaTableAddr + 169) != 0x03)
        {
            printk("169 [ERR] Driver strength support: 0x%x [Spec] 0x03\r\n", _REG08(ulParaTableAddr + 169));
        }

        printk("208 Guaranteed valid blocks of target: 0x%x\r\n", _REG08(ulParaTableAddr + 208));

        uwU16Para = _REG08(ulParaTableAddr + 210);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 209);
        printk("209 Block endurance for guaranteed valid blocks: 0x%x\r\n", uwU16Para);

        for (i = 0; i < 4; i++) //byte 211~242
        {
            uwU16Para = _REG08(ulParaTableAddr + 214 + (i << 3));
            uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 213 + (i << 3));
            uwU16Para_2 = _REG08(ulParaTableAddr + 216 + (i << 3));
            uwU16Para_2 = (uwU16Para << 8) | _REG08(ulParaTableAddr + 215 + (i << 3));
            printk("%d Blk%d ECC correctability bits:0x%x Codeword size:0x%x Max BadBlk:0x%x Blk endurance:0x%x\r\n",
                   211 + (i << 3), i, _REG08(ulParaTableAddr + 211 + (i << 3)),
                   _REG08(ulParaTableAddr + 212 + (i << 3)), uwU16Para, uwU16Para_2);
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_TSB_BICS4_QLC)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para, uwU16Para_2;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_JESD) //JESD
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] JESD\r\n", ubStr);
        }

        if ((_REG16(ulParaTableAddr + 4) != 0x0004) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("004 [ERR] Revision number: 0x%x [Spec] 0x0004\r\n", _REG16(ulParaTableAddr + 4));
        }

        if (_REG32(ulParaTableAddr + 4) != 0)
        {
            printk("006 MTL support:%d\r\n", _REG16(ulParaTableAddr + 6) & 0x0002);
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0008) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Prog/Erase:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Read:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0040) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] Toggle Mode DDR3:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }

        if (((_REG16(ulParaTableAddr + 8) & 0x0004) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] GET/SET Feature:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 COPYBACK:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }

        uwU16Para = _REG08(ulParaTableAddr + 12);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 11);
        if ((uwU16Para != 0x0080) && ((_REG32(ulParaTableAddr + 8) != 0)
                                      || (_REG32(ulParaTableAddr + 12) != 0)))
        {
            printk("011 [ERR] Secondary commands supported: 0x%x [Spec] 0x0080\r\n", uwU16Para);
        }

        for(i = 0; i < 12; i++) //KIOXIA
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        if ((_REG32(ulParaTableAddr + 80) != 0x00004000) && (_REG32(ulParaTableAddr + 80) != 0))
        {
            printk("080 [ERR] Data Bytes pre Page: 0x%x [Spec] 0x00004000\r\n", _REG32(ulParaTableAddr + 80));
        }
        if ((_REG16(ulParaTableAddr + 84) != 0x07A0) && (_REG32(ulParaTableAddr + 84) != 0))
        {
            printk("084 [ERR] Spare Bytes pre Page: 0x%x [Spec] 0x07A0\r\n", _REG16(ulParaTableAddr + 84));
        }
        if ((_REG32(ulParaTableAddr + 92) != 0x00000480) && (_REG32(ulParaTableAddr + 92) != 0))
        {
            printk("092 [ERR] Pages pre Blk: 0x%x [Spec] 0x00000480\r\n", _REG32(ulParaTableAddr + 92));
        }

        if (_REG32(ulParaTableAddr + 96) == 0x00000F4C)
        {
            printk("096 [64G_Die] Blks pre LUN: 0x00000F4C\r\n");
        }
        else
        {
            printk("096 [ERR] Blks pre LUN: 0x%x\r\n", _REG32(ulParaTableAddr + 96));
        }
        printk("100 LUNs pre CE: 0x%x\r\n", _REG08(ulParaTableAddr + 100));

        if (_REG08(ulParaTableAddr + 105) != 0x07)
        {
            printk("105 [ERR] Multi-plane operation attributes: 0x%x [Spec] 0x07\r\n",
                   _REG08(ulParaTableAddr + 105));
        }

        if (_REG08(ulParaTableAddr + 169) != 0x03)
        {
            printk("169 [ERR] Driver strength support: 0x%x [Spec] 0x03\r\n", _REG08(ulParaTableAddr + 169));
        }

        printk("208 Guaranteed valid blocks of target: 0x%x\r\n", _REG08(ulParaTableAddr + 208));

        uwU16Para = _REG08(ulParaTableAddr + 210);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 209);
        printk("209 Block endurance for guaranteed valid blocks: 0x%x\r\n", uwU16Para);

        for (i = 0; i < 4; i++) //byte 211~242
        {
            uwU16Para = _REG08(ulParaTableAddr + 214 + (i << 3));
            uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 213 + (i << 3));
            uwU16Para_2 = _REG08(ulParaTableAddr + 216 + (i << 3));
            uwU16Para_2 = (uwU16Para << 8) | _REG08(ulParaTableAddr + 215 + (i << 3));
            printk("%d Blk%d ECC correctability bits:0x%x Codeword size:0x%x Max BadBlk:0x%x Blk endurance:0x%x\r\n",
                   211 + (i << 3), i, _REG08(ulParaTableAddr + 211 + (i << 3)),
                   _REG08(ulParaTableAddr + 212 + (i << 3)), uwU16Para, uwU16Para_2);
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_TSB_BICS5)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U16 uwU16Para, uwU16Para_2;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_JESD) //JESD
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] JESD\r\n", ubStr);
        }

        if ((_REG16(ulParaTableAddr + 4) != 0x0004) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("004 [ERR] Revision number: 0x%x [Spec] 0x0004\r\n", _REG16(ulParaTableAddr + 4));
        }

        if (_REG32(ulParaTableAddr + 4) != 0)
        {
            printk("006 MTL support:%d\r\n", _REG16(ulParaTableAddr + 6) & 0x0002);
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0008) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Prog/Erase:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] multi-plane Read:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }
        if (((_REG16(ulParaTableAddr + 6) & 0x0040) == 0) && (_REG32(ulParaTableAddr + 4) != 0))
        {
            printk("006 [ERR] Toggle Mode DDR3:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 6));
        }

        if (((_REG16(ulParaTableAddr + 8) & 0x0004) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 [ERR] GET/SET Feature:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }
        if (((_REG16(ulParaTableAddr + 8) & 0x0010) == 0) && (_REG32(ulParaTableAddr + 8) != 0))
        {
            printk("008 COPYBACK:0 (0x%x)\r\n", _REG16(ulParaTableAddr + 8));
        }

        uwU16Para = _REG08(ulParaTableAddr + 12);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 11);
        if ((uwU16Para != 0x0080) && ((_REG32(ulParaTableAddr + 8) != 0)
                                      || (_REG32(ulParaTableAddr + 12) != 0)))
        {
            printk("011 [ERR] Secondary commands supported: 0x%x [Spec] 0x0080\r\n", uwU16Para);
        }

        for(i = 0; i < 12; i++) //KIOXIA
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        if ((_REG32(ulParaTableAddr + 80) != 0x00004000) && (_REG32(ulParaTableAddr + 80) != 0))
        {
            printk("080 [ERR] Data Bytes pre Page: 0x%x [Spec] 0x00004000\r\n", _REG32(ulParaTableAddr + 80));
        }
        if ((_REG16(ulParaTableAddr + 84) != 0x07A0) && (_REG32(ulParaTableAddr + 84) != 0))
        {
            printk("084 [ERR] Spare Bytes pre Page: 0x%x [Spec] 0x07A0\r\n", _REG16(ulParaTableAddr + 84));
        }
        if ((_REG32(ulParaTableAddr + 92) != 0x00000540) && (_REG32(ulParaTableAddr + 92) != 0))
        {
            printk("092 [ERR] Pages pre Blk: 0x%x [Spec] 0x00000540\r\n", _REG32(ulParaTableAddr + 92));
        }

        if (_REG32(ulParaTableAddr + 96) == 0x00000CFC)
        {
            printk("096 [64G_Die] Blks pre LUN: 0x00000CFC\r\n");
        }
        else if (_REG32(ulParaTableAddr + 96) == 0x0000192C)
        {
            printk("096 [128G_Die] Blks pre LUN: 0x0000192C\r\n");
        }
        else
        {
            printk("096 [ERR] Blks pre LUN: 0x%x\r\n", _REG32(ulParaTableAddr + 96));
        }
        printk("100 LUNs pre CE: 0x%x\r\n", _REG08(ulParaTableAddr + 100));

        if (_REG08(ulParaTableAddr + 105) != 0x07)
        {
            printk("105 [ERR] Multi-plane operation attributes: 0x%x [Spec] 0x07\r\n",
                   _REG08(ulParaTableAddr + 105));
        }

        if (_REG08(ulParaTableAddr + 169) != 0x0A)
        {
            printk("169 [ERR] Driver strength support: 0x%x [Spec] 0x0A\r\n", _REG08(ulParaTableAddr + 169));
        }

        if (_REG16(ulParaTableAddr + 172) != 0x1FFF)
        {
            printk("172 [ERR] DDR3/4 speed grade: 0x%x [Spec] 0x1FFF\r\n", _REG16(ulParaTableAddr + 172));
        }

        printk("208 Guaranteed valid blocks of target: 0x%x\r\n", _REG08(ulParaTableAddr + 208));

        uwU16Para = _REG08(ulParaTableAddr + 210);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 209);
        printk("209 Block endurance for guaranteed valid blocks: 0x%x\r\n", uwU16Para);

        for (i = 0; i < 4; i++) //byte 211~242
        {
            uwU16Para = _REG08(ulParaTableAddr + 214 + (i << 3));
            uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 213 + (i << 3));
            uwU16Para_2 = _REG08(ulParaTableAddr + 216 + (i << 3));
            uwU16Para_2 = (uwU16Para << 8) | _REG08(ulParaTableAddr + 215 + (i << 3));
            printk("%d Blk%d ECC correctability bits:0x%x Codeword size:0x%x Max BadBlk:0x%x Blk endurance:0x%x\r\n",
                   211 + (i << 3), i, _REG08(ulParaTableAddr + 211 + (i << 3)),
                   _REG08(ulParaTableAddr + 212 + (i << 3)), uwU16Para, uwU16Para_2);
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#elif defined(FTL_SANDISK_BICS3) || defined(FTL_SANDISK_BICS4) || defined(FTL_SANDISK_BICS5)\
    || defined(FTL_SANDISK_BICS4_QLC) || defined(FTL_SANDISK_BICS5_QLC)
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
#if 0
    U32 i;
    U16 uwU16Para, uwU16Para_2;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U8 ubStr[21];
    U32 ulParaTableSize = 256;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    //parsing
    if ((_REG32(ulParaTableAddr) != 0xFFFFFFFF) && (_REG32(ulParaTableAddr) != 0))
    {
        if (_REG32(ulParaTableAddr) != ASCII_JESD) //JESD
        {
            for(i = 0; i < 4; i++)
            {
                ubStr[i] = _REG08(ulParaTableAddr + i);
            }
            ubStr[4] = '\0';
            printk("000 [ERR] %s [Spec] JESD\r\n", ubStr);
        }

        for(i = 0; i < 12; i++) //KIOXIA
        {
            ubStr[i] = _REG08(ulParaTableAddr + 32 + i);
        }
        ubStr[12] = '\0';
        printk("032 %s\r\n", ubStr);

        for(i = 0; i < 20; i++) //part number
        {
            ubStr[i] = _REG08(ulParaTableAddr + 44 + i);
        }
        ubStr[20] = '\0';
        printk("044 %s\r\n", ubStr);

        printk("100 LUNs pre CE: 0x%x\r\n", _REG08(ulParaTableAddr + 100));

        printk("208 Guaranteed valid blocks of target: 0x%x\r\n", _REG08(ulParaTableAddr + 208));

        uwU16Para = _REG08(ulParaTableAddr + 210);
        uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 209);
        printk("209 Block endurance for guaranteed valid blocks: 0x%x\r\n", uwU16Para);

        for (i = 0; i < 4; i++) //byte 211~242
        {
            uwU16Para = _REG08(ulParaTableAddr + 214 + (i << 3));
            uwU16Para = (uwU16Para << 8) | _REG08(ulParaTableAddr + 213 + (i << 3));
            uwU16Para_2 = _REG08(ulParaTableAddr + 216 + (i << 3));
            uwU16Para_2 = (uwU16Para << 8) | _REG08(ulParaTableAddr + 215 + (i << 3));
            printk("%d Blk%d ECC correctability bits:0x%x Codeword size:0x%x Max BadBlk:0x%x Blk endurance:0x%x\r\n",
                   211 + (i << 3), i, _REG08(ulParaTableAddr + 211 + (i << 3)),
                   _REG08(ulParaTableAddr + 212 + (i << 3)), uwU16Para, uwU16Para_2);
        }
    }

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
#endif
    return ret;
}
#else
U32 ParseParaPage()
{
    U32 ret = ERR_OK;
    U32 i;
    U32 ulParaTableAddr = NAND_TEST_RESP_BUF_VA_ADDR;
    U32 ulParaTableSize = 256;

    printk("==== Print Parameter byte 0~%d ====\r\n", ulParaTableSize - 1);

    printk("[ERR] Parsing not support\r\n");

    for(i = 0; i < ulParaTableSize; i += 32)
    {
        printk("%x_%x_%x_%x_%x_%x_%x_%x\r\n",
               _REG32(ulParaTableAddr + i + 0),
               _REG32(ulParaTableAddr + i + 4),
               _REG32(ulParaTableAddr + i + 8),
               _REG32(ulParaTableAddr + i + 12),
               _REG32(ulParaTableAddr + i + 16),
               _REG32(ulParaTableAddr + i + 20),
               _REG32(ulParaTableAddr + i + 24),
               _REG32(ulParaTableAddr + i + 28));
    }
    return ret;
}
#endif

//#endif
