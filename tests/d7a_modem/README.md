## D7A modem on RIOT

This test demonstrates the [D7A](git@gitlab.cortus.com/CIoT_SDK/dash7-ap-open-source-stack) stack running on RIOT. When flashed, it will initialize a shell for interacting with the stack.

## Quick usage

To test D7A on RIOT, you can do the following:

1. Flash nodes with `make BOARD=<target> clean all flash`
2. 
3. 
4. 
5. 


## Note

Commands to communicate with the D7A Modem through an UART connection:
#Options in square bracket [] are not mandatory !!

- Send payload over D7A: d7a tx <my_payload> [qos] [timeout] [access_class] [addr] [resp_len]
- Get params from the D7A modem: d7a get <devaddr|class|tx_power>

