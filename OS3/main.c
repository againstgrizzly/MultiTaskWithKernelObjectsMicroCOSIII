/*
***********************************************************************************************
* INCLUDE FILES
***********************************************************************************************
*/


#include  <cpu.h>
#include  <lib_mem.h>
#include  "os.h" //main header file for mc3i
#include  <bsp_clk.h>
#include  <bsp_int.h>
#include  <bsp_os.h>
#include  "bsp_led.h"
#include  <bsp_uart.h>
#include <stdio.h>
#include  "os_app_hooks.h"
#include  "app_cfg.h" //congigures the application
#include  "../iodefines/iorx63n.h" //used to check if an LED is off or on

/*
***********************************************************************************************
* LOCAL GLOBAL VARIABLES
***********************************************************************************************
*/

#define APP_TASK_START_STK_SIZE 50;

static OS_TCB StartupTaskTCB;//TCB for start up task
static OS_TCB AppTask1_TCB;//TCB for first task
static OS_TCB AppTask2_TCB;//TCB for second task
static OS_MUTEX AppMutex;//mutex kernel object
static OS_Q AppQ;//message queue kernel object; see uC/OS-III book for more info
static CPU_STK StartupTaskStk[128];//Stack size of startup task
static CPU_STK AppTask1_Stk[128];//stack size first task
static CPU_STK AppTask2_Stk[128];//stack size second task
static int flag = 4;
/*
***********************************************************************************************
* FUNCTION PROTOTYPES
***********************************************************************************************
*/
static void StartupTask (void *p_arg);//Prototypes of the tasks are declared
static void AppTask1 (void *p_arg);
static void AppTask2 (void *p_arg);

