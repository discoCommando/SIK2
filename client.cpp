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

Client::Client(boost::asio::ip::basic_resolver_iterator< tcp > endpointTCPIterator, boost::asio::ip::basic_resolver_iterator< udp > endpointUDPIterator):
				mySocket_(ioService),
				tcpEndpointIterator_(endpointTCPIterator),
				udpEndpoint_(*endpointUDPIterator),
				udpSocket_(ioService, udpEndpoint_)
{

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
	mySocket_.async_read_some(
	boost::asio::buffer(tempBuffer, BUFSIZE),
	[this](boost::system::error_code ec, std::size_t /*length*/)
	{
		if (!ec)
		{
			std::cout<< tempBuffer.c_array();
			if (buffer[0] == 'C')
			{
				std::stringstream ss;
				ss << tempBuffer.data();
				ss >> id_;
				if (debug) 
				{
					std::cout << "nadano id = " << id_ << "\n";
				}
				sendUDP(tempBuffer);
				receiveReport();
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
			std::cout<< buffer.c_array();
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