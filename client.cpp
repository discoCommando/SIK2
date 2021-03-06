#include <boost/program_options.hpp>

#include <iostream>
#include <sstream> 
#include "client.hpp"


namespace po = boost::program_options;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;
const unsigned int RETRANSMIT_LIMIT = 10;

bool debug = false;
unsigned int retransmitLimit;
unsigned short port;
std::string server;
boost::asio::io_service ioService;

// Client::Client(boost::asio::ip::basic_resolver_iterator< boost::asio::ip::tcp > endpointTCPIterator, 
// 							 boost::asio::ip::udp::endpoint endpointUDP):
// 							 mySocket_(ioService),
// 							 tcpEndpointIterator_(endpointTCPIterator),
// 							 udpEndpoint_(endpointUDP),
// 							 udpSocket_(ioService, udpEndpoint_)
// {
// 	initTCP();
// 	//initUDP();
// }

Client::Client(boost::asio::ip::basic_resolver_iterator< tcp > &endpointTCPIterator, boost::asio::ip::basic_resolver_iterator< udp > &endpointUDPIterator):
				mySocket_(ioService),
				tcpEndpointIterator_(endpointTCPIterator),
				udpEndpoint_(endpointUDPIterator->endpoint()),
				udpSocket_(ioService),
				input_(ioService, ::dup(STDIN_FILENO)),
				output_(ioService, ::dup(STDOUT_FILENO))
{
	udpSocket_.open(udp::v4());
	std:: cout << "endpoint: " << udpEndpoint_ << "\n";
	initTCP();
	std::sprintf((char*)KEEPALIVE.elems, "KEEPALIVE\n");
	sendKEEPALIVE();
	readFromCin(10);
}

void Client::readFromCin(unsigned int howMuch)
{
	
	cinBuffer.assign(0);
	boost::asio::async_read(input_, boost::asio::buffer(cinBuffer, howMuch),
				[&, this](boost::system::error_code ec, std::size_t /*length*/)
				{
					if (!ec)
					{
						writeToCout(cinBuffer);
					}
					else
					{
					}
				});
}

void Client::writeToCout(Buffer buf)
{
	boost::asio::async_write(output_, boost::asio::buffer(buf),
				[&, this](boost::system::error_code ec, std::size_t /*length*/)
				{
					if (!ec)
					{
						
					}
					else
					{
					}
				});
}

void Client::sendKEEPALIVE()
{
	static boost::asio::deadline_timer timer(ioService, boost::posix_time::milliseconds(100));
	
	timer.expires_at(timer.expires_at() + boost::posix_time::milliseconds(100)); 
	timer.async_wait(
	[this](boost::system::error_code ec){
		
		sendUDP(KEEPALIVE);
		sendKEEPALIVE();
	});
	
	
}



void Client::initTCP()
{
	boost::asio::async_connect(mySocket_, tcpEndpointIterator_,
	[this](boost::system::error_code ec, boost::asio::ip::tcp::resolver::iterator)
	{
		if (!ec)
		{
			receiveClientId();;
		}
	});
}

void Client::cleanBuffer()
{
	buffer.assign(0);
}


void Client::receiveClientId()
{	
	static Buffer tempBuffer;
	tempBuffer.assign(0);
	
	if (debug)
	{
		std::cout << "początek receiveClientId()\n";
	}
	
	mySocket_.async_read_some(
	boost::asio::buffer(tempBuffer, BUFSIZE),
	[this](boost::system::error_code ec, std::size_t /*length*/)
	{
		if (!ec)
		{
			std::cout<< (char*)tempBuffer.c_array();
			std::stringstream ss;
			ss << (char*)tempBuffer.c_array();
			std::string temp;
			ss >> temp;
			if (temp == "CLIENT")
			{
				ss >> id_;
				if (debug) 
				{
					std::cout << "nadano id = " << id_ << "\n";
				}
				sendUDP(tempBuffer);
				receiveReport();
				receiveUDP();
			}
			else 
			{
				receiveClientId();
			}
		}
		else
		{
			mySocket_.close();
		}
	});
}


void Client::receiveReport()
{
	cleanBuffer();
	mySocket_.async_read_some(
	boost::asio::buffer(buffer, BUFSIZE),
	[this](boost::system::error_code ec, std::size_t /*length*/)
	{
		if (!ec)
		{
			//std::cerr<< (char *)buffer.c_array();
			receiveReport();
		}
		else
		{
			mySocket_.close();
		}
	});
}

