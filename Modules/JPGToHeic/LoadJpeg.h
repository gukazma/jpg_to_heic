#pragma once
#include <memory>
#include <libheif/heif.h>
std::shared_ptr<heif_image> loadJPEG(const char* filename);