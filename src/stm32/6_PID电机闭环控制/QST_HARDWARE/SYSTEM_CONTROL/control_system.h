#ifndef __CONTROL_SYSTEM_H
#define __CONTROL_SYSTEM_H

#include "stm32f10x.h"

/* TIM4 PWM：72MHz / (9 + 1) / (7199 + 1) = 1kHz。 */
#define CAR_PWM_PERIOD              ((u16)7199)
#define CAR_PWM_PRESCALER           ((u16)9)

/* 闭环每 50ms 更新一次；140 脉冲/周期约对应 1 转/秒。 */
#define CAR_CONTROL_PERIOD_MS       ((u16)50)
#define CAR_CRUISE_COUNTS           ((s16)140)
#define CAR_TURN_COUNTS             ((s16)100)
#define CAR_MIN_TARGET_COUNTS       ((s16)40)

/* 每轮约 2800 个编码器计数；每 10 个控制周期输出一次速度。 */
#define CAR_ENCODER_COUNTS_PER_REV  ((s32)2800)
#define CAR_SPEED_REPORT_STEPS      ((u16)10)

/* 原动作时间为 3200ms，直行和后退均延长到 2.5 倍。 */
#define CAR_STRAIGHT_TIME_MS        ((u16)8000)
#define CAR_REVERSE_TIME_MS         ((u16)8000)
#define CAR_STOP_PAUSE_MS           ((u16)800)

/* 周期路径每条直线边运行4秒，前进和后退采用相同目标轮速。 */
#define CAR_PATH_SEGMENT_TIME_MS    ((u16)4000)

/*
 * 双轮原地转向90度的编码器初值：按轮径65mm、轮距135mm估算。
 * 实车角度有固定误差时分别调整左右计数，不改 PI 参数。
 */
#define CAR_LEFT_SPIN_90_COUNTS     ((u32)1450)
#define CAR_RIGHT_SPIN_90_COUNTS    ((u32)1450)
#define CAR_TURN_TIMEOUT_MS         ((u16)6500)
#define CAR_TURN_DECEL_COUNTS       ((u32)450)

/* 实车标定结果：小车前进时左右编码器原始计数均为正。 */
#define LEFT_ENCODER_FORWARD_SIGN   ((s16)1)
#define RIGHT_ENCODER_FORWARD_SIGN  ((s16)1)

/* 增量 PI 参数。 */
#define CAR_PI_KP                   (7.0f)
#define CAR_PI_KI                   (0.8f)

/* 根据实测稳定 PWM 设置前进前馈；后退已经稳定，保留原前馈。 */
#define CAR_FORWARD_LEFT_FEEDFORWARD  ((s16)3000)
#define CAR_FORWARD_RIGHT_FEEDFORWARD ((s16)3300)
#define CAR_REVERSE_LEFT_FEEDFORWARD  ((s16)4200)
#define CAR_REVERSE_RIGHT_FEEDFORWARD ((s16)4200)
#define CAR_TURN_FEEDFORWARD          ((s16)4200)

/* 前进优先约束瞬时速度一致，后退保持已经验证可用的同步参数。 */
#define CAR_FORWARD_SYNC_SPEED_KP     (0.60f)
#define CAR_FORWARD_SYNC_POSITION_KP  (0.005f)
#define CAR_FORWARD_SYNC_LIMIT_COUNTS ((s16)45)
#define CAR_REVERSE_SYNC_SPEED_KP     (0.20f)
#define CAR_REVERSE_SYNC_POSITION_KP  (0.010f)
#define CAR_REVERSE_SYNC_LIMIT_COUNTS ((s16)35)
#define CAR_TURN_SYNC_SPEED_KP        (0.20f)
#define CAR_TURN_SYNC_POSITION_KP     (0.010f)
#define CAR_TURN_SYNC_LIMIT_COUNTS    ((s16)35)

/* 前进直线外环：把速度差和累计路程差直接换算为差动 PWM。 */
#define CAR_FORWARD_PWM_SYNC_SPEED_KP    (10.0f)
#define CAR_FORWARD_PWM_SYNC_POSITION_KP (0.10f)
#define CAR_FORWARD_PWM_SYNC_LIMIT       ((s16)800)
#define CAR_FORWARD_SPEED_FILTER_ALPHA   (0.25f)

/* 每个前进周期结束后，缓慢学习下一周期使用的前馈 PWM。 */
#define CAR_FEEDFORWARD_LEARN_DIVISOR     ((s16)4)
#define CAR_FEEDFORWARD_LEARN_START_STEP  ((u16)20)
#define CAR_FEEDFORWARD_MIN               ((s16)1800)
#define CAR_FEEDFORWARD_MAX               ((s16)6000)

typedef void (*ControlLedStep)(void);

void Control_System_Init(void);
void Control_SetWheels(s16 left_speed, s16 right_speed);
void Control_Stop(void);
void Control_RunTimedPID(s8 left_direction, s8 right_direction,
                         u16 run_time_ms, ControlLedStep led_effect);
u8 Control_RunEncoderPID(s8 left_direction, s8 right_direction,
                         u32 target_counts, u16 timeout_ms,
                         ControlLedStep led_effect);

/* 面向路径规划和后续避障的状态接口，灯效由接口内部自动绑定。 */
void Control_MoveForward(u16 run_time_ms);
void Control_MoveReverse(u16 run_time_ms);
u8 Control_TurnLeft90(void);
u8 Control_TurnRight90(void);

#endif
