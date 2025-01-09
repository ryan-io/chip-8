#pragma once
#include <array>
#include <fstream>
#include <iostream>

/// Reference: https://austinmorlan.com/posts/chip8_emulator/#16-8-bit-registers

namespace Emulation
{

#pragma region Definitions

	constexpr unsigned MemoryFootPrint = 4096;
	/// Chip-8 has 4K memory
	///	0x000-0x1FF: Originally reserved for the CHIP-8 interpreter,
	///	We will not write to this memory except for a few locations:
	///	0x050-0x0A0: 16 characters for the hexadecimal font set, 0-F
	///		ROMs will look for these characters in this location
	///	0x200-0xFFF: ROM instructions stored starting at 0x200
	///		Anything after the memory used in 0x200-0x*** is free for use
	using Memory = std::array<uint8_t, MemoryFootPrint>;

	/// Chip-8 has 16-bit index register
	///	The largest instruction memory address is 0xFFF
	///	hence the need for this register to be 16bits
	using IndexRegister = uint16_t;

	/// Register to hold the address of the next instruction to run (must be able to hold 0xFFF)
	using ProgramCounter = uint16_t;

	using StackPointer = uint8_t;
	/// Stack is used to store the address that the interpreter
	///	This class will maintain a stack pointer
	using Stack = std::array<uint16_t, 16>;

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
		{
			// Initialize the program counter to the root register address
			_programCounter = ROOT_REGISTER_ADDRESS;
		}

		/// <summary>
		/// Default destructor
		/// </summary>
		~Chip8 () = default;

		/// <summary>
		/// Load ROM into memory
		/// Includes all the instructions for the game
		/// </summary>
		void LoadROM (const char *path)
		{
			// open a binary file (std::ios::binary), place the file pointer at the end of the file (std::ios::ate)
			// ROMs are assumed to be binary files
			std::ifstream stream = std::ifstream (path, std::ios::binary | std::ios::ate);

			if (!stream.is_open ())
			{
				// TODO: Error handling
				std::cout << "Could not open stream.\n";
				return;
			}

			// this gets the size of the file (this is really the current position of the file pointer)
			// since the stream is opened with std::ios::ate, the file pointer is at the end of the file
			// this gives us the size of the file
			std::streampos contentSize = stream.tellg ();

			if (contentSize <= 4096)
			{
				// go back to the beginning of the file
				stream.seekg (0, std::ios::beg);
				// fill the buffer, this is one of the very very limited appropriate use cases for reinterpret_cast
				stream.read (reinterpret_cast<char *>(&_memory), contentSize);
				std::cout << "Loaded ROM: " << path << std::endl;
			}
			else
			{
				std::cout << "ROM is too large to load into memory.\n";
			}
		}

	private:
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
	};

#pragma endregion

}