#include "control_system.h"

#include "colorful_led.h"
#include "delay.h"
#include "encoder.h"
#include "sys.h"

/* 左电机：PB14/L-IB 控制方向，PB7/L-IA（TIM4_CH2）输出 PWM。 */
#define LEFT_DIRECTION  PBout(14)
#define LEFT_PWM        TIM4->CCR2

/* 右电机：PB13/R-IA 控制方向，PB6/R-IB（TIM4_CH1）输出 PWM。 */
#define RIGHT_DIRECTION PBout(13)
#define RIGHT_PWM       TIM4->CCR1

#define CAR_START_RAMP_STEPS ((u16)10)

typedef struct
{
    float output;
    float last_error;
} SpeedPI;

typedef struct
{
    s16 left_speed;
    s16 right_speed;
    u32 left_progress;
    u32 right_progress;
} ControlFeedback;

static s16 ClampS16(s16 value, s16 minimum, s16 maximum)
{
    if (value < minimum)
        return minimum;
    if (value > maximum)
        return maximum;
    return value;
}

static s16 ClampPwm(s16 pwm)
{
    return ClampS16(pwm, -(s16)CAR_PWM_PERIOD, (s16)CAR_PWM_PERIOD);
}

/**************************************************************************
函数功能：设置左电机的方向和 PWM
入口参数：speed，正数前进、负数后退、0 停止
返回值  ：无
**************************************************************************/
static void SetLeftMotor(s16 speed)
{
    u16 magnitude;

    speed = ClampPwm(speed);
    if (speed == 0)
    {
        LEFT_DIRECTION = 0;
        LEFT_PWM = 0;
    }
    else if (speed > 0)
    {
        LEFT_DIRECTION = 0;
        LEFT_PWM = (u16)speed;
    }
    else
    {
        magnitude = (u16)(-speed);
        LEFT_DIRECTION = 1;
        LEFT_PWM = CAR_PWM_PERIOD - magnitude;
    }
}

/**************************************************************************
函数功能：设置右电机的方向和 PWM
入口参数：speed，正数前进、负数后退、0 停止
返回值  ：无
**************************************************************************/
static void SetRightMotor(s16 speed)
{
    u16 magnitude;

    speed = ClampPwm(speed);
    if (speed == 0)
    {
        RIGHT_DIRECTION = 0;
        RIGHT_PWM = 0;
    }
    else if (speed > 0)
    {
        RIGHT_DIRECTION = 0;
        RIGHT_PWM = (u16)speed;
    }
    else
    {
        magnitude = (u16)(-speed);
        RIGHT_DIRECTION = 1;
        RIGHT_PWM = CAR_PWM_PERIOD - magnitude;
    }
}

void Control_SetWheels(s16 left_speed, s16 right_speed)
{
    SetLeftMotor(left_speed);
    SetRightMotor(right_speed);
}

void Control_Stop(void)
{
    Control_SetWheels(0, 0);
}

/**************************************************************************
函数功能：初始化电机方向 GPIO、TIM4 双通道 PWM 和两个编码器定时器
入口参数：无
返回值  ：无
**************************************************************************/
void Control_System_Init(void)
{
    GPIO_InitTypeDef gpio;
    TIM_TimeBaseInitTypeDef timer;
    TIM_OCInitTypeDef output_compare;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);

    TIM_TimeBaseStructInit(&timer);
    timer.TIM_Period = CAR_PWM_PERIOD;
    timer.TIM_Prescaler = CAR_PWM_PRESCALER;
    timer.TIM_ClockDivision = TIM_CKD_DIV1;
    timer.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &timer);

    TIM_OCStructInit(&output_compare);
    output_compare.TIM_OCMode = TIM_OCMode_PWM1;
    output_compare.TIM_OutputState = TIM_OutputState_Enable;
    output_compare.TIM_Pulse = 0;
    output_compare.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM4, &output_compare);
    TIM_OC2Init(TIM4, &output_compare);
    TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM4, ENABLE);

    Control_Stop();
    TIM_Cmd(TIM4, ENABLE);

    Encoder_Init_TIM2();
    Encoder_Init_TIM3();
    Encoder_Reset();
}

/**************************************************************************
函数功能：重置一个增量 PI 控制器
入口参数：控制器地址、车轮方向
返回值  ：无
说明    ：前馈 PWM 负责克服电机静摩擦，PI 只修正速度误差
**************************************************************************/
static void SpeedPI_Reset(SpeedPI *controller, s8 direction)
{
    controller->last_error = 0.0f;
    controller->output = (float)((s16)direction * CAR_PWM_FEEDFORWARD);
}

