#include "xtimer.h"
#include "bocia.h"
#include "unity.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"

int8_t run_test;

int8_t bocia_input_handler(bocia_channel_t *rd, proto_msg_t command_id,
        void *obj) {

    proto_msg_t ack_type = { ACK_TYPE, 1 };
    ack_type.dline = command_id.dline;

    Ack ack = ACK__INIT;

    ack.id = command_id.type;

    DEBUG("executing [%d,%d] message\n", command_id.type, command_id.flags);

    switch(command_id.type) {
        case LEDS_TYPE: {
            proto_msg_t head;
            head.type = LEDS_TYPE;
            head.dline = command_id.dline;

            Leds *leds = (Leds *) obj;
            if(leds->has_red) {
                if(leds->red == 1) {
                    gpio_set(LED_RED);
                }
                if(leds->red == 0) {
                    gpio_clear(LED_RED);
                }
            }
            if(leds->has_green) {
                if(leds->green == 1) {
                    gpio_set(LED_GREEN);
                }
                if(leds->green == 0) {
                    gpio_clear(LED_GREEN);
                }
            }
            if(leds->has_yellow) {
                if(leds->yellow == 1) {
                    gpio_set(LED_YELLOW);
                }
                if(leds->yellow == 0) {
                    gpio_clear(LED_YELLOW);
                }
            }
            leds->red = gpio_read(LED_RED);
            leds->green = gpio_read(LED_GREEN);
            leds->yellow = gpio_read(LED_YELLOW);

            //protobuf_to_uart(NULL, head, leds);
            rd->send(rd, head, leds);
            break;
        }
        case COMMAND_TYPE: {
            Command *cmd = (Command *) obj;
            printf("command: %ld, seq: %ld\n", cmd->id, cmd->seq);
            bocia_command(rd, cmd);
            break;
        }
        case SECRET_TYPE: {
            Secret *secret = (Secret *) obj;
            set_ap_password(secret->key);
            cc3200_reset();
            break;
        }
        case CONFIG_TYPE: {
            /*
            if(is_open()) {
                return INPUTHANDLER_ERR;
            }
             */
            //msg_t msg;
            //msg.type = TCPSERVER_CONFIG_OK;
            //msg.content.ptr = (char *) obj;
            bocia_set_cfg((Config *) obj);

            //msg_send(&msg, rd->target_pid);
            rd->send(rd, ack_type, &ack);

            break;
        }
        case PROFILE_TYPE: {
            Profile *profile;
            profile = (Profile*) obj;
            DEBUG("ssid: %s - pwd: %s\n", profile->uid, profile->pwd);
            sbapi_add_profile(profile->uid, profile->pwd);
            rd->send(rd, ack_type, &ack);
            break;
        }
        default:
            DEBUG("recv unmanaged type [%d,%d]\n", command_id.type,
                    command_id.flags);

    }
    return INPUTHANDLER_OK;
}


#if 0
void setUp(void)
{
}

void tearDown(void)
{
}
#endif


/**
 * start the processor without connecting
 */
void test_delete_profile(void) {
    // start uart monitor
    bocia_uart_init();

	int sts = sbapi_init(SBAPI_DEFAULT_CFG);
	sbapi_delete_all_profiles();
	TEST_ASSERT(sts > 0);
	printf("all WLAN profiles deleted\n");

}


void test_start_AP(void)
{
	msg_t msg;
	msg_t msg_queue[SBAPI_MSG_QUEUE_SIZE];

	/* initialize message queue */
	msg_init_queue(msg_queue, SBAPI_MSG_QUEUE_SIZE);

	sbapi_init(SBAPI_DEFAULT_CFG);
	sbapi_set_mode(ROLE_STA);

	run_test = 1;
	while (run_test)
	{
		msg_receive(&msg);
		switch (msg.type)
		{
		case SBAPI_IP_ACQUIRED:
			// test event
			printf("IP acquired\n");
			run_test = 0;

			break;
		}
	}
}
