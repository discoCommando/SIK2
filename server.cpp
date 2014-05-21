#include <boost/program_options.hpp>
#include <iostream>

namespace po = boost::program_options;

const unsigned short PORT = 16034;
const unsigned int FIFO_SIZE = 10560;
const unsigned int FIFO_LOW_WATERMARK = 0;
const unsigned int BUF_LEN = 10;
const unsigned int TX_INTERVAL = 5;

int main(int argc, char* argv[])
{
	try {
		bool debug = false;
		unsigned short port;
		unsigned int fifoSize;
		unsigned int fifoLowWatermark;
		unsigned int fifoHighWatermark;
		unsigned int bufLen;
		unsigned int txInterval;
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
			("high_watermark,H", po::value<unsigned int>(&fifoHighWatermark)->default_value(-1), 
						"FIFO_HIGH_WATERMARK value")
			("buf_len,X", po::value<unsigned int>(&bufLen)->default_value(BUF_LEN), 
						"BUF_LEN value")
			("tx_unsigned interval,i", po::value<unsigned int>(&txInterval)->default_value(TX_INTERVAL), 
						"TX_INTERVAL value")
		;
		
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) {
			std::cout << "Usage: options_description [options]\n";
			std::cout << desc;
			return 0;
		}
		if (vm.count("debug")) {
			debug = true;
		}
		if (fifoHighWatermark == -1){
			fifoHighWatermark = fifoSize;
		}
		
		if (debug) {
			std::cout << "port: " << port << " fifoSize: " << fifoSize << " fifoLowWatermark: " << fifoLowWatermark 
			<< " fifoHighWatermark: " << fifoHighWatermark << " bufLen: " <<  bufLen << " txInterval: " << txInterval << "\n";
		}

	}
	catch(std::exception& e)
	{
		std::cout << e.what() << "\n";
		return 1;
	}   
	return 0;
}