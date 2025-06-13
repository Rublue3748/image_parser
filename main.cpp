#include "png.hpp"
#include "scanner.hpp"
#include <fstream>

int main()
{
    Scanner current_file("images.png");

    if (PNG::is_PNG_file(current_file))
    {
        Image img = PNG::PNG_Try_Parse(current_file);
        std::ofstream output("output.img");
        output << img;
        output.close();
    }
    return 0;
}