/**************************************************************************
函数功能：执行一次增量 PI 速度闭环计算
入口参数：控制器、目标脉冲数、实测脉冲数
返回值  ：有符号 PWM
**************************************************************************/
static s16 SpeedPI_Update(SpeedPI *controller, s16 target, s16 measured)
{
    float error;
    float increment;

    if (target == 0)
    {
        controller->output = 0.0f;
        controller->last_error = 0.0f;
        return 0;
    }

    error = (float)(target - measured);
    increment = CAR_PI_KP * (error - controller->last_error)
              + CAR_PI_KI * error;
    controller->output += increment;

    if (controller->output > (float)CAR_PWM_PERIOD)
        controller->output = (float)CAR_PWM_PERIOD;
    else if (controller->output < -(float)CAR_PWM_PERIOD)
        controller->output = -(float)CAR_PWM_PERIOD;

    /* 禁止控制量跨过零点，避免一次误差使电机突然反向。 */
    if (target > 0 && controller->output < 0.0f)
        controller->output = 0.0f;
    else if (target < 0 && controller->output > 0.0f)
        controller->output = 0.0f;

    controller->last_error = error;
    return (s16)controller->output;
}

static s16 NormalizeLeftEncoder(s16 raw_count)
{
    return (s16)(raw_count * LEFT_ENCODER_FORWARD_SIGN);
}

static s16 NormalizeRightEncoder(s16 raw_count)
{
    return (s16)(raw_count * RIGHT_ENCODER_FORWARD_SIGN);
}

static u8 WheelReached(s8 direction, u32 progress, u32 target_counts)
{
    if (direction == 0)
        return 1;
    return (u8)(progress >= target_counts);
}

static u8 ActionReached(s8 left_direction, s8 right_direction,
                        const ControlFeedback *feedback, u32 target_counts)
{
    return (u8)(WheelReached(left_direction, feedback->left_progress,
                             target_counts)
             && WheelReached(right_direction, feedback->right_progress,
                             target_counts));
}

/**************************************************************************
函数功能：读取并归一化左右编码器，同时累计车轮实际前进脉冲
入口参数：左右轮动作方向、反馈结构体
返回值  ：无
**************************************************************************/
static void ReadControlFeedback(s8 left_direction, s8 right_direction,
                                ControlFeedback *feedback)
{
    s16 left_motion;
    s16 right_motion;

    feedback->left_speed = NormalizeLeftEncoder((s16)Read_Encoder(2));
    feedback->right_speed = NormalizeRightEncoder((s16)Read_Encoder(3));

    left_motion = (s16)(feedback->left_speed * left_direction);
    right_motion = (s16)(feedback->right_speed * right_direction);

    if (left_direction != 0 && left_motion > 0)
        feedback->left_progress += (u32)left_motion;
    if (right_direction != 0 && right_motion > 0)
        feedback->right_progress += (u32)right_motion;
}

/**************************************************************************
函数功能：根据起步阶段和转向剩余脉冲生成平滑的基础目标速度
入口参数：循环次数、巡航速度、是否按脉冲终止、当前反馈、目标脉冲
返回值  ：本周期目标脉冲数
**************************************************************************/
static s16 SelectBaseTarget(u16 control_step, s16 cruise_counts,
                            u8 use_encoder_target,
                            s8 left_direction, s8 right_direction,
                            const ControlFeedback *feedback,
                            u32 target_counts)
{
    s16 target;
    s16 ramp_target;
    u32 remaining;
    u32 wheel_remaining;

    target = cruise_counts;

    if (control_step < CAR_START_RAMP_STEPS)
    {
        ramp_target = CAR_MIN_TARGET_COUNTS
                    + (s16)(((s32)(cruise_counts - CAR_MIN_TARGET_COUNTS)
                              * (control_step + 1)) / CAR_START_RAMP_STEPS);
        if (ramp_target < target)
            target = ramp_target;
    }

    if (!use_encoder_target)
        return target;

    remaining = target_counts;
    if (left_direction != 0)
    {
        wheel_remaining = feedback->left_progress >= target_counts
                        ? 0 : target_counts - feedback->left_progress;
        if (wheel_remaining < remaining)
            remaining = wheel_remaining;
    }
    if (right_direction != 0)
    {
        wheel_remaining = feedback->right_progress >= target_counts
                        ? 0 : target_counts - feedback->right_progress;
        if (wheel_remaining < remaining)
            remaining = wheel_remaining;
    }

    if (remaining < CAR_TURN_DECEL_COUNTS)
    {
        ramp_target = CAR_MIN_TARGET_COUNTS
                    + (s16)(((u32)(cruise_counts - CAR_MIN_TARGET_COUNTS)
                              * remaining) / CAR_TURN_DECEL_COUNTS);
        if (ramp_target < target)
            target = ramp_target;
    }

    return target;
}

