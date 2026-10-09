#include "./BSP/TPAD/tpad.h"
#include "./SYSTEM/usart/usart.h"
uint16_t g_tpad_default_val=0;//用来承装空按比较值的
TIM_HandleTypeDef g_tpad_gtim_cap_init;
static GPIO_InitTypeDef tpad_gtim_cap_gpioa_init;//仅限内部使用
static void tpad_timx_cap_init(uint16_t psc)
{
    TIM_IC_InitTypeDef tpad_gtim_cap_channel_init={0};
        g_tpad_gtim_cap_init.Instance=TIM5;//定时器5
        g_tpad_gtim_cap_init.Init.Prescaler=psc;//分频
        g_tpad_gtim_cap_init.Init.CounterMode=TIM_COUNTERMODE_UP;//向上计时
        g_tpad_gtim_cap_init.Init.Period=0xffff;//65535
        g_tpad_gtim_cap_init.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;//不分频
        g_tpad_gtim_cap_init.Init.AutoReloadPreload=TIM_AUTORELOAD_PRELOAD_ENABLE;//预装载使能
    HAL_TIM_IC_Init(&g_tpad_gtim_cap_init);
        tpad_gtim_cap_channel_init.ICPolarity=TIM_ICPOLARITY_RISING;//上升沿触发
        tpad_gtim_cap_channel_init.ICSelection=TIM_ICSELECTION_DIRECTTI;//直接映射，IC1映射到TI1
        tpad_gtim_cap_channel_init.ICPrescaler=TIM_ICPSC_DIV1;//不分频
        tpad_gtim_cap_channel_init.ICFilter=0;//不开滤波器
    HAL_TIM_IC_ConfigChannel(&g_tpad_gtim_cap_init, &tpad_gtim_cap_channel_init, TIM_CHANNEL_2);
    HAL_TIM_IC_Start(&g_tpad_gtim_cap_init,TIM_CHANNEL_2);
}

void HAL_TIM_IC_MspInit(TIM_HandleTypeDef *htim)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM5_CLK_ENABLE();
        tpad_gtim_cap_gpioa_init.Pin=GPIO_PIN_1;//PA1
        tpad_gtim_cap_gpioa_init.Mode=GPIO_MODE_AF_PP;//复用输出
        tpad_gtim_cap_gpioa_init.Pull=GPIO_PULLDOWN;//下拉
        tpad_gtim_cap_gpioa_init.Speed=GPIO_SPEED_FREQ_HIGH;//高速
    HAL_GPIO_Init(GPIOA,&tpad_gtim_cap_gpioa_init);
}

static void tpad_reset(void)
{
        tpad_gtim_cap_gpioa_init.Mode=GPIO_MODE_OUTPUT_PP;//推挽输出
    HAL_GPIO_Init(GPIOA,&tpad_gtim_cap_gpioa_init);
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_1,GPIO_PIN_RESET);//输出0此时IO口相当于接地就可以放电
    HAL_Delay(10);//放电10ms
    TIM5->SR=0;//清除标志位
        tpad_gtim_cap_gpioa_init.Mode=GPIO_MODE_INPUT;//输入
        tpad_gtim_cap_gpioa_init.Pull=GPIO_NOPULL;//浮空
    TIM5->CNT=0;
    HAL_GPIO_Init(GPIOA,&tpad_gtim_cap_gpioa_init);//变为浮空输入开始充电，此时reset完成
}

static uint16_t tpad_get_val(void)
{
    tpad_reset();
    while(__HAL_TIM_GET_FLAG(&g_tpad_gtim_cap_init, TIM_FLAG_CC1)==0)//判断捕获标志位是否置1（没开中断也会置1）
    {
        if(TIM5->CNT>0xffff-500)//判断当前计数值是否超出规定范围
        {
            return TIM5->CNT;
        }
    }
    return __HAL_TIM_GET_COMPARE(&g_tpad_gtim_cap_init, TIM_CHANNEL_2);//返回比较值
}

static uint16_t tpad_get_maxval(uint8_t n)
{
    uint16_t value=0;
    uint16_t mvalue=0;
    while(n--)//重复n次
    {
        mvalue=tpad_get_val();
        if(mvalue>value)
        {
            value=mvalue;//取最大值
        }
    }
    return value;
}

uint8_t tpad_init(uint16_t psc)
{
    tpad_timx_cap_init(psc);//初始化分频系数
    uint16_t buf[10]={0};
    uint16_t temp=0;
    for(int i=0;i<10;i++)
    {
        buf[i]=tpad_get_val();//捕获0次
        HAL_Delay(10);
    }
    for(int i=0;i<9;i++)//排序
    {
        for(int j=i+1;j<10;j++)
        {
            if(buf[i]<buf[j])
            {
                temp=buf[i];
                buf[i]=buf[j];
                buf[j]=temp;
            }
        }
    }
    temp=0;
    for(int i=2;i<8;i++)
    {
        temp+=buf[i];
    }
    g_tpad_default_val=temp/6;//取中间6次的数值求平均值
    printf("初始值为:%d\r\n",g_tpad_default_val);//打印出来未按下的平均时长
    return 0;
}

uint8_t tpad_scan(uint8_t mode)
{
    static uint8_t keyen = 0;   /* 0, 可以开始检测;  >0, 还不能开始检测; */
    uint8_t res = 0;
    uint8_t sample = 3;         /* 默认采样次数为3次 */
    uint16_t rval;

    if (mode)
    {
        sample = 6;     /* 支持连按的时候，设置采样次数为6次 */
        keyen = 0;      /* 支持连按, 每次调用该函数都可以检测 */
    }

    rval = tpad_get_maxval(sample);

    if (rval > (g_tpad_default_val + 100))/* 大于tpad_default_val+TPAD_GATE_VAL,有效 */
    {
        if (keyen == 0)
        {
            res = 1;    /* keyen==0, 有效 */
        }

        //printf("r:%d\r\n", rval);   /* 输出计数值, 调试的时候才用到 */
        keyen = 3;      /* 至少要再过3次之后才能按键有效 */
    }

    if (keyen)keyen--;

    return res;
}

uint8_t tpad_scan2(uint8_t mode)
{
    static uint8_t mode_just=1;
    uint16_t rval=0;
    uint8_t res=0;
    rval=tpad_get_maxval(3);
    if(mode)
    {
        mode_just=3;
    }
    if(rval>(g_tpad_default_val+100))
    {
        if(mode_just==3)
        {
            res=1;
        }
        mode_just=0;
    }
    if(mode_just!=3) mode_just++;
    return res;
}


