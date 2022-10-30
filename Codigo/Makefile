all: offload_simulator mobile_node

offload_simulator: main.o func.o
	gcc -Wall -pthread main.o func.o -o offload_simulator

mobile_node: mobileNode.o
	gcc -Wall -pthread mobileNode.o -o mobile_node

main.o: main.c func.h
	gcc -c main.c

func.o: func.c func.h
	gcc -c func.c

mobileNode.o: mobileNode.c func.h
	gcc -c mobileNode.c