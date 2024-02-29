#include <string>
#include "cmdline.h"
#include <LoadJpeg.h>
#include <JpgToHeic.h>
int main(int argc, char** argv)
{
	cmdline::parser a;
	a.add<std::string>("input_filename", 'i', "input file name", true, "");
	a.add<std::string>("output_filename", 'o', "output file name", true, "");
	a.add<int>("quality", 'q', "output quality", false, 50, cmdline::range(0, 100));
	a.parse_check(argc, argv);

	int quality = a.get<int>("quality");
	std::string input_filename = a.get<std::string>("input_filename");
	std::string output_filename = a.get<std::string>("output_filename");

	JpgToHeic(input_filename, output_filename, quality);

	return 0;
}