// just copied a bunch of headers
// I'm not sure what we need
#include <linux/string.h>
#include "lcm_drv.h"
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <asm-generic/gpio.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/wakelock.h>
#include <mach/gpio_const.h>

#define CONFIG_MTK_LEGACY
#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#endif
/*#include <mach/mt_pm_ldo.h>*/

int mt_set_gpio_out(unsigned long pin, unsigned long output);
int mt_get_gpio_out(unsigned long pin);

static unsigned int lcm_rst_pin;
static unsigned int lcm_gpio_enn;
static unsigned int lcm_gpio_enp;

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define REGFLAG_DELAY 0xFFE
#define REGFLAG_END_OF_TABLE 0x1FF

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v) (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

struct LCM_setting_table
{
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = {
    {0xFF, 0x03, {0x98, 0x81, 0x03}},
    {0x01, 0x01, {0x00}},
    {0x02, 0x01, {0x00}},
    {0x03, 0x01, {0x73}},
    {0x04, 0x01, {0x00}},
    {0x05, 0x01, {0x00}},
    {0x06, 0x01, {0x0A}},
    {0x07, 0x01, {0x00}},
    {0x08, 0x01, {0x00}},
    {0x09, 0x01, {0x01}},
    {0x0A, 0x01, {0x01}},
    {0x0B, 0x01, {0x01}},
    {0x0C, 0x01, {0x01}},
    {0x0D, 0x01, {0x01}},
    {0x0E, 0x01, {0x01}},
    {0x0F, 0x01, {0x01}},
    {0x10, 0x01, {0x01}},
    {0x11, 0x01, {0x00}},
    {0x12, 0x01, {0x00}},
    {0x13, 0x01, {0x02}},
    {0x14, 0x01, {0x00}},
    {0x15, 0x01, {0x08}},
    {0x16, 0x01, {0x00}},
    {0x17, 0x01, {0x08}},
    {0x18, 0x01, {0x00}},
    {0x19, 0x01, {0x00}},
    {0x1A, 0x01, {0x00}},
    {0x1B, 0x01, {0x00}},
    {0x1C, 0x01, {0x00}},
    {0x1D, 0x01, {0x00}},
    {0x1E, 0x01, {0x40}},
    {0x1F, 0x01, {0x80}},
    {0x20, 0x01, {0x06}},
    {0x21, 0x01, {0x01}},
    {0x22, 0x01, {0x00}},
    {0x23, 0x01, {0x00}},
    {0x24, 0x01, {0x00}},
    {0x25, 0x01, {0x00}},
    {0x26, 0x01, {0x00}},
    {0x27, 0x01, {0x00}},
    {0x28, 0x01, {0x33}},
    {0x29, 0x01, {0x03}},
    {0x2A, 0x01, {0x00}},
    {0x2B, 0x01, {0x00}},
    {0x2C, 0x01, {0x00}},
    {0x2D, 0x01, {0x00}},
    {0x2E, 0x01, {0x00}},
    {0x2F, 0x01, {0x00}},
    {0x30, 0x01, {0x00}},
    {0x31, 0x01, {0x00}},
    {0x32, 0x01, {0x00}},
    {0x33, 0x01, {0x00}},
    {0x34, 0x01, {0x43}},
    {0x35, 0x01, {0x00}},
    {0x36, 0x01, {0x03}},
    {0x37, 0x01, {0x00}},
    {0x38, 0x01, {0x00}},
    {0x39, 0x01, {0x00}},
    {0x3A, 0x01, {0x00}},
    {0x3B, 0x01, {0x00}},
    {0x3C, 0x01, {0x00}},
    {0x3D, 0x01, {0x00}},
    {0x3E, 0x01, {0x00}},
    {0x3F, 0x01, {0x00}},
    {0x40, 0x01, {0x00}},
    {0x41, 0x01, {0x00}},
    {0x42, 0x01, {0x00}},
    {0x43, 0x01, {0x00}},
    {0x44, 0x01, {0x00}},
    {0x50, 0x01, {0x01}},
    {0x51, 0x01, {0x23}},
    {0x52, 0x01, {0x45}},
    {0x53, 0x01, {0x67}},
    {0x54, 0x01, {0x89}},
    {0x55, 0x01, {0xAB}},
    {0x56, 0x01, {0x01}},
    {0x57, 0x01, {0x23}},
    {0x58, 0x01, {0x45}},
    {0x59, 0x01, {0x67}},
    {0x5A, 0x01, {0x89}},
    {0x5B, 0x01, {0xAB}},
    {0x5C, 0x01, {0xCD}},
    {0x5D, 0x01, {0xEF}},
    {0x5E, 0x01, {0x00}},
    {0x5F, 0x01, {0x0C}},
    {0x60, 0x01, {0x0D}},
    {0x61, 0x01, {0x0E}},
    {0x62, 0x01, {0x0F}},
    {0x63, 0x01, {0x06}},
    {0x64, 0x01, {0x07}},
    {0x65, 0x01, {0x02}},
    {0x66, 0x01, {0x02}},
    {0x67, 0x01, {0x02}},
    {0x68, 0x01, {0x02}},
    {0x69, 0x01, {0x02}},
    {0x6A, 0x01, {0x02}},
    {0x6B, 0x01, {0x02}},
    {0x6C, 0x01, {0x02}},
    {0x6D, 0x01, {0x02}},
    {0x6E, 0x01, {0x05}},
    {0x6F, 0x01, {0x05}},
    {0x70, 0x01, {0x05}},
    {0x71, 0x01, {0x02}},
    {0x72, 0x01, {0x02}},
    {0x73, 0x01, {0x00}},
    {0x74, 0x01, {0x01}},
    {0x75, 0x01, {0x0C}},
    {0x76, 0x01, {0x0D}},
    {0x77, 0x01, {0x0E}},
    {0x78, 0x01, {0x0F}},
    {0x79, 0x01, {0x06}},
    {0x7A, 0x01, {0x07}},
    {0x7B, 0x01, {0x02}},
    {0x7C, 0x01, {0x02}},
    {0x7D, 0x01, {0x02}},
    {0x7E, 0x01, {0x02}},
    {0x7F, 0x01, {0x02}},
    {0x80, 0x01, {0x02}},
    {0x81, 0x01, {0x02}},
    {0x82, 0x01, {0x02}},
    {0x83, 0x01, {0x02}},
    {0x84, 0x01, {0x05}},
    {0x85, 0x01, {0x05}},
    {0x86, 0x01, {0x05}},
    {0x87, 0x01, {0x02}},
    {0x88, 0x01, {0x02}},
    {0x89, 0x01, {0x00}},
    {0x8A, 0x01, {0x01}},
    {0xFF, 0x03, {0x98, 0x81, 0x04}},
    {0x6C, 0x01, {0x15}},
    {0x6E, 0x01, {0x1E}},
    {0x6F, 0x01, {0x25}},
    {0x3A, 0x01, {0xA4}},
    {0x8D, 0x01, {0x19}},
    {0x87, 0x01, {0xBA}},
    {0x26, 0x01, {0x76}},
    {0xB2, 0x01, {0xD1}},
    {0xFF, 0x03, {0x98, 0x81, 0x01}},
    {0x22, 0x01, {0x0A}},
    {0x52, 0x01, {0x00}},
    {0x53, 0x01, {0x63}},
    {0x54, 0x01, {0x00}},
    {0x55, 0x01, {0x8A}},
    {0x50, 0x01, {0x7C}},
    {0x51, 0x01, {0x77}},
    {0x31, 0x01, {0x00}},
    {0x60, 0x01, {0x09}},
    {0xA0, 0x01, {0x1A}},
    {0xA1, 0x01, {0x38}},
    {0xA2, 0x01, {0x45}},
    {0xA3, 0x01, {0x14}},
    {0xA4, 0x01, {0x1A}},
    {0xA5, 0x01, {0x2A}},
    {0xA6, 0x01, {0x1F}},
    {0xA7, 0x01, {0x20}},
    {0xA8, 0x01, {0xB7}},
    {0xA9, 0x01, {0x1C}},
    {0xAA, 0x01, {0x29}},
    {0xAB, 0x01, {0x9E}},
    {0xAC, 0x01, {0x1D}},
    {0xAD, 0x01, {0x1D}},
    {0xAE, 0x01, {0x50}},
    {0xAF, 0x01, {0x27}},
    {0xB0, 0x01, {0x2C}},
    {0xB1, 0x01, {0x54}},
    {0xB2, 0x01, {0x64}},
    {0xB3, 0x01, {0x39}},
    {0xC0, 0x01, {0x1A}},
    {0xC1, 0x01, {0x38}},
    {0xC2, 0x01, {0x46}},
    {0xC3, 0x01, {0x14}},
    {0xC4, 0x01, {0x1A}},
    {0xC5, 0x01, {0x2B}},
    {0xC6, 0x01, {0x20}},
    {0xC7, 0x01, {0x21}},
    {0xC8, 0x01, {0xB7}},
    {0xC9, 0x01, {0x1C}},
    {0xCA, 0x01, {0x28}},
    {0xCB, 0x01, {0x9E}},
    {0xCC, 0x01, {0x1D}},
    {0xCD, 0x01, {0x1D}},
    {0xCE, 0x01, {0x50}},
    {0xCF, 0x01, {0x27}},
    {0xD0, 0x01, {0x2C}},
    {0xD1, 0x01, {0x55}},
    {0xD2, 0x01, {0x63}},
    {0xD3, 0x01, {0x39}},
    {0xFF, 0x03, {0x98, 0x81}},
    {0x35, 0x00, {0x00}},
    {0x11, 0x00, {0x00}},
    {REGFLAG_DELAY, 0x78, {0x00}},
    {0x29, 0x00, {0x00}},
    {REGFLAG_DELAY, 0x14, {0x00}},
    {REGFLAG_END_OF_TABLE, 0x00, {0x00}},
};

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
    {0x28, 0x00, {0x00}},
    {REGFLAG_DELAY, 0x14, {0x00}},
    {0x10, 0x00, {0x00}},
    {REGFLAG_DELAY, 0x78, {0x00}},
    {REGFLAG_END_OF_TABLE, 0x00, {0x00}},
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for (i = 0; i < count; i++)
    {

        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd)
        {

        case REGFLAG_DELAY:
            MDELAY(table[i].count);
            break;

        case REGFLAG_END_OF_TABLE:
            break;

        default:
            dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
        }
    }
}

