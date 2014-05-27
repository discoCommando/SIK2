#include <boost/program_options.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <sstream>
#include <cstdio>

#include "server.hpp"

namespace po = boost::program_options;

const unsigned short PORT = 16034;
const unsigned int FIFO_SIZE = 10560;
const unsigned int FIFO_LOW_WATERMARK = 0;
const unsigned int BUF_LEN = 10;
const unsigned int TX_INTERVAL = 5;

boost::asio::io_service ioService;



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
						std::cout<< "zaakceptowal " << ids_ - 1 << "\n"; 
					}
				}
				if (debug)
				{
					//std::cout<< " akceptacja c.d\n";
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
			socket_(std::move(socket)),
			udpSocket_(&(room_.getUDPSocket())),
			win_(fifoSize),
			ack_(0),
			timer(ioService, boost::posix_time::seconds(3)),
			isActive(false)
{
	timer.async_wait(
	[this](boost::system::error_code ec){
		room_.leave(shared_from_this());
	});
	actualInput.assign(0);
}

unsigned int Client::getFifoActualSize()
{
	return fifo_.size();
}


udp::endpoint Client::getUDPEndpoint()
{
	return *udpEndpoint_;
}

void Client::sendTCP(const Buffer message)
{
	auto self(shared_from_this());	
	if (debug)
	{
		//std::cout << "sendTCP\n";
	}
	boost::asio::async_write(socket_,
	boost::asio::buffer(message,
		strlen((char*)message.data())),
	[this, self](boost::system::error_code ec, std::size_t /*length*/)
	{
		if (!ec)
		{
			if (debug)
			{
				//std::cout << "udalo sie sendTCP\n";
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
	static Buffer buffer;
	buffer.assign(0);
	if (debug)
	{
		//std::cout << "receiveTCP\n";
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

std::string Client::giveReport()
{
	std::stringstream ss;
	ss <<socket_.remote_endpoint() << " FIFO: 0/0(min.0,max.0)";
	
	return ss.str();
}

void Client::sendUDP(const Buffer message)
{
	udpSocket_->async_send_to(boost::asio::buffer(message), *udpEndpoint_, [this, message](boost::system::error_code ec, std::size_t /*length*/)
	{
			if(debug)
			{
				std::cout << "data sent by udp to client:"  << id_ <<" "<< (char*)message.elems;
			}
	});
}

void Client::setUDPEndpoint(udp::endpoint& endpoint)
{
	if(debug) 
	{
		std::cout << "setudpendpoint: " << endpoint << "\n";
	}
	udpEndpoint_ = &endpoint;
}

unsigned int Client::ack()
{
	return ack_;
}
void Client::setAck(unsigned int newAck)
{
	ack_ = newAck;
}
void Client::setWin(unsigned int newWin)
{
	win_ = newWin;
}
unsigned int Client::win()
{
	return win_;
}

void Client::addData(Buffer& buffer)
{
	//TODO mutex
	std::sprintf((char*)actualInput.elems, "%s%s", (char*)actualInput.elems, (char*)buffer.elems);
	if (!isActive)
	{
		if (actualInput.size() >= fifoHighWatermark)
		{
			isActive = true;
		}
	}
}

void* Client::getActualInput()
{
	return (void *)actualInput.elems;
}


void Client::keepAlive()
{
	timer.expires_at(timer.expires_at() + boost::posix_time::milliseconds(1000)); 
}



Room::Room(const udp::endpoint& udpEndpoint):
	udpSocket_(ioService, udpEndpoint),
	udpEndpoint_(udpEndpoint)
{
	deliverReport();
	receiveUDP();
}

void Room::sendData(Buffer buffer, std::shared_ptr< Participant > participant)
{
	
}

void Room::sendDataToAll()
{
	static boost::asio::deadline_timer reportTimer(ioService, boost::posix_time::milliseconds(txInterval));
	
	reportTimer.expires_at(reportTimer.expires_at() + boost::posix_time::seconds(1)); 
	reportTimer.async_wait(
	[this](boost::system::error_code ec){
		static mixer_input inputs[participants_.size()];
		for (auto participant: participants_)
		{
			mixer_input temp;
			mixer_input.data = participant->getActualInput();
			mixer_input.len = participant->getFifoActualSize();
			inputs[participant->id()] = temp;
		}
		static Buffer data;
		data.assign(0);
		std::size_t outputSize;
		mixer(inputs, (void*)data.elems, &outputSize, txInterval);
		
		
	});
}




const udp::endpoint &Room::getRoomUDPEndpoint()
{
	return udpEndpoint_;
}

udp::socket& Room::getUDPSocket()
{
	return udpSocket_;
}

void Room::deliverReport()
{
	static boost::asio::deadline_timer reportTimer(ioService, boost::posix_time::seconds(1));
	
	
	if (debug) 
	{
		//std::cout << "deliver Report, size participants: " << participants_.size() <<"\n";
	}
	reportTimer.expires_at(reportTimer.expires_at() + boost::posix_time::seconds(1)); 
	reportTimer.async_wait(
	[this](boost::system::error_code ec){
		
		Buffer report;
		std::string tempReport;
		if (!debug) {
			tempReport = "\n";
		}
		else 
		{
			tempReport = "\n";
		}
		for (auto participant: participants_)
		{
			tempReport += participant->giveReport();
			tempReport += "\n";
		}
		std::sprintf((char*)report.elems, "%s", (char *)tempReport.c_str());
		
		for (auto participant: participants_)
		{
			if (debug)
			{
				//std::cout << "wysłano raport do " << participant->id() << "\n";
			}
			participant->sendTCP(report);
		}
		deliverReport();
	});
	
}

void Room::join(std::shared_ptr< Participant > participant)
{
	participants_.insert(participant);
	static Buffer tempBuffer;
	tempBuffer.assign(0);
	static std::stringstream ss;
	std::sprintf((char *)tempBuffer.elems, "CLIENT %u\n", participant->id());
	participant->sendTCP(tempBuffer);
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


void Room::handleClientID(const Buffer buffer, udp::endpoint& endpoint)
{
	std::stringstream ss;
	ss << (char *)buffer.data();
	unsigned int id;
	std::string temp;
	ss >> temp >> id;
	if (debug)
	{
		std::cout << "handleClientID " << id << "\n";
	}
	for (auto participant: participants_)
	{
		if (participant->id() == id)
		{
			participant->setUDPEndpoint(endpoint);
			return;
		}
	}
	
	//sendUDP(id);
	
}

void Room::sendUDP(unsigned int id, Buffer message)
{
	Buffer buf;
	std::string temp1 = "UDPENDPOINT ACTIVATED\n";
	std::sprintf((char*)buf.elems, "%s", temp1.c_str());
	if(debug)
				{
					std::cout << "data sent by udp to client:"  << id <<" "<< (char*)buf.elems;
				}
	for (auto participant: participants_)
	{
		if (participant->id() == id)
		{
			if(debug)
			{
				std::cout << participant->getUDPEndpoint() << "\n";
			}
			udpSocket_.async_send_to(boost::asio::buffer(message), participant->getUDPEndpoint(), [&, this](boost::system::error_code ec, std::size_t /*length*/)
			{
				
			});
			return;
		}
	}
	
}



void Room::handleUpload(Buffer buffer, udp::endpoint &endpoint)
{
	
	for (auto participant: participants_)
	{
		if (participant->getUDPEndpoint() == endpoint)
		{
			std::stringstream ss;
			ss << (char*)buffer.elems;
			static unsigned int tempNr;
			static std::string tempString;
			ss >> tempString >> tempNr;
			
			participant->setAck(participant->ack() + 1);
			
			ss.get(); // gettin \n
			std::string s((std::istreambuf_iterator<char>(ss.rdbuf())), std::istreambuf_iterator<char>());
			buffer.assign(0);
			std::sprintf((char*)buffer.elems, "%s", s.c_str());
			participant->addData(buffer);
			return;
		}
	}
	
}

void Room::handleKeepAlive(Buffer tempBuffer, udp::endpoint& endpoint)
{
	for (auto participant: participants_)
	{
		if(participant->getUDPEndpoint() == endpoint){
			participant->keepAlive();
		
			return;
		}
		
	}
}



void Room::receiveUDP()
{
	static Buffer tempBuffer;
	tempBuffer.assign(0);
	
	udpSocket_.async_receive_from(boost::asio::buffer(tempBuffer), tempUDPEndpoint_,
								  [&, this](boost::system::error_code ec, std::size_t /*length*/)
				{
					if (!ec)
					{
						if (tempUDPEndpoint_ != udpEndpoint_){
							std::stringstream ss;
							ss << (char*)tempBuffer.c_array();
							std::string beggining;
							ss >> beggining;
							if (beggining == "CLIENT")
							{
								handleClientID(tempBuffer, tempUDPEndpoint_);
							}
							else if (beggining == "UPLOAD")
							{
								handleUpload(tempBuffer, tempUDPEndpoint_);
							}
							else if (beggining == "KEEPALIVE")
							{
								handleKeepAlive(tempBuffer, tempUDPEndpoint_);
							}
						} else 
						{
							std::cout << "senderEndpoint == udpEndpoint_\n";
						}
						receiveUDP();
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