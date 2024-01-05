#ifndef __BX_PWM_H
#define __BX_PWM_H

#define BX_PWM0_BASE                      (0x1FE5C000)
#define BX_PWM1_BASE                      (0x1FE5C010)
#define BX_PWM2_BASE                      (0x1FE5C020)
#define BX_PWM3_BASE                      (0x1FE5C030)
#define BX_PWM4_BASE                      (0x1FE5C040)
#define BX_PWM5_BASE                      (0x1FE5C050)
#define BX_PWM6_BASE                      (0x1FE5C060)
#define BX_PWM7_BASE                      (0x1FE5C070)
#define BX_PWM8_BASE                      (0x1FE5C080)
#define BX_PWM9_BASE                      (0x1FE5C090)

// pwm控制寄存器的每个bit
#define BX_PWM_INT_LRC_EN                 (11)        // 低脉冲计数器中断使能
#define BX_PWM_INT_HRC_EN                 (10)        // 高脉冲计数器中断使能
#define BX_PWM_CNTR_RST                   (7)         // 使能CNTR计数器清零
#define BX_PWM_INT_SR                     (6)         // 中断状态位
#define BX_PWM_INTEN                      (5)         // 中断使能位
#define BX_PWM_SINGLE                     (4)         // 单脉冲控制位
#define BX_PWM_OE                         (3)         // 脉冲输出使能
#define BX_PWM_CNT_EN                     (0)         // 主计数器使能

// PWM寄存器偏移
#define BX_PWM_CNTR                       (0x0)
#define BX_PWM_HRC                        (0x4)
#define BX_PWM_LRC                        (0x8)
#define BX_PWM_CTRL                       (0xC)


// 硬件pwm工作模式
enum
{
    // 正常模式--连续输出pwm波形
    PWM_MODE_NORMAL = 0,
    
    // 单脉冲模式，每次调用只发送一个脉冲，调用间隔必须大于pwm周期
    PWM_MODE_PULSE
};


// 硬件pwm信息
typedef struct
{
    unsigned int num;                      // PWMn所在的num
    unsigned int mode;                      // 工作模式(单脉冲、连续脉冲)
    unsigned char duty1;                    // pwm的占空比
    unsigned char duty2;
    unsigned long period_ns;                // pwm周期(单位ns)
}pwm_info_t;




/*
 * 初始化PWMn
 * @pwm_info PWMn的详细信息
 */
void pwm_init(pwm_info_t *pwm_info);


/*
 * 禁止pwm
 * @pwm_info PWMn的详细信息
 */
void pwm_disable(pwm_info_t *pwm_info);



/*
 * 使能PWM
 * @pwm_info PWMn的详细信息
 */
void pwm_enable(pwm_info_t *pwm_info);


#endif