static void lcm_get_pin(void)
{
    static struct device_node *node;

    node = of_find_compatible_node(NULL, NULL, "mediatek,lcm_j5080_gpio_node");
    lcm_rst_pin = of_get_named_gpio(node, "lcm_rst_pin", 0);
    lcm_gpio_enn = of_get_named_gpio(node, "lcm_gpio_enn", 0);
    lcm_gpio_enp = of_get_named_gpio(node, "lcm_gpio_enp", 0);
    gpio_request(lcm_rst_pin, "lcm_rst_pin");
    gpio_request(lcm_gpio_enn, "lcm_gpio_enn");
    gpio_request(lcm_gpio_enp, "lcm_gpio_enp");
}

/*
// https://github.com/olexandr1712/ALPS-MP-M0.MP1-V2.55.6_VZ6737M_65_A_M0_KERNEL/blob/dafad3af2c8d0e1fcd6179da43e8667748e00a25/include/asm-generic/gpio.h#L70
static inline int gpio_direction_output(unsigned gpio, int value)
{
    return gpio_direction_output(gpio), value);
}

// https://github.com/olexandr1712/ALPS-MP-M0.MP1-V2.55.6_VZ6737M_65_A_M0_KERNEL/blob/dafad3af2c8d0e1fcd6179da43e8667748e00a25/include/asm-generic/gpio.h#L98
static inline void gpio_set_value(unsigned gpio, int value)
{
	return gpio_set_value(gpio), value);
}
*/
// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));

    params->type = 2;
    params->dsi.LANE_NUM = 4;
    params->dsi.data_format.format = 2;
    params->dsi.PS = 2;
    params->dsi.vertical_sync_active = 4;
    params->dsi.ssc_range = 4;
    params->dsi.mode = 1;
    params->dsi.horizontal_sync_active = 80;
    params->dsi.ssc_disable = 1;
    params->dsi.HS_TRAIL = 15;
    params->dsi.esd_check_enable = 1;
    params->dsi.customization_esd_check_enable = 1;
    params->dsi.lcm_esd_check_table[0].count = 1;
    params->width = 720;
    params->height = 1280;
    params->dsi.vertical_backporch = 20;
    params->dsi.vertical_frontporch = 20;
    params->dsi.vertical_active_line = 1280;
    params->dsi.horizontal_backporch = 120;
    params->dsi.horizontal_frontporch = 120;
    params->dsi.horizontal_active_pixel = 720;
    params->dsi.PLL_CLOCK = 250;
    params->dsi.lcm_esd_check_table[0].cmd = 10;
    params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
}

