#pragma once

#include "image.hpp"
#include "scanner.hpp"

namespace PNG
{

// Resets the file pointer after confirming if it is a png file
bool is_PNG_file(Scanner &file);
Image PNG_Try_Parse(Scanner &file);

} // namespace PNG