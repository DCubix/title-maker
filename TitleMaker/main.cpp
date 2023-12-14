#include <iostream>
#include "app/App.h"

int main(int argc, char** argv) {
	return (new App())->start(argc, argv);
}
