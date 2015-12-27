TARGETS = client server
C_SV = dhcpd.c
C_CL = dhcpc.c

all: $(TARGETS)

client: $(C_CL)
	gcc $(C_CL) -o client
server: $(C_SV)
	gcc $(C_SV) -o server