//Entry point of program
void main (void)
{
  OS_ERR  os_err;
  
  
  BSP_ClkInit();  /* Initialize the main clock */
  BSP_IntInit();  /* Initialize interrupt vector table  */
  BSP_OS_TickInit();  /* Initialize kernel tick timer  */
  
  Mem_Init();    /* Initialize Memory Managment Module */
  CPU_IntDis();  /* Disable all Interrupts  */
  CPU_Init();    /* Initialize the uC/CPU services  */
  
  OSInit(&os_err);  /* Initialize uC/OS-III  */
  if (os_err != OS_ERR_NONE) {
    while (1);
  }
  
  //create mutex;  
  OSMutexCreate(
                &AppMutex, //pointer to mutex
                "My App Mutex", //pointer to an ASCII string used to name the mutex
                &os_err 
                  );
  
  //create message queue; queues must be created before they are used
  OSQCreate(
            &AppQ, //pointer to the message queue control block
            "My App Queue", //pointer to an ASCII string used to name queue
            10u, //maximum size of the message queue
            &os_err //pointer to a varaibles that used to hold an error code
              );
  /*
  If the sen
  */
  
  App_OS_SetAllHooks();              /* Set all applications hooks */
  
  OSTaskCreate(&StartupTaskTCB,      /* Create the startup task */
               "Startup Task",
               StartupTask,
               0u,
               APP_CFG_STARTUP_TASK_PRIO,
               &StartupTaskStk[0u],
               StartupTaskStk[APP_CFG_STARTUP_TASK_STK_SIZE / 10u],
               APP_CFG_STARTUP_TASK_STK_SIZE,
               0u,
               0u,
               0u,
               (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               &os_err);
  if (os_err != OS_ERR_NONE) {
    while (1);
  }
  
  OSStart(&os_err);  /* Start multitasking (i.e. give control to uC/OS-III)  */
  
  while (DEF_ON) {  /* Should Never Get Here.                               */
    ;
  }
  
}

static  void  StartupTask (void *p_arg)
{
  OS_ERR  os_err;
  (void)p_arg;
  OS_TRACE_INIT();                                            /* Initialize the uC/OS-III Trace recorder              */
  BSP_OS_TickEnable();                                        /* Enable the tick timer and interrupt                  */
  BSP_LED_Init();                                             /* Initialize LEDs                                      */
  BSP_UART_Init();                                            /* Initialize UART                                      */
  BSP_UART_Start(115200u);                                    /* Start UART at 115200 baud rate                       */
  
#if OS_CFG_STAT_TASK_EN > 0u
  OSStatTaskCPUUsageInit(&os_err);                            /* Compute CPU capacity with no task running            */
#endif
  
#ifdef CPU_CFG_INT_DIS_MEAS_EN
  CPU_IntDisMeasMaxCurReset();
#endif
  BSP_UART_Printf("This is the startup task\n\r");            /* Print terminal message                               */
  
  
  
  
  
  
  //Create first task
  OSTaskCreate(&AppTask1_TCB,
               "App Task 1",
               AppTask1,
               0u,
               APP_TASK_1_STARTUP_TASK_PRIO,
               &AppTask1_Stk[0u],
               AppTask1_Stk[APP_CFG_APP_TASK_1_STK_SIZE/10u],
               APP_CFG_APP_TASK_1_STK_SIZE,
               0u,
               0u,
               0u,
               (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               &os_err);
  
  //Create second task
  OSTaskCreate(&AppTask2_TCB,
               "App Task 2",
               AppTask2,
               0u,
               APP_TASK_2_STARTUP_TASK_PRIO,
               &AppTask2_Stk[0u],
               AppTask2_Stk[APP_CFG_APP_TASK_2_STK_SIZE/10u],
               APP_CFG_APP_TASK_2_STK_SIZE,
               0u,
               0u,
               0u,
               (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               &os_err);
  
  
  
  while (1) {
    BSP_LED_Toggle(0);
    OSTimeDlyHMSM((CPU_INT16U) 0,
                  (CPU_INT16U) 0,
                  (CPU_INT16U) 0,
                  (CPU_INT32U)100,
                  (OS_OPT )OS_OPT_TIME_HMSM_STRICT,
                  (OS_ERR *)&os_err);
  }  
  
}

/*
When the resource is available (Instance variable flag) this message
posts a pointer to the flag using a message queue to AppTask2.
A mutex is used to guarantee mutual exclusion 
*/
static void AppTask1 (void *p_arg){
  
  OS_ERR err;
  CPU_TS ts;
  p_arg = p_arg;
  
  while (1) {
    
    //a task calls this function to suspend execution
    OSTimeDly (1,                //delay time in ticks until some time expires; example waits 1 tick
               OS_OPT_TIME_DLY,  //specifies user wants to use "relative mode"
               (OS_ERR *)&err);  //should return OS_ERR_because args are valid
    
    /*Always check the error code reurned by uC/OS-III. If "err" does not contains
    OS_ERR_NONE, OSTimeDly() did not perform the intended work
    */    
    
    
    //send message to AppQ message queue
    OSQPost (&AppQ,              //pointer to message queue
             (int *)flag,          //address of message payload
             sizeof(int *),     //specifies the size of the message (in bytes)
             OS_OPT_POST_FIFO,   //determines the type of post performed
             &err);
    
    
    
    
    OSMutexPend(&AppMutex,            //pointer to mutex
                0u,                   //timeout value; 0 indicates that task waits forever for mutx
                OS_OPT_PEND_BLOCKING, //determines if the user wants to block if the mutex is not available or not
                &ts,                  //pointer to timestamp; see documentation for more information
                &err);
    
    /* Access shared resource */ 
    OSMutexPost((OS_MUTEX *)&AppMutex, 
                (OS_OPT )OS_OPT_POST_NONE,
                (OS_ERR *)&err);
    
    
  }
}

/*
When the resource is available, it pulls a message off the message queue
The message is the address of the flag
All LEDs are turned off
The flag is then used to determine which led to turn on using a 
switch statement. It is then incremented and the lock released
*/
static void AppTask2 (void *p_arg){
  OS_ERR err;
  BSP_LED_Init();
  void *p_msg; //will contain the message (i.e., a pointer to "something"
  OS_MSG_SIZE msg_size; 
  CPU_TS ts;
  CPU_TS ts_delta;
  p_arg = p_arg;
  
  while (1) {
    
    BSP_UART_Printf("This is the startup task\n\r");            /* Print terminal message                               */
    
    
    
    p_msg = OSQPend((OS_Q *)&AppQ, 
                    (OS_TICK)0,
                    (OS_OPT )OS_OPT_PEND_BLOCKING,
                    &msg_size,
                    (CPU_TS *)&ts, //timestamp when message was sent
                    (OS_ERR *)&err);
    
    
    ts_delta = (OS_TS_GET()-ts); 
    
    //Process message received
    
    BSP_LED_Off(LED_ALL);
    flag = (int)p_msg;
    switch(flag){
      
    case 4 : BSP_LED_On(LED4); flag++;
    break;
    case 5 : BSP_LED_On(LED5);flag++;
    break;
    case 6 : BSP_LED_On(LED6);flag++;
    break;
    case 7 : BSP_LED_On(LED7);flag++;
    break;
    case 8 : BSP_LED_On(LED8);flag++;
    break;
    case 9 : BSP_LED_On(LED9);flag++;
    break;
    case 10 : BSP_LED_On(LED10);flag++;
    break;
    case 11 : BSP_LED_On(LED11);flag++;
    break;
    case 12 : BSP_LED_On(LED12);flag++;
    break;
    case 13 : BSP_LED_On(LED13);flag++;
    break;
    case 14 : BSP_LED_On(LED14);flag++;
    break;
    case 15 : BSP_LED_On(LED15);flag++;
    break;
    default: flag = 4; //Changes the flag to activate LED4
      break;
    }
    
    
  }
}







