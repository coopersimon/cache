#include "cache.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>

using namespace std;

bool power2(unsigned input);

int main(int argc, char *argv[])
{
	unsigned addr_bits, word_size, block_size, set_size, cache_size, htime, rtime, wtime;

	if (argc >= 8)
	{
		addr_bits = atoi(argv[1]);
		word_size = atoi(argv[2]);
		block_size = atoi(argv[3]);
		set_size = atoi(argv[4]);
		cache_size = atoi(argv[5]);
		htime = atoi(argv[6]);
		rtime = atoi(argv[7]);
		wtime = atoi(argv[8]);
	}
#ifdef DEBUG
    else if (argc < 8)
	{
		cout << "Input cache arguments (debug only) : " << endl;
		cin >> addr_bits >> word_size >> block_size >> set_size >> cache_size >> htime >> rtime >> wtime;
	}
#endif
    else
    {
	    addr_bits = 16;
	    word_size = 4;
	    block_size = 4;
	    set_size = 2;
	    cache_size = 8;
	    htime = 1;
	    rtime = 2;
	    wtime = 2;
    }


	if (addr_bits > 23 || !power2(word_size) || !power2(block_size) || !power2(set_size) || !power2(cache_size))
	{
		cout << "Invalid cache parameters." << endl;
		return EXIT_FAILURE;
	}

	cache state(addr_bits, word_size, block_size, set_size, cache_size, htime, rtime, wtime);

	string command;
	int address, response_time;
	word data;

	ifstream in_file;
	ofstream out_file;

	in_file.open("debug.txt");
	if (!in_file.is_open())
	{
		cout << "Could not find input file." << endl;
		return EXIT_FAILURE;
	}

	out_file.open("output.txt");

	while (in_file >> command)
	{
		if (command[0] == '#')	// get rid of comment by skipping over line
		{
			char* temp = new char[1019];
			in_file.getline(temp, 1019);
			delete[] temp;
		}
		
		else if (command == "read-req")
		{
			in_file >> address;
			response_time = state.read(address, data);
			if (response_time > htime)
				out_file << "read-ack " << ((address / (word_size * block_size)) % cache_size) << " miss " << response_time << " " << data << endl;
			else
				out_file << "read-ack " << ((address / (word_size * block_size)) % cache_size) << " hit " << response_time << " " << data << endl;
		}

		else if (command == "write-req")
		{
			in_file >> address;
			in_file >> data;
			response_time = state.write(address, data);
			if (response_time > htime)
				out_file << "write-ack " << ((address / (word_size * block_size)) % cache_size) << " miss " << response_time << endl;
			else
				out_file << "write-ack " << ((address / (word_size * block_size)) % cache_size) << " hit " << response_time << endl;
		}

		else if (command == "flush-req")
		{
			response_time = state.flush();
			out_file << "flush-ack " << response_time << endl;
		}

		else if (command == "debug-req")
		{
			vector<int> v_sets;
			vector<word> v_data;
			//state.debug(v_sets, v_data);
			cout << "debug-begin" << endl;
			for (int i = 0; i < v_data.size(); i++)
				cout << "in set: " << v_sets[i] << " data: " << v_data[i] << endl;

			cout << "debug-end" << endl;
		}

		else
			out_file << "invalid" << endl;
	}


	return 0;
}

bool power2(unsigned input)
{
	if ((input & (input - 1)) == 0)
		return true;
	else
		return false;
}
