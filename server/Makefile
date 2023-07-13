CC:=gcc
path_c:=./src/
path_o:=./bin/
srcs:=$(wildcard $(path_c)*.c)
objs:=$(srcs:$(path_c)%.c=$(path_o)%.o)
final:=NetDiskServer
install:$(final)
$(final):$(objs)
	$(CC) $^ -o $@ -pthread -lmysqlclient
$(path_o)%.o:$(path_c)%.c
	$(CC) -c $^ -o $@ -I ./head
.PHONY:rebuild clean
clean:
	$(RM) $(objs) $(final)
rebuild:clean install 
