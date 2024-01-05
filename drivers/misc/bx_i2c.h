// 硬件i2c接口的头文件

#ifndef __BX_I2C_H
#define __BX_I2C_H

// I2C寄存器
#define BX_I2C0_BASE                      (0x1FE58000)
#define BX_I2C1_BASE                      (0x1FE68000)
#define BX_I2C2_BASE                      (0x1FE70000)
#define BX_I2C3_BASE                      (0x1FE74000)
#define SHUT_CTRL_BASE                    (0x1FD00420)

// I2C的时钟频率
#define BX_I2C_CLOCK_DEFAULT              (100000)  // Hz. 默认频率
#define BX_I2C_CLOCK_MAX                  (400000)  // Hz, max 400 Kbits/sec


// I2C模块编号
typedef enum
{
    BX_I2C_0 = 0,
    BX_I2C_1,
    BX_I2C_2,
    BX_I2C_3
}bx_i2c_t;


// I2C数据传输方向
typedef enum
{
    BX_I2C_DIRECTION_WRITE = 0,       // 主机向从机写信息
    BX_I2C_DIRECTION_READ,            // 主机向从机读信息
}bx_i2c_direction_t;


// 硬件I2C信息
typedef struct
{
    bx_i2c_t I2Cx;                    // i2c模块编号
    unsigned long clock;                // i2c时钟频率，单位hz
}bx_i2c_info_t;


// I2C应答
typedef enum
{
    BX_I2C_ACK = 0,                   // 收到应答
    BX_I2C_NACK = 1,                  // 没收到应答
}bx_i2c_ack_t;


// 函数返回值
typedef enum
{
    BX_I2C_RET_OK = 0,                // OK
    BX_I2C_RET_TIMEOUT,               // 超时
}bx_i2c_ret_t;

extern int slave_addr;
extern bx_i2c_info_t i2c_info;

/*
 * 初始化指定i2c模块
 * @i2c_info_p 某个i2c模块的信息
 */
extern void i2c_init(bx_i2c_info_t *i2c_info_p);


/*
 * (再发送一个字节数据之后)接收从机发送的ACK信号
 * @i2c_info_p i2c模块信息
 * @ret BX_I2C_ACK or BX_I2C_NACK
 */
extern bx_i2c_ack_t i2c_receive_ack(bx_i2c_info_t *i2c_info_p);


/*
 * 接收数据
 * @i2c_info_p i2c模块信息
 * @buf 数据缓存
 * @len 待接收数据的长度
 */
extern bx_i2c_ret_t i2c_receive_data(bx_i2c_info_t *i2c_info_p, unsigned char *buf, int len);



/*
 * 发送START信号和地址
 * @i2c_info_p i2c模块信息
 * @slave_addr 从机地址
 * @direction 数据传输方向(读、写)
 */
extern bx_i2c_ret_t i2c_send_start_and_addr(bx_i2c_info_t *i2c_info_p, 
                                       unsigned char slave_addr,
                                       bx_i2c_direction_t direction);


/*
 * 发送数据
 * @i2c_info_p i2c模块信息
 * @data 待发送的数据
 * @len 待发送数据的长度
 */
extern bx_i2c_ret_t i2c_send_data(bx_i2c_info_t *i2c_info_p, unsigned char *data, int len);


/*
 * 发送STOP信号
 * @i2c_info_p i2c模块信息
 */
extern void i2c_send_stop(bx_i2c_info_t *i2c_info_p);


#endif

