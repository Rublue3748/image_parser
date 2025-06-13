#pragma once

#include "image.hpp"
#include "scanner.hpp"

// Resets the file pointer after confirming if it is a png file
namespace PNG
{

bool is_PNG_file(Scanner &file);
Image PNG_Try_Parse(Scanner &file);

} // namespace PNG