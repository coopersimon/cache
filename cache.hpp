#ifndef CACHE_HPP
#define CACHE_HPP

#define BYTE	 8

#include <list>
#include <vector>
#include <exception>

// 32 bits should be enough, switch to long long if needed
typedef long long word;


/*class param_exception : public exception
{
	virtual const char* what() const throw()
	{
		return "Invalid parameter";
	}
};*/

class block
{
private:
	bool valid;
	bool dirty;
	int tag;
	std::vector<word> data;

public:

	block(int block_size);

	bool check_tag(int address_tag);	// returns true if the tag matches AND the valid bit is set.

	void set_tag(int address_tag);					// sets tag for the block as well as setting valid bit
	int get_tag();									// returns tag
	void set_data(word data_in, int block_offset);	// sets one word of data in the block
	word get_data(int block_offset);				// returns one word of data from the block

	bool is_dirty();				// returns dirty bit value (not encapsulated enough?)
	void set_dirty(bool dirty_in);	// sets dirty bit

};

class set
{
private:
	std::list<block> blocks;

public:

	set(int set_size, int block_size);

	bool read(int block_offset, int address_tag, word& data, bool& dirty); // reads from block if hit
	bool write(int block_offset, int address_tag, word data, bool& dirty); // writes to block if hit (will set dirty to true)

	void set_data(int address_tag, word* data, int block_size); // sets data of block and removes LRA block (will set dirty to false)
	void get_data(word* data, int block_size);					// gets data of block at end of list
	bool get_dirty_data(int& address_tag, word* data, int block_size, int block_number); // gets data from block in the set, if the data is dirty (used for flushing)

	int write_back_address(int index, int cache_size, int block_size, int word_size);			// returns RAM address used for write back procedure (using last block in set)
	int write_back_address(int index, int cache_size, int block_size, int word_size, int tag);	// returns RAM address used for write back procedure (using inputted tag - for flushing)

};

// address is broken down as follows:
	// TAG-----INDEX---BLOCK OFFSET-BYTE OFFSET
	// TAG is used for validating
	// INDEX is used for finding the correct set & block
		// set is searched until block with correct TAG is found
	// BLOCK OFFSET is used for finding the correct word inside the block
	// BYTE OFFSET is used for finding the correct byte inside the block
		// only used in read/write byte: cache sim doesn't deal with this, therefore this is effectively ignored.

class cache
{
private:
		// parameters:
	unsigned RAM_size;	// words of RAM

	unsigned word_size;	// bytes per word
	unsigned block_size;	// words per block
	unsigned set_size;	// blocks per set
	unsigned cache_size; // sets per cache

	int hit_time;	// cycles per hit
	int read_time;	// cycles per block
	int write_time;	// cycles per block

		// memory:
	std::vector<word> RAM;		// the RAM: size defined in constructor
	std::vector<set> cache_mem;	// pointer to array of sets

	// each move a block of between RAM and cache.
	void write_back(int address, word* data);
	void read_in(int address, word* data);

public:

	cache(unsigned addr_bits, unsigned words_in, unsigned blocks_in, unsigned sets_in, unsigned size_in, int htime, int rtime, int wtime);

	// read and write read a single word of data from a block.
		// a cache read attempt happens first
		// if it misses, then the data is written back if necessary and then loaded from RAM
	int read(int address, word& data);
	int write(int address, word data);

	// flush checks every valid block in the cache, and writes back if data is dirty.
	int flush();

	// debug returns every valid memory address in the cache.
	//void debug(std::vector<int>& set, std::vector<word>& data);

};

#endif