/**************************************************************************
函数功能：根据左右轮瞬时速度差和累计路程差计算同步修正量
入口参数：左右方向和反馈值
返回值  ：目标脉冲修正量；正值表示左轮比右轮快
**************************************************************************/
static s16 CalculateSyncCorrection(s8 left_direction, s8 right_direction,
                                   const ControlFeedback *feedback)
{
    s16 left_motion_speed;
    s16 right_motion_speed;
    s32 position_error;
    float correction;

    if (left_direction == 0 || right_direction == 0)
        return 0;

    left_motion_speed = (s16)(feedback->left_speed * left_direction);
    right_motion_speed = (s16)(feedback->right_speed * right_direction);
    position_error = (s32)feedback->left_progress
                   - (s32)feedback->right_progress;

    correction = CAR_SYNC_SPEED_KP
               * (float)(left_motion_speed - right_motion_speed)
               + CAR_SYNC_POSITION_KP * (float)position_error;

    if (correction > (float)CAR_SYNC_LIMIT_COUNTS)
        correction = (float)CAR_SYNC_LIMIT_COUNTS;
    else if (correction < -(float)CAR_SYNC_LIMIT_COUNTS)
        correction = -(float)CAR_SYNC_LIMIT_COUNTS;

    return (s16)correction;
}

/**************************************************************************
函数功能：统一执行定时动作或编码器定距动作
入口参数：左右方向、运行时间、目标脉冲、终止方式和灯效函数
返回值  ：1 表示正常结束，0 表示编码器目标未在超时前到达
**************************************************************************/
static u8 RunPIDAction(s8 left_direction, s8 right_direction,
                       u16 run_time_ms, u32 target_counts,
                       u8 use_encoder_target, ControlLedStep led_effect)
{
    SpeedPI left_controller;
    SpeedPI right_controller;
    ControlFeedback feedback;
    u16 elapsed;
    u16 control_step;
    s16 cruise_counts;
    s16 base_target;
    s16 correction;
    s16 left_target;
    s16 right_target;
    s16 left_pwm;
    s16 right_pwm;
    u8 reached;

    if (left_direction < -1 || left_direction > 1
        || right_direction < -1 || right_direction > 1
        || (left_direction == 0 && right_direction == 0))
    {
        Control_Stop();
        return 0;
    }

    feedback.left_speed = 0;
    feedback.right_speed = 0;
    feedback.left_progress = 0;
    feedback.right_progress = 0;
    elapsed = 0;
    control_step = 0;
    reached = 0;
    cruise_counts = use_encoder_target ? CAR_TURN_COUNTS : CAR_CRUISE_COUNTS;

    SpeedPI_Reset(&left_controller, left_direction);
    SpeedPI_Reset(&right_controller, right_direction);
    Encoder_Reset();

    while (elapsed < run_time_ms)
    {
        ReadControlFeedback(left_direction, right_direction, &feedback);

        if (use_encoder_target
            && ActionReached(left_direction, right_direction,
                             &feedback, target_counts))
        {
            reached = 1;
            break;
        }

        base_target = SelectBaseTarget(control_step, cruise_counts,
                                       use_encoder_target,
                                       left_direction, right_direction,
                                       &feedback, target_counts);
        correction = CalculateSyncCorrection(left_direction, right_direction,
                                              &feedback);

        left_target = (s16)(base_target - correction);
        right_target = (s16)(base_target + correction);
        left_target = ClampS16(left_target, CAR_MIN_TARGET_COUNTS,
                               cruise_counts + CAR_SYNC_LIMIT_COUNTS);
        right_target = ClampS16(right_target, CAR_MIN_TARGET_COUNTS,
                                cruise_counts + CAR_SYNC_LIMIT_COUNTS);

        if (left_direction == 0
            || (use_encoder_target
                && WheelReached(left_direction, feedback.left_progress,
                                target_counts)))
            left_target = 0;
        else
            left_target = (s16)(left_target * left_direction);

        if (right_direction == 0
            || (use_encoder_target
                && WheelReached(right_direction, feedback.right_progress,
                                target_counts)))
            right_target = 0;
        else
            right_target = (s16)(right_target * right_direction);

        left_pwm = SpeedPI_Update(&left_controller, left_target,
                                  feedback.left_speed);
        right_pwm = SpeedPI_Update(&right_controller, right_target,
                                   feedback.right_speed);
        Control_SetWheels(left_pwm, right_pwm);

        if (led_effect != 0)
            led_effect();

        delay_ms(CAR_CONTROL_PERIOD_MS);
        elapsed += CAR_CONTROL_PERIOD_MS;
        control_step++;
    }

    Control_Stop();
    LED_All_Off_Step();

    if (use_encoder_target && !reached)
    {
        printf("PID TURN TIMEOUT L=%lu R=%lu\r\n",
               feedback.left_progress, feedback.right_progress);
    }

    delay_ms(CAR_STOP_PAUSE_MS);
    return use_encoder_target ? reached : 1;
}

void Control_RunTimedPID(s8 left_direction, s8 right_direction,
                         u16 run_time_ms, ControlLedStep led_effect)
{
    RunPIDAction(left_direction, right_direction, run_time_ms,
                 0, 0, led_effect);
}

u8 Control_RunEncoderPID(s8 left_direction, s8 right_direction,
                         u32 target_counts, u16 timeout_ms,
                         ControlLedStep led_effect)
{
    if (target_counts == 0 || timeout_ms == 0)
        return 0;

    return RunPIDAction(left_direction, right_direction, timeout_ms,
                        target_counts, 1, led_effect);
}
