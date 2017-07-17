#include "cache.hpp"
//#include <iostream>

using namespace std;

cache::cache(unsigned addr_bits, unsigned words_in, unsigned blocks_in, unsigned sets_in, unsigned size_in, int htime, int rtime, int wtime)
{
	// calculate RAM size: number of words needed.
	RAM_size = 1 << addr_bits;
	RAM_size /= words_in;
	
	for (unsigned i = 0; i < RAM_size; i++)
	{
		word temp = 0;
		RAM.push_back(temp);
	}
	RAM.shrink_to_fit();

	word_size = words_in;
	block_size = blocks_in;
	set_size = sets_in;
	cache_size = size_in;

	for (unsigned i = 0; i < cache_size; i++)
	{
		set temp(set_size, block_size);
		cache_mem.push_back(temp);
	}
	cache_mem.shrink_to_fit();

	hit_time = htime;
	read_time = rtime;
	write_time = wtime;
}

int cache::read(int address, word& data)
{
	int response_time = 0;
	bool dirty;			// if cache miss, used to see if block is dirty (needs to write back)
	int block_offset;	// used to get correct word
	int set_index;		// used to find the correct set
	int cache_tag;		// used to check for hit

	// check for cache hit
	block_offset = (address / word_size) % block_size;
	set_index = (address / (word_size * block_size)) % cache_size;
	cache_tag = address / (word_size * block_size * cache_size);

	if (!cache_mem[set_index].read(block_offset, cache_tag, data, dirty)) // if miss:
	{
		word *ram_data = new word[block_size];

		if (dirty)
		{
			// write back
			int write_addr = cache_mem[set_index].write_back_address(set_index, cache_size, block_size, word_size);
			
			cache_mem[set_index].get_data(ram_data, block_size);
			write_back(write_addr, ram_data);

			response_time += write_time;	
		}

		// read from RAM
		read_in(address - block_offset, ram_data);
		cache_mem[set_index].set_data(cache_tag, ram_data, block_size);

		// read to output
		data = ram_data[block_offset];

		response_time += read_time;

		delete[] ram_data;
	}

	response_time += hit_time;

	return response_time;
}

int cache::write(int address, word data)
{
	int response_time = 0;
	bool dirty;			// if cache miss, used to see if block is dirty (needs to write back)
	int block_offset;	// used to get correct word
	int set_index;	// used to find the correct block
	int cache_tag;		// used to check for hit
	
	// check for cache hit
	block_offset = (address / word_size) % block_size;
	set_index = (address / (word_size * block_size)) % cache_size;
	cache_tag = address / (word_size * block_size * cache_size);

	if (!cache_mem[set_index].write(block_offset, cache_tag, data, dirty)) // if miss:
	{
		word *ram_data = new word[block_size];

		if (dirty)
		{
			// write back
			int write_addr = cache_mem[set_index].write_back_address(set_index, cache_size, block_size, word_size);

			cache_mem[set_index].get_data(ram_data, block_size);
			write_back(write_addr, ram_data);

			response_time += write_time;
		}

		// read in from RAM
		read_in(address - block_offset, ram_data);
		cache_mem[set_index].set_data(cache_tag, ram_data, block_size);

		// write from input
		cache_mem[set_index].write(block_offset, cache_tag, data, dirty);

		response_time += read_time;

		delete[] ram_data;
	}

	response_time += hit_time;

	return response_time;
}

int cache::flush()
{
	int response_time = 0;
	word* data = new word[block_size];

	for (unsigned i = 0; i < cache_size; i++) // check through each set
	{
		for (unsigned j = 0; j < set_size; j++) // check through each block
		{
			int tag, write_addr;

			if (cache_mem[i].get_dirty_data(tag, data, block_size, j))	// if block is dirty...
			{
				// write back
				write_addr = cache_mem[i].write_back_address(i, cache_size, block_size, word_size, tag);
				write_back(write_addr, data);

				response_time += write_time;
			}
		}
	}

	delete[] data;

	return response_time;
}

