find_package(libheif CONFIG REQUIRED)

find_package(GDAL REQUIRED)
find_package(JPEG REQUIRED)

add_subdirectory(Vendors)
add_subdirectory(Runtime)
add_subdirectory(App)