static void lcm_init(void)
{
    printk("sym  lcm_init  start\n");
    lcm_get_pin();

    // initialization like: https://github.com/machester/tools/blob/637e6f243140d18fc9516d2500f959143e0dd978/mtk_lcm_patch/kernel-3.18/drivers/misc/mediatek/lcm/elink_lcm/tv080x0m_ns_xganl_boe/tv080x0m_ns_xganl_boe.c#L283-L287
    gpio_direction_output(lcm_gpio_enn, 0);
    gpio_direction_output(lcm_gpio_enp, 0);
    gpio_set_value(lcm_gpio_enn, 1);
    MDELAY(2);
    gpio_set_value(lcm_gpio_enp, 1);

    MDELAY(2);
    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(20);
    SET_RESET_PIN(1);
    MDELAY(120);

    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);

    printk("sym  lcm_init end\n");
}

static void lcm_suspend(void)
{
    printk("lcm_suspend start+++\n");

    push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);

    SET_RESET_PIN(0);
    MDELAY(5);

    gpio_direction_output(lcm_gpio_enp, 0);
    gpio_direction_output(lcm_gpio_enn, 0);
    gpio_set_value(lcm_gpio_enp, 0);
    MDELAY(2);
    gpio_set_value(lcm_gpio_enn, 0);
	
    mt_set_gpio_out(0x4E, 0);
    mt_set_gpio_out(0x50, 0);
	
    printk("lcm_suspend end---\n");
}

static void lcm_resume(void)
{
    printk("lcm_resume start+++\n");
    lcm_get_pin();

    gpio_direction_output(lcm_gpio_enn, 0);
    gpio_direction_output(lcm_gpio_enp, 0);
    gpio_set_value(lcm_gpio_enp, 1);
    MDELAY(1);
    gpio_set_value(lcm_gpio_enn, 1);

    MDELAY(1);
    SET_RESET_PIN(1);
    MDELAY(1);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(10);

    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);

    printk("lcm_resume end---\n");
}

static unsigned int lcm_compare_id(void)
{
    return 1;
}

// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER ili9881c_hd720_boe5p2_dj_5080_lcm_drv =
    {
        .name = "ili9881c_hd720_boe5p2_dj_5080",
        .set_util_funcs = lcm_set_util_funcs,
        .get_params = lcm_get_params,
        .init = lcm_init,
        .suspend = lcm_suspend,
        .resume = lcm_resume,
        .compare_id = lcm_compare_id,
};
