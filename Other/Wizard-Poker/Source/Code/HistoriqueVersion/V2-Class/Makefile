TARGET=client serveur

normal: $(TARGET)

client: client.cpp
	g++-5 -Wall client.cpp -o client

serveur: server.cpp
	g++-5 -Wall server.cpp -o server

clean:
	$(RM) $(TARGET)
