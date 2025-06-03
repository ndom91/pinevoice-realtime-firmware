#include <stdio.h>
#include <stdint.h>
#include <drv/rtc.h>
#include <drv/irq.h>
#include <bl606p_hbn.h>
#include <drv/timer.h>
// #include <aos/kernel.h>
// #include <k_api.h>
#include <time.h>
#include <bl606p_hbn.h>
#include <utils_list.h>

#define RTC_TIME_BASE_YEAR                          (1900)                                 //< Year,      Effective range[1970,2099]
#define RTC_TIME_MAX_VAL_YEAR                       (199)                                  ///< Year,     Maximum value
#define RTC_TIME_MAX_VAL_MON                        (11)                                   ///< Month,    Maximum value
#define RTC_TIME_MAX_VAL_DAY                        (31)                                   ///< Day,      Maximum value
#define RTC_TIME_MAX_VAL_HOUR                       (23)                                   ///< Hour,     Maximum value
#define RTC_TIME_MAX_VAL_MIN                        (59)                                   ///< Minute,   Maximum value
#define RTC_TIME_MAX_VAL_SEC                        (59)                                   ///< Second,   Maximum value
#define RTC_TIME_MIN_VAL_YEAR                       (70)                                   ///< Year,     Minimum value
#define RTC_TIME_MIN_VAL_MON                        (0)                                    ///< Month,    Minimum value
#define RTC_TIME_MIN_VAL_DAY                        (1)                                    ///< Day,      Minimum value
#define RTC_TIME_MIN_VAL_HOUR                       (0)                                    ///< Hour,     Minimum value
#define RTC_TIME_MIN_VAL_MIN                        (0)                                    ///< Minute,   Minimum value
#define RTC_TIME_MIN_VAL_SEC                        (0)                                    ///< Second,   Minimum value

#define SEC_PER_MIN  ((time_t)60)
#define SEC_PER_HOUR ((time_t)60 * SEC_PER_MIN)
#define SEC_PER_DAY  ((time_t)24 * SEC_PER_HOUR)
#define BL_RTC_COUNTER_TO_MS(CNT)  (((CNT) >> 5) - ((CNT) >> 11) - ((CNT) >> 12))  // ((CNT)*(1024-16-8)/32768)
#define BL_RTC_MAX_COUNTER         (0x000000FFFFFFFFFFllu)
#define BL_RTC_MAX_TIMESTAMP_MS    (BL_RTC_COUNTER_TO_MS(BL_RTC_MAX_COUNTER))
#define RTC_SEC_TO_TICK            32768
#define RTC_IS_LEAPYEAR(_year_)                  (((_year_) % 400) ? (((_year_) % 100) ? (((_year_) % 4) ? (int)0 : (int)1) : (int)0) : (int)1)

#define RTC_TIME_IS_BCD            0

static struct tm *s_rtc_base = NULL;

