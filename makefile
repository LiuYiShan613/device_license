CC:=g++
exe:=license
obj:=license.o aes256.o

all:$(obj)
	$(CC) -o $(exe) $(obj)
	@echo "Established successfully !"
  
%.o:%.cpp
	$(CC) -c $^

clean:
	rm -rf $(exe) $(obj) 
