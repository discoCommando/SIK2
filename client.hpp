#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <boost/asio.hpp>
const unsigned int BUFSIZE = 100;

typedef boost::array<short, BUFSIZE> Buffer;
Buffer KEEPALIVE;
class Client
{
public:
	Client(boost::asio::ip::tcp::resolver::iterator &endpointTCPIterator, boost::asio::ip::udp::resolver::iterator &endpointUDPIterator);
	void initTCP();
	void receiveClientId();
	void receiveUDP();
	void sendUDP(Buffer data);
	void sendUPLOAD();
	void receiveACK(Buffer data);
	void receiveDATA(Buffer data);
	void sendKEEPALIVE();
	void receiveReport();
	void cleanBuffer();
	void readFromCin(unsigned int howMuch); 
	void writeToCout(Buffer buf);
	
private:
	boost::asio::ip::tcp::socket mySocket_;
	boost::asio::ip::tcp::resolver::iterator tcpEndpointIterator_;
	boost::asio::ip::udp::endpoint udpEndpoint_;
	boost::asio::ip::udp::socket udpSocket_;
	unsigned int id_;
	boost::array<char, BUFSIZE> buffer;
	boost::asio::posix::stream_descriptor input_;
	boost::asio::posix::stream_descriptor output_;
	unsigned int ack_, win_, upload_;
	Buffer cinBuffer;
};

#endif 