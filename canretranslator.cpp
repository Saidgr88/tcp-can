#include "canretranslator.h"
#include <poll.h>

TCANRetranslator::TCANRetranslator(int num, uint32_t bitrate) :
	  canNum(num)
	, brate(bitrate)
{
	memset(&frame, 0, sizeof(frame));
}

TCANRetranslator::TCANRetranslator(int num) :
	  canNum(num)
	, brate(100000)
{
	memset(&frame, 0, sizeof(frame));
}

TCANRetranslator::~TCANRetranslator()
{
	system(std::string("sudo ifconfig can" + std::to_string(canNum) + " down").data());
}

bool TCANRetranslator::systemCall(std::string cmd)
{
	if (system(cmd.data()))
	{
		std::cout << "SYSTEMCALL " + cmd + " ERROR"; return false;
	}
	else
	{
		std::cout << cmd << '\n'; return true;
	}
}


bool TCANRetranslator::startServer()
{
	std::string canName = "can" + std::to_string(canNum);
	std::string cmd = "sudo ip link set " + canName + " type can bitrate " + std::to_string(brate);
	
	if(!systemCall(cmd)) return false;

	cmd = "sudo ifconfig " + canName + " up";
	if(!systemCall(cmd)) return false;

	canSocket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (canSocket < 0)
	{
		std::cout << "socket PF_CAN ERROR\n";
		return false;
	}

	strcpy(ifr.ifr_name, canName.data());
	int res = ioctl(canSocket, SIOCGIFINDEX, &ifr);
	if (res < 0)
	{
		std::cout << "ioctl ERROR\n";
		return false;
	}

	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	res = bind(canSocket, (sockaddr*)&addr, sizeof(addr));
	if (res < 0)
	{
		std::cout << "bind ERROR\n";
		return false;
	}

	return true;
}

EOperationResult TCANRetranslator::printFrame(can_frame canFrame)
{
	//if (&canFrame == nullptr) return EOR_ERR;

	std::cout << "[can_id: " << canFrame.can_id << "] [";
	for (int i = 0; i < canFrame.can_dlc; i++)
		if (canFrame.data[i] > 0)
			std::cout << canFrame.data[i];
		else
			std::cout << " ";

	std::cout << "]\n";

	return EOR_Ok;
}

EOperationResult TCANRetranslator::sendPackage(can_frame inFrame)
{
	return sendPackage(inFrame.can_id, inFrame.data, inFrame.can_dlc);
}

EOperationResult TCANRetranslator::sendPackage(uint32_t canId, uint8_t* data, uint8_t size)
{
	if (size > 8)		 return EOR_ERR;
	if (data == nullptr) return EOR_ERR;

	frame.can_id  = canId;
	frame.can_dlc = size;

	for (int i = 0; i < size; i++)
		frame.data[i] = data[i];

	if (write(canSocket, &frame, sizeof(frame)) != sizeof(frame))
		return EOR_ERR;

	return EOR_Ok;
}

EOperationResult TCANRetranslator::receivePackage(can_frame& outFrame, uint32_t msTimeout)
{
	return receivePackage(outFrame.can_id, outFrame.data, outFrame.can_dlc, outFrame.can_dlc, msTimeout);
}

EOperationResult TCANRetranslator::receivePackage(uint32_t &canId, uint8_t* data, uint8_t size, uint8_t &canDlc, uint32_t msTimeout)
{
	if (size < 1)		 return EOR_ERR;
	if (data == nullptr) return EOR_ERR;

	pollfd pfds[1];
	
	pfds[0].fd = canSocket; // Some socket descriptor
	pfds[0].events = POLLIN;  // Tell me when ready to read

	int gotPoll = poll(pfds, 1, msTimeout);
	if (gotPoll == 0) return EOR_TimeOut;

	int start = clock();

	while (1)
	{
		//std::cout << "start reading\n";
		int nbytes = read(canSocket, &frame, sizeof(frame));
		if (nbytes > 0)
		{
			canId  = frame.can_id;
			canDlc = frame.can_dlc;
			for (int i = 0; i < size; i++)
				data[i] = frame.data[i];

			break;
		}
		else if (clock() - start > msTimeout)
		{
			return EOR_TimeOut;
		}
		//std::cout << "can i reach this point?\n";
	}


	return EOR_Ok;
}



//==============================================================
//======================  TCP SOCKET  ==========================
//==============================================================


TSocket::TSocket(int port) :
	portNum(port)
{
	//memset(buffer, 0, BUFFER_SIZE - 1);
	buffer.resize(BUFFER_SIZE);
	std::cout << "buffer.size = " << buffer.size() << '\n';
}

TSocket::~TSocket()
{
	close(newSock);
	close(tcpSocket);
}


bool TSocket::startServer()
{
	tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (tcpSocket < 0)
		return false;

	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = INADDR_ANY;
	servAddr.sin_port = htons(portNum);

	if (bind(tcpSocket, (sockaddr*)&servAddr, sizeof(servAddr)) < 0)
		return false;

	if (listen(tcpSocket, 5) < 0)
	{
		std::cout << "ERROR LISTEN\n";
	}
	cliLen = sizeof(cliAddr);

	newSock = accept(tcpSocket, (sockaddr*)&cliAddr, &cliLen);
	if (newSock < 0)
		return false;


	return true;
}

void TSocket::connect()
{
	
}

EOperationResult TSocket::sendPackage(const std::string& data)
{
	char tmp[data.size()];
	for (int i = 0; i < data.size(); i++)
		tmp[i] = data[i];

	return sendPackage(tmp, data.size());
}

EOperationResult TSocket::sendPackage(char* data, uint8_t size)
{
	if (data == nullptr) return EOR_ERR;
	if (size == 0) return EOR_ERR;

	int nbytes = write(newSock, data, size);
	if (nbytes < 0) return EOR_ERR;

	return EOR_Ok;
}

EOperationResult TSocket::receivePackage(char* data, uint8_t size)
{
	if (data == nullptr) return EOR_ERR;
	if (size == 0) return EOR_ERR;

	//buffer.clear();
	std::cout << "in receive\n";
	while (1)
	{
		int nbytes = read(newSock, &buffer[0], BUFFER_SIZE - 1);
		if (nbytes < 0)
		{
			std::cout << "GOT ERROR     size = " << buffer.size() << "\n";
			return EOR_ERR;
		}
		else if (nbytes > 0)
		{
			for (int i = 0; i < size; i++)
				data[i] = buffer[i];

			return EOR_Ok;
		}

	}
		//close(newSock);


	return EOR_Ok;
}

void TSocket::printBuffer(char *data, uint8_t size)
{
	std::cout << "buffer.size = " << buffer.size() << '\n';
	for (int i = 0; i < buffer.size(); i++)
	{
		if (buffer[i] != 0)
			std::cout << buffer[i] << " ";
	}

	std::cout << '\n';
}

