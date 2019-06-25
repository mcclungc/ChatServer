#!/usr/bin/env python3 
#Connie McClung 
#CS372 Fall 2018
#Project 1
#chatserve.py
#This program instantiates a chat server to listen on specified port, accepting
#serial client connections until supervisor terminates program with SIGINT

# import modules
import socket
import sys
import signal

#organize chat server data and methods with chatServer class
class chatServer():
	
	#initialize chatServer object, create socket, bind, listen
	def __init__(self, port):
		host = None
		self.serverSocket = None
		self.serverHandle = input("Enter server handle: ")
		self.serverHandle = self.serverHandle + "> "
		# create socket, bind to first workable tuple returned by getaddrinfo(), and listen for connection
		# uses concepts and modeled after sample code explained at https://docs.python.org/3/library/socket.html
		for status in socket.getaddrinfo(host, port, socket.AF_UNSPEC, socket.SOCK_STREAM, 0, socket.AI_PASSIVE):
			addressFamily, socketType, protocolNumber, canonname, socketAddress = status
			try:
				self.serverSocket = socket.socket(addressFamily, socketType, protocolNumber)
			except OSError as errorMsg:
				self.serverSocket = None
				continue
			#after socket is successfully created, bind to socket and listen for connections
			try:
				self.serverSocket.bind(socketAddress)
				self.serverSocket.listen(1)
			except OSError as errorMsg:
				self.serverSocket.close()
				self.serverSocket = None
				continue
			break
			if self.serverSocket == None:
				print("Could not open server socket")
				sys.exit(1)
	
	# accept client connection
	def connect(self, port):
		print("listening on port "+str(port)+"...\n")
		clientConnection, clientAddress = self.serverSocket.accept()
		return clientConnection,clientAddress
	
	# with connected client, send and receive messages until server or client issues \quit request
	def handleComm(self, clientConnection, clientAddress):
		with clientConnection:
			print("Client connection from: " + str(clientAddress))
			#uses some concepts from socket TCPServer.py Chapter 2 of Computer Networking: A Top-Down Approach
			while True: 
				data = clientConnection.recv(500) #receive client message
				dataString = data.decode()
				#check for quit request from client
				if dataString[-5:] == "\quit":
					print("Client sent quit request - closing client connection...\n")
					break
				if not data:
					break
				print(data.decode())
				response = input(self.serverHandle) 
				response = self.serverHandle + response
				# ensure all data is sent
				# using concepts explained at https://docs.python.org/3/howto/sockets.html#socket-howto
				totalBytesSent = 0
				bytesLeft = len(response.encode())
				while totalBytesSent < len(response.encode()):
					bytesSent = clientConnection.send(response.encode()[totalBytesSent:])
					if bytesSent == 0:
						raise RuntimeError("Socket connection broken")
					totalBytesSent += bytesSent
				if response[-5:] == "\quit":
					print("Server issued quit request - closing client connection...\n")
					break
	
#  signal handler to exit program gracefully if SIGINT received
def sigintHandler(SIGINT, frame):
		print("\nSig Int received! Exiting program...\n")
		sys.exit(0)

#main: parse command line arguments, create server, and accept client connections for chat until SIGINT received		
def main():	
	#validate commandline args and assign to variables
	if len(sys.argv) != 2:
		print("Usage: "+ sys.argv[0] + " portnumber")
	port = int(sys.argv[1])

	#instantiate server object and loop to accept connections and chat
	myChatServer = chatServer(port)
	signal.signal(signal.SIGINT, sigintHandler)	
	while True:
		clientConnection, clientAddress = myChatServer.connect(port)
		myChatServer.handleComm(clientConnection, clientAddress)

	

# call the main() function to begin the program
if __name__ == '__main__':
    main()
	
	
