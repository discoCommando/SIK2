#ifndef SERVER_HPP
#define SERVER_HPP

#include <boost/asio.hpp>
#include <set>


using boost::asio::ip::tcp;
using boost::asio::ip::udp;
const unsigned int BUFSIZE = 100;

typedef boost::array<char, BUFSIZE> Buffer;

class Participant
{
public:
	Participant(unsigned int id);
	virtual ~Participant();
	virtual void sendTCP(const char* message) = 0;
	unsigned int id() const;
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
	void handleClientID(unsigned int clientId);
	void setUDPSocket(udp::endpoint endpoint);
private:
	std::set<std::shared_ptr<Participant>> participants_;
	udp::socket udpSocket_;
	Buffer buffer_;
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
	void sendTCP(const char* message);
	void initUDP();
	void start();
	void receiveTCP();
private:
	Room &room_;
	tcp::socket socket_;
	udp::endpoint udpEndpoint_;
};

#endif 