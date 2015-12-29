TARGETS = client server
C_SV = dhcpd.c queue.c
C_CL = dhcpc.c
C_COMMON = dhcp_common.c

all: $(TARGETS)

client: $(C_CL) $(C_COMMON)
	gcc $(C_CL) $(C_COMMON) -o client
server: $(C_SV) $(C_COMMON)
	gcc $(C_SV) $(C_COMMON) -o server
