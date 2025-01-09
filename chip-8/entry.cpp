#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>

#include "raylib.h"
#include "chip8.h"


int main (int argc, char *argv[])
{
	using namespace	Emulation;
	
	Chip8 chip8{};
	chip8.LoadROM ("specification.txt");
	Vector2 position = { 800/2, 400/2};

	// Initialization
	InitWindow (800, 400, Emulation::CHIP8_TITLE);

	SetTargetFPS (60);               

	// Main loop
	while (!WindowShouldClose ())    
	{
		// NOTE: pixel indices go from left to right, top to bottom
		// Update
		if (IsKeyDown (KEY_D)) position.x += 2.0f;
		if (IsKeyDown (KEY_A)) position.x -= 2.0f;
		if (IsKeyDown (KEY_W)) position.y -= 2.0f;
		if (IsKeyDown (KEY_S)) position.y += 2.0f;

		// Draw
		BeginDrawing ();

		ClearBackground (RAYWHITE);
		DrawCircleV (position, 50, MAROON);

		EndDrawing ();
	}

	return 0;
}