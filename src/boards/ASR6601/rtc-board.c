#include <math.h>
#include "tremo_rtc.h"
#include "tremo_rcc.h"
#include "tremo_pwr.h"
#include "utilities.h"
#include "timer.h"
#include "radio.h"
#include "rtc-board.h"
#include <tremo_regs.h>

#define MINIMAL_TIMEOUT 5
#define TICKS_PER_SECOND 32768
/*!
 * Number of seconds in a minute
 */
static const uint8_t SecondsInMinute = 60;

/*!
 * Number of seconds in an hour
 */
static const uint16_t SecondsInHour = 3600;

/*!
 * Number of seconds in a day
 */
static const uint32_t SecondsInDay = 86400;

/*!
 * Number of hours in a day
 */
//static const uint8_t HoursInDay = 24;

/*!
 * Number of seconds in a leap year
 */
static const uint32_t SecondsInLeapYear = 31622400;

/*!
 * Number of seconds in a year
 */
static const uint32_t SecondsInYear = 31536000;

/*!
 * Number of days in each month on a normal year
 */
static const uint8_t DaysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/*!
 * Number of days in each month on a leap year
 */
static const uint8_t DaysInMonthLeapYear[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/*!
 * RTC timer context
 */
typedef struct RtcCalendar_s {
    rtc_calendar_t CalendarTime;
} RtcCalendar_t;

/*!
 * Current RTC timer context
 */
RtcCalendar_t RtcCalendarContext;

TimerTime_t RtcTimerContext;

/*!
 * \brief Indicates if the RTC is already Initialized or not
 */
static bool RtcInitialized = false;

/*!
 * \brief Converts a RtcCalendar_t value into TimerTime_t value
 *
 * \param[IN/OUT] calendar Calendar value to be converted
 *                         [NULL: compute from "now",
 *                          Others: compute from given calendar value]
 * \retval timerTime New TimerTime_t value
 */
static TimerTime_t RtcConvertCalendarTickToTimerTime(RtcCalendar_t *calendar);

//static TimerTime_t RtcConvertTickToMs( TimerTime_t timeoutValue );


/*!
 * \brief Returns the internal RTC Calendar and check for RTC overflow
 *
 * \retval calendar RTC calendar
 */
static RtcCalendar_t RtcGetCalendar();

extern void rtc_check_syn();

void RtcInit() {
    if(RtcInitialized == false) {
        rtc_deinit();
        rcc_enable_peripheral_clk(RCC_PERIPHERAL_RTC, true);

        rtc_calendar_cmd(ENABLE);
        NVIC_EnableIRQ(RTC_IRQn);
        
        RtcInitialized = true;
    }
}

uint32_t RtcGetMinimumTimeout() {
    return MINIMAL_TIMEOUT;
}

uint32_t RtcMs2Tick(TimerTime_t milliseconds) {
    return (uint32_t) round(((double)milliseconds) * TICKS_PER_SECOND / 1000);
}


TimerTime_t RtcTick2Ms(uint32_t tick) {
    return (uint32_t) round((double) tick / TICKS_PER_SECOND * 1000);
}

void RtcDelayMs(TimerTime_t milliseconds) {
    //!!!
}

void RtcSetAlarm(uint32_t timeout) {
    RtcCalendar_t now;
    if(timeout <= MINIMAL_TIMEOUT)
        timeout = MINIMAL_TIMEOUT;

    // Load the RTC calendar
    now = RtcGetCalendar();

    // Save the calendar into RtcCalendarContext to be able to calculate the elapsed time
    RtcCalendarContext = now;

    rtc_cyc_cmd(DISABLE);
    rtc_config_cyc_max(RtcMs2Tick(timeout));
}


void RtcStopAlarm() {
    rtc_cyc_cmd(DISABLE);
    rtc_config_interrupt(RTC_CYC_IT, DISABLE);
}

void RtcStartAlarm(uint32_t timeout) {
    RtcSetAlarm(timeout);
    rtc_config_cyc_wakeup(ENABLE);
    rtc_cyc_cmd(ENABLE);
    rtc_config_interrupt(RTC_CYC_IT, ENABLE);
}

TimerTime_t RtcSetTimerContext() {
    RtcTimerContext = RtcConvertCalendarTickToTimerTime(NULL);
    return RtcTimerContext;
}

TimerTime_t RtcGetTimerContext() {return RtcTimerContext;}

uint32_t RtcGetCalendarTime(uint16_t* milliseconds) {
    *(milliseconds) = RtcConvertCalendarTickToTimerTime(NULL);
    return *(milliseconds) / 1000;
}

TimerTime_t RtcGetTimerValue() {return RtcConvertCalendarTickToTimerTime(NULL);}

uint32_t RtcGetTimerElapsedTime() {
    return RtcConvertCalendarTickToTimerTime(NULL) - RtcTimerContext;
}

static TimerTime_t RtcConvertCalendarTickToTimerTime(RtcCalendar_t *calendar) {
    TimerTime_t timeCounter = 0;
    RtcCalendar_t now;
    TimerTime_t timeCounterTemp = 0;

    // Passing a NULL pointer will compute from "now" else,
    // compute from the given calendar value
    if(calendar == NULL)
        now = RtcGetCalendar();
    else
        now = *calendar;

    // Years (calculation valid up to year 2099)
    for(int16_t i = 0; i < (now.CalendarTime.year-2000) ; i++) {
        if(( i == 0 ) || ( i % 4 ) == 0)
            timeCounterTemp += (uint32_t)SecondsInLeapYear;
        else
            timeCounterTemp += (uint32_t)SecondsInYear;
    }

    // Months (calculation valid up to year 2099)*/
    if((now.CalendarTime.year % 4) == 0) {
        for( uint8_t i = 0; i < (now.CalendarTime.month - 1); i++)
            timeCounterTemp += (uint32_t)(DaysInMonthLeapYear[i] * SecondsInDay);
    }
    else {
        for( uint8_t i = 0;  i < ( now.CalendarTime.month - 1 ); i++ )
            timeCounterTemp += ( uint32_t )( DaysInMonth[i] * SecondsInDay );
    }

    timeCounterTemp += (TimerTime_t)( now.CalendarTime.second +
                                      ((TimerTime_t)now.CalendarTime.minute * SecondsInMinute) +
                                      ((TimerTime_t)now.CalendarTime.hour * SecondsInHour) +
                                      ((TimerTime_t)((now.CalendarTime.day-1) * SecondsInDay)));

    timeCounter = timeCounterTemp*1000 + now.CalendarTime.subsecond / 1000;

    return (timeCounter);
}

static RtcCalendar_t RtcGetCalendar() {
    RtcCalendar_t now;
    rtc_get_calendar(&now.CalendarTime);
    return(now);
}

void RtcOnIrq() {
    uint8_t intr_stat;
    intr_stat =  rtc_get_status(RTC_CYC_SR);
 
    if(intr_stat == true) {
        rtc_cyc_cmd(DISABLE);
        rtc_config_interrupt(RTC_CYC_IT, DISABLE); // disable
        rtc_set_status(RTC_CYC_SR, false); // clear
        TimerIrqHandler();
        rtc_config_interrupt(RTC_CYC_IT, ENABLE); // enable
    }
}

void RtcProcess()
{
    // Not used on this platform.
}


