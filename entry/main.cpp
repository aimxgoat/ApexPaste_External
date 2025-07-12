#include "main.h"
#include <thread>
#include <iostream>
#include <Windows.h>

auto init() -> bool
{
	if (!mem::get_driver()) {
		std::cerr << ("Driver Failed") << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(5));
		return 1;
	}

	int result = MessageBox(
		NULL,
		("Ensure you are in firing range before clicking OK. This will initialize the cheat functions."),
		("AIMXEX Initialization"),
		MB_OK | MB_ICONINFORMATION
	);
	if (result != IDOK) return 1;

	mem::process_id = mem::get_process(("r5apex_dx12.exe"));
	if (mem::process_id == 0) {
		mem::process_id = mem::get_process(("r5apex_dx12.exe"));
	}

	mem::CR3();
	baseAddress = mem::find_base_address();

	std::cout << "BASE ADDR: 0x" << std::hex << baseAddress << std::endl;

	setupoverlay();

	std::thread player_thread(player_reading_work);
	std::thread glow_thread(glow_loop);
	render_loop();

	player_thread.join();
	glow_thread.join();

	return true;
}

auto main() -> int
{
	SetConsoleTitleA("gurt: sybau Private. apex external, @vrailn");

	cout << "gurt: sybau private " << endl;

	if (!init())
	{

		cout << "<!> failed to gurt: sybau" << endl;
		Sleep(3000);
		exit(0);
	}

}