static const uint8_t leap_year[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static const uint8_t noleap_year[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static const uint16_t g_noleap_daysbeforemonth[13] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

static inline int clock_isleapyear(int year)
{
    return (year % 400) ? ((year % 100) ? ((year % 4) ? 0 : 1) : 0) : 1;
}

static int clock_daysbeforemonth(int month, uint8_t leapyear)
{
    int retval = g_noleap_daysbeforemonth[month];

    if (month >= 2 && leapyear) {
        retval++;
    }

    return retval;
}

static inline int __bcd_to_int(unsigned char value)
{
    int temp = 0;
    temp = (value>>4)*10;
    temp += value&0x0f;
    return temp;
}

static inline unsigned char __int_to_bcd(int value)
{
	unsigned char temp = 0;
	temp = (value/10)*16;
	temp += (value%10);
	return temp;
}

static void clock_utc2calendar(time_t days, int *year, int *month,
                               int *day)
{

    /* There is one leap year every four years, so we can get close with the
     * following:
     */

    int value   = days  / (4 * 365 + 1); /* Number of 4-years periods since the epoch */
    days   -= value * (4 * 365 + 1); /* Remaining days */
    value <<= 2;                     /* Years since the epoch */

    /* Then we will brute force the next 0-3 years */
    uint8_t leapyear;
    int  tmp;

    for (; ;) {
        /* Is this year a leap year (we'll need this later too) */

        leapyear = clock_isleapyear(value + 1970);

        /* Get the number of days in the year */

        tmp = (leapyear ? 366 : 365);

        /* Do we have that many days? */

        if (days >= tmp) {
            /* Yes.. bump up the year */

            value++;
            days -= tmp;
        } else {
            /* Nope... then go handle months */

            break;
        }
    }

    /* At this point, value has the year and days has number days into this year */

    *year = 1970 + value;

    /* Handle the month (zero based) */
    int  min = 0;
    int  max = 11;

    do {
        /* Get the midpoint */

        value = (min + max) >> 1;

        /* Get the number of days that occurred before the beginning of the month
         * following the midpoint.
         */

        tmp = clock_daysbeforemonth(value + 1, leapyear);

        /* Does the number of days before this month that equal or exceed the
         * number of days we have remaining?
         */

        if (tmp > days) {
            /* Yes.. then the month we want is somewhere from 'min' and to the
             * midpoint, 'value'.  Could it be the midpoint?
             */

            tmp = clock_daysbeforemonth(value, leapyear);

            if (tmp > days) {
                /* No... The one we want is somewhere between min and value-1 */

                max = value - 1;
            } else {
                /* Yes.. 'value' contains the month that we want */

                break;
            }
        } else {
            /* No... The one we want is somwhere between value+1 and max */

            min = value + 1;
        }

        /* If we break out of the loop because min == max, then we want value
         * to be equal to min == max.
         */

        value = min;
    } while (min < max);

    /* The selected month number is in value. Subtract the number of days in the
     * selected month
     */
    days -= clock_daysbeforemonth(value, leapyear);

    /* At this point, value has the month into this year (zero based) and days has
     * number of days into this month (zero based)
     */

    *month = value + 1; /* 1-based */
    *day   = days + 1;  /* 1-based */
}

static int __check_tm_ok(struct tm *rtctime)
{
    if (rtctime->tm_year < 70 || rtctime->tm_year >= 200) {
        goto error_time;
    }

    int32_t leap = 1;

    leap = clock_isleapyear(rtctime->tm_year + 1900);

    if (rtctime->tm_sec < 0 || rtctime->tm_sec >= 60) {
        goto error_time;
    }

    if (rtctime->tm_min < 0 || rtctime->tm_min >= 60) {
        goto error_time;
    }

    if (rtctime->tm_hour < 0 || rtctime->tm_hour >= 24) {
        goto error_time;
    }

    if (rtctime->tm_mon < 0 || rtctime->tm_mon >= 12) {
        goto error_time;
    }

    if (leap) {
        if (rtctime->tm_mday < 1 || rtctime->tm_mday > leap_year[rtctime->tm_mon]) {
            goto error_time;
        }
    } else {
        if (rtctime->tm_mday < 1 || rtctime->tm_mday > noleap_year[rtctime->tm_mon]) {
            goto error_time;
        }
    }

    return 0;
error_time:
    return -1;
}

#define _INT_TO_BCD(is_bcd, num) (is_bcd)?__int_to_bcd((num)):(num)
#define _BCD_TO_INT(is_bcd, num) (is_bcd)?__bcd_to_int((num)):(num)

static void __tm_to_rtctime(csi_rtc_time_t *rtc_time, const struct tm *time, uint8_t is_bcd)
{
    rtc_time->tm_sec = _INT_TO_BCD(is_bcd, time->tm_sec);
    rtc_time->tm_min = _INT_TO_BCD(is_bcd, time->tm_min);
    rtc_time->tm_hour = _INT_TO_BCD(is_bcd, time->tm_hour);
    rtc_time->tm_wday = _INT_TO_BCD(is_bcd, time->tm_wday);
    rtc_time->tm_mday = _INT_TO_BCD(is_bcd, time->tm_mday);
    rtc_time->tm_mon = _INT_TO_BCD(is_bcd, time->tm_mon);
    rtc_time->tm_year = _INT_TO_BCD(is_bcd, time->tm_year - 70);
}

static void __rtctime_to_tm(struct tm *tim, const csi_rtc_time_t *time, uint8_t is_bcd)
{
    tim->tm_sec = _BCD_TO_INT(is_bcd, time->tm_sec);
    tim->tm_min = _BCD_TO_INT(is_bcd, time->tm_min);
    tim->tm_hour = _BCD_TO_INT(is_bcd, time->tm_hour);
    tim->tm_wday = _BCD_TO_INT(is_bcd, time->tm_wday);
    tim->tm_mday = _BCD_TO_INT(is_bcd, time->tm_mday);
    tim->tm_mon = _BCD_TO_INT(is_bcd, time->tm_mon);
    tim->tm_year = _BCD_TO_INT(is_bcd, time->tm_year);
    tim->tm_year += 70;
}
static time_t __clock_calendar2utc(int year, int month, int day)
{
    time_t days;

    /* Years since epoch in units of days (ignoring leap years). */

    days = (year - 1970) * 365;

    /* Add in the extra days for the leap years prior to the current year. */

    days += (year - 1969) >> 2;

    /* Add in the days up to the beginning of this month. */

    days += (time_t)clock_daysbeforemonth(month, clock_isleapyear(year));

    /* Add in the days since the beginning of this month (days are 1-based). */

    days += day - 1;

    /* Then convert the seconds and add in hours, minutes, and seconds */

    return days;
}

static struct tm *__gmtime_r(const time_t *timer, struct tm *result)
{
    time_t epoch;
    time_t jdn;
    int    year;
    int    month;
    int    day;
    int    hour;
    int    min;
    int    sec;

    /* Get the seconds since the EPOCH */

    epoch = *timer;

    /* Convert to days, hours, minutes, and seconds since the EPOCH */

    jdn    = epoch / SEC_PER_DAY;
    epoch -= SEC_PER_DAY * jdn;

    hour   = epoch / SEC_PER_HOUR;
    epoch -= SEC_PER_HOUR * hour;

    min    = epoch / SEC_PER_MIN;
    epoch -= SEC_PER_MIN * min;

    sec    = epoch;

    /* Convert the days since the EPOCH to calendar day */
    clock_utc2calendar(jdn, &year, &month, &day);

    /* Then return the struct tm contents */

    result->tm_year  = (int)year - 1900; /* Relative to 1900 */
    result->tm_mon   = (int)month - 1;   /* zero-based */
    result->tm_mday  = (int)day;         /* one-based */
    result->tm_hour  = (int)hour;
    result->tm_min   = (int)min;
    result->tm_sec   = (int)sec;
    return result;
}

static time_t __mktime(struct tm *tp)
{
    time_t ret;
    time_t jdn;

    /* Get the EPOCH-relative julian date from the calendar year,
     * month, and date
     */

    ret = __check_tm_ok(tp);
    if (ret < 0) {
        return -1;
    }

    jdn = __clock_calendar2utc(tp->tm_year + 1900, tp->tm_mon, tp->tm_mday);

    /* Return the seconds into the julian day. */

    ret = ((jdn * 24 + tp->tm_hour) * 60 + tp->tm_min) * 60 + tp->tm_sec;

    return ret;
}

static int32_t clock_check_tm_ok(const struct tm *rtctime)
{
    int32_t ret = 0;

    do {

        /**
         * First check whether the regular date is legal
        */
        if ((rtctime->tm_sec  > RTC_TIME_MAX_VAL_SEC)  ||  \
            (rtctime->tm_min  > RTC_TIME_MAX_VAL_MIN)  ||  \
            (rtctime->tm_hour > RTC_TIME_MAX_VAL_HOUR) ||  \
            (rtctime->tm_mon  > RTC_TIME_MAX_VAL_MON)  ||  \
            ((rtctime->tm_year > RTC_TIME_MAX_VAL_YEAR)  || (rtctime->tm_year < RTC_TIME_MIN_VAL_YEAR))) {
            ret = -1;
            break;
        }

        /**
         * Second check whether the day less than the minimum
        */
        if (rtctime->tm_mday < RTC_TIME_MIN_VAL_DAY) {
            ret = -1;
            break;
        }

        /**
         * Third check whether the day more than the maximum(based on whether the year is a leap year or not)
        */
        if (RTC_IS_LEAPYEAR(rtctime->tm_year + RTC_TIME_BASE_YEAR)) {
            if (rtctime->tm_mday > leap_year[rtctime->tm_mon]) {
                ret = -1;
            }
        } else {
            if (rtctime->tm_mday > noleap_year[rtctime->tm_mon]) {
                ret = -1;
            }
        }

    } while (0);

    return ret;
}


static uint64_t rtc_get_counter(void)
{
    uint32_t valLow, valHigh;

    HBN_Get_RTC_Timer_Val(&valLow, &valHigh);

    return ((uint64_t)valHigh << 32) | valLow;
}

static uint64_t rtc_get_compare_counter(void)
{
    uint32_t valLow, valHigh;

    valHigh = BL_RD_REG(HBN_BASE, HBN_TIME_H);
    valLow = BL_RD_REG(HBN_BASE, HBN_TIME_L);

    return ((uint64_t)valHigh << 32) | valLow;
}

static uint64_t rtc_get_timestamp_ms(void)
{
    uint64_t cnt;

    cnt = rtc_get_counter();

    return BL_RTC_COUNTER_TO_MS(cnt);
}

/**
  \brief       Internal timeout interrupt process function
  \param[in]   rtc    handle rtc handle to operate
  \return      None
*/
void rtc_irq_handler(void *arg)
{
    csi_dev_t *pdev = (csi_dev_t *)arg;
    csi_rtc_t *prtc;

    prtc = (csi_rtc_t*)utils_container_of(pdev, csi_rtc_t, dev);

    if (SET == HBN_Get_INT_State(HBN_INT_RTC)) {
        if (prtc->callback) {
            prtc->callback(prtc, prtc->arg);
        }
        //HBN_Clear_IRQ(HBN_INT_RTC);
        HBN_Clear_RTC_INT();
    }
}

/**
  \brief       Initialize RTC Interface. Initializes the resources needed for the RTC interface
  \param[in]   rtc    rtc handle to operate
  \param[in]   idx    rtc index
  \return      error code \ref csi_error_t
*/
csi_error_t csi_rtc_init(csi_rtc_t *rtc, uint32_t idx)
{
    CSI_PARAM_CHK(rtc, CSI_ERROR);
    csi_error_t ret = CSI_OK;

    if(idx != 0) {
        return CSI_ERROR;
    }

    if (s_rtc_base) {
        //already init, return success
        return ret;
    }

    s_rtc_base = malloc(sizeof(struct tm));
    if (NULL == s_rtc_base) {
        return CSI_ERROR;
    }

#ifdef CFG_USE_XTAL32K
    HBN_32K_Sel(HBN_32K_XTAL);
#else
    HBN_32K_Sel(HBN_32K_RC);
#endif

    HBN_Clear_RTC_Counter();
    HBN_Enable_RTC_Counter();

    return ret;
}

/**
  \brief       De-initialize RTC Interface. stops operation and releases the software resources used by the interface
  \param[in]   rtc    rtc handle to operate
  \return      None
*/
void csi_rtc_uninit(csi_rtc_t *rtc)
{
    CSI_PARAM_CHK_NORETVAL(rtc);

    return;
}

/**
  \brief       Set system date
  \param[in]   rtc        handle rtc handle to operate
  \param[in]   rtctime    pointer to rtc time
  \return      error code \ref csi_error_t
*/
csi_error_t csi_rtc_set_time(csi_rtc_t *rtc, const csi_rtc_time_t *rtctime)
{
    CSI_PARAM_CHK(rtc, CSI_ERROR);
    CSI_PARAM_CHK(rtctime, CSI_ERROR);
    csi_error_t ret = CSI_OK;
    struct tm tim;

     __rtctime_to_tm(&tim, rtctime, RTC_TIME_IS_BCD);
     if ((ret = __check_tm_ok(&tim)) != 0) {
         return ret;
     }

     __rtctime_to_tm(s_rtc_base, rtctime, RTC_TIME_IS_BCD);

    HBN_Clear_RTC_Counter();
    HBN_Enable_RTC_Counter();

    return ret;
}
/**
  \brief       Set system date but no wait
  \param[in]   rtc        rtc handle to operate
  \param[in]   rtctime    pointer to rtc time
  \return      error code \ref csi_error_t
*/
csi_error_t csi_rtc_set_time_no_wait(csi_rtc_t *rtc, const csi_rtc_time_t *rtctime)
{
    CSI_PARAM_CHK(rtc, CSI_ERROR);
    CSI_PARAM_CHK(rtctime, CSI_ERROR);
    csi_error_t ret = CSI_OK;
    struct tm tim;

    __rtctime_to_tm(&tim, rtctime, RTC_TIME_IS_BCD);
     if ((ret = __check_tm_ok(&tim)) != 0) {
         return ret;
     }

     __rtctime_to_tm(s_rtc_base, rtctime, RTC_TIME_IS_BCD);

    HBN_Clear_RTC_Counter();
    HBN_Enable_RTC_Counter();

    return ret;
}
/**
  \brief       Get system date
  \param[in]   rtc        handle rtc handle to operate
  \param[out]  rtctime    pointer to rtc time
  \return      error code \ref csi_error_t
*/
csi_error_t csi_rtc_get_time(csi_rtc_t *rtc, csi_rtc_time_t *rtctime)
{
    CSI_PARAM_CHK(rtc, CSI_ERROR);
    CSI_PARAM_CHK(rtctime, CSI_ERROR);
    struct tm tim;
    uint64_t time_stamp_ms = rtc_get_timestamp_ms();

    memset(&tim, 0, sizeof(struct tm));
    if (rtctime == NULL || rtc == NULL) {
        return -1;
    }

    time_stamp_ms = time_stamp_ms / 1000;
    time_stamp_ms += __mktime(s_rtc_base);
    __gmtime_r((const time_t *)&time_stamp_ms, &tim);

    __tm_to_rtctime(rtctime, &tim, RTC_TIME_IS_BCD);

    return CSI_OK;
}

/**
  \brief       Get alarm remaining time
  \param[in]   rtc    rtc handle to operate
  \return      the remaining time(s)
*/
uint32_t csi_rtc_get_alarm_remaining_time(csi_rtc_t *rtc)
{
    CSI_PARAM_CHK(rtc, 0U);
    uint64_t current_counter, compare_counter, remain;

    current_counter = rtc_get_counter();
    compare_counter = rtc_get_compare_counter();

    if (current_counter > compare_counter) {
        return 0;
    }

    remain = (compare_counter - current_counter) / RTC_SEC_TO_TICK;

    return (uint32_t)remain;
}

/**
  \brief       Config RTC alarm ture timer
  \param[in]   rtc         handle rtc handle to operate
  \param[in]   rtctime     time(s) to wake up
  \param[in]   callback    callback function
  \param[in]   arg         callback's param
  \return      error code \ref csi_error_t
*/
csi_error_t csi_rtc_set_alarm(csi_rtc_t *rtc, const csi_rtc_time_t *rtctime, void *callback, void *arg)
{
    CSI_PARAM_CHK(rtc, CSI_ERROR);
    CSI_PARAM_CHK(rtctime, CSI_ERROR);
    csi_error_t ret = (csi_error_t)clock_check_tm_ok((const struct tm *)rtctime);
    csi_rtc_time_t current_time;
    uint64_t settime;
    uint32_t lowval, highval;

    if (CSI_OK == ret) {
        rtc->callback = callback;
        rtc->arg = arg;
        csi_irq_attach(HBN_OUT0_IRQn, &rtc_irq_handler, &rtc->dev);
        csi_irq_enable(HBN_OUT0_IRQn);

        csi_rtc_get_time(rtc, &current_time);            ///< get current time
        settime = (uint64_t)((__mktime((struct tm *)rtctime) - __mktime((struct tm *)&current_time)) * RTC_SEC_TO_TICK);
        settime = settime + rtc_get_counter();
        lowval = (uint32_t)settime;
        highval = (uint32_t)(settime >> 32);
        HBN_Set_RTC_Timer(HBN_RTC_INT_DELAY_0T, lowval, highval, HBN_RTC_COMP_BIT0_39);
    }

    return ret;
}

/**
  \brief       Cancel the rtc alarm
  \param[in]   rtc    rtc handle to operate
  \return      error code \ref csi_error_t
*/
csi_error_t csi_rtc_cancel_alarm(csi_rtc_t *rtc)
{
    CSI_PARAM_CHK(rtc, CSI_ERROR);

    rtc->callback = NULL;
    rtc->arg = NULL;
    csi_irq_disable(HBN_OUT0_IRQn);
    csi_irq_detach(HBN_OUT0_IRQn);

    return CSI_OK;
}

/**
  \brief       Judge rtc is working
  \param[in]   rtc    handle rtc handle to operate
  \return      state of work
               ture - rtc is running
               false -rtc is not running
*/
bool csi_rtc_is_running(csi_rtc_t *rtc)
{
    CSI_PARAM_CHK(rtc, false);
    uint32_t tmpVal;

    tmpVal = BL_RD_REG(HBN_BASE, HBN_CTL);

    return (tmpVal & 0x1);
}
