/*
 * Copyright (C) 2022 Alibaba Group Holding Limited
 */
#include <board.h>

typedef struct _button_gpio_conf_ {
    int                 button_id;    /*物理按键ID*/
    int                 gpio_id;      /*引脚号*/
    button_gpio_level_t active_level; /*按键按下后的电平*/
} button_gpio_conf_t; 

typedef struct _button_gpio_event_conf_ {
    int                 button_id1; /*物理按键ID*/
    int                 button_id2; /*若是组合键，定义第二个物理按键ID，*/
    int                 keymsg_id;  /*按键消息，对应具体的执行ID*/
    button_press_type_t press_type;   /*按键类型，长按、短按*/
    int                 press_time; /*若是长按，填写持续的时间*/
} button_gpio_event_conf_t;


/*物理按键定义：GPIO 列请填写芯片对应的gpio引脚，引脚名称参考soc.h.还需确认引脚的复用关系*/
static button_gpio_conf_t button_gpio_confs[] = {
     {
        BUTTON_ID_VOL_UP,
        11 /*GPIO*/,
        LOW_LEVEL,
    },
    {
        BUTTON_ID_VOL_DOWN,
        5 /*GPIO*/,
        LOW_LEVEL,
    }, 
    {
        BUTTON_ID_PLAY,
        24 /*GPIO*/,
        HIGH_LEVEL,
    },
    {
        BUTTON_ID_USER,
        19 /*GPIO*/,
        LOW_LEVEL,
    },
    
    #if 0
    {
        BUTTON_ID_SONG_UP,
        3 /*GPIO*/,
        LOW_LEVEL,
    },
    {
        BUTTON_ID_SONG_DOWN,
        4 /*GPIO*/,
        LOW_LEVEL,
    },
    #endif
};

/*按键事件定义*/
static button_gpio_event_conf_t button_event_confs[] = {
    {
        BUTTON_ID_VOL_UP,
        BUTTON_ID_NULL,
        KEY_MSG_VOL_UP,
        BUTTON_PRESS_UP/*短按键*/,
        0 /*ms*/,
    },
    {
        BUTTON_ID_VOL_UP,
        BUTTON_ID_NULL,
        KEY_MSG_VOL_UP_0,
        BUTTON_PRESS_DOWN,
        0 /*ms*/,
    },
    {
        BUTTON_ID_VOL_DOWN,
        BUTTON_ID_NULL,
        KEY_MSG_VOL_DOWN,
        BUTTON_PRESS_UP/*短按键*/,
        0 /*ms*/,
    },
    {
        BUTTON_ID_VOL_DOWN,
        BUTTON_ID_NULL,
        KEY_MSG_VOL_DOWN_0,
        BUTTON_PRESS_DOWN,
        0 /*ms*/,
    },
    {
        BUTTON_ID_PLAY,
        BUTTON_ID_NULL,
        KEY_MSG_PLAY,
        BUTTON_PRESS_UP /*短按键*/,
        0 /*ms*/,
    },
    {
        BUTTON_ID_USER,
        BUTTON_ID_NULL,
        KEY_MSG_FACTORY,
        BUTTON_PRESS_LONG_DOWN /*长按键*/,
        6000 /*ms*/,
    },
    {
        BUTTON_ID_USER,
        BUTTON_ID_NULL,
        KEY_MSG_USER,
        BUTTON_PRESS_UP /*长按键*/,
        0 /*ms*/,
    },
    #if 0
    {
        BUTTON_ID_SONG_UP,
        BUTTON_ID_NULL,
        KEY_MSG_SONG_UP,
        BUTTON_PRESS_UP /*短按键*/,
        0 /*ms*/,
    },
    {
        BUTTON_ID_SONG_DOWN,
        BUTTON_ID_NULL,
        KEY_MSG_SONG_DOWN,
        BUTTON_PRESS_UP /*短按键*/,
        0 /*ms*/,
    },
    {
        BUTTON_ID_VOL_UP,
        NULL,
        KEY_MSG_FACTORY,
        BUTTON_PRESS_LONG_DOWN /*长按键*/,
        2000 /*ms*/,
    },
    {
        BUTTON_ID_WAKEUP,
        BUTTON_ID_VOL_DOWN,
        KEY_MSG_WIFI_PROV,
        BUTTON_PRESS_LONG_DOWN /*组合键，固定长按键*/,
        5000 /*ms*/,
    },
    #endif
};

void board_button_init(button_evt_cb_t keymsg_cb)
{
    button_init();

    /*添加物理按键*/
    for (int i = 0; i < sizeof(button_gpio_confs) / sizeof(button_gpio_conf_t); i++) {
        button_add_gpio(
            button_gpio_confs[i].button_id, button_gpio_confs[i].gpio_id, button_gpio_confs[i].active_level);
    }

    /*添加事件*/
    for (int i = 0; i < sizeof(button_event_confs) / sizeof(button_gpio_event_conf_t); i++) {
        if (button_event_confs[i].button_id2 == BUTTON_ID_NULL) {
            /*单按键*/
            button_evt_t b_tbl[] = { { .button_id = 0, .press_type = 0, .press_time = 0 } };
            b_tbl[0].button_id   = button_event_confs[i].button_id1;
            b_tbl[0].press_type    = button_event_confs[i].press_type;
            printf("####b_tbl[0].press_type:%d, button_id:%d, i:%d\r\n", b_tbl[0].press_type, b_tbl[0].button_id, i);
            b_tbl[0].press_time  = button_event_confs[i].press_time;
            button_add_event(
                button_event_confs[i].keymsg_id, b_tbl, sizeof(b_tbl) / sizeof(button_evt_t), keymsg_cb, "");
        } else {
            /*组合键*/
            button_evt_t b_com_tbl[] = { { .press_type = BUTTON_PRESS_LONG_DOWN, .button_id = 0, .press_time = 5000 },
                                         { .press_type = BUTTON_PRESS_LONG_DOWN, .button_id = 0, .press_time = 5000 } };
            b_com_tbl[0].button_id   = button_event_confs[i].button_id1;
            b_com_tbl[0].press_time  = button_event_confs[i].press_time;
            b_com_tbl[1].button_id   = button_event_confs[i].button_id2;
            b_com_tbl[1].press_time  = button_event_confs[i].press_time;
            button_add_event(button_event_confs[i].keymsg_id, b_com_tbl, sizeof(b_com_tbl) / sizeof(button_evt_t), keymsg_cb, "");
        }
    }
}
