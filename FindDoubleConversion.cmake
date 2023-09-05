
find_path(DOUBLE_CONVERSION_INCLUDE_DIR /double-conversion)
find_library(DOUBLE_CONVERSION_LIBRARY NAMES double-conversion)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  double-conversion
  DEFAULT_MSG
  DOUBLE_CONVERSION_LIBRARY DOUBLE_CONVERSION_INCLUDE_DIR)