void cache::write_back(int address, word* data)
{
	//cout << "writing back to " << address << endl;
	for (unsigned i = 0; i < block_size; i++)
		RAM[address + i] = data[i];

	return;
}

void cache::read_in(int address, word* data)
{
	for (unsigned i = 0; i < block_size; i++)
		data[i] = RAM[address + i];

	return;
}


set::set(int set_size, int block_size)
{
	for (int i = 0; i < set_size; i++)
	{
		block temp(block_size);
		blocks.push_back(temp);
	}
}

bool set::read(int block_offset, int address_tag, word& data, bool& dirty)
{
	for (list<block>::iterator i = blocks.begin(); i != blocks.end(); ++i)
	{
		// if the correct block is found:
		if (i->check_tag(address_tag))
		{
			// return correct word
			data = i->get_data(block_offset);

			// move block to front of list
			blocks.insert(blocks.begin(), *i);
			blocks.erase(i);

			// hit
			return true;
		}
	}

	// miss: see if the last block is dirty or not, as this the least recently accessed block and will be overwritten.
	dirty = blocks.back().is_dirty();

	return false;
}

bool set::write(int block_offset, int address_tag, word data, bool& dirty)
{
	for (list<block>::iterator i = blocks.begin(); i != blocks.end(); ++i)
	{
		// if the correct block is found:
		if (i->check_tag(address_tag))
		{
			// set correct word, and tag as dirty.
			i->set_data(data, block_offset);
			i->set_dirty(true);

			// move block to front of list
			blocks.insert(blocks.begin(), *i);
			blocks.erase(i);

			// hit
			return true;
		}
	}

	// miss: see if the last block is dirty or not, as this the least recently accessed block and will be overwritten.
	dirty = blocks.back().is_dirty();

	return false;
}

void set::set_data(int address_tag, word* data, int block_size)
{
	block temp = blocks.back();
	for (int j = 0; j < block_size; j++)
		temp.set_data(data[j], j);

	// set tag and valid bit, and set dirty bit to false.
	temp.set_tag(address_tag);

	// move to front and remove the end block
	blocks.push_front(temp);
	blocks.pop_back();

	return;
}

void set::get_data(word* data, int block_size)
{
	block temp = blocks.back();
	for (int j = 0; j < block_size; j++)
		data[j] = temp.get_data(j);

	return;
}

bool set::get_dirty_data(int& address_tag, word* data, int block_size, int block_number)
{
	list<block>::iterator i = blocks.begin();
	for (int j = 0; j < block_number; j++)
		i++;

	if (i->is_dirty())	// if dirty...
	{
		// return data and tag so it can be written back.
		for (int j = 0; j < block_size; j++)
			data[j] = i->get_data(j);

		address_tag = i->get_tag();

		i->set_dirty(false);

		return true;
	}

	return false;
}

int set::write_back_address(int index, int cache_size, int block_size, int word_size)
{
	int address = blocks.back().get_tag();
	address *= cache_size;
	address += index;
	address *= block_size;
	address *= word_size;

	return address;
}

int set::write_back_address(int index, int cache_size, int block_size, int word_size, int tag)
{
	int address = tag;
	address *= cache_size;
	address += index;
	address *= block_size;
	address *= word_size;

	return address;
}


block::block(int block_size)
{
	valid = false;
	dirty = false;

	for (int i = 0; i < block_size; i++)
	{
		word temp;
		data.push_back(temp);
	}
}

bool block::check_tag(int address_tag)
{
	if (tag == address_tag && valid)
		return true;
	else
		return false;
}

void block::set_tag(int address_tag)
{
	valid = true;
	dirty = false;
	tag = address_tag;
	return;
}

int block::get_tag()
{
	return tag;
}

void block::set_data(word data_in, int block_offset)
{
	data[block_offset] = data_in;
	return;
}

word block::get_data(int block_offset)
{
	return data[block_offset];
}

bool block::is_dirty()
{
	return dirty;
}

void block::set_dirty(bool dirty_in)
{
	dirty = dirty_in;
	return;
}
