#include <iostream>
#include <string>
#include "net.h"
#include "chat.h"
#include <commandline/commandline.h>
int main()
{
	//asio::io_context context;
	//asio::error_code ec;
	//net::connection<int> test;
	//if (!test.Connect("127.0.0.1", 2137))
	//{
	//	test.Listen(2137, "127.0.0.1");
	//}
	//net::message<char> a;


	//bool testBool = false;
	//std::thread testThread(
	//	[&testBool]()
	//	{
	//		while (!testBool)
	//		{
	//			std::cout << "kon\n";
	//			Sleep(1000);
	//		}
	//		std::cout << "NO MORE KON\n";
	//	});
	//std::string s;
	//std::cin >> s;
	//testBool = true;
	//testThread.join();
	//std::cout << s;
	std::string targetIP, nick;
	uint32_t targetPort;
	std::cout << "Target IP: ";
	std::cin >> targetIP;
	std::cout << "Target port: ";
	std::cin >> targetPort;
	std::cout << "Nickname: ";
	std::cin.ignore();
	std::getline(std::cin, nick);
	Commandline console;
	chatFrame chat(targetIP, targetPort, nick);
	bool isRunning = true;
	chat.Run();
	while (isRunning)
	{
		std::string input, receivedText;
		while (chat.ReadText(receivedText))
		{
			console.write(chat.name + ":\t" + receivedText);
		}

		if (console.has_command())
		{
			input = console.get_command();
			
			if (input == "/exit")
			{
				console.write(input);
				isRunning = false;
			}
			else
			{
				console.write("You:\t" + input);
				chat.SendText(input);
			}
		}
	}
	chat.Stop();
	
}
