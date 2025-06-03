#include <cli.h>
#include <string.h>
#include <stdlib.h>
#include "mfg_cli.h"
#include "co_hci.h"
#include "rwip.h"

bool bReset = false;
#define MFG_TX_POWER_MAX 20
#define MFG_TX_POWER_MIN -10
extern int hci_le_rx_test_v2_cmd_handler(struct hci_le_rx_test_v2_cmd const *param, uint16_t opcode,bool from_hci);
extern int hci_le_tx_test_v2_cmd_handler(struct hci_le_tx_test_v2_cmd const *param, uint16_t opcode,bool from_hci);
extern int hci_le_rx_test_v3_cmd_handler(struct hci_le_rx_test_v3_cmd const *param, uint16_t opcode,bool from_hci);
extern int hci_le_tx_test_v3_cmd_handler(struct hci_le_tx_test_v3_cmd const *param, uint16_t opcode,bool from_hci);
extern int hci_le_tx_test_v4_cmd_handler(struct hci_le_tx_test_v4_cmd const *param, uint16_t opcode,bool from_hci);
extern int hci_le_test_end_cmd_handler(void const *param, uint16_t opcode,bool from_hci);

void cmd_le_rx_test(char *buf, int len, int argc, char **argv)
{
    int status = 0;

    if(!bReset)
    {
        bReset = true;
        rwip_reset();
    }

    if(argc == 4)
    {
        struct hci_le_rx_test_v2_cmd param;
        param.rx_channel = (uint8_t)strtol(argv[1],NULL,16);
        param.phy = (uint8_t)strtol(argv[2],NULL,16);
        param.mod_idx = (uint8_t)strtol(argv[3],NULL,16);
        status = hci_le_rx_test_v2_cmd_handler((struct hci_le_rx_test_v2_cmd const *)&param, 0, false);    
    }
    #if 0
    else if(argc == 9)
    {
        struct hci_le_rx_test_v3_cmd param;
        param.rx_channel = (uint8_t)strtol(argv[1],NULL,16);
        param.phy = (uint8_t)strtol(argv[2],NULL,16);
        param.mod_idx = (uint8_t)strtol(argv[3],NULL,16);
        param.exp_cte_len = (uint8_t)strtol(argv[4],NULL,16);
        param.exp_cte_type = (uint8_t)strtol(argv[5],NULL,16);
        param.slot_dur = (uint8_t)strtol(argv[6],NULL,16);
        param.switching_pattern_len = (uint8_t)strtol(argv[7],NULL,16);
        memcpy(param.antenna_id, &argv[8], param.switching_pattern_len);
        status = hci_le_rx_test_v3_cmd_handler((struct hci_le_rx_test_v3_cmd const *)&param, 0);
    } 
    #endif
    else
    {
        logprintf("Wrong number of args\n");
        return;
    }

    if(status)
    {
        logprintf("le rx test fails to start (err:0x%x)\r\n", status);
    }
    else
    {
        logprintf("le rx test starts successfully\r\n");
    }
}

void cmd_le_tx_test(char *buf, int len, int argc, char **argv)
{
    int status = 0;

    if(!bReset)
    {
        bReset = true;
        rwip_reset();
    }

    if(argc == 6)
    {
        struct hci_le_tx_test_v4_cmd param;
        memset(&param, 0, sizeof(struct hci_le_tx_test_v4_cmd));
        param.tx_channel = (uint8_t)strtol(argv[1], NULL, 16);
        param.test_data_len = (uint8_t)strtol(argv[2], NULL, 16);
        param.pkt_payl = (uint8_t)strtol(argv[3], NULL, 16);
        param.phy = (uint8_t)strtol(argv[4], NULL, 16);
        param.tx_pwr_lvl = (uint8_t)strtol(argv[5], NULL, 16);
        param.cte_len = 0;
        param.switching_pattern_len = 2;//set switching_pattern_len to make it can pass the check in hci_le_tx_test_v4_cmd_handler.  
        status = hci_le_tx_test_v4_cmd_handler((struct hci_le_tx_test_v4_cmd const *)&param, 0, false);
    }
    else
    {
        logprintf("Wrong number of args\n");
        return;
    }
    
    if(status)
    { 
        logprintf("le tx test fails to start (err:0x%x)\r\n", status);
    }
    else
    {
        logprintf("le tx test starts successfully\r\n");
    }
}

void cmd_le_test_stop(char *buf, int len, int argc, char **argv)
{
        uint8_t status;
       
        status = hci_le_test_end_cmd_handler(NULL, 0, false);
        if(status)
        {
            logprintf("le test stop fails (err:0x%x)\r\n", status);
        }
}

// STATIC_CLI_CMD_ATTRIBUTE makes this(these) command(s) static
const static struct cli_command cmds_bt_mfg[] STATIC_CLI_CMD_ATTRIBUTE = {
    { "le_rx_test", "do le rx test v1", cmd_le_rx_test},
    { "le_tx_test", "do le tx test v1", cmd_le_tx_test},
    { "le_test_stop", "stop le test", cmd_le_test_stop},
};

int bt_mfg_cli_register(void)
{
    // static command(s) do NOT need to call aos_cli_register_command(s) to register.
    // However, calling aos_cli_register_command(s) here is OK but is of no effect as cmds_user are included in cmds list.
    // XXX NOTE: Calling this *empty* function is necessary to make cmds_user in this file to be kept in the final link.
    //return aos_cli_register_commands(cmds_user, sizeof(cmds_user)/sizeof(cmds_user[0]));
    return 0;
}
