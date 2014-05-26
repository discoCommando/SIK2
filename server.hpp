#ifndef SERVER_HPP
#define SERVER_HPP

#include <boost/asio.hpp>
#include <set>


using boost::asio::ip::tcp;
using boost::asio::ip::udp;
const unsigned int BUFSIZE = 100;

bool debug = false;
unsigned short port;
unsigned int fifoSize;
unsigned int fifoLowWatermark;
unsigned int fifoHighWatermark;
unsigned int bufLen;
unsigned int txInterval;

typedef boost::array<short, BUFSIZE> Buffer;

class Participant
{
public:
	Participant(unsigned int id);
	virtual ~Participant();
	virtual void sendTCP(const Buffer message) = 0;
	virtual void sendUDP(const Buffer message) = 0;
	unsigned int id() const;
	virtual std::string giveReport() = 0;
	virtual void setUDPEndpoint(udp::endpoint &endpoint) = 0;
	virtual udp::endpoint getUDPEndpoint() = 0;
protected:
	unsigned int id_;
};

class Room
{
public:
	Room(const udp::endpoint &udpEndpoint);
	void join(std::shared_ptr<Participant> participant);
	void leave(std::shared_ptr<Participant> participant);
	void deliverReport();
	void receiveUDP();
	void handleClientID(const Buffer buffer, udp::endpoint &endpoint);
	void handleUpload(const Buffer buffer);
	void handleAck(const Buffer buffer);
	void setUDPSocket(udp::endpoint endpoint);
	void sendUDP(unsigned int id);
	const udp::endpoint & getRoomUDPEndpoint();
	udp::socket & getUDPSocket();
private:
	std::set<std::shared_ptr<Participant>> participants_;
	udp::socket udpSocket_;
	udp::endpoint udpEndpoint_;
	Buffer buffer_;
	udp::endpoint tempUDPEndpoint_;
};

class Server
{

public:
	Server(const tcp::endpoint& endpoint, const udp::endpoint& udpEndpoint);
	void acceptClient();
private:
	tcp::acceptor acceptor_;
	tcp::socket socket_;
	unsigned int ids_;// następne wolne id
	Room room_;
};


class Client: public Participant,
		public std::enable_shared_from_this<Client>
{
public:
	Client(tcp::socket socket, Room& room, unsigned int id);
	void sendTCP(const Buffer message);
	void sendUDP(const Buffer message);
	void initUDP();
	void start();
	void receiveTCP();
	std::string giveReport();
	void setUDPEndpoint(udp::endpoint &endpoint);
	udp::endpoint getUDPEndpoint();
private:
	Room &room_;
	tcp::socket socket_;
	udp::endpoint *udpEndpoint_;
	udp::socket *udpSocket_;
};

#endif 