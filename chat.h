#pragma once
#include "net.h"
#include "threadsafe.h"
#include <string>

class chatFrame
{
public:
	const std::string IP, name;
	const int16_t port;
	enum class MessageType
	{
		TEXT
	};
	chatFrame(std::string _IP, uint16_t _port, std::string name = "");
	bool RunConnect();
	void RunListen();
	void Run();
	void SendText(const std::string& txt);
	bool ReadText(std::string& txt);
	void Stop();
protected:
	asio::io_context context;
	std::thread contextThread;
	net::connection<MessageType> target;
};

chatFrame::chatFrame(std::string _IP, uint16_t _port, std::string _name): IP(_IP), port(_port), target(context), name(_name == "" ? _IP : _name)
{

}
bool chatFrame::RunConnect()
{
	bool didConnect;
	try
	{
		didConnect = target.Connect(IP, port);
		if (didConnect)
		{
			contextThread = std::thread([this]() { context.run(); });
			std::cout << "\nChat started!\n";
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "\nChat exception: " << e.what() << "\n";
		return false;
	}
	
	return didConnect;
}
void chatFrame::RunListen()
{
	try
	{
		target.Listen(port, IP);
		contextThread = std::thread([this]() { context.run(); });
	}
	catch (std::exception& e)
	{
		std::cerr << "\nChat exception: " << e.what() << "\n";
		return;
	}
	std::cout << "\nChat started!\n";
}
void chatFrame::Run()
{
	if (!RunConnect())
	{
		RunListen();
	}
}
void chatFrame::SendText(const std::string& txt)
{
	net::message<MessageType> msg;
	msg.body.resize(txt.length());
	msg.header.msgBodySize = msg.body.size();
	std::memcpy(msg.body.data(), txt.c_str(), txt.length());
	target.Send(msg);
}
bool chatFrame::ReadText(std::string& txt)
{
	if (target.received.empty())
	{
		txt = "";
		return false;
	}
	net::message<MessageType> msg = target.received.pop();;
	size_t txtS = msg.header.msgBodySize;
	txt.assign(msg.body.begin(), msg.body.end());
	return true;
}
void chatFrame::Stop()
{
	target.Disconect();
	contextThread.join();
}