void Client::sendUDP(Buffer data)
{
	udpSocket_.async_send_to(boost::asio::buffer(data), udpEndpoint_, [this, data](boost::system::error_code ec, std::size_t /*length*/)
	{
			if(debug)
			{
				//std::cout << "data sent by udp to server"  << (char*) data.elems;
			}
	});
}

void Client::receiveUDP()
{
	static Buffer tempBuffer;
	tempBuffer.assign(0);
	static udp::endpoint senderEndpoint;
	if (debug)
	{
		std::cout << "receiveUDP by " << id_ << "\n";
	}
	udpSocket_.async_receive_from(boost::asio::buffer(tempBuffer), senderEndpoint,
								  [&, this](boost::system::error_code ec, std::size_t /*length*/)
				{
					if (!ec)
					{
						if (debug)
						{
							std::cout << "receiveUDP message: "<< (char *)tempBuffer.c_array();
							std::cout << "endpoints: " << senderEndpoint << " udpendpoint: " << udpEndpoint_ << "\n";
						}
						if(senderEndpoint == udpEndpoint_){
							std::cout << "senderEndpoint == udpEndpoint_\n";
							std::stringstream ss;
							ss << (char *)tempBuffer.c_array();
							std::string beggining;
							ss >> beggining;
							if (beggining == "DATA")
							{
								receiveDATA(tempBuffer);
							}
							else if (beggining == "ACK")
							{
								receiveACK(tempBuffer);
							}
						}
						receiveUDP();
					}
					else
					{
						
					}
					
				});
}

void Client::receiveACK(Buffer data)
{
	static std::stringstream ss;
	ss << (char*)data.data();
	static std::string temp;
	ss >> temp >> ack_ >> win_;
	sendUPLOAD();
}

void Client::receiveDATA(Buffer data)
{
	static bool first = true;
	
	std::stringstream ss;
	ss << (char*)data.elems;
	static unsigned int tempNr, tempAck, tempWin;
	static std::string tempString;
	ss >> tempString >> tempNr;
	if (first) 
	{
		ss >> ack_ >> win_;
	}
	else 
	{
		ss >> tempAck >> tempWin;
	}
	ss.get(); // gettin \n
	std::string s((std::istreambuf_iterator<char>(ss.rdbuf())), std::istreambuf_iterator<char>());
	data.assign(0);
	std::sprintf((char*)data.elems, "%s", s.c_str());
	writeToCout(data);
}

void Client::sendUPLOAD()
{
	static Buffer buffer;
	buffer.assign(0);
	std::sprintf((char*)buffer.data(), "UPLOAD %u\n", ack_);
	static Buffer tempBuffer;
	tempBuffer.assign(0);
	boost::asio::async_read(input_, boost::asio::buffer(tempBuffer, win_),
				[&, this](boost::system::error_code ec, std::size_t /*length*/)
				{
					if (!ec)
					{
						std::sprintf((char*)buffer.elems, "%s%s", (char*)buffer.elems, (char*)tempBuffer.elems);
						sendUDP(buffer);
					}
					else
					{
					}
				});

}

bool setValues(int argc, char* argv[])
{
	try {
		po::options_description desc("Allowed options");
		desc.add_options()
			("help", "produce help message")
			("debug,d", "debug version")	
			("port,p", po::value<unsigned short>(&port), 
						"PORT number")
			("retransmit_limit,X", po::value<unsigned int>(&retransmitLimit)->default_value(RETRANSMIT_LIMIT), 
						"BUF_LEN value")
			("server,s", po::value<std::string>(&server), 
						"server name or ip REQUIRED")
			
		;
		
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);
	
		
		if (vm.count("debug")) {
			std::cout << "Debug mode: on\n";
			debug = true;
		}
		
		if (debug) {
			std::cout << "server: " << server << "\nretransmit_limit: " << retransmitLimit <<"\nport: " << port << "\n";
		}
		if ((vm.count("help")) || (!vm.count("server"))) {
			std::cout << "Usage: options_description [options]\n" << vm.count(("server")) << "\n";
			std::cout << desc;
			return false;
		}
		return true;

	}
	catch(std::exception& e)
	{
		std::cout << e.what() << "\n";
		return false;
	}   
}


int main(int argc, char* argv[])
{
	try{
		if (setValues(argc, argv))
		{
			tcp::resolver resolver(ioService);
			udp::resolver udpResolver(ioService);
			std::string portStr = std::to_string(port);
			auto endpointIterator = resolver.resolve({ server, portStr });
			auto udpEndpointIterator = udpResolver.resolve({ server, portStr });
			Client client(endpointIterator, udpEndpointIterator);
			ioService.run();
		}
	} catch(std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}
	
	return 0;
}