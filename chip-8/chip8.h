#pragma once
#include <array>
#include <fstream>
#include <iostream>
#include <random>
#include <chrono>

/// Reference: https://austinmorlan.com/posts/chip8_emulator/#16-8-bit-registers

namespace Emulation
{

#pragma region Definitions

	constexpr unsigned MEMORY_FOOTPRINT = 4096;
	/// Chip-8 has 4K memory
	///	0x000-0x1FF: Originally reserved for the CHIP-8 interpreter,
	///	We will not write to this memory except for a few locations:
	///	0x050-0x0A0: 16 characters for the hexadecimal font set, 0-F
	///		ROMs will look for these characters in this location
	///	0x200-0xFFF: ROM instructions stored starting at 0x200
	///		Anything after the memory used in 0x200-0x*** is free for use
	using Memory = std::array<uint8_t, MEMORY_FOOTPRINT>;

	/// Chip-8 has 16-bit index register
	///	The largest instruction memory address is 0xFFF
	///	hence the need for this register to be 16bits
	using IndexRegister = uint16_t;

	/// Register to hold the address of the next instruction to run (must be able to hold 0xFFF)
	using ProgramCounter = uint16_t;

	constexpr uint8_t STACK_SIZE = 16;
	using StackPointer = uint8_t;
	/// Stack is used to store the address that the interpreter
	///	This class will maintain a stack pointer
	using Stack = std::array<uint16_t, STACK_SIZE>;

	/// Chip-8 has 35 opcodes
	/// Each opcode is 2 bytes long
	///	Per https://austinmorlan.com/posts/chip8_emulator/#16-8-bit-registers
	///		An instruction is two bytes but memory is addressed as a single byte, so
	///		when we fetch an instruction from memory we need to fetch a byte from PC
	///		and a byte from PC+1 and connect them into a single value.
	using OpCode = uint16_t;

	// Special timer register
	// Timers are relatively small and only need to hold values 0-255
	using TimerRegister = uint8_t;

	constexpr char const *CHIP8_TITLE = "CHIP-8 Emulator";
	constexpr auto CHIP8_DISPAY_WIDTH = 64;
	constexpr auto CHIP8_DISPLAY_HEIGHT = 32;
	/// Chip-8 has a 64x32 monochrome display
	///	Each pixel is either on or off; only a single bit is required to represent each pixel
	/// There are two types of pixels:
	/// 	* Display Pixel -> what appears on the screen to the user
	/// 	* Sprite Pixel -> what to draw at each pixel
	/// The display is drawn by XORing the sprite pixels with the display pixels
	///     * Sprite Pixel Off XOR Display Pixel Off = Display Pixel Off
	///		* Sprite Pixel Off XOR Display Pixel On = Display Pixel On
	///		* Sprite Pixel On XOR Display Pixel Off = Display Pixel On
	///		* Sprite Pixel On XOR Display Pixel On = Display Pixel Off
	using DisplayMemory = std::array<uint32_t, CHIP8_DISPAY_WIDTH *CHIP8_DISPLAY_HEIGHT>;

	constexpr auto ROOT_REGISTER_ADDRESS = 0x200;
	/// Chip-8 has 16 general purpose registers
	///	These are 8-bit data registers from V0 to VF (only take into account 0-F)
	///	Can hold values 0x00 to 0xFF
	///		* VF is used as a flag for some instructions as is considered "special" or a carry flag
	///	Two special registers: index register (I) and program counter (PC)
	///	16-bit index register is used to store memory addresses being used in operations
	///	Program instructions begin at 0x200, 
	using Registers = std::array<uint8_t, 16>;

#pragma endregion

#pragma region Fonts

	constexpr uint8_t FONT_SET_SIZE_BYTES = 80; // 16 characters, 5 bytes each = 80 byte footprint
	constexpr uint8_t FONT_SET_START_ADDRESS = 0x50;
	/// <summary>
	/// ROMs require sixteen characters to allow writing to the screen
	/// These need to be loaded into memory
	/// Each character is 5 bytes
	/// Each bit represents a pixel
	///		* 0 = off
	/// 	* 1 = on
	/// Example: Letter 'F' is represented by 0xF0, 0x80, 0xF0, 0x80, 0x80
	/// </summary>
	constexpr std::array<uint8_t, FONT_SET_SIZE_BYTES> FONT_SET
	{
		0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};

#pragma endregion


#pragma region CHIP-8

