#include <iostream>

#include "SGBD.h"

int main(int argc, char **argv)
{
	try
	{
		SGBD(argc, argv).Run();
	}
	catch(const std::exception &e)
	{
		std::cout << e.what() << std::endl;
		return 1;
	}

	return 0;
}