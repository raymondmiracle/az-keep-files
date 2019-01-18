#ifndef __MT65XX_LCM_LIST_H__
#define __MT65XX_LCM_LIST_H__

#include <lcm_drv.h>

#if defined(MTK_LCM_DEVICE_TREE_SUPPORT)
extern LCM_DRIVER lcm_common_drv;
#else
extern LCM_DRIVER ili9881c_hd720_boe5p2_dj_5080_lcm_drv;
extern LCM_DRIVER tm_ft8606_lcm_drv;
extern LCM_DRIVER ili9881c_hd720_dsi_vdo_lcm_drv;	
#endif

#ifdef BUILD_LK
extern void mdelay(unsigned long msec);
#endif

#endif
