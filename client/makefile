PATH_C:=./src/
PATH_O:=./obj/
LIST:=$(wildcard $(PATH_C)*.c)		#LIST = "./src/main.c ./src/add.c ..."
OBJ:=$(LIST:$(PATH_C)%.c=$(PATH_O)%.o)		#OBJ = "./src/main.o ...."	%表示要替换的字符串内容如果前边不加上$(PATH_C),就会替换"./src/main ...."
BIN:=$(LIST:$(PATH_C)%.c=%)   		#BIN = "main add mul ..."


main:$(OBJ)				#main的依赖文件是./src/main.o ....
	gcc $^ -o $@ -lpthread -lcrypt

all:$(BIN)
$(PATH_O)%.o:$(PATH_C)%.c		#%是从$(BIN)中进行查找替换的
	gcc -c $^ -o $@ 

.PHONY:rebuild clear
clear:
	rm main $(OBJ)
rebuild:clear main


