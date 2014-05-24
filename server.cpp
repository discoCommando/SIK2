#include <boost/program_options.hpp>
#include <boost/bind.hpp>
#include <iostream>

#include "server.hpp"

namespace po = boost::program_options;

const unsigned short PORT = 16034;
const unsigned int FIFO_SIZE = 10560;
const unsigned int FIFO_LOW_WATERMARK = 0;
const unsigned int BUF_LEN = 10;
const unsigned int TX_INTERVAL = 5;

boost::asio::io_service ioService;

bool debug = false;
unsigned short port;
unsigned int fifoSize;
unsigned int fifoLowWatermark;
unsigned int fifoHighWatermark;
unsigned int bufLen;
unsigned int txInterval;

Server::Server(const tcp::endpoint& endpoint, const udp::endpoint& udpEndpoint):
		acceptor_(ioService, endpoint),
		socket_(ioService),
		room_(udpEndpoint)
{
	ids_ = 0;
	acceptClient();
}


void Server::acceptClient()
{
	acceptor_.async_accept(socket_,
			[this](boost::system::error_code ec)
			{
				if (!ec)
				{
					std::make_shared<Client>(std::move(socket_), room_, ids_++)->start();
					
					if (debug)
					{
						std::cout<< "zaakceptowal " << ids_ - 1; 
					}
				}
				if (debug)
				{
					std::cout<< " akceptacja c.d\n";
				}
				
				acceptClient();
			});
	
}

Participant::Participant(unsigned int id):id_(id)
{
  
}

Participant::~Participant()
{
	if(debug)
		std::cout << "KONIEC MOJEGO ZYCIA\n";
}

unsigned int Participant::id() const
{
	return id_;
}



Client::Client(boost::asio::ip::tcp::socket socket, Room& room, unsigned int id):
			Participant(id),
			room_(room),
			socket_(std::move(socket))
{

}


void Client::sendTCP(const char * message)
{
	auto self(shared_from_this());	
	if (debug)
	{
		std::cout << "sendTCP\n";
	}
	boost::asio::async_write(socket_,
	boost::asio::buffer(message,
		strlen(message)),
	[this, self](boost::system::error_code ec, std::size_t /*length*/)
	{
		if (!ec)
		{
			if (debug)
			{
				std::cout << "udalo sie sendTCP\n";
			}
		}
		else
		{
			//room_.leave(shared_from_this());
			std::cout << "wyszed xD\n";
		}
	});
}


void Client::start()
{
	//room_.join
	room_.join(shared_from_this());
	if (debug) 
	{
		std::cout << id_ << " wszedl do pokoju\n";
	}
	receiveTCP();
	
}


void Client::receiveTCP()
{
	auto self(shared_from_this());
	boost::array<char, BUFSIZE> buffer;
	buffer.assign(0);
	if (debug)
	{
		std::cout << "receiveTCP\n";
	}
	socket_.async_read_some(
			boost::asio::buffer(buffer),
			[this, self](boost::system::error_code ec, std::size_t /*length*/)
			{
				if (!ec)
				{
					receiveTCP();
				}
				else
				{
					room_.leave(shared_from_this());
				}
			});
}



void Client::initUDP()
{

}



Room::Room(const udp::endpoint& udpEndpoint):
	udpSocket_(ioService, udpEndpoint)
{
	deliverReport();
	receiveUDP();
}


void Room::deliverReport()
{
	static boost::asio::deadline_timer reportTimer(ioService, boost::posix_time::seconds(1));
	
	
	if (debug) 
	{
		std::cout << "deliver Report, size participants: " << participants_.size() <<"\n";
	}
	reportTimer.expires_at(reportTimer.expires_at() + boost::posix_time::seconds(1)); 
	reportTimer.async_wait(
	[this](boost::system::error_code ec){
		char report[100] = "no siema\n";
		for (auto participant: participants_)
		{
			if (debug)
			{
				std::cout << "wysłano raport do " << participant->id() << "\n";
			}
			participant->sendTCP(report);
		}
		deliverReport();
	});
	
}

void Room::join(std::shared_ptr< Participant > participant)
{
	participants_.insert(participant);
	if(debug)
		std::cout << "participant " << participant->id()<< " joined room, participants size:" << participants_.size() << "\n";
// 	for (auto msg: recent_msgs_)
// 	participant->deliverTCP(msg);
}

void Room::leave(std::shared_ptr< Participant > participant)
{
	participants_.erase(participant);
	if(debug)
		std::cout << "wyszed" << std::endl;
}


void Room::handleClientID(unsigned int clientId)
{

}


void Room::receiveUDP()
{
	static Buffer tempBuffer;
	tempBuffer.assign(0);
	udp::endpoint senderEndpoint;
	udpSocket_.async_receive_from(boost::asio::buffer(tempBuffer), senderEndpoint,
								  [this](boost::system::error_code ec, std::size_t /*length*/)
				{
					if (!ec)
					{
						std::cout << tempBuffer.c_array();
					}
					else
					{
						
					}
					
				});

}

void Room::setUDPSocket(udp::endpoint endpoint)
{
	udp::socket temp(ioService, endpoint);
	
}


bool setValues(int argc, char* argv[])
{
	try {
		
		po::options_description desc("Allowed options");
		desc.add_options()
			("help", "produce help message")
			("debug,d", "debug version")
			("port,p", po::value<unsigned short>(&port)->default_value(PORT), 
						"PORT number")
			("fifo_size,F", po::value<unsigned int>(&fifoSize)->default_value(FIFO_SIZE), 
						"FIFO_SIZE value")
			("low_watermark,L", po::value<unsigned int>(&fifoLowWatermark)->default_value(FIFO_LOW_WATERMARK), 
						"FIFO_LOW_WATERMARK value")
			("high_watermark,H", po::value<unsigned int>(&fifoHighWatermark), 
						"FIFO_HIGH_WATERMARK value")
			("buf_len,X", po::value<unsigned int>(&bufLen)->default_value(BUF_LEN), 
						"BUF_LEN value")
			("tx_unsigned interval,i", po::value<unsigned int>(&txInterval)->default_value(TX_INTERVAL), 
						"TX_INTERVAL value")
		;
		
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);
		
		if (!vm.count("high_watermark")){
			fifoHighWatermark = fifoSize;
		}
		
		if (vm.count("debug")) {
			debug = true;
		}
		if (debug) {
			std::cout << "port: " << port << "\n"<< "fifoSize: " << fifoSize << "\nfifoLowWatermark: " << fifoLowWatermark 
			<< "\nfifoHighWatermark: " << fifoHighWatermark << "\nbufLen: " <<  bufLen << "\ntxInterval: " << txInterval << "\n";
		}
		if (vm.count("help")) {
			std::cout << "Usage: options_description [options]\n";
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
	try {
		if (setValues(argc, argv))
		{
			tcp::endpoint endpoint(tcp::v4(), port);
			udp::endpoint udpEndpoint(udp::v4(), port);
			Server s(endpoint, udpEndpoint);
			ioService.run();
		}
	}catch(std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}
	return 0;
}