	/// Chip-8 emulator
	/// Registers are dedicated locations on a CPU used to store data
	///	These are small, 4096 bytes of memory (0x000 to 0xFFF)
	///	Operations will often involve loading data from memory into these registers
	///	The results are then stored back into memory
	///	The thought process here is all 4kB will act as a single register
	///	Random access memory will be used as little as possible
	class Chip8
	{
	public:
		/// <summary>
		/// Default constructor
		/// </summary>
		Chip8 () 
			// Initialize the program counter to the root register address
			: _programCounter{ ROOT_REGISTER_ADDRESS }, 
			// Initialize the random number generator
			// CHIP-8 has an instruciton that places a random number into a register
			// random number needs to fit in a byte
			// the initialization values (0 & 255) are inclusive
			_randomDistribution { 0U, 255U }
		{
			// Load font set into memory
			for (auto i = 0; i < FONT_SET_SIZE_BYTES; i++)
			{
				_memory[FONT_SET_START_ADDRESS + 1] = FONT_SET[i];
			}
		}

		/// <summary>
		/// Default destructor
		/// </summary>
		~Chip8 () = default;

		/// <summary>
		/// Load ROM into memory
		/// Includes all the instructions for the game
		/// Throws:
		///		* RomNoDataError -> corrupted file
		/// 	* RomTooLargeError -> file too large
		/// </summary>
		void LoadROM (const char *path)
		{
			// open a binary file (std::ios::binary), place the file pointer at the end of the file (std::ios::ate)
			// ROMs are assumed to be binary files
			std::ifstream stream = std::ifstream (path, std::ios::binary | std::ios::ate);

			if (!stream.is_open ())
			{
				// TODO: error handling
				std::cout << "Could not open stream.\n";
				return;
			}

			try
			{
				// this gets the size of the file (this is really the current position of the file pointer)
				// since the stream is opened with std::ios::ate, the file pointer is at the end of the file
				// this gives us the size of the file
				std::streampos contentSize = stream.tellg ();

				if (contentSize <= 0)
				{
					throw RomNoDataError{};
				}

				if (contentSize > MEMORY_FOOTPRINT)
				{
					throw RomTooLargeError{};
				}

				// go back to the beginning of the file
				stream.seekg (0, std::ios::beg);
				// fill the buffer, this is one of the very very limited appropriate use cases for reinterpret_cast
				stream.read (reinterpret_cast<char *>(&_memory), contentSize);
			}
			catch (const std::exception &e)
			{
				if (stream.is_open ())
				{
					stream.close ();
				}

				throw e;
			}
		}

	private:
		/// <summary>
		/// Returns a value between 0-255
		/// </summary>
		/// <returns>Unsigned integer between 0 and 255</returns>
		uint8_t GetRandomByte ()
		{
			return _randomDistribution (_mersenneTwister);
		}

		Memory _memory{};

		Registers _registers{};

		IndexRegister _indexRegister{};

		ProgramCounter _programCounter{};

		StackPointer _stackPointer{};

		Stack _stack{};

		DisplayMemory _displayMemory{};

		TimerRegister _delayTimer{};

		TimerRegister _soundTimer{};

		OpCode _opcode{};

	#pragma region Random Number Generation

		// https://www.learncpp.com/cpp-tutorial/generating-random-numbers-using-mersenne-twister/
		// Mersenne Twister random number generator
		std::mt19937 _mersenneTwister{ static_cast<std::mt19937::result_type>(std::chrono::steady_clock::now ().time_since_epoch ().count ()) };

		// uniform distribution to put a constraint on the range of numbers generated
		// again, reference: https://www.learncpp.com/cpp-tutorial/generating-random-numbers-using-mersenne-twister/
		std::uniform_int_distribution<uint8_t> _randomDistribution{};

	#pragma endregion

	#pragma region Errors

		class RomNoDataError : public std::exception
		{
		public:
			const char *what () const noexcept override
			{
				return "No data was loaded from the ROM.";
			}
		};

		class RomTooLargeError : public std::exception
		{
			const char *what () const noexcept override
			{
				return "The ROM is too large to fit in memory.";
			}
		};

	#pragma endregion

	};

#pragma endregion

}