#ifndef CANRETRANSLATOR_H
#define CANRETRANSLATOR_H

#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <netinet/in.h>
#include <netdb.h>
#include <vector>

#define BUFFER_SIZE 256

enum EOperationResult
{
	EOR_Ok,
	EOR_ERR,
	EOR_TimeOut
};


class TSocket
{
public:
	TSocket() : portNum(2021) { buffer.resize(BUFFER_SIZE); }
	TSocket(int port);

	bool startServer();
	void connect();

	EOperationResult sendPackage	(const std::string& data);
	EOperationResult sendPackage   (char* data, uint8_t size);
	EOperationResult receivePackage(char* data, uint8_t size);

	void printBuffer(char* data, uint8_t size);

	~TSocket();

private:
	int portNum;
	int tcpSocket;
	int newSock;

	socklen_t cliLen;
	sockaddr_in servAddr;
	sockaddr_in cliAddr;

	std::vector<char> buffer;
	//char buffer[BUFFER_SIZE];
};

class TCANRetranslator
{
public:
	TCANRetranslator() : canNum(0) {}
	TCANRetranslator(int num, uint32_t bitrate);
	TCANRetranslator(int num);

	bool startServer();

	uint32_t getCanId() const { return frame.can_id;   }
	uint32_t getCanSz() const { return frame.can_dlc;  }

	EOperationResult printFrame(can_frame canFrame);

	EOperationResult	sendPackage(can_frame inFrame);
	EOperationResult	sendPackage(uint32_t  canId, uint8_t* data, uint8_t size);
	EOperationResult receivePackage(can_frame& outFrame, uint32_t msTimeout);
	EOperationResult receivePackage(uint32_t &canId, uint8_t* data, uint8_t size, uint8_t& canDlc, uint32_t msTimeout);

	~TCANRetranslator();
private:
	bool systemCall(std::string cmd);

	uint32_t brate;
	int canNum;
	int canSocket;
	//int tcpSocket;
	sockaddr_can addr;
	ifreq ifr;
	can_frame frame;
};

#endif//CANRETRANSLATOR_H
