#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __LPM_DEF_H_
#define __LPM_DEF_H_
 
#define LPM_MSG_NOP		0x0
#define LPM_MSG_NOP_SIZE	0x0

#define LPM_MSG_VER		0x1
#define LPM_MSG_VER_SIZE	0x5

#define LPM_MSG_READ_RTC	0x3
#define LPM_MSG_READ_RTC_SIZE	1

#define LPM_MSG_SET_TRIM	0x4
#define LPM_MSG_SET_TRIM_SIZE	0x0

#define LPM_MSG_ENTER_PASSIVE	0x5
#define LPM_MSG_ENTER_PASSIVE_SIZE	0x0

#define LPM_MSG_SET_WDT		0x6
#define LPM_MSG_SET_WDT_SIZE	2

#define LPM_MSG_SET_RTC		0x7
#define LPM_MSG_SET_RTC_SIZE	6

#define LPM_MSG_SET_FP		0x8
#define LPM_MSG_SET_FP_SIZE	1

#define LPM_MSG_SET_TIMER	0x9
#define LPM_MSG_SET_TIMER_SIZE	4

#define LPM_MSG_GET_STATUS	0xA
#define LPM_MSG_GET_STATUS_SIZE	0x1

#define LPM_MSG_GEN_RESET	0xB
#define LPM_MSG_GEN_RESET_SIZE	0x1

#define LPM_MSG_SET_WUD		0xC
#define LPM_MSG_SET_WUD_SIZE	0x0

#define LPM_MSG_GET_WUD		0xD
#define LPM_MSG_GET_WUD_SIZE	0x0

#define LPM_MSG_LGWR_OFFSET	0x10
#define LPM_MSG_LGWR_OFFSET_SIZE	0x2

#define LPM_MSG_SET_PIO		0x11
#define LPM_MSG_SET_PIO_SIZE	14

#define LPM_MSG_GET_ADV_FEA	0x12
#define LPM_MSG_GET_ADV_FEA_SIZE	1

#define LPM_MSG_SET_ADV_FEA	0x13
#define LPM_MSG_SET_ADV_FEA_SIZE	4

#define LPM_MSG_SET_KEY_SCAN	0x14
#define LPM_MSG_SET_KEY_SCAN_SIZE	2

#define LPM_MSG_CEC_ADDR	0x15
#define LPM_MSG_CEC_ADDR_SIZE	4

#define LPM_MSG_CEC_PARAMS	0x16
#define LPM_MSG_CEC_PARAMS_SIZE	14

#define LPM_MSG_CEC_SET_OSD_NAME  0x19
#define LPM_MSG_CEC_SET_OSD_NAME_SIZE  14

#define LPM_MSG_INFORM_HOST 0x17

#define LPM_MSG_SET_IR		0x41
#define LPM_MSG_SET_IR_SIZE	0x0

#define LPM_MSG_GET_IRQ		0x42
#define LPM_MSG_GET_IRQ_SIZE	0x3

#define LPM_MSG_TRACE_DATA	0x43
#ifdef MY_DEF_HERE
#define LPM_MSG_TRACE_DATA_SIZE	0x2
#else  
#define LPM_MSG_TRACE_DATA_SIZE	0x0
#endif  

#define LPM_MSG_BKBD_READ	0x44
#define LPM_MSG_BKBD_READ_SIZE	0x0

#define LPM_MSG_BKBD_WRITE	0x45
#define LPM_MSG_BKBD_WRITE_SIZE	0x6

#define LPM_MSG_EDID_INFO	0x46
#define LPM_MSG_EDID_INFO_SIZE	0x6

#define LPM_MSG_REPLY		0x80
#define LPM_MSG_REPLY_SIZE	0x0

#define LPM_MSG_ERR		0x82
#define LPM_MSG_ERR_SIZE	0x0

#define LPM_MAJOR_PROTO_VER	1
#define LPM_MINOR_PROTO_VER	7
#define LPM_MAJOR_SOFT_VER	2
#define LPM_MINOR_SOFT_VER	1
#define LPM_PATCH_SOFT_VER      0
#define LPM_BUILD_MONTH         07
#define LPM_BUILD_DAY           31
#define LPM_BUILD_YEAR          14

#define BYTE_MASK		0xFF

#define OWNER_MASK		0x3

#define NIBBLE_MASK		0xF

#define PIO_LEVEL_SHIFT		7
#define PIO_IT_SHIFT		6
#define PIO_DIRECTION_SHIFT	5

#define MSG_ZERO_SIZE		0

#define MSG_ID_AUTO		0

#define IRB_ENABLE		BIT(6)
#define LPM_CONFIG_1_OFFSET	0x4

#define DATA_OFFSET			0x010000

#define DMEM_PEN_HOLDING_CODE_SZ	0x80

#define PEN_HOLD_VAR_OFFSET_4xx		0xa4

#define LPM_DMEM_PEN_HOLD_VAR_RESET	{0xFF, 0xFF, 0xFF, 0xFF}
#ifdef MY_DEF_HERE
#define LPM_POWER_STATUS 0x48
#define XP70_IDLE_MODE 0xF
#endif  
#endif  