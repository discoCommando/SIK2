#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <boost/asio.hpp>

const unsigned int BUFSIZE = 100;

typedef boost::array<char, BUFSIZE> Buffer;
class Client
{
public:
	Client(boost::asio::ip::tcp::resolver::iterator endpointTCPIterator, boost::asio::ip::udp::resolver::iterator endpointUDPIterator);
	void initTCP();
	void receiveClientId();
	void receiveUDP();
	void sendUDP(Buffer data);
	void sendUPLOAD();
	void receiveACK();
	void receiveDATA();
	void sendKEEPALIVE();
	void receiveReport();
	void cleanBuffer();
	
private:
	boost::asio::ip::tcp::socket mySocket_;
	boost::asio::ip::tcp::resolver::iterator tcpEndpointIterator_;
	boost::asio::ip::udp::endpoint udpEndpoint_;
	boost::asio::ip::udp::socket udpSocket_;
	unsigned int id_;
	boost::array<char, BUFSIZE> buffer;
};

#endif 