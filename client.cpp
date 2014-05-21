#include <boost/program_options.hpp>
#include <iostream>

namespace po = boost::program_options;

const int RETRANSMIT_LIMIT = 10;

int main(int argc, char* argv[])
{
	try {
		bool debug = false;
		int retransmitLimit;
		std::string server = ""; 
		po::options_description desc("Allowed options");
		desc.add_options()
			("help", "produce help message")
			("debug,d", "debug version")
			("retransmit_limit,X", po::value<int>(&retransmitLimit)->default_value(RETRANSMIT_LIMIT), 
						"BUF_LEN value")
			("server,s", po::value<std::string>(&server), 
						"BUF_LEN value")
			
		;
		
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if ((vm.count("help")) || (server=="")) {
			std::cout << "Usage: options_description [options]\n";
			std::cout << desc;
			return 0;
		}
		if (vm.count("debug")) {
			std::cout << "Debug mode: on\n";
			debug = true;
		}
		
		if (debug) {
			std::cout << "server: " << server << "\nretransmit_limit: " << retransmitLimit << "\n";
		}

	}
	catch(std::exception& e)
	{
		std::cout << e.what() << "\n";
		return 1;
	}   
	return 0;
}